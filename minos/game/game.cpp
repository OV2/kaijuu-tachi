#include <minos.hpp>
#include <game/game.hpp>
#include <ramus/hash/md5.hpp>
#include <ramus/expand-path.hpp>
#include <ramus/shift-jis.hpp>

Markup::Node Game::settings;

#include "famicom.cpp"
#include "super-famicom.cpp"
#include "nintendo-64.cpp"
#include "mega-drive.cpp"
#include "nintendo-ds.cpp"

Game::Game(string pathName) {
  Game::initializeSettings();

  name = Location::base(pathName);
  type = Location::suffix(name).trimLeft(".", 1L);
  isFile = !directory::exists(pathName);
  path = pathName;
  if(!isFile) {
    path = path.append("/");
    manifest = loadManifest(path);
  }

  systemSettings = Game::settings.find({"system(ext=", type, ")"})(0);

  string temporaryRomType = systemSettings ? systemSettings["rom/ext"].text() : type;
  extensionSettings = Game::settings.find({"extension(name=", temporaryRomType, ")"})(0);
}

auto Game::play(string name) -> void {
  setEmulator(name);
  _play();
}

auto Game::play() -> void {
  setEmulator();
  _play();
}

//static
auto Game::initializeSettings() -> void {
  if(Game::settings["extension"]) return;
  Game::settings = BML::unserialize(file::read({Path::program(), "settings.bml"}));
}

auto Game::loadManifest(string path) -> Markup::Node {
  string manifestPath = {path, "manifest.bml"};
  if(file::exists(manifestPath)) {
    return BML::unserialize(file::read(manifestPath));
  } else if(Game::settings[{"icarus/supports/", type}]) {
    string icarus = Game::settings["icarus/path"].text();
    if(!file::exists(icarus)) {
      MessageDialog().setTitle("minos").setText({
        "Could not find icarus at path:\n",
        icarus, "\n",
        "Please check your path in settings.bml."
      }).error();
      throw;
    }
    if(auto manifest = execute(icarus, "--manifest", path)) {
      return BML::unserialize(manifest.output);
    }
  }
  throw;
}

auto Game::setEmulator() -> void {
  setEmulator(extensionSettings["emulator"].text());
}

auto Game::setEmulator(string name) -> void {
  emulatorName = name;

  emulatorSettings = Game::settings.find({"emulator(name=", emulatorName, ")"})(0);

  string temporaryRomType = systemSettings ? systemSettings["rom/ext"].text() : type;
  emulatorExtensionSettings = Game::settings.find({"emulator(name=", emulatorName, ")/", temporaryRomType})(0);
}

auto Game::emulatorSettingNode(string name) -> Markup::Node {
  for(const Markup::Node node : {emulatorExtensionSettings, emulatorSettings, extensionSettings}) {
    if(auto result = node.find(name)) return result(0);
  }
  return Markup::Node{};
}

auto Game::emulatorSetting(string name) -> string {
  return emulatorSettingNode(name).text();
}

auto Game::emulatorRamName(string ext) -> string {
  string ramNameFormat = emulatorSetting("ram-name-format");
  if(!ramNameFormat) return {Location::prefix(name), ".", ext};
  if(ramNameFormat.find("<name>")) {
    ramNameFormat.replace("<name>", Location::prefix(name));
  }
  if(ramNameFormat.find("<ext>")) {
    ramNameFormat.replace("<ext>", ext);
  }
  if(ramNameFormat.find("<internal>")) {
    ramNameFormat.replace("<internal>", romInternalName().trimRight(" "));
  }
  if(ramNameFormat.find("<md5>")) {
    file::read(temporaryRomPath);
    ramNameFormat.replace("<md5>", ramus::Hash::MD5(file::read(temporaryRomPath)).digest());
  }
  if(ramNameFormat.find("<fullstop-bug>")) {
    string baseName = Location::prefix(name);
    uint index = -1;
    for(uint i = 0; i <= baseName.size(); i++) {
      if(baseName[i] == '.') { index = i; break; }
    }
    if(index >= 0) ramNameFormat.replace("<fullstop-bug>", slice(baseName, 0, index));
    else ramNameFormat.replace("<fullstop-bug>", Location::prefix(name));
  }
  return ramNameFormat;
}

auto Game::romInternalName() -> string {
  if(type == "n64") return n64_romInternalName();
  return Location::prefix(name);
}

auto Game::validate() -> bool {
  if(isFile) return true;
  uint roms = 0;
  for(auto node : systemSettings.find("rom/*")) {
    if(node.name() == "query") {
      if(auto romNode = manifest.find(node.text())(0)) {
        if(file::exists({path, romNode["name"].text()})) {
          roms++;
        } else {
          error = {"Could not find ROM \"", romNode["name"].text(), "\"!"};
          return false;
        }
      }
    }
  }
  if(roms == 0) {
    error = "Could not find any valid ROM nodes in the manifest!";
    return false;
  }
  return true;
}

auto Game::depurify() -> void {
  promptSlots();
  depurifyRom();
  depurifyRam();
  depurifyStates();
}

auto Game::promptSlots() -> void {
  for(auto slotNode : systemSettings.find("slot")) {
    shared_pointer<Slot> slot{new Slot};
    slots.append(slot);

    if(!slotNode["query"]) continue;

    string requiredQuery = slotNode["query"].text();
    if(!manifest.find(requiredQuery)(0)) continue;

    string slotPath = BrowserDialog()
    .setTitle("Load Slotted Cartridge")
    .setPath(Location::dir(path))
    .setFilters(string{"Slotted Cartridge|*.", slotNode["ext"].text()})
    .openFolder();
    if(!slotPath) continue;

    slot->active = true;
    slot->path = slotPath;
    slot->manifest = loadManifest(slotPath);
  }
}

auto Game::depurifyRom() -> void {
  if(isFile) { temporaryRomPath = path; return; }
  temporaryRomPath = {
    path,
    Location::prefix(name), ".", extensionSettings["name"].text()
  };

  vector<Markup::Node> romPartNodes;
  if(systemSettings) {
    for(auto node : systemSettings.find("rom/*")) {
      if(node.name() == "ext") continue;
      if(node.name() == "query" && !manifest.find(node.text())(0)) continue;
      if(node.name() == "slot" && !slots[node["index"].natural()]->active) continue;
      if(node.name() == "special" && node["required-query"]) {
        if(!manifest.find(node["required-query"].text())(0)) continue;
      }
      romPartNodes.append(node);
    }
    compositeRom = romPartNodes.size() > 1;
  } else {
    compositeRom = false;
  }

  if(compositeRom) {
    file temporaryRom(temporaryRomPath, file::mode::writeread);

    string romName;
    auto slotNodes = systemSettings.find("slot");
    for(auto node : romPartNodes) {
      if(node.name() == "query") {
        romName = manifest.find(node.text())(0)["name"].text();
        appendFile(temporaryRom, {path, romName});
      } else if(node.name() == "slot") {
        uint index = node["index"].natural();
        auto slot = slots[index];
        #define slotRom(comp)\
          if(type == #comp)\
            comp##_slotRom(slotNodes[index], *slot, temporaryRom, manifest)
        slotRom(md);
        slotRom(sfcb);
        #undef slotRom
      } else if(node.name() == "special") {
        #define romSpecial(comp)\
          if(type == #comp)\
            comp##_romSpecial(node.text(), temporaryRom, manifest)
        romSpecial(fc);
        romSpecial(vs);
        romSpecial(pc10);
        romSpecial(nss);
        #undef romSpecial
      }
    }
    temporaryRom.close();
  } else {
    string romQuery = romPartNodes[0].text();
    romPath = {path, manifest.find(romQuery)(0)["name"].text()};
    file::move(romPath, temporaryRomPath);
  }
}

auto Game::depurifyRam() -> void {
  string emulatorRamPath = emulatorSetting("ram-path");
  if(!emulatorRamPath) emulatorRamPath = path;
  emulatorRamPath = ramus::expandPath(emulatorRamPath).trimRight("/").append("/");

  for(auto settingsRamNode : systemSettings.find("ram")) {
    RAMPaths ram;
    string ext = settingsRamNode["ext"].text();
    string pureRamFilename = emulatorRamName(ext);

    //Emulator RAM extension override
    ram.settings = settingsRamNode;
    if(auto node = emulatorSettingNode({"ram(sub=", ext, ")"})) {
      ram.settings = node;
      ext = ram.settings["ext"].text();
    }
    string impureRamFilename = emulatorRamName(ext);

    ram.temporaryPath = {emulatorRamPath, impureRamFilename};

    if(!isFile) {
      Markup::Node ramPartNode;
      for(auto node : ram.settings.find("*")) {
        if(node.name() == "ext") continue;
        if(node.name() == "query") {
          if(!manifest.find(node.text())(0)) continue;
          ramPartNode = node;
          break;
        }
        if(node.name() == "slot") {
          uint index = node["index"].natural();
          auto slot = slots[index];
          if(!slot->active) continue;
          string slotExt = Location::suffix(slot->path).trimLeft(".");
          auto slotSettings = Game::settings.find({"system(ext=", slotExt, ")"})(0);
          if(!slot->manifest.find(slotSettings["ram/query"].text())(0)) continue;
          ramPartNode = node;
          break;
        }
      }
      if(!ramPartNode) continue;

      if(ramPartNode.name() == "query") {
        string ramQuery = ramPartNode.text();
        auto manifestRamNode = manifest.find(ramQuery)(0);
        //"volatile" node suppresses generation of RAM files.
        if(manifestRamNode && manifestRamNode["name"] && !manifestRamNode["volatile"]) {
          ram.path = {path, manifestRamNode["name"].text()};
          ramPaths.append(ram);
        }
      } else if(ramPartNode.name() == "slot") {
        uint index = ramPartNode["index"].natural();
        auto slot = slots[index];
        string slotExt = Location::suffix(slot->path).trimLeft(".");
        auto slotSettings = Game::settings.find({"system(ext=", slotExt, ")"})(0);
        auto slotRamNode = slot->manifest.find(slotSettings["ram/query"].text())(0);
        if(slotRamNode && slotRamNode["name"] && !slotRamNode["volatile"]) {
          ram.path = {slot->path, slotRamNode["name"].text()};
          ramPaths.append(ram);
        }
      }
    } else {
      ram.path = {Location::dir(path), pureRamFilename};
      ramPaths.append(ram);
    }
  }

  for(auto ram : ramPaths) {
    directory::create(Location::path(ram.temporaryPath));
    if(Location::suffix(ram.temporaryPath) == ".dsv") {
      depurifyDsv(ram);
    } else {
      file::move(ram.path, ram.temporaryPath);
    }
  }
}

auto Game::depurifyStates() -> void {
  if(isFile) return;
  string statePath = emulatorSetting("state-path");
  string stateNameFormat = emulatorSetting("state-name");
  if(!statePath || !stateNameFormat) return;

  string gamepakStateDir = {path, emulatorName, "/"};
  string emulatorStateDir = statePath.trimRight("/").append("/");

  string pureExt = emulatorSetting("pure-state-extension");
  string stateExtension = pureExt ? pureExt : stateNameFormat.split(".").right();

  if(directory::exists(gamepakStateDir)) {
    string gamepakStatePath;
    string emulatorStateName;
    int ordinal;
    bool doNotDelete = false;

    string ordinalToken = "<ordinal:1>";
    for(string gamepakStateName : directory::files(gamepakStateDir)) {
      if(gamepakStateName.match({"state-?*.", stateExtension})) {
        ordinal = gamepakStateName.split("-")[1].split(".")[0].natural();
        gamepakStatePath = {gamepakStateDir, gamepakStateName};
        emulatorStateName = string{stateNameFormat}.
          replace("<name>", Location::prefix(name)).
          replace(ordinalToken, ordinal);
        file::move(
          {gamepakStateDir, gamepakStateName},
          {emulatorStateDir, emulatorStateName}
        );
      } else {
        doNotDelete = true;
      }
    }
    if(!doNotDelete) directory::remove(gamepakStateDir);
  }
}

auto Game::appendFile(file& destination, string sourcePath) -> void {
  file source(sourcePath, file::mode::read);
  uint size = source.size();
  uint8_t* buffer = new uint8_t[size];
  source.read(buffer, size);
  destination.write(buffer, size);
  source.close();
  delete[] buffer;
};

auto Game::appendFile(file& destination, string sourcePath, uint start, uint length) -> void {
  file source(sourcePath, file::mode::read);
  uint8_t* buffer = new uint8_t[length];
  source.seek(start);
  source.read(buffer, length);
  destination.write(buffer, length);
  source.close();
  delete[] buffer;
};

auto Game::repurify() -> void {
  repurifyRom();
  repurifyRam();
  repurifyStates();
}

auto Game::repurifyRom() -> void {
  if(isFile) return;
  if(compositeRom) file::remove(temporaryRomPath);
  else             file::move(temporaryRomPath, romPath);
}

auto Game::repurifyRam() -> void {
  for(auto ram : ramPaths) {
    if(Location::suffix(ram.temporaryPath) == ".dsv") {
      repurifyDsv(ram);
    } else {
      file::move(ram.temporaryPath, ram.path);
    }
    string ramNameFormat = emulatorRamName(ram.settings["ext"].text());
    if(ramNameFormat.find("/")) {
      string path = Location::path(ram.temporaryPath);
      if(!directory::contents(path)) directory::remove(path);
    }
  }
  ramPaths.reset();
}

auto Game::repurifyStates() -> void {
  if(isFile) return;
  string statePath = emulatorSetting("state-path");
  string stateNameFormat = emulatorSetting("state-name");
  if(!statePath || !stateNameFormat) return;

  string gamepakStateDir = {path, emulatorName, "/"};
  string emulatorStateDir = statePath.trimRight("/").append("/");

  string pureExt = emulatorSetting("pure-state-extension");
  string stateExtension = pureExt ? pureExt : stateNameFormat.split(".").right();

  string_vector stateNames = directory::files(emulatorStateDir);
  bool directoryExists = directory::exists(gamepakStateDir);
  string nameTemplate = string{stateNameFormat}.replace("<name>", Location::prefix(name));

  string ordinalToken = "<ordinal:1>";
  uint prefixLength = nameTemplate.find(ordinalToken)();
  uint suffixLength = nameTemplate.length() - prefixLength - ordinalToken.length();

  string prefix = slice(nameTemplate, 0, prefixLength);
  string suffix = slice(nameTemplate, nameTemplate.length() - suffixLength, suffixLength);
  string ordinal;
  for(string emulatorStateName : stateNames) {
    if(!emulatorStateName.beginsWith(prefix) || !emulatorStateName.endsWith(suffix)) continue;
    ordinal = slice(emulatorStateName,
      prefixLength,
      emulatorStateName.length() - prefixLength - suffixLength
    );
    if(!directoryExists) {
      directory::create(gamepakStateDir);
      directoryExists = true;
    }
    file::move(
      {emulatorStateDir, emulatorStateName},
      {gamepakStateDir, "state-", ordinal, ".", stateExtension}
    );
  }
}

auto Game::_play() -> void {
  if(!isFile && !validate()) {
    MessageDialog().setTitle("minos").setText({
      error, "\n",
      "Please review the manifest and the ROMs."
    }).error();
    return;
  }
  depurify();

  string applicationPath = emulatorSettings["path"].text();
  string filePath = temporaryRomPath;

  #if defined(PLATFORM_WINDOWS)
  applicationPath = applicationPath.transform("/", "\\");
  filePath = filePath.transform("/", "\\");
  #endif

  auto result = execute(applicationPath, filePath);
  if(!result) {
    MessageDialog().setTitle("minos").setText({
      "Error ", result.code, ": Could not launch ", emulatorName, "!\n"
    }).error();
  }

  repurify();
}

#include <minos.hpp>
#include <game/game.hpp>
#include <ramus/hash/md5.hpp>
#include <ramus/expand-path.hpp>
#include <ramus/run.hpp>
#include <ramus/shift-jis.hpp>

Markup::Node Game::settings;

#include "system/famicom.cpp"
#include "system/super-famicom.cpp"
#include "system/nintendo-64.cpp"
#include "system/mega-drive.cpp"
#include "system/nintendo-ds.cpp"

#include "purify.cpp"

Game::Game(const string& pathName) {
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

auto Game::play(const string& name) -> void {
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

auto Game::loadManifest(const string& path) -> Markup::Node {
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

auto Game::setEmulator(const string& name) -> void {
  emulatorName = name;

  emulatorSettings = Game::settings.find({"emulator(name=", emulatorName, ")"})(0);

  string temporaryRomType = systemSettings ? systemSettings["rom/ext"].text() : type;
  emulatorExtensionSettings = Game::settings.find({"emulator(name=", emulatorName, ")/", temporaryRomType})(0);
}

auto Game::emulatorSettingNode(const string& name) -> Markup::Node {
  for(const Markup::Node& node : {emulatorExtensionSettings, emulatorSettings, extensionSettings}) {
    if(auto result = node.find(name)) return result(0);
  }
  return Markup::Node{};
}

auto Game::emulatorSetting(const string& name) -> string {
  return emulatorSettingNode(name).text();
}

auto Game::emulatorRamName(const string& ext) -> string {
  string ramName = "";
  for(string query : {"ram-name-format", "ram-name-format/fallback"}) {
    string ramNameFormat = emulatorSetting(query);
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
    if(ramNameFormat.find("<goodset>")) {
      string goodsetName = romGoodsetName();
      if(!goodsetName) continue;
      ramNameFormat.replace("<goodset>", goodsetName);
    }
    if(ramNameFormat.find("<md5>")) {
      file::read(temporaryRomPath);
      ramNameFormat.replace("<md5>", ramus::Hash::MD5(file::read(temporaryRomPath)).digest());
    }
    if(ramNameFormat.find("<fullstop-bug>")) {
      string baseName = Location::prefix(name);
      if(auto index = baseName.find(".")) {
        ramNameFormat.replace("<fullstop-bug>", slice(baseName, 0, index()));
      } else {
        ramNameFormat.replace("<fullstop-bug>", Location::prefix(name));
      }
    }
    ramName = ramNameFormat;
    break;
  }

  return ramName;
}

auto Game::romInternalName() -> string {
  static string internalName;
  if(!internalName) {
    if(type == "n64") internalName = n64_romInternalName();
    else              internalName = Location::prefix(name);
  }
  return internalName;
}

auto Game::romGoodsetName() -> string {
  static string goodsetName;
  if(!goodsetName) {
    if(type == "n64") goodsetName = n64_romGoodsetName();
    else              goodsetName = Location::prefix(name);
  }
  return goodsetName;
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

  if(!file::exists(applicationPath)) {
    MessageDialog().setTitle("minos").setText({
      "Could not find ", emulatorName, "!\n"
      "Expected path: ", applicationPath
    }).error();
  }
  ramus::looseExecute(applicationPath, filePath);

  repurify();
}

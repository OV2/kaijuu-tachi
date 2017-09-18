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

auto Game::appendFile(file& destination, const string& sourcePath) -> void {
  file source(sourcePath, file::mode::read);
  uint size = source.size();
  uint8_t* buffer = new uint8_t[size];
  source.read(buffer, size);
  destination.write(buffer, size);
  source.close();
  delete[] buffer;
};

auto Game::appendFile(file& destination, const string& sourcePath, uint start, uint length) -> void {
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

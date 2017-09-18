Markup::Node Game::nes20Settings;

auto Game::fc_romSpecial(
  const string& specialType,
  file& temporaryRom,
  Markup::Node& manifest
) -> void {
  if(specialType == "nes20") {
    writeNes20Header(temporaryRom, manifest);
  }
  if(specialType == "undersize-prg") {
    //Fill in the PRG if it is smaller than a 4KB bank
    //Required for Galaxian
    uint romSize = manifest["board/prg/rom/size"].natural();
    if(romSize & 0x3fff) {
      file rom({path, manifest["board/prg/rom/name"].text()}, file::mode::read);
      uint8_t buffer[romSize];
      rom.read(buffer, romSize);
      uint size = romSize;
      while(size & 0x3fff) {
        temporaryRom.write(buffer, romSize);
        size += romSize;
      }
      rom.close();
    }
  }
}

auto Game::vs_romSpecial(
  const string& specialType,
  file& temporaryRom,
  Markup::Node& manifest
) -> void {
  if(specialType == "nes20") {
    writeNes20Header(temporaryRom, manifest);
  }
}

auto Game::pc10_romSpecial(
  const string& specialType,
  file& temporaryRom,
  Markup::Node& manifest
) -> void {
  fc_romSpecial(specialType, temporaryRom, manifest);
  if(specialType == "key16") {
    temporaryRom.seek(temporaryRom.size() - 1);
    uint8_t lastByte = temporaryRom.readl(1);
    temporaryRom.seek(temporaryRom.size());
    temporaryRom.writel(lastByte, 1);
    temporaryRom.writel(0x00, 1);
    temporaryRom.writel(0x00, 1);
    temporaryRom.writel(lastByte, 1);
    temporaryRom.writel(lastByte, 1);
    temporaryRom.writel(0x00, 1);
    temporaryRom.writel(0x00, 1);
  }
  if(specialType == "counter-out") {
    temporaryRom.writem(0x00000000, 4);
    temporaryRom.writem(0xffffffff, 4);
    temporaryRom.writem(0x00000000, 4);
    temporaryRom.writem(0xffffffff, 4);
  }
}

auto Game::writeNes20Header(file& temporaryRom, Markup::Node& manifest) -> void {
  if(!Game::nes20Settings["board"].text()) {
    Game::nes20Settings = BML::unserialize(file::read({Path::program(), "nes20.bml"}));
  }

  const uint8_t signature[] = "NES\x1A";
  temporaryRom.write(signature, 4U);

  bool isVs = !!manifest["side"];
  uint vsActiveSideIndex = 0;
  Markup::Node vsActiveSide;
  if(isVs) {
    auto side = manifest.find("side");
    vsActiveSideIndex = side(0)["ppu"] ? 0 : side(1)["ppu"] ? 1 : 0;
    vsActiveSide = side(vsActiveSideIndex);
  }

  string prgQuery;
  string chrQuery;
  if(!isVs) {
    prgQuery = "board/prg";
    chrQuery = "board/chr";
  } else {
    prgQuery = {"side[", vsActiveSideIndex, "]/prg"};
    chrQuery = {"side[", vsActiveSideIndex, "]/chr"};
  }
  auto prg = manifest.find(prgQuery)(0);
  auto chr = manifest.find(chrQuery)(0);
  uint prgromSize = prg["rom/size"].natural();
  uint chrromSize = chr["rom/size"].natural();
  uint prgramBatterySize = 0;
  uint prgramVolatileSize = isVs ? 0x800 : 0;
  for(auto ramNode : prg.find("ram")) {
    uint ramSize = ramNode["size"].natural();
    if(ramNode["volatile"]) prgramVolatileSize += ramSize;
    else                    prgramBatterySize  += ramSize;
  }
  uint chrramBatterySize = 0;
  uint chrramVolatileSize = 0;
  for(auto ramNode : chr.find("ram")) {
    uint ramSize = ramNode["size"].natural();
    if(ramNode["volatile"]) chrramVolatileSize += ramSize;
    else                    chrramBatterySize  += ramSize;
  }

  //byte 4
  temporaryRom.writel((prgromSize + 0x3fff) >> 14, 1);
  //byte 5
  temporaryRom.writel((chrromSize + 0x1fff) >> 13, 1);

  uint mapper;
  uint submapper;
  bool mirror;
  bool fourScreen;
  if(!isVs) {
    fetchNes20Mapper(manifest, mapper, submapper, mirror, fourScreen);
  } else {
    mapper = 99;
    string chipType = vsActiveSide["chip/type"].text();
    if(chipType == "108"
    || chipType == "109"
    || chipType == "118"
    || chipType == "119") {
      mapper = 206;
    } else if(chipType.match("74??32")) {
      mapper = 2;
    } else if(chipType.match("MMC1*")) {
      mapper = 1;
    }
    submapper = 0;
    mirror = false;
    fourScreen = true;
  }

  //byte 6
  bool battery = prgramBatterySize > 0 || chrramBatterySize > 0;
  bool trainer = false;
  temporaryRom.writel(
    (mirror     << 0   ) |
    (battery    << 1   ) |
    (trainer    << 2   ) |
    (fourScreen << 3   ) |
    (mapper << 4 & 0xf0),
  1);

  //byte 7
  bool isPc10 = !!manifest["board/pc10"];
  temporaryRom.writel(
    (isVs       << 0   ) |
    (isPc10     << 1   ) |
    (0x08              ) |  //NES 2.0
    (mapper << 0 & 0xf0),
  1);

  //byte 8
  temporaryRom.writel(
    (mapper >> 8 & 0x0f) |
    (submapper  << 4   ),
  1);

  //byte 9
  temporaryRom.writel(
    (prgromSize >> 22 & 0x0f) |
    (chrromSize >> 17 & 0xf0),
  1);

  uint volatileBits;
  uint batteryBits;

  //byte A
  volatileBits = prgramVolatileSize ? max(bit::first(bit::round(prgramVolatileSize)), 7) - 6 : 0;
  batteryBits  = prgramBatterySize  ? max(bit::first(bit::round(prgramBatterySize )), 7) - 6 : 0;
  temporaryRom.writel(
    (volatileBits << 0 & 0x0f) |
    (batteryBits  << 4 & 0xf0),
  1);

  //byte B
  volatileBits = chrramVolatileSize ? max(bit::first(bit::round(chrramVolatileSize)), 7) - 6 : 0;
  batteryBits  = chrramBatterySize  ? max(bit::first(bit::round(chrramBatterySize )), 7) - 6 : 0;
  temporaryRom.writel(
    (volatileBits << 0 & 0x0f) |
    (batteryBits  << 4 & 0xf0),
  1);

  //byte C
  bool region = manifest["board/region"].text() == "pal";
  temporaryRom.writel(
    (region << 0),
  1);  //TV region

  //byte D
  uint vsPPU = 0;
  uint vsDRM = 0;
  if(isVs) {
    string ppuRevision = vsActiveSide["ppu/revision"].text();
         if(ppuRevision == "RP2C03B"    ) vsPPU =  0;
    else if(ppuRevision == "RP2C03G"    ) vsPPU =  1;
    else if(ppuRevision == "RP2C04-0001") vsPPU =  2;
    else if(ppuRevision == "RP2C04-0002") vsPPU =  3;
    else if(ppuRevision == "RP2C04-0003") vsPPU =  4;
    else if(ppuRevision == "RP2C04-0004") vsPPU =  5;
    else if(ppuRevision == "RC2C03B"    ) vsPPU =  6;
    else if(ppuRevision == "RC2C03C"    ) vsPPU =  7;
    else if(ppuRevision == "RC2C05-01"  ) vsPPU =  8;
    else if(ppuRevision == "RC2C05-02"  ) vsPPU =  9;
    else if(ppuRevision == "RC2C05-03"  ) vsPPU = 10;
    else if(ppuRevision == "RC2C05-04"  ) vsPPU = 11;
    else if(ppuRevision == "RC2C05-05"  ) vsPPU = 12;
  }
  temporaryRom.writel(
    (vsPPU << 0) |
    (vsDRM << 4),
  1);

  //byte E
  temporaryRom.writel(0x00, 1);  //Reserved
  //byte F
  temporaryRom.writel(0x00, 1);  //Reserved
}

auto Game::fetchNes20Mapper(
  Markup::Node& manifest,
  uint& mapper,
  uint& submapper,
  bool& mirror,
  bool& fourScreen
) -> void {
  string boardId = manifest["board/id"].text();
  Markup::Node boardSettings;
  for(auto boardNode : Game::nes20Settings.find("board")) {
    //Tree Query Language does not work with multiple nodes of the same name
    bool correct = false;
    for(auto idNode : boardNode.find("id")) {
      correct = idNode.text() == boardId;
      if(correct) break;
    }
    if(!correct) continue;
    //If a mirror node exists, but it has no text
    if(!boardNode["mirror"].text()) {
      if(!!manifest["board/mirror"] != !!boardNode["mirror"]) continue;
    }
    auto manifestChip = manifest.find("board/chip");
    auto nodeChip = boardNode.find("chip");
    //Number of chip nodes must match exactly
    if(manifestChip.size() != nodeChip.size()) continue;
    boardSettings = boardNode;
    break;
  }
  if(boardSettings.name() == "board") {
    mapper = boardSettings["mapper"].natural();
    submapper = boardSettings["submapper"] ? boardSettings["submapper"].natural() : 0;
    auto manifestChip = manifest.find("board/chip");
    auto settingsChip = boardSettings.find("chip");
    for(uint i = 0; i < manifestChip.size(); i++) {
      for(auto typeNode : settingsChip(i).find("type")) {
        if(typeNode.text() != manifestChip(i)["type"].text()) continue;
        if(typeNode["mapper"   ]) mapper    = typeNode["mapper"   ].natural();
        if(typeNode["submapper"]) submapper = typeNode["submapper"].natural();
        break;
      }
    }
    if(boardSettings["mirror"]) {
      mirror = boardSettings["mirror"].text() == "vertical"
      || manifest["board/mirror/mode"].text() == "vertical";
    }
    fourScreen = !!boardSettings["four-screen"];
  } else {
    MessageDialog().setTitle("minos").setText({
      "Could not find board ", boardId, "!"
    }).error();
    mapper = 0;
    submapper = 0;
    mirror = false;
    fourScreen = false;
  }
}

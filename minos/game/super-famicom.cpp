auto Game::nss_romSpecial(
  string specialType,
  file& temporaryRom,
  Markup::Node& manifest
) -> void {
  if(specialType == "padding") {
    for(uint i : range(0x6000)) temporaryRom.writel(0x00, 1);
  }
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
}

auto Game::sfcb_slotRom(
  Markup::Node& slotSettings,
  Slot& slot,
  file& temporaryRom,
  Markup::Node& manifest
) -> void {
  string slotType = slotSettings["type"].text();
  if(slotType == "pss6x") {
    auto slotManifest = loadManifest(slot.path);
    auto slotRomName = slotManifest["board/grom/name"].text();
    appendFile(temporaryRom, {slot.path, slotRomName});

    const string queries[] = {"rom", "sa1/rom", "superfx/rom"};
    for(auto sfcbSlot : slotManifest.find("board/slot")) {
      for(string query : queries) {
        if(sfcbSlot[query]) appendFile(temporaryRom, {slot.path, sfcbSlot[string{query, "/name"}].text()});
      }
    }
  }
}

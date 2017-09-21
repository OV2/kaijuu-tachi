auto Game::md_slotRom(
  Markup::Node& slotSettings,
  Slot& slot,
  file& temporaryRom,
  Markup::Node& manifest
) -> void {
  string slotType = slotSettings["type"].text();
  if(slotType == "lock-on") {
    //name = {Location::prefix(name), "+", Location::prefix(subLocation), Location::suffix(name)};
    auto slotManifest = loadManifest(slot.path);
    if(auto slotRomNode = slotManifest["board/rom"]) {
      string slotRomName = slotRomNode["name"].text();
      uint slotRomSize = slotRomNode["size"].natural();
      appendFile(temporaryRom, {slot.path, slotRomName},
        (slotRomSize - 1) & 0x200000,  //if ROM is 4 MB, map in last 2 MB
        ((slotRomSize - 1) & 0x1fffff) + 1
      );
      //Map in the UPMEM chip when the locked-on ROM is exactly 1 MB.
      //Emulators should handle mirroring to $340000-$3fffff.
      if(slotRomSize == 0x100000) {
        auto upmemNode = manifest["board/lock-on/rom"];
        appendFile(temporaryRom, {path, upmemNode["name"].text()});
      }
    } else {
      MessageDialog().setTitle("minos").setText({
        "Invalid!\n",
        "Please choose a game without a Sega Virtua Processor."
      }).error();
    }
  }
}

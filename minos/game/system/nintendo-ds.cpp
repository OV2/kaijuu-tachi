// DeSmuME Footer Format
// At the end of a .dsv file is a 122-byte footer specifying properties about
// the game's save format.
const uint8_t DSV_HEADER[83] =
  "|<--Snip above h"
  "ere to create a "
  "raw sav by exclu"
  "ding this DeSmuM"
  "E savedata foote"
  "r:";

// Between the header and footer are 6 32-bit little-endian numbers.
// 0: Size of save file based on maximum write address
// 1: Size of save file after padding to a power of 2 size with 0xFF
// 2: Save type
// 3: Address size index
// 4: Size of save file that game expects (same as 1?)
// 5: version number (as of DeSmuME 0.9.10, this value is 0)
const Game::DSVType DSV_TYPES[] = {
// RAM Type, RAM Size, Size Index
  {"auto",         0, 0}, // 0 == Auto-Detect
  {"eeprom",     512, 1},
  {"eeprom",    8192, 2},
  {"eeprom",   65536, 2},
  {"sram",     32768, 2},
  {"flash",   262144, 3},
  {"flash",   524288, 3},
  {"flash",  1048576, 3},
  {"flash",  2097152, 3},
  {"flash",  4194304, 3},
  {"flash",  8388608, 3},
  {"flash", 16777216, 3},
  {"flash", 33554432, 3},
  {"flash", 67108864, 3}
};

const uint8_t DSV_FOOTER[17] = "|-DESMUME SAVE-|";

auto Game::depurifyDsv(RAMPaths& ram) -> void {
  if(!file::exists(ram.path)) return;
  file sav(ram.path, file::mode::read);
  uint fileSize = sav.size();
  uint8_t buffer[fileSize];
  sav.read(buffer, fileSize);
  sav.close();

  file dsv(ram.temporaryPath, file::mode::write);
  dsv.write(buffer, fileSize);
  dsv.write(DSV_HEADER, sizeof DSV_HEADER - 1);

  uint ramSize = 0;
  string ramType = "";
  if(!isFile) {
    string ramQuery = ram.settings["query"].text();
    auto ramNode = manifest.find(ramQuery)(0);
    ramSize = ramNode["size"].integer();
    string ramType = ramNode["type"].text();
  } else {
    ramSize = file::size(ram.path);
  }
  uint sizeIndex;
  for(auto dsvType : DSV_TYPES) {
    if(ramType && dsvType.ramType != ramType) continue;
    if(dsvType.ramSize != fileSize) continue;
    sizeIndex = dsvType.sizeIndex;
    break;
  }
  
  dsv.writel(ramSize, 4);
  dsv.writel(fileSize, 4);
  dsv.writel(0, 4);
  dsv.writel(sizeIndex, 4);
  dsv.writel(ramSize, 4);
  dsv.writel(0, 4);
  dsv.write(DSV_FOOTER, sizeof DSV_FOOTER - 1);
  dsv.close();
}

auto Game::repurifyDsv(RAMPaths& ram) -> void {
  if(!file::exists(ram.temporaryPath)) return;
  file dsv(ram.temporaryPath, file::mode::read);
  uint fileSize = dsv.size();
  uint8_t buffer[fileSize];
  dsv.read(buffer, fileSize);
  dsv.close();

  file sav(ram.path, file::mode::write);
  sav.write(buffer, fileSize - 82 - 24 - 16);
  sav.close();
  file::remove(ram.temporaryPath);
}

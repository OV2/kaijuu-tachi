auto Game::n64_romInternalName() -> string {
  //Shift JIS is dangerous to use for filenames; only older 日本
  //computers running DOS-based Windows use it for filenames, and because
  //double-byte codes do not guarantee that both bytes are 0x80 or
  //greater, characters not compatible with FAT and NTFS can be used
  //unintentionally. 
  //Most 日本 games that do not use ASCII use halfwidth ｶﾀｶﾅ, which
  //are 1 byte long each and always 0xA1 or higher. Only fullwidth かな and 漢字
  //are dangerous because they can include bytes below 0x80.
  file rom(temporaryRomPath, file::mode::read);
  rom.seek(0x00000020);
  string name = rom.reads(0x14);
  if(emulatorName == "Project64") {
    //Project64 interprets internal names as Windows-1252 and guards against
    //the use of forward slashes, colons, and backslashes by replacing them
    //with hyphen-minuses, semicolons, and hyphen-minuses respectively but will
    //raise an error with other incompatible symbols.
    name = name.transform("/:\\", "-;-");
  } else if(emulatorName == "mupen64plus") {
    //mupen64plus also interprets internal names as Windows-1252, though it
    //does not guard against incompatible symbols in filenames at all.
  } else {
  //name = ramus::convertShiftJIStoUTF8(name);
  }
  return name;  
}

auto Game::n64_romGoodsetName() -> string {
  if(emulatorName == "mupen64plus") {
    string applicationDir = Location::path(emulatorSettings["path"].text());

    string ini = file::read({applicationDir, "mupen64plus.ini"});
    ini.replace("\r\n", "\n");
    string_vector database = ini.split("\n");

    string md5 = ramus::Hash::MD5(file::read(temporaryRomPath)).digest().transform("abcdef", "ABCDEF");
    if(auto result = database.find({"[", md5, "]"})) {
      uint index = result();
      while(index < database.size() && !database[++index].beginsWith("[")) {
        if(database[index].beginsWith("GoodName")) {
          return database[index].split("=", 1).right();
        }
      }
    }
  }
  return "";
}

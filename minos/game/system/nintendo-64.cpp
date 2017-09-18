auto Game::n64_romInternalName() -> string {
  //Names are in Shift JIS, but Project64 stores them in Windows-1252.
  //Shift JIS is dangerous to use for filenames; only older 日本
  //computers running DOS-based Windows use it for filenames, and because
  //double-byte codes do not guarantee that both bytes are 0x80 or
  //greater, characters not compatible with FAT and NTFS can be used
  //unintentionally. Project64 guards against the use of forward slashes,
  //colons, and backslashes by replacing them with hyphen-minuses,
  //semicolons, and hyphen-minuses respectively but will raise an error
  //with other incompatible symbols.
  //Most 日本 games that do not use ASCII use halfwidth ｶﾀｶﾅ, which
  //are 1 byte long each. Only fullwidth かな and 漢字 are subject to the
  //dangerous implications.
  file rom(temporaryRomPath, file::mode::read);
  rom.seek(0x00000020);
  string name = rom.reads(0x14);
  if(emulatorName == "Project64") {
    name = name.replace("/", "-").replace(":", ";").replace("\\", "-");
  } else {
  //name = ramus::convertShiftJIStoUTF8(name);
  }
  return name;  
}

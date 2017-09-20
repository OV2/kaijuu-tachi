struct Game {
  Game(const string& pathname);

  auto play(const string& name) -> void;
  auto play() -> void;

private:
  static Markup::Node settings;
  static auto initializeSettings() -> void;

  auto loadManifest(const string& path) -> Markup::Node;
  auto setEmulator() -> void;
  auto setEmulator(const string& name) -> void;
  auto emulatorSettingNode(const string& name) -> Markup::Node;
  auto emulatorSetting(const string& name) -> string;
  auto emulatorRamName(const string& ext) -> string;

  auto romInternalName() -> string;
  auto romGoodsetName() -> string;

  auto _play() -> void;

  auto validate() -> bool;

  //purify.cpp
  auto depurify() -> void;
  auto promptSlots() -> void;
  auto depurifyRom() -> void;
  auto depurifyRam() -> void;
  auto depurifyStates() -> void;

  auto appendFile(file& destination, const string& sourcePath) -> void;
  auto appendFile(file& destination, const string& sourcePath, uint start, uint length) -> void;

  auto repurify() -> void;
  auto repurifyRom() -> void;
  auto repurifyRam() -> void;
  auto repurifyStates() -> void;

  string path;
  string name;
  string type;
  bool isFile;

  Markup::Node manifest;
  string romPath;

  Markup::Node systemSettings;
  Markup::Node extensionSettings;
  Markup::Node emulatorSettings;
  Markup::Node emulatorExtensionSettings;

  bool compositeRom;
  string emulatorName;
  struct Slot {
    bool active = false;
    string path;
    Markup::Node manifest;
  };
  vector<shared_pointer<Slot>> slots;
  string temporaryRomPath;
  struct RAMPaths {
    string path;
    string temporaryPath;
    Markup::Node settings;
  };
  vector<RAMPaths> ramPaths;
  string error;

  //Console-specific functions
  #define romSpecial(comp)\
    auto comp##_romSpecial(const string&, file&, Markup::Node&) -> void
  #define ramSpecial(comp)\
    auto comp##_ramSpecial(const string&, file&, Markup::Node&) -> void
  #define slotRom(comp)\
    auto comp##_slotRom(Markup::Node&, Slot&, file&, Markup::Node&) -> void
  #define slotRam(comp)\
    auto comp##_slotRam(Markup::Node&, Slot&, file&, Markup::Node&) -> void

  //famicom.cpp
  romSpecial(fc);
  romSpecial(vs);
  romSpecial(pc10);
  auto writeNes20Header(file&, Markup::Node&) -> void;
  auto fetchNes20Mapper(Markup::Node&, uint&, uint&, bool&, bool&) -> void;

  static Markup::Node nes20Settings;

  //super-famicom.cpp
  romSpecial(nss);
  slotRom(sfcb);

  //nintendo-64.cpp
  auto n64_romInternalName() -> string;
  auto n64_romGoodsetName() -> string;

public:
  struct DSVType {
    string ramType;
    uint ramSize;
    uint sizeIndex;
  };
private:

  //mega-drive.cpp
  slotRom(md);

  //nintendo-ds.cpp
  auto depurifyDsv(RAMPaths& ram) -> void;
  auto repurifyDsv(RAMPaths& ram) -> void;

  #undef romSpecial
  #undef ramSpecial
  #undef slotRom
  #undef slotRam
};

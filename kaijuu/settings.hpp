struct Settings {
  struct Rule {
    string name;
    string pattern;
    bool defaultAction;
    bool matchFiles;
    bool matchFolders;
    string command;
    string icon;
    bool multiSelection;
  };
  vector<Rule> rules;

  auto load() -> void {
    rules.reset();
    string_vector ruleIDs = registry::contents("HKCU\\Software\\Kaijuu\\");
    for(auto &ruleID : ruleIDs) {
      string path = {"HKCU\\Software\\Kaijuu\\", ruleID, "\\"};
      rules.append({
        registry::read({path, "Name"}),
        registry::read({path, "Pattern"}),
        registry::read({path, "Default"}) == "true",
        registry::read({path, "Files"}) == "true",
        registry::read({path, "Folders"}) == "true",
        registry::read({path, "Command"}),
        registry::read({path, "Icon"}),
      });
    }
    for(auto &rule : rules) {
      rule.multiSelection = rule.command.find("{paths}") || rule.command.find("{files}");
    }
  }

  auto save() -> void {
    registry::remove("HKCU\\Software\\Kaijuu\\");
    for(uint id : range(rules.size())) {
      auto &rule = rules(id);
      string path = {"HKCU\\Software\\Kaijuu\\", pad(id, 3, '0'), "\\"};
      registry::write({path, "Name"}, rule.name);
      registry::write({path, "Pattern"}, rule.pattern);
      registry::write({path, "Default"}, rule.defaultAction);
      registry::write({path, "Files"}, rule.matchFiles);
      registry::write({path, "Folders"}, rule.matchFolders);
      registry::write({path, "Command"}, rule.command);
      registry::write({path, "Icon"}, rule.icon);
    }
  }
} settings;

#include "interface.hpp"
Program* program = nullptr;
RuleEditor* ruleEditor = nullptr;

#include "resource/resource.cpp"

Program::Program(const string &pathname) : pathname(pathname) {
  setTitle("kaijuu v06r03");

  layout.setMargin(5);
  statusLabel.setFont(Font().setBold());

  uninstallButton.setText("Uninstall").onActivate({&Program::uninstall, this});
  installButton.setText("Install").onActivate({&Program::install, this});

  settingList.onActivate({&Program::modifyAction, this});
  settingList.onChange({&Program::synchronize, this});

  appendButton.setText("Append").onActivate({&Program::appendAction, this});
  modifyButton.setText("Modify").onActivate({&Program::modifyAction, this});
  moveUpButton.setText("Move Up").onActivate({&Program::moveUpAction, this});
  moveDownButton.setText("Move Down").onActivate({&Program::moveDownAction, this});
  removeButton.setText("Remove").onActivate({&Program::removeAction, this});
  importButton.setText("Import").onActivate({&Program::importAction, this});
  exportButton.setText("Export").onActivate({&Program::exportAction, this});
  resetButton.setText("Reset").onActivate({&Program::resetAction, this});
  helpButton.setText("Help ...").onActivate([&] { invoke("kaijuu.html"); });

  canvas.setIcon(resource::icon);

  onClose(&Application::quit);
  refresh();
  synchronize();
  setVisible();

  setFrameGeometry({64, 64, 1024 - 75, 480});
}

auto Program::synchronize() -> void {
  if(registry::read({"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved\\", classID}) == classDescription) {
    statusLabel.setText("Extension status: installed");
    installButton.setEnabled(false);
    uninstallButton.setEnabled(true);
  } else {
    statusLabel.setText("Extension status: not installed");
    installButton.setEnabled(true);
    uninstallButton.setEnabled(false);
  }
  modifyButton.setEnabled((bool)settingList.selected());
  if(settingList.selected()) {
    uint selection = settingList.items().find(settingList.selected()).get();
    moveUpButton.setEnabled(settings.rules.size() > 1 && selection > 0);
    moveDownButton.setEnabled(settings.rules.size() > 1 && selection < settings.rules.size() - 1);
    removeButton.setEnabled(true);
  } else {
    moveUpButton.setEnabled(false);
    moveDownButton.setEnabled(false);
    removeButton.setEnabled(false);
  }
  resetButton.setEnabled(settings.rules.size() > 0);
}

auto Program::refresh() -> void {
  settings.load();
  settingList.reset();
  settingList.append(TableViewHeader().setVisible()
    .append(TableViewColumn().setText("Name"))
    .append(TableViewColumn().setText("Default"))
    .append(TableViewColumn().setText("Match"))
    .append(TableViewColumn().setText("Pattern"))
    .append(TableViewColumn().setText("Command").setExpandable())
  );
  for(auto &rule : settings.rules) {
    string match = "Nothing";
    if(rule.matchFiles && rule.matchFolders) match = "Everything";
    else if(rule.matchFiles) match = "Files";
    else if(rule.matchFolders) match = "Folders";
    settingList.append(TableViewItem()
      .append(TableViewCell().setText(rule.name))
      .append(TableViewCell().setText(rule.defaultAction ? "Yes" : "No"))
      .append(TableViewCell().setText(match))
      .append(TableViewCell().setText(rule.pattern))
      .append(TableViewCell().setText(rule.command))
    );
  }
  settingList.resizeColumns();
}

auto Program::install() -> void {
  string command = {"regsvr32 \"", pathname, classDriver, "\""};
  _wsystem(utf16_t(command));
  synchronize();
}

auto Program::uninstall() -> void {
  string command = {"regsvr32 /u \"", pathname, classDriver, "\""};
  _wsystem(utf16_t(command));
  synchronize();
}

auto Program::appendAction() -> void {
  ruleEditor->show();
}

auto Program::modifyAction() -> void {
  if(!settingList.selected()) return;
  uint selection = settingList.items().find(settingList.selected()).get();
  ruleEditor->show(selection);
}

auto Program::moveUpAction() -> void {
  if(!settingList.selected()) return;
  uint selection = settingList.items().find(settingList.selected()).get();
  if(selection == 0) return;
  auto temp = settings.rules(selection - 1);
  settings.rules(selection - 1) = settings.rules(selection);
  settings.rules(selection) = temp;
  settings.save();
  refresh();
  settingList.item(selection - 1).setSelected();
  synchronize();
}

auto Program::moveDownAction() -> void {
  if(!settingList.selected()) return;
  uint selection = settingList.items().find(settingList.selected()).get();
  if(selection >= settings.rules.size() - 1) return;
  auto temp = settings.rules(selection + 1);
  settings.rules(selection + 1) = settings.rules(selection);
  settings.rules(selection) = temp;
  settings.save();
  refresh();
  settingList.item(selection + 1).setSelected();
  synchronize();
}

auto Program::removeAction() -> void {
  if(!settingList.selected()) return;
  uint selection = settingList.items().find(settingList.selected()).get();
  settings.rules.remove(selection);
  settings.save();
  refresh();
  synchronize();
}

auto Program::importAction() -> void {
  if(!file::exists("rules.bml")) {
    MessageWindow().setParent(*this).setText("Error: could not find rules.bml.").error();
    return;
  }
  if(MessageWindow().setParent(*this).setText("Warning: this will overwrite all rules! Are you sure you want to do this?")
  .question() == MessageWindow::Response::No) return;
  settings.rules.reset();

  auto input = BML::unserialize(file::read("rules.bml"));
  for(auto& node : input.find("rule")) {
    settings.rules.append({
      node["name"].text(),
      node["pattern"].text(),
      node["default-action"].boolean(),
      node["match-files"].boolean(),
      node["match-folders"].boolean(),
      node["command"].text(),
      node["multi-selection"].boolean(),
    });
  }

  settings.save();
  refresh();
  synchronize();
}

auto Program::exportAction() -> void {
  if(file::exists("rules.bml")) {
    if(MessageWindow().setParent(*this).setText("Warning: rules.bml already exists! Are you sure you want to overwrite it?")
    .question() == MessageWindow::Response::No) return;
  }
  string output;
  for(auto& rule : settings.rules) {
    Markup::Node node;
    node("rule/name").setValue(rule.name);
    node("rule/pattern").setValue(rule.pattern);
    node("rule/default-action").setValue(rule.defaultAction);
    node("rule/match-files").setValue(rule.matchFiles);
    node("rule/match-folders").setValue(rule.matchFolders);
    node("rule/command").setValue(rule.command);
    node("rule/multi-selection").setValue(rule.multiSelection);
    output.append(BML::serialize(node), "\n");
  }
  file::write("rules.bml", output);
  MessageWindow().setParent(*this).setText("Exported to rules.bml.").information();
}

auto Program::resetAction() -> void {
  if(MessageWindow().setParent(*this).setText("Warning: this will permanently remove all rules! Are you sure you want to do this?")
  .question() == MessageWindow::Response::No) return;
  settings.rules.reset();
  settings.save();
  refresh();
  synchronize();
}

RuleEditor::RuleEditor() : index(-1) {
  setTitle("Rule Editor");

  layout.setMargin(5);

  nameLabel.setText("Name:");
  nameValue.onChange({&RuleEditor::synchronize, this});

  patternLabel.setText("Pattern:");
  patternValue.onChange({&RuleEditor::synchronize, this});

  commandLabel.setText("Command:");
  commandValue.onChange({&RuleEditor::synchronize, this});

  commandSelect.setText("Select ...").onActivate([&] {
    string pathname = BrowserWindow().setParent(*this)
    .setPath(program->pathname)
    .setFilters({"Programs (*.exe)", "All Files (*)"})
    .open();
    if(pathname) {
      pathname.transform("/", "\\");
      commandValue.setText({"\"", pathname, "\" {file}"});
    }
    synchronize();
  });

  defaultAction.setText("Default Action");
  filesAction.setText("Match Files");
  foldersAction.setText("Match Folders");
  assignButton.setText("Assign");

  auto assign = [&] {
    Settings::Rule rule = {
      nameValue.text(),
      patternValue.text(),
      defaultAction.checked(),
      filesAction.checked(),
      foldersAction.checked(),
      commandValue.text()
    };
    if(index == -1) {
      settings.rules.append(rule);
    } else {
      settings.rules(index) = rule;
    }
    settings.save();
    setVisible(false);
    setModal(false);
    program->refresh();
    program->synchronize();
  };

  nameValue.onActivate(assign);
  patternValue.onActivate(assign);
  commandValue.onActivate(assign);
  assignButton.onActivate(assign);

  setFrameGeometry({128, 256, 500, 160});
}

auto RuleEditor::synchronize() -> void {
  bool enable = true;
  if(!nameValue.text()) enable = false;
  if(!patternValue.text()) enable = false;
  if(!commandValue.text()) enable = false;
  assignButton.setEnabled(enable);
}

auto RuleEditor::show(int ruleID) -> void {
  Settings::Rule rule{"", "", false, true, false, "", false};
  if(ruleID >= 0) rule = settings.rules(ruleID);

  index = ruleID;
  nameValue.setText(rule.name);
  patternValue.setText(rule.pattern);
  commandValue.setText(rule.command);
  defaultAction.setChecked(rule.defaultAction);
  filesAction.setChecked(rule.matchFiles);
  foldersAction.setChecked(rule.matchFolders);
  synchronize();
  setVisible();
  setFocused();
  nameValue.setFocused();

  setModal(true);
  while(visible()) {
    Application::processEvents();
  }
  program->setFocused();
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

auto isWow64() -> bool {
  LPFN_ISWOW64PROCESS fnIsWow64Process;
  fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
  if(!fnIsWow64Process) return false;
  BOOL result = FALSE;
  fnIsWow64Process(GetCurrentProcess(), &result);
  return result == TRUE;
}

auto CALLBACK WinMain(HINSTANCE module, HINSTANCE, LPSTR, int) -> int {
  if(isWow64()) {
    MessageWindow().setText("Error: you must run kaijuu64.exe on 64-bit Windows.").error();
    return 0;
  }

  wchar_t filename[MAX_PATH];
  GetModuleFileNameW(module, filename, MAX_PATH);

  string pathname = (const char*)utf8_t(filename);
  pathname.transform("\\", "/");
  pathname = Location::path(pathname);

  program = new Program(pathname);
  ruleEditor = new RuleEditor;
  Application::run();

  return 0;
}

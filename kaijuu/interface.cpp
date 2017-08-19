#include "interface.hpp"
Program* program = nullptr;
RuleEditor* ruleEditor = nullptr;

#include "resource/resource.cpp"

Program::Program(const string &pathname) : pathname(pathname) {
  setTitle("kaijuu v06r02");
  setFrameGeometry({64, 64, 1024 - 75, 480});

  layout.setMargin(5);
  statusLabel.setFont(Font().setBold());
  uninstallButton.setText("Uninstall");
  installButton.setText("Install");
  appendButton.setText("Append");
  modifyButton.setText("Modify");
  moveUpButton.setText("Move Up");
  moveDownButton.setText("Move Down");
  removeButton.setText("Remove");
  resetButton.setText("Reset");
  helpButton.setText("Help ...");

  canvas.setIcon(resource::icon);

  onClose(&Application::quit);
  installButton.onActivate({&Program::install, this});
  uninstallButton.onActivate({&Program::uninstall, this});
  settingList.onActivate({&Program::modifyAction, this});
  settingList.onChange({&Program::synchronize, this});
  appendButton.onActivate({&Program::appendAction, this});
  modifyButton.onActivate({&Program::modifyAction, this});
  moveUpButton.onActivate({&Program::moveUpAction, this});
  moveDownButton.onActivate({&Program::moveDownAction, this});
  removeButton.onActivate({&Program::removeAction, this});
  resetButton.onActivate({&Program::resetAction, this});
  helpButton.onActivate([&] { nall::invoke("kaijuu.html"); });
  refresh();
  synchronize();
  setVisible();
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
  setFrameGeometry({128, 256, 500, 160});

  layout.setMargin(5);
  nameLabel.setText("Name:");
  patternLabel.setText("Pattern:");
  commandLabel.setText("Command:");
  commandSelect.setText("Select ...");
  defaultAction.setText("Default Action");
  filesAction.setText("Match Files");
  foldersAction.setText("Match Folders");
  assignButton.setText("Assign");

  nameValue.onChange({&RuleEditor::synchronize, this});
  patternValue.onChange({&RuleEditor::synchronize, this});
  commandValue.onChange({&RuleEditor::synchronize, this});

  commandSelect.onActivate([&] {
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

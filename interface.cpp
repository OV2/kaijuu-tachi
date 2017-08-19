#include "interface.hpp"
Application *application = nullptr;
KaijuuAssociation *kaijuuAssociation = nullptr;

Application::Application(const string &pathname) : pathname(pathname) {
  setTitle("kaijuu v02");
  setFrameGeometry({64, 64, 640, 480});

  layout.setMargin(5);
  statusLabel.setFont("Tahoma, 8, Bold");
  uninstallButton.setText("Uninstall");
  installButton.setText("Install");
  settingList.setHeaderText("Filter", "Default", "Association", "Description");
  settingList.setHeaderVisible();
  appendButton.setText("Append");
  modifyButton.setText("Modify");
  removeButton.setText("Remove");

  append(layout);
  layout.append(installLayout, {~0, 0}, 5);
    installLayout.append(statusLabel, {~0, 0}, 5);
    installLayout.append(uninstallButton, {80, 0}, 5);
    installLayout.append(installButton, {80, 0});
  layout.append(settingLayout, {~0, ~0});
    settingLayout.append(settingList, {~0, ~0}, 5);
    settingLayout.append(controlLayout, {0, ~0});
      controlLayout.append(appendButton, {80, 0}, 5);
      controlLayout.append(modifyButton, {80, 0}, 5);
      controlLayout.append(removeButton, {80, 0}, 5);
      controlLayout.append(spacer, {0, ~0});
      controlLayout.append(canvas, {80, 88});

  image icon;
  icon.load({pathname, "kaijuu.png"});
  canvas.setImage(icon);
  canvas.update();

  onClose = &OS::quit;
  installButton.onActivate = {&Application::install, this};
  uninstallButton.onActivate = {&Application::uninstall, this};
  settingList.onActivate = {&Application::modifyAction, this};
  settingList.onChange = {&Application::synchronize, this};
  appendButton.onActivate = {&Application::appendAction, this};
  modifyButton.onActivate = {&Application::modifyAction, this};
  removeButton.onActivate = {&Application::removeAction, this};
  refresh();
  synchronize();
  setVisible();
}

void Application::synchronize() {
  if(registry::read({"HKLM/Software/Microsoft/Windows/CurrentVersion/Shell Extensions/Approved/", classID}) == classDescription) {
    statusLabel.setText("Extension status: installed");
    installButton.setEnabled(false);
    uninstallButton.setEnabled(true);
  } else {
    statusLabel.setText("Extension status: not installed");
    installButton.setEnabled(true);
    uninstallButton.setEnabled(false);
  }
  modifyButton.setEnabled(settingList.selected());
  removeButton.setEnabled(settingList.selected());
}

void Application::refresh() {
  settingList.reset();
  lstring rules = registry::contents("HKCU/Software/Kaijuu/");
  for(auto &rule : rules) {
    string name = string{rule}.rtrim<1>("/");
    string path = {"HKCU/Software/Kaijuu/", rule};
    settingList.append(name,
      registry::read({path, "Default"}),
      registry::read({path, "Association"}),
      registry::read({path, "Description"})
    );
  }
  settingList.autoSizeColumns();
}

void Application::install() {
  string command = {"regsvr32 \"", pathname, classDriver, "\""};
  _wsystem(utf16_t(command));
  synchronize();
}

void Application::uninstall() {
  string command = {"regsvr32 /u \"", pathname, classDriver, "\""};
  _wsystem(utf16_t(command));
  synchronize();
}

void Application::appendAction() {
  kaijuuAssociation->descriptionValue.setText("Open with kaijuu");
  kaijuuAssociation->filterValue.setEnabled(true);
  kaijuuAssociation->filterValue.setText("");
  kaijuuAssociation->associationValue.setText("");
  kaijuuAssociation->synchronize();
  kaijuuAssociation->setVisible();
  kaijuuAssociation->setFocused();
  kaijuuAssociation->filterValue.setFocused();
}

void Application::modifyAction() {
  unsigned selection = settingList.selection();
  lstring rules = registry::contents("HKCU/Software/Kaijuu/");
  string name = rules(selection);
  if(name.empty()) return;

  string path = {"HKCU/Software/Kaijuu/", name};
  kaijuuAssociation->descriptionValue.setText(registry::read({path, "Description"}));
  kaijuuAssociation->filterValue.setEnabled(false);
  kaijuuAssociation->filterValue.setText(string{name}.rtrim<1>("/"));
  kaijuuAssociation->associationValue.setText(registry::read({path, "Association"}));
  kaijuuAssociation->defaultAction.setChecked(registry::read({path, "Default"}) == "true");
  kaijuuAssociation->synchronize();
  kaijuuAssociation->setVisible();
  kaijuuAssociation->setFocused();
  kaijuuAssociation->associationSelect.setFocused();
}

void Application::removeAction() {
  unsigned selection = settingList.selection();
  lstring leaves = registry::contents("HKCU/Software/Kaijuu/");
  string name = leaves(selection);
  if(name.empty()) return;
  registry::remove({"HKCU/Software/Kaijuu/", name});
  refresh();
  synchronize();
}

KaijuuAssociation::KaijuuAssociation() {
  setTitle("Kaijuu Association");
  setFrameGeometry({128, 128, 500, 160});

  layout.setMargin(5);
  descriptionLabel.setText("Description:");
  filterLabel.setText("Filter:");
  associationLabel.setText("Association:");
  associationSelect.setText("Select ...");
  defaultAction.setText("Default folder action");
  helpButton.setText("Help ...");
  assignButton.setText("Assign");

  Font font("Tahoma, 8");
  unsigned length = 0;
  length = max(length, font.geometry("Description:").width);
  length = max(length, font.geometry("Filter:").width);
  length = max(length, font.geometry("Association:").width);

  append(layout);
  layout.append(descriptionLayout, {~0, 0}, 5);
    descriptionLayout.append(descriptionLabel, {length, 0}, 5);
    descriptionLayout.append(descriptionValue, {~0, 0});
  layout.append(filterLayout, {~0, 0}, 5);
    filterLayout.append(filterLabel, {length, 0}, 5);
    filterLayout.append(filterValue, {~0, 0});
  layout.append(associationLayout, {~0, 0}, 5);
    associationLayout.append(associationLabel, {length, 0}, 5);
    associationLayout.append(associationValue, {~0, 0}, 5);
    associationLayout.append(associationSelect, {80, 0});
  layout.append(controlLayout, {~0, 0});
    controlLayout.append(defaultAction, {0, 0}, 5);
    controlLayout.append(spacer, {~0, 0});
    controlLayout.append(helpButton, {80, 0}, 5);
    controlLayout.append(assignButton, {80, 0});

  Geometry geometry = Window::geometry();
  geometry.height = layout.minimumGeometry().height;
  setGeometry(geometry);

  filterValue.onChange = {&KaijuuAssociation::synchronize, this};

  associationSelect.onActivate = [&] {
    string pathname = DialogWindow::fileOpen(*this, application->pathname, "Executable Programs (*.exe)", "All Files (*)");
    if(pathname.empty() == false) associationValue.setText({"\"", pathname, "\" \"{path}\""});
    synchronize();
  };

  helpButton.onActivate = [&] {
    MessageWindow::information(*this,
      "The filter value is matched against selected folders.\n"
      "Upon successful match, the rule will appear in the context menu.\n"
      "Example: *.game will match any folder that ends in .game\n\n"
      "If default folder action is checked, double-clicking will execute the association.\n"
      "In this case, opening the folder must be done through the context menu instead."
    );
  };

  assignButton.onActivate = [&] {
    string path = {"HKCU/Software/Kaijuu/", filterValue.text(), "/"};
    registry::write({path, "Description"}, descriptionValue.text());
    registry::write({path, "Association"}, associationValue.text());
    registry::write({path, "Default"}, defaultAction.checked());
    application->refresh();
    application->synchronize();
    setVisible(false);
  };
}

void KaijuuAssociation::synchronize() {
  bool enable = true;
  if(descriptionValue.text().empty()) enable = false;
  if(filterValue.text().empty()) enable = false;
  if(associationValue.text().empty()) enable = false;
  assignButton.setEnabled(enable);
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

bool isWow64() {
  LPFN_ISWOW64PROCESS fnIsWow64Process;
  fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
  if(!fnIsWow64Process) return false;
  BOOL result = FALSE;
  fnIsWow64Process(GetCurrentProcess(), &result);
  return result == TRUE;
}

int CALLBACK WinMain(HINSTANCE module, HINSTANCE, LPSTR, int) {
  if(isWow64()) {
    MessageWindow::critical(Window::None, "Error: you must run kaijuu64.exe on 64-bit Windows.");
    return 0;
  }

  wchar_t filename[MAX_PATH];
  GetModuleFileNameW(module, filename, MAX_PATH);

  string pathname = (const char*)utf8_t(filename);
  pathname.transform("\\", "/");
  pathname = dir(pathname);

  application = new Application(pathname);
  kaijuuAssociation = new KaijuuAssociation;
  OS::main();

  return 0;
}

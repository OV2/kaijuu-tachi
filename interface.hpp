#include <nall/platform.hpp>
#include <nall/string.hpp>
#include <nall/windows/registry.hpp>
using namespace nall;

#include <phoenix/phoenix.hpp>
using namespace phoenix;

#include "guid.hpp"

struct Application : Window {
  VerticalLayout layout;
    HorizontalLayout installLayout;
      Label statusLabel;
      Button uninstallButton;
      Button installButton;
    HorizontalLayout settingLayout;
      ListView settingList;
      VerticalLayout controlLayout;
        Button appendButton;
        Button modifyButton;
        Button removeButton;
        Widget spacer;
        Canvas canvas;

  Application(const string &pathname);
  void synchronize();
  void refresh();
  void install();
  void uninstall();
  void appendAction();
  void modifyAction();
  void removeAction();

  string pathname;
};

struct KaijuuAssociation : Window {
  VerticalLayout layout;
    HorizontalLayout filterLayout;
      Label filterLabel;
      LineEdit filterValue;
      Button filterHelp;
    HorizontalLayout associationLayout;
      Label associationLabel;
      LineEdit associationValue;
      Button associationSelect;
    HorizontalLayout controlLayout;
      Widget spacer;
      Button assignButton;

  KaijuuAssociation();
  void synchronize();
};

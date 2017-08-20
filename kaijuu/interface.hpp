#include <nall/platform.hpp>
#include <nall/run.hpp>
#include <nall/string.hpp>
#include <nall/windows/registry.hpp>
using namespace nall;

#include <hiro/hiro.hpp>
using namespace hiro;

#include "guid.hpp"
#include "settings.hpp"

struct Program : Window {
  Program(const string &pathname);
  auto synchronize() -> void;
  auto refresh() -> void;
  auto install() -> void;
  auto uninstall() -> void;
  auto appendAction() -> void;
  auto modifyAction() -> void;
  auto moveUpAction() -> void;
  auto moveDownAction() -> void;
  auto removeAction() -> void;
  auto importAction() -> void;
  auto exportAction() -> void;
  auto resetAction() -> void;

  string pathname;

  VerticalLayout layout{this};
    HorizontalLayout installLayout{&layout, Size{~0, 0}};
      Label statusLabel{&installLayout, Size{~0, 0}};
      Button uninstallButton{&installLayout, Size{80, 0}};
      Button installButton{&installLayout, Size{80, 0}};
    HorizontalLayout settingLayout{&layout, Size{~0, ~0}};
      TableView settingList{&settingLayout, Size{~0, ~0}};
      VerticalLayout controlLayout{&settingLayout, Size{0, ~0}};
        Button appendButton{&controlLayout, Size{80, 0}};
        Button modifyButton{&controlLayout, Size{80, 0}};
        Button moveUpButton{&controlLayout, Size{80, 0}};
        Button moveDownButton{&controlLayout, Size{80, 0}};
        Button removeButton{&controlLayout, Size{80, 0}};
        Widget spacer{&controlLayout, Size{0, ~0}};
        Button importButton{&controlLayout, Size{80, 0}};
        Button exportButton{&controlLayout, Size{80, 0}};
        Button resetButton{&controlLayout, Size{80, 0}};
        Button helpButton{&controlLayout, Size{80, 0}};
        Canvas canvas{&controlLayout, Size{80, 88}};
};

struct RuleEditor : Window {
  RuleEditor();
  auto synchronize() -> void;
  auto show(int rule = -1) -> void;

  //bool modal;
  int index;

  VerticalLayout layout{this};
    HorizontalLayout nameLayout{&layout, Size{~0, 0}};
      Label nameLabel{&nameLayout, Size{80, 0}};
      LineEdit nameValue{&nameLayout, Size{~0, 0}};
    HorizontalLayout patternLayout{&layout, Size{~0, 0}};
      Label patternLabel{&patternLayout, Size{80, 0}};
      LineEdit patternValue{&patternLayout, Size{~0, 0}};
    HorizontalLayout commandLayout{&layout, Size{~0, 0}};
      Label commandLabel{&commandLayout, Size{80, 0}};
      LineEdit commandValue{&commandLayout, Size{~0, 0}};
      Button commandSelect{&commandLayout, Size{80, 0}};
    HorizontalLayout controlLayout{&layout, Size{~0, 0}};
      CheckLabel defaultAction{&controlLayout, Size{90, 0}};
      CheckLabel filesAction{&controlLayout, Size{90, 0}};
      CheckLabel foldersAction{&controlLayout, Size{90, 0}};
      Widget spacer{&controlLayout, Size{~0, 0}};
      Button assignButton{&controlLayout, Size{80, 0}};
};

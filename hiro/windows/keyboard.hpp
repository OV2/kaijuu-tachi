#if defined(Hiro_Keyboard)

namespace hiro {

struct pKeyboard {
  static auto poll() -> vector<bool>;
  static auto pressed(unsigned code) -> bool;

  static auto _pressed(uint16_t code) -> bool;
  static auto _translate(unsigned code, unsigned flags) -> signed;

  static auto initialize() -> void;

  static vector<uint16_t> keycodes;
};

}

#endif

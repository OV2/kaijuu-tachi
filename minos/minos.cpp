#include <minos.hpp>
#include <game/game.hpp>

#include <nall/main.hpp>
auto nall::main(string_vector args) -> void {
  Application::setName("minos");

  if(args.size() <= 1) {
    MessageDialog().setTitle("minos").setText({
      "Usage (with default emulator):\n",
      "minos \"<path_to_game>\"\n\n",

      "Usage (specify emulator):\n",
      "minos \"<path_to_game>\" <emulator_name>\n\n",

      "Configure emulators in settings.bml. See the README for more information."
    }).information();
    return;
  }

  Game game(args[1].transform("\\", "/").trimRight("/"));

  if(args.size() >= 3) {
    game.play(args[2]);
  } else {
    game.play();
  }
}

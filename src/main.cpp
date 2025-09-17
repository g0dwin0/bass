#include <format>
#include "frontend/window.hpp"
#include "agb.hpp"
#include "bus.hpp"
#include "cli11/CLI11.hpp"
#include "common.hpp"


int handle_args(int& argc, char** argv, std::string& filename) {
  CLI::App app{"", "bass"};
  app.add_option("-f,--file", filename, "path to ROM")->required();

  CLI11_PARSE(app, argc, argv);
  return 0;
}

int main(int argc, char** argv) {
  std::string filename = {};
  handle_args(argc, argv, filename); 

  // setup system thread
  AGB agb = {};
  Frontend f{&agb};

  std::vector<u8> file = read_file(filename);
  agb.bus.pak->load_data(file);
  SDL_SetWindowTitle(f.window, std::format("bass | {}", agb.pak.info.game_title).c_str());
  
  std::thread system = std::thread(&AGB::system_loop, &agb);

  while (f.state.running) {
    f.handle_events();
    f.render_frame();
  }

  agb.active = false;
  system.join();

  return 0;
}

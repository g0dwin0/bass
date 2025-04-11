#include "bass.hpp"
#include "bus.hpp"
#include "cli11/CLI11.hpp"
#include "common.hpp"
#include "spdlog/common.h"
#include "window.h"

int handle_args(int& argc, char** argv, std::string& filename) {
  CLI::App app{"", "bass"};
  app.add_option("-f,--file", filename, "path to ROM")->required();

  CLI11_PARSE(app, argc, argv);
  return 0;
}

int main(int argc, char** argv) {
  // setup system thread
  Bass bass;
  Frontend f{&bass};

  std::string filename;
  handle_args(argc, argv, filename);
  std::vector<u8> file = read_file(filename);
  bass.bus.pak->load_data(file);

  spdlog::set_level(spdlog::level::off);
  std::thread system = std::thread(&Bass::system_loop, &bass);

  while (f.state.running) {
    f.handle_events();
    f.render_frame();
  }

  bass.active = false;
  system.join();

  return 0;
}

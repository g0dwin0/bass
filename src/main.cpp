#include "bass.hpp"
#include "bus.hpp"
#include "cli11/CLI11.hpp"
#include "common.hpp"
#include "window.hpp"


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
  Bass bass = {};
  Frontend f{&bass};

  std::vector<u8> file = read_file(filename);
  bass.bus.pak->load_data(file);

  std::thread system = std::thread(&Bass::system_loop, &bass);

  while (f.state.running) {
    f.handle_events();
    f.render_frame();
  }

  bass.active = false;
  system.join();

  return 0;
}

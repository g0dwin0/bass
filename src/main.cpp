#include "io.hpp"
#include "bass.hpp"
#include "spdlog/spdlog.h"
#include "window.h"

// int handle_args(int argc, char** argv, std::string& filename) {
//   CLI::App app{"", "bass"};
//   app.add_option("-f,--file", filename, "path to ROM")->required();

//   CLI11_PARSE(app, argc, argv);
//   return 0;
// }

int main() {
  Bass bass;

  Frontend f{&bass};

  spdlog::set_level(spdlog::level::debug);

  std::string filename = "/home/toast/Projects/bass/roms/panda.gba";
  
  std::vector<u8> file = read_file(filename);

  spdlog::debug("loaded rom with filename: {}", filename);

  bass.bus.pak->load_data(file);

  for (size_t i = 0; i < 0x70000; i++) {
    bass.cpu.step();
  }
  // long long ctr = 0;
  while (f.state.running) {
    f.handle_events();
    f.render_frame();
    // bass.cpu.step();

    // bass.cpu.step();
    bass.ppu.tick();

    // if(ctr % 100000 == 0) {
    //   bass.ppu.frame_ready
    // }
    // ctr++;
  }

  f.shutdown();

  return 0;
}
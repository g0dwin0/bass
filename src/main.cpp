#include "io.hpp"
#include "bass.hpp"
#include "spdlog/spdlog.h"
#include "window.h"

int main() {
  Bass bass;

  Frontend f{&bass};

  spdlog::set_level(spdlog::level::debug);

  std::string_view filename = "/home/toast/Projects/bass/roms/arm/arm.gba";
  
  std::vector<u8> file = read_file(filename);

  spdlog::debug("loaded rom with filename: {}", filename);

  bass.bus.pak->load_data(file);

  for (size_t i = 0; i < 0x20000; i++) {
    bass.cpu.step();
    bass.ppu.DISPSTAT.VBLANK_FLAG = !bass.ppu.DISPSTAT.VBLANK_FLAG;
  } 

  while (f.state.running) {
    f.handle_events();
    f.render_frame();
    // bass.ppu.tick();
  }

  f.shutdown();

  return 0;
}
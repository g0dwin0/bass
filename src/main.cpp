#include "bass.hpp"
#include "io.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "window.h"

int main() {
  Bass bass;
  Frontend f{&bass};
  std::string_view filename = "/home/toast/Projects/bass/roms/arm/arm.gba";
  // std::string_view filename = "/home/toast/Projects/bass/roms/ARM_Any.gba";
  // std::string_view filename = "/home/toast/Projects/bass/roms/panda.gba";

  // std::string_view filename = "/home/toast/Projects/bass/roms/armwrestler.gba";
  
spdlog::set_level(spdlog::level::trace);
  std::vector<u8> file = read_file(filename);

  spdlog::debug("loaded rom with filename: {}", filename);

  bass.bus.pak->load_data(file);

  // for (size_t i = 0; i < 642; i++) {
  for (size_t i = 0; i < 1300; i++) {
    // for (size_t i = 0; i < 1528; i++) {
    
    bass.cpu.step();
    fmt::println("{} {:#010x}", i, bass.cpu.regs.r[15]);
    bass.ppu.DISPSTAT.VBLANK_FLAG = !bass.ppu.DISPSTAT.VBLANK_FLAG;
  }

  while (f.state.running) {
    f.handle_events();
    f.render_frame();
    // if (f.bass->bus.cycles_elapsed >= 300000) {
    //   bass.ppu.draw();
    //   f.bass->bus.cycles_elapsed -= 300000;
    // }
    // bass.ppu.tick();
  }

  f.shutdown();

  return 0;
}

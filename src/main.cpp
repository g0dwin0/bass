#include <spdlog/stopwatch.h>
#include "bass.hpp"
#include "common.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "spdlog/spdlog.h"
#include "cli11/CLI11.hpp"
#include "window.h"



int handle_args(int& argc, char** argv, std::string& filename) {
  CLI::App app{"", "bass"};
  app.add_option("-f,--file", filename, "path to ROM")->required();
  
  CLI11_PARSE(app, argc, argv);
  return 0;
}

int main(int argc, char** argv) {
  Bass bass;
  
  Frontend f{&bass};
  std::string filename;
  handle_args(argc, argv, filename);
  std::vector<u8> file = read_file(filename);
  bass.bus.pak->load_data(file);
  spdlog::set_level(spdlog::level::trace);
  

  for (size_t i = 0; i < 139; i++) {
    bass.cpu.step();
    SPDLOG_DEBUG("{} {:#010x}", i, bass.cpu.regs.r[15]);

    bass.bus.display_fields.DISPSTAT.VBLANK_FLAG = !bass.bus.display_fields.DISPSTAT.VBLANK_FLAG;
  }
  // exit(-1);
  // bool halted = false;
  while (f.state.running) {
    // if (!halted) {
    //   for (size_t c = 0; c < 280896; c++) {
    //     f.bass->cpu.step();
    //     // if(bass.cpu.regs.r[15] == 0x08003B64) {
    //     //   bass.cpu.print_registers();
    //     //   spdlog::set_level(spdlog::level::trace);
    //     //   halted = true;
    //     //   break;
    //     // }
    //     if (bass.bus.cycles_elapsed == 197120) {
    //       bass.ppu.draw();
    //       bass.bus.display_fields.DISPSTAT.VBLANK_FLAG = 1;
    //     }
    //     if (bass.bus.cycles_elapsed == 280896) { bass.bus.display_fields.DISPSTAT.VBLANK_FLAG = 0; }
    //   }
    // }
    bass.bus.cycles_elapsed = 0;
    f.handle_events();
    f.render_frame();
    // fmt::println("CE: {}", bass.bus.cycles_elapsed);
  }

  f.shutdown();
  
  return 0;
}

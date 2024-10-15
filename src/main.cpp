#include <spdlog/common.h>
#include "bass.hpp"
#include "bus.hpp"
#include "cli11/CLI11.hpp"
#include "common.hpp"
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
  spdlog::set_level(spdlog::level::off);

  // for (size_t i = 0; i < 1; i++) {
  //   // for (size_t i = 0; i < 269110; i++) {

  //   bass.cpu.step();
  //   if (bass.bus.cycles_elapsed == 197120) {
  //     bass.ppu.draw();
  //     bass.bus.display_fields.DISPSTAT.VBLANK_FLAG = 1;
  //     if (bass.bus.display_fields.DISPSTAT.VBLANK_IRQ_ENABLE) {
  //       fmt::println("shot off VBLANK interrupt");
  //       fmt::println("VBLANK FLAG: {}", +bass.bus.display_fields.DISPSTAT.VBLANK_FLAG);

  //       bass.bus.request_interrupt(Bus::LCD_VBLANK);
  //     }
  //   }
  //   if (bass.bus.cycles_elapsed == 280896) { bass.bus.display_fields.DISPSTAT.VBLANK_FLAG = 0; }
  //   // SPDLOG_DEBUG("{} x {:#010x}", i, bass.cpu.regs.r[15]);

  //   // bass.bus.display_fields.DISPSTAT.VBLANK_FLAG = !bass.bus.display_fields.DISPSTAT.VBLANK_FLAG;
  //   // fmt::println("R0:{:08X} R1:{:08X} R2:{:08X} R3:{:08X} R4:{:08X} R5:{:08X} R6:{:08X} R7:{:08X} R8:{:08X} R9:{:08X} R10:{:08X} R11:{:08X} R12:{:08X} R13:{:08X} R14:{:08X} R15:{:08X}
  //   CPSR:{:08X}",
  //   // bass.cpu.regs.r[0],
  //   // bass.cpu.regs.r[1],
  //   // bass.cpu.regs.r[2],
  //   // bass.cpu.regs.r[3],
  //   // bass.cpu.regs.r[4],
  //   // bass.cpu.regs.r[5],
  //   // bass.cpu.regs.r[6],
  //   // bass.cpu.regs.r[7],
  //   // bass.cpu.regs.r[8],
  //   // bass.cpu.regs.r[9],
  //   // bass.cpu.regs.r[10],
  //   // bass.cpu.regs.r[11],
  //   // bass.cpu.regs.r[12],
  //   // bass.cpu.regs.r[13],
  //   // bass.cpu.regs.r[14],

  //   // bass.cpu.regs.r[15],
  //   // bass.cpu.regs.CPSR.value
  //   // );
  // }
  // exit(-1);

  spdlog::set_level(spdlog::level::off);

  while (f.state.running) {
    if (!f.state.halted) {
      for (size_t c = 0; c < 280896; c++) {  // Run for a whole frame
        bass.cpu.step();
        bass.check_and_handle_interrupts();
      }
    }
    bass.bus.cycles_elapsed = 0;
  }

  return 0;
}

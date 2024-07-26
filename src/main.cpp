#include "CLI11.hpp"
#include "io.hpp"
#include "bass.hpp"
#include "spdlog/spdlog.h"

int handle_args(int argc, char** argv, std::string& filename) {
  CLI::App app{"", "bass"};
  app.add_option("-f,--file", filename, "path to ROM")->required();

  CLI11_PARSE(app, argc, argv);
  return 0;
}

int main(int argc, char** argv) {
  Bass bass;
  
  spdlog::set_level(spdlog::level::debug);
  
  std::string filename;
  handle_args(argc, argv, filename);
  
  std::vector<u8> file = read_file(filename);

  spdlog::debug("loaded rom with filename: {}", filename);

  bass.bus.pak->load_data(file);

  for (size_t i = 0; i < 4; i++) {
    bass.cpu.step();
    // fmt::println("\n");
  }
  // fmt::println("starting PC: {:#x}", bass.cpu.registers.pc);

  // fmt::println("{:#x} @ {:#x}", bass.bus.read32(bass.cpu.registers.pc),
  //              bass.cpu.registers.pc);

  // // while (f.state.active) {
  // //   f.handle_events();
  // //   f.render_frame();
  // // }

  // f.shutdown();

  return 0;
}
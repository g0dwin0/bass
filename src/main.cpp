#include "CLI11.hpp"
#include "io.hpp"
#include "bass.hpp"

using namespace Bass;

int handle_args(int& argc, char** argv, std::string& filename) {
  CLI::App app{"", "bass"};
  app.add_option("-f,--file", filename, "path to ROM")->required();
  
  CLI11_PARSE(app, argc, argv);
  return 0;
}

int main(int argc, char** argv) {

  static Bass::Instance bass;
  std::string filename;
  handle_args(argc, argv, filename);
  
  File f = read_file(filename);
  fmt::println("{}", filename);
  
  bass.bus.cart.load_data(f);



  fmt::println("game code: {}", bass.bus.cart.data.info.game_code);
  fmt::println("maker code: {}", bass.bus.cart.data.info.maker_code);
  fmt::println("software version: {}", bass.bus.cart.data.info.software_version);
  

  // while (f.state.active) {
  //   f.handle_events();
  //   f.render_frame();
  // }

  // f.shutdown();

  return 0;
}
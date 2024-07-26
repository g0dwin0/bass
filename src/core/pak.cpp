#include "core/pak.hpp"
#include <string>

#include "common.hpp"

void Pak::load_data(std::vector<u8>& f) {
  data = f;

  for (size_t i = 0; i < 192; i++) {  // i don't think i need to copy here
    info.header_bytes[i] = f.at(i);
  }

  log_cart_info();
};

void Pak::log_cart_info() {
  // for(size_t i = 0; i < 12; i++) {
  //   fmt::println("{}", this->info.game_title[i]);
  // }
  
  spdlog::info("GAME TITLE:       {}", std::string(info.game_title));
  spdlog::info("GAME CODE:        {}", std::string(info.game_code));
  spdlog::info("MAKER CODE:       {}", std::string(info.maker_code));
  spdlog::info("SOFTWARE VERSION: {}", info.software_version);
  // spdlog::info("GAME TITLE: {}", std::string(this->info.game_title));
}
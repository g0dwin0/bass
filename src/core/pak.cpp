#include "core/pak.hpp"

#include <string>

#include "common.hpp"

void Pak::load_data(std::vector<u8>& f) {
  data = f;

  for (size_t i = 0; i < 192; i++) {
    info.header_bytes[i] = f.at(i);
  }
  log_cart_info();
};

void Pak::log_cart_info() {
  

  spdlog::info("GAME TITLE:       {:.12}", info.game_title);
  spdlog::info("GAME CODE:        {:.4}", info.game_code);
  spdlog::info("MAKER CODE:       {:.2}", info.maker_code);
  spdlog::info("SOFTWARE VERSION: {}", info.software_version);
}
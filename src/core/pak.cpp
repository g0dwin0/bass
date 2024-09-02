#include "core/pak.hpp"
#include "common.hpp"
#include "spdlog/spdlog.h"

void Pak::load_data(std::vector<u8>& f) {
  for (size_t i = 0; i < f.size(); ++i) {
    data.at(i) = f.at(i);
  }

  for (size_t i = 0; i < 192; i++) {
    info.header_bytes[i] = f.at(i);
  }
  // log_cart_info();

  // fmt::println("pak sz after loading rom: {:#x}", data.size());
};

void Pak::log_cart_info() {
  spdlog::info("GAME TITLE:       {:.12}", info.game_title);
  spdlog::info("GAME CODE:        {:.4}", info.game_code);
  spdlog::info("MAKER CODE:       {:.2}", info.maker_code);
  spdlog::info("SOFTWARE VERSION: {}", info.software_version);
}
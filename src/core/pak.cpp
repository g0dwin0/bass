#include "core/pak.hpp"

#include <regex>

#include "common.hpp"

void Pak::load_data(std::vector<u8>& file_vector) {
  file_vector.resize(MAX_ROM_SIZE);

  data = file_vector;

  for (size_t i = 0; i < 192; i++) {
    info.header_bytes[i] = file_vector.at(i);
  }

  info.cartridge_save_type = get_cartridge_type();

  log_cart_info();
};

CartridgeType Pak::get_cartridge_type() const {
  std::string rom_str(reinterpret_cast<const char*>(data.data()), data.size());

  for (const auto& [type, regex] : cart_type_lookup_regex_map) {
    if (std::regex_search(rom_str, regex)) {
      return type;
    };
  }

  return UNKNOWN;
}

void Pak::log_cart_info() const {
  pak_logger->info("GAME TITLE:       {:.12}", info.game_title);
  pak_logger->info("GAME CODE:        {:.4}", info.game_code);
  pak_logger->info("MAKER CODE:       {:.2}", info.maker_code);
  pak_logger->info("SOFTWARE VERSION: {}", info.software_version);
  pak_logger->info("CARTRIDGE SAVE TYPE: {}", cart_type_lookup_map.at(info.cartridge_save_type));
}
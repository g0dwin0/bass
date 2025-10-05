#include "core/pak.hpp"

#include <regex>

#include "../../include/core/flash.hpp"
#include "common.hpp"

void Pak::load_data(std::vector<u8>& file_vector) {
  std::memcpy(data.data(), file_vector.data(), file_vector.size());

  for (size_t i = 0; i < 192; i++) {
    info.header_bytes[i] = file_vector.at(i);
  }

  info.cartridge_save_type = get_cartridge_type();

  if (info.cartridge_save_type == FLASH512) {
    flash_controller.manufacturer_id = 0x32;
    flash_controller.device_id       = 0x1B;
    info.uses_flash                  = true;
  }
  if (info.cartridge_save_type == FLASH1M) {
    flash_controller.manufacturer_id = 0x62;
    flash_controller.device_id       = 0x13;
    info.uses_flash                  = true;
  }

  log_cart_info();

  load_save();
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

void Pak::load_save() {
  std::string save_path = fmt::format("./saves/{:.4}.sav", info.game_code);
  fmt::println("save path: {}", save_path);
  if (std::filesystem::exists(save_path)) {
    std::ifstream save(save_path, std::ios::binary);

    auto save_vec = read_file(save_path);

    std::copy(save_vec.begin(), save_vec.end(), SRAM.begin());
    spdlog::info("save file loaded");
  } else {
    spdlog::info("no save file found");
  }
}
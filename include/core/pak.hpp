#pragma once
#include "spdlog/sinks/stdout_color_sinks.h"

static constexpr uint8_t ENTRY_POINT_LEN   = 4;
static constexpr uint8_t BITMAP_SIZE       = 156;
static constexpr uint8_t GAME_TITLE_LENGTH = 12;
static constexpr uint8_t GAME_CODE_LENGTH  = 4;
static constexpr uint8_t MAKER_CODE_LENGTH = 2;

#include <vector>

#include "common.hpp"

struct Pak {
  Pak() : data(0x2000000) {};

  std::shared_ptr<spdlog::logger> pak_logger = spdlog::stdout_color_mt("PAK");

  std::vector<u8> data;

  union {
    u8 header_bytes[192];
    struct {
      // 32bit ARM branch opcode
      u8 entry_point[ENTRY_POINT_LEN];

      // compressed bitmap, required.
      u8 logo[BITMAP_SIZE];

      // game title in ASCII (12 characters)
      char game_title[GAME_TITLE_LENGTH];

      // game code (shorthand 4 characters)
      char game_code[GAME_CODE_LENGTH];

      // maker code (shorthand 2 characters)
      char maker_code[MAKER_CODE_LENGTH];

      // fixed value (0x96 for valid roms)
      u8 RESERVED;

      // unit device code (0=GBA)
      u8 unit_code;

      // DEVICE TYPE
      u8 device_type;

      // should be 7 zeroes on valid roms
      u8 RESERVED[7];

      // software/game version number
      u8 software_version;

      // complement check / header checksum
      u8 complement_check;

      // reserved
      u8 RESERVED[2];
    };

  } info;

  void load_data(std::vector<u8>&);
  void log_cart_info();
};
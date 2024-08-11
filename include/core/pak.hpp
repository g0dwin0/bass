#pragma once
#define GAME_TITLE_LENGTH 12
#define GAME_CODE_LENGTH 4
#define MAKER_CODE_LENGTH 2
#include "common.hpp"
#include <vector>

struct Pak {
  Pak() {
    data.resize(0x2000000, 0);
  }

  std::vector<u8> data; // swap out for heap allocated array

  union {
    u8 header_bytes[192];
    struct {
      // 32bit ARM branch opcode
      u8 entry_point[4];

      // compressed bitmap, required.
      u8 logo[156];

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
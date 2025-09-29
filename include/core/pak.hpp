#pragma once
#include <filesystem>
#include <regex>
#include <vector>

#include "common.hpp"
#include "common/defs.hpp"
#include "enums.hpp"
#include "save/flash.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

static constexpr u8 BITMAP_SIZE       = 156;
static constexpr u8 GAME_TITLE_LENGTH = 12;
static constexpr u8 GAME_CODE_LENGTH  = 4;
static constexpr u8 MAKER_CODE_LENGTH = 2;
static constexpr u32 MAX_ROM_SIZE     = 32 * 1024 * 1024;

// https://dillonbeliveau.com/2020/06/05/GBA-FLASH.html
const std::unordered_map<CartridgeType, std::regex> cart_type_lookup_regex_map = {
    {  EEPROM,   std::regex("EEPROM_V")},
    {    SRAM,     std::regex("SRAM_V")},
    {   FLASH,    std::regex("FLASH_V")},
    {FLASH512, std::regex("FLASH512_V")},
    { FLASH1M,  std::regex("FLASH1M_V")},
};

const std::unordered_map<CartridgeType, std::string> cart_type_lookup_map = {
    {  EEPROM,       "EEPROM"},
    {    SRAM,         "SRAM"},
    {   FLASH,        "FLASH"},
    {FLASH512,     "FLASH512"},
    { FLASH1M,      "FLASH1M"},
    { UNKNOWN, "UNKNOWN/NONE"},
};

struct Pak {
  Pak() : data(MAX_ROM_SIZE), SRAM(0x20000) {
    std::fill(SRAM.begin(), SRAM.end(), 0xFF);

    u32 addr = 0x08000000;
    for (size_t i = 0; i < MAX_ROM_SIZE; i += 2) {
      *(u16*)(&data.at(i)) = (addr / 2) & 0xFFFF;
      addr += 2;
    }
  };
  ~Pak() {
    if (!std::filesystem::exists("./saves")) {
      if (!std::filesystem::create_directory("./saves")) {
        spdlog::error("could not create save directory, will not save.");
      }
    }

    std::ofstream save(fmt::format("saves/{:.4}.sav", info.game_code), std::ios::binary | std::ios::trunc);

    std::ostream_iterator<u8> output_iterator(save);
    std::copy(SRAM.begin(), SRAM.end(), output_iterator);

    save.close();
  }

  std::shared_ptr<spdlog::logger> pak_logger = spdlog::stdout_color_mt("PAK");

  std::vector<u8> data;
  std::vector<u8> SRAM;

  FlashController flash_controller;
  struct {
    union {
      u8 header_bytes[192];
      struct {
        // 32bit ARM branch opcode
        u32 entry_point;

        u8 logo[BITMAP_SIZE];

        char game_title[GAME_TITLE_LENGTH];

        char game_code[GAME_CODE_LENGTH];

        char maker_code[MAKER_CODE_LENGTH];

        // fixed value (0x96 for valid roms -- probably breaks user-made ROMs, ignore)
        u8 fixed_value;

        u8 unit_code;

        u8 device_type;

        // should be 7 zeroes on valid roms
        u8 RESERVED[7];

        u8 software_version;

        // complement check / header checksum
        u8 complement_check;

        u8 RESERVED[2];
      };
    };

    CartridgeType cartridge_save_type = UNKNOWN;
    bool uses_flash                   = false;
  } info;

  void load_data(std::vector<u8>&);
  void log_cart_info() const;
  void load_save();
  CartridgeType get_cartridge_type() const;
};
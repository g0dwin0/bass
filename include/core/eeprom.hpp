#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/defs.hpp"
#include "spdlog/common.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

enum struct EEPROM_MODE : u8 { NONE = 0, READ_REQUEST = 3, WRITE_REQUEST = 2 };
enum struct EEPROM_WRITE_SUB_MODE : u8 { WAIT_FOR_ADDRESS, WAIT_FOR_DATA, WAIT_FOR_END_BIT };
enum struct EEPROM_READ_SUB_MODE : u8 { WAIT_FOR_ADDRESS, WAIT_FOR_END_BIT };

const std::unordered_map<EEPROM_MODE, std::string> eeprom_str = {
    { EEPROM_MODE::READ_REQUEST,  "READ_REQUEST"},
    {EEPROM_MODE::WRITE_REQUEST, "WRITE_REQUEST"},
    {         EEPROM_MODE::NONE,          "NONE"},
};
struct EEPROMController {
  EEPROMController() : data(0x400) {
    cmd_bit_vec.reserve(128);

    // for(const auto& e : data) {
    //   assert(e == 0);
    // }

    eeprom_logger->set_level(spdlog::level::off);
    eeprom_logger->set_pattern("eeprom: %v");
  }

  std::shared_ptr<spdlog::logger> eeprom_logger = spdlog::stdout_color_mt("EEPROM");

  bool type_determined = false;

  // const u8 ADDRESS_SIZE = 14;
  const u8 ADDRESS_SIZE = 14;

  size_t selected_block = 0;
  size_t bits_to_read   = 0;
  // size_t BLOCK_SIZE      = 8;
  // size_t BLOCK_SIZE_BITS = 64;

  u64 write_val = 0;

  EEPROM_MODE mode                  = EEPROM_MODE::NONE;
  EEPROM_WRITE_SUB_MODE submode     = EEPROM_WRITE_SUB_MODE::WAIT_FOR_ADDRESS;
  EEPROM_READ_SUB_MODE read_submode = EEPROM_READ_SUB_MODE::WAIT_FOR_ADDRESS;

  std::vector<u8> cmd_bit_vec;
  void handle_write(u8 value);
  u8 handle_read();

  std::vector<u64> data;
};
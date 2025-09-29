#pragma once

#include <spdlog/logger.h>

#include "common/defs.hpp"
#include "enums.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"


enum struct FLASH_COMMAND : u8 {
  ENTER_CHIP_ID_MODE           = 0x90,
  EXIT_CHIP_ID_MODE            = 0xF0,
  PREPARE_TO_RECEIVE_ERASE_CMD = 0x80,
  ERASE_FULL_CHIP              = 0x10,
  ERASE_4KB_SECTOR             = 0x30,
  PREPARE_WRITE_BYTE           = 0xA0,
  SET_MEMORY_BANK              = 0xB0,
};

enum FLASH_MODE : u8 { CHIP_ID, READY, CMD_1, CMD_2, PREPARE_BYTE_WRITE, PREPARE_TO_ERASE, SET_MEM_BANK };
struct FlashController {

  FLASH_MODE mode = READY;
  u8 device_id;
  u8 manufacturer_id;
  u8 mem_bank = 0;

  CartridgeType flash_type;

  std::vector<u8> *SRAM;
  void print_flash_info();
  void handle_write(u32 address, u8 value);
  u8 handle_read(u32 address);
};
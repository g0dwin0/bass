#pragma once

#include "common/defs.hpp"
struct SRAM;

enum struct FLASH_COMMAND : u8 {
  ENTER_CHIP_ID_MODE           = 0x90,
  EXIT_CHIP_ID_MODE            = 0xF0,
  PREPARE_TO_RECEIVE_ERASE_CMD = 0x80,
  ERASE_FULL_CHIP              = 0x10,
  ERASE_4KB_SECTOR             = 0x30,
  PREPARE_WRITE_BYTE           = 0xA0,
  SET_MEMORY_BANK              = 0xB0,
};

enum FLASH_MODE : u8 {
  CHIP_ID,
  READY
};

struct FlashController {
  SRAM *sram = nullptr;
  void handle_write();
  void handle_read();
};
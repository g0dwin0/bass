#include "save/flash.hpp"

#include "spdlog/fmt/bundled/base.h"

void FlashController::print_flash_info() {
  fmt::println("FLASH TYPE: {}", flash_type == FLASH512 ? "FLASH512" : "FLASH1M");
  fmt::println("FLASH DEVICE ID: {:#02X}", device_id);
  fmt::println("FLASH MANUFACTURER ID: {:#02X}", manufacturer_id);
}

void FlashController::handle_write(u32 address, u8 value) {
  FLASH_COMMAND cmd = static_cast<FLASH_COMMAND>(value);

  if (mode == SET_MEM_BANK && address == 0X0E000000) {
    mem_bank = value;
    fmt::println("set memory bank to: {}", mem_bank);
    mode = READY;
    return;
  }

  if (mode == READY) {
    if ((u8)cmd == 0xAA && address == 0xE005555) mode = CMD_1;
    fmt::println("setting mode to CMD_1");
    return;
  }

  if (mode == CMD_1) {
    if ((u8)cmd == 0x55 && address == 0xE002AAA) mode = CMD_2;
    fmt::println("setting mode to CMD_2");
    return;
  }

    switch (cmd) {
      case FLASH_COMMAND::ENTER_CHIP_ID_MODE: {
        fmt::println("enter chip id mode");
        mode = CHIP_ID;
        break;
      }
      case FLASH_COMMAND::EXIT_CHIP_ID_MODE: {
        fmt::println("exit chip id");
        mode = READY;
        break;
      }
      case FLASH_COMMAND::PREPARE_TO_RECEIVE_ERASE_CMD: {
        fmt::println("prepare 2 receive erase cmd");
        mode = PREPARE_TO_ERASE;
        break;
      }
      case FLASH_COMMAND::ERASE_FULL_CHIP: {
        fmt::println("erase full chip");
        if (mode == PREPARE_TO_ERASE) {
          std::fill(SRAM->begin(), SRAM->end(), 0xFF);
          mode = READY;
        } else {
          fmt::println("prepare to erase was not set, doing nothing.");
        }
        break;
      }
      case FLASH_COMMAND::ERASE_4KB_SECTOR: {
        fmt::println("erase 4kb sector -- {:#010x}", address);

        if (mode == PREPARE_TO_ERASE) {
          u16 page = address & 0xF000;

          for (size_t i = 0; i < 0x1000; i++) {
            SRAM->at((mem_bank * 0x10000) + page + i) = 0xff;
          }

          fmt::println("sector erased");
        }
        fmt::println("mode is not prepare 2 erase, doing nothing");
        mode = READY;
        break;
      }
      case FLASH_COMMAND::PREPARE_WRITE_BYTE: {
        fmt::println("prepare write byte");
        mode = PREPARE_BYTE_WRITE;
        break;
      }
      case FLASH_COMMAND::SET_MEMORY_BANK: {
        fmt::println("set memory bank");
        mode = SET_MEM_BANK;
        break;
      }
      default: {
        if (mode == PREPARE_BYTE_WRITE) {
          mode                                                 = READY;
          SRAM->at((mem_bank * 0x10000) + (address % 0x10000)) = value;
        } else {
          fmt::println("invalid cmd: {:#02X}", static_cast<u8>(cmd));
        }
      }
    }
  // }
}

u8 FlashController::handle_read(u32 address) {
  if (mode == CHIP_ID) {
    fmt::println("returning chip info");
    if (address == 0x0E000000) return manufacturer_id;
    if (address == 0x0E000001) return device_id;
  }
  u8 val = SRAM->at((mem_bank * 0x10000) + address % 0x10000);
  fmt::println("[FLASH] READ: {}:{:#010x} -> {:#08x}", mem_bank, address, val);
  return val;
}
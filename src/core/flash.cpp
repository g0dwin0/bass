#include "../../include/core/flash.hpp"

#include "spdlog/fmt/bundled/base.h"

void FlashController::print_flash_info() {
  fmt::println("FLASH DEVICE ID: {:#02X}", device_id);
  fmt::println("FLASH MANUFACTURER ID: {:#02X}", manufacturer_id);
}

void FlashController::handle_write(u32 address, u8 value) {
  fmt::println("[WC] {:#08x} -> {:#02X}", address, value);
  auto cmd = static_cast<FLASH_COMMAND>(value);

  if (mode == FLASH_MODE::PREPARE_BYTE_WRITE) {
    mode = FLASH_MODE::NONE;
    fmt::println("[W] {}:{:#010x} -> {:#08x}", mem_bank, address, value);
    SRAM->at((mem_bank * 0x10000) + (address % 0x10000)) = value;

    return;
  }

  if (mode == FLASH_MODE::SET_MEM_BANK && address == 0X0E000000) {
    mem_bank = value;
    fmt::println("set memory bank to: {}", mem_bank);
    mode = FLASH_MODE::NONE;
    return;
  }

  if (step == FLASH_STEP::READY) {
    if (static_cast<u8>(cmd) == 0xAA && address == 0xE005555) step = FLASH_STEP::CMD_1;
    fmt::println("setting mode to CMD_1");
    return;
  }

  if (step == FLASH_STEP::CMD_1) {
    if (static_cast<u8>(cmd) == 0x55 && address == 0xE002AAA) step = FLASH_STEP::CMD_2;
    fmt::println("setting mode to CMD_2");
    return;
  }

  if (step == FLASH_STEP::CMD_2) {
    switch (cmd) {
      case FLASH_COMMAND::ENTER_CHIP_ID_MODE: {
        fmt::println("enter chip id mode");
        mode = FLASH_MODE::CHIP_ID;
        break;
      }
      case FLASH_COMMAND::EXIT_CHIP_ID_MODE: {
        fmt::println("exit chip id");
        mode = FLASH_MODE::NONE;
        break;
      }
      case FLASH_COMMAND::PREPARE_TO_RECEIVE_ERASE_CMD: {
        fmt::println("prepare 2 receive erase cmd");
        mode = FLASH_MODE::PREPARE_TO_ERASE;
        break;
      }
      case FLASH_COMMAND::ERASE_FULL_CHIP: {
        fmt::println("erasing full chip");
        if (mode == FLASH_MODE::PREPARE_TO_ERASE) {
          std::fill(SRAM->begin(), SRAM->end(), 0xFF);
          mode = FLASH_MODE::NONE;
        } else {
          fmt::println("prepare to erase was not set, doing nothing.");
        }
        break;
      }
      case FLASH_COMMAND::ERASE_4KB_SECTOR: {
        fmt::println("erase 4kb sector -- {:#010x}", address);

        if (mode == FLASH_MODE::PREPARE_TO_ERASE) {
          u16 page = address & 0xF000;

          for (size_t i = 0; i < 0x1000; i++) {
            SRAM->at((mem_bank * 0x10000) + page + i) = 0xff;
          }

          fmt::println("sector erased");
        }
        fmt::println("mode is not prepare 2 erase, doing nothing");
        mode = FLASH_MODE::NONE;
        break;
      }
      case FLASH_COMMAND::PREPARE_WRITE_BYTE: {
        fmt::println("prepare write byte");
        mode = FLASH_MODE::PREPARE_BYTE_WRITE;
        break;
      }
      case FLASH_COMMAND::SET_MEMORY_BANK: {
        fmt::println("set memory bank");
        mode = FLASH_MODE::SET_MEM_BANK;
        break;
      }
      default: {
        fmt::println("invalid cmd: {:#02X} returning to READY", static_cast<u8>(cmd));
        mode = FLASH_MODE::NONE;
      }
    }
    step = FLASH_STEP::READY;
  }
}

u8 FlashController::handle_read(u32 address) {
  if (mode == FLASH_MODE::CHIP_ID) {
    fmt::println("returning chip info");
    if (address == 0x0E000000) return manufacturer_id;
    if (address == 0x0E000001) return device_id;
  }
  u8 val = SRAM->at((mem_bank * 0x10000) + (address % 0x10000));
  fmt::println("[FLASH] READ: {}:{:#010x} -> {:#08x}", mem_bank, address, val);
  return val;
}
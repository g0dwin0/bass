#include "bus.hpp"

#include <cstdio>

#include "common/align.hpp"
#include "common/bytes.hpp"
#include "common/defs.hpp"
#include "common/test_structs.hpp"
#include "cpu.hpp"
#include "enums.hpp"
#include "flash.hpp"
#include "labels.hpp"
#include "pak.hpp"
#include "registers.hpp"
#include "sched/sched.hpp"

void Bus::request_interrupt(INTERRUPT_TYPE type) { interrupt_control.IF.v |= 1 << static_cast<u8>(type); }

u8 Bus::read8(u32 address, ACCESS_TYPE access_type) {
  cycles_elapsed += 1;
  tick_timers(1);
  // u8 word_alignment_offset = (address & 3);
#ifdef SST_TEST_MODE
  // TODO: make a separate function for these
  fmt::println("requested [8] read at {:#010x}", address);
  for (auto& transaction : transactions) {
    if (transaction.size == 2 && transaction.kind != WRITE && (transaction.addr == address + 1 || transaction.addr == address)) {
      transaction.accessed = true;
      fmt::println("misaligned READ8 -- SST");
      fmt::println("{:#010X} -> {:#010X}", address, transaction.data >> ((transaction.addr % 2) * 8));
      return transaction.data >> ((transaction.addr % 2) * 8);
    }

    if (transaction.size != 1 && transaction.kind != WRITE) continue;

    fmt::println("[R8] {:#010x} -> {:#010x}", address, transaction.data);

    if (address == transaction.addr) {
      transaction.accessed = true;
      fmt::println("returning: {:#010x}", transaction.data);
      return transaction.data;
    }

    if (address == transaction.base_addr) {
      return transaction.opcode;
    }
  }

  return address;
#else
  u32 v = 0x0;

  switch (static_cast<REGION>(address >> 24)) {
    case REGION::BIOS: {
      if (address >= 0x00003FFF) return cpu->pipeline.fetch >> (address & 1) * 8;

      if (cpu->regs.r[15] > 0x00003FFF) {
        v = std::rotr(bios_open_bus, (address & 3) * 8);
      } else {
        v = BIOS.at(address);
      }

      break;
    }

    case REGION::EWRAM: {
      v = EWRAM.at(address % 0x40000);
      break;
    }

    case REGION::IWRAM: {
      v = IWRAM.at(address % 0x8000);
      break;
    }

    case REGION::IO: {
      v = io_read(address);
      break;
    }

    case REGION::BG_OBJ_PALETTE: {
      // BG/OBJ Palette RAM
      return ppu->PALETTE_RAM.at(address % 0x400);
    }

    case REGION::VRAM: {
      u64 norm_addr = address & 0x1ffffu;

      if (norm_addr >= 0x18000) {
        norm_addr -= 0x8000;
      }

      v = *(uint8_t*)(&ppu->VRAM.at((norm_addr)));
      break;
    }

    case REGION::OAM: {
      return OAM.at(address % 0x400);
    }

    case REGION::PAK_WS0_0:
    case REGION::PAK_WS0_1: {
      // game pak read (ws0)
      cycles_elapsed += get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS0);
      tick_timers(get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS0));
      v = pak->data.at(address - 0x08000000);
      break;
    }

    case REGION::PAK_WS1_0:
    case REGION::PAK_WS1_1: {
      // game pak read (ws1)
      cycles_elapsed += get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS1);
      tick_timers(get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS1));
      v = pak->data.at(address - 0x0A000000);
      break;
    }

    case REGION::PAK_WS2_0:
    case REGION::PAK_WS2_1: {
      // game pak read (ws2)
      cycles_elapsed += get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS2);
      tick_timers(get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS2));
      v = pak->data[address - 0x0C000000];
      break;
    }

    case REGION::SRAM_0:
    case REGION::SRAM_1: {
      if (pak->info.uses_flash) {
        return pak->flash_controller.handle_read(address);
      }
      v = pak->SRAM[address % 0x8000];

      break;
    }

    default: {
      // fmt::println("[R8] unused/oob memory read: {:#X}", address);
      // return 0xFF;
      return cpu->pipeline.fetch >> (address & 1) * 8;
    }
  }

  // mem_logger->info("[R8] {:#010x} [{}] => {:#04x}", address, get_label(address), v);
  return v;

#endif
};
u16 Bus::read16(u32 address, ACCESS_TYPE access_type) {
  cycles_elapsed += 1;
  tick_timers(1);
  u32 _address = address;
  // u8 misaligned_by         = (address & 1) * 8;
  u8 word_alignment_offset = (address & 3);
  (void)_address;

  // (void)(word_alignment_offset);

  address = align(address, HALFWORD);

#ifdef SST_TEST_MODE
  fmt::println("requested [16] read at {:#010x}", address);
  for (auto& transaction : transactions) {
    if (transaction.size != 2 && transaction.kind != WRITE) continue;

    // if (address == align(transaction.addr, HALFWORD)) {
    fmt::println("[R16] {:#010x} -> {:#010x}", address, transaction.data);
    transaction.accessed = true;
    return transaction.data;
    // }

    if (address == transaction.base_addr) {
      return transaction.opcode;
    }
  }

  return address;

#else
  // fmt::println("[R16] {:#010x}", address);
  u32 v = 0;
  switch ((REGION)(address >> 24)) {
    case REGION::BIOS: {
      if (address >= 0x00003FFF) {
        v = cpu->pipeline.fetch;
        break;
      }
      if (cpu->regs.r[15] > 0x00003FFF) {  // BIOS only readable when PC in BIOS range
        //         v = std::rotr(bios_open_bus, 0);
        (void)word_alignment_offset;

        u8 a = bios_open_bus & 0xFF;
        u8 b = (bios_open_bus >> 8) & 0xFF;
        u8 c = (bios_open_bus >> 16) & 0xFF;
        u8 d = (bios_open_bus >> 24) & 0xFF;

        (void)a;
        (void)b;
        (void)c;
        (void)d;

        switch (word_alignment_offset) {
          case 0: {
            v = bios_open_bus;
            break;
          }
          case 1: {
            v = std::rotr((u16)bios_open_bus, 16);
            break;
          }
          case 2: {
            v = (bios_open_bus >> 16);
            break;
          }
          case 3: {
            v = bios_open_bus >> 16;
            break;
          }
          default: {
            v = bios_open_bus;
            break;
          }
        }
      } else {
        v = *(u16*)(&BIOS[address]);
      }
      break;
    }

    case REGION::EWRAM: {
      cycles_elapsed += 2;  // 3/3/6 -- mem access already accounts for 1
      tick_timers(2);
      v = *(u16*)(&EWRAM[address % 0x40000]);
      break;
    }

    case REGION::IWRAM: {
      cycles_elapsed += 1;
      tick_timers(1);
      v = *(u16*)(&IWRAM[address % 0x8000]);
      break;
    }

    case REGION::IO: {
      for (u32 i = 0; i < 2; i++) {
        v |= (io_read(address + i) << (8 * i));  // NOLINT
      }
      break;
    }
    case REGION::BG_OBJ_PALETTE: {
      return *(uint16_t*)(&ppu->PALETTE_RAM.at(address % 0x400));
    }

    case REGION::VRAM: {
      // fmt::println("HIT VRAM");
      u64 norm_addr = address & 0x1FFFFu;

      if (norm_addr >= 0x18000) norm_addr -= 0x8000u;
      return *(uint16_t*)(&ppu->VRAM[(norm_addr)]);
    }

    case REGION::OAM: {
      return *(uint16_t*)(&OAM[(address % 0x400)]);
    }

    case REGION::PAK_WS0_0:
    case REGION::PAK_WS0_1: {
      // game pak read (ws0)
      cycles_elapsed += get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS0);
      tick_timers(get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS0));
      v = *(uint16_t*)(&pak->data[address - 0x08000000]);
      break;
    }

    case REGION::PAK_WS1_0:
    case REGION::PAK_WS1_1: {
      // game pak read (ws1)
      cycles_elapsed += get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS1);
      tick_timers(get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS1));
      v = *(u16*)(&pak->data[address - 0x0A000000]);
      break;
    }

    case REGION::PAK_WS2_0:
    case REGION::PAK_WS2_1: {
      if(address >= 0x0d000000 && pak->eeprom.bits_to_read != 0) return pak->eeprom.handle_read();

      // game pak read (ws2);
      cycles_elapsed += get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS2);
      tick_timers(get_rom_cycles_by_waitstate(access_type, WAITSTATE::WS2));
      v = *(u16*)(&pak->data[address - 0x0C000000]);
      break;
    }

    case REGION::SRAM_0:
    case REGION::SRAM_1: {
      return pak->SRAM.at(_address % 0x8000) * 0x0101;
      // if(_address >= 0x0E000000) fmt::println("{:#010x}: {:#010x} m: {} -> {:#010x}", _address, v, misaligned_by, std::rotr(v, misaligned_by));
      break;
    }

    default: {
      v = cpu->pipeline.fetch;
      break;
    }
  }

  return v;
  // return static_cast<u16>(std::rotr(v, misaligned_by));
  // bus_logger->info("[R16] {:#010X} => {:#010x}", address, v);

#endif
};
u32 Bus::read32(u32 address, [[gnu::unused]] ACCESS_TYPE access_type) {
  cycles_elapsed += 1;
  tick_timers(1);

  // Scheduler::step(1);
  u32 _address = address;
  (void)_address;
  address = align(address, WORD);

#ifdef SST_TEST_MODE
  fmt::println("requested [32] read at {:#010x}", address);
  for (auto& transaction : transactions) {
    if (transaction.accessed) continue;
    if (transaction.size != 4 && transaction.kind != WRITE) continue;

    if (address == align(transaction.addr, WORD)) {
      fmt::println("[R32] [TX_ID: {}] {:#010x} -> {:#010x}", transaction.id, address, transaction.data);

      transaction.accessed = true;
      return transaction.data;
    }

    if (address == transaction.base_addr) {
      return transaction.opcode;
    }
  }
  return address;

#else

  // SPDLOG_DEBUG("[R32] {:#010x}", address);
  u32 v = 0;

  switch (static_cast<REGION>(address >> 24)) {
    case REGION::BIOS: {
      if (address >= 0x00003FFF) return cpu->pipeline.fetch;
      // BIOS read
      if (cpu->regs.r[15] > 0x00003FFF) {
        v = bios_open_bus;
      } else {
        v = *(uint32_t*)(&BIOS[address]);
      };
      break;
    }

    case REGION::EWRAM: {
      cycles_elapsed += 4;
      tick_timers(4);
      // get_wram_waitstates();
      v = *(u32*)(&EWRAM[address % 0x40000]);
      break;
    }

    case REGION::IWRAM: {
      v = *(u32*)(&IWRAM[address % 0x8000]);
      break;
    }

    case REGION::IO: {
      for (u32 i = 0; i < 4; i++) {
        v |= (io_read(address + i) << (8 * i));
      }
      break;
    }

    case REGION::BG_OBJ_PALETTE: {
      v = *(u32*)(&ppu->PALETTE_RAM[address % 0x400]);
      break;
    }

    case REGION::VRAM: {
      u64 max_addr = address & 0x1FFFFu;

      if (max_addr >= 0x18000) max_addr -= 0x8000u;

      v = *(u32*)(&ppu->VRAM.at(max_addr));
      break;
    }

    case REGION::OAM: {
      v = *(u32*)(&OAM[address % 0x400]);
      break;
    }

    case REGION::PAK_WS0_0:
    case REGION::PAK_WS0_1: {
      cycles_elapsed += get_rom_cycles_by_waitstate(ACCESS_TYPE::NON_SEQUENTIAL, WAITSTATE::WS0);
      cycles_elapsed += get_rom_cycles_by_waitstate(ACCESS_TYPE::SEQUENTIAL, WAITSTATE::WS0);
      tick_timers(get_rom_cycles_by_waitstate(ACCESS_TYPE::NON_SEQUENTIAL, WAITSTATE::WS0));
      tick_timers(get_rom_cycles_by_waitstate(ACCESS_TYPE::SEQUENTIAL, WAITSTATE::WS0));

      v = *(uint32_t*)(&pak->data[address - 0x08000000]);
      break;
    }

    case REGION::PAK_WS1_0:
    case REGION::PAK_WS1_1: {
      cycles_elapsed += get_rom_cycles_by_waitstate(ACCESS_TYPE::NON_SEQUENTIAL, WAITSTATE::WS1);
      cycles_elapsed += get_rom_cycles_by_waitstate(ACCESS_TYPE::SEQUENTIAL, WAITSTATE::WS1);

      tick_timers(get_rom_cycles_by_waitstate(ACCESS_TYPE::NON_SEQUENTIAL, WAITSTATE::WS1));
      tick_timers(get_rom_cycles_by_waitstate(ACCESS_TYPE::SEQUENTIAL, WAITSTATE::WS1));

      v = *(uint32_t*)(&pak->data[address - 0x0A000000]);
      break;
    }

    case REGION::PAK_WS2_0:
    case REGION::PAK_WS2_1: {
      cycles_elapsed += get_rom_cycles_by_waitstate(ACCESS_TYPE::NON_SEQUENTIAL, WAITSTATE::WS2);
      cycles_elapsed += get_rom_cycles_by_waitstate(ACCESS_TYPE::SEQUENTIAL, WAITSTATE::WS2);

      tick_timers(get_rom_cycles_by_waitstate(ACCESS_TYPE::NON_SEQUENTIAL, WAITSTATE::WS2));
      tick_timers(get_rom_cycles_by_waitstate(ACCESS_TYPE::SEQUENTIAL, WAITSTATE::WS2));

      v = *(u32*)(&pak->data[address - 0x0C000000]);
      break;
    }

    case REGION::SRAM_0:
    case REGION::SRAM_1: {
      v = pak->SRAM.at((_address) % 0x8000) * 0x01010101;
      break;
    }

    default: {
      v = cpu->pipeline.fetch;
      break;
    }
  }
  // mem_logger->info("[R32] {} => {:#010x}", get_label(address), v);
  // return std::rotr(v, misaligned_by);
  return v;
#endif
};

void Bus::write8(u32 address, u8 value) {
#ifdef SST_TEST_MODE
  fmt::println("write [8] at {:#010x} -> {:#010x}", address, value);
  for (auto& transaction : transactions) {
    if (transaction.size != 1 || transaction.kind != WRITE) continue;
    if (address == transaction.addr && value == transaction.data) {
      transaction.accessed = true;
    }
  }
  return;
#else
  cycles_elapsed += 1;
  tick_timers(1);
  switch ((REGION)(address >> 24)) {
    case REGION::EWRAM: {
      EWRAM.at(address % 0x40000) = value;
      break;
    }

    case REGION::IWRAM: {
      IWRAM.at(address % 0x8000) = value;
      break;
    }

    case REGION::IO: {
      // IO
      io_write(address, value);
      break;
    }

    case REGION::BG_OBJ_PALETTE: {
      fmt::println("{:#010X} -- {:#010X}", address, value);
      *(uint16_t*)&ppu->PALETTE_RAM.at((address % 0x400) & ~1) = value * 0x101;

      return;
    }

    case REGION::VRAM: {
      u64 max_addr = address & 0x1FFFFu;

      if (max_addr >= 0x18000) max_addr -= 0x8000u;

      if (max_addr >= 0x10000) return;

      // fmt::println("value:{:#04X}", value);
      *(uint16_t*)&ppu->VRAM.at(max_addr & ~1) = value * 0x101;
      return;
    }

    case REGION::SRAM_0:
    case REGION::SRAM_1: {
      if (pak->info.uses_flash) {
        pak->flash_controller.handle_write(address, value);
        return;
      }

      pak->SRAM.at(address % 0x8000) = value;
      break;
    }

    default: {
      // spdlog::warn("[W8] unused/oob memory write: {:#X}", address);

      return;
      // break;
    }
  }
    // mem_logger->info("[W8] {} => {:#x}", get_label(address), value);

#endif
}

void Bus::write16(u32 address, u16 value) {
  u32 _address = address;
  (void)_address;
  address = align(address, HALFWORD);
  cycles_elapsed += 1;
  tick_timers(1);
#ifdef SST_TEST_MODE
  bus_logger->debug("write [16] at {:#010x} -> {:#010x}", address, value);
  fmt::println("write [16] at {:#010x} -> {:#010x}", address, value);
  // #ifdef SST_TEST_MODE
  for (auto& transaction : transactions) {
    if (transaction.size != 2 || transaction.kind != WRITE) continue;
    u32 aligned_tx_addr = align(transaction.addr, HALFWORD);
    if (address == aligned_tx_addr && value == transaction.data) {
      fmt::println("valid write");
      transaction.accessed = true;
      return;
    }
  }

  fmt::println("write not in list: {:#010X}", address);
  // transaction.print();
  exit(-1);
  return;
#else

  switch ((REGION)(address >> 24)) {
    case REGION::EWRAM: {
      *(uint16_t*)(&EWRAM[address % 0x40000]) = value;
      break;
    }

    case REGION::IWRAM: {
      *(uint16_t*)(&IWRAM[address % 0x8000]) = value;

      break;
    }

    case REGION::IO: {
      // IO
      write8(address, value & 0xff);
      write8(address + 1, ((value & 0xff00) >> 8));
      return;
      break;
    }

    case REGION::BG_OBJ_PALETTE: {
      // BG/OBJ Palette RAM
      *(uint16_t*)(&ppu->PALETTE_RAM[address % 0x400]) = value;
      break;
    }

    case REGION::VRAM: {
      // if (address >= 0x06018000 && ppu->display_fields.DISPCNT.BG_MODE >= 3) return;

      u64 norm_addr = address & 0x1FFFFu;

      if (norm_addr >= 0x18000) norm_addr -= 0x8000u;

      *(uint16_t*)(&ppu->VRAM.at(norm_addr)) = value;
      break;
    }

    case REGION::OAM: {
      // OAM
      *(uint16_t*)(&OAM.at(address % 0x400)) = value;
      ppu->state.oam_changed                 = true;
      break;
    }

    

        case REGION::PAK_WS0_0:
    case REGION::PAK_WS0_1: {
      fmt::println("[ws0] eeprom: {:#010x}", value);
      break;
    }
        case REGION::PAK_WS1_0:
    case REGION::PAK_WS1_1: {
      fmt::println("[ws1] eeprom: {:#010x}", value);
      break;
    }

    case REGION::PAK_WS2_0:
    case REGION::PAK_WS2_1: {
      fmt::println("[ws2] eeprom: {:#010x}", value & 0b1);
      pak->eeprom.handle_write(value & 0b1);
      break;
    }


    case REGION::SRAM_0:
    case REGION::SRAM_1: {
      pak->SRAM.at(_address % 0x8000) = std::rotr(value, _address * 8) & 0xFF;
      break;
    }

    default: {
      // fmt::println("unused/oob memory write: {:#X}", address);
      return;
      // break;
    }
  }
    // mem_logger->info("[W16] {} => {:#x}", get_label(address), value);

#endif
}

void Bus::write32(u32 address, u32 value) {
  u32 _address = address;
  (void)_address;
  address = align(address, WORD);
  cycles_elapsed += 1;
  tick_timers(1);

#ifdef SST_TEST_MODE
  fmt::println("write [32] at {:#010x} -> {:#010x}", address, value);
  for (auto& transaction : transactions) {
    if (transaction.size != 4 || transaction.kind != WRITE) continue;

    u32 aligned_tx_addr = align(transaction.addr, WORD);
    if (address == aligned_tx_addr && value == transaction.data) {
      fmt::println("[W32] [TX_ID: {}] {:#010x} -> {:#010x}", transaction.id, address, transaction.data);

      transaction.accessed = true;
      return;
    }
  }

  fmt::println("write not in list");
  // transaction.print();
  exit(-1);

  return;
#else

  switch ((REGION)(address >> 24)) {
    case REGION::EWRAM: {
      *(uint32_t*)(&EWRAM.at(address % 0x40000)) = value;
      break;
    }

    case REGION::IWRAM: {
      *(uint32_t*)(&IWRAM.at(address % 0x8000)) = value;
      break;
    }

    case REGION::IO: {
      write16(address, value & 0x0000FFFF);
      write16(address + 2, (value & 0xFFFF0000) >> 16);
      // assert(0);
      return;
      break;
    }

    case REGION::BG_OBJ_PALETTE: {
      *(uint32_t*)(&ppu->PALETTE_RAM[(address % 0x400)]) = value;
      break;
    }

    case REGION::VRAM: {
      u64 norm_addr = address & 0x1FFFFu;

      if (norm_addr >= 0x18000) norm_addr -= 0x8000u;

      *(uint32_t*)(&ppu->VRAM.at(norm_addr)) = value;
      break;
    }

    case REGION::OAM: {
      *(uint32_t*)(&OAM[(address % 0x400)]) = value;
      ppu->state.oam_changed                = true;
      break;
    }

    case REGION::SRAM_0:
    case REGION::SRAM_1: {
      pak->SRAM.at(_address % 0x8000) = std::rotr(value, _address * 8) & 0xFF;
      break;
    }

    default: {
      // fmt::println("[W32] unused/oob memory write: {:#010x}", address);
      return;
      // break;
    }
  }
    // mem_logger->info("[W32] {} => {:#010x}", get_label(address), value);

#endif
}

u8 Bus::io_read(u32 address) {
  u8 retval = 0x0;

  switch (address) {
    case DISPCNT:
    case DISPCNT + 1: {
      retval = read_byte(ppu->display_fields.DISPCNT.v, address % 2);
      break;
    }
    case GREEN_SWAP:
    case GREEN_SWAP + 1: {
      retval = read_byte(ppu->display_fields.GREEN_SWAP.v, address % 2);
      break;
    }
    case DISPSTAT:
    case DISPSTAT + 1: {
      retval = read_byte(ppu->display_fields.DISPSTAT.v, address % 2);
      // bus_logger->info("disp stat read");
      break;
    }
    case VCOUNT:
    case VCOUNT + 1: {
      retval = read_byte(ppu->display_fields.VCOUNT.v, address % 2);
      break;
    }
    case BG0CNT:
    case BG0CNT + 1: {
      retval = read_byte(ppu->display_fields.BG0CNT.v, address % 0x2);
      break;
    }
    case BG1CNT:
    case BG1CNT + 1: {
      retval = read_byte(ppu->display_fields.BG1CNT.v, address % 0x2);
      break;
    }
    case BG2CNT:
    case BG2CNT + 1: {
      retval = read_byte(ppu->display_fields.BG2CNT.v, address % 0x2);
      break;
    }
    case BG3CNT:
    case BG3CNT + 1: {
      retval = read_byte(ppu->display_fields.BG3CNT.v, address % 0x2);
      break;
    }
    case WININ:
    case WININ + 1: {
      retval = read_byte(ppu->display_fields.WININ.v, address % 0x2);
      break;
    }

    case WINOUT:
    case WINOUT + 1: {
      retval = read_byte(ppu->display_fields.WINOUT.v, address % 0x2);
      break;
    }

    case BLDCNT:
    case BLDCNT + 1: {
      retval = read_byte(ppu->display_fields.BLDCNT.v, address % 0x2);
      break;
    }

    case BLDALPHA:
    case BLDALPHA + 1: {
      retval = read_byte(ppu->display_fields.BLDALPHA.v, address % 0x2);
      break;
    }

    case SOUND1CNT_L:
    case SOUND1CNT_L + 1: {
      retval = read_byte(apu->sound_registers.SOUND1CNT_L.v, address % 0x2);
      break;
    }

    case SOUND1CNT_X:
    case SOUND1CNT_X + 1:
    case SOUND1CNT_X + 2:
    case SOUND1CNT_X + 3: {
      retval = read_byte(apu->sound_registers.SOUND1CNT_X.v, address % 0x4);
      break;
    }

    case SOUND2CNT_L:
    case SOUND2CNT_L + 1: {
      retval = read_byte(apu->sound_registers.SOUND2CNT_L.v, address % 0x2);
      break;
    }

    case SOUND3CNT_L:
    case SOUND3CNT_L + 1: {
      retval = read_byte(apu->sound_registers.SOUND3CNT_L.v, address % 0x2);
      break;
    }

    case SOUND1CNT_H:
    case SOUND1CNT_H + 1: {
      retval = read_byte(apu->sound_registers.SOUND1CNT_H.v, address % 2);
      break;
    }

    case SOUND2CNT_H:
    case SOUND2CNT_H + 1: {
      retval = read_byte(apu->sound_registers.SOUND2CNT_H.v, address % 2);
      break;
    }

    case SOUND3CNT_H:
    case SOUND3CNT_H + 1: {
      retval = read_byte(apu->sound_registers.SOUND3CNT_H.v, address % 2);
      break;
    }

    case SOUND3CNT_X:
    case SOUND3CNT_X + 1: {
      retval = read_byte(apu->sound_registers.SOUND3CNT_X.v, address % 2);
      break;
    }

    case SOUND4CNT_L:
    case SOUND4CNT_L + 1:
    case SOUND4CNT_L + 2:
    case SOUND4CNT_L + 3: {
      retval = read_byte(apu->sound_registers.SOUND4CNT_L.v, address % 4);
      break;
    }

    case SOUND4CNT_H:
    case SOUND4CNT_H + 1: {
      retval = read_byte(apu->sound_registers.SOUND4CNT_H.v, address % 2);
      break;
    }

    case SOUNDCNT_L:
    case SOUNDCNT_L + 1: {
      retval = read_byte(apu->sound_registers.SOUNDCNT_L.v, address % 2);
      break;
    }

    case SOUNDCNT_H:
    case SOUNDCNT_H + 1: {
      retval = read_byte(apu->sound_registers.SOUNDCNT_H.v, address % 2);
      break;
    }

    case SOUNDCNT_X:
    case SOUNDCNT_X + 1:
    case SOUNDCNT_X + 2:
    case SOUNDCNT_X + 3: {
      retval = read_byte(apu->sound_registers.SOUNDCNT_X.v, address % 4);
      break;
    }

    case SOUNDBIAS:
    case SOUNDBIAS + 1: {
      retval = read_byte(system_control.sound_bias, address % 2);
      break;
    }

    case WAVE_RAM0_L:
    case WAVE_RAM0_L + 1:
    case WAVE_RAM0_H:
    case WAVE_RAM0_H + 1:
    case WAVE_RAM1_L:
    case WAVE_RAM1_L + 1:
    case WAVE_RAM1_H:
    case WAVE_RAM1_H + 1:
    case WAVE_RAM2_L:
    case WAVE_RAM2_L + 1:
    case WAVE_RAM2_H:
    case WAVE_RAM2_H + 1:
    case WAVE_RAM3_L:
    case WAVE_RAM3_L + 1:
    case WAVE_RAM3_H:
    case WAVE_RAM3_H + 1: {
      retval = WAVE_RAM.at(address % 0x10);
      break;
    }

    case DMA0CNT_L:
    case DMA0CNT_L + 1: return 0;

    case DMA1CNT_L:
    case DMA1CNT_L + 1: return 0;

    case DMA2CNT_L:
    case DMA2CNT_L + 1:
      return 0;

      // case DMA3CNT_L:
      // case DMA3CNT_L + 1: return 0;

    case DMA0CNT_H:
    case DMA0CNT_H + 1: {
      retval = read_byte(ch0->dmacnt_h.v, address % 2);
      break;
    }

    case DMA1CNT_H:
    case DMA1CNT_H + 1: {
      retval = read_byte(ch1->dmacnt_h.v, address % 2);
      break;
    }

    case DMA2CNT_H:
    case DMA2CNT_H + 1: {
      retval = read_byte(ch2->dmacnt_h.v, address % 2);
      break;
    }

    case DMA3CNT_L:
    case DMA3CNT_L + 1: return 0;

    case DMA3CNT_H:
    case DMA3CNT_H + 1: {
      retval = read_byte(ch3->dmacnt_h.v, address % 2);
      break;
    }

    case TM0CNT_L:
    case TM0CNT_L + 1: {
      retval = read_byte(tm0->counter, address % 2);
      // fmt::println("retval: {}", tm0->counter);
      break;
    }
    case TM0CNT_H:
    case TM0CNT_H + 1: {
      retval = read_byte(tm0->ctrl.v, address % 2);
      break;
    }

    case TM1CNT_L:
    case TM1CNT_L + 1: {
      // retval = read_byte(tm1->counter, address % 2);

      // u16 ticks = (cycles_elapsed - tm1->start_time + cycles_elapsed) / tm1->get_divider_val();

      // tm1->counter = 0;
      retval = read_byte(tm1->counter, address % 2);
      break;
    }
    case TM1CNT_H:
    case TM1CNT_H + 1: {
      retval = read_byte(tm1->ctrl.v, address % 2);
      break;
    }

    case TM2CNT_L:
    case TM2CNT_L + 1: {
      tm2->counter = 0;
      retval       = read_byte(tm2->counter, address % 2);
      break;
    }
    case TM2CNT_H:
    case TM2CNT_H + 1: {
      retval = read_byte(tm2->ctrl.v, address % 2);
      break;
    }

    case TM3CNT_L:
    case TM3CNT_L + 1: {
      tm3->counter = 0;
      retval       = read_byte(tm3->counter, address % 2);
      break;
    }
    case TM3CNT_H:
    case TM3CNT_H + 1: {
      retval = read_byte(tm0->ctrl.v, address % 2);
      break;
    }

    case SIODATA32: bus_logger->debug("READING FROM SIODATA32 UNIMPL"); break;
    case SIOMULTI1: bus_logger->debug("READING FROM SIOMULTI1 UNIMPL"); break;
    case SIOMULTI2: bus_logger->debug("READING FROM SIOMULTI2 UNIMPL"); break;
    case SIOMULTI3: bus_logger->debug("READING FROM SIOMULTI3 UNIMPL"); break;
    case SIOCNT: {
      // bus_logger->debug("READING FROM SIOCNT UNIMPL");
      break;
    }
    case SIOMLT_SEND: break;
    case KEYINPUT:
    case KEYINPUT + 1: {
      retval = read_byte(keypad_input.KEYINPUT.v, address % 2);
      break;
    }
    case KEYCNT: bus_logger->debug("READING FROM UNIMPL KEYCNT"); break;
    case RCNT: retval = 0x0; break;
    case RCNT + 1:
      retval = 0x80;
      break;
      // case RCNT: break;

    case JOYCNT: bus_logger->debug("READING FROM UNIMPL JOYCNT"); break;
    case JOY_RECV: bus_logger->debug("READING FROM UNIMPL JOY_RECV"); break;
    case JOY_TRANS: bus_logger->debug("READING FROM UNIMPL JOY_TRANS"); break;
    case JOYSTAT: bus_logger->debug("READING FROM UNIMPL JOYSTAT"); break;
    case IE:
    case IE + 1: {
      retval = read_byte(interrupt_control.IE.v, address % 2);
      break;
    }
    case IF:
    case IF + 1: {
      retval = read_byte(interrupt_control.IF.v, address % 2);
      break;
    }
    case WAITCNT:
    case WAITCNT + 1:
    case WAITCNT + 2:
    case WAITCNT + 3: {
      retval = read_byte(system_control.WAITCNT.v, address % 4);
      // system_control.WAITCNT.
      break;
    }
    case IME:
    case IME + 1:
    case IME + 2:
    case IME + 3: {
      retval = read_byte(interrupt_control.IME.v, address % 4);
      break;
    }

    case 0x400006A:
    case 0x400006B:
    case 0x4000076:
    case 0x4000077:
    case 0x400007E:
    case 0x400007F:
    case 0x400008A:
    case 0x400008B:
    case 0x400006E:
    case 0x400006F:
    case UNKNOWN136:
    case UNKNOWN136 + 1:
    case UNKNOWN142:
    case UNKNOWN142 + 1:
    case UNKNOWN15A:
    case UNKNOWN15A + 1:
    case UNKNOWN302:
    case UNKNOWN302 + 1: return 0x00;

    case POSTFLG:
      break;
      // case HALTCNT: break;

    default: {
      // fmt::println("misaligned/partial read {:#08x} - [{}]", address, get_label(address));
      // assert(0);
      return cpu->pipeline.fetch >> (address & 1) * 8;
    }
  }
  // fmt::println("[IO READ] {} => {:#010x}", get_label(address), retval);
  return retval;
}
void Bus::io_write(u32 address, u8 value) {
  (void)address;
  (void)value;

#ifndef SST_TEST_MODE
  if (address > 0x040003FE) return;
  switch (address) {
    case DISPCNT:
    case DISPCNT + 1: {
      auto old_mapping_mode = ppu->display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING;
      set_byte(ppu->display_fields.DISPCNT.v, address % 2, value);
      auto new_mapping_mode = ppu->display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING;

      ppu->state.mapping_mode_changed = old_mapping_mode != new_mapping_mode;
      if (ppu->state.mapping_mode_changed) fmt::println("mode changed");

      // bus_logger->debug("NEW DISPCNT: {:#010x}", ppu->display_fields.DISPCNT.v);
      break;
    }
    case GREEN_SWAP:
    case GREEN_SWAP + 1: {
      set_byte(ppu->display_fields.GREEN_SWAP.v, address % 2, value);
      // bus_logger->debug("NEW GREENSWAP: {:#010x}", ppu->display_fields.GREEN_SWAP.v);
      break;
    }
    case DISPSTAT:
    case DISPSTAT + 1: {
      if (address == DISPSTAT) {
        value = (value & 0b11111000);
        ppu->display_fields.DISPSTAT.v |= value;
        // set_byte(ppu->display_fields.DISPSTAT.v, address % 2, );
      } else {
        set_byte(ppu->display_fields.DISPSTAT.v, address % 2, value);
      }

      // bus_logger->debug("NEW DISPSTAT: {:#010x}", ppu->display_fields.DISPSTAT.v, address);
      break;
    }

    case BG0CNT:
    case BG0CNT + 1: {
      set_byte(ppu->display_fields.BG0CNT.v, address % 2, value);
      ppu->display_fields.BG0CNT.v &= 0b1101111111111111;
      break;
    }

    case BG1CNT:
    case BG1CNT + 1: {
      set_byte(ppu->display_fields.BG1CNT.v, address % 2, value);
      ppu->display_fields.BG1CNT.v &= 0b1101111111111111;
      break;
    }

    case BG2CNT:
    case BG2CNT + 1: {
      set_byte(ppu->display_fields.BG2CNT.v, address % 0x2, value);
      break;
    }
    case BG3CNT:
    case BG3CNT + 1: {
      set_byte(ppu->display_fields.BG3CNT.v, address % 0x2, value);
      break;
    }

    case BG0HOFS:
    case BG0HOFS + 1: {
      set_byte(ppu->display_fields.BG0HOFS.v, address % 0x2, value);
      break;
    }
    case BG0VOFS:
    case BG0VOFS + 1: {
      set_byte(ppu->display_fields.BG0VOFS.v, address % 0x2, value);
      break;
    }

    case BG1VOFS:
    case BG1VOFS + 1: {
      set_byte(ppu->display_fields.BG1VOFS.v, address % 0x2, value);
      break;
    }
    case BG1HOFS:
    case BG1HOFS + 1: {
      set_byte(ppu->display_fields.BG1HOFS.v, address % 0x2, value);
      break;
    }

    case BG2VOFS:
    case BG2VOFS + 1: {
      set_byte(ppu->display_fields.BG2VOFS.v, address % 0x2, value);
      break;
    }
    case BG2HOFS:
    case BG2HOFS + 1: {
      set_byte(ppu->display_fields.BG2HOFS.v, address % 0x2, value);
      break;
    }

    case BG3HOFS:
    case BG3HOFS + 1: {
      set_byte(ppu->display_fields.BG3HOFS.v, address % 0x2, value);
      break;
    }
    case BG3VOFS:
    case BG3VOFS + 1: {
      set_byte(ppu->display_fields.BG3VOFS.v, address % 0x2, value);
      break;
    }

    case BG2PA:
    case BG2PA + 1: {
      set_byte(ppu->display_fields.BG2PA.v, address % 0x2, value);
      break;
    }
    case BG2PB:
    case BG2PB + 1: {
      set_byte(ppu->display_fields.BG2PB.v, address % 0x2, value);
      break;
    }
    case BG2PC:
    case BG2PC + 1: {
      set_byte(ppu->display_fields.BG2PC.v, address % 0x2, value);
      break;
    }
    case BG2PD:
    case BG2PD + 1: {
      set_byte(ppu->display_fields.BG2PD.v, address % 0x2, value);
      break;
    }

    case BG3PA:
    case BG3PA + 1: {
      set_byte(ppu->display_fields.BG3PA.v, address % 0x2, value);
      break;
    }
    case BG3PB:
    case BG3PB + 1: {
      set_byte(ppu->display_fields.BG3PB.v, address % 0x2, value);
      break;
    }
    case BG3PC:
    case BG3PC + 1: {
      set_byte(ppu->display_fields.BG3PC.v, address % 0x2, value);
      break;
    }
    case BG3PD:
    case BG3PD + 1: {
      set_byte(ppu->display_fields.BG3PD.v, address % 0x2, value);
      break;
    }

    case BG2X:
    case BG2X + 1:
    case BG2X + 2:
    case BG2X + 3: {
      set_byte(ppu->display_fields.BG2X.v, address % 0x4, value);
      break;
    }
    case BG2Y:
    case BG2Y + 1:
    case BG2Y + 2:
    case BG2Y + 3: {
      set_byte(ppu->display_fields.BG2Y.v, address % 0x4, value);
      break;
    }

    case BG3X:
    case BG3X + 1:
    case BG3X + 2:
    case BG3X + 3: {
      set_byte(ppu->display_fields.BG3X.v, address % 0x4, value);
      break;
    }
    case BG3Y:
    case BG3Y + 1:
    case BG3Y + 2:
    case BG3Y + 3: {
      set_byte(ppu->display_fields.BG3Y.v, address % 0x4, value);
      break;
    }

    case WIN0H:
    case WIN0H + 1: {
      set_byte(ppu->display_fields.WIN0H.v, address % 0x2, value);
      break;
    }
    case WIN1H:
    case WIN1H + 1: {
      set_byte(ppu->display_fields.WIN1H.v, address % 0x2, value);
      break;
    }
    case WIN0V:
    case WIN0V + 1: {
      set_byte(ppu->display_fields.WIN0V.v, address % 0x2, value);
      break;
    }
    case WIN1V:
    case WIN1V + 1: {
      set_byte(ppu->display_fields.WIN1V.v, address % 0x2, value);
      break;
    }
    case WININ:
    case WININ + 1: {
      set_byte(ppu->display_fields.WININ.v, address % 0x2, value & 0x3f);
      break;
    }

    case WINOUT:
    case WINOUT + 1: {
      set_byte(ppu->display_fields.WINOUT.v, address % 0x2, value & 0x3f);
      break;
    }

    case MOSAIC:
    case MOSAIC + 1: {
      set_byte(ppu->display_fields.MOSAIC.v, address % 0x2, value);
      break;
    }
    case BLDCNT: {
      set_byte(ppu->display_fields.BLDCNT.v, 0, value);
      break;
    }
    case BLDCNT + 1: {
      set_byte(ppu->display_fields.BLDCNT.v, 1, value & 0x3f);
      break;
    }
    case BLDALPHA:
    case BLDALPHA + 1: {
      set_byte(ppu->display_fields.BLDALPHA.v, address % 0x2, value & 0x1f);
      break;
    }

    case BLDY:
    case BLDY + 1: {
      set_byte(ppu->display_fields.BLDY.v, address % 0x2, value);
      break;
    }
    case SOUND1CNT_L:
    case SOUND1CNT_L + 1: {
      set_byte(apu->sound_registers.SOUND1CNT_L.v, address % 2, value);

      apu->sound_registers.SOUND1CNT_L.v &= 0x7f;
      break;
    }
    case SOUND1CNT_H:
    case SOUND1CNT_H + 1: {
      set_byte(apu->sound_registers.SOUND1CNT_H.v, address % 2, value);
      // bus_logger->debug("WRITING TO SOUND1CNT_H UNIMPL");

      apu->sound_registers.SOUND1CNT_H.v &= ~0b111111;
      break;
    }

    case SOUND1CNT_X:
    case SOUND1CNT_X + 1:
    case SOUND1CNT_X + 2:
    case SOUND1CNT_X + 3: {
      set_byte(apu->sound_registers.SOUND1CNT_X.v, address % 4, value);

      apu->sound_registers.SOUND1CNT_X.v &= (1 << 14);
      break;
    }

    case SOUND2CNT_L:
    case SOUND2CNT_L + 1: {
      set_byte(apu->sound_registers.SOUND2CNT_L.v, address % 2, value);

      apu->sound_registers.SOUND2CNT_L.v &= 0xFFC0;
      break;
    }

    case SOUND2CNT_H:
    case SOUND2CNT_H + 1: {
      set_byte(apu->sound_registers.SOUND2CNT_H.v, address % 2, value);
      // bus_logger->debug("WRITING TO SOUND1CNT_H UNIMPL");

      apu->sound_registers.SOUND2CNT_H.v &= 0X4000;

      break;
    }

    case SOUND3CNT_L:
    case SOUND3CNT_L + 1: {
      set_byte(apu->sound_registers.SOUND3CNT_L.v, address % 2, value);

      apu->sound_registers.SOUND3CNT_L.v &= 0XE0;
      break;
    }

    case SOUND3CNT_H:
    case SOUND3CNT_H + 1: {
      set_byte(apu->sound_registers.SOUND3CNT_H.v, address % 2, value);

      apu->sound_registers.SOUND3CNT_H.v &= 0b1110000000000000;
      break;
    }

    case SOUND3CNT_X:
    case SOUND3CNT_X + 1:
    case SOUND3CNT_X + 2:
    case SOUND3CNT_X + 3: {
      set_byte(apu->sound_registers.SOUND3CNT_X.v, address % 4, value);

      apu->sound_registers.SOUND3CNT_X.v &= (1 << 14);
      break;
    };

    case SOUND4CNT_L:
    case SOUND4CNT_L + 1:
    case SOUND4CNT_L + 2:
    case SOUND4CNT_L + 3: {
      set_byte(apu->sound_registers.SOUND4CNT_L.v, address % 4, value);

      apu->sound_registers.SOUND4CNT_L.v &= 0xFF00;
      break;
    }

    case SOUND4CNT_H:
    case SOUND4CNT_H + 1:
    case SOUND4CNT_H + 2:
    case SOUND4CNT_H + 3: {
      set_byte(apu->sound_registers.SOUND4CNT_H.v, address % 4, value);
      apu->sound_registers.SOUND4CNT_H.v &= 0x40FF;

      break;
    }
    case SOUNDCNT_L: {
      set_byte(apu->sound_registers.SOUNDCNT_L.v, address % 2, value & 0b01110111);
      break;
    }
    case SOUNDCNT_L + 1: {
      set_byte(apu->sound_registers.SOUNDCNT_L.v, address % 2, value);
      break;
    }

    case SOUNDCNT_H:
    case SOUNDCNT_H + 1: {
      set_byte(apu->sound_registers.SOUNDCNT_H.v, address % 2, value);

      if (value & (1 << 4) && ((address % 2) == 1)) {  // reset
        bus_logger->info("resetting FIFO A");
        apu->FIFO_A = {};
      }

      if (value & (1 << 7) && ((address % 2) == 1)) {  // reset
        bus_logger->info("resetting FIFO B");
        apu->FIFO_B = {};
      }

      apu->sound_registers.SOUNDCNT_H.v &= 0X770F;
      break;
    }

    case SOUNDCNT_X:
    case SOUNDCNT_X + 1:
    case SOUNDCNT_X + 2:
    case SOUNDCNT_X + 3: {
      if (address != SOUNDCNT_X) break;

      set_byte(apu->sound_registers.SOUNDCNT_X.v, address % 4, value & (1 << 7));
      break;
    }

    case SOUNDBIAS:
    case SOUNDBIAS + 1: {
      set_byte(system_control.sound_bias, address % 2, value);
      // system_control.sound_bias = value;
      break;
    }
      // case WAVE_RAM: bus_logger->debug("WRITING TO WAVE_RAM UNIMPL"); break;

    case WAITCNT:
    case WAITCNT + 1:
    case WAITCNT + 2:
    case WAITCNT + 3: {
      set_byte(system_control.WAITCNT.v, address % 4, value);

      system_control.WAITCNT.v &= 0b1101111111111111;
      break;
    }

    case WAVE_RAM0_L:
    case WAVE_RAM0_L + 1:
    case WAVE_RAM0_H:
    case WAVE_RAM0_H + 1:
    case WAVE_RAM1_L:
    case WAVE_RAM1_L + 1:
    case WAVE_RAM1_H:
    case WAVE_RAM1_H + 1:
    case WAVE_RAM2_L:
    case WAVE_RAM2_L + 1:
    case WAVE_RAM2_H:
    case WAVE_RAM2_H + 1:
    case WAVE_RAM3_L:
    case WAVE_RAM3_L + 1:
    case WAVE_RAM3_H:
    case WAVE_RAM3_H + 1: {
      WAVE_RAM.at(address % 0x10) = value;
      break;
    }

    case FIFO_A:
    case FIFO_A + 1:
    case FIFO_A + 2:
    case FIFO_A + 3: {
      if (apu->FIFO_A.size() == 32) return;
      apu->FIFO_A.push(value);
      break;
    }

    case FIFO_B:
    case FIFO_B + 1:
    case FIFO_B + 2:
    case FIFO_B + 3: {
      if (apu->FIFO_B.size() == 32) return;
      apu->FIFO_B.push(value);
      break;
    }

    case DMA0SAD ... DMA0SAD + 3: {
      assert(ch0 != nullptr);
      set_byte(ch0->src, address % 0x4, value);
      break;
    }
    case DMA0DAD ... DMA0DAD + 3: {
      set_byte(ch0->dst, address % 0x4, value);
      break;
    }

    case DMA0CNT_L ... DMA0CNT_L + 1: {
      set_byte(ch0->dmacnt_l.v, address % 0x2, value);
      break;
    }
    case DMA0CNT_H:
    case DMA0CNT_H + 1: {
      bool stopped = !ch0->dmacnt_h.dma_enable;
      set_byte(ch0->dmacnt_h.v, address % 2, value);
      bool started = ch0->dmacnt_h.dma_enable;
      ch0->dmacnt_h.v &= 0xf7e0;

      if (stopped && started) {
        ch0->internal_src       = ch0->src;
        ch0->internal_dst       = ch0->dst;
        ch0->internal_word_size = ch0->dmacnt_l.word_count;
      }
      break;
    }

    case DMA1SAD ... DMA1SAD + 3: {
      assert(ch1 != nullptr);
      set_byte(ch1->src, address % 0x4, value);
      break;
    }
    case DMA1DAD ... DMA1DAD + 3: {
      set_byte(ch1->dst, address % 0x4, value);
      break;
    }

    case DMA1CNT_L:
    case DMA1CNT_L + 1: {
      set_byte(ch1->dmacnt_l.v, address % 0x2, value);
      break;
    }
    case DMA1CNT_H ... DMA1CNT_H + 1: {
      bool stopped = !ch1->dmacnt_h.dma_enable;
      set_byte(ch1->dmacnt_h.v, address % 2, value);
      bool started = ch1->dmacnt_h.dma_enable;
      ch1->dmacnt_h.v &= 0xf7e0;

      if (stopped && started) {
        ch1->internal_src       = ch1->src;
        ch1->internal_dst       = ch1->dst;
        ch1->internal_word_size = ch1->dmacnt_l.word_count;
      }

      break;
    }

    case DMA2SAD ... DMA2SAD + 3: {
      assert(ch2 != nullptr);
      set_byte(ch2->src, address % 0x4, value);
      break;
    }
    case DMA2DAD ... DMA2DAD + 3: {
      set_byte(ch2->dst, address % 0x4, value);
      break;
    }

    case DMA2CNT_L:
    case DMA2CNT_L + 1: {
      set_byte(ch2->dmacnt_l.v, address % 0x2, value);
      break;
    }
    case DMA2CNT_H:
    case DMA2CNT_H + 1: {
      bool stopped = !ch2->dmacnt_h.dma_enable;
      set_byte(ch2->dmacnt_h.v, address % 2, value);
      bool started = ch2->dmacnt_h.dma_enable;
      ch2->dmacnt_h.v &= 0xf7e0;

      if (stopped && started) {
        ch2->internal_src       = ch2->src;
        ch2->internal_dst       = ch2->dst;
        ch2->internal_word_size = ch2->dmacnt_l.word_count;
      }

      break;
    }

    case DMA3SAD ... DMA3SAD + 3: {
      assert(ch3 != nullptr);
      set_byte(ch3->src, address % 0x4, value);
      break;
    }
    case DMA3DAD ... DMA3DAD + 3: {
      set_byte(ch3->dst, address % 0x4, value);
      break;
    }

    case DMA3CNT_L ... DMA3CNT_L + 1: {
      set_byte(ch3->dmacnt_l.v, address % 0x2, value);
      break;
    }
    case DMA3CNT_H ... DMA3CNT_H + 1: {
      bool stopped = !ch3->dmacnt_h.dma_enable;
      set_byte(ch3->dmacnt_h.v, address % 2, value);
      bool started = ch3->dmacnt_h.dma_enable;
      ch3->dmacnt_h.v &= 0xffe0;

      if (stopped && started) {
        ch3->internal_src       = ch3->src;
        ch3->internal_dst       = ch3->dst;
        ch3->internal_word_size = ch3->dmacnt_l.word_count;
      }

      break;
    }
    case TM0CNT_L:
    case TM0CNT_L + 1: {
      set_byte(tm0->reload_value, address % 2, value);
      break;
    }
    case TM0CNT_H:
    case TM0CNT_H + 1: {
      bool stopped = !tm0->ctrl.timer_start_stop;
      set_byte(tm0->ctrl.v, address % 2, value);
      bool started = tm0->ctrl.timer_start_stop;

      if (stopped && started) {  // Timer goes from OFF TO ON (0->1)
        tm0->reload_counter();
        tm0->start_time = cycles_elapsed + 2;
        fmt::println("updated start time to value: {}", tm0->start_time);
      }
      break;
    }
    case TM1CNT_L:
    case TM1CNT_L + 1: {
      set_byte(tm1->reload_value, address % 2, value);
      break;
    }
    case TM1CNT_H:
    case TM1CNT_H + 1: {
      bool stopped = tm1->ctrl.timer_start_stop == false;
      set_byte(tm1->ctrl.v, address % 2, value);
      bool started = tm1->ctrl.timer_start_stop;

      if (stopped && started) {
        tm1->reload_counter();
      }
      break;
    }
    case TM2CNT_L:
    case TM2CNT_L + 1: {
      set_byte(tm2->reload_value, address % 2, value);

      break;
    }
    case TM2CNT_H:
    case TM2CNT_H + 1: {
      bool stopped = tm2->ctrl.timer_start_stop == false;
      set_byte(tm2->ctrl.v, address % 2, value);
      bool started = tm2->ctrl.timer_start_stop;
      // tm2->ctrl.v &= 0x7F;

      if (stopped && started) {
        tm2->reload_counter();
        // Scheduler::Schedule(Scheduler::EventType::TIMER2_START, cycles_elapsed + 2);
      }
      break;
    }
    case TM3CNT_L:
    case TM3CNT_L + 1: {
      set_byte(tm3->reload_value, address % 2, value);
      break;
    }
    case TM3CNT_H:
    case TM3CNT_H + 1: {
      bool stopped = tm3->ctrl.timer_start_stop == false;
      set_byte(tm3->ctrl.v, address % 2, value);
      bool started = tm3->ctrl.timer_start_stop;

      if (stopped && started) {
        tm3->reload_counter();
        // tm3->ctrl.v &= 0x7F;

        // Scheduler::Schedule(Scheduler::EventType::TIMER3_START, cycles_elapsed + 2);
      }

      break;
    }
    case SIODATA32: break;
    case SIOMULTI1: break;
    case SIOMULTI2: break;
    case SIOMULTI3: break;
    case SIOCNT: break;
    case SIOMLT_SEND: break;
    case KEYINPUT: break;
    case KEYCNT: bus_logger->debug("WRITING TO UNIMPL KEYCNT"); break;
    case RCNT: bus_logger->debug("WRITING TO UNIMPL RCNT"); break;
    case JOYCNT: bus_logger->debug("WRITING TO UNIMPL JOYCNT"); break;
    case JOY_RECV: bus_logger->debug("WRITING TO UNIMPL JOY_RECV"); break;
    case JOY_TRANS: bus_logger->debug("WRITING TO UNIMPL JOY_TRANS"); break;
    case JOYSTAT: bus_logger->debug("WRITING TO UNIMPL JOYSTAT"); break;
    case IE:
    case IE + 1: {
      set_byte(interrupt_control.IE.v, address % 2, value);
      // if (old_ie == interrupt_control.IE.v) break;
      // bus_logger->debug("WROTE {:#010x} to IE - [{:#010x}]", value, address);
      // bus_logger->debug("NEW IE: {:#010x}", interrupt_control.IE.v, address);
      break;
    }
    case IF ... IF + 1: {
      if (address == IF) {
        interrupt_control.IF.v &= ~value;
      } else {
        interrupt_control.IF.v &= ~(value << 8);  // zero out upper bits of reg
      }
      // bus_logger->debug("WROTE {:#010x} to IF - [{:#010x}]", value, address);
      // bus_logger->debug("NEW IF: {:#010x}", interrupt_control.IF.v, address);
      break;
    }

    case IME ... IME + 3: {
      if (address != IME) return;
      interrupt_control.IME.v = (value & 0b1);
      // bus_logger->debug("new IME: {:#010x} - [{:#010x}]", interrupt_control.IME.v, address);B
      break;
    }
    case POSTFLG: {
      set_byte(system_control.POSTFLG, 0, value);
      break;
    }
    case HALTCNT: {
      set_byte(system_control.HALTCNT, 0, value);
      break;
    }

    case 0x4000800:
    case 0x4000800 + 1:
    case 0x4000800 + 2:
    case 0x4000800 + 3: {
      set_byte(system_control.IMC, address % 4, value);
      break;
    }

    default: {
      bus_logger->debug("misaligned write: {:#010x}", address);
    }
  }
#endif
}
u8 Bus::get_rom_cycles_by_waitstate(const ACCESS_TYPE access_type, const WAITSTATE ws) {
  switch (access_type) {
    case ACCESS_TYPE::SEQUENTIAL: {
      switch (ws) {
        case WAITSTATE::WS0: {
          switch (system_control.WAITCNT.WS0_SECOND_ACCESS) {
            case 0: return 2;
            case 1: return 1;
          }
          break;
        }
        case WAITSTATE::WS1: {
          switch (system_control.WAITCNT.WS1_SECOND_ACCESS) {
            case 0: return 4;
            case 1: return 1;
          }
          break;
        }
        case WAITSTATE::WS2: {
          switch (system_control.WAITCNT.WS2_SECOND_ACCESS) {
            case 0: return 8;
            case 1: return 1;
          }
          break;
        }
      }
      break;
    }
    case ACCESS_TYPE::NON_SEQUENTIAL: {
      switch (ws) {
        case WAITSTATE::WS0: {
          switch (system_control.WAITCNT.WS0_FIRST_ACCESS) {
            case 0: return 4;
            case 1: return 3;
            case 2: return 2;
            case 3: return 8;
          }
          break;
        }
        case WAITSTATE::WS1: {
          switch (system_control.WAITCNT.WS1_FIRST_ACCESS) {
            case 0: return 4;
            case 1: return 3;
            case 2: return 2;
            case 3: return 8;
          }
          break;
        }
        case WAITSTATE::WS2: {
          switch (system_control.WAITCNT.WS2_FIRST_ACCESS) {
            case 0: return 4;
            case 1: return 3;
            case 2: return 2;
            case 3: return 8;
          }
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }
  return 255;
}
u8 Bus::get_wram_waitstates() { return 1 + (15 - system_control.wait_control_wram); }

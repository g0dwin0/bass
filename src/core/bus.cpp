#include "bus.hpp"

#include <cstdio>

#include "common/bytes.hpp"
#include "common/test_structs.hpp"
#include "enums.hpp"
#include "labels.hpp"
#include "registers.hpp"

void Bus::request_interrupt(InterruptType t) { interrupt_control.IF.v |= (1 << (u8)t); }

u8 Bus::read8(u32 address, [[gnu::unused]] ACCESS_TYPE access_type) {
#ifdef SST_TEST_MODE
  // TODO: make a separate function for these, extra overhead does not matter at all
  fmt::println("requested [8] read at {:#010x}", address);
  for (const auto& transaction : transactions) {
    if (transaction.size == 2 && transaction.kind != WRITE && (transaction.addr == address + 1 || transaction.addr == address)) {
      fmt::println("misaligned READ8 -- SST");
      fmt::println("{:#010X} -> {:#010X}", address, transaction.data >> ((transaction.addr % 2) * 8));
      return transaction.data >> ((transaction.addr % 2) * 8);
    }

    if (transaction.size != 1 && transaction.kind != WRITE) continue;

    fmt::println("[R8] {:#010x} -> {:#010x}", address, transaction.data);

    if (address == transaction.addr) {
      fmt::println("returning: {:#010x}", transaction.data);
      return transaction.data;
    }

    if (address == transaction.base_addr) { return transaction.opcode; }
  }

  return address;
#else
  u32 v;

  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      v = BIOS.at(address);
      break;
    }

    case 0x02000000 ... 0x02FFFFFF: {
      v = EWRAM.at(address % 0x40000);
      break;
    }

    case 0x03000000 ... 0x03FFFFFF: {
      // v = shift_16(IWRAM, address - 0x03000000);
      v = IWRAM.at(address % 0x8000);
      // if (address == 0x03FFFFFC) fmt::println("[R8] {:#010x} => {:#04x}", address, v);
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      v = io_read(address);
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      // if (address >= 0x06018000 && display_fields.DISPCNT.BG_MODE >= 3) return 0;
      // u32 addr = address;

      // if (addr >= 0x17FFF) { addr -= 0x8000; }

      v = *(uint8_t*)(&VRAM.data()[(address - 0x06000000)]);
      break;
    }

    case 0x08000000 ... 0x09FFFFFF: {
      // game pak read (ws0)
      v = pak->data.at(address - 0x08000000);
      break;
    }

    case 0x0A000000 ... 0x0BFFFFFF: {
      // game pak read (ws1)
      v = pak->data.at(address - 0x0A000000);
      break;
    }

    case 0x0C000000 ... 0x0DFFFFFF: {
      // game pak read (ws2)
      v = pak->data.at(address - 0x0C000000);
      break;
    }

    default: {
      // fmt::println("[R8] unused/oob memory read: {:#X}", address);
      return 0xFF;
    }
  }

  // fmt::println("[R8] {:#010x} [{}] => {:#04x}", address, get_label(address), v);
  return v;

#endif
};
u16 Bus::read16(u32 address, [[gnu::unused]] ACCESS_TYPE access_type) {
  address = cpu->align(address, HALFWORD);

#ifdef SST_TEST_MODE
  fmt::println("requested [16] read at {:#010x}", address);
  for (const auto& transaction : transactions) {
    if (transaction.size != 2 && transaction.kind != WRITE) continue;

    if (address == cpu->align(transaction.addr, HALFWORD)) {
      fmt::println("[R16] {:#010x} -> {:#010x}", address, transaction.data);

      return transaction.data;
    }

    if (address == transaction.base_addr) { return transaction.opcode; }
  }

  return address;

#else
  // fmt::println("[R16] {:#010x}", address);
  u32 v = 0;
  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      v = *(uint16_t*)(&BIOS.data()[address]);
      // v = shift_16(BIOS, address);
      break;
    }

    case 0x02000000 ... 0x0203FFFF: {
      v = *(uint16_t*)(&EWRAM.data()[address % 0x40000]);
      // v = shift_16(EWRAM, address - 0x02000000);
      break;
    }

    case 0x03000000 ... 0x03FFFFFF: {
      v = *(uint16_t*)(&IWRAM.data()[address % 0x8000]);
      // v = shift_16(IWRAM, address - 0x03000000);
      // if (address == 0x03FFFFFC) fmt::println("[R16] {:#010x} => {:#04x}", address, v);

      break;
    }

    case 0x04000000 ... 0x040003FE: {
      for (size_t i = 0; i < 2; i++) {
        // fmt::println("[R16] io read: {:#010x} - [{}]", address + i, get_label(address + i));
        v |= (io_read(address + i) << (8 * i));
      }
      // fmt::println("[R16] retval: {:#010x}", v);

      // fmt::println("16 bit IO read");
      break;
    }
    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      return *(uint16_t*)(&PALETTE_RAM.data()[address % 0x400]);
    }

    case 0x06000000 ... 0x06017FFF: {
      return *(uint16_t*)(&VRAM.data()[(address - 0x06000000)]);
    }

    case 0x08000000 ... 0x09FFFFFF: {
      // game pak read (ws0)
      // v = shift_16(pak->data, address - 0x08000000);
      v = *(uint16_t*)(&pak->data[address - 0x08000000]);
      break;
    }

    case 0x0A000000 ... 0x0BFFFFFF: {
      // game pak read (ws1)
      v = *(uint16_t*)(&pak->data[address - 0x0A000000]);
      break;
    }

    case 0x0C000000 ... 0x0DFFFFFF: {
      // game pak read (ws2);
      v = *(uint16_t*)(&pak->data[address - 0x0C000000]);
      break;
    }

    default: {
      // fmt::println("[R16] unused/oob memory read: {:#X}", address);
      // TODO: implement open bus behaviour
      return 0xFFFF;
    }
  }
  // bus_logger->info("[R16] {:#010X} => {:#010x}", address, v);
  return v;
#endif
};
u32 Bus::read32(u32 address, [[gnu::unused]] ACCESS_TYPE access_type) {
  address = cpu->align(address, WORD);

#ifdef SST_TEST_MODE
  fmt::println("requested [32] read at {:#010x}", address);
  for (auto& transaction : transactions) {
    if (transaction.accessed) continue;
    if (transaction.size != 4 && transaction.kind != WRITE) continue;

    if (address == cpu->align(transaction.addr, WORD)) {
      fmt::println("[R32] [TX_ID: {}] {:#010x} -> {:#010x}", transaction.id, address, transaction.data);

      transaction.accessed = true;
      return transaction.data;
    }

    if (address == transaction.base_addr) { return transaction.opcode; }
  }
  return address;

#else

  // SPDLOG_DEBUG("[R32] {:#010x}", address);
  u32 v = 0;

  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      v = *(uint32_t*)(&BIOS.data()[address]);
      // v = shift_32(BIOS, address);
      break;
    }

    case 0x02000000 ... 0x02FFFFFF: {
      v = *(uint32_t*)(&EWRAM.data()[address % 0x40000]);
      // v = shift_32(EWRAM, address % 0x40000);
      break;
    }

    case 0x03000000 ... 0x03FFFFFF: {
      v = *(uint32_t*)(&IWRAM.data()[address % 0x8000]);
      // v = shift_32(IWRAM, address % 0x8000);
      // if (address == 0x03FFFFFC) fmt::println("[R32] {:#010x} => {:#04x}", address, v);

      break;
    }

    case 0x04000000 ... 0x040003FE: {
      // fmt::println("trying to read io address [32]: {:#010x}", address);
      // fmt::println("fun value: {}", fun_value);
      // assert(0);
      // u32 retval = 0;
      for (size_t i = 0; i < 4; i++) {
        // fmt::println("[R32] io read: {:#010x} - [{}]", address + i, get_label(address + i));
        v |= (io_read(address + i) << (8 * i));
      }
      // fmt::println("[R32] retval: {:#010x}", v);
      // fmt::println("[IO READ (32)] [{}] - {:#010x}", get_label(address), retval);
      // return retva;
      // v = handle_io_read(address);
      break;
    }

    case 0x05000000 ... 0x05FFFFFF: {
      // BG/OBJ Palette RAM
      // v = shift_32(PALETTE_RAM, address % 0x400);
      v = *(uint32_t*)(&PALETTE_RAM.data()[address % 0x400]);
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      // if (address >= 0x06018000 && display_fields.DISPCNT.BG_MODE >= 3) return 0;
      v = *(uint32_t*)(&VRAM.data()[address - 0x06000000]);
      break;
    }

    case 0x07000000 ... 0x07FFFFFF: {
      // v = shift_32(OAM, address % 0x400);
      v = *(uint32_t*)(&OAM.data()[address % 0x400]);
      break;
    }

    case 0x08000000 ... 0x09FFFFFF: {
      // game pak read (ws0)
      // *(pak->data()
      v = *(uint32_t*)(&pak->data[address - 0x08000000]);
      // v = shift_32(pak->data, address - 0x08000000);
      break;
    }

    case 0x0A000000 ... 0x0BFFFFFF: {
      // game pak read (ws1)
      v = *(uint32_t*)(&pak->data[address - 0x0A000000]);
      break;
    }

    case 0x0C000000 ... 0x0DFFFFFF: {
      // game pak read (ws2)
      v = *(uint32_t*)(&pak->data[address - 0x0C000000]);
      break;
    }

    default: {
      // spdlog::warn("[R32] unused/oob memory read: {:#X}", address);
      return 0xFFFFFFFF;  // TODO: implement open bus behavior
    }
  }

  // mem_logger->info("[R32] {} => {:#010x}", get_label(address), v);
  return v;
#endif
};

void Bus::write8(u32 address, u8 value) {
#ifdef SST_TEST_MODE
  fmt::println("write [8] at {:#010x} -> {:#010x}", address, value);
  for (auto& transaction : transactions) {
    if (transaction.size != 1 || transaction.kind != WRITE) continue;
    if (address == transaction.addr && value == transaction.data) { transaction.accessed = true; }
  }
  return;
#else

  switch (address) {
    case 0x02000000 ... 0x0203FFFF: {
      EWRAM.at(address - 0x02000000) = value;
      // set32(EWRAM, address - 0x02000000, value);
      break;
    }

    case 0x03000000 ... 0x03FFFFFF: {
      IWRAM.at(address % 0x8000) = value;
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      // IO
      io_write(address, value);
      break;
    }

    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      return;
      PALETTE_RAM.at(address - 0x05000000) = value;
      // set32(PALETTE_RAM, address - 0x05000000, value);
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      // VRAM
      // assert(0);
      return;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      return;
      OAM.at(address - 0x07000000) = value;
      // set32(OAM, address - 0x07000000, value);
      break;
    }

    case 0x0E000000 ... 0x0E00FFFF: {
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
  address = ARM7TDMI::align(address, HALFWORD);
#ifdef SST_TEST_MODE
  bus_logger->debug("write [16] at {:#010x} -> {:#010x}", address, value);
  fmt::println("write [16] at {:#010x} -> {:#010x}", address, value);
  // #ifdef SST_TEST_MODE
  for (auto& transaction : transactions) {
    if (transaction.size != 2 || transaction.kind != WRITE) continue;
    u32 aligned_tx_addr = ARM7TDMI::align(transaction.addr, HALFWORD);
    if (address == aligned_tx_addr && value == transaction.data) {
      fmt::println("valid write");
      transaction.accessed = true;
    }
  }
  return;
#else

  switch (address) {
    case 0x02000000 ... 0x02FFFFFF: {
      // set16(EWRAM, address - 0x02000000, value);
      *(uint16_t*)(&EWRAM.data()[address % 0x40000]) = value;
      break;
    }

    case 0x03000000 ... 0x03FFFFFF: {
      *(uint16_t*)(&IWRAM.data()[address % 0x8000]) = value;
      // if (address == 0x03FFFFFC) fmt::println("[W16] {:#010x} => {:#04x}", address, value);

      break;
    }

    case 0x04000000 ... 0x040003FE: {
      // IO
      // fmt::println("IO WRITE [16]: {:#010x} {:#010x}", address, value);
      write8(address, value & 0xff);
      write8(address + 1, ((value & 0xff00) >> 8));
      return;
      break;
    }

    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      // set16(PALETTE_RAM, address - 0x05000000, value);
      *(uint16_t*)(&PALETTE_RAM.data()[address - 0x05000000]) = value;
      break; /*  */
    }

    case 0x06000000 ... 0x06017FFF: {
      // if (address >= 0x06018000 && display_fields.DISPCNT.BG_MODE >= 3) return;
      *(uint16_t*)(&VRAM.data()[address - 0x06000000]) = value;
      break;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      // set16(OAM, address - 0x07000000, value);
      *(uint16_t*)(&OAM.data()[address - 0x07000000]) = value;
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
  address = cpu->align(address, WORD);

#ifdef SST_TEST_MODE
  fmt::println("write [32] at {:#010x} -> {:#010x}", address, value);
  for (auto& transaction : transactions) {
    if (transaction.size != 4 || transaction.kind != WRITE) continue;

    u32 aligned_tx_addr = cpu->align(transaction.addr, WORD);
    if (address == aligned_tx_addr && value == transaction.data) {
      fmt::println("[W32] [TX_ID: {}] {:#010x} -> {:#010x}", transaction.id, address, transaction.data);

      transaction.accessed = true;
    }
  }

  return;
#else

  switch (address) {
    case 0x02000000 ... 0x02FFFFFF: {
      // set32(EWRAM, address % 0x40000, value);
      *(uint32_t*)(&EWRAM.data()[address % 0x40000]) = value;
      break;
    }

    case 0x03000000 ... 0x03FFFFFF: {
      *(uint32_t*)(&IWRAM.data()[address % 0x8000]) = value;
      // if (address == 0x03FFFFFC) fmt::println("[W32] {:#010x} => {:#04x}", address, value);

      // set32(IWRAM, address % 0x8000, value);
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      // IO
      // fmt::println("IO ADDR: {:#010x}", address);
      write16(address, value & 0x0000FFFF);
      write16(address + 2, (value & 0xFFFF0000) >> 16);
      // assert(0);
      return;
      break;
    }

    case 0x05000000 ... 0x05FFFFFF: {
      // BG/OBJ Palette RAM
      *(uint32_t*)(&PALETTE_RAM.data()[(address % 0x400)]) = value;
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      // VRAM
      *(uint32_t*)(&VRAM.data()[(address - 0x06000000)]) = value;
      break;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      *(uint32_t*)(&OAM.data()[(address % 0x400)]) = value;
      break;
    }

    case 0x0E000000 ... 0x0E00FFFF: {
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
    case DISPCNT ... DISPCNT + 1: {
      retval = read_byte(display_fields.DISPCNT.v, address % 2);
      bus_logger->info("dispcnt read");
      break;
    }
    case GREEN_SWAP ... GREEN_SWAP + 1: SPDLOG_DEBUG("READING FROM GREEN_SWAP UNIMPL"); break;
    case DISPSTAT ... DISPSTAT + 1: {
      retval = read_byte(display_fields.DISPSTAT.v, address % 2);
      // bus_logger->info("disp stat read");
      break;
    }
    case VCOUNT ... VCOUNT + 1: {
      retval = read_byte(display_fields.VCOUNT.v, address % 2);
      break;
    }
    case BG0CNT ... BG0CNT + 1: {
      retval = read_byte(display_fields.BG0CNT.v, address % 0x2);
      // SPDLOG_DEBUG("NEW BG0CNT: {:#010x}", display_fields.BG0CNT.v);
      break;
    }
    case BG1CNT ... BG1CNT + 1: {
      retval = read_byte(display_fields.BG1CNT.v, address % 0x2);
      // SPDLOG_DEBUG("NEW BG1CNT: {:#010x}", display_fields.BG1CNT.v);
      break;
    }
    case BG2CNT ... BG2CNT + 1: {
      retval = read_byte(display_fields.BG2CNT.v, address % 0x2);
      // SPDLOG_DEBUG("NEW BG2CNT: {:#010x}", display_fields.BG2CNT.v);
      break;
    }
    case BG3CNT ... BG3CNT + 1: {
      retval = read_byte(display_fields.BG3CNT.v, address % 0x2);
      // SPDLOG_DEBUG("NEW BG3CNT: {:#010x}", display_fields.BG3CNT.v);
      break;
    }
    case WININ:
    case WININ + 1: {
      retval = read_byte(display_fields.WININ.v, address % 0x2);
      break;
    }

    case WINOUT:
    case WINOUT + 1: {
      retval = read_byte(display_fields.WINOUT.v, address % 0x2);
      break;
    }

    case BLDCNT:
    case BLDCNT + 1: {
      retval = read_byte(display_fields.BLDCNT.v, address % 0x2);
      break;
    }

    case BLDALPHA:
    case BLDALPHA + 1: {
      retval = read_byte(display_fields.BLDALPHA.v, address % 0x2);
      break;
    }

    case SOUND1CNT_L: {
      retval = read_byte(sound_registers.SOUND1CNT_L.v, address % 0x2);
      break;
    }

    case SOUND1CNT_H: {
      retval = read_byte(sound_registers.SOUND1CNT_H.v, 0) & 0b11000000;
      break;
    }
    case SOUND1CNT_H + 1: {
      retval = read_byte(sound_registers.SOUND1CNT_H.v, 1);
      break;
    }

    case SOUND1CNT_X + 1: {
      retval = read_byte(sound_registers.SOUND1CNT_X.v, 1) & 0b01000000;
      break;
    }

    case SOUND2CNT_L: {
      retval = read_byte(sound_registers.SOUND2CNT_L.v, 0) & 0xc0;
      break;
    }
    case SOUND2CNT_L + 1: {
      retval = read_byte(sound_registers.SOUND2CNT_L.v, 1);
      break;
    }

    // case SOUND2CNT_H: {
    //   retval = read_byte(sound_registers.SOUND2CNT_H.v, 0);
    //   break;
    // }
    case SOUND2CNT_H + 1: {
      retval = read_byte(sound_registers.SOUND2CNT_H.v, 1) & 0x40;
      break;
    }

    case SOUND3CNT_L: {
      retval = read_byte(sound_registers.SOUND3CNT_L.v, 0) & 0b11100000;
      break;
    }

    case SOUND3CNT_H + 1: {
      retval = read_byte(sound_registers.SOUND3CNT_H.v, 1) & 0b11100000;
      break;
    }

    case SOUND3CNT_X + 1: {
      retval = read_byte(sound_registers.SOUND3CNT_X.v, 1) & 0b01000000;
      break;
    }

    case SOUND4CNT_L + 1: {
      retval = read_byte(sound_registers.SOUND4CNT_L.v, 1);
      break;
    }

    case SOUND4CNT_H: {
      retval = read_byte(sound_registers.SOUND4CNT_H.v, 0);
      break;
    }
    case SOUND4CNT_H + 1: {
      retval = read_byte(sound_registers.SOUND4CNT_H.v, 1) & 0b01000000;
      break;
    }

    case SOUNDCNT_L:
    case SOUNDCNT_L + 1: {
      retval = read_byte(sound_registers.SOUNDCNT_L.v, address % 2);
      break;
    }

    case SOUNDCNT_H:
    case SOUNDCNT_H + 1: {
      retval = read_byte(sound_registers.SOUNDCNT_H.v, address % 2);
      break;
    }

    case SOUNDCNT_X: {
      retval = read_byte(sound_registers.SOUNDCNT_X.v, 0);
      break;
    }

    case SOUNDBIAS: {
      retval = system_control.sound_bias;
      break;
    }
    case WAVE_RAM: bus_logger->debug("READING FROM WAVE_RAM UNIMPL"); break;
    // case FIFO_A: break;
    // case FIFO_B: break;
    case DMA0SAD: break;
    case DMA0DAD: break;
    case DMA0CNT_L: break;
    case DMA0CNT_H ... DMA0CNT_H + 1: {
      retval = ch0->get_values_cnt_h(address % 0x2);
      break;
    }
    case DMA1SAD: break;
    case DMA1DAD: break;
    case DMA1CNT_L: bus_logger->debug("READING FROM DMA1CNT_L UNIMPL"); break;
    case DMA1CNT_H ... DMA1CNT_H + 1: {
      retval = ch1->get_values_cnt_h(address % 0x2);
      break;
    }
    case DMA2SAD: break;
    case DMA2DAD: break;
    case DMA2CNT_L: bus_logger->debug("READING FROM DMA2CNT_L UNIMPL"); break;
    case DMA2CNT_H ... DMA2CNT_H + 1: {
      retval = ch2->get_values_cnt_h(address % 0x2);
      break;
    }
    case DMA3SAD: bus_logger->debug("READING FROM DMA3SAD UNIMPL"); break;
    case DMA3DAD: bus_logger->debug("READING FROM DMA3DAD UNIMPL"); break;
    case DMA3CNT_L: bus_logger->debug("READING FROM DMA3CNT_L UNIMPL"); break;
    case DMA3CNT_H ... DMA3CNT_H + 1: {
      retval = ch3->get_values_cnt_h(address % 0x2);
      break;
    }
    case TM0CNT_L: bus_logger->debug("READING FROM TM0CNT_L UNIMPL"); break;
    case TM0CNT_H: bus_logger->debug("READING FROM TM0CNT_H UNIMPL"); break;
    case TM1CNT_L: bus_logger->debug("READING FROM TM1CNT_L UNIMPL"); break;
    case TM1CNT_H: bus_logger->debug("READING FROM TM1CNT_H UNIMPL"); break;
    case TM2CNT_L: bus_logger->debug("READING FROM TM2CNT_L UNIMPL"); break;
    case TM2CNT_H: bus_logger->debug("READING FROM TM2CNT_H UNIMPL"); break;
    case TM3CNT_L: bus_logger->debug("READING FROM TM3CNT_L UNIMPL"); break;
    case TM3CNT_H: bus_logger->debug("READING FROM TM3CNT_H UNIMPL"); break;
    case SIODATA32: bus_logger->debug("READING FROM SIODATA32 UNIMPL"); break;
    case SIOMULTI1: bus_logger->debug("READING FROM SIOMULTI1 UNIMPL"); break;
    case SIOMULTI2: bus_logger->debug("READING FROM SIOMULTI2 UNIMPL"); break;
    case SIOMULTI3: bus_logger->debug("READING FROM SIOMULTI3 UNIMPL"); break;
    case SIOCNT: bus_logger->debug("READING FROM SIOCNT UNIMPL"); break;
    case SIOMLT_SEND: bus_logger->debug("READING FROM SIOMLT_SEND UNIMPL"); break;
    case KEYINPUT ... KEYINPUT + 1: {
      retval = read_byte(keypad_input.KEYINPUT.v, address % 2);
      break;
    }
    case KEYCNT: bus_logger->debug("READING FROM UNIMPL KEYCNT"); break;
    case RCNT: bus_logger->debug("READING FROM UNIMPL RCNT"); break;
    case JOYCNT: bus_logger->debug("READING FROM UNIMPL JOYCNT"); break;
    case JOY_RECV: bus_logger->debug("READING FROM UNIMPL JOY_RECV"); break;
    case JOY_TRANS: bus_logger->debug("READING FROM UNIMPL JOY_TRANS"); break;
    case JOYSTAT: bus_logger->debug("READING FROM UNIMPL JOYSTAT"); break;
    case IE ... IE + 1: {
      retval = read_byte(interrupt_control.IE.v, address % 2);
      break;
    }
    case IF ... IF + 1: {
      retval = read_byte(interrupt_control.IF.v, address % 2);
      break;
    }
    case WAITCNT:  break;
    case IME ... IME + 3: {
      retval = read_byte(interrupt_control.IME.v, address % 4);
      break;
    }
    case UNKNOWN136:
    case UNKNOWN136 + 1:
    case UNKNOWN142:
    case UNKNOWN142 + 1:
    case UNKNOWN15A:
    case UNKNOWN15A + 1:
    case UNKNOWN206:
    case UNKNOWN206 + 1:
    case UNKNOWN302:
    case UNKNOWN302 + 1: return 0x00;

    case POSTFLG: break;
    // case HALTCNT: break;
    default: {
      // fmt::println("misaligned/partial read {:#08x} - [{}]", address, get_label(address));
      // assert(0);
      break;
    }
  }
  // fmt::println("[IO READ] {} => {:#010x}", get_label(address), retval);
  return retval;
}
void Bus::io_write(u32 address, u8 value) {
  // auto r = (REG)address;
  // fmt::println("write: {:#010x}", address);
  switch (address) {
    case DISPCNT ... DISPCNT + 1: {
      set_byte(display_fields.DISPCNT.v, address % 2, value);
      bus_logger->debug("NEW DISPCNT: {:#010x}", display_fields.DISPCNT.v);
      break;
    }
    case GREEN_SWAP ... GREEN_SWAP + 1: {
      set_byte(display_fields.GREEN_SWAP.v, address % 2, value);
      bus_logger->debug("NEW GREENSWAP: {:#010x}", display_fields.GREEN_SWAP.v);
      break;
    }
    case DISPSTAT ... DISPSTAT + 1: {
      if (address == DISPSTAT) {
        value = (value & 0b11111000);
        display_fields.DISPSTAT.v |= value;
        // set_byte(display_fields.DISPSTAT.v, address % 2, );
      } else {
        set_byte(display_fields.DISPSTAT.v, address % 2, value);
      }

      bus_logger->debug("NEW DISPSTAT: {:#010x}", display_fields.DISPSTAT.v, address);
      break;
    }
    case VCOUNT: bus_logger->debug("WRITING TO VCOUNT UNIMPL"); break;
    case BG0CNT: {
      u8 old_character_base_block = display_fields.BG0CNT.CHAR_BASE_BLOCK;

      set_byte(display_fields.BG0CNT.v, 0, value);

      u8 new_character_base_block = display_fields.BG0CNT.CHAR_BASE_BLOCK;

      if (old_character_base_block != new_character_base_block) ppu->state.cbb_changed[0] = true;

      bus_logger->debug("NEW BG0CNT: {:#010x}", display_fields.BG0CNT.v);
      break;
    }

    case BG0CNT + 1: {
      set_byte(display_fields.BG0CNT.v, 1, value & 0xdf);
      break;
    }
    case BG1CNT: {
      u8 old_character_base_block = display_fields.BG1CNT.CHAR_BASE_BLOCK;

      set_byte(display_fields.BG1CNT.v, 0, value);

      u8 new_character_base_block = display_fields.BG1CNT.CHAR_BASE_BLOCK;

      if (old_character_base_block != new_character_base_block) ppu->state.cbb_changed[1] = true;
      bus_logger->debug("NEW BG1CNT: {:#010x}", display_fields.BG1CNT.v);
      break;
    }
    case BG1CNT + 1: {
      set_byte(display_fields.BG1CNT.v, 1, value & 0xdf);
      break;
    };

    case BG2CNT ... BG2CNT + 1: {
      u8 cbb = display_fields.BG2CNT.CHAR_BASE_BLOCK;
      set_byte(display_fields.BG2CNT.v, address % 0x2, value);
      bus_logger->debug("NEW BG2CNT: {:#010x}", display_fields.BG2CNT.v);
      u8 n_cbb = display_fields.BG2CNT.CHAR_BASE_BLOCK;

      if (cbb != n_cbb) ppu->state.cbb_changed[2] = true;
      break;
    }
    case BG3CNT ... BG3CNT + 1: {
      u8 cbb = display_fields.BG3CNT.CHAR_BASE_BLOCK;
      set_byte(display_fields.BG3CNT.v, address % 0x2, value);
      u8 n_cbb = display_fields.BG3CNT.CHAR_BASE_BLOCK;

      if (cbb != n_cbb) ppu->state.cbb_changed[3] = true;
      bus_logger->debug("NEW BG3CNT: {:#010x}", display_fields.BG3CNT.v);
      break;
    }

    case BG0HOFS ... BG0HOFS + 1: {
      set_byte(display_fields.BG0HOFS.v, address % 0x2, value);
      break;
    }
    case BG0VOFS ... BG0VOFS + 1: {
      set_byte(display_fields.BG0VOFS.v, address % 0x2, value);
      break;
    }

    case BG1VOFS ... BG1VOFS + 1: {
      set_byte(display_fields.BG1VOFS.v, address % 0x2, value);
      break;
    }
    case BG1HOFS ... BG1HOFS + 1: {
      set_byte(display_fields.BG1HOFS.v, address % 0x2, value);
      break;
    }

    case BG2VOFS ... BG2VOFS + 1: {
      set_byte(display_fields.BG2VOFS.v, address % 0x2, value);
      break;
    }
    case BG2HOFS ... BG2HOFS + 1: {
      set_byte(display_fields.BG2HOFS.v, address % 0x2, value);
      break;
    }

    case BG3HOFS ... BG3HOFS + 1: {
      set_byte(display_fields.BG3HOFS.v, address % 0x2, value);
      break;
    }
    case BG3VOFS ... BG3VOFS + 1: {
      set_byte(display_fields.BG3VOFS.v, address % 0x2, value);
      break;
    }

    case BG2PA: bus_logger->debug("WRITING TO BG2PA UNIMPL"); break;
    case BG2PB: bus_logger->debug("WRITING TO BG2PB UNIMPL"); break;
    case BG2PC: bus_logger->debug("WRITING TO BG2PC UNIMPL"); break;
    case BG2PD: bus_logger->debug("WRITING TO BG2PD UNIMPL"); break;
    case BG2X: bus_logger->debug("WRITING TO BG2X UNIMPL"); break;
    case BG2Y: bus_logger->debug("WRITING TO BG2Y UNIMPL"); break;
    case BG3PA: bus_logger->debug("WRITING TO BG3PA UNIMPL"); break;
    case BG3PB: bus_logger->debug("WRITING TO BG3PB UNIMPL"); break;
    case BG3PC: bus_logger->debug("WRITING TO BG3PC UNIMPL"); break;
    case BG3PD: bus_logger->debug("WRITING TO BG3PD UNIMPL"); break;
    case BG3X: bus_logger->debug("WRITING TO BG3X UNIMPL"); break;
    case BG3Y: bus_logger->debug("WRITING TO BG3Y UNIMPL"); break;
    case WIN0H: bus_logger->debug("WRITING TO WIN0H UNIMPL"); break;
    case WIN1H: bus_logger->debug("WRITING TO WIN1H UNIMPL"); break;
    case WIN0V: bus_logger->debug("WRITING TO WIN0V UNIMPL"); break;
    case WIN1V: bus_logger->debug("WRITING TO WIN1V UNIMPL"); break;
    case WININ:
    case WININ + 1: {
      set_byte(display_fields.WININ.v, address % 0x2, value & 0x3f);
      break;
    }

    case WINOUT:
    case WINOUT + 1: {
      set_byte(display_fields.WINOUT.v, address % 0x2, value & 0x3f);
      break;
    }

    case MOSAIC: bus_logger->debug("WRITING TO MOSAIC UNIMPL"); break;
    case BLDCNT: {
      set_byte(display_fields.BLDCNT.v, 0, value);
      break;
    }
    case BLDCNT + 1: {
      set_byte(display_fields.BLDCNT.v, 1, value & 0x3f);
      break;
    }
    case BLDALPHA:
    case BLDALPHA + 1: {
      set_byte(display_fields.BLDALPHA.v, address % 0x2, value & 0x1f);
      break;
    }

    case BLDY: bus_logger->debug("WRITING TO BLDY UNIMPL"); break;
    case SOUND1CNT_L: {
      set_byte(sound_registers.SOUND1CNT_L.v, 0, value & 0x7f);
      break;
    }
    case SOUND1CNT_H:
    case SOUND1CNT_H + 1: {
      set_byte(sound_registers.SOUND1CNT_H.v, address % 2, value);
      // bus_logger->debug("WRITING TO SOUND1CNT_H UNIMPL");
      break;
    }

    case SOUND1CNT_X: {
      set_byte(sound_registers.SOUND1CNT_X.v, 0, value);
      break;
    };
    case SOUND1CNT_X + 1: {
      set_byte(sound_registers.SOUND1CNT_X.v, 1, value & 0xf8);
      break;
    }

    case SOUND2CNT_L:
    case SOUND2CNT_L + 1: {
      set_byte(sound_registers.SOUND2CNT_L.v, address % 2, value);
      break;
    }

    case SOUND2CNT_H:
    case SOUND2CNT_H + 1: {
      set_byte(sound_registers.SOUND2CNT_H.v, address % 2, value);
      // bus_logger->debug("WRITING TO SOUND1CNT_H UNIMPL");
      break;
    }

    case SOUND3CNT_L: {
      set_byte(sound_registers.SOUND3CNT_L.v, 0, value);
      break;
    }

    case SOUND3CNT_H:
    case SOUND3CNT_H + 1: {
      set_byte(sound_registers.SOUND3CNT_H.v, address % 2, value);
      // bus_logger->debug("WRITING TO SOUND1CNT_H UNIMPL");
      break;
    }

    case SOUND3CNT_X:
    case SOUND3CNT_X + 1: {
      set_byte(sound_registers.SOUND3CNT_X.v, address % 2, value);
      break;
    };

    case SOUND4CNT_L:
    case SOUND4CNT_L + 1: {
      set_byte(sound_registers.SOUND4CNT_L.v, address % 2, value);
      break;
    }

    case SOUND4CNT_H:
    case SOUND4CNT_H + 1: {
      set_byte(sound_registers.SOUND4CNT_H.v, address % 2, value);
      break;
    }
    case SOUNDCNT_L: {
      set_byte(sound_registers.SOUNDCNT_L.v, address % 2, value & 0b01110111);
      break;
    }
    case SOUNDCNT_L + 1: {
      set_byte(sound_registers.SOUNDCNT_L.v, address % 2, value);
      break;
    }

    case SOUNDCNT_H: {
      set_byte(sound_registers.SOUNDCNT_H.v, address % 2, value & 0b00001111);
      break;
    }
    case SOUNDCNT_H + 1: {
      set_byte(sound_registers.SOUNDCNT_H.v, address % 2, value & 0b01110111);
      break;
    }

    case SOUNDCNT_X: {
      set_byte(sound_registers.SOUNDCNT_X.v, 0, value & (1 << 7));
      break;
    }

    case SOUNDBIAS: {
      system_control.sound_bias = value;
      break;
    }
    case WAVE_RAM: bus_logger->debug("WRITING TO WAVE_RAM UNIMPL"); break;
    case FIFO_A: bus_logger->debug("WRITING TO FIFO_A UNIMPL"); break;
    case FIFO_B: bus_logger->debug("WRITING TO FIFO_B UNIMPL"); break;

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
    case DMA0CNT_H ... DMA0CNT_H + 1: {
      ch0->set_values_cnt_h(address % 0x2, value);
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

    case DMA1CNT_L ... DMA1CNT_L + 1: {
      set_byte(ch1->dmacnt_l.v, address % 0x2, value);
      break;
    }
    case DMA1CNT_H ... DMA1CNT_H + 1: {
      ch1->set_values_cnt_h(address % 0x2, value);
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

    case DMA2CNT_L ... DMA2CNT_L + 1: {
      set_byte(ch2->dmacnt_l.v, address % 0x2, value);
      break;
    }
    case DMA2CNT_H ... DMA2CNT_H + 1: {
      ch2->set_values_cnt_h(address % 0x2, value);
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
      ch3->set_values_cnt_h(address % 0x2, value);
      break;
    }
    case TM0CNT_L: bus_logger->debug("WRITING TO TM0CNT_L UNIMPL"); break;
    case TM0CNT_H: bus_logger->debug("WRITING TO TM0CNT_H UNIMPL"); break;
    case TM1CNT_L: bus_logger->debug("WRITING TO TM1CNT_L UNIMPL"); break;
    case TM1CNT_H: bus_logger->debug("WRITING TO TM1CNT_H UNIMPL"); break;
    case TM2CNT_L: bus_logger->debug("WRITING TO TM2CNT_L UNIMPL"); break;
    case TM2CNT_H: bus_logger->debug("WRITING TO TM2CNT_H UNIMPL"); break;
    case TM3CNT_L: bus_logger->debug("WRITING TO TM3CNT_L UNIMPL"); break;
    case TM3CNT_H: bus_logger->debug("WRITING TO TM3CNT_H UNIMPL"); break;
    case SIODATA32: bus_logger->debug("WRITING TO SIODATA32 UNIMPL"); break;
    case SIOMULTI1: bus_logger->debug("WRITING TO SIOMULTI1 UNIMPL"); break;
    case SIOMULTI2: bus_logger->debug("WRITING TO SIOMULTI2 UNIMPL"); break;
    case SIOMULTI3: bus_logger->debug("WRITING TO SIOMULTI3 UNIMPL"); break;
    case SIOCNT: bus_logger->debug("WRITING TO SIOCNT UNIMPL"); break;
    case SIOMLT_SEND: bus_logger->debug("WRITING TO SIOMLT_SEND UNIMPL"); break;
    case KEYINPUT: break;
    case KEYCNT: bus_logger->debug("WRITING TO UNIMPL KEYCNT"); break;
    case RCNT: bus_logger->debug("WRITING TO UNIMPL RCNT"); break;
    case JOYCNT: bus_logger->debug("WRITING TO UNIMPL JOYCNT"); break;
    case JOY_RECV: bus_logger->debug("WRITING TO UNIMPL JOY_RECV"); break;
    case JOY_TRANS: bus_logger->debug("WRITING TO UNIMPL JOY_TRANS"); break;
    case JOYSTAT: bus_logger->debug("WRITING TO UNIMPL JOYSTAT"); break;
    case IE ... IE + 1: {
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
    case WAITCNT: bus_logger->debug("WRITING TO UNIMPL WAITCNT"); break;
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
    case HALTCNT: break;
    default: {
      bus_logger->debug("misaligned write: {:#010x}", address);
    }
  }
}

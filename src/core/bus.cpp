#include "bus.hpp"

#include <cstdio>
#include <stdexcept>

#include "labels.hpp"

void Bus::request_interrupt(InterruptType t) { interrupt_control.IF.v |= (1 << t); }

void Bus::handle_interrupts() { assert(0); };
u8 Bus::read8(u32 address) {
  u32 v;
  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      return BIOS.at(address);
    }

    case 0x02000000 ... 0x0203FFFF: {
      v = EWRAM.at(address - 0x02000000);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      // v = shift_16(IWRAM, address - 0x03000000);
      v = IWRAM.at(address - 0x03000000);
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      v = io_read(address);
      fmt::println("8 bit IO read");
      break;
    }

    case 0x06000000 ... 0x6FFFFFF: {
      return 0x0;
      assert(0);
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
      spdlog::warn("[R8] unused/oob memory read: {:#X}", address);
      return 0xFF;
    }
  }

  SPDLOG_DEBUG("[R8] {:#010x} => {:#04x}", address, v);
  return v;
};
u16 Bus::read16(u32 address) {
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
      v = *(uint16_t*)(&EWRAM.data()[address - 0x02000000]);
      // v = shift_16(EWRAM, address - 0x02000000);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      v = *(uint16_t*)(&IWRAM.data()[address - 0x03000000]);
      // v = shift_16(IWRAM, address - 0x03000000);
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      for (size_t i = 0; i < 2; i++) {
        v |= (io_read(address) << 8 * i);
      }
      // fmt::println("16 bit IO read");
      break;
    }

    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      return *(uint16_t*)(&PALETTE_RAM.data()[address % 0x400]);
    }

    case 0x06000000 ... 0X06017FFF: {
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
      // spdlog::warn("[R16] unused/oob memory read: {:#X}", address);
      // TODO: implement open bus behaviour
      return 0xFFFF;
    }
  }
  SPDLOG_DEBUG("[R16] {:#010X} => {:#010x}", address, v);
  return v;
};
u32 Bus::read32(u32 address) {
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
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      // fmt::println("trying to read io address [32]: {:#010x}", address);
      // fmt::println("fun value: {}", fun_value);
      // assert(0);
      // u32 retval = 0;
      for (size_t i = 0; i < 4; i++) {
        v |= (io_read(address) << 8 * i);
      }
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

    case 0x06000000 ... 0x06FFFFFF: {
      v = *(uint32_t*)(&VRAM.data()[address % 0x20000]);
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
      v = *(uint32_t*)(&pak->data[address - 0xA8000000]);
      break;
    }

    case 0x0C000000 ... 0x0DFFFFFF: {
      // game pak read (ws2)
      v = *(uint32_t*)(&pak->data[address - 0x0C000000]);
      break;
    }

    default: {
      spdlog::warn("[R32] unused/oob memory read: {:#X}", address);
      return 0xFFFFFFFF;  // TODO: implement open bus behavior
    }
  }

  SPDLOG_DEBUG("[R32] {} => {:#010x}", get_label(address), v);
  return v;
};

void Bus::write8(const u32 address, u8 value) {
  switch (address) {
    case 0x02000000 ... 0x0203FFFF: {
      EWRAM.at(address - 0x02000000) = value;
      // set32(EWRAM, address - 0x02000000, value);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      IWRAM.at(address - 0x03000000) = value;
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      // IO
      handle_io_write(address, value);
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
      assert(0);
      return;
      // VRAM.at(address - 0x06000000) = value;
      // // set32(VRAM, address - 0x06000000, value);
      // fmt::println("writing to VRAM (8b)");
      // assert(0);
      // break;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      return;
      OAM.at(address - 0x07000000) = value;
      // set32(OAM, address - 0x07000000, value);
      break;
    }

    default: {
      spdlog::warn("[W8] unused/oob memory write: {:#X}", address);
      return;
      // break;
    }
  }
  SPDLOG_INFO("[W8] {} => {:#x}", get_label(address), value);
}

void Bus::write16(const u32 address, u16 value) {
  switch (address) {
    case 0x02000000 ... 0x0203FFFF: {
      // set16(EWRAM, address - 0x02000000, value);
      *(uint16_t*)(&EWRAM.data()[address - 0x02000000]) = value;
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      // set16(IWRAM, address - 0x03000000, value);
      *(uint16_t*)(&IWRAM.data()[address - 0x03000000]) = value;
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
      *(uint16_t*)(&VRAM.data()[address % 0x18000]) = value;
      break;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      // set16(OAM, address - 0x07000000, value);
      *(uint16_t*)(&OAM.data()[address - 0x07000000]) = value;
      break;
    }

    default: {
      spdlog::warn("unused/oob memory write: {:#X}", address);
      return;
      // break;
    }
  }
  // SPDLOG_INFO("[W16] {} => {:#x}", get_label(address), value);
}

u8 Bus::io_read(u32 address) {
  u8 retval = 0x0;

  switch (address) {
    case DISPCNT ... DISPCNT + 1: {
      retval = read_byte(display_fields.DISPCNT.v, address % 2);
      break;
    }
    case GREEN_SWAP ... GREEN_SWAP + 1: SPDLOG_DEBUG("READING FROM GREEN_SWAP UNIMPL"); break;
    case DISPSTAT ... DISPSTAT + 1: {
      retval = read_byte(display_fields.DISPSTAT.v, address % 2);
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
    case BG0HOFS: fmt::println("READING FROM BG0HOFS UNIMPL"); break;
    case BG0VOFS: fmt::println("READING FROM BG0VOFS UNIMPL"); break;
    case BG1HOFS: SPDLOG_DEBUG("READING FROM BG1HOFS UNIMPL"); break;
    case BG1VOFS: SPDLOG_DEBUG("READING FROM BG1VOFS UNIMPL"); break;
    case BG2HOFS: SPDLOG_DEBUG("READING FROM BG2HOFS UNIMPL"); break;
    case BG2VOFS: SPDLOG_DEBUG("READING FROM BG2VOFS UNIMPL"); break;
    case BG3HOFS: SPDLOG_DEBUG("READING FROM BG3HOFS UNIMPL"); break;
    case BG3VOFS: SPDLOG_DEBUG("READING FROM BG3VOFS UNIMPL"); break;
    case BG2PA: SPDLOG_DEBUG("READING FROM BG2PA UNIMPL"); break;
    case BG2PB: SPDLOG_DEBUG("READING FROM BG2PB UNIMPL"); break;
    case BG2PC: SPDLOG_DEBUG("READING FROM BG2PC UNIMPL"); break;
    case BG2PD: SPDLOG_DEBUG("READING FROM BG2PD UNIMPL"); break;
    case BG2X: SPDLOG_DEBUG("READING FROM BG2X UNIMPL"); break;
    case BG2Y: SPDLOG_DEBUG("READING FROM BG2Y UNIMPL"); break;
    case BG3PA: SPDLOG_DEBUG("READING FROM BG3PA UNIMPL"); break;
    case BG3PB: SPDLOG_DEBUG("READING FROM BG3PB UNIMPL"); break;
    case BG3PC: SPDLOG_DEBUG("READING FROM BG3PC UNIMPL"); break;
    case BG3PD: SPDLOG_DEBUG("READING FROM BG3PD UNIMPL"); break;
    case BG3X: SPDLOG_DEBUG("READING FROM BG3X UNIMPL"); break;
    case BG3Y: SPDLOG_DEBUG("READING FROM BG3Y UNIMPL"); break;
    case WIN0H: SPDLOG_DEBUG("READING FROM WIN0H UNIMPL"); break;
    case WIN1H: SPDLOG_DEBUG("READING FROM WIN1H UNIMPL"); break;
    case WIN0V: SPDLOG_DEBUG("READING FROM WIN0V UNIMPL"); break;
    case WIN1V: SPDLOG_DEBUG("READING FROM WIN1V UNIMPL"); break;
    case WININ: SPDLOG_DEBUG("READING FROM WININ UNIMPL"); break;
    case WINOUT: SPDLOG_DEBUG("READING FROM WINOUT UNIMPL"); break;
    case MOSAIC: SPDLOG_DEBUG("READING FROM MOSAIC UNIMPL"); break;
    case BLDCNT: SPDLOG_DEBUG("READING FROM BLDCNT UNIMPL"); break;
    case BLDALPHA: SPDLOG_DEBUG("READING FROM BLDALPHA UNIMPL"); break;
    case BLDY: SPDLOG_DEBUG("READING FROM BLDY UNIMPL"); break;
    case SOUND1CNT_L: SPDLOG_DEBUG("READING FROM SOUND1CNT_L UNIMPL"); break;
    case SOUND1CNT_H: SPDLOG_DEBUG("READING FROM SOUND1CNT_H UNIMPL"); break;
    case SOUND1CNT_X: SPDLOG_DEBUG("READING FROM SOUND1CNT_X UNIMPL"); break;
    case SOUND2CNT_L: SPDLOG_DEBUG("READING FROM SOUND2CNT_L UNIMPL"); break;
    case SOUND2CNT_H: SPDLOG_DEBUG("READING FROM SOUND2CNT_H UNIMPL"); break;
    case SOUND3CNT_L: SPDLOG_DEBUG("READING FROM SOUND3CNT_L UNIMPL"); break;
    case SOUND3CNT_H: SPDLOG_DEBUG("READING FROM SOUND3CNT_H UNIMPL"); break;
    case SOUND3CNT_X: SPDLOG_DEBUG("READING FROM SOUND3CNT_X UNIMPL"); break;
    case SOUND4CNT_L: SPDLOG_DEBUG("READING FROM SOUND4CNT_L UNIMPL"); break;
    case SOUND4CNT_H: SPDLOG_DEBUG("READING FROM SOUND4CNT_H UNIMPL"); break;
    case SOUNDCNT_L: SPDLOG_DEBUG("READING FROM SOUNDCNT_L UNIMPL"); break;
    case SOUNDCNT_H: SPDLOG_DEBUG("READING FROM SOUNDCNT_H UNIMPL"); break;
    case SOUNDCNT_X: SPDLOG_DEBUG("READING FROM SOUNDCNT_X UNIMPL"); break;
    case SOUNDBIAS: {
      retval = system_control.sound_bias;
      break;
    }
    case WAVE_RAM: SPDLOG_DEBUG("READING FROM WAVE_RAM UNIMPL"); break;
    case FIFO_A: SPDLOG_DEBUG("READING FROM FIFO_A UNIMPL"); break;
    case FIFO_B: SPDLOG_DEBUG("READING FROM FIFO_B UNIMPL"); break;
    case DMA0SAD: SPDLOG_DEBUG("READING FROM DMA0SAD UNIMPL"); break;
    case DMA0DAD: SPDLOG_DEBUG("READING FROM DMA0DAD UNIMPL"); break;
    case DMA0CNT_L: SPDLOG_DEBUG("READING FROM DMA0CNT_L UNIMPL"); break;
    case DMA0CNT_H: SPDLOG_DEBUG("READING FROM DMA0CNT_H UNIMPL"); break;
    case DMA1SAD: SPDLOG_DEBUG("READING FROM DMA1SAD UNIMPL"); break;
    case DMA1DAD: SPDLOG_DEBUG("READING FROM DMA1DAD UNIMPL"); break;
    case DMA1CNT_L: SPDLOG_DEBUG("READING FROM DMA1CNT_L UNIMPL"); break;
    case DMA1CNT_H: SPDLOG_DEBUG("READING FROM DMA1CNT_H UNIMPL"); break;
    case DMA2SAD: SPDLOG_DEBUG("READING FROM DMA2SAD UNIMPL"); break;
    case DMA2DAD: SPDLOG_DEBUG("READING FROM DMA2DAD UNIMPL"); break;
    case DMA2CNT_L: SPDLOG_DEBUG("READING FROM DMA2CNT_L UNIMPL"); break;
    case DMA2CNT_H: SPDLOG_DEBUG("READING FROM DMA2CNT_H UNIMPL"); break;
    case DMA3SAD: SPDLOG_DEBUG("READING FROM DMA3SAD UNIMPL"); break;
    case DMA3DAD: SPDLOG_DEBUG("READING FROM DMA3DAD UNIMPL"); break;
    case DMA3CNT_L: SPDLOG_DEBUG("READING FROM DMA3CNT_L UNIMPL"); break;
    case DMA3CNT_H: SPDLOG_DEBUG("READING FROM DMA3CNT_H UNIMPL"); break;
    case TM0CNT_L: SPDLOG_DEBUG("READING FROM TM0CNT_L UNIMPL"); break;
    case TM0CNT_H: SPDLOG_DEBUG("READING FROM TM0CNT_H UNIMPL"); break;
    case TM1CNT_L: SPDLOG_DEBUG("READING FROM TM1CNT_L UNIMPL"); break;
    case TM1CNT_H: SPDLOG_DEBUG("READING FROM TM1CNT_H UNIMPL"); break;
    case TM2CNT_L: SPDLOG_DEBUG("READING FROM TM2CNT_L UNIMPL"); break;
    case TM2CNT_H: SPDLOG_DEBUG("READING FROM TM2CNT_H UNIMPL"); break;
    case TM3CNT_L: SPDLOG_DEBUG("READING FROM TM3CNT_L UNIMPL"); break;
    case TM3CNT_H: SPDLOG_DEBUG("READING FROM TM3CNT_H UNIMPL"); break;
    case SIODATA32: SPDLOG_DEBUG("READING FROM SIODATA32 UNIMPL"); break;
    case SIOMULTI1: SPDLOG_DEBUG("READING FROM SIOMULTI1 UNIMPL"); break;
    case SIOMULTI2: SPDLOG_DEBUG("READING FROM SIOMULTI2 UNIMPL"); break;
    case SIOMULTI3: SPDLOG_DEBUG("READING FROM SIOMULTI3 UNIMPL"); break;
    case SIOCNT: SPDLOG_DEBUG("READING FROM SIOCNT UNIMPL"); break;
    case SIOMLT_SEND: SPDLOG_DEBUG("READING FROM SIOMLT_SEND UNIMPL"); break;
    case KEYINPUT ... KEYINPUT + 1: {
      retval = read_byte(keypad_input.KEYINPUT.v, address % 2);
      break;
    }
    case KEYCNT: SPDLOG_DEBUG("READING FROM UNIMPL KEYCNT"); break;
    case RCNT: SPDLOG_DEBUG("READING FROM UNIMPL RCNT"); break;
    case JOYCNT: SPDLOG_DEBUG("READING FROM UNIMPL JOYCNT"); break;
    case JOY_RECV: SPDLOG_DEBUG("READING FROM UNIMPL JOY_RECV"); break;
    case JOY_TRANS: SPDLOG_DEBUG("READING FROM UNIMPL JOY_TRANS"); break;
    case JOYSTAT: SPDLOG_DEBUG("READING FROM UNIMPL JOYSTAT"); break;
    case IE ... IE + 1: {
      retval = read_byte(interrupt_control.IE.v, address % 2);
      break;
    }
    case IF ... IF + 1: {
      retval = read_byte(interrupt_control.IF.v, address % 2);
      break;
    }
    case WAITCNT: SPDLOG_DEBUG("READING FROM UNIMPL WAITCNT"); break;
    case IME ... IME + 3: {
      retval = read_byte(interrupt_control.IME.v, address % 4);
      break;
    }
    case POSTFLG: break;
    case HALTCNT: break;
    default: {
      fmt::println("misaligned/partial read {:#08x}", address);
      assert(0);
    }
  }
  // fmt::println("[IO READ] {} => {:#010x}", get_label(address), retval);
  return retval;
}
void Bus::handle_io_write(u32 address, u8 value) {
  // auto r = (REG)address;
  // fmt::println("write: {:#010x}", address);
  switch (address) {
    case DISPCNT ... DISPCNT + 1: {
      set_byte(display_fields.DISPCNT.v, address % 2, value);
      fmt::println("NEW DISPCNT: {:#010x}", display_fields.DISPCNT.v);
      break;
    }
    case GREEN_SWAP ... GREEN_SWAP + 1: {
      set_byte(display_fields.GREEN_SWAP.v, address % 2, value);
      SPDLOG_DEBUG("NEW GREENSWAP: {:#010x}", display_fields.GREEN_SWAP.v);
      break;
    }
    case DISPSTAT ... DISPSTAT + 1: {
      set_byte(display_fields.DISPSTAT.v, address % 2, value);

      SPDLOG_DEBUG("NEW DISPSTAT: {:#010x}", display_fields.DISPSTAT.v, address);
      break;
    }
    case VCOUNT: SPDLOG_DEBUG("WRITING TO VCOUNT UNIMPL"); break;
    case BG0CNT ... BG0CNT + 1: {
      set_byte(display_fields.BG0CNT.v, address % 0x2, value);
      SPDLOG_DEBUG("NEW BG0CNT: {:#010x}", display_fields.BG0CNT.v);
      break;
    }
    case BG1CNT ... BG1CNT + 1: {
      set_byte(display_fields.BG1CNT.v, address % 0x2, value);
      SPDLOG_DEBUG("NEW BG1CNT: {:#010x}", display_fields.BG1CNT.v);
      break;
    }
    case BG2CNT ... BG2CNT + 1: {
      set_byte(display_fields.BG2CNT.v, address % 0x2, value);
      SPDLOG_DEBUG("NEW BG2CNT: {:#010x}", display_fields.BG2CNT.v);
      break;
    }
    case BG3CNT ... BG3CNT + 1: {
      set_byte(display_fields.BG3CNT.v, address % 0x2, value);
      SPDLOG_DEBUG("NEW BG3CNT: {:#010x}", display_fields.BG3CNT.v);
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

    case BG2PA: SPDLOG_DEBUG("WRITING TO BG2PA UNIMPL"); break;
    case BG2PB: SPDLOG_DEBUG("WRITING TO BG2PB UNIMPL"); break;
    case BG2PC: SPDLOG_DEBUG("WRITING TO BG2PC UNIMPL"); break;
    case BG2PD: SPDLOG_DEBUG("WRITING TO BG2PD UNIMPL"); break;
    case BG2X: SPDLOG_DEBUG("WRITING TO BG2X UNIMPL"); break;
    case BG2Y: SPDLOG_DEBUG("WRITING TO BG2Y UNIMPL"); break;
    case BG3PA: SPDLOG_DEBUG("WRITING TO BG3PA UNIMPL"); break;
    case BG3PB: SPDLOG_DEBUG("WRITING TO BG3PB UNIMPL"); break;
    case BG3PC: SPDLOG_DEBUG("WRITING TO BG3PC UNIMPL"); break;
    case BG3PD: SPDLOG_DEBUG("WRITING TO BG3PD UNIMPL"); break;
    case BG3X: SPDLOG_DEBUG("WRITING TO BG3X UNIMPL"); break;
    case BG3Y: SPDLOG_DEBUG("WRITING TO BG3Y UNIMPL"); break;
    case WIN0H: SPDLOG_DEBUG("WRITING TO WIN0H UNIMPL"); break;
    case WIN1H: SPDLOG_DEBUG("WRITING TO WIN1H UNIMPL"); break;
    case WIN0V: SPDLOG_DEBUG("WRITING TO WIN0V UNIMPL"); break;
    case WIN1V: SPDLOG_DEBUG("WRITING TO WIN1V UNIMPL"); break;
    case WININ: SPDLOG_DEBUG("WRITING TO WININ UNIMPL"); break;
    case WINOUT: SPDLOG_DEBUG("WRITING TO WINOUT UNIMPL"); break;
    case MOSAIC: SPDLOG_DEBUG("WRITING TO MOSAIC UNIMPL"); break;
    case BLDCNT: SPDLOG_DEBUG("WRITING TO BLDCNT UNIMPL"); break;
    case BLDALPHA: SPDLOG_DEBUG("WRITING TO BLDALPHA UNIMPL"); break;
    case BLDY: SPDLOG_DEBUG("WRITING TO BLDY UNIMPL"); break;
    case SOUND1CNT_L: SPDLOG_DEBUG("WRITING TO SOUND1CNT_L UNIMPL"); break;
    case SOUND1CNT_H: SPDLOG_DEBUG("WRITING TO SOUND1CNT_H UNIMPL"); break;
    case SOUND1CNT_X: SPDLOG_DEBUG("WRITING TO SOUND1CNT_X UNIMPL"); break;
    case SOUND2CNT_L: SPDLOG_DEBUG("WRITING TO SOUND2CNT_L UNIMPL"); break;
    case SOUND2CNT_H: SPDLOG_DEBUG("WRITING TO SOUND2CNT_H UNIMPL"); break;
    case SOUND3CNT_L: SPDLOG_DEBUG("WRITING TO SOUND3CNT_L UNIMPL"); break;
    case SOUND3CNT_H: SPDLOG_DEBUG("WRITING TO SOUND3CNT_H UNIMPL"); break;
    case SOUND3CNT_X: SPDLOG_DEBUG("WRITING TO SOUND3CNT_X UNIMPL"); break;
    case SOUND4CNT_L: SPDLOG_DEBUG("WRITING TO SOUND4CNT_L UNIMPL"); break;
    case SOUND4CNT_H: SPDLOG_DEBUG("WRITING TO SOUND4CNT_H UNIMPL"); break;
    case SOUNDCNT_L: SPDLOG_DEBUG("WRITING TO SOUNDCNT_L UNIMPL"); break;
    case SOUNDCNT_H: SPDLOG_DEBUG("WRITING TO SOUNDCNT_H UNIMPL"); break;
    case SOUNDCNT_X: SPDLOG_DEBUG("WRITING TO SOUNDCNT_X UNIMPL"); break;
    case SOUNDBIAS: {
      system_control.sound_bias = value;
      break;
    }
    case WAVE_RAM: SPDLOG_DEBUG("WRITING TO WAVE_RAM UNIMPL"); break;
    case FIFO_A: SPDLOG_DEBUG("WRITING TO FIFO_A UNIMPL"); break;
    case FIFO_B: SPDLOG_DEBUG("WRITING TO FIFO_B UNIMPL"); break;
    case DMA0SAD: SPDLOG_DEBUG("WRITING TO DMA0SAD UNIMPL"); break;
    case DMA0DAD: SPDLOG_DEBUG("WRITING TO DMA0DAD UNIMPL"); break;
    case DMA0CNT_L: SPDLOG_DEBUG("WRITING TO DMA0CNT_L UNIMPL"); break;
    case DMA0CNT_H: SPDLOG_DEBUG("WRITING TO DMA0CNT_H UNIMPL"); break;
    case DMA1SAD: SPDLOG_DEBUG("WRITING TO DMA1SAD UNIMPL"); break;
    case DMA1DAD: SPDLOG_DEBUG("WRITING TO DMA1DAD UNIMPL"); break;
    case DMA1CNT_L: SPDLOG_DEBUG("WRITING TO DMA1CNT_L UNIMPL"); break;
    case DMA1CNT_H: SPDLOG_DEBUG("WRITING TO DMA1CNT_H UNIMPL"); break;
    case DMA2SAD: SPDLOG_DEBUG("WRITING TO DMA2SAD UNIMPL"); break;
    case DMA2DAD: SPDLOG_DEBUG("WRITING TO DMA2DAD UNIMPL"); break;
    case DMA2CNT_L: SPDLOG_DEBUG("WRITING TO DMA2CNT_L UNIMPL"); break;
    case DMA2CNT_H: SPDLOG_DEBUG("WRITING TO DMA2CNT_H UNIMPL"); break;
    case DMA3SAD: SPDLOG_DEBUG("WRITING TO DMA3SAD UNIMPL"); break;
    case DMA3DAD: SPDLOG_DEBUG("WRITING TO DMA3DAD UNIMPL"); break;
    case DMA3CNT_L: SPDLOG_DEBUG("WRITING TO DMA3CNT_L UNIMPL"); break;
    case DMA3CNT_H: SPDLOG_DEBUG("WRITING TO DMA3CNT_H UNIMPL"); break;
    case TM0CNT_L: SPDLOG_DEBUG("WRITING TO TM0CNT_L UNIMPL"); break;
    case TM0CNT_H: SPDLOG_DEBUG("WRITING TO TM0CNT_H UNIMPL"); break;
    case TM1CNT_L: SPDLOG_DEBUG("WRITING TO TM1CNT_L UNIMPL"); break;
    case TM1CNT_H: SPDLOG_DEBUG("WRITING TO TM1CNT_H UNIMPL"); break;
    case TM2CNT_L: SPDLOG_DEBUG("WRITING TO TM2CNT_L UNIMPL"); break;
    case TM2CNT_H: SPDLOG_DEBUG("WRITING TO TM2CNT_H UNIMPL"); break;
    case TM3CNT_L: SPDLOG_DEBUG("WRITING TO TM3CNT_L UNIMPL"); break;
    case TM3CNT_H: SPDLOG_DEBUG("WRITING TO TM3CNT_H UNIMPL"); break;
    case SIODATA32: SPDLOG_DEBUG("WRITING TO SIODATA32 UNIMPL"); break;
    case SIOMULTI1: SPDLOG_DEBUG("WRITING TO SIOMULTI1 UNIMPL"); break;
    case SIOMULTI2: SPDLOG_DEBUG("WRITING TO SIOMULTI2 UNIMPL"); break;
    case SIOMULTI3: SPDLOG_DEBUG("WRITING TO SIOMULTI3 UNIMPL"); break;
    case SIOCNT: SPDLOG_DEBUG("WRITING TO SIOCNT UNIMPL"); break;
    case SIOMLT_SEND: SPDLOG_DEBUG("WRITING TO SIOMLT_SEND UNIMPL"); break;
    case KEYINPUT: break;
    case KEYCNT: SPDLOG_DEBUG("WRITING TO UNIMPL KEYCNT"); break;
    case RCNT: SPDLOG_DEBUG("WRITING TO UNIMPL RCNT"); break;
    case JOYCNT: SPDLOG_DEBUG("WRITING TO UNIMPL JOYCNT"); break;
    case JOY_RECV: SPDLOG_DEBUG("WRITING TO UNIMPL JOY_RECV"); break;
    case JOY_TRANS: SPDLOG_DEBUG("WRITING TO UNIMPL JOY_TRANS"); break;
    case JOYSTAT: SPDLOG_DEBUG("WRITING TO UNIMPL JOYSTAT"); break;
    case IE ... IE + 1: {
      set_byte(interrupt_control.IE.v, address % 2, value);
      SPDLOG_DEBUG("WROTE {:#010x} to IE - [{:#010x}]", value, address);
      SPDLOG_DEBUG("NEW IE: {:#010x}", interrupt_control.IE.v, address);
      break;
    }
    case IF ... IF + 1: {
      set_byte(interrupt_control.IF.v, address % 2, value);
      SPDLOG_DEBUG("WROTE {:#010x} to IF - [{:#010x}]", value, address);
      SPDLOG_DEBUG("NEW IF: {:#010x}", interrupt_control.IE.v, address);
      break;
    }
    case WAITCNT: SPDLOG_DEBUG("WRITING TO UNIMPL WAITCNT"); break;
    case IME ... IME + 3: {
      set_byte(interrupt_control.IME.v, address % 4, value);
      SPDLOG_DEBUG("new IME: {:#010x} - [{:#010x}]", interrupt_control.IME.v, address);
    }
    case POSTFLG: break;
    case HALTCNT: break;
    default: {
      fmt::println("misaligned write: {:#010x}", address);
      // exit(-1);
    }
  }
}

void Bus::write32(const u32 address, u32 value) {
  switch (address) {
    case 0x02000000 ... 0x02FFFFFF: {
      // set32(EWRAM, address % 0x40000, value);
      *(uint32_t*)(&EWRAM.data()[address % 0x40000]) = value;
      break;
    }

    case 0x03000000 ... 0x03FFFFFF: {
      *(uint32_t*)(&IWRAM.data()[address % 0x8000]) = value;
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
      *(uint32_t*)(&VRAM.data()[(address % 0x18000)]) = value;
      break;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      *(uint32_t*)(&OAM.data()[(address % 0x400)]) = value;
      break;
    }

    default: {
      spdlog::warn("[W32] unused/oob memory write: {:#010x}", address);
      return;
      // break;
    }
  }
  // SPDLOG_INFO("[W32] {} => {:#010x}", get_label(address), value);
}
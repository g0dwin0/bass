#include "bus.hpp"

#include <cstdio>

#include "labels.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

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
      v = handle_io_read(address);
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
u16 Bus::read16(u32 address, bool quiet) {
  // fmt::println("[R16] {:#010x}", address);
  u32 v;
  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      return *(uint16_t*)(&BIOS.data()[address]);
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
      v = handle_io_read(address);
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
      spdlog::warn("[R16] unused/oob memory read: {:#X}", address);
      return 0xFFFF;
    }
  }
  if (!quiet) { SPDLOG_DEBUG("[R16] {:#010X} => {:#010x}", address, v); }
  return v;
};
u32 Bus::read32(u32 address) {
  // SPDLOG_DEBUG("[R32] {:#010x}", address);
  u32 v;

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
      assert(0);
      v = handle_io_read(address);
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
      // set32(IWRAM, address - 0x03000000, value);
      break;
    }

    case 0x04000000 ... 0x040003FE: {
      // IO
      handle_io_write(address, value);
      // assert(0);
      // set32(IO, address - 0x04000000, value);
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
  SPDLOG_INFO("[W16] {} => {:#x}", get_label(address), value);
}

u32 Bus::handle_io_read(u32 address) {
  u32 retval = 0x0;
  switch (address) {
    case DISPCNT: {
      retval = display_fields.DISPCNT.v;
      break;
    }
    case GREEN_SWAP: SPDLOG_DEBUG("READING FROM GREEN_SWAP UNIMPL"); break;
    case DISPSTAT: {
      retval = display_fields.DISPSTAT.v;
      break;
    }
    case VCOUNT: SPDLOG_DEBUG("READING FROM VCOUNT UNIMPL"); break;
    case BG0CNT: SPDLOG_DEBUG("READING FROM BG0CNT UNIMPL"); break;
    case BG1CNT: SPDLOG_DEBUG("READING FROM BG1CNT UNIMPL"); break;
    case BG2CNT: SPDLOG_DEBUG("READING FROM BG2CNT UNIMPL"); break;
    case BG3CNT: SPDLOG_DEBUG("READING FROM BG3CNT UNIMPL"); break;
    case BG0HOFS: SPDLOG_DEBUG("READING FROM BG0HOFS UNIMPL"); break;
    case BG0VOFS: SPDLOG_DEBUG("READING FROM BG0VOFS UNIMPL"); break;
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
      return system_control.sound_bias;
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
    case KEYINPUT: {
      retval = keypad_input.KEYINPUT.v;
      break;
    }
    case KEYCNT: SPDLOG_DEBUG("READING FROM UINIMPL KEYCNT"); break;
    case RCNT: SPDLOG_DEBUG("READING FROM UINIMPL RCNT"); break;
    case JOYCNT: SPDLOG_DEBUG("READING FROM UINIMPL JOYCNT"); break;
    case JOY_RECV: SPDLOG_DEBUG("READING FROM UINIMPL JOY_RECV"); break;
    case JOY_TRANS: SPDLOG_DEBUG("READING FROM UINIMPL JOY_TRANS"); break;
    case JOYSTAT: SPDLOG_DEBUG("READING FROM UINIMPL JOYSTAT"); break;
    case IE: SPDLOG_DEBUG("READING FROM UINIMPL IE"); break;
    case IF: SPDLOG_DEBUG("READING FROM UINIMPL IF"); break;
    case WAITCNT: SPDLOG_DEBUG("READING FROM UINIMPL WAITCNT"); break;
    case IME: {
      retval = interrupt_control.IME.v;
    }
    case POSTFLG: break;
    case HALTCNT: break;
    default: {
      fmt::println("misaligned/partial read {:#08x}", address);
      assert(0);
    }
  }
  SPDLOG_DEBUG("[IO READ] {:#010X} => {:#08x}", address, retval);
  return retval;
}
void Bus::handle_io_write(u32 address, u8 value) {
  // auto r = (REG)address;
  switch (address) {
    case DISPCNT: {
      display_fields.DISPCNT.v = value;
      break;
    }
    case GREEN_SWAP: SPDLOG_DEBUG("WRITING TO GREEN_SWAP UNIMPL"); break;
    case DISPSTAT: {
      display_fields.DISPSTAT.v = value;
      break;
    }
    case VCOUNT: SPDLOG_DEBUG("WRITING TO VCOUNT UNIMPL"); break;
    case BG0CNT: SPDLOG_DEBUG("WRITING TO BG0CNT UNIMPL"); break;
    case BG1CNT: SPDLOG_DEBUG("WRITING TO BG1CNT UNIMPL"); break;
    case BG2CNT: SPDLOG_DEBUG("WRITING TO BG2CNT UNIMPL"); break;
    case BG3CNT: SPDLOG_DEBUG("WRITING TO BG3CNT UNIMPL"); break;
    case BG0HOFS: SPDLOG_DEBUG("WRITING TO BG0HOFS UNIMPL"); break;
    case BG0VOFS: SPDLOG_DEBUG("WRITING TO BG0VOFS UNIMPL"); break;
    case BG1HOFS: SPDLOG_DEBUG("WRITING TO BG1HOFS UNIMPL"); break;
    case BG1VOFS: SPDLOG_DEBUG("WRITING TO BG1VOFS UNIMPL"); break;
    case BG2HOFS: SPDLOG_DEBUG("WRITING TO BG2HOFS UNIMPL"); break;
    case BG2VOFS: SPDLOG_DEBUG("WRITING TO BG2VOFS UNIMPL"); break;
    case BG3HOFS: SPDLOG_DEBUG("WRITING TO BG3HOFS UNIMPL"); break;
    case BG3VOFS: SPDLOG_DEBUG("WRITING TO BG3VOFS UNIMPL"); break;
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
    } break;
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
    case KEYCNT: SPDLOG_DEBUG("WRITING TO UINIMPL KEYCNT"); break;
    case RCNT: SPDLOG_DEBUG("WRITING TO UINIMPL RCNT"); break;
    case JOYCNT: SPDLOG_DEBUG("WRITING TO UINIMPL JOYCNT"); break;
    case JOY_RECV: SPDLOG_DEBUG("WRITING TO UINIMPL JOY_RECV"); break;
    case JOY_TRANS: SPDLOG_DEBUG("WRITING TO UINIMPL JOY_TRANS"); break;
    case JOYSTAT: SPDLOG_DEBUG("WRITING TO UINIMPL JOYSTAT"); break;
    case IE: {
      SPDLOG_DEBUG("WROTE {:#010x} to  IE - [{:#010x}]", value, address);
      if (address % 2 == 0) {  // 1st byte
        interrupt_control.IE.v &= ~0xFF;
        interrupt_control.IE.v |= value;

      } else {  // 2nd byte
        interrupt_control.IE.v &= ~0xFF00;
        interrupt_control.IE.v |= (value << 8);
      }
      break;
    }
    case IF: SPDLOG_DEBUG("WRITING TO UINIMPL IF"); break;
    case WAITCNT: SPDLOG_DEBUG("WRITING TO UINIMPL WAITCNT"); break;
    case IME: {
      interrupt_control.IME.v = value;
      SPDLOG_DEBUG("WROTE {:#010x} to  IME - [{:#010x}]", value, address);
    }
    case POSTFLG: break;
    case HALTCNT: break;
    default: {
      fmt::println("misaligned write: {:#010x}", address);
      // exit(-1);
    }
  }
}
// TODO: replace with different typepunned arrays
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
      write16(address, (value & 0xFFFF0000) >> 8);
      write16(address + 2, value & 0x0000FFFF);
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
  SPDLOG_INFO("[W32] {} => {:#010x}", get_label(address), value);
}
#include "bus.hpp"

#include <cstdio>

#include "spdlog/spdlog.h"

u16 Bus::shift_16(const std::vector<u8>& vec, const u32 address) { return (vec.at(address + 1) << 8) + (vec.at(address)); }
u32 Bus::shift_32(const std::vector<u8>& vec, const u32 address) { return ((vec.at(address + 3) << 24) + (vec.at(address + 2) << 16) + (vec.at(address + 1) << 8) + (vec.at(address))); }
// 0011001100100000
std::array<uint8_t, 2> u16_to_u8s(u32 input) {
  return {
      static_cast<uint8_t>(input & 0xFF),
      static_cast<uint8_t>((input >> 8) & 0xFF),
  };
}
std::array<uint8_t, 4> u32_to_u8s(uint32_t input) {
  return {static_cast<uint8_t>(input & 0xFF), static_cast<uint8_t>((input >> 8) & 0xFF), static_cast<uint8_t>((input >> 16) & 0xFF), static_cast<uint8_t>((input >> 24) & 0xFF)};
}

void Bus::set16(std::vector<u8>& vec, u32 address, u16 value) {
  std::array<u8, 2> v = u16_to_u8s(value);

  for (size_t i = 0; i < 2; i++) {
    vec[address + i] = v[i];
  }
}
void Bus::set32(std::vector<u8>& vec, u32 address, u32 value) {
  std::array<u8, 4> v = u32_to_u8s(value);

  for (size_t i = 0; i < 4; i++) {
    vec[address + i] = v[i];
  }
}

u8 Bus::read8(u32 address) {
  // fmt::println("[R8] {:#010x}", address);

  u32 v;
  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      return shift_16(BIOS, address);
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

    case 0x04000000 ... 0x04000050: {
      v = ppu->handle_read(address) & 0xFF;
      break;
    }

    case 0x04000051 ... 0x040003FE: {
      v = IO.at(address - 0x04000000);
      break;
    }

    case 0x06000000 ... 0x6FFFFFF: {
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
  fmt::println("[R8] {:#010x} => {:#04x}", address, v);
  return v;
};
u16 Bus::read16(u32 address) {
  // fmt::println("[R16] {:#010x}", address);
  u32 v;
  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      v = shift_16(BIOS, address);
      break;
    }

    case 0x02000000 ... 0x0203FFFF: {
      v = shift_16(EWRAM, address - 0x02000000);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      v = shift_16(IWRAM, address - 0x03000000);
      break;
    }

    case 0x04000000 ... 0x04000050: {
      v = ppu->handle_read(address);
      break;
    }

    case 0x04000051 ... 0x040003FE: {
      v = shift_16(IO, address - 0x04000000);
      break;
    }

    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      v = shift_16(PALETTE_RAM, address - 0x05000000);
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      assert(0);
      break;
    }

    case 0x08000000 ... 0x09FFFFFF: {
      // game pak read (ws0)
      v = shift_16(pak->data, address - 0x08000000);
      break;
    }

    case 0x0A000000 ... 0x0BFFFFFF: {
      // game pak read (ws1)
      v = shift_16(pak->data, address - 0x0A000000);
      break;
    }

    case 0x0C000000 ... 0x0DFFFFFF: {
      // game pak read (ws2)
      v = shift_16(pak->data, address - 0x0C000000);
      break;
    }

    default: {
      spdlog::warn("[R16] unused/oob memory read: {:#X}", address);
      return 0xFFFF;
    }
  }
  fmt::println("[R16] {:#010x} => {:#010x}", address, v);
  return v;
};
u32 Bus::read32(u32 address) {
  // SPDLOG_DEBUG("[R32] {:#010x}", address);
  u32 v;
  
  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      v = shift_32(BIOS, address);
      break;
    }

    case 0x02000000 ... 0x0203FFFF: {
      v = shift_32(EWRAM, address - 0x02000000);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      v = shift_32(IWRAM, address - 0x03000000);
      break;
    }

    case 0x04000000 ... 0x04000050: {
      v = ppu->handle_read(address);
      break;
    }

    case 0x04000051 ... 0x040003FE: {
      v = shift_32(IO, address - 0x04000000);
      break;
    }

    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      v = shift_32(PALETTE_RAM, address - 0x05000000);
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      assert(0);
      break;
    }

    case 0x08000000 ... 0x09FFFFFF: {
      // game pak read (ws0)
      v = shift_32(pak->data, address - 0x08000000);
      break;
    }

    case 0x0A000000 ... 0x0BFFFFFF: {
      // game pak read (ws1)
      v = shift_32(pak->data, address - 0x0A000000);
      break;
    }

    case 0x0C000000 ... 0x0DFFFFFF: {
      // game pak read (ws2)
      v = shift_32(pak->data, address - 0x0C000000);
      break;
    }

    default: {
      spdlog::warn("[R32] unused/oob memory read: {:#X}", address);
      return 0xFFFFFFFF; // TODO: implement open bus behavior
    }
  }
  // SPDLOG_DEBUG("[R32] {:#010x} => {:#010x}", address, v);
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

    case 0x04000000 ... 0x4000056: {
      return;
      // ppu->handle_write(address, value);
      // SPDLOG_CRITICAL("8 bit writes to PPU not allowed");
      // assert(0);
      break;
    }

    case 0x4000057 ... 0x040003FE: {
      // IO
      set32(IO, address - 0x04000000, value);
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
  // SPDLOG_INFO("[W8] {:#x} => {:#x}", address, value);
}

void Bus::write16(const u32 address, u16 value) {
  switch (address) {
    case 0x02000000 ... 0x0203FFFF: {
      set16(EWRAM, address - 0x02000000, value);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      set16(IWRAM, address - 0x03000000, value);
      break;
    }

    case 0x04000000 ... 0x4000056: {
      ppu->handle_write(address, value);
      break;
    }

    case 0x4000057 ... 0x040003FE: {
      // IO
      set16(IO, address - 0x04000000, value);
      break;
    }

    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      set16(PALETTE_RAM, address - 0x05000000, value);
      break; /*  */
    }

    case 0x06000000 ... 0x06017FFF: {
      // VRAM
      set16(VRAM, address - 0x06000000, value);
      // if(value != 0) {SPDLOG_INFO("[W16] {:#x} => {:#x}", address, value); assert(0);}
      // assert(0);

      // fmt::println("writing to VRAM");
      // assert(0);
      break;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      set16(OAM, address - 0x07000000, value);
      break;
    }

    default: {
      spdlog::warn("unused/oob memory write: {:#X}", address);
      return;
      // break;
    }
  }
  SPDLOG_INFO("[W16] {:#x} => {:#x}", address, value);
}
// TODO: replace with different typepunned arrays
void Bus::write32(const u32 address, u32 value) {
  switch (address) {
    case 0x02000000 ... 0x0203FFFF: {
      set32(EWRAM, address - 0x02000000, value);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      set32(IWRAM, address - 0x03000000, value);
      break;
    }

    case 0x04000000 ... 0x4000056: {
      // ppu->handle_write(address, value);
      SPDLOG_CRITICAL("32 bit writes to PPU not allowed");
      assert(0);
      break;
    }

    case 0x4000057 ... 0x040003FE: {
      // IO
      set32(IO, address - 0x04000000, value);
      break;
    }

    case 0x05000000 ... 0x050003FF: {
      // BG/OBJ Palette RAM
      set32(PALETTE_RAM, address - 0x05000000, value);
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      // VRAM
      set32(VRAM, address - 0x06000000, value);
      fmt::println("writing to VRAM (32-b)");
      assert(0);
      break;
    }

    case 0x07000000 ... 0X070003FF: {
      // OAM
      set32(OAM, address - 0x07000000, value);
      break;
    }

    default: {
      spdlog::warn("[W32] unused/oob memory write: {:#X}", address);
      return;
      // break;
    }
  }
  SPDLOG_INFO("[W32] {:#x} => {:#x}", address, value);
}
#include "bus.hpp"

enum class RETURN_SIZE { U16, U32 };

u16 Bus::shift_16(const std::vector<u8>& vec, const u32 address) {
  return (vec.at(address + 1) << 8) + (vec.at(address));
}
u32 Bus::shift_32(const std::vector<u8>& vec, const u32 address) {
  return ((vec.at(address + 3) << 24) + (vec.at(address + 2) << 16) +
          (vec.at(address + 1) << 8) + (vec.at(address)));
}

std::array<uint8_t, 2> u16_to_u8s(u32 input) {
  return {
      static_cast<uint8_t>(input & 0xFF),
      static_cast<uint8_t>((input >> 8) & 0xFF),
  };
}

void Bus::set16(std::vector<u8>& vec, u32 address, u16 value) {
  std::array<u8, 2> v = u16_to_u8s(value);

  for (size_t i = 0; i < 2; i++) {
    vec[address + i] = v[i];
  }
}

u8 Bus::read8(u32 address) const { return address; };
u16 Bus::read16(u32 address) const {
  // do the shit;
  return address;
};
u32 Bus::read32(u32 address) {
  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      // BIOS read
      return shift_32(BIOS, address);
    }

    case 0x02000000 ... 0x0203FFFF: {
      // internal work ram read
      return shift_32(IWRAM, address);
    }

    case 0x03000000 ... 0x03007FFF: {
      // external work ram (on pak) read
      return shift_32(EWRAM, address);
    }

    case 0x08000000 ... 0x09FFFFFF: {
      // game pak read (ws0)
      return shift_32(pak->data, address - 0x08000000);
    }

    case 0x0A000000 ... 0x0BFFFFFF: {
      // game pak read (ws1)
      return shift_32(pak->data, address - 0x0A000000);
    }

    case 0x0C000000 ... 0x0DFFFFFF: {
      // game pak read (ws2)
      return shift_32(pak->data, address - 0x0C000000);
    }

    default: {
      spdlog::warn("unused/oob memory read: {:#X}", address);
      return 0xFFFFFFFF;
    }
  }
};

void Bus::write16(const u32 address, u16 value) {
  switch (address) {
    case 0x02000000 ... 0x0203FFFF: {
      // internal work ram read
      set16(IWRAM, address - 0x02000000, value);
      break;
    }

    case 0x03000000 ... 0x03007FFF: {
      set16(EWRAM, address - 0x03000000, value);
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
      break;
    }

    case 0x06000000 ... 0x06017FFF: {
      // VRAM
      set16(VRAM, address - 0x06000000, value);
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
  SPDLOG_INFO("[W] {:#x} => {:#x}", address, value);
}
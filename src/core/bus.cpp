#include "bus.hpp"


enum class RETURN_SIZE { U16, U32 };

u16 Bus::shift_16(std::vector<u8> vec, u32 address) {
  return (vec.at(address + 1) << 8) + (vec.at(address));
}
u32 Bus::shift_32(std::vector<u8> vec, u32 address) {
  // fmt::println("v: {:#04x}", vec.at(address));
  // fmt::println("v: {:#04x}", vec.at(address + 1));
  // fmt::println("v: {:#04x}", vec.at(address + 2));
  // fmt::println("v: {:#04x}", vec.at(address + 3));

  return ((vec.at(address + 3) << 24) + (vec.at(address + 2) << 16) +
          (vec.at(address + 1) << 8) + (vec.at(address)));
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
      spdlog::warn("unused/oob memory read: {:08X}", address);
      return 0xFFFFFFFF;
    }
  }
};

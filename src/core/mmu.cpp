#include "mmu.hpp"

using fmt::println;

u8 MMU::read8(const u32 address) const {
  return 0;

  switch (address) {
    case 0x00000000 ... 0x00003FFF: {
      println("[MMU] tried to read BIOS address @ {:#32x}", address);
      break;
    }
    case 0x00004000 ... 0x01FFFFFF: {
      fmt::println("[MMU] read unused address @ {:#32x}", address);
      break;
    }
    case 0x02000000 ... 0x0203FFFF: {
      fmt::println("[MMU] read EWRAM address @ {:#32x}", address);
      break;
    }
    case 0x02040000 ... 0x02FFFFFF: {
      fmt::println("[MMU] read unused address @ {:#32x}", address);
      break;
    }
    case 0x03000000 ... 0x03007FFF: {
      fmt::println("[MMU] read IWRAM address @ {:#32x}", address);
      break;
    }
    case 0x03008000 ... 0x03FFFFFF: {
      fmt::println("[MMU] read unused address @ {:#32x}", address);
      break;
    }
    case 0x04000000 ... 0x040003FE: {
      fmt::println("[MMU] read i/o address @ {:#32x}", address);
      break;
    }
    case 0x04000400 ... 0x04FFFFFF: {
      fmt::println("[MMU] read unused address @ {:#32x}", address);
      break;
    }

      // IDM
    case 0x05000000 ... 0x050003FF: {
      fmt::println("[MMU] read BG/OBJ palette address @ {:#32x}", address);
      break;
    }
    case 0x05000400 ... 0x05FFFFFF: {
      fmt::println("[MMU] read unused address @ {:#32x}", address);
      break;
    }
    case 0x06000000 ... 0x06017FFF: {
      fmt::println("[MMU] read VRAM address @ {:#32x}", address);
      break;
    }
    case 0x06018000 ... 0x06FFFFFF: {
      fmt::println("[MMU] read unused address @ {:#32x}", address);
      break;
    }
    case 0x07000000 ... 0x070003FF: {
      fmt::println("[MMU] read OAM address @ {:#32x}", address);
      break;
    }
    case 0x07000400 ... 0x07FFFFFF: {
      fmt::println("[MMU] read unused address @ {:#32x}", address);
      break;
    }

      // Cart Mem

    case 0x08000000 ... 0x09FFFFFF: {
      // bus->cart.data.memory[0x08000000 - address];
      fmt::println("[MMU] read cart address @ {:#32x}", address);
      break;
    }
    case 0x0A000000 ... 0x0BFFFFFF: {
      fmt::println("[MMU] read cart address @ {:#32x}", address);
      break;
    }
    case 0x0C000000 ... 0x0DFFFFFF: {
      fmt::println("[MMU] read cart address @ {:#32x}", address);
      break;
    }
    case 0x0E000000 ... 0x0E00FFFF: {
      fmt::println("[MMU] read cart SRAM address @ {:#32x}", address);
      break;
    }

    default: {
      fmt::println("[MMU] read unmapped address @ {:#32x}", address);
      break;
    }
  }
};
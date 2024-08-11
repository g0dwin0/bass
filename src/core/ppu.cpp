#include "ppu.hpp"

#include "utils.h"
enum class DISP_REG : u32 {
  DISPCNT  = 0x4000000,
  DISPSTAT = 0x4000004,
  VCOUNT   = 0x4000006,
  BG0CNT   = 0x4000008,
  BG1CNT   = 0x40000A,
  BG2CNT   = 0x400000C,
  BG3CNT   = 0x400000E,
  BG0HOFS  = 0x4000010,
  BG0VOFS  = 0x4000012,
  BG1HOFS  = 0x4000014,
  BG1VOFS  = 0x4000016,
  BG2HOFS  = 0x4000018,
  BG2VOFS  = 0x400001A,
  BG3HOFS  = 0x400001C,
  BG3VOFS  = 0x400001E,
  BG2X_L   = 0x4000028,
  BG2X_H   = 0x400002A,
  BG2Y_L   = 0x400002C,
  BG2Y_H   = 0x400002E,
  BG2PA    = 0x4000020,
  BG2PB    = 0x4000022,
  BG2PC    = 0x4000024,
  BG2PD    = 0x4000026,
  BG3PA    = 0x4000030,
  BG3PB    = 0x4000032,
  BG3PC    = 0x4000034,
  BG3PD    = 0x4000036,

};

void PPU::tick() {
  // Mode 3
  // for (size_t i = 0, j = 0; i < ((240 * 160)); i++, j += 2) {
  //   frame_buffer[i] = BGR555toRGB888(bus->VRAM.at(j), bus->VRAM.at(j + 1));
  // }


  //Mode 4
  // for (size_t i = 0, j = 0; i < ((240 * 160)); i++, j += 2) {
  //   bus->PALETTE_RAM[]
  //   frame_buffer[i] = BGR555toRGB888(bus->VRAM.at(j), bus->VRAM.at(j + 1));
  // }

  // fmt::println("fb 0: {:#010x}", frame_buffer.at(0));
  // frame_buffer.at(0)
  // bus->VRAM
}

void PPU::handle_write(const u32 address, u16 value) {
  auto reg = (DISP_REG)(address);
  spdlog::set_level(spdlog::level::trace);

  switch (reg) {
    case DISP_REG::DISPCNT: {
      DISPCNT.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "DISPCNT", value);
      break;
    }
    case DISP_REG::DISPSTAT: {
      DISPSTAT.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "DISPSTAT", value);
      break;
    }
    case DISP_REG::VCOUNT: {
      VCOUNT.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "VCOUNT", value);
      break;
    }
    case DISP_REG::BG0CNT: {
      BG0CNT.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG0CNT", value);
      break;
    }
    case DISP_REG::BG1CNT: {
      BG1CNT.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG1CNT", value);
      break;
    }
    case DISP_REG::BG2CNT: {
      BG2CNT.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2CNT", value);
      break;
    }
    case DISP_REG::BG3CNT: {
      BG3CNT.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG3CNT", value);
      break;
    }
    case DISP_REG::BG0HOFS: {
      BG0HOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG0HOFS", value);
      break;
    }
    case DISP_REG::BG0VOFS: {
      BG0VOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG0VOFS", value);
      break;
    }
    case DISP_REG::BG1HOFS: {
      BG1HOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG1HOFS", value);
      break;
    }
    case DISP_REG::BG1VOFS: {
      BG1VOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG1VOFS", value);
      break;
    }
    case DISP_REG::BG2HOFS: {
      BG2HOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2HOFS", value);
      break;
    }
    case DISP_REG::BG2VOFS: {
      BG2VOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2VOFS", value);
      break;
    }
    case DISP_REG::BG3HOFS: {
      BG3HOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG3HOFS", value);
      break;
    }
    case DISP_REG::BG3VOFS: {
      BG3VOFS.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG3VOFS", value);
      break;
    }
    case DISP_REG::BG2X_L: {
      BG2X_L.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2X_L", value);
      break;
    }
    case DISP_REG::BG2X_H: {
      BG2X_H.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2X_H", value);
      break;
    }
    case DISP_REG::BG2Y_L: {
      BG2Y_L.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2Y_L", value);
      break;
    }
    case DISP_REG::BG2Y_H: {
      BG2Y_H.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2Y_H", value);
      break;
    }
    case DISP_REG::BG2PA: {
      BG2PA.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2PA", value);
      break;
    }
    case DISP_REG::BG2PB: {
      BG2PB.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2PB", value);
      break;
    }
    case DISP_REG::BG2PC: {
      BG2PC.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2PC", value);
      break;
    }
    case DISP_REG::BG2PD: {
      BG2PD.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG2PD", value);
      break;
    }
    case DISP_REG::BG3PA: {
      BG3PA.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG3PA", value);
      break;
    }
    case DISP_REG::BG3PB: {
      BG3PB.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG3PB", value);
      break;
    }
    case DISP_REG::BG3PC: {
      BG3PC.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG3PC", value);
      break;
    }
    case DISP_REG::BG3PD: {
      BG3PD.v = value;
      SPDLOG_INFO("write to {}: {:016b}", "BG3PD", value);
      break;
    }
    default: {
      SPDLOG_CRITICAL("could not cast: {:#x}", address);
      exit(-1);
    }
  }
}

u32 PPU::handle_read(const u32 address) {
  auto reg = (DISP_REG)(address);
  spdlog::set_level(spdlog::level::trace);

  switch (reg) {
    case DISP_REG::DISPCNT: { return DISPCNT.v; }
    case DISP_REG::DISPSTAT: { return DISPSTAT.v; }
    case DISP_REG::VCOUNT: { return VCOUNT.v; }
    case DISP_REG::BG0CNT: { return BG0CNT.v; }
    case DISP_REG::BG1CNT: { return BG1CNT.v; }
    case DISP_REG::BG2CNT: { return BG2CNT.v; }
    case DISP_REG::BG3CNT: { return BG3CNT.v; }
    case DISP_REG::BG0HOFS: { return BG0HOFS.v; }
    case DISP_REG::BG0VOFS: { return BG0VOFS.v; }
    case DISP_REG::BG1HOFS: { return BG1HOFS.v; }
    case DISP_REG::BG1VOFS: { return BG1VOFS.v; }
    case DISP_REG::BG2HOFS: { return BG2HOFS.v; }
    case DISP_REG::BG2VOFS: { return BG2VOFS.v; }
    case DISP_REG::BG3HOFS: { return BG3HOFS.v; }
    case DISP_REG::BG3VOFS: { return BG3VOFS.v; }
    case DISP_REG::BG2X_L: { return BG2X_L.v; }
    case DISP_REG::BG2X_H: { return BG2X_H.v; }
    case DISP_REG::BG2Y_L: { return BG2Y_L.v; }
    case DISP_REG::BG2Y_H: { return BG2Y_H.v; }
    case DISP_REG::BG2PA: { return BG2PA.v; }
    case DISP_REG::BG2PB: { return BG2PB.v; }
    case DISP_REG::BG2PC: { return BG2PC.v; }
    case DISP_REG::BG2PD: { return BG2PD.v; }
    case DISP_REG::BG3PA: { return BG3PA.v; }
    case DISP_REG::BG3PB: { return BG3PB.v; }
    case DISP_REG::BG3PC: { return BG3PC.v; }
    case DISP_REG::BG3PD: { return BG3PD.v; }
    default: { fmt::println("could not read PPU register with address: {:#x}", address); return 0;}
  }
}


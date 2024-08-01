#pragma once

#include "common.hpp"

enum class MODE { BITMAP_MODE_3 = 3, BITMAP_MODE_4 = 4, BITMAP_MODE_5 = 5 };

struct PPU {
  PPU() {
    frame.resize((240 * 160) * 2);
    spdlog::debug("PPU initialized");
  }

  void handle_write(const u32 address, u16 value);
  void handle_read();
  

  std::vector<u8> frame;

  union {
    u16 v;
    struct {
      u8 BG_MODE                 : 3;
      u8                         : 1;
      u8 DISPLAY_FRAME_SELECT    : 1;
      u8 HBLANK_INTERVAL_FREE    : 1;
      u8 OBJ_CHAR_VRAM_MAPPING   : 1;
      u8 FORCED_BLANK            : 1;
      u8 SCREEN_DISPLAY_BG0      : 1;
      u8 SCREEN_DISPLAY_BG1      : 1;
      u8 SCREEN_DISPLAY_BG2      : 1;
      u8 SCREEN_DISPLAY_BG3      : 1;
      u8 SCREEN_DISPLAY_OBJ      : 1;
      u8 WINDOW_0_DISPLAY_FLAG   : 1;
      u8 WINDOW_1_DISPLAY_FLAG   : 1;
      u8 OBJ_WINDOW_DISPLAY_FLAG : 1;
    };
  } DISPCNT;

  union {
    u16 v;
    struct {
      u8 GREEN_SWAP : 1;
      u16           : 15;
    };
  } GREEN_SWAP;

  union {
    u16 v;
    struct {
      u8 VBLANK_FLAG          : 1;
      u8 HBLANK_FLAG          : 1;
      u8 V_COUNTER_FLAG       : 1;
      u8 VBLANK_IRQ_ENABLE    : 1;
      u8 HBLANK_IRQ_ENABLE    : 1;
      u8 V_COUNTER_IRQ_ENABLE : 1;
      u8                      : 2;
      u8 VCOUNT_SETTING;
    };
  } DISPSTAT;

  union {
    u16 v;
    struct {
      u8 LY;
      u8 : 8;
    };
  } VCOUNT;

  union {
    u16 v;
    struct {
      u8 BG_PRIORITY       : 2;
      u8 CHAR_BASE_BLOCK   : 2;
      u8                   : 2;
      u8 MOSAIC            : 1;
      u8 COLORS_PALETTES   : 1;
      u8 SCREEN_BASE_BLOCK : 5;
      u8                   : 1;
      u8 SCREEN_SIZE       : 2;
    };
  } BG0CNT, BG1CNT;

  union {
    u16 v;
    struct {
      u8 BG_PRIORITY           : 2;
      u8 CHAR_BASE_BLOCK       : 2;
      u8                       : 2;
      u8 MOSAIC                : 1;
      u8 COLORS_PALETTES       : 1;
      u8 SCREEN_BASE_BLOCK     : 5;
      u8 DISPLAY_AREA_OVERFLOW : 1;
      u8 SCREEN_SIZE           : 2;
    };
  } BG2CNT, BG3CNT;

  union {
    u16 v;
    struct {
      u16 OFFSET : 9;
      u8         : 7;
    };
  } BG0HOFS, BG0VOFS, BG1HOFS, BG1VOFS, BG2HOFS, BG2VOFS, BG3HOFS, BG3VOFS;

  union {
    u32 v;
    struct {
      u8 FRACTIONAL_PORT;
      u32 INTEGER_PORT : 19;
      u8 SIGN          : 1;
      u8               : 4;
    };
  } BG2X_L, BG2X_H, BG2Y_L, BG2Y_H;

  union {
    u16 v;
    struct {
      u8 FRACTIONAL_PORT;
      u32 INTEGER_PORT : 7;
      u8 SIGN          : 1;
    };
  } BG2PA, BG2PB, BG2PC, BG2PD;

  union {
    u16 v;
    struct {
      u8 FRACTIONAL_PORT;
      u32 INTEGER_PORT : 7;
      u8 SIGN          : 1;
    };
  } BG3PA, BG3PB, BG3PC, BG3PD;
};
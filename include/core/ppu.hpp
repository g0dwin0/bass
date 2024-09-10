#pragma once

#include "bus.hpp"
#include "common.hpp"

enum BG_MODE { TILE_MAP_MODE_0 = 0, TILE_MAP_MODE_1 = 1, TILE_MAP_MODE_2 = 2, BITMAP_MODE_3 = 3, BITMAP_MODE_4 = 4, BITMAP_MODE_5 = 5 };

struct PPU {
  PPU() { std::memset(frame_buffer, 0, sizeof(frame_buffer) * 4); }

  Bus* bus          = nullptr;
  u32* frame_buffer = new u32[38400];

  [[nodiscard]] u32 get_color_by_index(const u8 x);

  void draw();
};
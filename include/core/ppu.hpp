#pragma once

#include "bus.hpp"
#include "common/defs.hpp"


enum BG_MODE { MODE_0 = 0, MODE_1 = 1, MODE_2 = 2, MODE_3 = 3, MODE_4 = 4, MODE_5 = 5 };
enum FLIP { NORMAL, MIRRORED };

union ScreenEntry {
  u16 v;
  struct {
    u16 tile_index     : 10;
    u8 HORIZONTAL_FLIP : 1;
    u8 VERTICAL_FLIP   : 1;
    u8 PAL_BANK        : 4;
  };
};

typedef std::array<u8, 64> Tile;
typedef std::array<Tile, 512> TileSet;
typedef std::array<ScreenEntry, 64 * 64> TileMap;

struct PPU {
  PPU() { std::memset(frame_buffer, 0, sizeof(frame_buffer) * 4); }

  Bus* bus              = nullptr;
  u32* frame_buffer     = new u32[38400];
  u32* tile_set_texture = new u32[38400];

  u32* tile_map_texture_buffer_0   = new u32[512 * 512];
  u32* tile_map_texture_buffer_1   = new u32[512 * 512];
  u32* tile_map_texture_buffer_2   = new u32[512 * 512];
  u32* tile_map_texture_buffer_3   = new u32[512 * 512];

  std::array<u32*, 4> tile_map_texture_buffer_arr = {tile_map_texture_buffer_0, tile_map_texture_buffer_1, tile_map_texture_buffer_2, tile_map_texture_buffer_3};
  
  u32* composite_bg_texture_buffer = new u32[512 * 512];

  std::unordered_map<u8, std::string> screen_sizes = {
      {0, "32x32"},
      {1, "64x32"},
      {2, "32x64"},
      {3, "64x64"},
  };

  std::array<TileSet, 4> tile_sets  = {};
  std::array<TileMap, 4> tile_maps = {};

  [[nodiscard]] u32 get_color_by_index(const u8 x, u8 palette_num = 0);

  void step();
  void draw(bool called_manually = false);
  u32 absolute_sbb(u8 bg, u8 map_x = 0);
  u32 relative_cbb(u8 bg);
};
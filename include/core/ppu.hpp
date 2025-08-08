#pragma once
#include "bus.hpp"
#include "common/defs.hpp"
#include "double_buffer.hpp"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"
enum BG_MODE : u8 { MODE_0 = 0, MODE_1 = 1, MODE_2 = 2, MODE_3 = 3, MODE_4 = 4, MODE_5 = 5 };
enum struct FLIP : u8 { NORMAL, MIRRORED };
enum struct OBJ_MODE : u8 { NORMAL, SEMI_TRANSPARENT, OBJ_WINDOW, PROHIBITED };
enum struct OBJ_SHAPE : u8 { SQUARE, HORIZONTAL, VERTICAL, PROHIBITED };
enum COLOR_MODE : u8 { BPP4 = 0, BPP8 = 1 };

static constexpr u32 OBJ_DATA_BASE_ADDR = 0x06010000;
static constexpr u32 OBJ_DATA_OFFSET    = 0x00010000;

// An entry in a map, representing the background.
// Imagine a 32x32 map, where each entry has the properties described below. This is makes up the LAYOUT of the background
// Not to be confused with the **tile set**, which contains the tiles, which are USED in the Screen Entry.
union ScreenBlockEntry {
  u16 v;
  struct {
    u16 tile_index     : 10;
    u8 HORIZONTAL_FLIP : 1;
    u8 VERTICAL_FLIP   : 1;
    u8 PAL_BANK        : 4;
  };
};

using PaletteIndex    = u8;
using Tile            = std::array<PaletteIndex, 8 * 8>;
using TileSet         = std::array<Tile, 512>;
using TileMap         = std::array<ScreenBlockEntry, 64 * 64>;
using TransparencyMap = std::array<bool, 512 * 512>;

struct PPU {
  static constexpr u32 VRAM_BASE            = 0x06000000;
  static constexpr u32 PALETTE_RAM_BASE     = 0x05000000;
  static constexpr u32 PALETTE_RAM_OBJ_BASE = 0x05000200;
  static constexpr u32 MAIN_VIEWPORT_PITCH  = 240;
  static constexpr u32 _4BPP                = 0;
  static constexpr u32 _8BPP                = 1;

  std::shared_ptr<spdlog::logger> ppu_logger = spdlog::stdout_color_mt("PPU");

  Bus* bus = nullptr;

  u32* disp_buf          = new u32[241 * 160];
  u32* write_buf         = new u32[241 * 160];
  bool framebuffer_ready = true;

  bool fresh_buffer = false;

  struct State {  // track changes to ppu specific registers, for example the changing of the character/screen base blocks
    std::array<bool, 4> cbb_changed = {true, true, true, true};
    bool oam_changed                = true;
    bool mapping_mode_changed       = true;

  } state;

  union OAM_Entry {
    u64 v;
    struct {
      // attr0

      u8 y;
      bool rotation_scaling_flag      : 1;
      // double size flag when rotation scaling is used, otherwise obj disable
      bool obj_disable_double_sz_flag : 1;
      OBJ_MODE obj_mode               : 2;
      bool obj_mosaic                 : 1;
      bool is_8bpp                    : 1;
      OBJ_SHAPE obj_shape             : 2;

      // attr 1

      u16 x                           : 9;
      u8                              : 3;
      FLIP horizontal_flip            : 1;
      FLIP vertical_flip              : 1;
      u8 obj_size                     : 2;

      // attr 2
      u16 char_name                   : 10;
      u8 priority_relative_to_bg      : 2;
      u8 pal_number                   : 4;

      // attr 3 (affine parameters)
      u16                             : 16;
    };
  };

  struct OBJ {
    std::array<PaletteIndex, 64 * 64> data = {};
  };

  // OAM entries map to OBJs with the same index, because the max size of an OBJ is 64x64 [square]
  // we calculate all the OBJs at OAM refill, and then simply draw them on the screen when they're queued up
  // This should make things like flipping easier

  // OAM is dirty -> memcpy to PPU
  // We have new entries, draw each single OBJ that is not prohibited/never will be drawn

  // In OBJ rendering code, check entries per scanline, if we match, draw as usual by fetching lines from the corresponding OBJ map.
  /*
  OBJs being different sizes /shouldn't/ matter, because they'll be null anyway
  */
  std::array<OAM_Entry, 128> entries;
  std::array<OBJ, 128> objs;

  u32* tile_set_texture = new u32[38400];

  u32* backdrop                  = new u32[512 * 512];
  u32* tile_map_texture_buffer_0 = new u32[512 * 512];
  u32* tile_map_texture_buffer_1 = new u32[512 * 512];
  u32* tile_map_texture_buffer_2 = new u32[512 * 512];
  u32* tile_map_texture_buffer_3 = new u32[512 * 512];
  u32* obj_texture_buffer        = new u32[256 * 256];
  u32* obj_viewer_texture_buffer = new u32[256 * 256];

  std::array<u32*, 4> tile_map_texture_buffer_arr = {tile_map_texture_buffer_0, tile_map_texture_buffer_1, tile_map_texture_buffer_2, tile_map_texture_buffer_3};

  std::array<std::array<bool, 512 * 512>, 4> transparency_maps;

  u32* composite_bg_texture_buffer = new u32[512 * 512];

  DoubleBuffer db = DoubleBuffer(disp_buf, write_buf);

  std::unordered_map<u8, std::string> screen_sizes = {
      {0, "32x32"},
      {1, "64x32"},
      {2, "32x64"},
      {3, "64x64"},
  };

  std::array<TileSet, 4> tile_sets = {};
  std::array<TileMap, 4> tile_maps = {};

  [[nodiscard]] u32 get_color_by_index(const u8 x, u8 palette_num = 0, bool color_depth_is_8bpp = false);
  [[nodiscard]] u32 get_obj_color_by_index(const u8 x, u8 palette_num = 0, bool color_depth_is_8bpp = false);

  void step(bool called_manually = false);
  void load_tiles(u8 bg, u8 color_depth);
  u32 absolute_sbb(u8 bg, u8 map_x = 0);
  u32 relative_cbb(u8 bg);

  // Returns tuple containing BGxHOFS, BGxVOFS (in that order)
  std::tuple<u16, u16> get_bg_offset(u8 bg_id);

  bool background_enabled(u8 bg_id);
  std::string get_obj_size_string(const OAM_Entry&);
  Tile get_obj_tile_by_tile_index(u16, COLOR_MODE);
  // Returns object height measured by the amount of TILES
  u8 get_obj_height(const OAM_Entry&);

  // Returns object width measured by the amount of TILES
  u8 get_obj_width(const OAM_Entry&);

  bool is_valid_obj(const OAM_Entry&);

  void repopulate_objs();
};
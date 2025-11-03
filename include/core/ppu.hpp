#pragma once
struct Bus;
#include "bus.hpp"
#include "common/defs.hpp"
#include "double_buffer.hpp"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"
enum BG_MODE : u8 { MODE_0 = 0, MODE_1 = 1, MODE_2 = 2, MODE_3 = 3, MODE_4 = 4, MODE_5 = 5 };
enum struct FLIP : u8 { NORMAL, MIRRORED };
enum struct OBJ_MODE : u8 { NORMAL, SEMI_TRANSPARENT, OBJ_WINDOW, PROHIBITED };
enum struct OBJ_SHAPE : u8 { SQUARE, HORIZONTAL, VERTICAL, PROHIBITED };

static constexpr u32 OBJ_DATA_OFFSET = 0x10000;

struct Item {
  u8 bg_id;    // primary key
  u8 bg_prio;  // first tiebreaker
};

// An entry in a map, representing the background.
// Imagine a 32x32 map, where each entry has the properties described below. This is makes up the LAYOUT of the background
// Not to be confused with the **tile set**, which contains the tiles, which are USED in the Screen Entry.
union ScreenBlockEntryMode0 {
  u16 v;
  struct {
    u16 tile_index     : 10;
    u8 HORIZONTAL_FLIP : 1;
    u8 VERTICAL_FLIP   : 1;
    u8 PAL_BANK        : 4;
  };
};
union ScreenBlockEntryMode1 {
  u8 tile_index;
};

using PaletteIndex    = u8;
using Tile            = std::array<PaletteIndex, 8 * 8>;
using TileSet         = std::array<Tile, 1024>;
using TileMap         = std::array<ScreenBlockEntryMode0, 64 * 64>;
using AffineTileMap   = std::array<ScreenBlockEntryMode1, 128 * 128>;
using TransparencyMap = std::array<bool, 512 * 512>;

struct PPU {
  PPU() : VRAM(0x18000) {
    // initialize affine frame buffers
    for (auto& arr : tile_map_affine_texture_buffer_arr) {
      arr.resize(1024 * 1024);
    }

    // initialize text frame buffers
    for (auto& arr : tile_map_texture_buffer_arr) {
      arr.resize(512 * 512);
    }

    backdrop.resize(512 * 512);
  }
  static constexpr u32 VRAM_BASE            = 0x06000000;
  static constexpr u32 PALETTE_RAM_BG_BASE  = 0x05000000;
  static constexpr u32 PALETTE_RAM_OBJ_BASE = 0x05000200;
  static constexpr u32 SYSTEM_DISPLAY_WIDTH = 240;

  std::shared_ptr<spdlog::logger> ppu_logger = spdlog::stdout_color_mt("PPU");

  Bus* bus = nullptr;

  u32* disp_buf  = new u32[241 * 160];
  u32* write_buf = new u32[241 * 160];

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
      COLOR_DEPTH color_depth         : 1;
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
    // };
  };

  struct {
    union {
      u16 v = 0;
      struct {
        u8 BG_MODE                   : 3;
        u8                           : 1;
        u8 DISPLAY_FRAME_SELECT      : 1;
        u8 HBLANK_INTERVAL_FREE      : 1;
        u8 OBJ_CHAR_VRAM_MAPPING     : 1;  // 0 = 2D, 1 = 1D
        u8 FORCED_BLANK              : 1;
        bool SCREEN_DISPLAY_BG0      : 1;
        bool SCREEN_DISPLAY_BG1      : 1;
        bool SCREEN_DISPLAY_BG2      : 1;
        bool SCREEN_DISPLAY_BG3      : 1;
        bool SCREEN_DISPLAY_OBJ      : 1;
        bool WINDOW_0_DISPLAY_FLAG   : 1;
        bool WINDOW_1_DISPLAY_FLAG   : 1;
        bool OBJ_WINDOW_DISPLAY_FLAG : 1;
      };
    } DISPCNT = {};

    union {
      u16 v = 0;
      struct {
        u8 GREEN_SWAP : 1;
        u16           : 15;
      };
    } GREEN_SWAP = {};

    union {
      u16 v = 0;
      struct {
        bool VBLANK_FLAG          : 1;
        bool HBLANK_FLAG          : 1;
        bool VCOUNT_MATCH_FLAG    : 1;
        bool VBLANK_IRQ_ENABLE    : 1;
        bool HBLANK_IRQ_ENABLE    : 1;
        bool V_COUNTER_IRQ_ENABLE : 1;
        u8 LCD_INIT_READY         : 1;
        u8 VCOUNT_MSB             : 1;
        u8 LYC;
      };

      void set_vblank() { VBLANK_FLAG = true; }
      void reset_vblank() { VBLANK_FLAG = false; }
      void set_hblank() { HBLANK_FLAG = true; }
      void reset_hblank() { HBLANK_FLAG = false; }

    } DISPSTAT = {};

    union {
      u16 v = 0;

      struct {
        u8 LY;
        u8 : 8;
      };
    } VCOUNT = {};

    union BGCNT {
      u16 v = 0;

      struct {
        u8 BG_PRIORITY          : 2;
        u8 CHAR_BASE_BLOCK      : 2;
        u8                      : 2;
        u8 MOSAIC               : 1;
        COLOR_DEPTH color_depth : 1;
        u8 SCREEN_BASE_BLOCK    : 5;
        u8 BG_WRAP              : 1;  // BG2/BG3: Display Area Overflow (0=Transparent, 1=Wraparound)
        u8 SCREEN_SIZE          : 2;
      };
    } BG0CNT, BG1CNT, BG2CNT, BG3CNT = {};

    union {
      u16 v = 0;

      struct {
        u16 OFFSET : 9;
        u8         : 7;
      };
    } BG0HOFS, BG0VOFS, BG1HOFS, BG1VOFS, BG2HOFS, BG2VOFS, BG3HOFS, BG3VOFS = {};

    union {
      u16 v = 0;

      struct {
        u8 FRACTIONAL_PORT;
        u32 INTEGER_PORT : 7;
        u8 SIGN          : 1;
      };
    } BG2PA, BG2PB, BG2PC, BG2PD, BG3PA, BG3PB, BG3PC, BG3PD = {};

    union BGREFPOINT {
      u32 v = 0;
      struct {
        u8 FRACTIONAL_PORT;
        u32 INTEGER_PORT : 19;
        u8 SIGN          : 1;
        u8               : 4;
      };
    } BG2X, BG2Y, BG3X, BG3Y = {};

    union {
      u16 v = 0;
      struct {
        u8 X2;
        u8 X1;
      };
    } WIN0H, WIN1H = {};

    union {
      u16 v = 0;
      struct {
        u8 Y2;
        u8 Y1;
      };
    } WIN0V, WIN1V = {};

    union {
      u16 v;
      struct {
        u8 WIN0_BG_ENABLE_BITS   : 4;
        u8 WIN0_OBJ_ENABLE_BIT   : 1;
        u8 WIN0_COLOR_SPECIAL_FX : 1;
        u8                       : 2;
        u8 WIN1_BG_ENABLE_BITS   : 4;
        u8 WIN1_OBJ_ENABLE_BIT   : 1;
        u8 WIN1_COLOR_SPECIAL_FX : 1;
        u8                       : 2;
      };
    } WININ = {};

    union {
      u16 v = 0;
      struct {
        u8 OUTSIDE_BG_ENABLE_BITS      : 4;
        u8 OUTSIDE_OBJ_ENABLE_BIT      : 1;
        u8 OUTSIDE_COLOR_SPECIAL_FX    : 1;
        u8                             : 2;
        u8 OBJ_WINDOW_BG_ENABLE_BITS   : 4;
        u8 OBJ_WINDOW_OBJ_ENABLE_BIT   : 1;
        u8 OBJ_WINDOW_COLOR_SPECIAL_FX : 1;
        u8                             : 2;
      };
    } WINOUT = {};

    union {
      u32 v = 0;
      struct {
        u8 BG_MOSAIC_H_SIZE  : 4;
        u8 BG_MOSAIC_V_SIZE  : 4;
        u8 OBJ_MOSAIC_H_SIZE : 4;
        u8 OBJ_MOSAIC_V_SIZE : 4;
        u16                  : 16;
      };
    } MOSAIC = {};

    u16 : 16;

    union {
      u16 v = 0;
      struct {
        u8 BG0_1ST_TARGET_PIXEL   : 1;
        u8 BG1_1ST_TARGET_PIXEL   : 1;
        u8 BG2_1ST_TARGET_PIXEL   : 1;
        u8 BG3_1ST_TARGET_PIXEL   : 1;
        u8 OBJ_1ST_TARGET_PIXEL   : 1;
        u8 BD_1ST_TARGET_PIXEL    : 1;
        COLOR_FX COLOR_SPECIAL_FX : 2;

        u8 BG0_2ND_TARGET_PIXEL   : 1;
        u8 BG1_2ND_TARGET_PIXEL   : 1;
        u8 BG2_2ND_TARGET_PIXEL   : 1;
        u8 BG3_2ND_TARGET_PIXEL   : 1;
        u8 OBJ_2ND_TARGET_PIXEL   : 1;
        u8 BD_2ND_TARGET_PIXEL    : 1;

        u8                        : 2;
      };
    } BLDCNT = {};

    union {
      u16 v = 0;
      struct {
        u8 EVA_COEFFICIENT_FIRST_TARGET  : 5;
        u8                               : 3;
        u8 EVA_COEFFICIENT_SECOND_TARGET : 5;
        u8                               : 3;
      };
    } BLDALPHA = {};

    union {
      u32 v = 0;
      struct {
        u8 EVY_COEFFICIENT : 5;
        u32                : 27;
      };
    } BLDY;

  } display_fields = {};

  struct OBJ {
    std::array<PaletteIndex, 64 * 64> data = {};
  };

  struct PixInfo {
    u32 color_id;
    u32 color;
    u8 prio;
    bool transparent = true;
    u8 bg_id;
    bool active;
    u8 oam_idx;
  };

  std::array<OAM_Entry, 128> entries;
  std::array<OBJ, 128> objs;
  std::vector<u8> VRAM;

  std::vector<u32> backdrop;

  std::array<std::vector<u32>, 4> tile_map_texture_buffer_arr;
  std::array<std::vector<u32>, 4> tile_map_affine_texture_buffer_arr;

  std::array<std::array<bool, 512 * 512>, 4> transparency_maps;
  std::array<PixInfo, 512 * 512> background_layer;
  std::array<PixInfo, 256 * 256> sprite_layer;

  std::array<TileSet, 4> tile_sets              = {};
  std::array<TileMap, 4> tile_maps              = {};
  std::array<AffineTileMap, 4> affine_tile_maps = {};

  u32 latched_bg2x = 0;
  u32 latched_bg2y = 0;
  u32 latched_bg3x = 0;
  u32 latched_bg3y = 0;

  void reset_sprite_layer();
  u32* composite_bg_texture_buffer = new u32[512 * 512];

  DoubleBuffer db = DoubleBuffer(write_buf, disp_buf);

  std::unordered_map<u8, std::string> screen_sizes_str_map = {
      {0, "32x32"},
      {1, "64x32"},
      {2, "32x64"},
      {3, "64x64"},
  };

  [[nodiscard]] u32 get_color_by_index(u8 x, u8 palette_num, COLOR_DEPTH color_depth) const;
  [[nodiscard]] u32 get_obj_color_by_index(u8 x, u8 palette_num, COLOR_DEPTH color_depth) const;

  void step();
  void load_tiles(u8 bg, COLOR_DEPTH color_depth);

  void draw_mode_0_scanline();

  std::tuple<u16, u16> get_affine_bg_size(u8 bg_id);

  bool background_enabled(u8 bg_id);

  u8 get_bg_prio(u8) const;

  // void populate_tile_map(const std::vector<u8>& bg_ids);

  // ======= sprite =======
  std::string get_obj_size_string(const OAM_Entry&);

  Tile get_obj_tile_by_tile_index(u16, COLOR_DEPTH);

  // returns object height measured by the amount of tiles
  u8 get_obj_height(const OAM_Entry&);

  // returns object width measured by the amount of tiles
  u8 get_obj_width(const OAM_Entry&);

  bool is_valid_obj(const OAM_Entry&);

  // re-renders the sprite table based on the current OAM & mapping mode
  void repopulate_objs();

  // ======= text mode =======

  u32 absolute_sbb(u8 bg, u8 map_x = 0);
  u32 relative_cbb(u8 bg);

  // Returns tuple containing BGxHOFS, BGxVOFS (in that order)
  std::tuple<u16, u16> get_text_bg_offset(u8 bg_id) const;
  std::tuple<u16, u16> get_render_offset(u8 screen_size);

  // affine related stuff
  std::tuple<u8, u8> get_affine_bg_tile_sizes(u8 bg_id);

  // blending stuff
  const std::unordered_map<COLOR_FX, std::string> special_fx_str_map = {
      {               COLOR_FX::NONE,                "NONE"},
      {     COLOR_FX::ALPHA_BLENDING,      "Alpha Blending"},
      {COLOR_FX::BRIGHTNESS_INCREASE, "Brightness Increase"},
      {COLOR_FX::BRIGHTNESS_DECREASE, "Brightness Decrease"},
  };
};
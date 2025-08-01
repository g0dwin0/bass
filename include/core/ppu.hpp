#pragma once
#include "bus.hpp"
#include "common/defs.hpp"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"
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

using Tile            = std::array<u8, 8 * 8>;
using TileSet         = std::array<Tile, 512>;
using TileMap         = std::array<ScreenEntry, 64 * 64>;
using TransparencyMap = std::array<bool, 512 * 512>;

struct PPU {
  static constexpr u32 VRAM_BASE           = 0x06000000;
  static constexpr u32 PALETTE_RAM_BASE    = 0x05000000;
  static constexpr u32 MAIN_VIEWPORT_PITCH = 240;
  static constexpr u32 _4BPP               = 0;
  static constexpr u32 _8BPP               = 1;

  std::shared_ptr<spdlog::logger> ppu_logger = spdlog::stdout_color_mt("PPU");

  Bus* bus = nullptr;

  u32* disp_buf          = new u32[241 * 160];
  u32* write_buf         = new u32[241 * 160];
  bool framebuffer_ready = true;

  bool fresh_buffer = false;

  struct State {  // track changes to ppu specific registers, for example the changing of the character/screen base blocks
    std::array<bool, 4> cbb_changed = {true, true, true, true};
  } state;

  struct DoubleBuffer {
    u32* write_buf = nullptr;
    u32* disp_buf  = nullptr;

    bool framebuffer_ready = false;
    std::mutex framebuffer_mutex;
    std::condition_variable framebuffer_cv;

   public:
    DoubleBuffer(u32* _write_buf, u32* _disp_buf) : write_buf(_write_buf), disp_buf(_disp_buf) {}

    void write(size_t idx, u32 value) {
      write_buf[idx] = value;

      if (idx == (241 * 160) - 1) {
        request_swap();
      }
    }

    void request_swap() {
      std::lock_guard<std::mutex> lock(framebuffer_mutex);
      framebuffer_ready = true;
      framebuffer_cv.notify_one();
    };

    void swap_buffers() { std::swap(write_buf, disp_buf); };
  };

  u32* tile_set_texture = new u32[38400];

  u32* backdrop                  = new u32[512 * 512];
  u32* tile_map_texture_buffer_0 = new u32[512 * 512];
  u32* tile_map_texture_buffer_1 = new u32[512 * 512];
  u32* tile_map_texture_buffer_2 = new u32[512 * 512];
  u32* tile_map_texture_buffer_3 = new u32[512 * 512];

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

  [[nodiscard]] u32 get_color_by_index(const u8 x, u8 palette_num = 0, bool offset_colors = false);

  void step(bool called_manually = false);
  void load_tiles(u8 bg, u8 color_depth);
  u32 absolute_sbb(u8 bg, u8 map_x = 0);
  u32 relative_cbb(u8 bg);

  // Returns tuple containing BGxHOFS, BGxVOFS (in that order)
  std::tuple<u16, u16> get_bg_offset(u8 bg_id);

  bool background_enabled(u8 bg_id);
};
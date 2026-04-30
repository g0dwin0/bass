#include "ppu.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>
#include <utility>

#include "bus.hpp"
#include "common/color_conversion.hpp"
#include "common/defs.hpp"

bool PPU::is_valid_obj(const OAM_Entry& oam_entry) {
  if (oam_entry.obj_mode == OBJ_MODE::PROHIBITED) return false;
  if (oam_entry.obj_shape == OBJ_SHAPE::PROHIBITED) return false;
  if (oam_entry.obj_disable_double_sz_flag) return false;  // TODO: when in mode 2, this is used as the double size flag, and thus this is not valid anymore

  return true;
}

void PPU::repopulate_objs() {
  u8 index = 0;
  for (const auto& entry : entries) {
    if (!is_valid_obj(entry)) {
      index++;
      continue;
    }

    Tile current_tile;

    const u16 obj_height = get_obj_height(entry);  // height of OBJ in tiles
    const u16 obj_width  = get_obj_width(entry);   // width of OBJ in tiles

    u32 current_charname = 0;  // Tile ID

    for (size_t tile_y = 0; tile_y < obj_height; tile_y++) {
      for (size_t tile_x = 0; tile_x < obj_width; tile_x++) {
        if (display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING == Bus::ONE_DIMENSIONAL) {
          current_charname = entry.char_name + (tile_x + (tile_y * obj_width));
          if (entry.color_depth == COLOR_DEPTH::BPP8) current_charname = entry.char_name + (tile_x + (tile_y * obj_width)) * 2;

        } else {
          current_charname = (entry.char_name + tile_x + (tile_y * 32)) % 1024;
        }

        current_tile = get_obj_tile_by_tile_index(current_charname, entry.color_depth);

        for (size_t y = 0; y < 8; y++) {
          for (size_t x = 0; x < 8; x++) {
            objs[index].data.at((tile_x * 8) + (tile_y * (64 * 8)) + ((y * 64) + x)) = current_tile.at((y * 8) + x);
          }
        }
      }
    }

    if (entry.horizontal_flip == FLIP::MIRRORED) {
      for (u8 y = 0; y < get_obj_height(entry) * 8; y++) {
        std::reverse(std::begin(objs[index].data) + (y * 64), std::begin(objs[index].data) + (y * 64) + get_obj_width(entry) * 8);
      }
    }

    if (entry.vertical_flip == FLIP::MIRRORED) {
      std::array<u8, 64 * 64> tmp = {};
      for (u8 y = 0; y > get_obj_height(entry) * 8; y++) {
        for (u8 x = 0; x < 64; x++) {
          std::copy(std::begin(objs[index].data) + (y * 64), std::begin(objs[index].data) + (y * 64) + 64, std::begin(tmp) + ((64 - y) * 64));
        }
      }
      objs[index].data = tmp;
    }

    index++;
  }
}

u8 PPU::get_obj_height(const OAM_Entry& c) {
  switch (c.obj_shape) {
    case OBJ_SHAPE::SQUARE: {
      switch (c.obj_size) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        default: {
          assert(0 && "bad obj size");
        }
      }
    }
    case OBJ_SHAPE::HORIZONTAL: {
      switch (c.obj_size) {
        case 0: return 1;
        case 1: return 1;
        case 2: return 2;
        case 3: return 4;
        default: {
          assert(0 && "bad obj size");
        }
      }
    }
    case OBJ_SHAPE::VERTICAL: {
      switch (c.obj_size) {
        case 0: return 2;
        case 1: return 4;
        case 2: return 4;
        case 3: return 8;
        default: {
          assert(0 && "bad obj size");
        }
      }
    }
    case OBJ_SHAPE::PROHIBITED: assert(0);
    default: {
      assert(0 && "bad obj shape");
    }
  };

  assert(0);
  std::unreachable();
};

u8 PPU::get_obj_width(const OAM_Entry& c) {
  switch (c.obj_shape) {
    case OBJ_SHAPE::SQUARE: {
      switch (c.obj_size) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        default: {
          fmt::println("bad obj width");
          exit(-1);
        }
      }
    };
    case OBJ_SHAPE::HORIZONTAL: {
      switch (c.obj_size) {
        case 0: return 2;
        case 1: return 4;
        case 2: return 4;
        case 3: return 8;
        default: {
          fmt::println("bad obj width");
          exit(-1);
        }
      }
    }
    case OBJ_SHAPE::VERTICAL: {
      switch (c.obj_size) {
        case 0: return 1;
        case 1: return 1;
        case 2: return 2;
        case 3: return 4;
        default: {
          fmt::println("bad obj width");
          exit(-1);
        }
      }
    }
    case OBJ_SHAPE::PROHIBITED: {
      assert(0 && "prohibited sprite");
    }
      // default: {assert(0); return -1;}
  };

  return -1;
};

u32 PPU::get_color_by_index(const u8 x, const u8 palette_num, const COLOR_DEPTH color_depth) {
  u16 color_index;

  if (color_depth == COLOR_DEPTH::BPP8) {
    color_index = read_palette_ram<u16>(PALETTE_RAM_BG_BASE + (x * 2));

  } else {
    color_index = read_palette_ram<u16>(PALETTE_RAM_BG_BASE + (x * 2) + (0x20 * palette_num));
  }

  return BGR555_TO_RGB888_LUT[color_index];
}

u32 PPU::get_obj_color_by_index(const u8 palette_index, const u8 bank_number, const COLOR_DEPTH color_depth) {
  u16 color_index = 0;

  if (color_depth == COLOR_DEPTH::BPP8) {
    // color_index = bus->read16(PALETTE_RAM_OBJ_BASE + (palette_index * 2));
    color_index = read_palette_ram<u16>(PALETTE_RAM_OBJ_BASE + (palette_index * 2));

  } else {
    // assert(palette_index <= 15);
    // color_index = bus->read16(PALETTE_RAM_OBJ_BASE + (palette_index * 2) + (BANK_SIZE * bank_number));
    color_index = read_palette_ram<u16>(PALETTE_RAM_OBJ_BASE + (palette_index * 2) + (BANK_SIZE * bank_number));
  }

  return BGR555_TO_RGB888_LUT[color_index];
}

std::tuple<u16, u16> PPU::get_text_bg_offset(u8 bg_id) const {
  switch (bg_id) {
    case 0: return {+display_fields.BG0HOFS.OFFSET, +display_fields.BG0VOFS.OFFSET};
    case 1: return {+display_fields.BG1HOFS.OFFSET, +display_fields.BG1VOFS.OFFSET};
    case 2: return {+display_fields.BG2HOFS.OFFSET, +display_fields.BG2VOFS.OFFSET};
    case 3: return {+display_fields.BG3HOFS.OFFSET, +display_fields.BG3VOFS.OFFSET};
    default: assert(0);
  }
}

// WIDTH (X) - HEIGHT (Y)
std::tuple<u16, u16> PPU::get_render_offset(u8 screen_size) {
  switch (screen_size) {
    case 0: return {256, 256};
    case 1: return {512, 256};
    case 2: return {256, 512};
    case 3: return {512, 512};
    default: assert(0);
  }
}
std::tuple<u8, u8> PPU::get_affine_bg_tile_sizes(u8 bg_id) {
  switch (bg_id) {
    case 0: return {16, 16};
    case 1: return {32, 32};
    case 2: return {64, 64};
    case 3: return {128, 128};
    default: assert(0);
  }
}

void PPU::load_tiles(u8 bg, const COLOR_DEPTH color_depth) {
  // ppu_logger->debug("loading tiles");
  // assert(color_depth < 2);
  u8 x = 0;
  Tile tile;
  u32 rel_cbb = relative_cbb(bg);

  if (color_depth == COLOR_DEPTH::BPP4) {
    for (size_t tile_index = 0; tile_index < 1024; tile_index++) {
      for (size_t byte = 0; byte < 32; byte++) {
        auto indexed_tile = rel_cbb + (tile_index * 0x20) + byte;

        tile[((byte * 2) + x)]       = VRAM[indexed_tile] & 0x0F;
        tile[((byte * 2) + (x + 1))] = (VRAM[indexed_tile] & 0xF0) >> 4;
      }

      tile_sets[bg][tile_index] = tile;
    }
  } else {
    for (size_t tile_index = 0; tile_index < 1024; tile_index++) {
      for (size_t byte = 0; byte < 64; byte++) {
        tile[byte] = VRAM[rel_cbb + (tile_index * 0x40) + byte];
      }

      tile_sets[bg][tile_index] = tile;
    }
  }
}

Tile PPU::get_obj_tile_by_tile_index(const u16 tile_id, const COLOR_DEPTH color_mode) {
  // u8 x = 0;
  // TODO: we construct a new tile each time, we know that obj tiles are fixed. there are FIXED amount of tiles; this is a waste, because we also need to do a copy at the end, just use an existing
  // array of tiles instead of having to allocate, and then copy each time
  Tile tile;

  switch (color_mode) {
    case COLOR_DEPTH::BPP4: {
      for (size_t byte = 0; byte < 0x20; byte++) {
        tile[(byte * 2)]     = VRAM[OBJ_DATA_OFFSET + (tile_id * 0x20) + byte] & 0x0F;         // X = 0
        tile[(byte * 2) + 1] = (VRAM[OBJ_DATA_OFFSET + (tile_id * 0x20) + byte] & 0xF0) >> 4;  // X = 1
      }
      break;
    }
    case COLOR_DEPTH::BPP8: {
      for (size_t byte = 0; byte < 0x40; byte++) {
        tile[byte] = VRAM[OBJ_DATA_OFFSET + (tile_id * 0x20) + byte];
      }
      break;
    }
  }

  return tile;
}

inline u32 PPU::absolute_sbb(u8 bg, u8 map_x) const {
  switch (bg) {
    case 0: return VRAM_BASE + ((display_fields.BG0CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 1: return VRAM_BASE + ((display_fields.BG1CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 2: return VRAM_BASE + ((display_fields.BG2CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 3: return VRAM_BASE + ((display_fields.BG3CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    default: assert(0);
  }
}
inline u32 PPU::relative_cbb(const u8 bg_id) const {
  switch (bg_id) {
    case 0: return display_fields.BG0CNT.CHAR_BASE_BLOCK * 0X4000;
    case 1: return display_fields.BG1CNT.CHAR_BASE_BLOCK * 0X4000;
    case 2: return display_fields.BG2CNT.CHAR_BASE_BLOCK * 0X4000;
    case 3: return display_fields.BG3CNT.CHAR_BASE_BLOCK * 0X4000;
    default: assert(0);
  }
}
bool PPU::background_enabled(const u8 bg_id) const {
  switch (bg_id) {
    case 0: return display_fields.DISPCNT.SCREEN_DISPLAY_BG0;
    case 1: return display_fields.DISPCNT.SCREEN_DISPLAY_BG1;
    case 2: return display_fields.DISPCNT.SCREEN_DISPLAY_BG2;
    case 3: return display_fields.DISPCNT.SCREEN_DISPLAY_BG3;
    default: assert(0);
  }
}
void PPU::draw_backdrop_on_ly(const u8& LY) {
  for (size_t x = 0; x < 240; x++) {
    backdrop[(LY * SYSTEM_DISPLAY_WIDTH) + x] = get_color_by_index(0, 0, COLOR_DEPTH::BPP4);
    auto& bg_pixel                            = background_layer.at((LY * SYSTEM_DISPLAY_WIDTH) + x);

    bg_pixel.prio  = 5;
    bg_pixel.color = get_color_by_index(0, 0, COLOR_DEPTH::BPP4);
  }
}
std::vector<BG_INFO> PPU::get_active_bg_by_prios() const {
  std::vector<BG_INFO> active_bgs = {};

  // determine priority
  for (int bg = 3; bg >= 0; bg--) {
    if (!background_enabled(bg)) {
      continue;
    }
    active_bgs.emplace_back(static_cast<u8>(bg), get_bg_prio(bg));
  }

  std::ranges::stable_sort(active_bgs, [](const BG_INFO& a, const BG_INFO& b) {
    if (a.bg_prio != b.bg_prio) return a.bg_prio > b.bg_prio;  // primary
    return a.bg_id > b.bg_id;                                  // tiebreaker
  });
  return active_bgs;
}
void PPU::vertical_flip(Tile& tile) {
  Tile tb;

  for (size_t row = 0; row < 8; row++) {
    for (size_t pix_n = 0; pix_n < 8; pix_n++) {
      tb[(row * 8) + pix_n] = tile[((7 - row) * 8) + pix_n];
    }
  }

  tile = tb;
}
void PPU::horizontal_flip(Tile& tile) {
  for (size_t row = 0; row < 8; row++) {
    std::reverse(tile.begin() + (row * 8), (tile.begin() + 8 + (row * 8)));
  }
}
void PPU::step() {
  switch (display_fields.DISPCNT.BG_MODE) {
    case MODE_0: {
      const auto& LY = display_fields.VCOUNT.LY;

      // TODO: do we really need this on the stack each single time?
      std::array<u8, 4> screen_sizes = {
          display_fields.BG0CNT.SCREEN_SIZE,
          display_fields.BG1CNT.SCREEN_SIZE,
          display_fields.BG2CNT.SCREEN_SIZE,
          display_fields.BG3CNT.SCREEN_SIZE,
      };
      std::array<COLOR_DEPTH, 4> bg_bpp = {
          display_fields.BG0CNT.color_depth,
          display_fields.BG1CNT.color_depth,
          display_fields.BG2CNT.color_depth,
          display_fields.BG3CNT.color_depth,
      };

      // process bgs
      for (u8 bg = 0; bg < 4; bg++) {
        load_tiles(bg, bg_bpp[bg]);  // only gets called when dispstat corresponding to bg changes
        if (!background_enabled(bg)) continue;

        auto [x_offset, y_offset] = get_text_bg_offset(bg);

        // load tiles into tilesets from charblocks

        // Load screenblocks to our background tile map
        switch (screen_sizes[bg]) {
          case 0: {
            // [0]
            u8 map_x = 0;
            // let's calculate tile y beforehand...
            u8 tile_y = ((LY + y_offset) % 256) / 8;

            // TODO: probably shouldn't re-populate screenblock entry map every single scanline -- expensive
            for (u32 tile_x = 0; tile_x < 32; tile_x++) {
              u32 dest = (tile_y * 64) + tile_x;

              tile_maps[bg][dest] = ScreenBlockEntryMode0{
                  .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),  // should be able to directly access vram, bus adds alot of overhead
              };
            }
            break;
          }
          case 1: {
            // [0][1]

            for (u32 map_x = 0; map_x < 2; map_x++) {
              for (u32 tile_y = 0; tile_y < 32; tile_y++) {
                for (u32 tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * 32);

                  tile_maps[bg][dest] = ScreenBlockEntryMode0{
                      .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                  };
                }
              }
            };
            break;
          }
          case 2: {
            // [0]
            // [1]
            for (u32 map_x = 0; map_x < 2; map_x++) {
              for (u32 tile_y = 0; tile_y < 32; tile_y++) {
                for (u32 tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * (32 * 64));

                  tile_maps[bg][dest] = ScreenBlockEntryMode0{
                      .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                  };
                }
              }
            };
            break;
          }
          case 3: {
            // [0][1]
            // [2][3]
            for (u32 map_x = 0; map_x < 4; map_x++) {
              for (u32 tile_y = 0; tile_y < 32; tile_y++) {
                for (u32 tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * 32);

                  if (map_x == 2) {
                    dest = (tile_y * 64) + tile_x + (64 * 32);
                  }
                  if (map_x == 3) {
                    dest = (tile_y * 64) + tile_x + (64 * 32) + (32);
                  }

                  tile_maps[bg][dest] = ScreenBlockEntryMode0{
                      .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                  };
                }
              }
            }
            // assert(0);
            break;
          }
          default: assert(0);
        }

        auto [x_render_offset, y_render_offset] = get_render_offset(screen_sizes[bg]);

        // write scanlines to the buffers
        u8 tile_y = ((LY + y_offset) % y_render_offset) / 8;
        u8 y      = ((LY + y_offset) % y_render_offset) % 8;

        for (size_t tile_x = 0; tile_x < 64; tile_x++) {
          const ScreenBlockEntryMode0& entry = tile_maps[bg][(tile_y * 64) + tile_x];
          Tile tile                          = tile_sets[bg][entry.tile_index];

          // OBJ are now flipped on a individual level, but the order of OBJ is what needs to be flipped, (including the OBJ being flipped)
          if (entry.VERTICAL_FLIP) vertical_flip(tile);
          if (entry.HORIZONTAL_FLIP) horizontal_flip(tile);

          // setting up tile map
          for (size_t x = 0; x < 8; x++) {
            auto clr = get_color_by_index(tile[(y * 8) + x], entry.PAL_BANK, bg_bpp[bg]);

            // Palette Index = 0 -- if so save entry in the transparency map. Used during composition
            transparency_maps[bg][(tile_y * (SCREEN_WIDTH * 8) + (y * SCREEN_WIDTH) + (tile_x * 8) + x)] = tile[(y * 8) + x] == 0;

            tile_map_texture_buffer_arr[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = clr;
          }
        }

        // if (LY > 159) continue;
        //  In case that some or all BGs are set to same priority then BG0 is having the highest, and BG3 the lowest priority.
      }

      if (LY > 159) return;

      const bool draw_sprites = display_fields.DISPCNT.SCREEN_DISPLAY_OBJ;

      // ==================================== composition ====================================

      std::vector<BG_INFO> active_bgs = get_active_bg_by_prios();

      draw_backdrop_on_ly(LY);

      for (const auto& [bg_id, bg_prio] : active_bgs) {
        auto [x_offset, y_offset]               = get_text_bg_offset(bg_id);
        auto [x_render_offset, y_render_offset] = get_render_offset(screen_sizes[bg_id]);
        for (size_t x = 0; x < 240; x++) {
          u32 complete_x_offset = (x + x_offset) % x_render_offset;
          u32 complete_y_offset = ((LY + y_offset) % y_render_offset) * 512;

          if (transparency_maps[bg_id][(complete_x_offset + complete_y_offset)]) {
            continue;
          }

          background_layer[(LY * SYSTEM_DISPLAY_WIDTH) + x].color       = tile_map_texture_buffer_arr[bg_id][(complete_x_offset + complete_y_offset)];
          background_layer[(LY * SYSTEM_DISPLAY_WIDTH) + x].prio        = bg_prio;
          background_layer[(LY * SYSTEM_DISPLAY_WIDTH) + x].bg_id       = bg_id;
          background_layer[(LY * SYSTEM_DISPLAY_WIDTH) + x].transparent = transparency_maps[bg_id][(complete_x_offset + complete_y_offset)];
        }
      }

      if (!draw_sprites) {
        for (size_t x = 0; x < 240; x++) {
          const auto bg_px = background_layer[(LY * SYSTEM_DISPLAY_WIDTH) + x];
          db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, bg_px.color);
        }
        break;
      }

      // process sprites
      std::memcpy(entries.data(), bus->OAM.data(), 0x400);  // TODO: expensive! find another way
      repopulate_objs();                                    // TODO: a write to change 1 entry will lead to us re-populating the entire table -- re-populate by index

      for (size_t entry_idx = 0; entry_idx < 128; entry_idx++) {
        const OAM_Entry& oam_entry = entries.at(entry_idx);

        if (oam_entry.obj_disable_double_sz_flag) continue;                                                      // obj is disabled
        if (oam_entry.obj_mode == OBJ_MODE::PROHIBITED || oam_entry.obj_mode == OBJ_MODE::OBJ_WINDOW) continue;  // obj has a prohibited mode
        if (oam_entry.obj_shape == OBJ_SHAPE::PROHIBITED) continue;                                              // obj is prohibited shape

        u16 y_relative_to_top_of_obj = (((i16)LY - (i16)oam_entry.y) + 256) % 256;
        if (y_relative_to_top_of_obj >= (get_obj_height(oam_entry) * 8)) continue;

        if (((oam_entry.y + (get_obj_height(oam_entry) * 8)) % 256) <= LY) continue;  // we've passed the last scanline

        // fmt::println("index: {} type: {}", entry_idx, (u8)oam_entry.obj_mode);

        auto obj_width = get_obj_width(oam_entry);
        // auto obj_height = get_obj_height(oam_entry);

        for (size_t tile_x = 0; tile_x < obj_width; tile_x++) {
          for (u8 pixel_x = 0; pixel_x < 8; pixel_x++) {
            const auto& palette_index_of_pixel = objs[entry_idx].data.at(((y_relative_to_top_of_obj) * 64) + (tile_x * 8) + pixel_x);

            auto clr = get_obj_color_by_index(palette_index_of_pixel, oam_entry.pal_number, oam_entry.color_depth);

            u32 line_height = (oam_entry.y + y_relative_to_top_of_obj) % 256;
            line_height *= 256;
            u32 f_x = (oam_entry.x + (tile_x * 8) + pixel_x) % 512;

            // if (line_height + f_x >= 65536) continue;
            if (f_x >= 240) continue;

            auto& obj_px = sprite_layer.at(line_height + f_x);

            if (obj_px.prio < oam_entry.priority_relative_to_bg) continue;
            if (obj_px.oam_idx < entry_idx) continue;

            if (palette_index_of_pixel == 0) continue;

            obj_px.color       = clr;
            obj_px.prio        = oam_entry.priority_relative_to_bg;
            obj_px.transparent = palette_index_of_pixel == 0;
            obj_px.oam_idx     = entry_idx;

            // TODO: re-implement writing to obj texture buffer (for obj window screen in debugger)
            // obj_texture_buffer[line_height + f_x] = clr;
            // db.write((LY * MAIN_VIEWPORT_PITCH) + f_x, clr);
          }
        }
      }

      for (size_t x = 0; x < 240; x++) {
        // BG ID = 0 -- OBJ PRIO = 1 // BG GETS DRAWN OVER OBJ
        const auto& bg_px  = background_layer.at((LY * SYSTEM_DISPLAY_WIDTH) + x);
        const auto& obj_px = sprite_layer.at(LY * 256 + x);

        // lower number wins
        if (obj_px.prio > bg_px.prio) {  // draw bg
          db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, bg_px.color);

          if (bg_px.transparent) {
            db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, sprite_layer.at(LY * 256 + x).color);
          }

        } else {
          if (!sprite_layer.at(LY * 256 + x).transparent) {
            db.write(LY * SYSTEM_DISPLAY_WIDTH + x, sprite_layer.at(LY * 256 + x).color);

          } else {
            db.write(LY * SYSTEM_DISPLAY_WIDTH + x, bg_px.color);
          }
        }
      }

      break;
    }

    case MODE_1: {
      const auto& LY = display_fields.VCOUNT.LY;

      // TODO: do we really need this on the stack each single time?
      std::array screen_sizes = {
          display_fields.BG0CNT.SCREEN_SIZE,
          display_fields.BG1CNT.SCREEN_SIZE,
          display_fields.BG2CNT.SCREEN_SIZE,
          display_fields.BG3CNT.SCREEN_SIZE,
      };
      std::array bg_bpp = {
          display_fields.BG0CNT.color_depth,
          display_fields.BG1CNT.color_depth,
          display_fields.BG2CNT.color_depth,
          display_fields.BG3CNT.color_depth,
      };

      // process non affine bgs
      for (u8 bg = 0; bg < 2; bg++) {
        // fmt::println("BG: {}", bg);
        // if (!background_enabled(bg)) continue;

        auto [x_offset, y_offset] = get_text_bg_offset(bg);

        // load tiles into tilesets from charblocks
        load_tiles(bg, bg_bpp[bg]);  // only gets called when dispstat corresponding to bg changes

        // Load screenblocks to our background tile map
        switch (screen_sizes[bg]) {
          case 0: {
            // [0]
            u8 map_x = 0;
            // let's calculate tile y beforehand...
            u8 tile_y = ((LY + y_offset) % 256) / 8;

            // TODO: probably shouldn't re-populate screenblock entry map every single scanline -- expensive
            // for (size_t tile_y = 0; tile_y < 32; tile_y++) {
            for (u32 tile_x = 0; tile_x < 32; tile_x++) {
              u32 dest = (tile_y * 64) + tile_x;

              tile_maps[bg][dest] = ScreenBlockEntryMode0{
                  // .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),  // should be able to directly access vram, bus adds alot of overhead
                  .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),  // should be able to directly access vram, bus adds alot of overhead

              };
            }
            break;
          }
          case 1: {
            // [0][1]

            for (u32 map_x = 0; map_x < 2; map_x++) {
              for (u32 tile_y = 0; tile_y < 32; tile_y++) {
                for (u32 tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * 32);

                  tile_maps[bg][dest] = ScreenBlockEntryMode0{
                      // .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                      .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),

                  };
                }
              }
            };
            break;
          }
          case 2: {
            // [0]
            // [1]
            for (u32 map_x = 0; map_x < 2; map_x++) {
              for (u32 tile_y = 0; tile_y < 32; tile_y++) {
                for (u32 tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * (32 * 64));

                  tile_maps[bg][dest] = ScreenBlockEntryMode0{
                      // .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                      .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),

                  };
                }
              }
            };
            break;
          }
          case 3: {
            // [0][1]
            // [2][3]
            for (u32 map_x = 0; map_x < 4; map_x++) {
              for (u32 tile_y = 0; tile_y < 32; tile_y++) {
                for (u32 tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * 32);

                  if (map_x == 2) {
                    dest = (tile_y * 64) + tile_x + (64 * 32);
                  }
                  if (map_x == 3) {
                    dest = (tile_y * 64) + tile_x + (64 * 32) + (32);
                  }

                  tile_maps[bg][dest] = ScreenBlockEntryMode0{
                      // .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                      .v = read_vram<u16>(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),

                  };
                }
              }
            };
            // assert(0);
            break;
          }
        }

        auto [x_render_offset, y_render_offset] = get_render_offset(screen_sizes[bg]);

        // write scanlines to the buffers
        u8 tile_y = ((LY + y_offset) % y_render_offset) / 8;
        u8 y      = ((LY + y_offset) % y_render_offset) % 8;

        for (size_t tile_x = 0; tile_x < 64; tile_x++) {
          const ScreenBlockEntryMode0& entry = tile_maps[bg][(tile_y * 64) + tile_x];
          Tile tile                          = tile_sets[bg][entry.tile_index];

          // OBJ are now flipped on a individual level, but the order of OBJ is what needs to be flipped, (including the OBJ being flipped)

          if (entry.VERTICAL_FLIP) {
            Tile tb;

            for (size_t row = 0; row < 8; row++) {  // TODO: move into func
              for (size_t pix_n = 0; pix_n < 8; pix_n++) {
                tb[(row * 8) + pix_n] = tile[((7 - row) * 8) + pix_n];
              }
            }

            tile = tb;
          }

          if (entry.HORIZONTAL_FLIP) {
            for (size_t row = 0; row < 8; row++) {  // TODO: move into func
              std::reverse(tile.begin() + (row * 8), (tile.begin() + 8 + (row * 8)));
            }
          }

          for (size_t x = 0; x < 8; x++) {
            auto clr = get_color_by_index(tile[(y * 8) + x], entry.PAL_BANK, bg_bpp[bg]);

            // Palette Index = 0 -- if so save entry in the transparency map. Used during composition
            transparency_maps[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = ((tile[(y * 8) + x] == 0));

            tile_map_texture_buffer_arr[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = clr;
          }
        }

        // if (LY > 159) continue;
      }

      // process affine bg
      if (background_enabled(2)) {
        u8 tile_y = LY % 256 / 8;
        // fmt::println("HI");
        load_tiles(2, COLOR_DEPTH::BPP8);  // only gets called when dispstat corresponding to bg changes

        // TODO: probably shouldn't re-populate screenblock entry map every single scanline -- expensive
        // for (size_t tile_y = 0; tile_y < 32; tile_y++) {

        for (u32 tile_x = 0; tile_x < 32; tile_x++) {
          u32 dest = (tile_y * 64) + tile_x;

          affine_tile_maps[2][dest] = ScreenBlockEntryMode1{
              // .tile_index = bus->read8(absolute_sbb(2, 0) + (tile_x + tile_y * 32)),  // should be able to directly access vram, bus adds alot of overhead
              .tile_index = read_vram<u8>(absolute_sbb(2, 0) + (tile_x + tile_y * 32)),  // should be able to directly access vram, bus adds alot of overhead

          };
        }

        u8 y = ((LY)) % 8;

        auto [affine_map_width, affine_map_height] = get_affine_bg_tile_sizes(2);

        for (size_t tile_x = 0; tile_x < affine_map_width; tile_x++) {
          const ScreenBlockEntryMode1& entry = affine_tile_maps[2][(tile_y * affine_map_width) + tile_x];
          Tile tile                          = tile_sets[2][entry.tile_index];

          for (size_t x = 0; x < 8; x++) {
            auto clr = get_color_by_index(tile[(y * 8) + x], 0, COLOR_DEPTH::BPP8);

            // Palette Index = 0 -- if so save entry in the transparency map. Used during composition

            transparency_maps[2][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = ((tile[(y * 8) + x] == 0));
            // rot_scal_bg2_buf[((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = ((tile[(y * 8) + x] == 0));

            tile_map_affine_texture_buffer_arr[2][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = clr;
          }
        }
      }

      if (LY > 159) return;

      const bool draw_sprites = display_fields.DISPCNT.SCREEN_DISPLAY_OBJ;

      // ====================================  composition ====================================
      std::vector<BG_INFO> active_bgs = {};

      // determine priority
      for (int bg = 2; bg >= 0; bg--) {
        if (!background_enabled(bg)) {
          continue;
        }
        active_bgs.emplace_back(static_cast<u8>(bg), get_bg_prio(bg));
      }

      std::ranges::stable_sort(active_bgs, [](const BG_INFO& a, const BG_INFO& b) {
        if (a.bg_prio != b.bg_prio) return a.bg_prio > b.bg_prio;  // primary
        return a.bg_id > b.bg_id;                                  // tiebreaker
      });

      // draw backdrop
      for (size_t x = 0; x < 240; x++) {
        backdrop[(LY * SYSTEM_DISPLAY_WIDTH) + x] = get_color_by_index(0, 0, COLOR_DEPTH::BPP4);
        auto& bg_pixel                            = background_layer.at((LY * SYSTEM_DISPLAY_WIDTH) + x);

        bg_pixel.prio  = 5;
        bg_pixel.color = get_color_by_index(0, 0, COLOR_DEPTH::BPP4);
      }

      for (const auto& bg : active_bgs) {
        auto [x_offset, y_offset]               = get_text_bg_offset(bg.bg_id);
        auto [x_render_offset, y_render_offset] = get_render_offset(screen_sizes[bg.bg_id]);

        if (bg.bg_id == 2) {
          x_offset        = 0;
          y_offset        = 0;
          x_render_offset = 0;
          y_render_offset = 0;
        }

        for (size_t x = 0; x < 240; x++) {
          auto& bg_px = background_layer.at((LY * SYSTEM_DISPLAY_WIDTH) + x);
          u32 complete_x_offset;
          u32 complete_y_offset;

          if (bg.bg_id == 2) {
            complete_x_offset = (x + latched_bg2x) % 1024;
            complete_y_offset = (LY + y_offset) * 512;

            // complete_x_offset = (x) % 1024;
            // complete_y_offset = ((LY + y_offset)) * 512;

          } else {
            complete_x_offset = (x + x_offset) % x_render_offset;
            complete_y_offset = ((LY + y_offset) % y_render_offset) * 512;
          }

          if (transparency_maps[bg.bg_id][(complete_x_offset + complete_y_offset)]) {
            continue;
          }

          if (bg.bg_id == 2) {
            bg_px.color = tile_map_affine_texture_buffer_arr[2][(complete_x_offset + complete_y_offset)];
            // fmt::println("{}", bg_px.color);
          } else {
            bg_px.color = tile_map_texture_buffer_arr[bg.bg_id][(complete_x_offset + complete_y_offset)];
          }
          bg_px.prio = bg.bg_prio;

          // if (!draw_sprites) {
          //   // fmt::println("BG{} -> {}", bg.bg_id,tile_map_texture_buffer_arr[bg.bg_id][(complete_x_offset + complete_y_offset)]);
          // }
        }
      }

      if (!draw_sprites) {
        for (size_t x = 0; x < 240; x++) {
          const auto bg_px = background_layer.at((LY * SYSTEM_DISPLAY_WIDTH) + x);
          db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, bg_px.color);
        }
        break;
      }

      // process sprites
      // TODO: can just reinterpret cast the OAM
      // when it comes to repopulation, it is only necessary when OAM is written to, OR when >=
      std::memcpy(entries.data(), bus->OAM.data(), 0x400);
      // state.oam_changed = false;
      repopulate_objs();  // TODO: a write to change 1 entry will lead to us re-populating the entire table -- re-populate by index

      for (int entry_idx = 0; entry_idx < 128; entry_idx++) {
        const OAM_Entry& oam_entry = entries.at(entry_idx);

        if (oam_entry.obj_disable_double_sz_flag) continue;                                                      // obj is disabled
        if (oam_entry.obj_mode == OBJ_MODE::PROHIBITED || oam_entry.obj_mode == OBJ_MODE::OBJ_WINDOW) continue;  // obj has a probihibted mode
        if (oam_entry.obj_shape == OBJ_SHAPE::PROHIBITED) continue;                                              // obj is probihibted shape

        u16 y_relative_to_top_of_obj = (((i16)LY - (i16)oam_entry.y) + 256) % 256;
        if (y_relative_to_top_of_obj >= (get_obj_height(oam_entry) * 8)) continue;

        if (((oam_entry.y + (get_obj_height(oam_entry) * 8)) % 256) <= LY) continue;  // we've passed the last scanline

        auto obj_width = get_obj_width(oam_entry);

        for (size_t tile_x = 0; tile_x < obj_width; tile_x++) {
          for (u8 pixel_x = 0; pixel_x < 8; pixel_x++) {
            const auto& palette_index_of_pixel = objs[entry_idx].data.at(((y_relative_to_top_of_obj) * 64) + (tile_x * 8) + pixel_x);

            auto clr = get_obj_color_by_index(palette_index_of_pixel, oam_entry.pal_number, oam_entry.color_depth);

            u32 line_height = (oam_entry.y + y_relative_to_top_of_obj) % 256;
            line_height *= 256;
            u32 f_x = (oam_entry.x + (tile_x * 8) + pixel_x) % 512;

            if (f_x >= 240) continue;

            auto& obj_px = sprite_layer.at(line_height + f_x);

            if (obj_px.prio < oam_entry.priority_relative_to_bg) continue;
            if (obj_px.oam_idx < entry_idx) continue;

            if (palette_index_of_pixel == 0) continue;

            obj_px.color       = clr;
            obj_px.prio        = oam_entry.priority_relative_to_bg;
            obj_px.transparent = palette_index_of_pixel == 0;
            obj_px.oam_idx     = entry_idx;

            // obj_texture_buffer[line_height + f_x] = clr;
            // db.write((LY * MAIN_VIEWPORT_PITCH) + f_x, clr);
          }
        }
        // fmt::print("\n");
      }

      for (size_t x = 0; x < 240; x++) {
        // BG ID = 0 -- OBJ PRIO = 1 // BG GETS DRAWN OVER OBJ
        const auto& bg_px  = background_layer.at((LY * SYSTEM_DISPLAY_WIDTH) + x);
        const auto& obj_px = sprite_layer.at((LY * 256) + x);

        // lower number wins
        if (obj_px.prio > bg_px.prio) {  // draw bg
          db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, bg_px.color);
          // if (bg_px.transparent && bg_px.prio == 0) db.write((LY * MAIN_VIEWPORT_PITCH) + x, Colors::RED);
          // if (bg_px.transparent && bg_px.prio == 1) db.write((LY * MAIN_VIEWPORT_PITCH) + x, Colors::GREEN);
          // if (bg_px.transparent && bg_px.prio == 2) db.write((LY * MAIN_VIEWPORT_PITCH) + x, Colors::BLUE);
          // if (bg_px.transparent && bg_px.prio == 3) db.write((LY * MAIN_VIEWPORT_PITCH) + x, Colors::BLUE + 0x2000);
          if (bg_px.transparent) {
            db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, sprite_layer.at((LY * 256) + x).color);
          }

        } else {
          if (!(sprite_layer.at((LY * 256) + x).transparent)) {
            // auto s_c = sprite_layer.at((LY * 256) + x).color;
            db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, sprite_layer.at((LY * 256) + x).color);
            // if (s_c) db.write((LY * MAIN_VIEWPORT_PITCH) + x, 0x1231231);
          } else {
            db.write((LY * SYSTEM_DISPLAY_WIDTH) + x, bg_px.color);
          }
        }
      }
      break;
    }
    case MODE_3: {
      // fmt::println("MODE_3");
      for (size_t i = 0, j = 0; i < (240 * 160); i++, j += 2) {
        db.write(i, BGR555_TO_RGB888_LUT.at((VRAM.at(j)) | (VRAM.at(j + 1) << 8)));
      }

      // db.swap_buffers();
      break;
    };
    case MODE_4: {
      // fmt::println("MODE_4 - LY: {}", display_fields.VCOUNT.LY);
      const auto& LY   = display_fields.VCOUNT.LY;
      const auto& page = display_fields.DISPCNT.DISPLAY_FRAME_SELECT;

      if (LY > 159) return;

      for (size_t i = 0; i < 240; i++) {
        db.write(((LY * 240) + i), get_color_by_index(VRAM.at(((LY * 240) + i) + (BITMAP_MODE_PAGE_OFFSET * page)), 0, COLOR_DEPTH::BPP8));
      }

      // if(LY == 159) db.swap_buffers();

      break;
    }

    default: {
      ppu_logger->error("mode is not set, or could not draw gfx mode: {} - are you sure the mode is set by the program?", +display_fields.DISPCNT.BG_MODE);
      // assert(0);
      break;
    }
  }
}

u8 PPU::get_bg_prio(const u8 bg) const {
  switch (bg) {
    case 0: return display_fields.BG0CNT.BG_PRIORITY;
    case 1: return display_fields.BG1CNT.BG_PRIORITY;
    case 2: return display_fields.BG2CNT.BG_PRIORITY;
    case 3: return display_fields.BG3CNT.BG_PRIORITY;
    default: assert(0);
  }

  std::unreachable();
}

void PPU::reset_sprite_layer() {
  for (auto& r : sprite_layer) {
    r.prio     = 10;
    r.active   = false;
    r.oam_idx  = 128;
    r.color    = 0;
    r.color_id = 0;
  }
}

void PPU::draw_window() {
  // return;
  // TODO(perf): move to specific register writes. they don't need to be recalculated each time a frame is drawn

  // win 0
  auto w0_x1 = display_fields.WIN0H.X1;  // left
  auto w0_x2 = display_fields.WIN0H.X2;  // right
  auto w0_y1 = display_fields.WIN0V.Y1;  // top
  auto w0_y2 = display_fields.WIN0V.Y2;  // bottom, should be +1

  // win 1
  auto w1_x1 = display_fields.WIN1H.X1;  // left
  auto w1_x2 = display_fields.WIN1H.X2;  // right
  auto w1_y1 = display_fields.WIN1V.Y1;  // top
  auto w1_y2 = display_fields.WIN1V.Y2;  // bottom, should be +1

  if (w0_x2 > 240 || w0_x1 > w0_x2) w0_x2 = 240;
  if (w0_y2 > 160 || w0_y1 > w0_y2) w0_y2 = 160;

  if (w1_x2 > 240 || w1_x1 > w1_x2) w1_x2 = 240;
  if (w1_y2 > 160 || w1_y1 > w1_y2) w1_y2 = 160;

  // draw outside window

  std::vector<BG_INFO> active_bgs = {};

  u8 enabled_bg_bits = get_enabled_bg_bits();

  u8 anded_win_out_bg = enabled_bg_bits & display_fields.WINOUT.OUTSIDE_BG_ENABLE_BITS;
  u8 anded_win0_bg    = enabled_bg_bits & display_fields.WININ.WIN0_BG_ENABLE_BITS;
  u8 anded_win1_bg    = enabled_bg_bits & display_fields.WININ.WIN1_BG_ENABLE_BITS;

  if (anded_win_out_bg != 0 && (display_fields.WININ.WIN0_BG_ENABLE_BITS | display_fields.WININ.WIN1_BG_ENABLE_BITS | display_fields.WININ.WIN0_OBJ_ENABLE_BIT |
                                display_fields.WININ.WIN1_OBJ_ENABLE_BIT)) {  // TODO(perf): replace with memcpy (it's effectively just copying the composite bg)
    for (u8 y = 0; y < 160; y++) {
      for (u8 x = 0; x < 240; x++) {
        for (u8 i = 0; i < 4; i++) {
          if ((1 << i & anded_win_out_bg) != 0) {
            if (!transparency_maps.at(i).at((y * 512) + x)) db.write((y * 240) + x, tile_map_texture_buffer_arr.at(i).at((y * 512) + x));
          }
        }
      }
    }
  }

  if (display_fields.DISPCNT.WINDOW_1_DISPLAY_FLAG) {
    for (u8 y = w1_y1; y < w1_y2; y++) {
      for (u8 x = w1_x1; x < w1_x2; x++) {
        for (u8 i = 0; i < 4; i++) {
          if ((1 << i & anded_win1_bg) != 0) {
            // if (!transparency_maps.at(i).at((y * 512) + x))
            db.write((y * 240) + x, tile_map_texture_buffer_arr.at(i).at((y * 512) + x));
          }
        }

        if (display_fields.WININ.WIN1_OBJ_ENABLE_BIT) {
          const auto& pix = sprite_layer.at(y * 256 + x);
          // if (pix.color_id != 0)
          db.write((y * 240) + x, pix.color);
        }
      }
    }
  }

  if (display_fields.DISPCNT.WINDOW_0_DISPLAY_FLAG) {
    for (i8 i = 3; i > 0; i--) {
      auto [x_offset, y_offset]               = get_text_bg_offset(i);
      auto [x_render_offset, y_render_offset] = get_render_offset(i);

      for (u8 y = w0_y1; y < w0_y2; y++) {
        for (u8 x = w0_x1; x < w0_x2; x++) {
          u32 complete_x_offset = (x + x_offset) % x_render_offset;
          u32 complete_y_offset = ((y + y_offset) % y_render_offset) * 512;

          // (void)(anded_win0_bg);
          // db.write(y*240+x, 0xF2F2F2F2);
          if ((1 << i & anded_win0_bg) != 0) {
            if (!transparency_maps.at(i).at(complete_x_offset + complete_y_offset)) db.write((y * 240) + x, tile_map_texture_buffer_arr.at(i).at((complete_y_offset) + complete_x_offset));
            // }
          }

          if (display_fields.WININ.WIN0_OBJ_ENABLE_BIT) {
            const auto& pix = sprite_layer.at(y * 256 + x);
            if (pix.color_id != 0) db.write((y * 240) + x, pix.color);
          }
        }
      }
    }
  }
}
u8 PPU::get_enabled_bg_bits() const {
  return static_cast<u8>((display_fields.DISPCNT.SCREEN_DISPLAY_BG3 << 3) | (display_fields.DISPCNT.SCREEN_DISPLAY_BG2 << 2) | (display_fields.DISPCNT.SCREEN_DISPLAY_BG0 << 1) |
                         display_fields.DISPCNT.SCREEN_DISPLAY_BG0);
}
#include "ppu.hpp"

#include <algorithm>
#include <cassert>

#include "bus.hpp"
#include "common/color_conversion.hpp"
#include "common/defs.hpp"

// 16 colors (each color being 2 bytes)
static constexpr u8 BANK_SIZE                = 16 * 2;
static constexpr u32 SCREEN_WIDTH            = 512;
static constexpr u32 BITMAP_MODE_PAGE_OFFSET = 0xA000;

bool PPU::is_valid_obj(const OAM_Entry& oam_entry) {
  if (oam_entry.obj_mode == OBJ_MODE::PROHIBITED) return false;
  if (oam_entry.obj_shape == OBJ_SHAPE::PROHIBITED) return false;
  if (oam_entry.obj_disable_double_sz_flag) return false;

  return true;
}

void PPU::repopulate_objs() {
  u8 index = 0;
  for (const auto& entry : entries) {
    if (is_valid_obj(entry) == false) {
      index++;
      continue;
    };
    // fmt::println("processing entry: {}", index);
    Tile current_tile;

    auto obj_width  = get_obj_width(entry);
    auto obj_height = get_obj_height(entry);

    // assert(entry.is_8bpp != true);

    u32 current_charname = 0;

    for (size_t tile_y = 0; tile_y < obj_height; tile_y++) {
      for (size_t tile_x = 0; tile_x < obj_width; tile_x++) {
        if (bus->display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING == Bus::ONE_DIMENSIONAL) {
          current_charname = (entry.char_name + tile_x + (tile_y * 8)) % 1024;
        } else {
          current_charname = (entry.char_name + tile_x + (tile_y * 32)) % 1024;
        }

        current_tile = get_obj_tile_by_tile_index(current_charname, entry.is_8bpp ? BPP8 : BPP4);

        for (size_t y = 0; y < 8; y++) {
          for (size_t x = 0; x < 8; x++) {
            objs[index].data.at(((tile_x * 8) + (tile_y * (64 * 8))) + ((y * 64) + x)) = current_tile.at((y * 8) + x);
          }
        }
      }
    }

    if (entry.horizontal_flip == FLIP::MIRRORED) {
      for (u8 y = 0; y < 64; y++) {
        std::reverse(std::begin(objs[index].data) + (y * 64), std::begin(objs[index].data) + (y * 64) + 64);
      }
    }

    if (entry.vertical_flip == FLIP::MIRRORED) {
      std::array<u8, 64 * 64> tmp;
      for (u8 y = 64; y > 0; y--) {
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
    case OBJ_SHAPE::SQUARE: return 1 << c.obj_size;
    case OBJ_SHAPE::HORIZONTAL: {
      if (c.obj_size == 0) return 1;
      if (c.obj_size == 1) return 1;
      if (c.obj_size == 2) return 2;
      if (c.obj_size == 3) return 4;
      break;
    }
    case OBJ_SHAPE::VERTICAL: {
      if (c.obj_size == 0) return 2;
      if (c.obj_size == 1) return 4;
      if (c.obj_size == 2) return 2;
      if (c.obj_size == 3) return 8;
      break;
    }
    case OBJ_SHAPE::PROHIBITED: assert(0);
  };

  assert(0);
  return 0;
};
u8 PPU::get_obj_width(const OAM_Entry& c) {
  switch (c.obj_shape) {
    case OBJ_SHAPE::SQUARE: return 1 << c.obj_size;
    case OBJ_SHAPE::HORIZONTAL: {
      if (c.obj_size == 0) return 2;
      if (c.obj_size == 1) return 4;
      if (c.obj_size == 2) return 4;
      if (c.obj_size == 3) return 8;
      break;
    }
    case OBJ_SHAPE::VERTICAL: {
      if (c.obj_size == 0) return 1;
      if (c.obj_size == 1) return 1;
      if (c.obj_size == 2) return 2;
      if (c.obj_size == 3) return 4;
      break;
    }
    case OBJ_SHAPE::PROHIBITED: {
      assert(0);
      return -1;
    }
      // default: {assert(0); return -1;}
  };

  return -1;
};

std::string PPU::get_obj_size_string(const OAM_Entry& c) {
  switch (c.obj_shape) {
    case OBJ_SHAPE::SQUARE: {
      if (c.obj_size == 0) return "8x8";
      if (c.obj_size == 1) return "16x16";
      if (c.obj_size == 2) return "32x32";
      if (c.obj_size == 3) return "64x64";

      assert(0);
      break;
    }
    case OBJ_SHAPE::HORIZONTAL: assert(0);
    case OBJ_SHAPE::VERTICAL: assert(0);
    case OBJ_SHAPE::PROHIBITED: assert(0);
  }

  return "";
};

u32 PPU::get_color_by_index(u8 x, u8 palette_num, bool color_depth_is_8bpp) {
  u16 r;

  if (color_depth_is_8bpp) {
    r = bus->read16(PALETTE_RAM_BASE + (x * 2));
  } else {
    r = bus->read16(PALETTE_RAM_BASE + (x * 2) + (0x20 * palette_num));
  }

  return BGR555_TO_RGB888_LUT[r];
}

u32 PPU::get_obj_color_by_index(u8 palette_index, u8 bank_number, bool color_depth_is_8bpp) {
  u16 r = 0;
  if (color_depth_is_8bpp) {
    r = bus->read16(PALETTE_RAM_OBJ_BASE + (palette_index * 2) + bank_number);
  } else {
    // Palette Index * 2, because we're reading a 16 bit value (2 bytes)
    // fmt::println("palette idx = {}", palette_index);
    assert(palette_index <= 15);

    r = bus->read16(PALETTE_RAM_OBJ_BASE + (palette_index * 2) + (BANK_SIZE * bank_number));
  }

  // return BGR555toRGB888((r & 0xFF), (r >> 8) & 0xFF);
  return BGR555_TO_RGB888_LUT[r];
}

std::tuple<u16, u16> PPU::get_bg_offset(u8 bg) {
  switch (bg) {
    case 0: return {+bus->display_fields.BG0HOFS.OFFSET, +bus->display_fields.BG0VOFS.OFFSET};
    case 1: return {+bus->display_fields.BG1HOFS.OFFSET, +bus->display_fields.BG1VOFS.OFFSET};
    case 2: return {+bus->display_fields.BG2HOFS.OFFSET, +bus->display_fields.BG2VOFS.OFFSET};
    case 3: return {+bus->display_fields.BG3HOFS.OFFSET, +bus->display_fields.BG3VOFS.OFFSET};
  }

  assert(0);
  return {};
}
// LHS - V, RHS - H
std::tuple<u16, u16> PPU::get_render_offset(u8 screen_size) {
  switch (screen_size) {
    case 0: return {256, 256};
    case 1: return {512, 256};
    case 2: return {256, 256};
    case 3: return {512, 512};
  }

  assert(0);
  return {};
}

void PPU::load_tiles(u8 bg, u8 color_depth) {
  ppu_logger->debug("loading tiles");
  assert(color_depth < 2);
  u8 x = 0;
  Tile tile;
  u32 rel_cbb = relative_cbb(bg);

  if (color_depth == _4BPP) {
    for (size_t tile_index = 0; tile_index < 1024; tile_index++) {
      for (size_t byte = 0; byte < 32; byte++) {
        tile.at((byte * 2) + x)       = bus->VRAM[rel_cbb + (tile_index * 0x20) + byte] & 0x0F;         // X = 0
        tile.at((byte * 2) + (x + 1)) = (bus->VRAM[rel_cbb + (tile_index * 0x20) + byte] & 0xF0) >> 4;  // X = 1
      }

      tile_sets[bg][tile_index] = tile;
    }
  } else {
    for (size_t tile_index = 0; tile_index < 1024; tile_index++) {
      for (size_t byte = 0; byte < 64; byte++) {
        tile[byte] = bus->VRAM[rel_cbb + (tile_index * 0x40) + byte];
      }

      tile_sets[bg][tile_index] = tile;
    }
  }
}

Tile PPU::get_obj_tile_by_tile_index(u16 tile_id, COLOR_MODE color_mode) {
  u8 x = 0;
  Tile tile;
  if (color_mode == _4BPP) {
    for (size_t byte = 0; byte < 32; byte++) {
      tile[(byte * 2) + x]       = bus->VRAM[OBJ_DATA_OFFSET + (tile_id * 0x20) + byte] & 0x0F;         // X = 0
      tile[(byte * 2) + (x + 1)] = (bus->VRAM[OBJ_DATA_OFFSET + (tile_id * 0x20) + byte] & 0xF0) >> 4;  // X = 1
    }
  } else {
    for (size_t byte = 0; byte < 64; byte++) {
      tile[byte] = bus->VRAM[OBJ_DATA_OFFSET + (tile_id * 0x40) + byte];
    }
  }

  return tile;
}

inline u32 PPU::absolute_sbb(u8 bg, u8 map_x) {
  // assert(bg < 4);

  switch (bg) {
    case 0: return VRAM_BASE + ((bus->display_fields.BG0CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 1: return VRAM_BASE + ((bus->display_fields.BG1CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 2: return VRAM_BASE + ((bus->display_fields.BG2CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 3: return VRAM_BASE + ((bus->display_fields.BG3CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
  }
  throw std::runtime_error("bad sbb");
  return 0;
}
inline u32 PPU::relative_cbb(u8 bg) {
  switch (bg) {
    case 0: return (bus->display_fields.BG0CNT.CHAR_BASE_BLOCK * 0X4000);
    case 1: return (bus->display_fields.BG1CNT.CHAR_BASE_BLOCK * 0X4000);
    case 2: return (bus->display_fields.BG2CNT.CHAR_BASE_BLOCK * 0X4000);
    case 3: return (bus->display_fields.BG3CNT.CHAR_BASE_BLOCK * 0X4000);
  }
  return 0x0;
}

bool PPU::background_enabled(u8 bg_id) {
  switch (bg_id) {
    case 0: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG0);
    case 1: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG1);
    case 2: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG2);
    case 3: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG3);
  }

  assert(0);
  return false;
}
void PPU::clear_buffers() {
  std::memset(tile_map_texture_buffer_0, 0, 512 * 512 * 4);
  std::memset(tile_map_texture_buffer_1, 0, 512 * 512 * 4);
  std::memset(tile_map_texture_buffer_2, 0, 512 * 512 * 4);
  std::memset(tile_map_texture_buffer_3, 0, 512 * 512 * 4);
}
void PPU::step() {
  // TODO: This should be a scanline renderer, at the start of each scanline 1 scanline should be written to the framebuffer.
  //       At VBLANK, the framebuffer should be copied by whatever frontend and drawn onto the screen.
  switch (bus->display_fields.DISPCNT.BG_MODE) {
    case BG_MODE::MODE_0: {
      const auto& LY = bus->display_fields.VCOUNT.LY;

      // TODO: do we really need this on the stack each single time?
      std::array<u8, 4> screen_sizes = {
          bus->display_fields.BG0CNT.SCREEN_SIZE,
          bus->display_fields.BG1CNT.SCREEN_SIZE,
          bus->display_fields.BG2CNT.SCREEN_SIZE,
          bus->display_fields.BG3CNT.SCREEN_SIZE,
      };
      std::array<u8, 4> bg_bpp = {
          bus->display_fields.BG0CNT.COLOR_MODE,
          bus->display_fields.BG1CNT.COLOR_MODE,
          bus->display_fields.BG2CNT.COLOR_MODE,
          bus->display_fields.BG3CNT.COLOR_MODE,
      };

      // process bgs
      for (u8 bg = 0; bg < 4; bg++) {
        if (!background_enabled(bg)) continue;

        auto [x_offset, y_offset] = get_bg_offset(bg);

        // load tiles into tilesets from charblocks

        // if (state.cbb_changed[bg]) {
        load_tiles(bg, bg_bpp[bg]);  // only gets called when dispstat corresponding to bg changes
        // state.cbb_changed[bg] = false;
        // }

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

              tile_maps[bg][dest] = ScreenBlockEntry{
                  .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),  // should be able to directly access vram, bus adds alot of overhead
                  // .v = bus->VRAM.at((absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)) - VRAM_BASE),  // should be able to directly access vram, bus adds alot of overhead

              };
            }
            // };
            // assert(0);
            break;
          }
          case 1: {
            // [0][1]

            for (u32 map_x = 0; map_x < 2; map_x++) {
              for (u32 tile_y = 0; tile_y < 32; tile_y++) {
                for (u32 tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * 32);

                  tile_maps[bg][dest] = ScreenBlockEntry{
                      .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                  };
                }
              }
            };
            break;
          }
          case 2: {
            // [0]
            // [1]

            assert(0);
            break;
          }
          case 3: {  // REFACTOR: reloading the entire tilemap every scanline is unnecessarily expensive
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

                  tile_maps[bg][dest] = ScreenBlockEntry{
                      .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
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
          const ScreenBlockEntry& entry = tile_maps[bg][(tile_y * 64) + tile_x];
          Tile tile                     = tile_sets[bg][entry.tile_index];

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
            transparency_maps[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = ((tile[(y * 8) + x] == 0) ? true : false);

            tile_map_texture_buffer_arr[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = clr;
          }
        }

        // if (LY > 159) continue;
        //  In case that some or all BGs are set to same priority then BG0 is having the highest, and BG3 the lowest priority.
      }

      if (LY > 159) return;
      // ====================================   bg composition ====================================
      std::vector<Item> active_bgs = {};

      // determine priority
      for (u8 bg = 0; bg < 4; bg++) {
        if (!background_enabled(bg)) {
          continue;
        }
        active_bgs.push_back({bg, get_bg_prio(bg)});
      }

      std::stable_sort(active_bgs.begin(), active_bgs.end(), [](const Item& a, const Item& b) {
        if (a.bg_prio != b.bg_prio) return a.bg_prio > b.bg_prio;  // primary
        return a.bg_id > b.bg_id;                                  // tiebreaker 2
      });

      // draw backdrop
      for (size_t x = 0; x < 240; x++) {
        db.write((LY * MAIN_VIEWPORT_PITCH) + x, get_color_by_index(0, 0, true));
      }

      for (const auto& bg : active_bgs) {
        // fmt::println("twan: {}", bg.bg_id);;
        auto [x_offset, y_offset]               = get_bg_offset(bg.bg_id);
        auto [x_render_offset, y_render_offset] = get_render_offset(screen_sizes[bg.bg_id]);
        for (size_t x = 0; x < 240; x++) {
          u32 complete_x_offset = (x + x_offset) % x_render_offset;
          u32 complete_y_offset = ((LY + y_offset) % y_render_offset) * 512;

          assert((complete_x_offset + complete_y_offset) < 256 * 512);

          if (transparency_maps[bg.bg_id][(complete_x_offset + complete_y_offset)]) continue;

          db.write((LY * MAIN_VIEWPORT_PITCH) + x, tile_map_texture_buffer_arr[bg.bg_id][(complete_x_offset + complete_y_offset)]);
        }
      }
      // std::memset(transparency_maps[0].data(), false, 512 * 512);

      // process sprites
      // if (state.oam_changed) {  // reload oam
      std::memcpy(entries.data(), bus->OAM.data(), 0x400);
      state.oam_changed = false;
      repopulate_objs();  // TODO: a write to change 1 entry will lead to us re-populating the entire table -- re-populate by index
      // }

      // if (state.mapping_mode_changed) {
      // repopulate_objs();
      //   state.mapping_mode_changed = false;
      // }

      // const auto& mapping_mode = bus->display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING;
      for (size_t entry_idx = 0; entry_idx < 128; entry_idx++) {
        const auto& oam_entry = entries.at(entry_idx);
        if (oam_entry.y > LY) continue;                                       // not on scanline where we're supposed to draw the sprite
        if ((oam_entry.y + (get_obj_height(oam_entry) * 8)) <= LY) continue;  // we've passed the last scanline
        if (oam_entry.obj_disable_double_sz_flag) continue;                   // obj is disabled
        if (oam_entry.obj_mode == OBJ_MODE::PROHIBITED) continue;             // obj has a probihibted mode
        if (oam_entry.obj_shape == OBJ_SHAPE::PROHIBITED) continue;           // obj is probihibted shape

        // ppu_logger->info("drawing OBJ with charname of: {}", entry.char_name);
        // ppu_logger->info("OBJ X: {}", entry.x);
        // ppu_logger->info("OBJ Y: {}", entry.y);
        // ppu_logger->info("OBJ shape: {}", entry.obj_shape == OBJ_SHAPE::SQUARE ? "square" : "not a square");
        // ppu_logger->info("OBJ H-Flip: {}", entry.horizontal_flip == FLIP::MIRRORED);
        // ppu_logger->info("OBJ V-Flip: {}", entry.vertical_flip == FLIP::MIRRORED);
        // ppu_logger->info("OBJ is 8bpp: {}", entry.is_8bpp == 1);
        // ppu_logger->info("OBJ palette bank: {}", entry.pal_number);
        // ppu_logger->info("OBJ size: {}", get_obj_size_string(entry));
        // ppu_logger->info("current mapping mode: {}", mapping_mode ? "1D" : "2D");
        if (LY == 0) continue;
        if (oam_entry.y == 0) continue;

        // auto char_name = oam_entry.char_name;
        // fmt::println("LY: {}", LY);
        // fmt::println("Entry.Y: {}", entry.y);

        // u8 tile_y = (LY - oam_entry.y) / 8;

        // (void)char_name;
        // (void)tile_y;

        u8 y_relative_to_top_of_obj = (LY - oam_entry.y);
        // Tile current_tile;

        auto obj_width  = get_obj_width(oam_entry);
        auto obj_height = get_obj_height(oam_entry);

        (void)obj_height;

        for (size_t tile_x = 0; tile_x < obj_width; tile_x++) {
          for (u8 pixel_x = 0; pixel_x < 8; pixel_x++) {
            const auto& palette_index_of_pixel = objs[entry_idx].data.at(((y_relative_to_top_of_obj) * 64) + (tile_x * 8) + pixel_x);

            if (palette_index_of_pixel == 0) continue;  // transparent -- skip, color will be that of the whatever is behind it.

            auto clr = get_obj_color_by_index(palette_index_of_pixel, oam_entry.pal_number, oam_entry.is_8bpp);

            // entry.x
            u32 line_height                                                        = (oam_entry.y + y_relative_to_top_of_obj) * 256;
            obj_texture_buffer[line_height + oam_entry.x + (tile_x * 8) + pixel_x] = clr;

            if ((LY * MAIN_VIEWPORT_PITCH) + oam_entry.x + (tile_x * 8) + pixel_x >= 240 * 160) continue;

            db.write((LY * MAIN_VIEWPORT_PITCH) + oam_entry.x + (tile_x * 8) + pixel_x, clr);
          }
        }
        // fmt::print("\n");
      }

      break;
    }

    case BG_MODE::MODE_3: {
      // fmt::println("MODE_3");
      for (size_t i = 0, j = 0; i < (240 * 160); i++, j += 2) {
        db.write(i, BGR555_TO_RGB888_LUT[(bus->VRAM.at(j)) | (bus->VRAM.at(j + 1) << 8)]);
      }

      // db.swap_buffers();
      break;
    };
    case BG_MODE::MODE_4: {
      // fmt::println("MODE_4 - LY: {}", bus->display_fields.VCOUNT.LY);
      const auto& LY   = bus->display_fields.VCOUNT.LY;
      const auto& page = bus->display_fields.DISPCNT.DISPLAY_FRAME_SELECT;

      if (LY > 159) return;

      for (size_t i = 0; i < 240; i++) {
        db.write(((LY * 240) + i), get_color_by_index(bus->VRAM.at(((LY * 240) + i) + (BITMAP_MODE_PAGE_OFFSET * page)), 0, true));
      }

      // if(LY == 159) db.swap_buffers();

      break;
    }

    default: {
      ppu_logger->error("mode is not set, or could not draw gfx mode: {} - are you sure the mode is set by the program?", +bus->display_fields.DISPCNT.BG_MODE);
      // assert(0);
      break;
    }
  }
}

u8 PPU::get_bg_prio(u8 bg) {
  switch (bg) {
    case 0: return bus->display_fields.BG0CNT.BG_PRIORITY;
    case 1: return bus->display_fields.BG1CNT.BG_PRIORITY;
    case 2: return bus->display_fields.BG2CNT.BG_PRIORITY;
    case 3: return bus->display_fields.BG3CNT.BG_PRIORITY;
  }
  assert(0);
  return -1;
}

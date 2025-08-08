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

    assert(entry.is_8bpp != true);

    u32 current_charname = 0;

    for (size_t tile_y = 0; tile_y < obj_height; tile_y++) {
      for (size_t tile_x = 0; tile_x < obj_width; tile_x++) {
        if (bus->display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING == Bus::_1D) {
          current_charname = (entry.char_name + tile_x + (tile_y * 8)) % 1024;
        } else {
          current_charname = (entry.char_name + tile_x + (tile_y * 32)) % 1024;
        }

        current_tile = get_obj_tile_by_tile_index(current_charname, entry.is_8bpp ? BPP8 : BPP4);
        // fmt::println("TILE X: {} -- TILE Y: {} -- ENTRY ID: {}", tile_x, tile_y, (entry.char_name + tile_x + (tile_y * 8)) % 1024);

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
    default: assert(0);
  };

  assert(0);
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
    case OBJ_SHAPE::PROHIBITED: assert(0);
    default: assert(0);
  };

  assert(0);
};

std::string PPU::get_obj_size_string(const OAM_Entry& c) {
  switch (c.obj_shape) {
    case OBJ_SHAPE::SQUARE: {
      if (c.obj_size == 0) return fmt::format("8x8");
      if (c.obj_size == 1) return fmt::format("16x16");
      if (c.obj_size == 2) return fmt::format("32x32");
      if (c.obj_size == 3) return fmt::format("64x64");

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
  u16 r = color_depth_is_8bpp ? bus->read16(PALETTE_RAM_BASE + (x * 2) + palette_num) : bus->read16(PALETTE_RAM_BASE + (x * 2) + (0x20 * palette_num));

  return BGR555toRGB888((r & 0xFF), (r >> 8) & 0xFF);
  // return BGR555_TO_RGB888_LUT[r];
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

  return BGR555toRGB888((r & 0xFF), (r >> 8) & 0xFF);
  // return BGR555_TO_RGB888_LUT[r];
}

std::tuple<u16, u16> PPU::get_bg_offset(u8 bg) {
  switch (bg) {
    case 0: return {+bus->display_fields.BG0HOFS.OFFSET, +bus->display_fields.BG0VOFS.OFFSET};
    case 1: return {+bus->display_fields.BG1HOFS.OFFSET, +bus->display_fields.BG1VOFS.OFFSET};
    case 2: return {+bus->display_fields.BG2HOFS.OFFSET, +bus->display_fields.BG2VOFS.OFFSET};
    case 3: return {+bus->display_fields.BG3HOFS.OFFSET, +bus->display_fields.BG3VOFS.OFFSET};
    default: assert(0);
  }
}
void PPU::load_tiles(u8 bg, u8 color_depth) {
  ppu_logger->debug("loading tiles");
  assert(color_depth < 2);
  u8 x = 0;
  Tile tile;
  if (color_depth == _4BPP) {
    for (size_t tile_index = 0; tile_index < 512; tile_index++) {
      for (size_t byte = 0; byte < 32; byte++) {
        if (x == 8) x = 0;
        tile[((byte / 4) * 8) + x]       = bus->VRAM[relative_cbb(bg) + (tile_index * 0x20) + byte] & 0x0F;         // X = 0
        tile[((byte / 4) * 8) + (x + 1)] = (bus->VRAM[relative_cbb(bg) + (tile_index * 0x20) + byte] & 0xF0) >> 4;  // X = 1
        x += 2;
      }

      tile_sets[bg][tile_index] = tile;
    }
  } else {
    for (size_t tile_index = 0; tile_index < 256; tile_index++) {
      for (size_t byte = 0; byte < 64; byte++) {
        tile[((byte / 8) * 8)] = bus->VRAM[relative_cbb(bg) + (tile_index * 0x40) + byte];
      }

      tile_sets[bg][tile_index] = tile;
    }
  }
}

// Tile PPU::get_obj_tile_by_tile_index(const OAM_Entry& entry) {
Tile PPU::get_obj_tile_by_tile_index(u16 tile_id, COLOR_MODE color_mode) {
  // u32 address = OBJ_DATA_BASE_ADDR + (tile_index*)
  // ppu_logger->debug("fetching tile index: {}", entry.char_name);

  u8 x = 0;
  Tile tile;
  if (color_mode == _4BPP) {
    for (size_t byte = 0; byte < 32; byte++) {
      if (x == 8) x = 0;
      tile[((byte / 4) * 8) + x]       = bus->VRAM[OBJ_DATA_OFFSET + (tile_id * 0x20) + byte] & 0x0F;         // X = 0
      tile[((byte / 4) * 8) + (x + 1)] = (bus->VRAM[OBJ_DATA_OFFSET + (tile_id * 0x20) + byte] & 0xF0) >> 4;  // X = 1
      x += 2;
    }
  } else {
      assert(0 && "hey, this is broken :-)");
    for (size_t byte = 0; byte < 64; byte++) {
      tile[((byte / 8) * 8)] = bus->VRAM[OBJ_DATA_OFFSET + (tile_id * 0x40) + byte];
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
    default: assert(0);
  }
}
inline u32 PPU::relative_cbb(u8 bg) {
  switch (bg) {
    case 0: return (bus->display_fields.BG0CNT.CHAR_BASE_BLOCK * 0X4000);
    case 1: return (bus->display_fields.BG1CNT.CHAR_BASE_BLOCK * 0X4000);
    case 2: return (bus->display_fields.BG2CNT.CHAR_BASE_BLOCK * 0X4000);
    case 3: return (bus->display_fields.BG3CNT.CHAR_BASE_BLOCK * 0X4000);
    default: assert(0);
  }
}

bool PPU::background_enabled(u8 bg_id) {
  switch (bg_id) {
    case 0: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG0);
    case 1: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG1);
    case 2: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG2);
    case 3: return (bus->display_fields.DISPCNT.SCREEN_DISPLAY_BG3);
    default: assert(0);
  }
}
void PPU::step(bool called_manually) {
  // TODO: This should be a scanline renderer, at the start of each scanline 1 scanline should be written to the framebuffer.
  //       At VBLANK, the framebuffer should be copied by whatever frontend and drawn onto the screen.
  switch (bus->display_fields.DISPCNT.BG_MODE) {
    case BG_MODE::MODE_0: {
      if (called_manually) {
        break;
      }
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

        // load tiles into tilesets from charblocks

        if (state.cbb_changed[bg]) {
          load_tiles(bg, bg_bpp[bg]);  // only gets called when dispstat corresponding to bg changes
          state.cbb_changed[bg] = false;
        }

        // Load screenblocks to our background tile map
        switch (screen_sizes[bg]) {
          case 0: {
            // [0]
            u8 map_x = 0;

            // TODO: probably shouldn't re-populate screenblock entry map every single scanline -- expensive
            for (size_t tile_y = 0; tile_y < 32; tile_y++) {
              for (size_t tile_x = 0; tile_x < 32; tile_x++) {
                u32 dest = (tile_y * 64) + tile_x;
                //  + (map_x * 32);
                assert(map_x == 0);

                tile_maps[bg][dest] = ScreenBlockEntry{
                    // avoid read16, use VRAM directly
                    .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                };

                // fmt::println("addr: {:#010x}", absolute_sbb(map_x) + ((tile_x + (tile_y * 32)) * 2));

                // ScreenEntry{.v = bus->read16(VRAM_BASE + (se_index(tile_x, tile_y, 0x800)))};
              }
            };
            // assert(0);
            break;
          }
          case 1: {
            // [0][1]

            for (size_t map_x = 0; map_x < 2; map_x++) {
              for (size_t tile_y = 0; tile_y < 32; tile_y++) {
                for (size_t tile_x = 0; tile_x < 32; tile_x++) {
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
          case 3: {
            // [0][1]
            // [2][3]
            for (size_t map_x = 0; map_x < 4; map_x++) {
              for (size_t tile_y = 0; tile_y < 32; tile_y++) {
                for (size_t tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * 32);

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

        // write scanlines to the buffers
        u8 tile_y = LY / 8;
        u8 y      = LY % 8;

        // for (size_t tile_y = 0; tile_y < 32; tile_y++) {
        for (size_t tile_x = 0; tile_x < 32; tile_x++) {
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

            tile_map_texture_buffer_arr[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = clr;
          }
        }

        // REFACTOR: because we render scanline by scanline
        //  we essentially just copy to the tile map, then we grab the lines from it, however we ALSO render each scanline in the tilemap line by line
        // if we were to do it each frame... performance rip, so for now a little hack that just renders the bottom 28 missing lines if we hit scanline 227
        // find a better way to do it later.
        if (LY == 226) {
          for (u16 _ly = 226; _ly < 0x100; _ly++) {
            u8 tile_y = _ly / 8;
            u8 y      = _ly % 8;

            // for (size_t tile_y = 0; tile_y < 32; tile_y++) {
            for (size_t tile_x = 0; tile_x < 32; tile_x++) {
              const ScreenBlockEntry& screen_entry = tile_maps[bg][(tile_y * 64) + tile_x];
              Tile tile                            = tile_sets[bg][screen_entry.tile_index];

              if (screen_entry.VERTICAL_FLIP) {
                Tile tb;

                for (size_t row = 0; row < 8; row++) {
                  for (size_t pix_n = 0; pix_n < 8; pix_n++) {
                    tb[(row * 8) + pix_n] = tile[((7 - row) * 8) + pix_n];
                  }
                }

                tile = tb;
              }
              if (screen_entry.HORIZONTAL_FLIP) {
                for (size_t row = 0; row < 8; row++) {
                  std::reverse(tile.begin() + (row * 8), (tile.begin() + 8 + (row * 8)));
                }
              }

              for (size_t x = 0; x < 8; x++) {
                auto clr = get_color_by_index(tile[(y * 8) + x], screen_entry.PAL_BANK, bg_bpp[bg]);

                tile_map_texture_buffer_arr[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = clr;
              }
            }
          }
        }

        if (LY > 159) continue;
        //  In case that some or all BGs are set to same priority then BG0 is having the highest, and BG3 the lowest priority.
        auto [x_offset, y_offset] = get_bg_offset(bg);
        for (size_t x = 0; x < 240; x++) {
          u32 complete_x_offset = (x + x_offset) % 256;
          u32 complete_y_offset = ((LY + y_offset) % 256) * 512;

          assert((complete_x_offset + complete_y_offset) < 256 * 512);

          // if (transparency_maps[bg][(complete_x_offset + complete_y_offset)]) continue;
// fmt::println("LY: {}", LY);
// fmt::println("MVP: {}", MAIN_VIEWPORT_PITCH);
// fmt::println("x: {}", x);


          db.write((LY * MAIN_VIEWPORT_PITCH) + x, tile_map_texture_buffer_arr[bg][(complete_x_offset + complete_y_offset)]);
        }
      }

      // process sprites
      if (state.oam_changed) {  // reload oam
        std::memcpy(entries.data(), bus->OAM.data(), 0x400);
        state.oam_changed = false;
        repopulate_objs();  // TODO: a write to change 1 entry will lead to us re-populating the entire table -- re-populate by index
      }

      if (state.mapping_mode_changed) {
        repopulate_objs();
        state.mapping_mode_changed = false;
      }

      if (LY > 159) return;
      // const auto& mapping_mode = bus->display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING;
      for (size_t entry_idx = 127; entry_idx > 0; entry_idx--) {
        const auto& oam_entry = entries.at(entry_idx);
        // if (entry.y != LY) continue;  // obj is not on the current line
        if (oam_entry.y > LY) continue;                                          // not on scanline where we're supposed to draw the sprite
        if ((oam_entry.y + (get_obj_height(oam_entry) * 7) - 1) < LY) continue;  // we've passed the scanline
        if (oam_entry.obj_disable_double_sz_flag) continue;                      // obj is disabled
        if (oam_entry.obj_mode == OBJ_MODE::PROHIBITED) continue;                // obj has a probihibted mode
        if (oam_entry.obj_shape == OBJ_SHAPE::PROHIBITED) continue;              // obj is probihibted shape

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

        auto char_name = oam_entry.char_name;
        // fmt::println("LY: {}", LY);
        // fmt::println("Entry.Y: {}", entry.y);

        u8 tile_y = (LY - oam_entry.y) / 8;

        (void)char_name;
        (void)tile_y;

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
            /*

            [00][01][02][03][04][05][06][07]
            [08][09][10][11][12][13][14][15]
            [00][01][02][03][04][05][06][07]
            [00][01][02][03][04][05][06][07]
            [00][01][02][03][04][05][06][07]
            [00][01][02][03][04][05][06][07]

            */
            // entry.x
            u32 line_height                                                        = (oam_entry.y + y_relative_to_top_of_obj) * 256;
            obj_texture_buffer[line_height + oam_entry.x + (tile_x * 8) + pixel_x] = clr;
            // fmt::println("LY = {}", LY);
            // fmt::println("MVP = {}", MAIN_VIEWPORT_PITCH);
            // fmt::println("oam_entry.x = {}", oam_entry.x);
            // fmt::println("tile_x = {}", tile_x);
            // fmt::println("pixel_x: {}",  pixel_x);
            
            if((LY * MAIN_VIEWPORT_PITCH) + oam_entry.x + (tile_x * 8) + pixel_x >= 240*160) continue;
            db.write((LY * MAIN_VIEWPORT_PITCH) + oam_entry.x + (tile_x * 8) + pixel_x, clr);
            // [line_height + entry.x + (tile_x * 8) + pixel_x] = clr;
          }
        }
        // fmt::print("\n");
      }

      break;
    }

    case BG_MODE::MODE_3: {
      for (size_t i = 0, j = 0; i < (240 * 160); i++, j += 2) {
        db.write(i, BGR555toRGB888(bus->VRAM.at(j), bus->VRAM.at(j + 1)));
      }

      db.request_swap();
      break;
    };
    case BG_MODE::MODE_4: {
      const auto& LY   = bus->display_fields.VCOUNT.LY;
      const auto& page = bus->display_fields.DISPCNT.DISPLAY_FRAME_SELECT;

      if (LY > 160) return;

      for (size_t i = 0; i < 240; i++) {
        db.write(((LY * 240) + i), get_color_by_index(bus->VRAM.at(((LY * 240) + i) + (BITMAP_MODE_PAGE_OFFSET * page)), 0, true));
      }

      db.request_swap();

      break;
    }

    default: {
      ppu_logger->error("mode is not set, or could not draw gfx mode: {} - are you sure the mode is set by the program?", +bus->display_fields.DISPCNT.BG_MODE);
      assert(0);
      break;
    }
  }
}
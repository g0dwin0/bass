#include "ppu.hpp"

#include <algorithm>

#include "common/color_conversion.hpp"
#include "common/defs.hpp"

static constexpr u32 SCREEN_WIDTH = 512;
static constexpr u32 BITMAP_MODE_PAGE_OFFSET = 0xA000;

u32 PPU::get_color_by_index(u8 x, u8 palette_num, bool color_depth_is_8bpp) {
  u16 r = color_depth_is_8bpp ? bus->read16(PALETTE_RAM_BASE + (x * 2) + palette_num) : bus->read16(PALETTE_RAM_BASE + (x * 2) + (0x20 * palette_num));

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
  Tile t;
  if (color_depth == 0) {
    for (size_t tile_index = 0; tile_index < 512; tile_index++) {
      for (size_t byte = 0; byte < 32; byte++) {
        if (x == 8) x = 0;
        t[((byte / 4) * 8) + x]       = bus->VRAM[relative_cbb(bg) + (tile_index * 0x20) + byte] & 0x0F;         // X = 0
        t[((byte / 4) * 8) + (x + 1)] = (bus->VRAM[relative_cbb(bg) + (tile_index * 0x20) + byte] & 0xF0) >> 4;  // X = 1
        x += 2;
      }

      tile_sets[bg][tile_index] = t;
    }
  } else {
    for (size_t tile_index = 0; tile_index < 256; tile_index++) {
      for (size_t byte = 0; byte < 64; byte++) {
        t[((byte / 8) * 8)] = bus->VRAM[relative_cbb(bg) + (tile_index * 0x40) + byte];
      }

      tile_sets[bg][tile_index] = t;
    }
  }
}
u32 PPU::absolute_sbb(u8 bg, u8 map_x) {
  // assert(bg < 4);

  switch (bg) {
    case 0: return VRAM_BASE + ((bus->display_fields.BG0CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 1: return VRAM_BASE + ((bus->display_fields.BG1CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 2: return VRAM_BASE + ((bus->display_fields.BG2CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 3: return VRAM_BASE + ((bus->display_fields.BG3CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    default: assert(0);
  }
}
u32 PPU::relative_cbb(u8 bg) {
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

      const auto& LY = bus->display_fields.VCOUNT.LY;

      // Load tileset for enabled backgrounds
      // for (size_t y = 0; y < 512; y++) {
      //   for (size_t x = 0; x < 512; x++) {
      //     backdrop[(y*512) + x] = get_color_by_index(0, 0);
      //   }
      // }
      for (u8 bg = 0; bg < 4; bg++) {
        if (!background_enabled(bg)) continue;

        // load tiles into tilesets from charblocks

        if (state.cbb_changed[bg]) {
          load_tiles(bg, bg_bpp[bg]);  // TODO: only call this when dispstat changes
          state.cbb_changed[bg] = false;
        }

        // Load tile map from screenblocks
        switch (screen_sizes[bg]) {
          case 0: {
            // [0]
            u8 map_x = 0;

            for (size_t tile_y = 0; tile_y < 32; tile_y++) {
              for (size_t tile_x = 0; tile_x < 32; tile_x++) {
                u32 dest = (tile_y * 64) + tile_x;
                //  + (map_x * 32);
                assert(map_x == 0);

                tile_maps[bg][dest] = ScreenEntry{
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

                  tile_maps[bg][dest] = ScreenEntry{
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

                  tile_maps[bg][dest] = ScreenEntry{
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
          const ScreenEntry& entry = tile_maps[bg][(tile_y * 64) + tile_x];
          Tile tile                = tile_sets[bg][entry.tile_index];

          if (entry.VERTICAL_FLIP) {
            Tile tb;

            for (size_t row = 0; row < 8; row++) {
              for (size_t pix_n = 0; pix_n < 8; pix_n++) {
                tb[(row * 8) + pix_n] = tile[((7 - row) * 8) + pix_n];
              }
            }

            tile = tb;
          }
          if (entry.HORIZONTAL_FLIP) {
            for (size_t row = 0; row < 8; row++) {
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
              const ScreenEntry& entry = tile_maps[bg][(tile_y * 64) + tile_x];
              Tile tile                = tile_sets[bg][entry.tile_index];

              if (entry.VERTICAL_FLIP) {
                Tile tb;

                for (size_t row = 0; row < 8; row++) {
                  for (size_t pix_n = 0; pix_n < 8; pix_n++) {
                    tb[(row * 8) + pix_n] = tile[((7 - row) * 8) + pix_n];
                  }
                }

                tile = tb;
              }
              if (entry.HORIZONTAL_FLIP) {
                for (size_t row = 0; row < 8; row++) {
                  std::reverse(tile.begin() + (row * 8), (tile.begin() + 8 + (row * 8)));
                }
              }

              for (size_t x = 0; x < 8; x++) {
                auto clr = get_color_by_index(tile[(y * 8) + x], entry.PAL_BANK, bg_bpp[bg]);

                tile_map_texture_buffer_arr[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * SCREEN_WIDTH) + ((tile_x * 8) + x))] = clr;
              }
            }
          }
        }

        if (LY > 160) continue;
        //  In case that some or all BGs are set to same priority then BG0 is having the highest, and BG3 the lowest priority.
        auto [x_offset, y_offset] = get_bg_offset(bg);
        for (size_t x = 0; x < 240; x++) {
          u32 complete_x_offset = (x + x_offset) % 256;
          u32 complete_y_offset = ((LY + y_offset) % 256) * 512;

          assert((complete_x_offset + complete_y_offset) < 256 * 512);

          // if (transparency_maps[bg][(complete_x_offset + complete_y_offset)]) continue;

          db.write((LY * MAIN_VIEWPORT_PITCH) + x, tile_map_texture_buffer_arr[bg][(complete_x_offset + complete_y_offset)]);
        }
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
      ppu_logger->info("mode is not set, or could not draw gfx mode: {} - are you sure the mode is set by the program?", +bus->display_fields.DISPCNT.BG_MODE);
      assert(0);
      break;
    }
  }
}

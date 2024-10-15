#include "ppu.hpp"

#include "common/color_conversion.hpp"
#define VRAM_BASE 0x06000000
#define PALETTE_RAM_BASE 0x05000000
#define MAIN_VIEWPORT_PITCH 240

u32 PPU::get_color_by_index(const u8 x, u8 palette_num) {
  u16 r = bus->read16(PALETTE_RAM_BASE + (x * 2) + (0x20 * palette_num));

  return BGR555toRGB888((r & 0xFF), (r >> 8) & 0xFF);
}

u32 PPU::absolute_sbb(u8 bg, u8 map_x) {
  assert(bg < 4);

  switch (bg) {
    case 0: return VRAM_BASE + ((bus->display_fields.BG0CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 1: return VRAM_BASE + ((bus->display_fields.BG1CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 2: return VRAM_BASE + ((bus->display_fields.BG2CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    case 3: return VRAM_BASE + ((bus->display_fields.BG3CNT.SCREEN_BASE_BLOCK + map_x) * 0X800);
    default: assert(0);
  }
}
u32 PPU::relative_cbb(u8 bg) {
  assert(bg < 4);
  switch (bg) {
    case 0: return (bus->display_fields.BG0CNT.CHAR_BASE_BLOCK * 0X4000);
    case 1: return (bus->display_fields.BG1CNT.CHAR_BASE_BLOCK * 0X4000);
    case 2: return (bus->display_fields.BG2CNT.CHAR_BASE_BLOCK * 0X4000);
    case 3: return (bus->display_fields.BG3CNT.CHAR_BASE_BLOCK * 0X4000);
    default: assert(0);
  }
}

void PPU::draw(bool called_manually) {
  // TODO: This should be a scanline renderer, at the start of each scanline 1 scanline should be written to the framebuffer.
  //       At VBLANK, the framebuffer should be copied by whatever frontend and drawn onto the screen.
  switch (bus->display_fields.DISPCNT.BG_MODE) {
    case BG_MODE::MODE_0: {
      if (called_manually) { break; }

      std::array<size_t, 4> screen_sizes = {
          bus->display_fields.BG0CNT.SCREEN_SIZE,
          bus->display_fields.BG1CNT.SCREEN_SIZE,
          bus->display_fields.BG2CNT.SCREEN_SIZE,
          bus->display_fields.BG3CNT.SCREEN_SIZE,
      };
      const auto& LY = bus->display_fields.VCOUNT.LY;

      // break;
      u8 x = 0;
      Tile t;

      // Load tileset for enabled backgrounds
      for (size_t bg = 0; bg < 4; bg++) {
        // load tiles into tilesets from charblocks
        for (size_t tile_index = 0; tile_index < 512; tile_index++) {
          for (size_t byte = 0; byte < 32; byte++) {
            if (x == 8) x = 0;

            // u8 lsb = bus->VRAM[tile_index + byte] & 0x0F;
            // u8 msb = (bus->VRAM[tile_index + byte] & 0xF0) >> 4;
            // fmt::println("byte ({}) / 32: {}", byte, (byte / 4));
            t[((byte / 4) * 8) + x]       = bus->VRAM[relative_cbb(bg) + (tile_index * 0x20) + byte] & 0x0F;         // X = 0
            t[((byte / 4) * 8) + (x + 1)] = (bus->VRAM[relative_cbb(bg) + (tile_index * 0x20) + byte] & 0xF0) >> 4;  // X = 1

            // fmt::println("color index: {}", t[(byte / 4) + (x)]);
            // fmt::println("color index: {}", t[(byte / 4) + (x + 1)]);

            // u8 palette_id = 3;

            // tile_set_texture[((tile_index * (240 * 8)) % 38400) + ((byte / 4) * 240) + ((tile_index / 20) * 8) + x] = get_color_by_index(bus->VRAM[(tile_index * 0x20) + byte] & 0x0F, palette_id);

            // tile_set_texture[((tile_index * (240 * 8)) % 38400) + ((byte / 4) * 240) + ((tile_index / 20) * 8) + (x + 1)] =
            //     get_color_by_index((bus->VRAM[(tile_index * 0x20) + byte] & 0xF0) >> 4, palette_id);  // X = 1

            // fmt::print("{} {}", t[(byte / 4) + x], tile_set_texture[((tile_index * (240 * 8)) % 38400) + ((byte / 4) * 240) + ((tile_index / 20) * 8) + (x + 1)]);
            // fmt::println("");
            // fmt::print("{} {}", tile_set_texture[((tile_index * (240 * 8)) % 38400) + ((byte / 4) * 240) + ((tile_index / 20) * 8) + x],
            //  tile_set_texture[((tile_index * (240 * 8)) % 38400) + ((byte / 4) * 240) + ((tile_index / 20) * 8) + (x + 1)]);

            // fmt::println("tile index: {}", tile_index);
            // if (tile_index == 2) break;
            x += 2;
            SPDLOG_DEBUG("HELLO!");
          }

          tile_sets[bg][tile_index] = t;
        }

        // Load tile map from screenblocks
        switch (screen_sizes[bg]) {
          case 0: {
            // [0]
            for (size_t map_x = 0; map_x < 1; map_x++) {
              for (size_t tile_y = 0; tile_y < 32; tile_y++) {
                for (size_t tile_x = 0; tile_x < 32; tile_x++) {
                  u32 dest = (tile_y * 64) + tile_x + (map_x * 32);

                  tile_maps[bg][dest] = ScreenEntry{
                      .v = bus->read16(absolute_sbb(bg, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                  };

                  // fmt::println("addr: {:#010x}", absolute_sbb(map_x) + ((tile_x + (tile_y * 32)) * 2));

                  // ScreenEntry{.v = bus->read16(VRAM_BASE + (se_index(tile_x, tile_y, 0x800)))};
                }
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
                      .v = bus->read16(absolute_sbb(1, map_x) + ((tile_x + (tile_y * 32)) * 2)),
                  };
                }
              }
            };
            // assert(0);
            break;
          }
        }

        /* for (size_t r = 0; r < 64; r++) {
        //   for (size_t a = 0; a < 64; a++) {
        //     fmt::print("{:02d} ", +tile_map[(r * 64) + a].tile_index);
        //   }
        //   fmt::println("");
        // }

        // Map is copied, time to render.
        // fmt::println("Map is copied, time to render.");
        // u32 iter = 0;
        */

        // write scanlines to the buffers
        // for (size_t tile_y = 0; tile_y < 64; tile_y++) {
        // fmt::println("iter: {}", iter++);
        // read out screen entry
        // assert(((tile_y * 32) + tile_x) < (32 * 32));

        u8 tile_y = LY / 8;
        u8 y      = LY % 8;

        for (size_t tile_x = 0; tile_x < 64; tile_x++) {
          const ScreenEntry& entry = tile_maps[bg][(tile_y * 64) + tile_x];
          const Tile& tile         = tile_sets[bg][entry.tile_index];
          const u32 SCREEN_WIDTH   = 512;

          for (size_t tile_x = 0; tile_x < 64; tile_x++) {
            for (size_t x = 0; x < 8; x++) {
              tile_map_texture_buffer_arr[bg][((tile_y * (SCREEN_WIDTH * 8)) + (y * 512) + ((tile_x * 8) + x))] = get_color_by_index(tile[(y * 8) + x], entry.PAL_BANK);
            }
          }
        }
        // fmt::println("Palette bank: {}", entry.PAL_BANK);
        // }
        // }
      }

      // Draw to main viewport (adjusted with scrolling registers)
      // u32 x_offset = bus->display_fields.BG0HOFS.OFFSET;
      // u32 y_offset = bus->display_fields.BG0VOFS.OFFSET;

      // for (size_t y = 0; y < 160; y++) {
      //   for (size_t x = 0; x < 240; x++) {
      //     frame_buffer[(y * MAIN_VIEWPORT_PITCH) + x] = tile_map_texture_buffer[x_offset + (y_offset * 512) + (y * 512) + x];
      //   }
      // }

      break;
    }
    case BG_MODE::MODE_3: {
      fmt::println("draw called");
      for (size_t i = 0, j = 0; i < (240 * 160); i++, j += 2) {
        frame_buffer[i] = BGR555toRGB888(bus->VRAM.at(j), bus->VRAM.at(j + 1));
      }
      break;
    };
    case BG_MODE::MODE_4: {
      for (size_t i = 0; i < (240 * 160); i++) {
        frame_buffer[i] = get_color_by_index(bus->VRAM.at(i));
      }
      break;
    }

    default: {
      SPDLOG_DEBUG("mode is not set, or could not draw gfx mode: {} - are you sure the mode is set by the program?", +bus->display_fields.DISPCNT.BG_MODE);
      break;
    }
  }
}

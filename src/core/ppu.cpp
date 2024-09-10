#include "ppu.hpp"
#include "spdlog/spdlog.h"
#include "utils.h"

u32 PPU::get_color_by_index(const u8 x) {
  u16 r  = bus->read16(0x05000000 + (x * 2), true);
  u8 lsb = r & 0xFF;
  u8 msb = ((r >> 8) & 0xFF);
  

  u32 f = BGR555toRGB888(lsb, msb);

  return f;
}
void PPU::draw() {
  switch (bus->display_fields.DISPCNT.BG_MODE) {
    case BG_MODE::BITMAP_MODE_3: {
      fmt::println("draw called");
      for (size_t i = 0, j = 0; i < (240 * 160); i++, j += 2) {
        frame_buffer[i] = BGR555toRGB888(bus->VRAM.at(j), bus->VRAM.at(j + 1));
      }
      break;
    };
    case BG_MODE::BITMAP_MODE_4: {
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

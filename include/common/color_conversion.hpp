#pragma once
#include <array>
#include <unordered_map>
#include <utility>

#include "defs.hpp"

inline constexpr u8 convert5to8(uint8_t value5) {
  return (value5 * 255 + 15) / 31;  // Adding 15 for rounding
}

inline constexpr u32 BGR555toRGB888(u8 lsb, u8 msb) {
  const u16 bgr555 = (static_cast<u16>(msb) << 8) | lsb;

  const u8 blue5  = (bgr555 & 0x1F);
  const u8 green5 = (bgr555 >> 5) & 0x1F;
  const u8 red5   = (bgr555 >> 10) & 0x1F;

  const u8 b = convert5to8(blue5);
  const u8 g = convert5to8(green5);
  const u8 r = convert5to8(red5);

  return (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | b;
}

constexpr std::array<u32, 0x10000> compute_color_lut() {
  std::array<u32, 0x10000> lut;

  for (u8 lsb = 0; lsb < 255; lsb++) {
    for (u8 msb = 0; msb < 255; msb++) {
      lut.at((msb << 8) + lsb) = BGR555toRGB888(lsb, msb);
    }
  }

  return lut;
}

constexpr auto BGR555_TO_RGB888_LUT = compute_color_lut();

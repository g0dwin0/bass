#pragma once
#include <array>
#include <cmath>

#include "defs.hpp"

constexpr u8 convert5to8(uint8_t value5) { return (value5 * 255 + 15) / 31; }

constexpr u32 BGR555toRGB888(u8 msb, u8 lsb) {
  const u16 bgr555 = (static_cast<u16>(msb) << 8) | lsb;

  u8 B = (bgr555 & 0x1F);
  u8 G = (bgr555 >> 5) & 0x1F;
  u8 R = (bgr555 >> 10) & 0x1F;

  double lcdGamma = 4.0, outGamma = 2.2;
  double lb = pow(B / 31.0, lcdGamma);
  double lg = pow(G / 31.0, lcdGamma);
  double lr = pow(R / 31.0, lcdGamma);

  u8 r = static_cast<u8>(pow((0 * lb + 50 * lg + 255 * lr) / 255, 1 / outGamma) * (255 * 255 / 280));
  u8 g = static_cast<u8>(pow((30 * lb + 230 * lg + 10 * lr) / 255, 1 / outGamma) * (255 * 255 / 280));
  u8 b = static_cast<u8>(pow((220 * lb + 10 * lg + 50 * lr) / 255, 1 / outGamma) * (255 * 255 / 280));

  return (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | b;
}

constexpr std::array<u32, 0x10000> compute_color_lut() {
  std::array<u32, 0x10000> lut;

  for (u16 msb = 0; msb <= 255; msb++) {
    for (u16 lsb = 0; lsb <= 255; lsb++) {
      lut.at((msb << 8) + lsb) = BGR555toRGB888(msb, lsb);
    }
  }

  return lut;
}

constexpr auto BGR555_TO_RGB888_LUT = compute_color_lut();

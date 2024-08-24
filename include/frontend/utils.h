#pragma once
#include "common.hpp"

inline u8 convert5to8(uint8_t value5) { 
    return (value5 * 255 + 15) / 31; // Adding 15 for rounding
}

inline u32 BGR555toRGB888(u8 lsb, u8 msb) {
    u16 bgr555 = (static_cast<u16>(msb) << 8) | lsb;

    // Corrected bit extraction for BGR555
    u8 blue5  = (bgr555 & 0x1F);        // Bits 0-4
    u8 green5 = (bgr555 >> 5) & 0x1F;   // Bits 5-9
    u8 red5   = (bgr555 >> 10) & 0x1F;  // Bits 10-14

    u8 b = convert5to8(blue5);
    u8 g = convert5to8(green5);
    u8 r = convert5to8(red5);

    return (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | b;
}
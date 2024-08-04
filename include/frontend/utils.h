#pragma once
#include "common.hpp"

uint8_t convert5to8(uint8_t value5) {
    return (value5 << 3) | (value5 >> 2);
}

// Function to convert BGR555 to RGB888 ignoring the alpha bit and return the RGB888 value
uint32_t BGR555toRGB888(uint8_t lsb, uint8_t msb) {

    uint16_t bgr555 = (static_cast<uint16_t>(msb) << 8) | lsb;

    uint8_t blue5  = (bgr555 & 0x1F);
    uint8_t green5 = (bgr555 >> 5) & 0x1F;
    uint8_t red5   = (bgr555 >> 10) & 0x1F;


    uint8_t b = convert5to8(blue5);
    uint8_t g = convert5to8(green5);
    uint8_t r = convert5to8(red5);
    
    return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
}
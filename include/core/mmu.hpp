#pragma once
#include "common.hpp"
#include "bus.h"

struct MMU {
    u8 read8(const u32 address) const;
    // void write8(const u32 address, const u8 value) const;

    // u8 read16(const u32 address) const;
    // void write16(const u32 address, const u8 value) const;

    // u8 read32(const u32 address) const;
    // void write32(const u32 address, const u8 value) const;

    

    Bus* bus;
};
#pragma once
#include "common.hpp"
#include "cart.hpp"
#include <array>
using namespace Bass;

struct Bus {
    // memory/devices
    std::array<u8, 0x4000> BIOS;
    std::array<u8, 0x40000> EWRAM;    
    std::array<u8, 0x8000> IWRAM;
    
    Cart cart;
};
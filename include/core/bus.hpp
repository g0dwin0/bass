#pragma once

#include "common.hpp"
#include "pak.hpp"

struct Bus {
  // memory/devices

  std::vector<u8> BIOS;
  std::vector<u8> IWRAM;
  std::vector<u8> EWRAM;

  Bus(): BIOS(0x4000), IWRAM(0x8000), EWRAM(0x40000) { spdlog::debug("created bus"); }

  Pak* pak = nullptr;

  u8 read8(u32 address) const;
  u16 read16(u32 address) const;
  u32 read32(u32 address);

  
  u16 shift_16(std::vector<u8> vec, u32 address);
  u32 shift_32(std::vector<u8> vec, u32 address);
  
};
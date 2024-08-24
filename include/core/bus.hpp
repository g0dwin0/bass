#pragma once

#include "common.hpp"
#include "pak.hpp"
struct Bus;
struct PPU;
#include "ppu.hpp"
#include "spdlog/spdlog.h"
struct Bus {
  // memory/devices
  std::vector<u8> BIOS;
  std::vector<u8> IWRAM;
  std::vector<u8> EWRAM;
  std::vector<u8> IO;
  std::vector<u8> VRAM;
  std::vector<u8> PALETTE_RAM;
  std::vector<u8> OAM;
  std::vector<u8> SRAM;
  

  u32 cycles_elapsed = 0;

  Bus()
      : BIOS(0x4000), IWRAM(0x8000), EWRAM(0x40000), IO(0x500), VRAM(0x18000), PALETTE_RAM(0x400), OAM(0x400), SRAM(0x10000)  {
    spdlog::debug("created bus");
  }

  Pak* pak = nullptr;
  PPU* ppu = nullptr;

  u8 read8(u32 address);
  u16 read16(u32 address);
  u32 read32(u32 address);

  void write8(u32 address, u8 value);
  void write16(u32 address, u16 value);
  void write32(u32 address, u32 value);

  void set8(std::vector<u8>& v, u32 address,
            u8 value);  // TODO: move to utils or smth
  void set16(std::vector<u8>& v, u32 address,
             u16 value);  // TODO: move to utils or smth
  void set32(std::vector<u8>& v, u32 address,
             u32 value);  // TODO: move to utils or smth

  u16 shift_16(const std::vector<u8>& vec, const u32 address);
  u32 shift_32(const std::vector<u8>& vec, const u32 address);
};
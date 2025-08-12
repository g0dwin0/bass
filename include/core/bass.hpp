#pragma once
#include "common/stopwatch.hpp"
#include "core/bus.hpp"
#include "core/cpu.hpp"
#include "core/pak.hpp"
#include "core/ppu.hpp"
#include "dma.hpp"
struct Bass {
  ARM7TDMI cpu = {};
  PPU ppu      = {};
  Bus bus      = {};
  Pak pak      = {};

  Stopwatch stopwatch;

  std::array<DMAContext*, 4> dma_channels;

  std::atomic<bool> active = true;

  Bass();
  ~Bass();

  void set_ppu_interrupts();
  void check_for_dma();
  void system_loop();

 private:
  bool start_timing_cond_met(DMAContext* ch);
};

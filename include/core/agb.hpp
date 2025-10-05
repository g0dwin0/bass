#pragma once
// #include "common/stopwatch.hpp"
#include "core/bus.hpp"
#include "core/cpu.hpp"
#include "core/dma.hpp"
#include "core/pak.hpp"
#include "core/ppu.hpp"
#include "timer.hpp"

struct AGB {
  ARM7TDMI cpu = {};
  PPU ppu      = {};
  Bus bus      = {};
  Pak pak      = {};
  static u64 cycles_elapsed;

  // Stopwatch stopwatch;

  std::array<DMAContext*, 4> dma_channels;
  std::array<Timer, 4> timers;

  std::atomic<bool> active = true;

  AGB();
  ~AGB();

  void check_for_dma() const;
  void tick_timers(u64 cycles);
  void system_loop();

  bool start_timing_cond_met(DMAContext* ch) const;
};

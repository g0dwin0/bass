#pragma once

#include "bus.hpp"
#include "common.hpp"


#include "registers.hpp"

struct ARM7TDMI {
  ARM7TDMI() { spdlog::debug("created arm"); }
  Registers regs;

  Bus* bus = nullptr;

  // STEP

  struct Pipeline {
    u32 fetch   = 0;
    u32 decode  = 0;
    u32 execute = 0;

    bool first_cycle = true;
  } pipeline;

  u32 fetch(const u32 address);
  void decode();
  void execute(u32 operation);

  void step();
};
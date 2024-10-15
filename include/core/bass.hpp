#pragma once
#include "core/cpu.hpp"
#include "core/bus.hpp"
#include "core/pak.hpp"
#include "core/ppu.hpp"
struct Bass {
    
  ARM7TDMI cpu;
  PPU ppu;
  
  Bus bus;
  Pak pak;

  Bass();
  void check_and_handle_interrupts();
};

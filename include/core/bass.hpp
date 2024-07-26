#pragma once
#include "core/cpu.hpp"
#include "core/bus.hpp"
#include "core/pak.hpp"
//
struct Bass {
    
  ARM7TDMI cpu;
  Bus bus;
  Pak pak;

  Bass();
};

#pragma once
#include "core/mmu.hpp"

namespace Bass {

  struct Instance {
    Instance() { mmu.bus = &bus; };

    Bus bus;
    MMU mmu;
  };
}  // namespace Bass

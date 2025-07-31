#include "registers.hpp"

#include <cassert>

#include "spdlog/fmt/bundled/core.h"
// #include "instructions/arm.hpp"

u32 Registers::get_spsr(BANK_MODE m) {
  switch (m) {
    case USER:
    case SYSTEM: {
      // fmt::println("fetching spsr of SYSTEM/USER: {:#010x}", CPSR.value);
      return CPSR.value;
    }
    case FIQ: {
      // fmt::println("fetching spsr of FIQ: {:#010x}", SPSR_fiq);
      return SPSR_fiq;
    }
    case IRQ: {
      // fmt::println("fetching spsr of IRQ: {:#010x}", SPSR_irq);
      return SPSR_irq;
    }
    case SUPERVISOR: {
      // fmt::println("fetching spsr of SVC: {:#010x}", SPSR_svc);
      return SPSR_svc;
    }
    case ABORT: {
      // fmt::println("fetching spsr of ABT: {:#010x}", SPSR_abt);
      return SPSR_abt;
    }
    case UNDEFINED: {
      // fmt::println("fetching spsr of UND: {:#010x}", SPSR_und);
      return SPSR_und;
    }
    default: assert(0);
  }
}
std::string Registers::get_mode_string(BANK_MODE bm) {
  switch (bm) {
    case USER: return "USER";
    case SYSTEM: return "SYSTEM";
    case FIQ: return "FIQ";
    case IRQ: return "IRQ";
    case SUPERVISOR: return "SVC";
    case ABORT: return "ABT";
    case UNDEFINED: return "UND";
    default: return fmt::format("invalid mode: {:#010x}", (u8)bm);
  }
}

u32& Registers::get_reg(u8 i) {
  assert(i < 16);

  if (i < 8) { return r[i]; }

  // fmt::println("{:#010x}", CPSR.MODE_BIT | 0);

  // fmt::println("{:#010x}", (CPSR.MODE_BIT | 0x10));

  switch ((CPSR.MODE_BIT | 0x10)) {
    case USER:
    case SYSTEM: {
      return r[i];
    }
    case FIQ: {
      if (i > 7 && i < 15) { return fiq_r[i]; }
      return r[i];
    }
    case SUPERVISOR: {
      if ((i < 13) || (i == 15)) { return r[i]; }
      return svc_r[i];
    }
    case ABORT: {
      if ((i < 13) || (i == 15)) { return r[i]; }
      return abt_r[i];
    }
    case IRQ: {
      if ((i < 13) || (i == 15)) { return r[i]; }
      return irq_r[i];
    }
    case UNDEFINED: {
      if ((i < 13) || (i == 15)) { return r[i]; }
      return und_r[i];
    }
    default: {
    if (i > 7 && i < 15) { return zero; }
      return r[i];
    }
  }

  assert(0);
}

// When we load a SPSR, we should first copy registers, then copy the actual
// SPSR value into CPSR (get mode bits from SPSR, then copy)

void Registers::load_spsr_to_cpsr() {
  switch (CPSR.MODE_BIT) {
    case USER:
    case SYSTEM: {
      return;
    }
    case FIQ: {
      CPSR.value = SPSR_fiq | 0x10;
      break;
    }
    case IRQ: {
      CPSR.value = SPSR_irq | 0x10;
      break;
    }
    case SUPERVISOR: {
      CPSR.value = SPSR_svc | 0x10;
      break;
    }
    case ABORT: {
      CPSR.value = SPSR_abt | 0x10;
      break;
    }
    case UNDEFINED: {
      CPSR.value = SPSR_und | 0x10;
      break;
    }
    default: {
      assert(0);
    }
  }
}
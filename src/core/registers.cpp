#include "registers.hpp"

#include <cassert>
#include "instructions/arm.hpp"

u32 Registers::get_spsr(BANK_MODE m) {
  switch (m) {
    case USER:
    case SYSTEM:
      return CPSR.value;
    case FIQ:
      return SPSR_fiq;
    case IRQ:
      return SPSR_irq;
    case SUPERVISOR:
      return SPSR_svc;
    case ABORT:
      return SPSR_abt;
    case UNDEFINED:
      return SPSR_und;
    default:
      assert(0);
  }
}

void Registers::copy(BANK_MODE new_mode) {
  BANK_MODE current_mode = CPSR.MODE_BITS;

  if (new_mode == CPSR.MODE_BITS) return;
  if ((new_mode == USER || new_mode == SYSTEM) &&
      (current_mode == USER || current_mode == SYSTEM)) {
    return;
  }
  // if(new_mode != FIQ || current_mode != FIQ) return;

  // SPDLOG_DEBUG("copying {} into active bank", mode_map.at(new_mode));

  // SPDLOG_DEBUG("new mode: {}", (u8)new_mode);

  switch (new_mode) {
    case USER:
    case SYSTEM: {
      // SPDLOG_DEBUG("user system path hit");
      switch (current_mode) {
        case SUPERVISOR: {
          // SVC -> System/User
          
          for (size_t i = 13; i < 15; i++) {
            svc_r[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = user_system_bank[i];
          }
          break;
        }
        case ABORT: {
          // ABT -> System/User
          
          for (size_t i = 13; i < 15; i++) {
            abt_r[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = user_system_bank[i];
          }

          break;
        }
        case UNDEFINED: {
          // UND -> System/User
          // SPDLOG_DEBUG("D");
          for (size_t i = 13; i < 15; i++) {
            und_r[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = user_system_bank[i];
          }

          break;
        }
        case FIQ: {
          // FIQ -> System/User
          for (size_t i = 8; i < 15; i++) {  // save fiq 8 to 14 to its bank
            fiq_r[i] = r[i];
          }

          for (size_t i = 8; i < 15;
               i++) {  // copy IRQ bank values to current bank
            r[i] = user_system_bank[i];
          }

          break;
        }
        case IRQ: {
          // IRQ -> System/User
          for (size_t i = 13; i < 15; i++) {
            irq_r[i] = r[i];
          }

          // load IRQ values into current bank
          for (size_t i = 13; i < 15; i++) {
            r[i] = user_system_bank[i];
          }
          break;
        }

        default: {
          SPDLOG_DEBUG("new mode: {}", (u8)new_mode);
          SPDLOG_DEBUG("current mode: {}", (u8)current_mode);
          assert(0);
        }
      }
      break;
    }

    case FIQ: {
      // assert(0);
      switch (current_mode) {
        case USER:
        case SYSTEM: {
          // User/System -> FIQ
          SPDLOG_DEBUG("path hit");
          // Save R8-R13 (shared by all modes except for FIQ (shared registers)
          for (size_t i = 8; i < 15; i++) {
            user_system_bank[i] = r[i];
          }

          // Write R8 to R14 FIQ to current bank
          for (size_t i = 8; i < 15; i++) {
            r[i] = fiq_r[i];
          }
          return;
          break;
        }

        case SUPERVISOR: {
          // SVC -> FIQ

          // Save Current Bank Register
          for (size_t i = 13; i < 15; i++) {
            svc_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {
            user_system_bank[i] = r[i];
          }

          // Write R8 to R14 FIQ to current bank
          for (size_t i = 8; i < 15; i++) {
            r[i] = fiq_r[i];
          }
          break;
        }
        case ABORT: {
          // ABT -> FIQ

          // Save Current Bank Register
          for (size_t i = 13; i < 15; i++) {
            abt_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {
            user_system_bank[i] = r[i];
          }

          // Write R8 to R14 FIQ to current bank
          for (size_t i = 8; i < 15; i++) {
            r[i] = fiq_r[i];
          }
          break;
        }
        case UNDEFINED: {
          // UND -> FIQ

          // Save Current Bank Register
          for (size_t i = 13; i < 15; i++) {
            und_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {
            user_system_bank[i] = r[i];
          }

          // Write R8 to R14 FIQ to current bank
          for (size_t i = 8; i < 15; i++) {
            r[i] = fiq_r[i];
          }
          break;
        }
        case IRQ: {
          // IRQ -> FIQ

          // Save Current Bank Register
          for (size_t i = 13; i < 15; i++) {
            irq_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {
            user_system_bank[i] = r[i];
          }

          // Write R8 to R14 FIQ to current bank
          for (size_t i = 8; i < 15; i++) {
            r[i] = fiq_r[i];
          }
          break;
        }

        default: {
          assert(0);
        }
      }
      break;
    }
    case IRQ: {
      switch (current_mode) {
        case USER:
        case SYSTEM: {
          // System/User -> IRQ
          // user_system_bank[13] = r[13];
          // user_system_bank[14] = r[14];

          for (size_t i = 13; i < 15; i++) {
            user_system_bank[i] = r[i];
          }

          // load IRQ values into current bank

          // r[13] = irq_r[13];
          // r[14] = irq_r[14];

          for (size_t i = 13; i < 15; i++) {
            r[i] = irq_r[i];
          }
          break;
        }
        case SUPERVISOR: {
          // SVC -> IRQ
          
          for (size_t i = 13; i < 15; i++) {
            svc_r[i] = r[i];
          }


          for (size_t i = 13; i < 15; i++) {
            r[i] = irq_r[i];
          }
          break;
        }
        case ABORT: {
          // ABT -> IRQ
          // abt_r[13] = r[13];
          // abt_r[14] = r[14];

          for (size_t i = 13; i < 15; i++) {
            abt_r[i] = r[i];
          }



          for (size_t i = 13; i < 15; i++) {
            r[i] = irq_r[i];
          }

          break;
        }
        case UNDEFINED: {
          // UND -> IRQ

          for (size_t i = 13; i < 15; i++) {
            und_r[i] = r[i];
          }

          // load IRQ values into current bank
          r[13] = irq_r[13];
          r[14] = irq_r[14];

          for (size_t i = 13; i < 15; i++) {
            r[i] = irq_r[i];
          }

          break;
        }
        case FIQ: {
          // FIQ -> IRQ

          fiq_r[8]  = r[8];
          fiq_r[9]  = r[9];
          fiq_r[10] = r[10];
          fiq_r[11] = r[11];
          fiq_r[12] = r[12];
          fiq_r[13] = r[13];
          fiq_r[14] = r[14];

          for (size_t i = 8; i < 15; i++) {  // save fiq 8 to 14 to its bank
            fiq_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {  // r8 to r12 are shared banks
            r[i] = user_system_bank[i];
          }

          for (size_t i = 13; i < 15;
               i++) {  // copy IRQ bank values to current bank
            r[i] = irq_r[i];
          }

          break;
        }

        default: {
          assert(0);
        }
      }
      break;
    }
    case SUPERVISOR: {
      switch (current_mode) {
        case USER:
        case SYSTEM: {
          //  User/System -> SVC

          for (size_t i = 13; i < 15; i++) {
            user_system_bank[i] = r[i];
          }

          // load User/System values into current bank
          for (size_t i = 13; i < 15; i++) {
            r[i] = svc_r[i];
          }
          break;
        }
        case ABORT: {
          // ABT -> SVC

          for (size_t i = 13; i < 15; i++) {
            abt_r[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = svc_r[i];
          }

          break;
        }
        case UNDEFINED: {
          // UND -> SVC

          for (size_t i = 13; i < 15; i++) {
            und_r[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = svc_r[i];
          }

          break;
        }
        case FIQ: {
          // FIQ -> SVC

          fiq_r[8]  = r[8];
          fiq_r[9]  = r[9];
          fiq_r[10] = r[10];
          fiq_r[11] = r[11];
          fiq_r[12] = r[12];
          fiq_r[13] = r[13];
          fiq_r[14] = r[14];

          for (size_t i = 8; i < 15; i++) {  // save fiq 8 to 14 to its bank
            fiq_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {  // r8 to r12 are shared banks
            r[i] = user_system_bank[i];
          }

          for (size_t i = 13; i < 15;
               i++) {  // copy SVC bank values to current bank
            r[i] = svc_r[i];
          }

          break;
        }
        case IRQ: {
          // IRQ -> SVC
          for (size_t i = 13; i < 15; i++) {
            irq_r[i] = r[i];
          }

          // load IRQ values into current bank
          for (size_t i = 13; i < 15; i++) {
            r[i] = svc_r[i];
          }
          break;
        }
        default: {
          assert(0);
        }
      }
      break;
    }
    case ABORT: {
      switch (current_mode) {
        case USER:
        case SYSTEM: {
          //  User/System -> ABT

          for (size_t i = 13; i < 15; i++) {
            user_system_bank[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = abt_r[i];
          }
          break;
        }
        case UNDEFINED: {
          // UND -> ABT

          for (size_t i = 13; i < 15; i++) {
            und_r[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = abt_r[i];
          }

          break;
        }
        case FIQ: {
          // FIQ -> ABT

          fiq_r[8]  = r[8];
          fiq_r[9]  = r[9];
          fiq_r[10] = r[10];
          fiq_r[11] = r[11];
          fiq_r[12] = r[12];
          fiq_r[13] = r[13];
          fiq_r[14] = r[14];

          for (size_t i = 8; i < 15; i++) {  // save fiq 8 to 14 to its bank
            fiq_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {  // r8 to r12 are shared banks
            r[i] = user_system_bank[i];
          }

          for (size_t i = 13; i < 15;
               i++) {  // copy SVC bank values to current bank
            r[i] = abt_r[i];
          }

          break;
        }
        case SUPERVISOR: {
          // SVC -> ABT
          for (size_t i = 13; i < 15; i++) {
            svc_r[i] = r[i];
          }

          // load IRQ values into current bank
          for (size_t i = 13; i < 15; i++) {
            r[i] = abt_r[i];
          }
          break;
        }
        case IRQ: {
          // IRQ -> ABT
          for (size_t i = 13; i < 15; i++) {
            irq_r[i] = r[i];
          }

          // load IRQ values into current bank
          for (size_t i = 13; i < 15; i++) {
            r[i] = abt_r[i];
          }
          break;
        }
        default: {
          assert(0);
        }
      }
      break;
    }
    case UNDEFINED: {
      switch (current_mode) {
        case USER:
        case SYSTEM: {
          //  User/System -> UNDR

          for (size_t i = 13; i < 15; i++) {
            user_system_bank[i] = r[i];
          }

          for (size_t i = 13; i < 15; i++) {
            r[i] = und_r[i];
          }
          break;
        }
        case ABORT: {
          // ABT -> UND
          for (size_t i = 13; i < 15; i++) {
            abt_r[i] = r[i];
          }
          for (size_t i = 13; i < 15; i++) {
            r[i] = und_r[i];
          }
          break;
        }
        case FIQ: {
          // FIQ -> UND

          fiq_r[8]  = r[8];
          fiq_r[9]  = r[9];
          fiq_r[10] = r[10];
          fiq_r[11] = r[11];
          fiq_r[12] = r[12];
          fiq_r[13] = r[13];
          fiq_r[14] = r[14];

          for (size_t i = 8; i < 15; i++) {  // save fiq 8 to 14 to its bank
            fiq_r[i] = r[i];
          }

          for (size_t i = 8; i < 13; i++) {  // r8 to r12 are shared banks
            r[i] = user_system_bank[i];
          }

          for (size_t i = 13; i < 15;
               i++) {  // copy UND bank values to current bank
            r[i] = und_r[i];
          }

          break;
        }
        case SUPERVISOR: {
          // SVC -> UND
          for (size_t i = 13; i < 15; i++) {
            svc_r[i] = r[i];
          }

          // load IRQ values into current bank
          for (size_t i = 13; i < 15; i++) {
            r[i] = und_r[i];
          }
          break;
        }
        case IRQ: {
          // IRQ -> UND
          for (size_t i = 13; i < 15; i++) {
            irq_r[i] = r[i];
          }

          // load IRQ values into current bank
          for (size_t i = 13; i < 15; i++) {
            r[i] = und_r[i];
          }
          break;
        }
        default: {
          assert(0);
        } break;
      }
      break;
    }

    default: {
      fmt::println("tried to copy from bank: {:#x}", (u32)new_mode);
      assert(0);
    }
  }
}

// When we load an SPSR, we should first copy registers, then copy the actual
// SPSR value into CPSR (get mode bits from SPSR, then copy)

void Registers::load_spsr_to_cpsr() {
  BANK_MODE l = CPSR.MODE_BITS;

  BANK_MODE new_mode = ((BANK_MODE)((get_spsr(l) & 0x1f) | 0x10));

  fmt::println("new mode: {:#x}", (u8)new_mode);
  copy(new_mode);
  switch (CPSR.MODE_BITS) {
    case USER:
    case SYSTEM: {
      return;
    }
    case FIQ: {
      CPSR.value = SPSR_fiq;
      break;
    }
    case IRQ: {
      CPSR.value = SPSR_irq;
      break;
    }
    case SUPERVISOR: {
      CPSR.value = SPSR_svc;
      break;
    }
    case ABORT: {
      CPSR.value = SPSR_abt;
      break;
    }
    case UNDEFINED: {
      CPSR.value = SPSR_und;
      break;
    }
    default:{
      assert(0);}
  }
}
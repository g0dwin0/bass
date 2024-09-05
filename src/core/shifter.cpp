#include <stdexcept>

#include "core/cpu.hpp"

u32 ARM7TDMI::shift(SHIFT_MODE mode, u64 value, u64 amount, bool amount_from_reg, bool never_rrx) {
  switch (mode) {
    case LSL: {
      SPDLOG_DEBUG("Value: {}", value);
      SPDLOG_DEBUG("Shift Amount: {}", amount);
      if (amount_from_reg) {
        if (amount == 0) { return value; }

        if (amount == 32) {
          bool set = (value & 1);
          set ? set_carry() : reset_carry();
          return 0;
        }

        if (amount > 32) {
          reset_carry();
          return 0;
        }
      }

      if (amount == 0) {
        SPDLOG_DEBUG("[LSL] shift amount is 0, returning inital value of {:#010x}", value);
        return value;
      }

      u64 s_m = (value << amount);
      (value & (1 << (32 - amount))) != 0 ? set_carry() : reset_carry();

      SPDLOG_DEBUG("performed LSL with value: {:#010x}, by LSL'd by amount: {} result: {:#010x}", value, amount, s_m);
      return s_m % 0x100000000;
    }
    case LSR: {
      if (amount == 0 && !amount_from_reg) amount = 32;

      if (amount == 0) return value;

      u64 s_m = (value >> amount);
      // SPDLOG_DEBUG(
      //     "performed LSR with value: {:#010x}, by LSR'd by amount: {} result: {:#010x}",
      //     value, amount, s_m);

      (value & (1 << (amount - 1))) != 0 ? set_carry() : reset_carry();

      return s_m;
    }
    case ASR: {
      if (amount == 0 && amount_from_reg) return value;
      if (amount == 0) {
        u32 ret_val = (value & 0x80000000) ? 0xFFFFFFFF : 0x00000000;

        (value & (1 << (amount - 1))) != 0 ? set_carry() : reset_carry();

        return ret_val;
      }

      s32 m = (static_cast<s32>(value)) >> amount;

      SPDLOG_DEBUG("performed ASR with value: {:#010x}, by ASR'd by amount: {} result: {:#010x}", value, amount, m);

      (value & (1 << (amount - 1))) != 0 ? set_carry() : reset_carry();

      return static_cast<u32>(m);
    }
    case ROR: {
      if (amount == 0 && amount_from_reg) return value;

      bool is_rrx = false;
      if (amount == 0 && !never_rrx) {
        // SPDLOG_DEBUG("IS RRX!");
        is_rrx = true;
        amount = 1;
      }
      if (amount == 0) return value;
      if (amount > 32) amount %= 32;

      u32 r = std::rotr(static_cast<u32>(value), amount);

      SPDLOG_DEBUG("performed ROR with value: {:#010x}, by ROR'd by amount: {} result: {:#010x}", value, amount, r);

      if (is_rrx) {  // RRX

        r &= ~(1 << 31);

        r |= (regs.CPSR.CARRY_FLAG << 31);

        (value & 1) == 1 ? set_carry() : reset_carry();
        return r;
      }

      (value & (1 << (amount - 1))) != 0 ? set_carry() : reset_carry();

      return r;
    }
    default: {
      throw std::runtime_error("invalid shift mode");
    }
  }
}

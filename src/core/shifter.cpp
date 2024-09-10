#include <stdexcept>

#include "core/cpu.hpp"

u32 asr(u32 x, u32 shift) {
  u32 signBit = x & 0x80000000;

  u32 result = x >> shift;

  if (signBit) {
    u32 mask = (1 << shift) - 1;
    mask <<= (32 - shift);

    result |= mask;
  }

  return result;
}
u32 ARM7TDMI::shift(SHIFT_MODE mode, u64 value, u64 amount, bool special, bool never_rrx, bool affect_flags) {
  switch (mode) {
    case LSL: {
      if (amount == 0) {
        SPDLOG_DEBUG("[LSL] shift amount is 0, returning inital value of {:#010x}", value);
        return value;
      }

      // fmt::println("Value: {:#010x}", value);
      // fmt::println("Shift Amount: {:#010x}", amount);

      if (!special) {
        if (amount >= 32) {
          if (amount == 32) {
            if (affect_flags) { (value & 1) != 0 ? set_carry() : reset_carry(); }
          } else {
            if (affect_flags) { reset_carry(); }
          }
          return 0;
        }
      }

      u32 s_m = (value << amount);
      // fmt::println("s_m = {:#010x}", s_m);
      (value & (1 << (32 - amount))) != 0 ? set_carry() : reset_carry();

      SPDLOG_DEBUG("performed LSL with value: {:#010x}, by LSL'd by amount: {} result: {:#010x}", value, amount, s_m);
      return s_m;
    }
    case LSR: {
      if (special) {
        if (amount == 0) {
          if (affect_flags) { (value & (1 << 31)) != 0 ? set_carry() : reset_carry(); }
          return 0;
        }
      } else {
        if (amount == 0) { return value; }
        if (amount >= 32) {
          if (amount == 32) {
            if (affect_flags) { (value & (1 << 31)) != 0 ? set_carry() : reset_carry(); }
          } else {
            if (affect_flags) { reset_carry(); }
          }
          return 0;
        }
      }

      u32 s_m = (value >> amount);

      if (affect_flags) { (value & (1 << (amount - 1))) != 0 ? set_carry() : reset_carry(); }

      return s_m;
    }
    case ASR: {
      if (amount == 0 && special) {
        u32 ret_val = (value & 1 << 31) ? 0xFFFFFFFF : 0x00000000;

        if (affect_flags) { (value & (1 << 31)) != 0 ? set_carry() : reset_carry(); }

        return ret_val;
      }

      if(amount >= 32) {
        u32 ret_val = (value & 1 << 31) ? 0xFFFFFFFF : 0x00000000;

        if (affect_flags) { (value & (1 << 31)) != 0 ? set_carry() : reset_carry(); }

        return ret_val;
      }
      
      if (amount == 0) return value;

      u32 m = asr(value, amount);

      SPDLOG_DEBUG("performed ASR with value: {:#010x}, by ASR'd by amount: {} result: {:#010x}", value, amount, m);

      if (affect_flags) { (value & (1 << (amount - 1))) != 0 ? set_carry() : reset_carry(); }

      return m;
    }
    case ROR: {
      if (amount == 0 && !special) return value;

      bool is_rrx = false;
      if (amount == 0 && special && !never_rrx) {
        SPDLOG_DEBUG("IS RRX!");
        is_rrx = true;
        amount = 1;
      }

      if (amount > 32) amount %= 32;

      u32 r = std::rotr(static_cast<u32>(value), amount);

      SPDLOG_DEBUG("performed ROR with value: {:#010x}, by ROR'd by amount: {} result: {:#010x}", value, amount, r);

      if (is_rrx) {  // RRX

        r &= ~(1 << 31);

        r |= (regs.CPSR.CARRY_FLAG << 31);

        if (affect_flags) { (value & 1) == 1 ? set_carry() : reset_carry(); }
        return r;
      }

      if (affect_flags) { (value & (1 << (amount - 1))) != 0 ? set_carry() : reset_carry(); }

      return r;
    }
    default: {
      throw std::runtime_error("invalid shift mode");
    }
  }
}

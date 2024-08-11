#include <limits>
#include <stdexcept>

#include "core/cpu.hpp"
#include "utils/bitwise.hpp"

std::pair<u32, bool> ARM7TDMI::shift(SHIFT_MODE mode, u32 value, u8 amount) {
  switch (mode) {
    case LSL: {
      u64 s_m = value << amount;
      return {value << amount, s_m > std::numeric_limits<u32>::max()};
    }
    case LSR: {
      // u64 s_m = value >> amount;
      return {value >> amount, true};
    }
    case ASR: {
      // throw std::runtime_error("cannot perform ASR in barrel shifter");

      return {0, 0};
    }
    case ROR: {
      return {rotateRight(value, amount * 2), false};
    }
    default: {
      return {0, 0};
    }
  }
}

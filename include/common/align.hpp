#pragma once
#include "../enums.hpp"
[[nodiscard]] inline static constexpr u32 align(const u32 value, const ALIGN_TO align_to) {
  if (align_to == WORD) {
    return value & ~3;
  } else {
    return value & ~1;
  }
}
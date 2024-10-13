#pragma once
#include <cassert>
#include <type_traits>
#include "defs.hpp"

template <typename T>
void set_byte(T& r, u8 byte_idx, u8 v) {
  static_assert(std::is_arithmetic<T>::value, "Invalid type");
  assert(sizeof(T) > byte_idx);
  r &= ~(0xFF << byte_idx * 8);
  r |= (v << byte_idx * 8);
};

template <typename T>
u8 read_byte(T& v, u8 byte_idx) {
  static_assert(std::is_arithmetic<T>::value, "Invalid type");
  assert(sizeof(T) > byte_idx);

  return v >> (byte_idx * 8) & 0xFF;
};


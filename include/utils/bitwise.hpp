#pragma once
#include "common.hpp"

inline u32 rotateRight(u8 value, u8 numBits) {
  numBits %= 32;

  return (value >> numBits) | (value << (32 - numBits));
}

inline u32 rotateLeft(u8 value, u8 numBits) {
    numBits %= 32;
  return (value << numBits) | (value >> (32 - numBits));
}
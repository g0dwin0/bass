#pragma once
#include "common.hpp"

inline u32 rotateImmediate(u8 value, u8 numBits, bool rotateLeft) {
    numBits %= 32;

    if (rotateLeft) {
        // Rotate left
        return (value << numBits) | (value >> (32 - numBits));
    } else {
        // Rotate right
        return (value >> numBits) | (value << (32 - numBits));
    }
}
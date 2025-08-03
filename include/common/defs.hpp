#pragma once
#include <cstdint>
#include <limits>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

static constexpr auto U8_MAX = std::numeric_limits<u8>::max();
static constexpr auto U16_MAX = std::numeric_limits<u16>::max();
static constexpr auto U32_MAX = std::numeric_limits<u32>::max();
static constexpr auto U64_MAX = std::numeric_limits<u64>::max();
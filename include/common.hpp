#pragma once
#define FMT_HEADER_ONLY
#define CONCAT(x, y) x ## y
#define EXPAND(x, y) CONCAT(x, y)
#define RESERVED EXPAND(reserved, __LINE__)

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstdlib>



typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef int8_t     s8;
typedef int16_t    s16;
typedef int32_t    s32;
typedef int64_t    s64;


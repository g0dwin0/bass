#pragma once

#include <climits>
#include <limits>
#define FMT_HEADER_ONLY
#define CONCAT(x, y) x##y
#define EXPAND(x, y) CONCAT(x, y)
#define RESERVED EXPAND(reserved, __LINE__)

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/fmt/fmt.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

// Type Defs
using u8   = uint8_t;
using u16  = uint16_t;
using u32  = uint32_t;
using u64  = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

// Max Values
#define U8_MAX std::numeric_limits<u8>::max()
#define U16_MAX std::numeric_limits<u16>::max()
#define U32_MAX std::numeric_limits<u32>::max()
#define U64_MAX std::numeric_limits<u64>::max()

inline std::vector<u8> read_file(std::string_view filename) {
  std::ifstream file(filename.data(), std::ios::binary);

  if (!file.good()) {
    fmt::println("io: failed to load file: {}", filename);
    // assert(0 & "failed to load file");
    assert(0);
  }

  file.unsetf(std::ios::skipws);
  std::streampos fileSize;
  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<u8> vec;
  vec.reserve(fileSize);

  vec.insert(vec.begin(), std::istream_iterator<u8>(file), std::istream_iterator<u8>());

  return vec;
}
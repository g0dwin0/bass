#pragma once
#include "spdlog/fmt/fmt.h"
#define FMT_HEADER_ONLY
#define CONCAT(x, y) x##y
#define EXPAND(x, y) CONCAT(x, y)
#define RESERVED EXPAND(reserved, __LINE__)

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

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
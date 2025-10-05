#pragma once

#include <climits>
#define CONCAT(x, y) x##y
#define EXPAND(x, y) CONCAT(x, y)
#define RESERVED EXPAND(reserved, __LINE__)

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

#include "common/defs.hpp"
#include "spdlog/spdlog.h"

inline std::vector<u8> read_file(std::string filename) {
  std::ifstream file(filename.data(), std::ios::binary);

  if (!file.good()) {
    fmt::println("failed to load file: {}", filename);
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

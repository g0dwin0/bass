#pragma once
#include <fstream>
#include <vector>
#include <iterator>
#include "common.hpp"
#include "fmt/core.h"

inline std::vector<u8> read_file(std::string_view filename) {
  std::ifstream file(filename.data(), std::ios::binary);

  if (!file.good()) {
    throw std::runtime_error(
        fmt::format("io: failed to load file: {}", filename));
  }

  file.unsetf(std::ios::skipws);
  std::streampos fileSize;
  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<u8> vec;
  vec.reserve(fileSize);

  vec.insert(vec.begin(), std::istream_iterator<u8>(file),
             std::istream_iterator<u8>());

  return vec;
}
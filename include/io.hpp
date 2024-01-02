#pragma once
#include <fstream>
#include <vector>

#include "common.hpp"

struct File {
  std::vector<u8> data;
  u64 file_size;
  std::string path;
};

inline File read_file(std::string filename) {
  std::ifstream file(filename, std::ios::binary);

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

  fmt::println("file size: {:d} ({:#04x})", static_cast<u64>(fileSize), static_cast<u64>(fileSize));

  vec.insert(vec.begin(), std::istream_iterator<u8>(file),
             std::istream_iterator<u8>());

  return File{vec, static_cast<u64>(fileSize), filename};
}
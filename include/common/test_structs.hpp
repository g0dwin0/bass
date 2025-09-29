#pragma once
#include <string>
#include <unordered_map>

#include "defs.hpp"
#include "spdlog/fmt/bundled/core.h"

enum SST_ACCESS_TYPE : u32 { INSTRUCTION_READ, GENERAL_READ, WRITE };

const std::unordered_map<SST_ACCESS_TYPE, std::string> TransactionType = {
    {INSTRUCTION_READ, "INSTRUCTION READ"},
    {    GENERAL_READ,     "GENERAL READ"},
    {           WRITE,            "WRITE"}
};

struct Transaction {
  u8 id = 0;
  SST_ACCESS_TYPE kind;
  u8 size;
  u32 addr;
  u32 data;

  u32 opcode;
  u32 base_addr;

  bool accessed = false;

  void print() const {
    fmt::println("===== TRANSACTION {} ======", id);
    fmt::println("kind: {}", TransactionType.at(kind));
    fmt::println("size: {:#010x}", size);
    fmt::println("addr: {:#010x}", addr);
    fmt::println("data: {:#010x}", data);
    fmt::println("opcode: {:#010x}", opcode);
    fmt::println("base_addr: {:#010x}", base_addr);
    fmt::println("=======================");
  }
};
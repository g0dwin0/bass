#pragma once

#include "common.hpp"
#include "registers.hpp"
// struct InstructionInfo;
// #include "arm.hpp"
struct ARM7TDMI;
#include "cpu.hpp"

struct InstructionInfo {
  u32 opcode = 0;
  u32 loc    = 0;

  u32 op2;
  u32 offset;

  u8 Rd : 4;
  u8 Rn : 4;
  u8 Rs : 4;
  u8 Rm : 4;

  u8 A  : 1;
  u8 P  : 1;
  u8 U  : 1;
  u8 B  : 1;
  u8 W  : 1;
  u8 L  : 1;
  u8 I  : 1;
  u8 S  : 1;
  u8 H  : 1;

  Condition condition;

  std::string mnemonic = "";

  void (*func_ptr)(ARM7TDMI& c, InstructionInfo& instr) = NULL;
  void print_params() {
    fmt::println("opcode: {:#010x}", +opcode);
    fmt::println("Rd: {:#010x}", +Rd);
    fmt::println("Rn: {:#010x}", +Rn);
    fmt::println("Rs: {:#010x}", +Rs);
    fmt::println("Rm: {:#010x}", +Rm);
    fmt::println("A: {:#x}\n", +A);
    fmt::println("P: {:#x}", +P);
    fmt::println("U: {:#x}", +U);
    fmt::println("S: {:#x}", +S);
    fmt::println("W: {:#x}", +W);
    fmt::println("L: {:#x}\n", +L);
    fmt::println("B: {:#x}", +B);
    fmt::println("I: {:#x}", +I);
    fmt::println("H: {:#x}", +H);
  }
};
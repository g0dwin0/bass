#pragma once

#include "bus.hpp"
#include "common.hpp"
#include "registers.hpp"
struct ARM7TDMI;
#include "cpu.hpp"

struct InstructionInfo {
  u32 opcode = 0;
  // InstructionType instructionType = InstructionType::UNDEFINED;

  u32 op2    = 0;
  u32 offset = 0;

  u8 Rd = 0;
  u8 Rn = 0;
  u8 Rs = 0;
  u8 Rm = 0;

  u8 A = 0;
  u8 P = 0;
  u8 U = 0;
  u8 B = 0;
  u8 W = 0;
  u8 L = 0;
  u8 I = 0;
  u8 S = 0;
  u8 H = 0;

  u8 PC_BIT = 0;
  u8 LR_BIT = 0;

  u8 shift_amount = 0;
  u8 shift_type   = 0;

  u32 imm   = 0;
  u8 rotate = 0;

  bool R = false;

  Condition condition = Condition::AL;

  // std::string mnemonic = "";

  bool empty       = false;
  bool pc_relative = false;

  void (*func_ptr)(ARM7TDMI& c, InstructionInfo& instr) = NULL;
  void print_params() {
    fmt::println("opcode: {:#010x} ", +opcode);
    // fmt::println("loc: {:#010x}", +loc);
    fmt::println("Rd: {:#010x} {}", +Rd, +Rd);
    fmt::println("Rn: {:#010x} {}", +Rn, +Rn);
    fmt::println("Rs: {:#010x} {}", +Rs, +Rs);
    fmt::println("Rm: {:#010x} {}", +Rm, +Rm);
    fmt::println("A: {:#x}", +A);
    fmt::println("P: {:#x}", +P);
    fmt::println("U: {:#x}", +U);
    fmt::println("S: {:#x}", +S);
    fmt::println("W: {:#x}", +W);
    fmt::println("L: {:#x}", +L);
    fmt::println("B: {:#x}", +B);
    fmt::println("I: {:#x}", +I);
    fmt::println("H: {:#x}", +H);
    fmt::println("op2: {:#x}", +op2);
    fmt::println("imm: {:#x}", imm);
    fmt::println("rotate: {:#x}\n\n", rotate);
  }
};
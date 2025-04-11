#pragma once

#include "bus.hpp"
#include "common.hpp"
#include "registers.hpp"
struct ARM7TDMI;
#include "cpu.hpp"

struct instruction_info {
  u32 opcode = 0;
  u32 loc    = 0;

  u32 op2    = 0;
  u32 offset = 0;

  u8 Rd     : 4 = 0;
  u8 Rn     : 4 = 0;
  u8 Rs     : 4 = 0;
  u8 Rm     : 4 = 0;

  u8 A      : 1 = 0;
  u8 P      : 1 = 0;
  u8 U      : 1 = 0;
  u8 B      : 1 = 0;
  u8 W      : 1 = 0;
  u8 L      : 1 = 0;
  u8 I      : 1 = 0;
  u8 S      : 1 = 0;
  u8 H      : 1 = 0;

  u8 PC_BIT : 1 = 0;
  u8 LR_BIT : 1 = 0;

  u8 shift_amount = 0;
  u8 shift_type   = 0;

  u32 imm   = 0;
  u8 rotate = 0;

  bool shift_value_is_register = false;

  Condition condition = Condition::AL;

  std::string mnemonic = "";

  bool empty       = false;
  bool pc_relative = false;

  void (*func_ptr)(ARM7TDMI& c, instruction_info& instr) = NULL;
  void print_params() {
    // SPDLOG_DEBUG("opcode: {:#010x} ", +opcode);
    // SPDLOG_DEBUG("loc: {:#010x}", +loc);
    // // SPDLOG_DEBUG("Rd: {:#010x} {}", +Rd, +Rd);
    // // SPDLOG_DEBUG("Rn: {:#010x} {}", +Rn, +Rn);
    // // SPDLOG_DEBUG("Rs: {:#010x} {}", +Rs, +Rs);
    // // SPDLOG_DEBUG("Rm: {:#010x} {}", +Rm, +Rm);
    // // SPDLOG_DEBUG("A: {:#x}\n", +A);
    // // SPDLOG_DEBUG("P: {:#x}", +P);
    // // SPDLOG_DEBUG("U: {:#x}", +U);
    // // SPDLOG_DEBUG("S: {:#x}", +S);
    // // SPDLOG_DEBUG("W: {:#x}", +W);
    // // SPDLOG_DEBUG("L: {:#x}\n", +L);
    // // SPDLOG_DEBUG("B: {:#x}", +B);
    // // SPDLOG_DEBUG("I: {:#x}", +I);
    // // SPDLOG_DEBUG("H: {:#x}", +H);
    // SPDLOG_DEBUG("op2: {:#x}", +op2);
  }
};
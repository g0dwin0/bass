#pragma once

#include "bus.hpp"
#include "common.hpp"
#include "registers.hpp"
#include "instructions/instruction.hpp"

enum DATA_PROCESSING_OPS {
  AND,
  EOR,
  SUB,
  RSB,
  ADD,
  ADC,
  SBC,
  RSC,
  TST,
  TEQ,
  CMP,
  CMN,
  ORR,
  MOV,
  BIC,
  MVN
};

struct ARM7TDMI {
  ARM7TDMI();
  Registers regs;

  enum CPU_MODE { ARM, THUMB } mode;

  Bus* bus = nullptr;

  struct Pipeline {
    InstructionInfo fetch   = {};
    InstructionInfo decode  = {};
    InstructionInfo execute = {};
  } pipeline;

  [[nodiscard]] InstructionInfo fetch(const u32 address);
  [[nodiscard]] InstructionInfo decode(InstructionInfo& op);
  
  void flush_pipeline();

  void step();
  void print_pipeline();
  void execute(InstructionInfo& instr);

  bool check_condition(InstructionInfo& i);
};
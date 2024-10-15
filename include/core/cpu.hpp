#pragma once

#include "bus.hpp"
#include "common.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"

enum DATA_PROCESSING_OPS {
  AND = 0x0,
  EOR = 0x1,
  SUB = 0x2,
  RSB = 0x3,
  ADD = 0x4,
  ADC = 0x5,
  SBC = 0x6,
  RSC = 0x7,
  TST = 0x8,
  TEQ = 0x9,
  CMP = 0xA,
  CMN = 0xB,
  ORR = 0xC,
  MOV = 0xD,
  BIC = 0xE,
  MVN = 0xF,
};

enum BOUNDARY { HALFWORD, WORD };
struct ARM7TDMI {
  ARM7TDMI();
  Registers regs;

  enum SHIFT_MODE : u8 {
    LSL,
    LSR,
    ASR,
    ROR,
  };
  // enum SHIFT_TYPE { REGISTER_SHIFT, IMMEDIATE_SHIFT };
  Bus* bus = nullptr;

  struct Pipeline {
    instruction_info fetch   = {};
    instruction_info decode  = {};
    instruction_info execute = {};
  } pipeline;

  [[nodiscard]] instruction_info fetch(const u32 address);
  [[nodiscard]] instruction_info decode(instruction_info& op);

  [[nodiscard]] instruction_info arm_decode(instruction_info& op);
  [[nodiscard]] instruction_info thumb_decode(instruction_info& op);

  void flush_pipeline();

  u16 step();
  void print_pipeline();
  inline void execute(instruction_info& instr);

  inline bool check_condition(instruction_info& i);
  [[nodiscard]] std::string get_addressing_mode_string(const instruction_info& instr);

  void print_registers();

  [[nodiscard]] std::string_view get_shift_type_string(u8 shift_type);
  [[nodiscard]] u32 shift(SHIFT_MODE mode, u64 value, u64 amount, bool special, bool never_rrx = false, bool affect_flags = true);

  [[nodiscard]] bool interrupt_queued();
  
  inline void set_zero() { regs.CPSR.ZERO_FLAG = 1; };
  inline void reset_zero() { regs.CPSR.ZERO_FLAG = 0; };

  inline void set_negative() { regs.CPSR.SIGN_FLAG = 1; };
  inline void reset_negative() { regs.CPSR.SIGN_FLAG = 0; };

  inline void set_carry() { regs.CPSR.CARRY_FLAG = 1; };
  inline void reset_carry() { regs.CPSR.CARRY_FLAG = 0; };

  inline void set_overflow() { regs.CPSR.OVERFLOW_FLAG = 1; };
  inline void reset_overflow() { regs.CPSR.OVERFLOW_FLAG = 0; };

  [[nodiscard]] u32 handle_shifts(instruction_info& instr, bool affect_flags = true);
  [[nodiscard]] u32 align_address(u32 address, BOUNDARY b);
};
#pragma once

#include "bus.hpp"
#include "common.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"

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
enum DATA_PROCESSING_MODE { IMMEDIATE, REGISTER_SHIFT, IMMEDIATE_SHIFT };
enum BOUNDARY { HALFWORD, WORD };
struct ARM7TDMI {
  ARM7TDMI();
  Registers regs;
  u16 cycles_consumed;

  enum SHIFT_MODE {
    LSL,
    LSR,
    ASR,
    ROR,
  };
  enum SHIFT_TYPE { REGISTER_SHIFT, IMMEDIATE_SHIFT };
  Bus* bus = nullptr;

  struct Pipeline {
    InstructionInfo fetch   = {};
    InstructionInfo decode  = {};
    InstructionInfo execute = {};
  } pipeline;

  [[nodiscard]] InstructionInfo fetch(const u32 address);
  [[nodiscard]] InstructionInfo decode(InstructionInfo& op);

  void flush_pipeline();
  // void refill_pipeline();

  u16 step();
  void print_pipeline();
  void execute(InstructionInfo& instr);

  bool check_condition(InstructionInfo& i);
  [[nodiscard]] std::string get_addressing_mode_string(
      const InstructionInfo& instr);

  void print_registers();

  void handle_data_processing(DATA_PROCESSING_MODE m, InstructionInfo& instr);
  [[nodiscard]] std::string_view get_shift_type_string(u8 shift_type);
  [[nodiscard]] u32 shift(SHIFT_MODE mode, u64 value, u64 amount,
                          bool amount_from_reg, bool never_rrx = false);

  inline void set_zero() { regs.CPSR.ZERO_FLAG = 1; };
  inline void reset_zero() { regs.CPSR.ZERO_FLAG = 0; };

  inline void set_negative() { regs.CPSR.SIGN_FLAG = 1; };
  inline void reset_negative() { regs.CPSR.SIGN_FLAG = 0; };

  inline void set_carry() { regs.CPSR.CARRY_FLAG = 1; };
  inline void reset_carry() { regs.CPSR.CARRY_FLAG = 0; };

  inline void set_overflow() { regs.CPSR.OVERFLOW_FLAG = 1; };
  inline void reset_overflow() { regs.CPSR.OVERFLOW_FLAG = 0; };

  [[nodiscard]] u32 handle_shifts(InstructionInfo& instr);
  [[nodiscard]] u32 align_address(u32 address, BOUNDARY b);

  bool pipeline_refilled = false;
};
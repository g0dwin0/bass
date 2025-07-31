#pragma once

#include <spdlog/logger.h>

#include <unordered_map>

#include "common/test_structs.hpp"
#include "enums.hpp"
#include "spdlog/common.h"
struct InstructionInfo;
struct Bus;
#include "bus.hpp"
#include "common.hpp"
#include "common/defs.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

enum ALU_OP {
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

struct ARM7TDMI {
  ARM7TDMI();
  Registers regs = {};

  static constexpr u32 IRQ_VECTOR = 0x18;

  enum SHIFT_MODE : u8 {
    LSL,
    LSR,
    ASR,
    ROR,
  };

  Bus* bus = nullptr;

  bool flushed_pipeline = false;

  struct Pipeline {
    InstructionInfo fetch   = {};
    InstructionInfo decode  = {};
    InstructionInfo execute = {};
  } pipeline = {};

  std::shared_ptr<spdlog::logger> cpu_logger = spdlog::stdout_color_mt("ARM7TDMI");

  [[nodiscard]] InstructionInfo fetch(const u32 address);
  [[nodiscard]] InstructionInfo decode(InstructionInfo& op);

  [[nodiscard]] InstructionInfo arm_decode(InstructionInfo& op);
  [[nodiscard]] InstructionInfo thumb_decode(InstructionInfo& op);

  void flush_pipeline();

  u16 step();

  void print_pipeline() const;
  void execute(InstructionInfo& instr);
  void handle_interrupts();

  [[nodiscard]] inline bool check_condition(const InstructionInfo& i) const;
  [[nodiscard]] std::string get_addressing_mode(const InstructionInfo& instr) const;

  void print_registers() const;

  [[nodiscard]] std::string get_shift_string(u8 shift_type) const;

  // TODO: too many params, pack into struct
  [[nodiscard]] u32 shift(SHIFT_MODE mode, u64 value, u64 amount, bool special, bool never_rrx = false, bool affect_flags = true);

  [[nodiscard]] bool interrupt_queued() const;

  inline void set_zero() { regs.CPSR.ZERO_FLAG = 1; };
  inline void reset_zero() { regs.CPSR.ZERO_FLAG = 0; };

  inline void set_negative() { regs.CPSR.SIGN_FLAG = 1; };
  inline void reset_negative() { regs.CPSR.SIGN_FLAG = 0; };

  inline void set_carry() { regs.CPSR.CARRY_FLAG = 1; };
  inline void reset_carry() { regs.CPSR.CARRY_FLAG = 0; };

  inline void set_overflow() { regs.CPSR.OVERFLOW_FLAG = 1; };
  inline void reset_overflow() { regs.CPSR.OVERFLOW_FLAG = 0; };

  [[nodiscard]] u32 handle_shifts(InstructionInfo& instr, bool affect_flags = true);

  [[nodiscard]] static constexpr u32 align(u32 value, ALIGN_TO align_to) {
    if (align_to == WORD) {
      return value & ~3;
    } else {
      return value & ~1;
    }
  }

  // Aligns to value depending on the mode of the CPU
  [[nodiscard]] constexpr u32 align_by_current_mode(u32 value) {
    if (regs.CPSR.STATE_BIT == ARM_MODE) {
      return value & ~3;
    } else {
      return value & ~1;
    }
  }
};
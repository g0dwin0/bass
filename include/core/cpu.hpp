#pragma once

#include <spdlog/logger.h>

#include "bus.hpp"
#include "capstone/capstone.h"
#include "common/defs.hpp"
#include "instructions/arm.hpp"
#include "registers.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

typedef void (*FuncPtr)(ARM7TDMI*, u32);

enum struct ALU_OP : u8 {
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

enum struct THUMB_ALU_OP : u8 {
  AND = 0x0,
  EOR = 0x1,
  LSL = 0x2,
  LSR = 0x3,
  ASR = 0x4,
  ADC = 0x5,
  SBC = 0x6,
  ROR = 0x7,
  TST = 0x8,
  NEG = 0x9,
  CMP = 0xA,
  CMN = 0xB,
  ORR = 0xC,
  MUL = 0xD,
  BIC = 0xE,
  MVN = 0xF,
};

static constexpr u32 IRQ_VECTOR = 0x18;

struct ARM7TDMI {
  ARM7TDMI();
  Registers regs = {};

  enum SHIFT_MODE : u8 {
    LSL,
    LSR,
    ASR,
    ROR,
  };

  Bus* bus = nullptr;

  bool flushed_pipeline = false;

  struct Pipeline {
    u32 fetch   = 0;
    u32 decode  = 0;
    u32 execute = 0;
  } pipeline;

  std::shared_ptr<spdlog::logger> cpu_logger = spdlog::stdout_color_mt("ARM7TDMI");

  constexpr FuncPtr arm_decode(u32 shifted_opcode) {
    if ((shifted_opcode & 0xfcf) == 0x9) {
      return ARM_MULTIPLY;
    }

    if ((shifted_opcode & 0xf8f) == 0b000010001001) {
      return ARM_MULTIPLY;
    }

    if ((shifted_opcode & 0xfbf) == 0x109) {
      return ARM_SWAP;
    }

    if ((shifted_opcode & 0b111000001111) == 0b1011) {  //
      return ARM_HALFWORD_LOAD_STORE;
    }

    if ((shifted_opcode & 0b111000011101) == 0b000000011101) {
      return ARM_SIGNED_LOAD;
    }

    if ((shifted_opcode & 0b111110011111) == 0b000100000000) {
      return ARM_PSR_TRANSFER;
    }

    if ((shifted_opcode & 0b111110110000) == 0b001100100000) {
      return ARM_PSR_TRANSFER;
    }

    // BX
    if ((shifted_opcode & 0xFFF) == 0b000'10010'0001) {
      return ARM_BRANCH_EXCHANGE;
    }

    // DATA PROCESSING
    if ((shifted_opcode & 0b111000000001) == 0) {
      return ARM_DATA_PROCESSING;
    }
    if ((shifted_opcode & 0b111000001001) == 1) {
      return ARM_DATA_PROCESSING;
    }
    if ((shifted_opcode & 0b111000000000) == 0b001000000000) {
      return ARM_DATA_PROCESSING;
    }

    if ((shifted_opcode & 0b111000000000) == 0b010000000000) {
      return ARM_SINGLE_DATA_TRANSFER;
    }

    if ((shifted_opcode & 0b111000000001) == 0b011000000000) {
      return ARM_SINGLE_DATA_TRANSFER;
    }

    if ((shifted_opcode & 0b111000000000) == 0b100000000000) {
      return ARM_BLOCK_DATA_TRANSFER;
    }

    if ((shifted_opcode & 0b111000000000) == 0b101000000000) {
      return ARM_BRANCH_LINK;
    }

    if ((shifted_opcode & 0b111100000000) == 0b111100000000) {
      return ARM_SOFTWARE_INTERRUPT;
    }

    SPDLOG_DEBUG("[ARM7] failed to decode: {:#010x} PC: {:#010x}", instr.opcode, regs.r[15]);
    return nullptr;
  };

  constexpr FuncPtr thumb_decode(u16 opcode) {
    if ((opcode & 0b1111100000) == 0b0001100000) {  // ADD, SUB
      return THUMB_ADD_SUB;
    }

    if ((opcode & 0b1110000000) == 0) {  // MOVE SHIFTED REG
      return THUMB_MOVE_SHIFTED_REG;
    }

    if ((opcode & 0b1110000000) == 0b0010000000) {
      return THUMB_MOV_CMP_ADD_SUB;
    }

    if ((opcode & 0b1111110000) == 0b0100000000) {  // data proc
      return THUMB_DATA_PROC;
    }

    if ((opcode & 0b1111111100) == 0b0100011100) {
      return THUMB_ADD_CMP_MOV_HI;
    }

    if ((opcode & 0b1111110000) == 0b0100010000) {
      return THUMB_ADD_CMP_MOV_HI;
    }

    if ((opcode & 0b1111100000) == 0b0100100000) {
      return THUMB_LDR_PC_REL;
    }

    if ((opcode & 0b1111'0'11000) == 0b0101'0'01000) {
      return THUMB_SIGN_EXTENDED_LOAD_STORE_REG_OFFSET;
    }

    if ((opcode & 0b1111'011000) == 0b0101011000) {
      return THUMB_SIGN_EXTENDED_LOAD_STORE_REG_OFFSET;
    }

    if ((opcode & 0b1111'0'11'000) == 0b0101000000 || (opcode & 0b1111011000) == 0b0101010000) {
      return THUMB_WORD_BYTE_REG_OFFSET;
    }

    if ((opcode & 0b1110000000) == 0b0110000000) {
      return THUMB_WORD_BYTE_IMM_OFFSET;
    }

    if ((opcode & 0b1111000000) == 0b1000000000) {
      return THUMB_HALFWORD_IMM_OFFSET;
    }

    if ((opcode & 0b1111000000) == 0b1001000000) {  // THUMB 11
      return THUMB_LOAD_STORE_SP_REL;
    }

    if ((opcode & 0b1111000000) == 0b1010000000) {  // THUMB 12
      return THUMB_ADD_SP_PC;
    }

    if ((opcode & 0b1111111000) == 0b1011000000) {  // THUMB 13
      return THUMB_ADD_OFFSET_SP;
    }

    if ((opcode & 0b1111'011000) == 0b1011'010'000) {  // PUSH/POP
      return THUMB_PUSH_POP;
    }

    if ((opcode & 0b1111000000) == 0b1100000000) {  // LDM/STM
      return THUMB_BLOCK_DATA_TRANSFER;
    }

    // SWI
    if ((opcode & 0b1111111100) == 0b1101111100) {
      return ARM_SOFTWARE_INTERRUPT;
    }

    // Bcc
    if ((opcode & 0b1111000000) == 0b1101000000) {
      return THUMB_CONDITIONAL_BRANCH;
    }

    // B (unconditional)
    if ((opcode & 0b1111100000) == 0b1110000000) {
      return THUMB_UNCONDITIONAL_BRANCH;  // unconditional
    }

    // BL suffix
    if ((opcode & 0b1111100000) == 0b1111000000) {
      return THUMB_BLX_PREFIX;
    }

    // BL suffix
    if ((opcode & 0b1111100000) == 0b1111100000) {
      return THUMB_BL_SUFFIX;
    }

    return nullptr;
  };

  void flush_pipeline();
  u64 cycles_this_step;
  u64 step();

  void print_pipeline() const;
  void execute(u32 opcode);
  void handle_interrupts();

  [[nodiscard]] bool check_condition(Condition i) const;

  void print_registers() const;

  [[nodiscard]] std::string get_shift_string(u8 shift_type) const;

  // TODO: too many params, pack into struct
  [[nodiscard]] u32 shift(SHIFT_MODE mode, u64 value, u64 amount, bool special, bool never_rrx = false, bool affect_flags = true);

  [[nodiscard]] bool interrupt_queued() const;

  void set_zero() { regs.CPSR.ZERO_FLAG = true; };
  void reset_zero() { regs.CPSR.ZERO_FLAG = false; };

  void set_negative() { regs.CPSR.SIGN_FLAG = true; };
  void reset_negative() { regs.CPSR.SIGN_FLAG = false; };

  void set_carry() { regs.CPSR.CARRY_FLAG = true; };
  void reset_carry() { regs.CPSR.CARRY_FLAG = false; };

  void set_overflow() { regs.CPSR.OVERFLOW_FLAG = true; };
  void reset_overflow() { regs.CPSR.OVERFLOW_FLAG = false; };

  [[nodiscard]] u32 handle_shifts(bool I, bool R, u8 Rm, u8 Rs, u8 shift_type, u32 imm, u8 rotate, u8 shift_amount, bool S, bool affect_flags = true);

  u32 fetch(u32 address);

  constexpr std::array<FuncPtr, 4096> fill_arm_func_tables() {
    std::array<FuncPtr, 4096> arm_funcs = {};

    for (u32 i = 0; i < 4096; i++) {
      arm_funcs[i] = arm_decode(i);
    }

    return arm_funcs;
  };
  constexpr std::array<FuncPtr, 1024> fill_thumb_func_tables() {
    std::array<FuncPtr, 1024> thumb_funcs = {};

    for (u16 i = 0; i < 1024; i++) {
      thumb_funcs[i] = thumb_decode(i);
    }

    return thumb_funcs;
  };

  std::array<FuncPtr, 4096> arm_funcs   = fill_arm_func_tables();
  std::array<FuncPtr, 1024> thumb_funcs = fill_thumb_func_tables();

  [[nodiscard]] constexpr u32 align_by_current_mode(u32 value) const {
    if (regs.CPSR.STATE_BIT == ARM_MODE) {
      return value & ~3;
    } else {
      return value & ~1;
    }
  }
};
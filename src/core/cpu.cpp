#include "core/cpu.hpp"

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>

#include "capstone/arm.h"
#include "capstone/capstone.h"
#include "registers.hpp"

ARM7TDMI::ARM7TDMI() {
  cpu_logger->set_level(spdlog::level::trace);
  regs.r[15] = 0x08000000;
}
void ARM7TDMI::flush_pipeline() {
  // fmt::println("R15 pre-flush: {:#010x}", regs.r[15]);

  flushed_pipeline = true;
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    pipeline = {};

    pipeline.decode = bus->read32(regs.r[15]);
    pipeline.fetch  = bus->read32(regs.r[15] + 4);

    regs.r[15] += 4;
  } else {
    pipeline = {};

    pipeline.decode = bus->read16(regs.r[15]);
    pipeline.fetch  = bus->read16(regs.r[15] + 2);

    regs.r[15] += 2;
  }
  return;
}

u32 ARM7TDMI::handle_shifts(bool I, bool R, u8 Rm, u8 Rs, u8 shift_type, u32 imm, u8 rotate, u8 shift_amount, bool S, bool affects_flags) {
  u8 add_amount = 0;
  if (I == 0) {
    if (R) {  // I = 0, R = 1
      // spdlog::info("SHIFT VALUE IS REGISTER! Operand r{} (Rm) - r{} (Rs) [{:#010x}]", Rm, Rs, regs.get_reg(Rs) & 0xff);
      // instr.print_params();

      if (Rm == 15) {
        add_amount += 4;
      }

      return shift((ARM7TDMI::SHIFT_MODE)shift_type,
                   regs.get_reg(Rm) + add_amount,  // PC reads as PC+12 (PC already reads
                                                   // +8) when used as shift register
                   regs.get_reg(Rs) & 0xff, false, true, affects_flags);
    } else {
      return shift((ARM7TDMI::SHIFT_MODE)shift_type, regs.get_reg(Rm), shift_amount, true, false, affects_flags);
    }
  }

  if (S) {
    // cpu_logger->info("immediate value rotate before: {}", instr.imm);
    return shift(ROR, imm, rotate * 2, false, true);
  } else {
    // cpu_logger->info("rot val: {:#x}", std::rotr(imm, rotate * 2));
    // if((instr.rotate * 2) == 0) return instr.imm;
    return std::rotr(imm, rotate * 2);
  }
}

u32 ARM7TDMI::fetch(u32 address) {
  u32 opcode = regs.CPSR.STATE_BIT == THUMB_MODE ? bus->read16(address) : bus->read32(address);

  if (regs.r[15] <= 0x3FFF) bus->bios_open_bus = opcode;
  return opcode;
}

inline bool ARM7TDMI::interrupt_queued() const { return ((bus->interrupt_control.IE.v & 0x3fff) & (bus->interrupt_control.IF.v & 0x3fff)) != 0; }

inline void ARM7TDMI::handle_interrupts() {
  if (!regs.CPSR.irq_disable && interrupt_queued() && bus->interrupt_control.IME.enabled) {
    // fmt::println("IRQ fired!");

    auto cpsr         = regs.CPSR.value;
    CPU_MODE cpu_mode = regs.CPSR.STATE_BIT;

    regs.CPSR.MODE_BIT    = IRQ;
    regs.CPSR.STATE_BIT   = ARM_MODE;
    regs.CPSR.irq_disable = true;

    regs.get_reg(14) = cpu_mode == ARM_MODE ? regs.r[15] - 4 : regs.r[15];

    regs.SPSR_irq = cpsr;

    regs.r[15] = IRQ_VECTOR;
    flush_pipeline();
    // return 0;
  };
}

u16 ARM7TDMI::step() {
  // fmt::println("R0:{:08X} R1:{:08X} R2:{:08X} R3:{:08X} R4:{:08X} R5:{:08X} R6:{:08X} R7:{:08X} R8:{:08X} R9:{:08X} R10:{:08X} R11:{:08X} R12:{:08X} R13:{:08X} R14:{:08X} R15:{:08X}",
  // regs.get_reg(0),
  //              regs.get_reg(1), regs.get_reg(2), regs.get_reg(3), regs.get_reg(4), regs.get_reg(5), regs.get_reg(6), regs.get_reg(7), regs.get_reg(8), regs.get_reg(9), regs.get_reg(10),
  //              regs.get_reg(11),
  //  regs.get_reg(12), regs.get_reg(13), regs.get_reg(14), regs.get_reg(15));
  // regs.r[15] &= ~1;

  if (regs.r[15] != align_by_current_mode(regs.r[15])) {
    fmt::println("something left PC unaligned -- addr: {:#010x}", regs.r[15]);
    exit(-1);
    assert(0);
  }

  // if (regs.r[15] == 0x134) { fmt::println("branched to IRQ VECTOR -- cycles elapsed: {}", bus->cycles_elapsed); }

  pipeline.execute = pipeline.decode;
  pipeline.decode  = pipeline.fetch;

  handle_interrupts();

  // flushed_pipeline = false;

  // print_pipeline();
  bus->cycles_elapsed += 1;

  pipeline.fetch = fetch(regs.r[15]);

  execute(pipeline.execute);

  regs.r[15] += regs.CPSR.STATE_BIT == THUMB_MODE ? 2 : 4;

  return 0;
}

void ARM7TDMI::print_pipeline() const {
  fmt::println("============ PIPELINE ============");
  fmt::println("Fetch:  {:#010X}", this->pipeline.fetch);
  fmt::println("Decode  Phase Instr: {:#010X}", this->pipeline.decode);
  fmt::println("Execute Phase Instr: {:#010X}", this->pipeline.execute);
  fmt::println("PC (R15): {:#010X}", this->regs.r[15]);
  fmt::println("==================================");
}

void ARM7TDMI::execute(u32 opcode) {
  u16 shifted_opcode;
  // fmt::println("execute?");
  FuncPtr func = nullptr;
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    // fmt::println("ARM OPCODE: {:08X}", opcode);
    shifted_opcode = (u16)(((opcode & 0xff00000) >> 16) | ((opcode & 0b11110000) >> 4));
    func           = arm_funcs[shifted_opcode];
  } else {
    // fmt::println("THUMB OPCODE: {:#08X}", opcode);
    shifted_opcode = static_cast<u16>(opcode >> 6);
    func           = thumb_funcs[shifted_opcode];
  }

  if (func == nullptr) spdlog::warn("could not resolve, opcode: {:#08X} - s: {:#08X}", opcode, shifted_opcode);
  if (!check_condition(static_cast<Condition>(opcode >> 28)) && regs.CPSR.STATE_BIT == ARM_MODE) return;

  if (func == nullptr) {
    fmt::println("could not execute instruction {:#010X}", opcode);
    // assert(0);
    return;
  }

  func(this, opcode);
}

bool ARM7TDMI::check_condition(Condition cond) const {
  switch (cond) {
    case EQ: {
      return regs.CPSR.ZERO_FLAG;
    }
    case NE: {
      return !regs.CPSR.ZERO_FLAG;
    }
    case CS: {
      return regs.CPSR.CARRY_FLAG;
    }
    case CC: {
      return !regs.CPSR.CARRY_FLAG;
    }
    case MI: {
      return regs.CPSR.SIGN_FLAG;
    }
    case PL: {
      return !regs.CPSR.SIGN_FLAG;
    }
    case VS: {
      return regs.CPSR.OVERFLOW_FLAG;
    }
    case VC: {
      return !regs.CPSR.OVERFLOW_FLAG;
    }
    case HI: {
      return regs.CPSR.CARRY_FLAG && !regs.CPSR.ZERO_FLAG;
    }
    case LS: {
      return !regs.CPSR.CARRY_FLAG || regs.CPSR.ZERO_FLAG;
    }
    case GE: {
      return regs.CPSR.SIGN_FLAG == regs.CPSR.OVERFLOW_FLAG;
    }
    case LT: {
      return regs.CPSR.SIGN_FLAG != regs.CPSR.OVERFLOW_FLAG;
    }
    case GT: {
      return !regs.CPSR.ZERO_FLAG && regs.CPSR.SIGN_FLAG == regs.CPSR.OVERFLOW_FLAG;
    }
    case LE: {
      return regs.CPSR.ZERO_FLAG || regs.CPSR.SIGN_FLAG != regs.CPSR.OVERFLOW_FLAG;
    }
    case AL: {
      return true;
    }
    case NV: {
      // SPDLOG_WARN("instruction has NV condition.");
      return false;
    }
  }

  assert(0);
  return -1;
}

void ARM7TDMI::print_registers() const {
  for (u8 i = 0; i < 13; i++) {
    fmt::println("r{}: {:#010x}", i, regs.r[i]);
  }
  fmt::println("r13 (SP): {:#010x}", regs.r[13]);
  fmt::println("r14 (LR): {:#010x}", regs.r[14]);
  fmt::println("r15 (PC): {:#010x}\n", regs.r[15]);
}

std::string ARM7TDMI::get_shift_string(u8 shift_type) const {
  assert(shift_type < 4);

  switch (shift_type) {
    case 0: return "LSL";
    case 1: return "LSR";
    case 2: return "ASR";
    case 3: return "ROR";

    default: {
      assert(0);
      return "";
    }
  };
}
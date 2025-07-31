#include "core/cpu.hpp"

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <unordered_map>

#include "common/test_structs.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
ARM7TDMI::ARM7TDMI() {
  cpu_logger->set_level(spdlog::level::trace);
  regs.r[15] = 0x08000000;
}
void ARM7TDMI::flush_pipeline() {
  // fmt::println("R15 pre-flush: {:#010x}", regs.r[15]);
  
  flushed_pipeline = true;
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    InstructionInfo d = {};
    InstructionInfo e = {};

    pipeline = {};

    e.opcode = bus->read32(regs.r[15]);
    d.opcode = bus->read32(regs.r[15] + 4);

    pipeline.fetch  = d;
    pipeline.decode = decode(e);

    regs.r[15] += 4;
  } else {
    InstructionInfo d = {};
    InstructionInfo e = {};
    regs.r[15]        = align(regs.r[15], HALFWORD);
    // SPDLOG_DEBUG("R15: {:#08x}", regs.r[15]);
    pipeline = {};

    e.opcode = bus->read16(regs.r[15]);
    d.opcode = bus->read16(regs.r[15] + 2);

    pipeline.fetch  = d;
    pipeline.decode = decode(e);

    regs.r[15] += 2;
  }
  return;
}

u32 ARM7TDMI::handle_shifts(InstructionInfo& instr, bool affects_flags) {
  u8 add_amount = 0;
  if (instr.I == 0) {
    if (instr.R) {  // I = 0, R = 1
      // cpu_logger->info("SHIFT VALUE IS REGISTER! Operand r{} (Rm) - r{} (Rs) [{:#010x}]", +instr.Rm, +instr.Rs, regs.get_reg(instr.Rs) & 0xff);
      // instr.print_params();

      // if (instr.Rs == instr.Rm) { add_amount += 4; }
      if (instr.Rm == 15) {
        fmt::println("this is +4");
        add_amount += 4;
      }
      return shift((ARM7TDMI::SHIFT_MODE)instr.shift_type,
                   regs.get_reg(instr.Rm) + add_amount,  // PC reads as PC+12 (PC already reads
                                                         // +8) when used as shift register
                   regs.get_reg(instr.Rs) & 0xff, false, true, affects_flags);
    } else {
      // if (instr.Rm == 15) { add_amount += 4; }
      // cpu_logger->info("immediate shift: {} on {:#x}", instr.shift_amount, regs.r[instr.Rm]);
      return shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, regs.get_reg(instr.Rm), instr.shift_amount, true, false, affects_flags);
    }
  }

  if (instr.S) {
    // cpu_logger->info("immediate value rotate before: {}", instr.imm);
    return shift(ROR, instr.imm, instr.rotate * 2, false, true);
  } else {
    // cpu_logger->info("rot val: {:#x}", std::rotr(instr.imm, instr.rotate * 2));
    // if((instr.rotate * 2) == 0) return instr.imm;
    return std::rotr(instr.imm, instr.rotate * 2);
  }
}

InstructionInfo ARM7TDMI::decode(InstructionInfo& instr) {
  assert(instr.empty == false);
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    return arm_decode(instr);
  } else {
    return thumb_decode(instr);
  }
}

InstructionInfo ARM7TDMI::fetch(u32 address) {
  InstructionInfo instr = {};
  instr.opcode          = regs.CPSR.STATE_BIT == THUMB_MODE ? bus->read16(address) : bus->read32(address);
  // instr.loc    = address;

  instr.empty = false;
  // fmt::println("{:#x}",address);
  return instr;
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
    // regs.SPSR_svc    = cpsr;
    regs.SPSR_irq    = cpsr;
    
    regs.r[15]       = IRQ_VECTOR;
    flush_pipeline();
    // return 0;
  };
}

u16 ARM7TDMI::step() {
  // fmt::println("R0:{:08X} R1:{:08X} R2:{:08X} R3:{:08X} R4:{:08X} R5:{:08X} R6:{:08X} R7:{:08X} R8:{:08X} R9:{:08X} R10:{:08X} R11:{:08X} R12:{:08X} R13:{:08X} R14:{:08X} R15:{:08X}",
  // regs.GetReg(0),
  //              regs.GetReg(1), regs.GetReg(2), regs.GetReg(3), regs.GetReg(4), regs.GetReg(5), regs.GetReg(6), regs.GetReg(7), regs.GetReg(8), regs.GetReg(9), regs.GetReg(10), regs.GetReg(11),
  //  regs.GetReg(12), regs.GetReg(13), regs.GetReg(14), regs.GetReg(15));

  // if (regs.r[15] != align_by_current_mode(regs.r[15])) {
  //   fmt::println("something left PC unaligned");
  //   assert(0);
  // }


  // if (regs.r[15] == 0x134) { fmt::println("branched to IRQ VECTOR -- cycles elapsed: {}", bus->cycles_elapsed); }

  if (!pipeline.fetch.empty) {
    pipeline.execute = pipeline.decode;
    pipeline.decode  = pipeline.fetch;
  }

  handle_interrupts();

  // flushed_pipeline = false;

  // print_pipeline();
  bus->cycles_elapsed += 1;

  pipeline.fetch = fetch(regs.r[15]);

  // OPTIMIZE: we go through the pipeline as real HW does, but is there a point to decoding an instruction that might get flushed?
  // consider merging decoding and execution into one action, keep pipeline as is, but do decoding AND executing
  // in execution stage.

  if (this->pipeline.decode.opcode != 0) { pipeline.decode = decode(pipeline.decode); }
  execute(pipeline.execute);

  regs.r[15] += regs.CPSR.STATE_BIT == THUMB_MODE ? 2 : 4;

  return 0;
}

void ARM7TDMI::print_pipeline() const {
  fmt::println("============ PIPELINE ============");
  fmt::println("Fetch:  {:#010X}", this->pipeline.fetch.opcode);
  fmt::println("Decode  Phase Instr: {:#010X}", this->pipeline.decode.opcode);
  fmt::println("Execute Phase Instr: {:#010X}", this->pipeline.execute.opcode);
  fmt::println("PC (R15): {:#010X}", this->regs.r[15]);
  fmt::println("==================================");
}

void ARM7TDMI::execute(InstructionInfo& instr) {
  if (instr.func_ptr == nullptr) {
    // fmt::println("could not execute instruction {:#010X}", instr.opcode);
    // assert(0);
    return;
  }
  // cpu_logger->info("{} - {:#010x}", instr.mnemonic, regs.r[15]);
  // cpu_logger->info("N: {} Z: {} C: {} V: {}", +regs.CPSR.SIGN_FLAG, +regs.CPSR.ZERO_FLAG, +regs.CPSR.CARRY_FLAG, +regs.CPSR.OVERFLOW_FLAG);
  if (check_condition(instr)) {
    instr.func_ptr(*this, instr);
  } else {
    // fmt::println("cond: {}", (u8)instr.condition);
    // fmt::println("N: {} Z: {} C: {} V: {}", +regs.CPSR.SIGN_FLAG, +regs.CPSR.ZERO_FLAG, +regs.CPSR.CARRY_FLAG, +regs.CPSR.OVERFLOW_FLAG);
    // fmt::println("failed: {}", instr.mnemonic);
    // instr.print_params();
  }
}

bool ARM7TDMI::check_condition(const InstructionInfo& i) const {
  // fmt::println("instruction type: {}", (u8)i.instructionType);
  // fmt::println("{:#010x}", i.empty);
  if (regs.CPSR.STATE_BIT == THUMB_MODE && i.empty == false) return true;
  switch (i.condition) {
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
    default: {
      fmt::println("reached default, exiting.");
      assert(0);
    }
  }
}
std::string ARM7TDMI::get_addressing_mode(const InstructionInfo& instr) const {
  std::string addressing_mode = fmt::format("{}{}", instr.U ? "i" : "d", instr.P ? "b" : "a");

  return addressing_mode;
}

void ARM7TDMI::print_registers() const {
  for (u8 i = 0; i < 13; i++) {
    fmt::println("r{}: {:#010x}", i, regs.r[i]);
  }
  fmt::println("r13 (SP): {:#010x}", regs.r[13]);
  fmt::println("r14 (LR): {:#010x}", regs.r[14]);
  fmt::println("r15 (PC): {:#010x}\n", regs.r[15]);
}

// const std::unordered_map<u8, std::string> shift_string_map = {
//     {0, "LSL"},
//     {1, "LSR"},
//     {2, "ASR"},
//     {3, "ROR"},
// };

std::string ARM7TDMI::get_shift_string(u8 shift_type) const {
  assert(shift_type < 4);

  switch (shift_type) {
    case 0: {
      return "LSL";
    }
    case 1: {
      return "LSR";
    }
    case 2: {
      return "ASR";
    }
    case 3: {
      return "ROR";
    }
    default: {
      assert(0);
    }
  };
}
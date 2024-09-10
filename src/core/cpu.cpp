#include "core/cpu.hpp"

#include <cstdlib>

#include "decode/arm.cpp"
#include "decode/thumb.cpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "shifter.cpp"
#include "spdlog/spdlog.h"

ARM7TDMI::ARM7TDMI() {
  regs.r[15] = 0x08000000;
  spdlog::debug("cpu initialized");
}
void ARM7TDMI::flush_pipeline() {
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    SPDLOG_DEBUG("[ARM] flushing");
    // InstructionInfo f;
    instruction_info d;
    instruction_info e;

    pipeline = {};
    e.opcode = bus->read32(regs.r[15]);
    e.loc    = regs.r[15];
    d.opcode = bus->read32(regs.r[15] + 4);
    d.loc    = regs.r[15] + 4;

    d.empty = false;
    e.empty = false;

    pipeline.fetch  = d;
    pipeline.decode = decode(e);

    regs.r[15] += 4;
    SPDLOG_DEBUG("[ARM] flushed");
  } else {
    SPDLOG_DEBUG("[THUMB] flushing");
    instruction_info d;
    instruction_info e;
    regs.r[15] = align_address(regs.r[15], HALFWORD);
    SPDLOG_DEBUG("R15: {:#08x}", regs.r[15]);
    pipeline = {};
    e.opcode = bus->read16(regs.r[15]);
    e.loc    = regs.r[15];
    d.opcode = bus->read16(regs.r[15] + 2);
    d.loc    = regs.r[15] + 2;

    d.empty = false;
    e.empty = false;

    pipeline.fetch  = d;
    pipeline.decode = decode(e);

    regs.r[15] += 2;
  }
  return;
}

u32 ARM7TDMI::align_address(u32 address, BOUNDARY b) {
  if (b == WORD) {
    return address & ~3;
  } else {
    return address & ~1;
  }
}

u32 ARM7TDMI::handle_shifts(instruction_info& instr) {
  u8 add_amount = 0;
  if (instr.I == 0) {
    if (instr.shift_value_is_register) {
      if (instr.Rs == instr.Rm) { add_amount += 4; }
      if (instr.Rm == 15) { add_amount += 4; }
      // fmt::println("SHIFT VALUE IS REGISTER! Operand r{} - r{} []", +instr.Rm, +instr.Rs, regs.r[instr.Rs] & 0xff);
      return shift((ARM7TDMI::SHIFT_MODE)instr.shift_type,
                   regs.r[instr.Rm] + add_amount,  // PC reads as PC+12 (PC already reads
                                                   // +8) when used as shift register
                   regs.r[instr.Rs] & 0xff, false);
    } else {
      // if (instr.Rm == 15) { add_amount += 4; }
      // SPDLOG_DEBUG("immediate shift: {} on {:#x}", instr.shift_amount, regs.r[instr.Rm]);
      return shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, regs.r[instr.Rm] + add_amount, instr.shift_amount, true);
    }
  }

  // SPDLOG_DEBUG("immediate value rotate before: {}", instr.imm);
  if (instr.S) {
    return shift(ROR, instr.imm, instr.rotate * 2, false, true);
  } else {
    return std::rotr(instr.imm, instr.rotate * 2);
  }
}

instruction_info ARM7TDMI::decode(instruction_info& instr) {
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    return arm_decode(instr);
  } else {
    return thumb_decode(instr);
  }
}

instruction_info ARM7TDMI::fetch(u32 address) {
  instruction_info instr;
  instr.opcode = regs.CPSR.STATE_BIT == THUMB_MODE ? bus->read16(address) : bus->read32(address);
  instr.loc    = address;
  instr.empty  = false;
  // fmt::println("{:#x}",address);
  return instr;
}
u16 ARM7TDMI::step() {
  if (!pipeline.fetch.empty) {
    pipeline.execute = pipeline.decode;
    pipeline.decode  = pipeline.fetch;
  }
  bus->cycles_elapsed += 1;

  pipeline.fetch = fetch(regs.r[15]);

  if (this->pipeline.decode.opcode != 0) { pipeline.decode = decode(pipeline.decode); }

  if (this->pipeline.execute.opcode != 0) { execute(pipeline.execute); }

  regs.r[15] += regs.CPSR.STATE_BIT ? 2 : 4;

  // move everything up, so we can fetch a new value

  // print_pipeline();

  return 0;
}

void ARM7TDMI::print_pipeline() {
  spdlog::debug("============ PIPELINE ============");
  spdlog::debug("Fetch:  {:#010X}", this->pipeline.fetch.opcode);
  spdlog::debug("Decode  Phase Instr: {:#010X}", this->pipeline.decode.opcode);
  spdlog::debug("Execute Phase Instr: {:#010X}", this->pipeline.execute.opcode);
  spdlog::debug("PC (R15): {:#010X}", this->regs.r[15]);
  spdlog::debug("==================================");
}

void ARM7TDMI::execute(instruction_info& instr) {
  if (instr.func_ptr == nullptr) {
    SPDLOG_CRITICAL("could not execute instruction {:#10X}", instr.opcode);
    assert(0);
  }
  spdlog::info("{}", instr.mnemonic);
  if (check_condition(instr)) {
    instr.func_ptr(*this, instr);
  } else {
    // fmt::println("N: {} Z: {} C: {} V: {}", +regs.CPSR.SIGN_FLAG, +regs.CPSR.ZERO_FLAG, +regs.CPSR.CARRY_FLAG, +regs.CPSR.OVERFLOW_FLAG);
    // fmt::println("failed: {}", instr.mnemonic);
  }
}

bool ARM7TDMI::check_condition(instruction_info& i) {
  // if (regs.CPSR.STATE_BIT == THUMB_MODE) return true;
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
      SPDLOG_WARN("instruction has NV condition.");
      return false;
    }
    default: {
      fmt::println("reached default, exiting.");
      assert(0);
    }
  }
}
std::string ARM7TDMI::get_addressing_mode_string(const instruction_info& instr) {
  std::string addressing_mode = fmt::format("{}{}", instr.U ? "i" : "d", instr.P ? "b" : "a");

  return addressing_mode;
}

void ARM7TDMI::print_registers() {
  fmt::println("\nr0: {:#010x}", regs.r[0]);
  fmt::println("r1: {:#010x}", regs.r[1]);
  fmt::println("r2: {:#010x}", regs.r[2]);
  fmt::println("r3: {:#010x}", regs.r[3]);
  fmt::println("r4: {:#010x}", regs.r[4]);
  fmt::println("r5: {:#010x}", regs.r[5]);
  fmt::println("r6: {:#010x}", regs.r[6]);
  fmt::println("r7: {:#010x}", regs.r[7]);
  fmt::println("r8: {:#010x}", regs.r[8]);
  fmt::println("r9: {:#010x}", regs.r[9]);
  fmt::println("r10: {:#010x}", regs.r[10]);
  fmt::println("r11: {:#010x}", regs.r[11]);
  fmt::println("r12: {:#010x}", regs.r[12]);
  fmt::println("r13: {:#010x}", regs.r[13]);
  fmt::println("r14: {:#010x}", regs.r[14]);
  fmt::println("r15: {:#010x}\n", regs.r[15]);
}

std::string_view ARM7TDMI::get_shift_type_string(u8 shift_type) {
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
      return "INVALID";
    }
  };
}
#include "instructions/arm.hpp"

#include <cassert>

#include "cpu.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "spdlog/fmt/bundled/core.h"

inline bool is_negative(u32 v) { return v >= 0x80000000; }
inline bool is_zero(u32 v) { return v == 0; }

void ARM::Instructions::B(ARM7TDMI& c, InstructionInfo& instr) {
  if (instr.L) { c.regs.get_reg(14) = c.regs.r[15] - 4; }

  c.regs.r[15] += instr.offset;

  c.regs.r[15] &= ~1;
  c.flush_pipeline();
}

void ARM::Instructions::BX(ARM7TDMI& c, InstructionInfo& instr) {
  // 0001b: BX{cond}  Rn    ;PC=Rn, T=Rn.0   (ARMv4T and ARMv5 and up)
  c.regs.r[15]          = c.regs.get_reg(instr.Rn);
  c.regs.CPSR.STATE_BIT = (CPU_MODE)(c.regs.get_reg(instr.Rn) & 0b1);

  // c.regs.CPSR.STATE_BIT ? SPDLOG_INFO("entered THUMB mode") : SPDLOG_INFO("entered ARM mode");
  c.regs.r[15] = c.align_by_current_mode(c.regs.r[15]);
  
  c.flush_pipeline();
}

void ARM::Instructions::NOP([[gnu::unused]] ARM7TDMI& c, [[gnu::unused]] InstructionInfo& instr) { return; }
void ARM::Instructions::AND(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, instr.S);

  check_r15_lookahead(instr, op1);

  u32 result = op1 & instr.op2;

  if (instr.S) {
    is_negative(result) ? c.set_negative() : c.reset_negative();
    is_zero(result) ? c.set_zero() : c.reset_zero();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) { c.flush_pipeline(); }
}

void ARM::Instructions::EOR(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, instr.S);

  check_r15_lookahead(instr, op1);

  u32 result = op1 ^ instr.op2;

  if (instr.S) {
    is_zero(result) ? c.set_zero() : c.reset_zero();
    is_negative(result) ? c.set_negative() : c.reset_negative();
    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) { c.flush_pipeline(); }
}
void ARM::Instructions::BIC(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, instr.S);

  check_r15_lookahead(instr, op1);

  auto result = op1 & (~instr.op2);

  if (instr.S) {
    is_negative(result) ? c.set_negative() : c.reset_negative();
    is_zero(result) ? c.set_zero() : c.reset_zero();
    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) { c.flush_pipeline(); }
}

void ARM::Instructions::check_r15_lookahead(InstructionInfo& instr, u32& op1) {
  if (instr.Rn == 15 && instr.I == 0 && instr.R && op1 != U32_MAX && op1 != 0) op1 += 4;
  // if (instr.I == 0 && instr.shift_value_is_register && instr.Rm == 15 && instr.op2 != U32_MAX && instr.op2 != 0) instr.op2 += 4;
}

void ARM::Instructions::RSB(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, false);

  check_r15_lookahead(instr, op1);

  u32 result = instr.op2 - op1;

  u32 op2_msb = instr.op2 & 0x80000000;
  u32 rn_msb  = op1 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (instr.S) {
    is_zero(result) ? c.set_zero() : c.reset_zero();
    is_negative(result) ? c.set_negative() : c.reset_negative();
    ((op2_msb ^ res_msb) & (op2_msb ^ rn_msb)) ? c.set_overflow() : c.reset_overflow();
    instr.op2 >= c.regs.get_reg(instr.Rn) ? c.set_carry() : c.reset_carry();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) { c.flush_pipeline(); }
}

void ARM::Instructions::SUB(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, false);

  check_r15_lookahead(instr, op1);

  u32 result = op1 - instr.op2;

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (instr.S) {
    is_negative((op1 - instr.op2)) ? c.set_negative() : c.reset_negative();
    is_zero((op1 - instr.op2)) ? c.set_zero() : c.reset_zero();

    op1 >= instr.op2 ? c.set_carry() : c.reset_carry();
    (rn_msb != rm_msb && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();

    if (instr.Rd == 15) {
      c.regs.load_spsr_to_cpsr();
    }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) {  // TODO: align
    c.regs.get_reg(instr.Rd) = c.align_by_current_mode(c.regs.get_reg(instr.Rd));
    c.flush_pipeline();
  }
}

void ARM::Instructions::SBC(ARM7TDMI& c, InstructionInfo& instr) {
  bool old_carry = c.regs.CPSR.CARRY_FLAG;
  u32 op1        = c.regs.get_reg(instr.Rn);
  instr.op2      = c.handle_shifts(instr, false);

  check_r15_lookahead(instr, op1);

  // fmt::println("op1: {:#010x}", op1);
  // fmt::println("op2: {:#010x}", instr.op2);

  u32 result = op1 - instr.op2 + (old_carry - 1);

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (instr.S) {
    is_negative(result) ? c.set_negative() : c.reset_negative();
    is_zero(result) ? c.set_zero() : c.reset_zero();
    (op1 >= ((u64)instr.op2 + (1 - old_carry))) ? c.set_carry() : c.reset_carry();
    ((rn_msb != rm_msb) && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) {
    c.regs.get_reg(instr.Rd) = c.align_by_current_mode(c.regs.get_reg(instr.Rd));
    c.flush_pipeline();
  }
}

void ARM::Instructions::MVN(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr, instr.S);

  if (instr.S) {
    ~instr.op2 == 0 ? c.set_zero() : c.reset_zero();
    (~instr.op2 & 0x80000000) != 0 ? c.set_negative() : c.reset_negative();
    if (instr.Rd == 15 && instr.S) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = ~instr.op2;

  if (instr.Rd == 15) {
    c.regs.get_reg(instr.Rd) = c.align_by_current_mode(c.regs.get_reg(instr.Rd));
    c.flush_pipeline();
  }
}

void ARM::Instructions::ADC(ARM7TDMI& c, InstructionInfo& instr) {
  // Carry from barrel shifter should not be used for this instruction.
  bool old_carry = c.regs.CPSR.CARRY_FLAG;

  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, false);

  u64 result_64 = ((u64)c.regs.get_reg(instr.Rn) + instr.op2 + old_carry);
  u32 result_32 = (c.regs.get_reg(instr.Rn) + instr.op2 + old_carry);

  check_r15_lookahead(instr, op1);

  if (instr.S) {
    result_32 == 0 ? c.set_zero() : c.reset_zero();

    u32 overflow = ~(op1 ^ instr.op2) & (op1 ^ result_32);
    ((overflow >> 31) & 1) ? c.set_overflow() : c.reset_overflow();

    (result_64 > U32_MAX) ? c.set_carry() : c.reset_carry();
    is_negative(result_32) ? c.set_negative() : c.reset_negative();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = (op1 + instr.op2 + old_carry);

  if (instr.Rd == 15) {
    c.regs.get_reg(instr.Rd) = c.align_by_current_mode(c.regs.get_reg(instr.Rd));
    c.flush_pipeline();
  }
}

void ARM::Instructions::TEQ(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr, instr.S);

  u32 m = c.regs.get_reg(instr.Rn) ^ instr.op2;

  u32 rn_msb  = c.regs.get_reg(instr.Rn) & 0x80000000;
  u32 rm_msb  = c.regs.get_reg(instr.Rm) & 0x80000000;
  u32 res_msb = m & 0x80000000;

  // (rn_msb == rm_msb && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();

  if (!instr.S) {
    if (rn_msb != 0 && rm_msb != 0 && res_msb == 0) {
      c.set_overflow();
    } else if (rn_msb == 0 && rm_msb == 0 && res_msb != 0) {
      c.set_overflow();
    } else {
      c.reset_overflow();
    }
  }

  is_negative(m) ? c.set_negative() : c.reset_negative();
  is_zero(m) ? c.set_zero() : c.reset_zero();

  if (instr.Rd == 15 && instr.S) { c.regs.load_spsr_to_cpsr(); }
}

void ARM::Instructions::MOV(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr, instr.S);

  if (instr.S) {
    is_zero(instr.op2) ? c.set_zero() : c.reset_zero();
    is_negative(instr.op2) ? c.set_negative() : c.reset_negative();
    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = instr.op2;

  if (instr.Rd == 15) {
    c.regs.get_reg(instr.Rd) = c.align_by_current_mode(c.regs.get_reg(instr.Rd));
    c.flush_pipeline();
  }
}

void ARM::Instructions::ADD(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, false);
  check_r15_lookahead(instr, op1);

  if (instr.pc_relative) { op1 &= ~2; }

  u64 result_64 = (u64)op1 + instr.op2;
  u32 result_32 = op1 + instr.op2;

  if (instr.S) {
    result_32 == 0 ? c.set_zero() : c.reset_zero();

    u32 overflow = ~(op1 ^ instr.op2) & (op1 ^ result_64);
    ((overflow >> 31) & 1) ? c.set_overflow() : c.reset_overflow();

    (result_64 > U32_MAX) ? c.set_carry() : c.reset_carry();
    is_negative(result_32) ? c.set_negative() : c.reset_negative();
    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result_32;

  if (instr.Rd == 15) {
    c.regs.get_reg(15) = c.align_by_current_mode(c.regs.get_reg(15));
    c.flush_pipeline();
  }
}

void ARM::Instructions::TST(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, instr.S);

  check_r15_lookahead(instr, op1);

  u32 d = op1 & instr.op2;

  is_zero(d) ? c.set_zero() : c.reset_zero();
  is_negative(d) ? c.set_negative() : c.reset_negative();

  if (instr.Rd == 15 && instr.S) { c.regs.load_spsr_to_cpsr(); }
}

void ARM::Instructions::STRH(ARM7TDMI& c, InstructionInfo& instr) {
  u32 value                = c.regs.get_reg(instr.Rd);
  u32 address              = c.regs.get_reg(instr.Rn);
  u8 immediate_offset      = instr.offset;
  u32 register_offset      = c.regs.get_reg(instr.Rm);
  u8 added_writeback_value = 0;
  if (instr.Rn == 15) added_writeback_value += 4;
  if (instr.Rd == 15) value += 4;

  // instr.print_params();

  // fmt::println("addr: {:#010x}", address);

  // u32 offset = instr.I == 1 ? immediate_offset : register_offset;
  // u32 register_offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.get_reg(instr.Rm), instr.shift_amount, true, false, false);
  // u8 rotate_by        = 0;

  if (instr.P == 0) {  // P == 0

    c.bus->write16(address & ~1, value);

    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }

  } else {  // P == 1

    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }

    c.bus->write16(address & ~1, value);
  }

  InstructionInfo _instr = instr;
  single_data_transfer_writeback_str(c, _instr, address, added_writeback_value);
}

void ARM::Instructions::STR(ARM7TDMI& c, InstructionInfo& instr) {
  // u32 address = instr.Rn == 15 ? c.regs.get_reg(instr.Rn) + 4 : c.regs.get_reg(instr.Rn);
  u32 value                = c.regs.get_reg(instr.Rd);
  u32 address              = c.regs.get_reg(instr.Rn);
  u8 added_writeback_value = 0;

  if (instr.Rn == 15) added_writeback_value = 4;
  if (instr.Rd == 15) value += 4;

  // c.cpu_logger->debug("addr: {:#010x}", address);

  u32 offset_from_reg = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.get_reg(instr.Rm), instr.shift_amount, true, false, false);
  // instr.print_params();
  // c.cpu_logger->debug("offset from reg: {:#010x}", offset_from_reg);
  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register

  if (instr.P == 0) {  // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

    if (instr.B) {  // B - Byte/Word bit (0=transfer 32bit/word, 1=transfer 8bit/byte)
      c.bus->write8(address, value);
    } else {
      c.bus->write32(c.align(address, WORD), value);
    }

    if (instr.U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (instr.I == 0 ? instr.offset : offset_from_reg);
    } else {
      address -= (instr.I == 0 ? instr.offset : offset_from_reg);
    }

  } else {  // Post added offset

    if (instr.U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (instr.I == 0 ? instr.offset : offset_from_reg);
    } else {
      address -= (instr.I == 0 ? instr.offset : offset_from_reg);
    }

    if (instr.B) {  // B - Byte/Word bit (0=transfer 32bit/word, 1=transfer 8bit/byte)
      c.bus->write8(address, value);
    } else {
      c.bus->write32(c.align(address, WORD), value);
    }
  }

  single_data_transfer_writeback_str(c, instr, address, added_writeback_value);
}
void ARM::Instructions::THUMB_LDR([[gnu::unused]] ARM7TDMI& c, [[gnu::unused]] InstructionInfo& instr) { return; };

void ARM::Instructions::THUMB_STR(ARM7TDMI& c, InstructionInfo& instr) {
  // u32 address = instr.Rn == 15 ? c.regs.get_reg(instr.Rn) + 4 : c.regs.get_reg(instr.Rn);
  u32 value                = c.regs.get_reg(instr.Rd);
  u32 address              = c.regs.get_reg(instr.Rn);
  u8 added_writeback_value = 0;

  if (instr.Rn == 15) added_writeback_value = 4;
  if (instr.Rd == 15) value += 4;

  // c.cpu_logger->debug("addr: {:#010x}", address);

  u32 offset_from_reg = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.get_reg(instr.Rm), instr.shift_amount, true, false, false);
  // instr.print_params();
  // c.cpu_logger->debug("offset from reg: {:#010x}", offset_from_reg);
  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register

  if (instr.P == 0) {  // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

    if (instr.B) {  // B - Byte/Word bit (0=transfer 32bit/word, 1=transfer 8bit/byte)
      c.bus->write8(address, value);
    } else {
      c.bus->write32(c.align(address, HALFWORD), value);
    }

    if (instr.U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (instr.I == 0 ? instr.offset : offset_from_reg);
    } else {
      address -= (instr.I == 0 ? instr.offset : offset_from_reg);
    }

  } else {  // Post added offset

    if (instr.U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (instr.I == 0 ? instr.offset : offset_from_reg);
    } else {
      address -= (instr.I == 0 ? instr.offset : offset_from_reg);
    }

    if (instr.B) {  // B - Byte/Word bit (0=transfer 32bit/word, 1=transfer 8bit/byte)
      c.bus->write8(address, value);
    } else {
      c.bus->write32(c.align(address, HALFWORD), value);
    }
  }

  single_data_transfer_writeback_str(c, instr, address, added_writeback_value);
}

void ARM::Instructions::single_data_transfer_writeback_str(ARM7TDMI& c, InstructionInfo& instr, u32 address, u8 added_writeback_value) {
  // Write-back is optional when P=1, forced when P=0
  // When P=1, check for writeback bit and writeback (if set)
  // Write-backs do not need the address aligned, only writes/loads do.
  // If R15 is the writeback register, add 4 (and align, obviously)
  // REFACTOR: i do believe that handling things like alignment are better handled on the side of the bus, not the instruction
  if (instr.P == 0 || (instr.P == 1 && instr.W)) {
    u32 writeback_addr       = address + added_writeback_value;
    c.regs.get_reg(instr.Rn) = writeback_addr;

    // If we wrote to R15, we need to flush, as the pipeline is now invalid.
    if (instr.Rn == 15) {
      c.regs.r[15] = c.align(writeback_addr, c.regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
      c.flush_pipeline();
    }
  }
};

void ARM::Instructions::single_data_transfer_writeback_ldr(ARM7TDMI& c, InstructionInfo& instr, u32 address, u8 added_writeback_value) {
  // Write-back is optional when P=1, forced when P=0
  // When P=1, check for writeback bit and writeback (if set)
  // Write-backs do not need the address aligned, only writes/loads do.
  // If R15 is the writeback register, add 4 (and align, obviously)
  // REFACTOR: i do believe that handling things like alignment are better handled on the side of the bus, not the instruction

  if (instr.Rd == 15) {
    c.regs.r[15] = c.align(c.regs.r[15], c.regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
    c.flush_pipeline();
  }

  if (instr.Rd == instr.Rn) return;

  if (instr.P == 0 || (instr.P == 1 && instr.W)) {
    // fmt::println("instr rn: {}", instr.Rn);

    u32 writeback_addr       = address + added_writeback_value;
    c.regs.get_reg(instr.Rn) = writeback_addr;

    if (instr.Rn == 15) {
      fmt::println("R15: {:#010x}", c.regs.get_reg(instr.Rn));
      c.flush_pipeline();
    }
  }
}

void ARM::Instructions::CMN(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, false);

  u64 r_64 = (u64)op1 + instr.op2;

  u32 r = op1 + instr.op2;

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = r & 0x80000000;

  is_negative(r) ? c.set_negative() : c.reset_negative();
  is_zero(r) ? c.set_zero() : c.reset_zero();
  (r_64 > U32_MAX) ? c.set_carry() : c.reset_carry();

  if (rn_msb != 0 && rm_msb != 0 && res_msb == 0) {
    c.set_overflow();
  } else if (rn_msb == 0 && rm_msb == 0 && res_msb != 0) {
    c.set_overflow();
  } else {
    c.reset_overflow();
  }

  if (instr.Rd == 15 && instr.S) { c.regs.load_spsr_to_cpsr(); }
}

void ARM::Instructions::RSC(ARM7TDMI& c, InstructionInfo& instr) {
  bool old_carry = c.regs.CPSR.CARRY_FLAG;
  u32 op1        = c.regs.get_reg(instr.Rn);

  instr.op2 = c.handle_shifts(instr, false);

  check_r15_lookahead(instr, op1);

  u32 result = (instr.op2 - op1) + (old_carry - 1);

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (instr.S) {
    result >= 0x80000000 ? c.set_negative() : c.reset_negative();
    result == 0 ? c.set_zero() : c.reset_zero();
    instr.op2 >= (op1 + (1 - old_carry)) ? c.set_carry() : c.reset_carry();
    ((rn_msb != rm_msb) && (rm_msb != res_msb)) ? c.set_overflow() : c.reset_overflow();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) {  // TODO: align this
    c.regs.get_reg(instr.Rd) = c.align_by_current_mode(c.regs.get_reg(instr.Rd));
    c.flush_pipeline();
  }
}
void ARM::Instructions::LDR(ARM7TDMI& c, InstructionInfo& instr) {
  InstructionInfo _instr = instr;

  u32 address              = c.regs.get_reg(instr.Rn);
  u16 rotate_by            = 0;
  u8 added_writeback_value = 0;
  // fmt::println("Rm{} - {:#010X}", instr.Rm, c.regs.get_reg(instr.Rm));
  // fmt::println("Rm{} ROR'D {} - {:#010X}", instr.Rm, instr.shift_amount, std::rotr(c.regs.get_reg(instr.Rm), instr.shift_amount));

  u32 offset_from_reg = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.get_reg(instr.Rm), instr.shift_amount, true, false, false);

  if (instr.Rn == 15) added_writeback_value += 4;

  if (instr.pc_relative) {
    // fmt::println("PC-relative");
    address &= ~2;
  }

  // c.cpu_logger->debug("addr: {:#010x}", address);
  // c.cpu_logger->debug("offset from reg: {:#010x}", offset_from_reg);

  // instr.print_params();

  if (instr.P == 0) {  // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

    rotate_by = (address & 3) * 8;

    if (instr.B) {
      c.regs.get_reg(instr.Rd) = c.bus->read8(address);
    } else {
      c.regs.get_reg(instr.Rd) = std::rotr(c.bus->read32(address), rotate_by);  // Rotate new contents if address was misaligned
    }

    if (instr.U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (instr.I == 0 ? instr.offset : offset_from_reg);
    } else {
      address -= (instr.I == 0 ? instr.offset : offset_from_reg);
    }

  } else {
    if (instr.U) {
      address += (instr.I == 0 ? instr.offset : offset_from_reg);
    } else {
      address -= (instr.I == 0 ? instr.offset : offset_from_reg);
    }

    rotate_by = (address & 3) * 8;

    if (instr.B) {
      c.regs.get_reg(instr.Rd) = c.bus->read8(address);
    } else {
      c.regs.get_reg(instr.Rd) = std::rotr(c.bus->read32(address), rotate_by);  // Rotate new contents if address was misaligned
    }
  }

  single_data_transfer_writeback_ldr(c, _instr, address, added_writeback_value);
}

void ARM::Instructions::LDRSB(ARM7TDMI& c,
                              InstructionInfo& instr) {  // TODO: Re-write this, too long and unnecessarily complex
  u32 address              = c.regs.get_reg(instr.Rn);
  u8 immediate_offset      = instr.offset;
  u32 register_offset      = c.regs.get_reg(instr.Rm);
  u8 added_writeback_value = 0;
  if (instr.Rn == 15) added_writeback_value += 4;

  // instr.print_params();

  // fmt::println("addr: {:#010x}", address);

  // u32 offset = instr.I == 1 ? immediate_offset : register_offset;
  // u32 register_offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.get_reg(instr.Rm), instr.shift_amount, true, false, false);
  // u8 rotate_by        = 0;

  // On ARM7 aka ARMv4 aka NDS7/GBA:

  // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
  // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value

  // u8 misaligned_load_rotate_value = 0;

  if (instr.P == 0) {  // P == 0
    u32 value = c.bus->read8(address);
    if (value & (1 << 7)) value |= 0xffffff00;
    c.regs.get_reg(instr.Rd) = value;

    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }

  } else {  // P == 1

    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }

    u32 value = c.bus->read8(address);
    if (value & (1 << 7)) value |= 0xffffff00;
    c.regs.get_reg(instr.Rd) = value;
  }
  InstructionInfo _instr = instr;
  single_data_transfer_writeback_ldr(c, _instr, address, added_writeback_value);
}

void ARM::Instructions::LDRSH(ARM7TDMI& c, InstructionInfo& instr) {
  u32 address              = c.regs.get_reg(instr.Rn);
  u8 immediate_offset      = instr.offset;
  u32 register_offset      = c.regs.get_reg(instr.Rm);
  u8 added_writeback_value = 0;
  if (instr.Rn == 15) added_writeback_value += 4;

  // instr.print_params();

  // fmt::println("addr: {:#010x}", address);

  // u32 offset = instr.I == 1 ? immediate_offset : register_offset;
  // u32 register_offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.get_reg(instr.Rm), instr.shift_amount, true, false, false);
  // u8 rotate_by        = 0;

  // On ARM7 aka ARMv4 aka NDS7/GBA:

  // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
  // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value

  // u8 misaligned_load_rotate_value = 0;

  if (instr.P == 0) {  // P == 0
    if ((address & 1) != 0) {
      u32 value = c.bus->read8(address);
      if (value & (1 << 7)) value |= 0xffffff00;
      c.regs.get_reg(instr.Rd) = value;
    } else {
      u32 value = c.bus->read16(address);
      if (value & (1 << 15)) value |= 0xFFFF0000;
      c.regs.get_reg(instr.Rd) = value;
    }

    // c.regs.get_reg(instr.Rd) = std::rotr(c.regs.get_reg(instr.Rd), misaligned_load_rotate_value);
    // fmt::println("offset {:#010X}", immediate_offset);
    // fmt::println("b4 {:#010X}", address);
    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }
    // fmt::println("after {:#010X}", address);

  } else {  // P == 1

    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }

    if ((address & 1) != 0) {  // info: this does not happen on the DS (ARM946E-S) so change if this is ever used as a core for that device
      u32 value = c.bus->read8(address);
      if (value & (1 << 7)) value |= 0xffffff00;
      c.regs.get_reg(instr.Rd) = value;
    } else {
      u32 value = c.bus->read16(address);
      if (value & (1 << 15)) value |= 0xFFFF0000;
      c.regs.get_reg(instr.Rd) = value;
    }
  }
  InstructionInfo _instr = instr;
  single_data_transfer_writeback_ldr(c, _instr, address, added_writeback_value);
}

void ARM::Instructions::LDRH(ARM7TDMI& c,
                             InstructionInfo& instr) {  // TODO: implement writeback logic
  u32 address              = c.regs.get_reg(instr.Rn);
  u8 immediate_offset      = instr.offset;
  u32 register_offset      = c.regs.get_reg(instr.Rm);
  u8 added_writeback_value = 0;
  if (instr.Rn == 15) added_writeback_value += 4;

  // instr.print_params();

  // fmt::println("addr: {:#010x}", address);

  // u32 offset = instr.I == 1 ? immediate_offset : register_offset;
  // u32 register_offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.get_reg(instr.Rm), instr.shift_amount, true, false, false);
  // u8 rotate_by        = 0;

  // On ARM7 aka ARMv4 aka NDS7/GBA:

  // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
  // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value

  u8 misaligned_load_rotate_value = 0;

  if (instr.P == 0) {  // P == 0

    if ((address & 1) != 0) misaligned_load_rotate_value = 8;
    // fmt::println("aligned?: {}", (address & 1) == 0);

    u32 value = c.bus->read16(address & ~1);
    // fmt::println("value read: {:#010x}", value);
    value                    = std::rotr(value, misaligned_load_rotate_value);
    c.regs.get_reg(instr.Rd) = value;

    // fmt::println("value after shift: {:#010x}", c.regs.get_reg(instr.Rd));

    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }

  } else {  // P == 1

    if (instr.U) {
      address += (instr.I ? immediate_offset : register_offset);
    } else {
      address -= (instr.I ? immediate_offset : register_offset);
    }

    if ((address & 1) != 0) misaligned_load_rotate_value = 8;
    // fmt::println("aligned?: {}", (address & 1) == 0);

    u32 value = c.bus->read16(address & ~1);
    // fmt::println("value read: {:#010x}", value);
    value                    = std::rotr(value, misaligned_load_rotate_value);
    c.regs.get_reg(instr.Rd) = value;

    // fmt::println("value after shift: {:#010x}", c.regs.get_reg(instr.Rd));
  }
  InstructionInfo _instr = instr;
  single_data_transfer_writeback_ldr(c, _instr, address, added_writeback_value);
}

void ARM::Instructions::CMP(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, false);

  u32 result = op1 - instr.op2;

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  is_zero(result) ? c.set_zero() : c.reset_zero();
  is_negative(result) ? c.set_negative() : c.reset_negative();
  (rn_msb != rm_msb && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();
  (op1 >= instr.op2) ? c.set_carry() : c.reset_carry();

  if (instr.Rd == 15 && instr.S) { c.regs.load_spsr_to_cpsr(); }
}

void ARM::Instructions::ORR(ARM7TDMI& c, InstructionInfo& instr) {
  u32 op1   = c.regs.get_reg(instr.Rn);
  instr.op2 = c.handle_shifts(instr, instr.S);

  check_r15_lookahead(instr, op1);

  u32 result = op1 | instr.op2;

  if (instr.S) {
    is_zero(result) ? c.set_zero() : c.reset_zero();
    is_negative(result) ? c.set_negative() : c.reset_negative();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.get_reg(instr.Rd) = result;

  if (instr.Rd == 15) { c.flush_pipeline(); }
}
void ARM::Instructions::MRS(ARM7TDMI& c, InstructionInfo& instr) {
  u32 psr = 0;

  if (instr.P == 0) {  // P(sr) (0=CPSR, 1=SPSR_<current_mode>)
    psr = c.regs.CPSR.value;
  } else {
    psr = c.regs.get_spsr(c.regs.CPSR.MODE_BIT);
  }

  c.regs.get_reg(instr.Rd) = psr;
}

void ARM::Instructions::SWI(ARM7TDMI& c, [[gnu::unused]] InstructionInfo& instr) {
  auto cpsr = c.regs.CPSR.value;

  c.regs.CPSR.MODE_BIT    = SUPERVISOR;
  c.regs.svc_r[14]        = c.regs.r[15] - (c.regs.CPSR.STATE_BIT == THUMB_MODE ? 2 : 4);
  c.regs.SPSR_svc         = cpsr;
  c.regs.CPSR.STATE_BIT   = ARM_MODE;
  c.regs.CPSR.irq_disable = true;

  c.regs.r[15] = 0x00000008;

  c.flush_pipeline();
}

void ARM::Instructions::MSR(ARM7TDMI& c, InstructionInfo& instr) {
  // TODO: refactor, this is some 4AM code

  u32 immediate_amount = (instr.opcode & 0xff);                 // used for transfers using immediate values
  u8 shift_amount      = (instr.opcode & 0b111100000000) >> 8;  // immediate is rotated by this amount multiplied by two.

  bool modify_flag_field      = (instr.opcode & (1 << 19)) ? true : false;
  bool modify_status_field    = (instr.opcode & (1 << 18)) ? true : false;
  bool modify_extension_field = (instr.opcode & (1 << 17)) ? true : false;
  bool modify_control_field   = (instr.opcode & (1 << 16)) ? true : false;

  u32 op_value = 0;

  if (instr.I == 0) {  // 0 = Register, 1 = Immediate
    op_value = c.regs.get_reg(instr.Rm);
  } else {
    op_value = std::rotr(immediate_amount, shift_amount * 2);
  }

  switch (instr.P) {  // CPSR = 0, SPSR = 1
    case PSR::CPSR: {
      if (modify_flag_field) {
        c.regs.CPSR.value &= ~0xFF000000;
        c.regs.CPSR.value |= (op_value & 0xFF000000);

        // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
      }
      if (modify_status_field && c.regs.CPSR.MODE_BIT != USER) {
        c.regs.CPSR.value &= ~0x00FF0000;
        c.regs.CPSR.value |= (op_value & 0x00FF0000);
      }
      if (modify_extension_field && c.regs.CPSR.MODE_BIT != USER) {
        c.regs.CPSR.value &= ~0x0000FF00;
        c.regs.CPSR.value |= (op_value & 0x0000FF00);
      }

      if (modify_control_field && c.regs.CPSR.MODE_BIT != USER) {
        op_value |= 0x10;

        // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
        auto old_mode = c.regs.CPSR.STATE_BIT;

        c.regs.CPSR.value &= ~0x000000FF;
        c.regs.CPSR.value |= (op_value & 0x000000FF);
        c.regs.CPSR.value |= 0x10;

        if (old_mode != c.regs.CPSR.STATE_BIT) { c.flush_pipeline(); }
      }
      break;
    }

    case PSR::SPSR_MODE: {  // TODO: put this in handler functions, having this per case fully written out looks like dogshit
      // fmt::println("------ SPSR");
      switch (c.regs.CPSR.MODE_BIT) {
        case USER:
        case SYSTEM: {
          // assert(0);
          break;
        }
        case FIQ: {
          if (modify_flag_field) {
            c.regs.SPSR_fiq &= ~0xFF000000;
            c.regs.SPSR_fiq |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_fiq &= ~0x00FF0000;
            c.regs.SPSR_fiq |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_fiq &= ~0x0000FF00;
            c.regs.SPSR_fiq |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && c.regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = c.regs.CPSR.STATE_BIT;

            c.regs.SPSR_fiq &= ~0x000000FF;
            c.regs.SPSR_fiq |= (op_value & 0x000000FF);
            // c.regs.SPSR_fiq |= 0x10;

            // if (old_mode != c.regs.CPSR.STATE_BIT) { c.flush_pipeline(); }
          }

          break;
        }
        case IRQ: {
          // fmt::println("new value: {:#010x}", op_value);
          if (modify_flag_field) {
            c.regs.SPSR_irq &= ~0xFF000000;
            c.regs.SPSR_irq |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_irq &= ~0x00FF0000;
            c.regs.SPSR_irq |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_irq &= ~0x0000FF00;
            c.regs.SPSR_irq |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && c.regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("op value & 0xff: {:#010x}", op_value & 0xff);
            c.regs.SPSR_irq &= ~0x000000FF;
            c.regs.SPSR_irq |= (op_value & 0x000000FF);

            // c.regs.SPSR_irq |= 0x10;
          }
          break;
        }
        case SUPERVISOR: {
          if (modify_flag_field) {
            c.regs.SPSR_svc &= ~0xFF000000;
            c.regs.SPSR_svc |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_svc &= ~0x00FF0000;
            c.regs.SPSR_svc |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_svc &= ~0x0000FF00;
            c.regs.SPSR_svc |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && c.regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = c.regs.CPSR.STATE_BIT;

            c.regs.SPSR_svc &= ~0x000000FF;
            c.regs.SPSR_svc |= (op_value & 0x000000FF);
            // c.regs.SPSR_svc |= 0x10;

            // if (old_mode != c.regs.CPSR.STATE_BIT) { c.flush_pipeline(); }
          }
          break;
        }
        case ABORT: {
          // fmt::println("wrote to ABT");
          if (modify_flag_field) {
            c.regs.SPSR_abt &= ~0xFF000000;
            c.regs.SPSR_abt |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_abt &= ~0x00FF0000;
            c.regs.SPSR_abt |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_abt &= ~0x0000FF00;
            c.regs.SPSR_abt |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && c.regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = c.regs.CPSR.STATE_BIT;

            c.regs.SPSR_abt &= ~0x000000FF;
            c.regs.SPSR_abt |= (op_value & 0x000000FF);
            // c.regs.SPSR_abt |= 0x10;

            // if (old_mode != c.regs.CPSR.STATE_BIT) { c.flush_pipeline(); }
          }
          break;
        }
        case UNDEFINED: {
          if (modify_flag_field) {
            c.regs.SPSR_und &= ~0xFF000000;
            c.regs.SPSR_und |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_und &= ~0x00FF0000;
            c.regs.SPSR_und |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && c.regs.CPSR.MODE_BIT != USER) {
            c.regs.SPSR_und &= ~0x0000FF00;
            c.regs.SPSR_und |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && c.regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = c.regs.CPSR.STATE_BIT;

            c.regs.SPSR_und &= ~0x000000FF;
            c.regs.SPSR_und |= (op_value & 0x000000FF);
            // c.regs.SPSR_und |= 0x10;

            // if (old_mode != c.regs.CPSR.STATE_BIT) { c.flush_pipeline(); }
          }
          break;
        }
      }
      break;
    };

    default: {
      c.cpu_logger->critical("INVALID RM PASSED: {}", +instr.Rm);
    }
  }
}

void ARM::Instructions::STM(ARM7TDMI& c, InstructionInfo& instr) {  // REFACTOR: this could written much shorter
  u32 base_address = c.regs.get_reg(instr.Rn);                      // Base
  u16 Rlist_value  = 0;
  u32 value = 0;

  if (c.regs.CPSR.STATE_BIT == THUMB_MODE) {
    Rlist_value = instr.opcode & 0xFF;
  } else {
    Rlist_value = instr.opcode & 0xFFFF;
  }

  std::vector<u8> Rlist_vec;

  bool forced_writeback_empty_rlist = false;
  bool base_first_in_reg_list       = false;
  bool base_in_reg_list             = false;

  (void)(base_first_in_reg_list);
  (void)(forced_writeback_empty_rlist);

  // instr.print_params();

  for (size_t idx = 0; idx < 16; idx++) {
    if (Rlist_value & (1 << idx)) {
      // quirk: if the base register is the first register in the reglist, we writeback the old value of the base.
      if (idx == instr.Rn && Rlist_vec.empty()) { base_first_in_reg_list = true; }
      if (idx == instr.Rn && !Rlist_vec.empty()) { base_in_reg_list = true; }
      Rlist_vec.push_back(idx);
    }
  }

  if (instr.LR_BIT) { Rlist_vec.push_back(14); }

  if (Rlist_vec.empty()) {  // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+/-40h (ARMv4-v5)
    Rlist_vec.push_back(15);
    forced_writeback_empty_rlist = true;
    instr.W                      = 0;
  }

  // Handle forced writeback quirk
  if (forced_writeback_empty_rlist) {
    // fmt::println("{} - {}", instr.mnemonic, c.bus->cycles_elapsed);

    fmt::println("handling forced writeback P:{} U:{}", instr.P, instr.U);

    if (instr.U == 0) base_address -= 0x40;  // changed because of PUSH POP breaking

    if ((instr.P == 0 && instr.U == 0) || (instr.P && instr.U)) base_address += 0x4;

    u8 mode_dependent_offset = c.regs.CPSR.STATE_BIT == THUMB_MODE ? 2 : 4;

    u32 value = c.regs.r[15] + mode_dependent_offset;

    c.bus->write32(base_address, value);

    if ((instr.P == 0 && instr.U == 0) || (instr.P && instr.U)) base_address -= 0x4;

    if (instr.P && instr.U) base_address += 0x40;

    if (!instr.P && instr.U) base_address += 0x40;

    if (instr.S) {
      c.regs.r[instr.Rn] = base_address;
    } else {
      c.regs.get_reg(instr.Rn) = base_address;
    }

    fmt::println("done?");
    return;
  }

  // Check mode, and execute transfer
  if (instr.P == 1 && instr.U == 0) {  // Pre-decrement (DB)
    base_address -= (Rlist_vec.size() * 4);

    // fmt::println("STMDB base address: {:#010x}", base_address);

    if (base_in_reg_list && instr.W) {
      // fmt::println("pre transfer: base is in reglist, setting r{} to {:#010x}", instr.Rn, base_address);

      if (instr.S) {
        c.regs.r[instr.Rn] = base_address;
      } else {
        c.regs.get_reg(instr.Rn) = base_address;
      }
    }

    // fmt::println("r{} = {:#010x}", instr.Rn, c.regs.get_reg(instr.Rn));

    for (const auto& r_num : Rlist_vec) {
      // if (instr.Rn == r_num) { fmt::println("Rn{} IN STMDB: {:#010x}", +instr.Rn, c.regs.get_reg(instr.Rn)); }

      if (instr.S) {
        value = c.regs.r[r_num];
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      } else {
        value = c.regs.get_reg(r_num);
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      }

      base_address += 4;
    }

    if (instr.W && !base_in_reg_list && instr.U == 0) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address - (Rlist_vec.size() * 4);
      } else {
        c.regs.get_reg(instr.Rn) = base_address - (Rlist_vec.size() * 4);
      }
    }

    if (instr.W && !base_in_reg_list && instr.U == 1) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address;
      } else {
        c.regs.get_reg(instr.Rn) = base_address;
      }
    }

    // Handle forced writeback
    if (forced_writeback_empty_rlist) {}

    if (instr.Rn == 15 && instr.W) { c.flush_pipeline(); }
    return;
  }

  if (instr.P == 1 && instr.U == 1) {  // Pre-increment (IB)

    // Writeback base immediately -- do not defer (quirk: needs reference)
    if (base_in_reg_list && instr.W) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address + (Rlist_vec.size() * 4);
      } else {
        c.regs.get_reg(instr.Rn) = base_address + (Rlist_vec.size() * 4);
      }
    }

    for (const auto& r_num : Rlist_vec) {  // REFACTOR: we are rechecking conditional multiple times
      // fmt::println("about to write the value of: r{}", r_num);
      base_address += 4;
      if (instr.S) {  // Force write into user bank modes
        value = c.regs.r[r_num];
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      } else {  // write to whatever mode we're currently in
        value = c.regs.get_reg(r_num);
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      }
    }

    // sanity check
    // fmt::println("0 sanity check R{} post: {:#010x}", instr.Rn, c.regs.get_reg(instr.Rn));

    // if (instr.W && !base_in_reg_list) {
    //   fmt::println("prong");
    //   c.regs.get_reg(instr.Rn) = base_address;
    // }

    if (instr.W && !base_in_reg_list) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address;
      } else {
        c.regs.get_reg(instr.Rn) = base_address;
      }
    }

    if (instr.W && instr.Rn == 15) { c.flush_pipeline(); }
    // sanity check
    // fmt::println("1 sanity check R{} post: {:#010x}", instr.Rn, c.regs.get_reg(instr.Rn));

    return;
  }

  if (instr.P == 0 && instr.U == 0) {  // Post-decrement (DA)
    fmt::println("Rn {}: {:#010x}", instr.Rn, c.regs.get_reg(instr.Rn));

    // if (base_in_reg_list && instr.W) {
    //   fmt::println("s0 Rn {}: {:#010x}", instr.Rn, c.regs.get_reg(instr.Rn));
    //   c.regs.get_reg(instr.Rn) = base_address - (Rlist_vec.size() * 4);
    //   fmt::println("s0 Rn {}: {:#010x}", instr.Rn, c.regs.get_reg(instr.Rn));
    // }

    if (base_in_reg_list && instr.W) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address - (Rlist_vec.size() * 4);
      } else {
        c.regs.get_reg(instr.Rn) = base_address - (Rlist_vec.size() * 4);
      }
    }

    base_address -= (Rlist_vec.size() * 4);
    u32 value = 0;

    for (const auto& r_num : Rlist_vec) {  // REFACTOR: we are rechecking conditional multiple times
      // fmt::println("about to write the value of: r{}", r_num);
      base_address += 4;
      if (instr.S) {  // Write using values from user bank modes
        value = c.regs.r[r_num];
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      } else {  // write using values of whatever mode we're currently in
        value = c.regs.get_reg(r_num);
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      }
    }

    if (instr.W && !base_in_reg_list) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address - (Rlist_vec.size() * 4);
      } else {
        c.regs.get_reg(instr.Rn) = base_address - (Rlist_vec.size() * 4);
      }
    }

    if (instr.W && instr.Rn == 15) { c.flush_pipeline(); }
    // if (instr.W && !base_in_reg_list) { c.regs.get_reg(instr.Rn) = base_address; }

    fmt::println("s2 Rn {}: {:#010x}", instr.Rn, c.regs.get_reg(instr.Rn));

    return;
  }

  if (instr.P == 0 && instr.U == 1) {  // Post-increment (IA)
    if (base_in_reg_list && instr.W) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address + (Rlist_vec.size() * 4);
      } else {
        c.regs.get_reg(instr.Rn) = base_address + (Rlist_vec.size() * 4);
      }
    }

    u32 value = 0;

    for (const auto& r_num : Rlist_vec) {  // REFACTOR: we are rechecking conditional multiple times
      // fmt::println("about to write the value of: r{}", r_num);
      if (instr.S) {  // Force write into user bank modes
        value = c.regs.r[r_num];
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      } else {  // write to whatever mode we're currently in
        value = c.regs.get_reg(r_num);
        if (r_num == 15) { value = c.regs.r[15] + 4; }
        if (base_in_reg_list && instr.Rn == 15 && r_num == 15 && instr.W) value -= 4;
        c.bus->write32(base_address, value);
      }
      base_address += 4;
    }

    if (instr.W && !base_in_reg_list) {
      if (instr.S) {
        c.regs.r[instr.Rn] = base_address;
      } else {
        c.regs.get_reg(instr.Rn) = base_address;
      }
    }

    if (instr.Rn == 15 && instr.W) { c.flush_pipeline(); }

    return;
  }

  c.cpu_logger->error("invalid STM mode");
  assert(0);
  return;
}

void ARM::Instructions::UMULL(ARM7TDMI& c, InstructionInfo& instr) {
  u64 res = static_cast<uint64_t>(c.regs.get_reg(instr.Rm)) * c.regs.r[instr.Rs];

  c.regs.get_reg(instr.Rd) = res >> 32;
  c.regs.get_reg(instr.Rn) = res & 0xffffffff;

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();
    c.reset_carry();
  }
}

void ARM::Instructions::SMULL(ARM7TDMI& c, InstructionInfo& instr) {
  s32 rm  = static_cast<u32>(c.regs.get_reg(instr.Rm));
  s32 rs  = static_cast<u32>(c.regs.r[instr.Rs]);
  s64 res = (s64)rm * rs;

  SPDLOG_DEBUG("LO: {:#x}", lo);
  c.regs.get_reg(instr.Rn) = instr.Rn == 15 ? (u32)(res & 0xffffffff) + 4 : (u32)(res & 0xffffffff);
  c.regs.get_reg(instr.Rd) = (u32)(res >> 32);

  if (instr.S) {
    (static_cast<u64>(res) >= 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();
    c.reset_carry();
  }
}

void ARM::Instructions::SMLAL(ARM7TDMI& c, InstructionInfo& instr) {
  u64 rdhi = instr.Rd == 15 ? c.regs.get_reg(instr.Rd) + 4 : c.regs.get_reg(instr.Rd);
  u64 rdlo = instr.Rn == 15 ? c.regs.get_reg(instr.Rn) + 4 : c.regs.get_reg(instr.Rn);
  u32 _rm  = instr.Rm == 15 ? c.regs.get_reg(instr.Rm) + 4 : c.regs.get_reg(instr.Rm);
  u32 _rs  = instr.Rs == 15 ? c.regs.get_reg(instr.Rs) + 4 : c.regs.get_reg(instr.Rs);

  s64 rm = static_cast<s32>(_rm);
  s64 rs = static_cast<s32>(_rs);

  s64 big_number = (static_cast<u64>(rdhi) << 32) + rdlo;

  s64 res = (rm * rs) + big_number;

  c.regs.get_reg(instr.Rd) = res >> 32;
  c.regs.get_reg(instr.Rn) = res & 0xffffffff;

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();

    // assert(0);
  }
  c.reset_carry();
}

void ARM::Instructions::UMLAL(ARM7TDMI& c, InstructionInfo& instr) {
  // Rd = Hi
  // Rn = Lo

  u64 big_number = (static_cast<u64>(c.regs.get_reg(instr.Rd)) << 32) + c.regs.get_reg(instr.Rn);
  u64 res        = big_number + (static_cast<u64>(c.regs.get_reg(instr.Rm)) * static_cast<u64>(c.regs.r[instr.Rs]));

  c.regs.get_reg(instr.Rd) = res >> 32;
  c.regs.get_reg(instr.Rn) = res & 0xffffffff;

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();

    // assert(0);
  }
  c.reset_carry();
}

void ARM::Instructions::MLA(ARM7TDMI& c, InstructionInfo& instr) {
  auto _rm = instr.Rm == 15 ? c.regs.get_reg(instr.Rm) + 4 : c.regs.get_reg(instr.Rm);
  auto _rs = instr.Rs == 15 ? c.regs.get_reg(instr.Rs) + 4 : c.regs.get_reg(instr.Rs);
  auto _rn = instr.Rn == 15 ? c.regs.get_reg(instr.Rn) + 4 : c.regs.get_reg(instr.Rn);

  auto result = (_rm * _rs) + _rn;

  // seems to be the case for ARM mode, could be different in thumb
  if (instr.Rd == 15) result += 4;

  if (instr.S) {
    result == 0 ? c.set_zero() : c.reset_zero();
    result >= 0x80000000 ? c.set_negative() : c.reset_negative();
  }

  c.regs.get_reg(instr.Rd) = result;
}
void ARM::Instructions::MUL(ARM7TDMI& c, InstructionInfo& instr) {
  auto _rm = instr.Rm == 15 ? c.regs.get_reg(instr.Rm) + 4 : c.regs.get_reg(instr.Rm);
  auto _rs = instr.Rs == 15 ? c.regs.get_reg(instr.Rs) + 4 : c.regs.get_reg(instr.Rs);

  auto result = (_rm * _rs);

  // seems to be the case for ARM mode, could be different in thumb
  if (instr.Rd == 15) result += 4;

  if (instr.S) {
    result == 0 ? c.set_zero() : c.reset_zero();
    result >= 0x80000000 ? c.set_negative() : c.reset_negative();

    c.reset_carry();
  }
  c.regs.get_reg(instr.Rd) = result;
}

// void ARM::InstructionInfo::LDM_THUMB(ARM7TDMI& c, InstructionInfo& _instr) {

// }

void ARM::Instructions::LDM(ARM7TDMI& c, InstructionInfo& _instr) {
  InstructionInfo instr = _instr;  // When the pipeline gets flushed the reference gets invalidated, do a copy.

  u32 address           = c.regs.get_reg(instr.Rn);  // Base
  u32 operating_address = address;
  u16 Rlist_value       = instr.opcode & 0xFFFF;
  u8 pass               = 0;

  if (c.regs.CPSR.STATE_BIT == THUMB_MODE) { Rlist_value = instr.opcode & 0xFF; }

  // instr.print_params();

  std::vector<u8> Rlist_vec;

  bool r15_in_rlist              = false;
  bool forced_writeback          = false;
  bool base_in_reg_list          = false;
  bool pipeline_has_been_flushed = false;

  // Load Rlist values into vector, check for any conditions used at transfer.
  for (size_t idx = 0; idx < 16; idx++) {
    if (Rlist_value & (1 << idx)) {
      if (idx == instr.Rn) { base_in_reg_list = true; }
      if (idx == 15) { r15_in_rlist = true; }
      Rlist_vec.push_back(idx);
    }
  }

  // In THUMB mode, the PC-bit can be set, which means that the PC is pushed into the register list.
  if (instr.PC_BIT) {
    Rlist_vec.push_back(15);
    r15_in_rlist = true;
  }

  // If the Rlist is empty, there are some hardware quirks, set flags to be used at transfer.
  if (Rlist_vec.empty()) {
    // fmt::println("Rlist empty");
    Rlist_vec.push_back(15);
    r15_in_rlist     = true;
    forced_writeback = true;
    instr.W          = 0;
  }

  bool use_userbanks = instr.S && !r15_in_rlist;

  (void)base_in_reg_list;
  (void)forced_writeback;
  // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

  operating_address -= Rlist_vec.size() * 4;

  if (instr.U == 1) operating_address += Rlist_vec.size() * 4;
  if (instr.P && instr.U) operating_address += 4;  // LDMIB
  if (instr.P == 0 && instr.U == 0) operating_address += 4;
  // if (instr.P == 0 && instr.U) return;  // LDMIA

  // Transfer
  for (const auto& r_num : Rlist_vec) {  // Iterate through Register List

    if (use_userbanks) {
      // fmt::println("using userbanks....");
      c.regs.r[r_num] = c.bus->read32(operating_address + (pass * 4));
    } else {
      c.regs.get_reg(r_num) = c.bus->read32(operating_address + (pass * 4));
    }

    pass++;
  }

  // fmt::println("base in reg list: {}", base_in_reg_list);

  if (instr.W && !base_in_reg_list) {  // This is the deferred writeback, only executes if base register is not in register list.
    if (instr.U == 0) {
      if (use_userbanks) {
        c.regs.r[instr.Rn] = address - (Rlist_vec.size() * 4);
      } else {
        c.regs.get_reg(instr.Rn) = address - (Rlist_vec.size() * 4);
      }
    }

    if (instr.U == 1) {
      if (use_userbanks) {
        c.regs.r[instr.Rn] = address + (Rlist_vec.size() * 4);
      } else {
        c.regs.get_reg(instr.Rn) = address + (Rlist_vec.size() * 4);
      }
    }

    // if (instr.U == 1) { c.regs.get_reg(instr.Rn) = address + (Rlist_vec.size() * 4); }
  }

  if (forced_writeback) {
    if (use_userbanks) {
      c.regs.r[instr.Rn] += 0x40;
    } else {
      c.regs.get_reg(instr.Rn) += 0x40;
    }
  }

  if ((instr.W && instr.Rn == 15) || forced_writeback) {
    pipeline_has_been_flushed = true;
    c.regs.r[15]              = c.align_by_current_mode(c.regs.r[15]);
    c.flush_pipeline();
    fmt::println("new PC: {:#010X}", c.regs.r[15]);
  }

  if (r15_in_rlist) {
    // fmt::println("hi");
    // instr.print_params();
    if (instr.S) {
      fmt::println("loading SPSR!");
      c.regs.load_spsr_to_cpsr();
    }

    if (!pipeline_has_been_flushed) {
      fmt::println("pipeline not flushed, flushing.");
      c.regs.r[15] = c.align_by_current_mode(c.regs.r[15]);
      c.flush_pipeline();
    }
  }
  return;

  SPDLOG_CRITICAL("invalid LDM mode");
  assert(0);
}

void ARM::Instructions::SWP(ARM7TDMI& c, InstructionInfo& instr) {
  u32 swap_value                  = c.regs.get_reg(instr.Rm);
  u32 address                     = c.regs.get_reg(instr.Rn);
  u8 misaligned_load_rotate_value = (address & 3) * 8;

  if (instr.Rm == 15) swap_value += 4;
  if (instr.Rn == 15) address += 4;

  if (misaligned_load_rotate_value) { fmt::println("[SWP] misaligned rotate value: {}", misaligned_load_rotate_value); }

  if (instr.B) {
    u8 byte_at_loc           = c.bus->read8(address);
    c.regs.get_reg(instr.Rd) = byte_at_loc;
    c.bus->write8(address, swap_value);
  } else {
    // address                  = c.align(address, WORD);  // Word writes must be aligned, and the value read must be rotated
    u32 data                 = c.bus->read32(address);
    c.regs.get_reg(instr.Rd) = std::rotr(data, misaligned_load_rotate_value);
    c.bus->write32(address, swap_value);
  }

  if (instr.Rd == 15) {
    c.regs.r[15] = c.align_by_current_mode(c.regs.r[15]);
    c.flush_pipeline();
  }
}

void ARM::Instructions::BLL(ARM7TDMI& c, InstructionInfo& instr) {
  u32 offset = (instr.op2 << 12);
  if (offset >= 0x400000) { offset -= 0x800000; }
  c.regs.get_reg(14) = c.regs.r[15] + offset;
}

void ARM::Instructions::BLH(ARM7TDMI& c, InstructionInfo& instr) {
  u32 pc = c.regs.get_reg(15);
  u32 lr = c.regs.get_reg(14);

  c.regs.get_reg(14) = (pc - 2) | 1;
  c.regs.get_reg(15) = lr + (instr.op2 << 1);

  c.flush_pipeline();
}

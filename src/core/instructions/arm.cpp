#include "instructions/arm.hpp"

#include <cassert>
#include "cpu.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"

bool is_negative(u32 v) { return v >= 0x80000000; }
bool is_zero(u32 v) { return v == 0; }

void ARM::Instructions::B(ARM7TDMI& c, instruction_info& instr) {
  if (instr.L) { c.regs.r[14] = c.regs.r[15] - 4; }

  c.regs.r[15] += (instr.offset);

  c.regs.r[15] &= ~1;

  // SPDLOG_DEBUG("{:#x}", c.regs.r[15]);

  c.flush_pipeline();
}

void ARM::Instructions::BX(ARM7TDMI& c, instruction_info& instr) {
  // 0001b: BX{cond}  Rn    ;PC=Rn, T=Rn.0   (ARMv4T and ARMv5 and up)

  // c.regs.set_reg(15, c.regs.r[instr.Rn] & ~1);
  c.regs.r[15] = (c.regs.r[instr.Rn] & ~1);

  // SPDLOG_INFO("R15: {:#010x}", c.regs.r[15]);

  c.regs.CPSR.STATE_BIT = (CPU_MODE)(c.regs.r[instr.Rn] & 0b1);

  // c.regs.CPSR.STATE_BIT ? SPDLOG_INFO("entered THUMB mode") : SPDLOG_INFO("entered ARM mode");

  c.flush_pipeline();

  return;
}

void ARM::Instructions::NOP([[gnu::unused]] ARM7TDMI& c, [[gnu::unused]] instruction_info& instr) { return; }
void ARM::Instructions::AND(ARM7TDMI& c, instruction_info& instr) {
  // instr.print_params();
  instr.op2 = c.handle_shifts(instr);

  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] & instr.op2;

  if (instr.S) {
    is_negative(c.regs.r[instr.Rd]) ? c.set_negative() : c.reset_negative();
    is_zero(c.regs.r[instr.Rd]) ? c.set_zero() : c.reset_zero();
  }

  return;
}

void ARM::Instructions::EOR(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr);

  u32 result = c.regs.r[instr.Rn] ^ instr.op2;

  if (instr.S) {
    is_zero(result) ? c.set_zero() : c.reset_zero();
    is_negative(result) ? c.set_negative() : c.reset_negative();
  }

  c.regs.r[instr.Rd] = result;
}
void ARM::Instructions::BIC(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr);

  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] & ~(instr.op2);

  if (instr.S) {
    is_negative(c.regs.r[instr.Rd]) ? c.set_negative() : c.reset_negative();
    is_zero(c.regs.r[instr.Rd]) ? c.set_zero() : c.reset_zero();
  }
}

void ARM::Instructions::RSB(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr, false);

  u32 result = instr.op2 - c.regs.r[instr.Rn];

  u32 op2_msb = instr.op2 & 0x80000000;
  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (instr.S) {
    is_zero(result) ? c.set_zero() : c.reset_zero();
    is_negative(result) ? c.set_negative() : c.reset_negative();
    ((op2_msb ^ res_msb) & (op2_msb ^ rn_msb)) ? c.set_overflow() : c.reset_overflow();
    instr.op2 >= c.regs.r[instr.Rn] ? c.set_carry() : c.reset_carry();
  }

  c.regs.r[instr.Rd] = result;
}

void ARM::Instructions::SUB(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr, false);

  // fmt::println("[SUB] Rn: {}", c.regs.r[instr.Rn]);
  // fmt::println("[SUB] Op2: {}", instr.op2);
  // fmt::println("[SUB] R: {}", (c.regs.r[instr.Rn] - instr.op2));
  u32 result = c.regs.r[instr.Rn] - instr.op2;

  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (instr.S) {
    is_negative((c.regs.r[instr.Rn] - instr.op2)) ? c.set_negative() : c.reset_negative();
    is_zero((c.regs.r[instr.Rn] - instr.op2)) ? c.set_zero() : c.reset_zero();
    
    c.regs.r[instr.Rn] >= instr.op2 ? c.set_carry() : c.reset_carry();
    (rn_msb != rm_msb && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.r[instr.Rd] = result;

  if (instr.Rd == 15) {
    c.regs.r[instr.Rd] &= ~3;
    c.flush_pipeline();
  }
}

void ARM::Instructions::SBC(ARM7TDMI& c, instruction_info& instr) {
  bool old_carry = c.regs.CPSR.CARRY_FLAG;
  instr.op2      = c.handle_shifts(instr, false);

  // SPDLOG_DEBUG("INSTR RN: {} - Rn V {:#x}", +instr.Rn, c.regs.r[instr.Rn]);
  // SPDLOG_DEBUG("INSTR RM REG: {} - op2: {:#x}", +instr.Rm, instr.op2);

  u32 result = c.regs.r[instr.Rn] - instr.op2 + (old_carry - 1);

  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (instr.S) {
    is_negative(result) ? c.set_negative() : c.reset_negative();
    is_zero(result) ? c.set_zero() : c.reset_zero();

    (c.regs.r[instr.Rn] >= ((u64)instr.op2 + (1 - old_carry))) ? c.set_carry() : c.reset_carry();

    ((rn_msb != rm_msb) && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();
  }

  c.regs.r[instr.Rd] = result;
}

void ARM::Instructions::MVN(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr);

  if (instr.S) {
    ~instr.op2 == 0 ? c.set_zero() : c.reset_zero();
    (~instr.op2 & 0x80000000) != 0 ? c.set_negative() : c.reset_negative();
  }

  c.regs.r[instr.Rd] = ~instr.op2;
}

void ARM::Instructions::ADC(ARM7TDMI& c, instruction_info& instr) {
  // instr.print_params();
  bool o_carry = c.regs.CPSR.CARRY_FLAG;  // Carry from barrel shifter should
                                          // not be used for this instruction.
  instr.op2 = c.handle_shifts(instr, false);

  u64 result_64 = ((u64)c.regs.r[instr.Rn] + instr.op2 + o_carry);
  u32 result_32 = (c.regs.r[instr.Rn] + instr.op2 + o_carry);

  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 rm_msb  = c.regs.r[instr.Rm] & 0x80000000;
  u32 res_msb = result_32 & 0x80000000;

  if (instr.S) {
    result_32 == 0 ? c.set_zero() : c.reset_zero();
    ((rn_msb == rm_msb) && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();
    (result_64 > U32_MAX) ? c.set_carry() : c.reset_carry();
    is_negative(result_32) ? c.set_negative() : c.reset_negative();
  }

  c.regs.r[instr.Rd] = (c.regs.r[instr.Rn] + instr.op2 + o_carry);
}

void ARM::Instructions::TEQ(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr);

  u32 m = c.regs.r[instr.Rn] ^ instr.op2;

  is_negative(m) ? c.set_negative() : c.reset_negative();
  is_zero(m) ? c.set_zero() : c.reset_zero();
}

void ARM::Instructions::MOV(ARM7TDMI& c, instruction_info& instr) {
  // instr.print_params();
  instr.op2 = c.handle_shifts(instr, instr.S);

  if (instr.S) {
    is_zero(instr.op2) ? c.set_zero() : c.reset_zero();
    is_negative(instr.op2) ? c.set_negative() : c.reset_negative();
    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.r[instr.Rd] = instr.op2;

  if (instr.Rd == 15) {
    // c.regs.r[instr.Rd] +=&= ~3;
    c.flush_pipeline();
  }
}
void ARM::Instructions::ADD(ARM7TDMI& c, instruction_info& instr) {
  // ADD{cond}{S} Rd,Rn,Op2 ;* ;add               Rd = Rn+Op2
  // instr.print_params();
  // SPDLOG_DEBUG("Rn: {}", +instr.Rn);
  // SPDLOG_DEBUG("Rm: {}", +instr.Rm);

  instr.op2 = c.handle_shifts(instr, false);

  u64 result = (u64)c.regs.r[instr.Rn] + instr.op2;
  u32 r_32   = c.regs.r[instr.Rn] + instr.op2;

  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  // fmt::println("RN: {:#010x}", c.regs.r[instr.Rn]);
  // fmt::println("OP2: {:#010x}", instr.op2);
  // fmt::println("R64: {:#010x}", result);

  if (instr.S) {
    r_32 == 0 ? c.set_zero() : c.reset_zero();
    ((rn_msb == rm_msb) && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();
    (result > U32_MAX) ? c.set_carry() : c.reset_carry();
    is_negative(r_32) ? c.set_negative() : c.reset_negative();
  }

  if (c.regs.CPSR.STATE_BIT == THUMB_MODE) {
    if (instr.Rn == 15) { r_32 &= ~2; }
  }
  
  c.regs.r[instr.Rd] = r_32;
  // TODO: force align address if R15
  if (instr.Rd == 15) {
    c.regs.r[instr.Rd] &= ~3;
    c.flush_pipeline();
  }
}

void ARM::Instructions::TST(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr);

  u32 d = c.regs.r[instr.Rn] & instr.op2;

  is_zero(d) ? c.set_zero() : c.reset_zero();
  is_negative(d) ? c.set_negative() : c.reset_negative();
}

void ARM::Instructions::STRH(ARM7TDMI& c, instruction_info& instr) {
  u32 address = c.regs.r[instr.Rn];

  // u8 immediate_offset = (((instr.opcode & 0xf00) >> 8) << 4) + (instr.opcode & 0xf);
  
  u32 shifted_reg_offset = 0;
  if (instr.I == 0) { shifted_reg_offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.r[instr.Rm], instr.shift_amount, false, true, false); }

  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register
  // instr.print_params();

  if (instr.P == 0) {
    // address = c.align_address(address, HALFWORD);
    address = c.align_address(address, HALFWORD);
    c.bus->write16(address, c.regs.r[instr.Rd] & 0xffff);

    if (instr.U) {
      c.regs.r[instr.Rn] += (instr.I ? instr.offset : shifted_reg_offset);
      address = c.regs.r[instr.Rn];
    } else {
      c.regs.r[instr.Rn] -= (instr.I ? instr.offset : shifted_reg_offset);
      address = c.regs.r[instr.Rn];
    }

  } else {
    bool base_is_destination = false;
    u32 written_val          = c.regs.r[instr.Rd];
    if (instr.Rd == instr.Rn) { base_is_destination = true; }

    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += (instr.I ? instr.offset : shifted_reg_offset);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] + (instr.I ? instr.offset : shifted_reg_offset);
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= (instr.I ? instr.offset : shifted_reg_offset);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] - (instr.I ? instr.offset : shifted_reg_offset);
      }
    }

    address = c.align_address(address, HALFWORD);
    c.bus->write16(address, base_is_destination ? written_val : c.regs.r[instr.Rd] & 0xffff);
  }
}

void ARM::Instructions::STR(ARM7TDMI& c, instruction_info& instr) {
  // instr.print_params();
  // assert(0);
  u32 address = c.regs.r[instr.Rn];

  // if (instr.B) throw std::runtime_error("strb not implemented");
  // assert(instr.I == 0);

  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register
  // instr.print_params();

  // SPDLOG_DEBUG("[STR] RD: {}", +instr.Rd);
  u8 added_value = 0;
  if (instr.Rd == 15) { added_value += 4; }

  if (instr.P == 0) {
    // SPDLOG_DEBUG("[STR] P is 0!");

    if (instr.B) {
      c.bus->write8(address, c.regs.r[instr.Rd]);
    } else {
      address = c.align_address(address, WORD);
      c.bus->write32(address, c.regs.r[instr.Rd] + added_value);
    }

    if (instr.U) {
      c.regs.r[instr.Rn] += (instr.I == 0 ? instr.offset : c.regs.r[instr.Rm]);
      address = c.regs.r[instr.Rn];
    } else {
      c.regs.r[instr.Rn] -= (instr.I == 0 ? instr.offset : c.regs.r[instr.Rm]);
      address = c.regs.r[instr.Rn];
    }

  } else {
    // SPDLOG_DEBUG("[STR] P is 1!");
    bool base_is_destination = false;
    u32 written_val          = c.regs.r[instr.Rd];
    if (instr.Rd == instr.Rn) { base_is_destination = true; }

    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += (instr.I == 0 ? instr.offset : c.regs.r[instr.Rm]);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] + (instr.I == 0 ? instr.offset : c.regs.r[instr.Rm]);
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= (instr.I == 0 ? instr.offset : c.regs.r[instr.Rm]);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] - (instr.I == 0 ? instr.offset : c.regs.r[instr.Rm]);
      }
    }

    if (instr.B) {
      c.bus->write8(address, base_is_destination ? written_val : c.regs.r[instr.Rd]);
    } else {
      address = c.align_address(address, WORD);
      c.bus->write32(address, base_is_destination ? written_val : c.regs.r[instr.Rd] + added_value);
    }

    // c.bus->write32(address, c.regs.r[instr.Rd]);
  }
}

void ARM::Instructions::CMN(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr, false);

  u64 r_64 = (u64)c.regs.r[instr.Rn] + instr.op2;

  u32 r = c.regs.r[instr.Rn] + instr.op2;

  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 rm_msb  = c.regs.r[instr.Rm] & 0x80000000;
  u32 res_msb = r & 0x80000000;

  // fmt::println("[R64] {:#010x}", (u64)c.regs.r[instr.Rn] + instr.op2);

  is_negative(r) ? c.set_negative() : c.reset_negative();
  is_zero(r) ? c.set_zero() : c.reset_zero();
  (r_64 > U32_MAX) ? c.set_carry() : c.reset_carry();
  (rn_msb == rm_msb && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();
}

void ARM::Instructions::RSC(ARM7TDMI& c, instruction_info& instr) {
  bool old_carry = c.regs.CPSR.CARRY_FLAG;
  instr.op2      = c.handle_shifts(instr, false);
  // int l          = (instr.op2 - c.regs.r[instr.Rn]) + (old_carry - 1);

  c.regs.r[instr.Rd] = (instr.op2 - c.regs.r[instr.Rn]) + (old_carry - 1);

  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 rm_msb  = c.regs.r[instr.Rm] & 0x80000000;
  u32 res_msb = c.regs.r[instr.Rd] & 0x80000000;

  if (instr.S) {
    c.regs.r[instr.Rd] >= 0x80000000 ? c.set_negative() : c.reset_negative();
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
    instr.op2 >= (c.regs.r[instr.Rn] + (1 - old_carry)) ? c.set_carry() : c.reset_carry();
    ((rn_msb != rm_msb) && (rm_msb != res_msb)) ? c.set_overflow() : c.reset_overflow();
  }

  if (instr.Rd == 15) {
    c.regs.r[instr.Rd] &= ~3;
    c.flush_pipeline();
  }
}
void ARM::Instructions::LDR(ARM7TDMI& c,
                            instruction_info& _instr) {  // TODO: Re-write this, too long and unnecessarily complex
  instruction_info instr = _instr;                       // Pipeline flush invalidates reference due
                                                         // to pipeline change, copy necessary
  u32 shifted_register_value = 0;
  if (instr.I) { shifted_register_value = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.r[instr.Rm], instr.shift_amount, true, false, false); }

  u32 c_address = c.regs.r[instr.Rn];

  if (instr.pc_relative) {
    // SPDLOG_DEBUG("instr offset: {}", instr.offset);
    c_address = (c.regs.r[15] + instr.offset) & ~2;
    // SPDLOG_DEBUG("read address: {:#010x}", c_address);
    c.regs.r[instr.Rd] = c.bus->read32(c_address);
    // c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);
    return;
  }

  u8 misaligned_load_rotate_value = (c_address & 3) * 8;

  if (instr.P == 0) {  // Forced writeback

    // SPDLOG_DEBUG("[LDR] P = 0");

    if (instr.B) {
      c.regs.r[instr.Rd] = c.bus->read8(c_address);
    } else {
      misaligned_load_rotate_value = (c_address & 3) * 8;
      if (misaligned_load_rotate_value) { SPDLOG_DEBUG("misaligned rotate value: {}", misaligned_load_rotate_value); }

      c_address = c.align_address(c_address, WORD);  // word loads are forcibly aligned to the nearest word address
      // SPDLOG_DEBUG("loading from {:#010x}", c_address);

      c.regs.r[instr.Rd] = c.bus->read32(c_address);
      c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);  // Rotate new contents if address was misaligned
      // SPDLOG_DEBUG("new r{}: {:#010x}", +instr.Rd, c.regs.r[instr.Rd]);
    }

    // flush pipeline if PC changed, refill with new instructions
    if (instr.Rd == 15) { c.flush_pipeline(); }

    // Writeback logic
    if (!(instr.Rd == instr.Rn)) {  // Don't writeback to base if destination is same as base
      if (instr.U) {
        c.regs.r[instr.Rn] += (instr.I ? shifted_register_value : instr.offset);
      } else {
        c.regs.r[instr.Rn] -= (instr.I ? shifted_register_value : instr.offset);
      }
    }

  } else {
    // SPDLOG_DEBUG("[LDR] P = 1");
    // SPDLOG_DEBUG("[LDR] U = {}", +instr.U);
    // SPDLOG_DEBUG("[LDR] I = {}", +instr.I);
    // SPDLOG_DEBUG("PRE: {:#x}", c_address);
    // SPDLOG_DEBUG("IMMEDIATE: {:#x}", instr.offset);
    // SPDLOG_DEBUG("SHIFTED REG VALUE: {:#x}", shifted_register_value);

    // Write-back logic
    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += (instr.I ? shifted_register_value : instr.offset);
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] + (instr.I ? shifted_register_value : instr.offset);
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= (instr.I ? shifted_register_value : instr.offset);
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] - (instr.I ? shifted_register_value : instr.offset);
      }
    }

    if (instr.B) {
      c.regs.r[instr.Rd] = c.bus->read8(c_address);
    } else {
      misaligned_load_rotate_value = (c_address & 3) * 8;
      // SPDLOG_DEBUG("[LDR] loading from {:#010x}", c_address);
      if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[LDR] misaligned rotate value: {}", misaligned_load_rotate_value); }

      c_address          = c.align_address(c_address, WORD);
      c.regs.r[instr.Rd] = c.bus->read32(c_address);
      c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);
    }

    if (instr.Rd == 15) { c.flush_pipeline(); }
  }
}

void ARM::Instructions::LDRSB(ARM7TDMI& c,
                              instruction_info& _instr) {  // TODO: Re-write this, too long and unnecessarily complex
  // Rn - base register
  // Rd - Source / Destination Register (where we're writing to)

  // LDR - Load data from memory into register [Rd]
  // instr.print_params();

  // assert(instr.I == 0);

  instruction_info instr = _instr;  // Pipeline flush invalidates reference due
                                    // to pipeline change, copy necessary

  u32 shifted_reg_offset = 0;
  if (instr.I == 0) { shifted_reg_offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.r[instr.Rm], instr.shift_amount, false, true, false); }

  u32 c_address = c.regs.r[instr.Rn];

  if (instr.P == 0) {  // Forced writeback
    c.regs.r[instr.Rd] = c.bus->read8(c_address);

    c.regs.r[instr.Rd] &= ~0xffffff00;
    if ((c.regs.r[instr.Rd] & (1 << 7))) { c.regs.r[instr.Rd] |= 0xffffff00; }

    if (instr.Rd == 15) { c.flush_pipeline(); }


    if (!(instr.Rd == instr.Rn)) {
      if (instr.U) {
        c.regs.r[instr.Rn] += (instr.I ? instr.offset : shifted_reg_offset);
        // address = c.regs.r[instr.Rn];
      } else {
        c.regs.r[instr.Rn] -= (instr.I ? instr.offset : c.regs.r[instr.Rm]);
        // address = c.regs.r[instr.Rn];
      }
    }
  } else {
    // SPDLOG_DEBUG("[LDRSB] P = 1");
    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += (instr.I ? instr.offset : c.regs.r[instr.Rm]);
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] + (instr.I ? instr.offset : c.regs.r[instr.Rm]);
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= (instr.I ? instr.offset : c.regs.r[instr.Rm]);
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] - (instr.I ? instr.offset : c.regs.r[instr.Rm]);
      }
    }

    // SPDLOG_DEBUG("ADDRESS: {:#X}", c_address);

    c.regs.r[instr.Rd] = c.bus->read8(c_address);

    c.regs.r[instr.Rd] &= ~0xffffff00;
    if ((c.regs.r[instr.Rd] & (1 << 7))) { c.regs.r[instr.Rd] |= 0xffffff00; }
    // SPDLOG_DEBUG("new r{}: {:#010x}", +instr.Rd, c.regs.r[instr.Rd]);

    if (instr.Rd == 15) { c.flush_pipeline(); }
  }
}

void ARM::Instructions::LDRSH(ARM7TDMI& c,
                              instruction_info& instr) {  // TODO: implement writeback logic

  u32 address = c.regs.r[instr.Rn];  // Read address from base register

  u8 immediate_offset             = (((instr.opcode & 0xf00) >> 8) << 4) + (instr.opcode & 0xf);
  u8 misaligned_load_rotate_value = 0;

  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register
  // instr.print_params();

  if (instr.P == 0) {
    // misaligned_load_rotate_value = (address & 1) * 8;

    // if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[LDRSH] misaligned rotate value: {}", misaligned_load_rotate_value); }

    // address = c.align_address(address,
    //                           HALFWORD);  // Force align address before reading.

    if ((address & 1) != 0) {  // check if address is misaligned
      SPDLOG_DEBUG("[LDRSH] address is misaligned");
      c.regs.r[instr.Rd] = c.bus->read8(address);
      if ((c.regs.r[instr.Rd] & (1 << 7))) {  // Sign extend
        c.regs.r[instr.Rd] |= 0xffffff00;
      }
    } else {
      c.regs.r[instr.Rd] = c.bus->read16(address);

      c.regs.r[instr.Rd] &= ~0xffff0000;
      if ((c.regs.r[instr.Rd] & (1 << 15))) { c.regs.r[instr.Rd] |= 0xffff0000; }

      c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);
    }
    // SPDLOG_DEBUG("new r{}: {:#010x}", +instr.Rd, c.regs.r[instr.Rd]);

    if (instr.Rd == 15) { c.flush_pipeline(); }

    if (instr.U) {                                                              // On post increment, write back is always enabled.
      c.regs.r[instr.Rn] += (instr.I ? immediate_offset : c.regs.r[instr.Rm]);  // Increase base register (post indexing)
      address = c.regs.r[instr.Rn];
    } else {
      c.regs.r[instr.Rn] -= (instr.I ? immediate_offset : c.regs.r[instr.Rm]);  // Decrease base register (post indexing)
      address = c.regs.r[instr.Rn];
    }

  } else {
    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] + (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] - (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      }
    }

    if ((address & 1) != 0) {  // check if address is misaligned
      SPDLOG_DEBUG("[LDRSH] address is misaligned");
      c.regs.r[instr.Rd] = c.bus->read8(address);
      if ((c.regs.r[instr.Rd] & (1 << 7))) {  // Sign extend
        c.regs.r[instr.Rd] |= 0xffffff00;
      }

    } else {
      c.regs.r[instr.Rd] = c.bus->read16(address);
      SPDLOG_DEBUG("before unsetting top 16 bits: {:#010x}", c.regs.r[instr.Rd]);
      // c.regs.r[instr.Rd] &= ~0xffff0000;
      if ((c.regs.r[instr.Rd] & (1 << 15))) { c.regs.r[instr.Rd] |= 0xffff0000; }

      c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);

      // SPDLOG_DEBUG("new r{}: {:#010x}", +instr.Rd, c.regs.r[instr.Rd]);

      if (instr.Rd == 15) { c.flush_pipeline(); }
    }
  }
}

void ARM::Instructions::LDRH(ARM7TDMI& c,
                             instruction_info& instr) {  // TODO: implement writeback logic

  u32 address = c.regs.r[instr.Rn];  // Read address from base register

  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register
  // instr.print_params();
  u32 shifted_reg_offset = 0;
  if (instr.I == 0) { shifted_reg_offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.r[instr.Rm], instr.shift_amount, false, true, false); }

  u8 misaligned_load_rotate_value = (address & 1) * 8;

  if (instr.P == 0) {
    misaligned_load_rotate_value = (address & 1) * 8;

    // if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[LDRH] misaligned rotate value: {}", misaligned_load_rotate_value); }

    // SPDLOG_DEBUG("[LDRH] loading from {:#010x}", c.align_address(address, HALFWORD));
    address            = c.align_address(address, HALFWORD);
    c.regs.r[instr.Rd] = c.bus->read16(address);

    c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);

    if (instr.Rd == 15) { c.flush_pipeline(); }

    if (!(instr.Rd == instr.Rn)) {
      if (instr.U) {                                                          // On post increment, write back is always enabled.
        c.regs.r[instr.Rn] += (instr.I ? instr.offset : shifted_reg_offset);  // Increase base register (post indexing)
        address = c.regs.r[instr.Rn];
      } else {
        c.regs.r[instr.Rn] -= (instr.I ? instr.offset : shifted_reg_offset);  // Decrease base register (post indexing)
        address = c.regs.r[instr.Rn];
      }
    }
  } else {
    // SPDLOG_DEBUG("[LDRH] P = {}", +instr.P);
    // SPDLOG_DEBUG("[LDRH] U = {}", +instr.U);
    // SPDLOG_DEBUG("[LDRH] I = {}", +instr.I);
    // SPDLOG_DEBUG("[LDRH] immediate offset = {}", +instr.offset);
    // SPDLOG_DEBUG("[LDRH] register offset = {}", shifted_reg_offset);

    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += (instr.I ? instr.offset : shifted_reg_offset);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] + (instr.I ? instr.offset : shifted_reg_offset);
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= (instr.I ? instr.offset : shifted_reg_offset);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] - (instr.I ? instr.offset : shifted_reg_offset);
      }
    }

    misaligned_load_rotate_value = (address & 1) * 8;

    if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[LDRH] misaligned rotate value: {}", misaligned_load_rotate_value); }

    // SPDLOG_DEBUG("[LDRH] loading from {:#010x}", c.align_address(address, HALFWORD));
    address            = c.align_address(address, HALFWORD);
    c.regs.r[instr.Rd] = c.bus->read16(address);

    c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);

    if (instr.Rd == 15) { c.flush_pipeline(); }
  }
}

void ARM::Instructions::CMP(ARM7TDMI& c, instruction_info& instr) {
  // instr.print_params();
  //  CMP - set condition codes on Op1 - Op2

  instr.op2 = c.handle_shifts(instr, false);

  u32 r    = c.regs.r[instr.Rn] - instr.op2;
  // u64 r_64 = (u64)c.regs.r[instr.Rn] - instr.op2;

  u32 rn_msb  = c.regs.r[instr.Rn] & 0x80000000;
  u32 rm_msb  = instr.op2 & 0x80000000;
  u32 res_msb = r & 0x80000000;
  // u64 r64_msb = r_64 & 0x80000000;
  // SPDLOG_DEBUG("r64 overflow: {}", rn_msb != rm_msb && rn_msb != r64_msb);
  // SPDLOG_TRACE("[CMP] r: {:#010x}", r);
  is_zero(r) ? c.set_zero() : c.reset_zero();
  is_negative(r) ? c.set_negative() : c.reset_negative();
  (rn_msb != rm_msb && rn_msb != res_msb) ? c.set_overflow() : c.reset_overflow();
  (c.regs.r[instr.Rn] >= instr.op2) ? c.set_carry() : c.reset_carry();

  if (instr.Rd == 15 && instr.S) { c.regs.load_spsr_to_cpsr(); }
}
void ARM::Instructions::ORR(ARM7TDMI& c, instruction_info& instr) {
  instr.op2 = c.handle_shifts(instr, instr.S);

  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] | instr.op2;

  if (instr.S) {
    is_zero(c.regs.r[instr.Rd]) ? c.set_zero() : c.reset_zero();
    is_negative(c.regs.r[instr.Rd]) ? c.set_negative() : c.reset_negative();
  }
}
void ARM::Instructions::MRS(ARM7TDMI& c, instruction_info& instr) { c.regs.r[instr.Rd] = (instr.P ? c.regs.get_spsr(c.regs.CPSR.MODE_BITS) : c.regs.CPSR.value); }

void ARM::Instructions::SWI(ARM7TDMI& c, [[gnu::unused]] instruction_info& instr) {
  SPDLOG_DEBUG("SWI CALLED");
  
  c.regs.svc_r[14] = c.regs.r[15] - 2;
  c.regs.SPSR_svc  = c.regs.CPSR.value;

  c.regs.copy(SUPERVISOR);
  c.regs.CPSR.MODE_BITS = SUPERVISOR;

  c.regs.CPSR.IRQ_DISABLE = true;
  c.regs.CPSR.STATE_BIT   = ARM_MODE;

  c.regs.r[15] = 0x00000008;
  c.flush_pipeline();
}

void ARM::Instructions::MSR(ARM7TDMI& c, instruction_info& instr) {
  // TODO: refactor, this is some 4AM code

  // SPDLOG_INFO(instr.P);
  // fmt::println("opcode: {:#010x}", instr.opcode);
  u32 amount      = (instr.opcode & 0xff);
  u8 shift_amount = (instr.opcode & 0b111100000000) >> 8;
  // fmt::println("amount: {:#010x}", amount);
  // fmt::println("shift amount: {:#010x}", shift_amount);

  bool modify_flag_field    = (instr.opcode & (1 << 19)) ? true : false;
  bool modify_control_field = (instr.opcode & (1 << 16)) ? true : false;

  u32 op_value = 0;

  if (instr.I) {  // 0 = Register, 1 = Immediate
    op_value = std::rotr(amount, shift_amount * 2);
  } else {
    op_value = c.regs.r[instr.Rm];
  }

  // SPDLOG_DEBUG("[MSR] op_value: {:#010x}", op_value);
  // fmt::println("P: {}", +instr.P);

  switch (instr.P) {  // CPSR = 0, SPSR = 1
    case PSR::CPSR: {
      // if (instr.I) {  // We use the immediate value (amount)
      if (modify_flag_field) {
        c.regs.CPSR.value &= ~0xF0000000;
        c.regs.CPSR.value |= (op_value & 0xF0000000);
        SPDLOG_DEBUG("[CPSR] new flag field: {:#x}", op_value & 0xF0000000);
      }
      if (modify_control_field) {
        op_value |= 0x10;
        SPDLOG_DEBUG("[CPSR] new control field: {:#x}", op_value & 0x000000FF);
        c.regs.copy((BANK_MODE)(op_value & 0x1f));
        c.regs.CPSR.value &= ~0x000000FF;
        c.regs.CPSR.value |= (op_value & 0x000000FF);
      }
      break;
    }

    case PSR::SPSR_MODE: {
      switch (c.regs.CPSR.MODE_BITS) {
        case USER:
        case SYSTEM: {
          assert(0);
          break;
        }
        case FIQ: {
          SPDLOG_DEBUG("WROTE TO FIQ");
          if (modify_flag_field) {
            c.regs.SPSR_fiq &= ~0xF0000000;
            c.regs.SPSR_fiq |= (op_value & 0xF0000000);
            SPDLOG_DEBUG("[FIQ SPSR] new flag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_control_field) {
            op_value |= 0x10;
            SPDLOG_DEBUG("[FIQ SPSR] new control field: {:#x}", op_value & 0x000000FF);

            c.regs.SPSR_fiq &= ~0x000000FF;
            c.regs.SPSR_fiq |= (op_value & 0x000000FF);
          }
          break;
        }
        case IRQ: {
          if (modify_flag_field) {
            c.regs.SPSR_irq &= ~0xF0000000;
            c.regs.SPSR_irq |= (op_value & 0xF0000000);
          }
          if (modify_control_field) {
            op_value |= 0x10;
            SPDLOG_DEBUG("[IRQ SPSR] new control field: {:#x}", op_value & 0x000000FF);
            c.regs.SPSR_irq &= ~0x000000FF;
            c.regs.SPSR_irq |= (op_value & 0x000000FF);
          }
          break;
        }
        case SUPERVISOR: {
          if (modify_flag_field) {
            c.regs.SPSR_svc &= ~0xF0000000;
            c.regs.SPSR_svc |= (op_value & 0xF0000000);
          }
          if (modify_control_field) {
            op_value |= 0x10;
            SPDLOG_DEBUG("[SVC SPSR] new control field: {:#x}", op_value & 0x000000FF);
            c.regs.SPSR_svc &= ~0x000000FF;
            c.regs.SPSR_svc |= (op_value & 0x000000FF);
          }
          break;
        }
        case ABORT: {
          if (modify_flag_field) {
            c.regs.SPSR_abt &= ~0xF0000000;
            c.regs.SPSR_abt |= (op_value & 0xF0000000);
          }
          if (modify_control_field) {
            op_value |= 0x10;
            SPDLOG_DEBUG("[ABT SPSR] new control field: {:#x}", op_value & 0x000000FF);
            c.regs.SPSR_abt &= ~0x000000FF;
            c.regs.SPSR_abt |= (op_value & 0x000000FF);
          }
          break;
        }
        case UNDEFINED: {
          if (modify_flag_field) {
            c.regs.SPSR_und &= ~0xF0000000;
            c.regs.SPSR_und |= (op_value & 0xF0000000);
          }
          if (modify_control_field) {
            op_value |= 0x10;
            SPDLOG_DEBUG("[UND SPSR] new control field: {:#x}", op_value & 0x000000FF);
            c.regs.SPSR_und &= ~0x000000FF;
            c.regs.SPSR_und |= (op_value & 0x000000FF);
          }
          break;
        }
      }
      break;
    };

    default: {
      SPDLOG_CRITICAL("INVALID RM PASSED: {}", +instr.Rm);
    }
  }
}

void ARM::Instructions::STM(ARM7TDMI& c, instruction_info& instr) {  // REFACTOR: this could written much shorter
  u16 reg_list_v = instr.opcode & 0xFFFF;

  if (c.regs.CPSR.STATE_BIT == THUMB_MODE) { reg_list_v = instr.opcode & 0xFF; }

  std::vector<u8> reg_list;

  bool forced_writeback       = false;
  // bool base_first_in_reg_list = false;
  // (void)(base_first_in_reg_list);

  for (size_t idx = 0; idx < 16; idx++) {
    if (reg_list_v & (1 << idx)) {
      // if (idx == instr.Rn && reg_list.empty()) { base_first_in_reg_list = true; }

      reg_list.push_back(idx);
    }
  }
  if (instr.LR_BIT) { reg_list.push_back(14); }

  if (reg_list.empty()) {
    // SPDLOG_DEBUG("[STM] reg list empty, pushing 15");
    reg_list.push_back(15);
    // flush_queued     = true;
    forced_writeback = true;
    instr.W          = 0;
    // return;
  }

  // SPDLOG_DEBUG("[STM] unaligned address: {:#010x}", c.regs.r[instr.Rn]);
  u32 base = c.regs.r[instr.Rn];  // Base
  // u32 new_base = 0;
  // SPDLOG_DEBUG("[STM] aligned address: {:#010x}", base);

  if (instr.P == 1 && instr.U == 0) {  // Pre-decrement
    // SPDLOG_DEBUG("hit pre-decrement [DB]");

    base -= (reg_list.size() * 4);

    u8 pass = 0;

    if (forced_writeback) {
      c.regs.r[instr.Rn] -= 0x40;
      base -= 0x40;
      SPDLOG_DEBUG("NEW RN: {:#010x}", c.regs.r[instr.Rn]);
      reg_list.push_back(15);
    }

    for (const auto& r_num : reg_list) {
      if (instr.Rn == r_num) { SPDLOG_DEBUG("Rn{} IN STMDB: {:#010x}", +instr.Rn, c.regs.r[instr.Rn]); }
      if (instr.S) {
        c.bus->write32(c.align_address(base + (pass * 4), BOUNDARY::WORD), r_num == 15 ? c.regs.user_system_bank[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4) : c.regs.user_system_bank[r_num]);
      } else {
        c.bus->write32(c.align_address(base + (pass * 4), BOUNDARY::WORD), r_num == 15                                       ? c.regs.r[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4)
                                                                           : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? base
                                                                                                                             : c.regs.r[r_num]);
      }
      pass++;
    }

    if (instr.W && !forced_writeback) { c.regs.r[instr.Rn] = base; }
    return;
  }

  if (instr.P == 1 && instr.U == 1) {  // Pre-increment
    // SPDLOG_DEBUG("[STM] hit pre-increment");

    // SPDLOG_DEBUG("[STMIB] {:#010x}", base);

    u8 pass = 0;

    if (forced_writeback) { reg_list.push_back(15); }

    for (const auto& r_num : reg_list) {
      pass++;
      if (instr.S) {
        c.bus->write32((base + (pass * 4)), r_num == 15 ? c.regs.user_system_bank[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4) : c.regs.user_system_bank[r_num]);
      } else {
        c.bus->write32((base + (pass * 4)), r_num == 15                                       ? c.regs.r[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4)
                                            : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? base + (reg_list.size()) * 4
                                                                                              : c.regs.r[r_num]);
      }
    }

    if (forced_writeback) {
      c.regs.r[instr.Rn] += 0x40;
      base += 0x40;
      // SPDLOG_DEBUG("NEW RN: {:#010x}", c.regs.r[instr.Rn]);
    }

    base += (reg_list.size() * 4);
    // SPDLOG_DEBUG("STMIB BASE: {:#010x}", base);

    if (instr.W && !forced_writeback) {
      SPDLOG_DEBUG("write back is first in rlist?: {}", base_first_in_reg_list);
      // SPDLOG_DEBUG("[STMIB] writing back: {:#010x}", base);
      c.regs.r[instr.Rn] = base;
    }
    return;
  }

  if (instr.P == 0 && instr.U == 0) {  // Post-decrement (DA)
    // SPDLOG_DEBUG("hit post-decrement");

    base -= (reg_list.size() * 4);

    u8 pass = 0;

    if (forced_writeback) {
      c.regs.r[instr.Rn] -= 0x40;
      base -= 0x40;
      // SPDLOG_DEBUG("NEW RN: {:#010x}", c.regs.r[instr.Rn]);
      reg_list.push_back(15);
    }

    for (const auto& r_num : reg_list) {
      pass++;
      if (instr.S) {
        c.bus->write32(base + (pass * 4), r_num == 15 ? c.regs.user_system_bank[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4) : c.regs.user_system_bank[r_num]);
      } else {
        c.bus->write32(base + (pass * 4), r_num == 15 ? c.regs.r[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4) : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? base : c.regs.r[r_num]);
      }
    }

    if (instr.W && !forced_writeback) { c.regs.r[instr.Rn] = base; }

    return;
  }

  if (instr.P == 0 && instr.U == 1) {  // Post-increment
    // SPDLOG_DEBUG("[STMIA] hit post-increment");

    u8 pass = 0;
    for (const auto& r_num : reg_list) {
      if (instr.S) {
        c.bus->write32(base + (pass * 4), r_num == 15 ? c.regs.user_system_bank[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4) : c.regs.user_system_bank[r_num]);
      } else {
        // ARM_MODE
        c.bus->write32(base + (pass * 4), r_num == 15                                       ? c.regs.r[r_num] + (c.regs.CPSR.STATE_BIT ? 2 : 4)
                                          : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? (base + (reg_list.size() * 4))
                                                                                            : c.regs.r[r_num]);
      }
      pass++;
    }

    base += (reg_list.size() * 4);
    // SPDLOG_DEBUG("STMIA BASE: {:#010x}", base);
    if (forced_writeback) { c.regs.r[instr.Rn] += 0x40; }
    if (instr.W && !forced_writeback) { c.regs.r[instr.Rn] = base; }

    return;
  }

  SPDLOG_CRITICAL("invalid STM mode");
  assert(0);
  return;
}

void ARM::Instructions::UMULL(ARM7TDMI& c, instruction_info& instr) {
  u64 res = static_cast<uint64_t>(c.regs.r[instr.Rm]) * c.regs.r[instr.Rs];

  // Rd = Hi
  // Rn = Lo

  c.regs.r[instr.Rd] = res >> 32;
  c.regs.r[instr.Rn] = res & 0xffffffff;

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();
  }
}

void ARM::Instructions::SMULL(ARM7TDMI& c, instruction_info& instr) {
  s32 rm = static_cast<u32>(c.regs.r[instr.Rm]);
  s32 rs = static_cast<u32>(c.regs.r[instr.Rs]);

  // SPDLOG_DEBUG("RM: {:#x}", rm);
  // SPDLOG_DEBUG("RS: {:#x}", rs);

  // SPDLOG_DEBUG("res: {}", rm * rs);

  s64 res = (s64)rm * rs;
  
  SPDLOG_DEBUG("LO: {:#x}", lo);
  c.regs.r[instr.Rd] = (u32)(res >> 32);
  c.regs.r[instr.Rn] = (u32)(res & 0xffffffff);

  if (instr.S) {
    (static_cast<u64>(res) >= 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();

    // assert(0);
  }
}

void ARM::Instructions::SMLAL(ARM7TDMI& c, instruction_info& instr) {
  s64 rm = static_cast<s32>(static_cast<u32>(c.regs.r[instr.Rm]));
  s64 rs = static_cast<s32>(static_cast<u32>(c.regs.r[instr.Rs]));

  s64 big_number = (static_cast<u64>(c.regs.r[instr.Rd]) << 32) + c.regs.r[instr.Rn];

  s64 res = (rm * rs) + big_number;
  
  c.regs.r[instr.Rd] = res >> 32;
  c.regs.r[instr.Rn] = res & 0xffffffff;

  SPDLOG_DEBUG("RM: {}", rm);
  SPDLOG_DEBUG("RS: {}", rs);
  SPDLOG_DEBUG("Rd(Hi): {}", c.regs.r[instr.Rd]);
  SPDLOG_DEBUG("Rn(Lo): {}", c.regs.r[instr.Rn]);

  SPDLOG_DEBUG("calculated hi: {:#010x}", hi);
  SPDLOG_DEBUG("calculated lo: {:#010x}", lo);

  SPDLOG_DEBUG("big_number: {}", big_number);

  // SPDLOG_DEBUG("big num: {:#010x}", big_number);

  SPDLOG_DEBUG("res: {}", res);

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();

    // assert(0);
  }
}

void ARM::Instructions::UMLAL(ARM7TDMI& c, instruction_info& instr) {
  // Rd = Hi
  // Rn = Lo

  u64 big_number = (static_cast<u64>(c.regs.r[instr.Rd]) << 32) + c.regs.r[instr.Rn];
  u64 res        = big_number + (static_cast<u64>(c.regs.r[instr.Rm]) * static_cast<u64>(c.regs.r[instr.Rs]));

  c.regs.r[instr.Rd] = res >> 32;
  c.regs.r[instr.Rn] = res & 0xffffffff;

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();

    // assert(0);
  }
}

void ARM::Instructions::MLA(ARM7TDMI& c, instruction_info& instr) {
  c.regs.r[instr.Rd] = (c.regs.r[instr.Rm] * c.regs.r[instr.Rs]) + c.regs.r[instr.Rn];

  if (instr.S) {
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
    c.regs.r[instr.Rd] >= 0x80000000 ? c.set_negative() : c.reset_negative();
  }
}
void ARM::Instructions::MUL(ARM7TDMI& c, instruction_info& instr) {
  SPDLOG_DEBUG("Rm: {} - OP2: {:#x}", +instr.Rm, instr.op2);
  SPDLOG_DEBUG("Rn: {:#x} - Rn V: {:#x}", +instr.Rn, c.regs.r[instr.Rn]);

  c.regs.r[instr.Rd] = (c.regs.r[instr.Rm] * c.regs.r[instr.Rs]);

  if (instr.S) {
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
    c.regs.r[instr.Rd] >= 0x80000000 ? c.set_negative() : c.reset_negative();
  }
}

void ARM::Instructions::LDM(ARM7TDMI& c, instruction_info& instr) {
  u16 reg_list_v = instr.opcode & 0xFFFF;

  if (c.regs.CPSR.STATE_BIT == THUMB_MODE) { reg_list_v = instr.opcode & 0xFF; }

  std::vector<u8> reg_list;
  bool flush_queued     = false;
  bool forced_writeback = false;

  bool in_reg_list = false;

  for (size_t idx = 0; idx < 16; idx++) {
    if (reg_list_v & (1 << idx)) {
      if (idx == instr.Rn) { in_reg_list = true; }
      if (idx == 15) { flush_queued = true; }
      reg_list.push_back(idx);
    }
  }

  if (instr.PC_BIT) {
    reg_list.push_back(15);
    flush_queued = true;
  }

  if (reg_list.empty()) {
    // SPDLOG_DEBUG("[LDM] reg list empty, pushing 15 PC: {:#010x}", c.regs.r[15]);
    reg_list.push_back(15);
    flush_queued     = true;
    forced_writeback = true;
    instr.W          = 0;
    // return;
  }

  // SPDLOG_DEBUG("[LDM] unaligned address: {:#010x}", c.regs.r[instr.Rn]);
  u32 address = c.align_address(c.regs.r[instr.Rn], WORD);  // Base
  // SPDLOG_DEBUG("[LDM] aligned address: {:#010x}", address);

  if (instr.P == 0 && instr.U == 1) {  // Post-increment
    // SPDLOG_INFO("post increment");
    u8 pass = 0;

    for (const auto& r_num : reg_list) {
      if (instr.S) {
        c.regs.user_system_bank[r_num] = c.bus->read32(address + (pass * 4));
        // if(forced_writeback) {
        //   c.regs.user_system_bank[instr.Rn] += 4;
        // }
      } else {
        c.regs.r[r_num] = c.bus->read32(address + (pass * 4));
        // if(forced_writeback) {
        //   c.regs.r[instr.Rn] += 4;
        // }
      }
      pass++;
    }

    if (instr.W) {
      if (!in_reg_list) c.regs.r[instr.Rn] += (reg_list.size() * 4);
    }
    if (flush_queued) { c.flush_pipeline(); }
    if (forced_writeback && !in_reg_list) {
      c.regs.r[instr.Rn] += 0x40;
      return;
    }
    return;
  }

  if (instr.P == 0 && instr.U == 0) {  // Post-decrement
    SPDLOG_DEBUG("hit post decrement");
    u8 pass = 0;

    address -= (reg_list.size() * 4);

    for (const auto& r_num : reg_list) {
      pass++;

      if (instr.S) {
        c.regs.user_system_bank[r_num] = c.bus->read32(address + (pass * 4));
      } else {
        c.regs.r[r_num] = c.bus->read32(address + (pass * 4));
      }
    }

    if (instr.W && !in_reg_list) { c.regs.r[instr.Rn] -= (reg_list.size() * 4); }
    if (flush_queued) { c.flush_pipeline(); }
    if (forced_writeback) {
      c.regs.r[instr.Rn] -= 0x40;
      return;
    }
    return;
  }

  if (instr.P == 1 && instr.U == 0) {  // Pre-decrement
    SPDLOG_DEBUG("hit pre decrement");
    // assert(0);
    u8 pass = 0;

    address -= (reg_list.size() * 4);

    for (const auto& r_num : reg_list) {
      if (instr.S) {
        c.regs.user_system_bank[r_num] = c.bus->read32(address + (pass * 4));
      } else {
        c.regs.r[r_num] = c.bus->read32(address + (pass * 4));
      }
      pass++;
    }

    if (instr.W && !in_reg_list) { c.regs.r[instr.Rn] -= (reg_list.size() * 4); }
    if (flush_queued) { c.flush_pipeline(); }
    if (forced_writeback) {
      c.regs.r[instr.Rn] -= 0x40;
      return;
    }
    return;
  }

  if (instr.P == 1 && instr.U == 1) {  // Pre-increment (IB)
    SPDLOG_DEBUG("hit pre increment (IB)");

    u8 pass = 0;

    for (const auto& r_num : reg_list) {
      pass++;
      if (instr.S) {
        c.regs.user_system_bank[r_num] = c.bus->read32(address + (pass * 4));
      } else {
        c.regs.r[r_num] = c.bus->read32(address + (pass * 4));
      }
    }

    if (instr.W && !in_reg_list) { c.regs.r[instr.Rn] += (pass * 4); }
    if (flush_queued) { c.flush_pipeline(); }
    if (forced_writeback) { c.regs.r[instr.Rn] += 0x40; }
    return;
  }

  SPDLOG_CRITICAL("invalid LDM mode");
  assert(0);
}

void ARM::Instructions::SWP(ARM7TDMI& c, instruction_info& instr) {
  u32 rm_pre_swap                 = c.regs.r[instr.Rm];
  bool dest_is_source             = (instr.Rd == instr.Rm);
  u32 address                     = c.regs.r[instr.Rn];
  u8 misaligned_load_rotate_value = (c.regs.r[instr.Rn] & 3) * 8;

  if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[SWP] misaligned rotate value: {}", misaligned_load_rotate_value); }

  if (instr.B) {
    u8 byte_at_loc     = c.bus->read8(address);
    c.regs.r[instr.Rd] = byte_at_loc;
    c.bus->write8(address, dest_is_source ? rm_pre_swap : c.regs.r[instr.Rm]);
  } else {
    address            = c.align_address(c.regs.r[instr.Rn], WORD);  // Word writes must be aligned, and the value read must be rotated
    u32 word_at_loc    = c.bus->read32(address);
    c.regs.r[instr.Rd] = std::rotr(word_at_loc, misaligned_load_rotate_value);
    c.bus->write32(address, dest_is_source ? rm_pre_swap : c.regs.r[instr.Rm]);
  }
}

void ARM::Instructions::BLL(ARM7TDMI& c, instruction_info& instr) {
  // SPDLOG_DEBUG("op2: {:#010x}", (c.regs.r[15]) + (instr.op2 << 12));
  u32 offset = (instr.op2 << 12);
  // SPDLOG_DEBUG("calculated offset: {:#010x}", offset);
  if (offset >= 0x400000) {
    // SPDLOG_DEBUG("neg!");
    offset -= 0x800000;
  }
  c.regs.r[14] = (c.regs.r[15]) + (offset);
}

void ARM::Instructions::BLH(ARM7TDMI& c, instruction_info& instr) {
  u32 pc = c.regs.r[15];
  u32 lr = c.regs.r[14];

  c.regs.r[14] = (pc - 2) | 1;
  c.regs.r[15] = lr + (instr.op2 << 1);

  c.flush_pipeline();
}

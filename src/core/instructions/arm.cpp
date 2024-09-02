#include "instructions/arm.hpp"

#include <cassert>
#include <limits>

#include "cpu.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "spdlog/spdlog.h"

bool check_for_overflow(u32 before, u32 after) {
  SPDLOG_DEBUG("before: {:#08x}", before);
  SPDLOG_DEBUG("after: {:#08x}", after);

  if ((before >= 0x80000000 && after < 0x80000000) || ((before < 0x80000000) && (after >= 0x80000000))) {
    return true;
  } else {
    return false;
  }
}

bool is_negative(u32 v) { return v >= 0x80000000; }
bool is_zero(u32 v) { return v == 0; }

void ARM::Instructions::B(ARM7TDMI& c, InstructionInfo& instr) {
  if (instr.L) { c.regs.r[14] = c.regs.r[15] - 4; }

  c.regs.r[15] += (instr.offset);

  c.regs.r[15] &= ~1;

  SPDLOG_DEBUG("{:#x}", c.regs.r[15]);

  c.flush_pipeline();
}

void ARM::Instructions::BX(ARM7TDMI& c, InstructionInfo& instr) {
  // 0001b: BX{cond}  Rn    ;PC=Rn, T=Rn.0   (ARMv4T and ARMv5 and up)

  // c.regs.set_reg(15, c.regs.r[instr.Rn] & ~1);
  c.regs.r[15] = (c.regs.r[instr.Rn] & ~1);

  // SPDLOG_INFO("R15: {:#010x}", c.regs.r[15]);

  c.regs.CPSR.STATE_BIT = (CPU_MODE)(c.regs.r[instr.Rn] & 0b1);

  // c.regs.CPSR.STATE_BIT ? SPDLOG_INFO("entered THUMB mode") : SPDLOG_INFO("entered ARM mode");

  c.flush_pipeline();

  return;
}

void ARM::Instructions::NOP([[gnu::unused]] ARM7TDMI& c, [[gnu::unused]] InstructionInfo& instr) { return; }
void ARM::Instructions::AND(ARM7TDMI& c, InstructionInfo& instr) {
  // instr.print_params();
  instr.op2 = c.handle_shifts(instr);

  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] & instr.op2;

  // assert(instr.S == 0);
  if (instr.S) {
    c.regs.r[instr.Rd] >= 0x80000000 ? c.set_negative() : c.reset_negative();
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
  }
  return;
}

void ARM::Instructions::EOR(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);

  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] ^ instr.op2;

  if (instr.S) {
    is_zero(c.regs.r[instr.Rd]) ? c.set_zero() : c.reset_zero();
    is_negative(c.regs.r[instr.Rd]) ? c.set_negative() : c.reset_negative();
  }
}
void ARM::Instructions::BIC(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);

  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] & ~(instr.op2);

  if (instr.S) {
    is_negative(c.regs.r[instr.Rd]) ? c.set_negative() : c.reset_negative();
    is_zero(c.regs.r[instr.Rd]) ? c.set_zero() : c.reset_zero();
  }
}

void ARM::Instructions::RSB(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);
  SPDLOG_DEBUG("Rm: {} - OP2: {:#x}", +instr.Rm, instr.op2);
  SPDLOG_DEBUG("Rn: {:#x} - Rn V: {:#x}", +instr.Rn, c.regs.r[instr.Rn]);

  bool op2_negative = instr.op2 >= 0x80000000;
  bool rn_negative  = c.regs.r[instr.Rn] >= 0x80000000;

  bool diffSigns = (op2_negative ^ rn_negative);

  c.regs.r[instr.Rd] = instr.op2 - c.regs.r[instr.Rn];

  bool res_negative = c.regs.r[instr.Rd] >= 0x80000000;

  bool rDiff = (res_negative ^ op2_negative);

  if (instr.S) {
    is_zero(c.regs.r[instr.Rd]) ? c.set_zero() : c.reset_zero();
    is_negative(c.regs.r[instr.Rd]) ? c.set_negative() : c.reset_negative();
    (rDiff & diffSigns) ? c.set_overflow() : c.reset_overflow();
    instr.op2 >= c.regs.r[instr.Rn] ? c.set_carry() : c.reset_carry();
  }
  // assert(instr.S == 0);
}

void ARM::Instructions::SUB(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);

  SPDLOG_DEBUG("performing {} (Rn) - {} (op2)", c.regs.r[instr.Rn], instr.op2);

  u32 l = c.regs.r[instr.Rn] - instr.op2;

  SPDLOG_DEBUG("L: {:#x}", l);

  if (instr.S) {
    is_negative(l) ? c.set_negative() : c.reset_negative();
    is_zero(l) ? c.set_zero() : c.reset_zero();
    (c.regs.r[instr.Rd] >= instr.op2) ? c.set_carry() : c.reset_carry();
    (c.regs.r[instr.Rd] >= 0x80000000 && (l < 0x80000000)) ? c.set_overflow() : c.reset_overflow();

    if (instr.Rd == 15) { c.regs.load_spsr_to_cpsr(); }
  }

  c.regs.r[instr.Rd] = (c.regs.r[instr.Rn] - instr.op2);
}

void ARM::Instructions::SBC(ARM7TDMI& c, InstructionInfo& instr) {
  bool old_carry = c.regs.CPSR.CARRY_FLAG;
  instr.op2      = c.handle_shifts(instr);
  SPDLOG_DEBUG("INSTR RN: {} - Rn V {:#x}", +instr.Rn, c.regs.r[instr.Rn]);
  SPDLOG_DEBUG("INSTR RM REG: {} - op2: {:#x}", +instr.Rm, instr.op2);

  int l = (c.regs.r[instr.Rn] - instr.op2) + (old_carry - 1);
  fmt::println("L (calced int): {}", l);

  if (instr.S) {
    l < 0 ? c.set_negative() : c.reset_negative();
    l == 0 ? c.set_zero() : c.reset_zero();
    l >= 0 ? c.set_carry() : c.reset_carry();
    ((c.regs.r[instr.Rd] >= 0x80000000) && l >= 0) ? c.set_overflow() : c.reset_overflow();
  }
  c.regs.r[instr.Rd] = (c.regs.r[instr.Rn] - instr.op2) + (old_carry - 1);
}

void ARM::Instructions::MVN(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);

  c.regs.r[instr.Rd] = ~instr.op2;
  if (instr.S) {
    ~instr.op2 == 0 ? c.set_zero() : c.reset_zero();
    (~instr.op2 & 0x80000000) != 0 ? c.set_negative() : c.reset_negative();
  }
}

void ARM::Instructions::ADC(ARM7TDMI& c, InstructionInfo& instr) {
  // instr.print_params();
  bool o_carry = c.regs.CPSR.CARRY_FLAG;  // Carry from barrel shifter should
                                          // not be used for this instruction.
  instr.op2 = c.handle_shifts(instr);

  u64 t_v = c.regs.r[instr.Rn];
  // fmt::println("t_v: {}", t_v);

  // SPDLOG_DEBUG("OP2: {:#010x}", instr.op2);
  // SPDLOG_DEBUG("Rn: {:#010x}", c.regs.r[instr.Rn]);
  // SPDLOG_DEBUG("CARRY: {:#010x}", +o_carry);

  // fmt::println("t_v: {:#010x}", c.regs.r[instr.Rn] + instr.op2 + (o_carry ? 1 : 0));

  c.regs.r[instr.Rd] = (c.regs.r[instr.Rn] + instr.op2 + o_carry);

  if (instr.S) {
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
    ((t_v < 0x80000000) && (t_v + instr.op2 + o_carry) >= 0x80000000) ? c.set_overflow() : c.reset_overflow();
    ((t_v + instr.op2 + o_carry) >= std::numeric_limits<u32>::max()) ? c.set_carry() : c.reset_carry();
    (c.regs.r[instr.Rd] >= 0x80000000) ? c.set_negative() : c.reset_negative();
  }
}

void ARM::Instructions::TEQ(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);

  u32 m = c.regs.r[instr.Rn] ^ instr.op2;

  is_negative(m) ? c.set_negative() : c.reset_negative();
  is_zero(m) ? c.set_zero() : c.reset_zero();
}

void ARM::Instructions::MOV(ARM7TDMI& c, InstructionInfo& instr) {
  // instr.print_params();
  instr.op2 = c.handle_shifts(instr);

  c.regs.r[instr.Rd] = instr.op2;

  SPDLOG_DEBUG("[MOV] instr.op2: {:#010x}", instr.op2);

  if (instr.S) {
    is_zero(instr.op2) ? c.set_zero() : c.reset_zero();
    is_negative(instr.op2) ? c.set_negative() : c.reset_negative();
  }
  if (instr.Rd == 15) {
    c.regs.r[instr.Rd] &= ~3;
    c.flush_pipeline();
  }
}
void ARM::Instructions::ADD(ARM7TDMI& c, InstructionInfo& instr) {
  // ADD{cond}{S} Rd,Rn,Op2 ;* ;add               Rd = Rn+Op2
  // instr.print_params();
  if (c.regs.CPSR.STATE_BIT == THUMB_MODE) {
    if (instr.Rn == 15) {
      c.regs.r[instr.Rd] = ((c.regs.r[15]) & ~2) + instr.op2;
      return;
    } else if (instr.Rn == 13) {
      c.regs.r[instr.Rd] = (c.regs.r[13]) + instr.op2;
      return;
    }
  }

  instr.op2 = c.handle_shifts(instr);
  
  u64 t_v = c.regs.r[instr.Rn];

  // fmt::println("t_v: {}", t_v);

  // fmt::println("t_v: {}", c.regs.r[instr.Rn] + instr.op2);

  c.regs.r[instr.Rd] = (c.regs.r[instr.Rn] + instr.op2);

  // fmt::println("r: {:#010x}", r);

  if (instr.S) {
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
    ((t_v < 0x80000000) && t_v + instr.op2 >= 0x80000000) ? c.set_overflow() : c.reset_overflow();

    ((t_v + instr.op2) >= std::numeric_limits<u32>::max()) ? c.set_carry() : c.reset_carry();
  }
}

void ARM::Instructions::TST(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);

  u32 d = c.regs.r[instr.Rn] & instr.op2;

  is_zero(d) ? c.set_zero() : c.reset_zero();
  is_negative(d) ? c.set_negative() : c.reset_negative();
}

void ARM::Instructions::STRH(ARM7TDMI& c, InstructionInfo& instr) {
  u32 address = c.regs.r[instr.Rn];

  u8 immediate_offset = (((instr.opcode & 0xf00) >> 8) << 4) + (instr.opcode & 0xf);

  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register
  // instr.print_params();
  if (address >= 0x60000000 && address <= 0x60017FFF) { SPDLOG_INFO("VRAM WRITE: {:#010x} => {}", c.regs.r[instr.Rn], instr.I ? immediate_offset : c.regs.r[instr.Rm]); }
  if (instr.P == 0) {
    // address = c.align_address(address, HALFWORD);
    address = c.align_address(address, HALFWORD);
    c.bus->write16(address, c.regs.r[instr.Rd] & 0xffff);

    if (instr.U) {
      c.regs.r[instr.Rn] += (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      address = c.regs.r[instr.Rn];
    } else {
      c.regs.r[instr.Rn] -= (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      address = c.regs.r[instr.Rn];
    }

  } else {
    bool base_is_destination = false;
    u32 written_val          = c.regs.r[instr.Rd];
    if (instr.Rd == instr.Rn) { base_is_destination = true; }

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
    address = c.align_address(address, HALFWORD);
    c.bus->write16(address, base_is_destination ? written_val : c.regs.r[instr.Rd] & 0xffff);
  }
}

void ARM::Instructions::STR(ARM7TDMI& c, InstructionInfo& instr) {
  // instr.print_params();
  // assert(0);
  u32 address = c.regs.r[instr.Rn];

  // if (instr.B) throw std::runtime_error("strb not implemented");
  assert(instr.I == 0);

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

void ARM::Instructions::CMN(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2 = c.handle_shifts(instr);

  u64 r_64 = c.regs.r[instr.Rn] + instr.op2;

  u32 r = c.regs.r[instr.Rn] + instr.op2;
  is_negative(r) ? c.set_negative() : c.reset_negative();
  is_zero(r) ? c.set_zero() : c.reset_zero();

  (r_64 >= 0x100000000) ? c.set_carry() : c.reset_carry();
  (c.regs.r[instr.Rn] >= 0x80000000 && (r < 0x80000000)) ? c.set_overflow() : c.reset_overflow();
}

void ARM::Instructions::RSC(ARM7TDMI& c, InstructionInfo& instr) { c.regs.r[instr.Rd] = (instr.op2 - c.regs.r[instr.Rn]) + (c.regs.CPSR.CARRY_FLAG - 1); }
void ARM::Instructions::LDR(ARM7TDMI& c,
                            InstructionInfo& _instr) {  // TODO: Re-write this, too long and unnecessarily complex
  // Rn - base register
  // Rd - Source / Destination Register (where we're loading to)

  // LDR - Load data from memory into register [Rd]
  // instr.print_params();

  // assert(instr.B == 0);
  // assert(instr.I == 0);

  InstructionInfo instr = _instr;  // Pipeline flush invalidates reference due
                                   // to pipeline change, copy necessary

  // assert(instr.B == 0);

  if (instr.I) { instr.offset = c.shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, c.regs.r[instr.Rm], instr.shift_amount, false); }

  u32 c_address = c.regs.r[instr.Rn];

  if (instr.pc_relative) {
    SPDLOG_DEBUG("instr offset: {}", instr.offset);
    SPDLOG_DEBUG("read address: {:#010x}", (c.regs.r[15] + instr.offset) & ~2);
    c.regs.r[instr.Rd] = c.bus->read32((c.regs.r[15] + instr.offset) & ~2);
    // c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);
    return;
  }

  u8 misaligned_load_rotate_value = (c_address & 3) * 8;

  if (instr.P == 0) {  // Forced writeback

    SPDLOG_DEBUG("[LDR] P = 0");

    if (instr.B) {
      c.regs.r[instr.Rd] = c.bus->read8(c_address);
    } else {
      misaligned_load_rotate_value = (c_address & 3) * 8;
      if (misaligned_load_rotate_value) { SPDLOG_DEBUG("misaligned rotate value: {}", misaligned_load_rotate_value); }

      c_address = c.align_address(c_address, WORD);  // word loads are forcibly aligned to the nearest word address
      SPDLOG_DEBUG("loading from {:#010x}", c_address);

      c.regs.r[instr.Rd] = c.bus->read32(c_address);
      c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);  // Rotate new contents if address was misaligned
      SPDLOG_DEBUG("new r{}: {:#010x}", +instr.Rd, c.regs.r[instr.Rd]);
    }

    // flush pipeline if PC changed, refill with new instructions
    if (instr.Rd == 15) { c.flush_pipeline(); }

    // Writeback logic
    if (!(instr.Rd == instr.Rn)) {  // Don't writeback to base if destination is same as base
      if (instr.U) {
        c.regs.r[instr.Rn] += instr.offset;
      } else {
        c.regs.r[instr.Rn] -= instr.offset;
      }
    }

  } else {
    SPDLOG_DEBUG("[LDR] P = 1");
    SPDLOG_DEBUG("[LDR] I = {}", +instr.I);

    // Write-back logic
    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += instr.offset;
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] + instr.offset;
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= instr.offset;
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] - instr.offset;
      }
    }

    if (instr.B) {
      c.regs.r[instr.Rd] = c.bus->read8(c_address);
    } else {
      misaligned_load_rotate_value = (c_address & 3) * 8;
      SPDLOG_DEBUG("[LDR] loading from {:#010x}", c_address);
      if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[LDR] misaligned rotate value: {}", misaligned_load_rotate_value); }

      c_address          = c.align_address(c_address, WORD);
      c.regs.r[instr.Rd] = c.bus->read32(c_address);
      c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);
    }

    if (instr.Rd == 15) { c.flush_pipeline(); }
  }
}

void ARM::Instructions::LDRSB(ARM7TDMI& c,
                              InstructionInfo& _instr) {  // TODO: Re-write this, too long and unnecessarily complex
  // Rn - base register
  // Rd - Source / Destination Register (where we're writing to)

  // LDR - Load data from memory into register [Rd]
  // instr.print_params();

  // assert(instr.I == 0);

  InstructionInfo instr = _instr;  // Pipeline flush invalidates reference due
                                   // to pipeline change, copy necessary

  u32 c_address = c.regs.r[instr.Rn];

  if (instr.P == 0) {  // Forced writeback
    c.regs.r[instr.Rd] = c.bus->read8(c_address);

    c.regs.r[instr.Rd] &= ~0xffffff00;
    if ((c.regs.r[instr.Rd] & (1 << 7))) { c.regs.r[instr.Rd] |= 0xffffff00; }

    SPDLOG_DEBUG("new r{}: {:#010x}", +instr.Rd, c.regs.r[instr.Rd]);

    if (instr.Rd == 15) { c.flush_pipeline(); }

    SPDLOG_DEBUG("hi.... Rn is still: {}", +instr.Rn);

    if (!(instr.Rd == instr.Rn)) {
      if (instr.U) {
        c.regs.r[instr.Rn] += instr.offset;
        // address = c.regs.r[instr.Rn];
      } else {
        c.regs.r[instr.Rn] -= instr.offset;
        // address = c.regs.r[instr.Rn];
      }
    }
    fmt::println("DONE!");
  } else {
    SPDLOG_DEBUG("[LDRSB] P = 1");
    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += instr.offset;
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] + instr.offset;
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= instr.offset;
        c_address = c.regs.r[instr.Rn];
      } else {
        c_address = c.regs.r[instr.Rn] - instr.offset;
      }
    }

    c.regs.r[instr.Rd] = c.bus->read8(c_address);

    c.regs.r[instr.Rd] &= ~0xffffff00;
    if ((c.regs.r[instr.Rd] & (1 << 7))) { c.regs.r[instr.Rd] |= 0xffffff00; }
    SPDLOG_DEBUG("new r{}: {:#010x}", +instr.Rd, c.regs.r[instr.Rd]);

    if (instr.Rd == 15) { c.flush_pipeline(); }
  }
}

void ARM::Instructions::LDRSH(ARM7TDMI& c,
                              InstructionInfo& instr) {  // TODO: implement writeback logic

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
                             InstructionInfo& instr) {  // TODO: implement writeback logic

  u32 address = c.regs.r[instr.Rn];  // Read address from base register

  u8 immediate_offset = (((instr.opcode & 0xf00) >> 8) << 4) + (instr.opcode & 0xf);

  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register
  // instr.print_params();

  u8 misaligned_load_rotate_value = (address & 1) * 8;

  if (instr.P == 0) {
    misaligned_load_rotate_value = (address & 1) * 8;

    if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[LDRH] misaligned rotate value: {}", misaligned_load_rotate_value); }

    SPDLOG_DEBUG("[LDRH] loading from {:#010x}", c.align_address(address, HALFWORD));
    address            = c.align_address(address, HALFWORD);
    c.regs.r[instr.Rd] = c.bus->read16(address);

    c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);

    if (instr.Rd == 15) { c.flush_pipeline(); }

    if (!(instr.Rd == instr.Rn)) {
      if (instr.U) {                                                              // On post increment, write back is always enabled.
        c.regs.r[instr.Rn] += (instr.I ? immediate_offset : c.regs.r[instr.Rm]);  // Increase base register (post indexing)
        address = c.regs.r[instr.Rn];
      } else {
        c.regs.r[instr.Rn] -= (instr.I ? immediate_offset : c.regs.r[instr.Rm]);  // Decrease base register (post indexing)
        address = c.regs.r[instr.Rn];
      }
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

    misaligned_load_rotate_value = (address & 1) * 8;

    if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[LDRH] misaligned rotate value: {}", misaligned_load_rotate_value); }

    SPDLOG_DEBUG("[LDRH] loading from {:#010x}", c.align_address(address, HALFWORD));
    address            = c.align_address(address, HALFWORD);
    c.regs.r[instr.Rd] = c.bus->read16(address);

    c.regs.r[instr.Rd] = std::rotr(c.regs.r[instr.Rd], misaligned_load_rotate_value);

    if (instr.Rd == 15) { c.flush_pipeline(); }

    // c.regs.r[instr.Rd] = c.bus->read16(address);
    // assert(0);
    // c.bus->write16(address, c.regs.r[instr.Rd] & 0xffff);
  }
}

void ARM::Instructions::CMP(ARM7TDMI& c, InstructionInfo& instr) {
  // instr.print_params();
  //  CMP - set condition codes on Op1 - Op2
  instr.op2 = c.handle_shifts(instr);

  u32 r = c.regs.r[instr.Rn] - instr.op2;

  SPDLOG_DEBUG("INSTR RN: {:#08x}", c.regs.r[instr.Rn]);
  SPDLOG_DEBUG("INSTR OP2: {:#08x}", instr.op2);

  is_zero(r) ? c.set_zero() : c.reset_zero();
  is_negative(r) ? c.set_negative() : c.reset_negative();
  (c.regs.r[instr.Rn] >= 0x80000000 && (r < 0x80000000)) ? c.set_overflow() : c.reset_overflow();
  (c.regs.r[instr.Rn] >= instr.op2) ? c.set_carry() : c.reset_carry();

  if (instr.Rd == 15 && instr.S) { c.regs.load_spsr_to_cpsr(); }
}
void ARM::Instructions::ORR(ARM7TDMI& c, InstructionInfo& instr) {
  instr.op2          = c.handle_shifts(instr);
  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] | instr.op2;

  if (instr.S) {
    is_zero(c.regs.r[instr.Rd]) ? c.set_zero() : c.reset_zero();
    is_negative(c.regs.r[instr.Rd]) ? c.set_negative() : c.reset_negative();
  }
}

void ARM::Instructions::MRS(ARM7TDMI& c, InstructionInfo& instr) { c.regs.r[instr.Rd] = c.regs.CPSR.value; }

void ARM::Instructions::SWI(ARM7TDMI& c, [[gnu::unused]] InstructionInfo& instr) {
  c.regs.copy(SUPERVISOR);

  c.regs.r[14] = c.regs.r[15] - 4;

  c.regs.r[15] = 0x00000008;
  c.flush_pipeline();
}

void ARM::Instructions::MSR(ARM7TDMI& c, InstructionInfo& instr) {
  // SPDLOG_INFO(instr.P);
  // fmt::println("opcode: {:#010x}", instr.opcode);
  u32 amount      = (instr.opcode & 0xff);
  u8 shift_amount = (instr.opcode & 0b111100000000) >> 8;
  // fmt::println("amount: {:#010x}", amount);
  // fmt::println("shift amount: {:#010x}", shift_amount);

  bool modify_flag_field    = (instr.opcode & (1 << 19)) ? true : false;
  bool modify_control_field = (instr.opcode & (1 << 16)) ? true : false;

  // TODO: replace with shift function

  u32 op_value = 0;
  //  = rotateRight(amount, shift_amount * 2);

  if (instr.I) {
    op_value = std::rotr(amount, shift_amount * 2);
  } else {
    op_value = c.regs.r[instr.Rm];
  }
  // fmt::println("P: {}", +instr.P);

  switch (instr.P) {  // CPSR = 0, SPSR = 1
    case PSR::CPSR: {
      // if (instr.I) {  // We use the immediate value (amount)
      if (modify_flag_field) {
        c.regs.CPSR.value &= ~0xF0000000;
        c.regs.CPSR.value |= (op_value & 0xF0000000);
      }
      if (modify_control_field) {
        op_value |= 0x10;
        SPDLOG_DEBUG("new control field: {:#x}", op_value & 0x000000FF);
        c.regs.copy((BANK_MODE)(op_value & 0x1f));
        c.regs.CPSR.value &= ~0x000000FF;
        c.regs.CPSR.value |= (op_value & 0x000000FF);
      }
      break;
    }

      // };

    case PSR::SPSR_MODE: {
      switch (c.regs.CPSR.MODE_BITS) {
        case USER:
        case SYSTEM: {
          assert(0);
          break;
        }
        case FIQ: {
          if (modify_flag_field) {
            c.regs.SPSR_fiq &= ~0xF0000000;
            c.regs.SPSR_fiq |= (amount & 0xF0000000);
          }
          if (modify_control_field) {
            amount |= 0x10;
            SPDLOG_DEBUG("new control field: {:#x}", amount & 0x000000FF);

            c.regs.SPSR_fiq &= ~0x000000FF;
            c.regs.SPSR_fiq |= (amount & 0x000000FF);
          }
          break;
        }
        case IRQ: {
          if (modify_flag_field) {
            c.regs.SPSR_irq &= ~0xF0000000;
            c.regs.SPSR_irq |= (amount & 0xF0000000);
          }
          if (modify_control_field) {
            amount |= 0x10;
            SPDLOG_DEBUG("new control field: {:#x}", amount & 0x000000FF);
            c.regs.SPSR_irq &= ~0x000000FF;
            c.regs.SPSR_irq |= (amount & 0x000000FF);
          }
          break;
        }
        case SUPERVISOR: {
          if (modify_flag_field) {
            c.regs.SPSR_svc &= ~0xF0000000;
            c.regs.SPSR_svc |= (amount & 0xF0000000);
          }
          if (modify_control_field) {
            amount |= 0x10;
            SPDLOG_DEBUG("new control field: {:#x}", amount & 0x000000FF);
            c.regs.SPSR_svc &= ~0x000000FF;
            c.regs.SPSR_svc |= (amount & 0x000000FF);
          }
          break;
        }
        case ABORT: {
          if (modify_flag_field) {
            c.regs.SPSR_abt &= ~0xF0000000;
            c.regs.SPSR_abt |= (amount & 0xF0000000);
          }
          if (modify_control_field) {
            amount |= 0x10;
            SPDLOG_DEBUG("new control field: {:#x}", amount & 0x000000FF);
            c.regs.SPSR_abt &= ~0x000000FF;
            c.regs.SPSR_abt |= (amount & 0x000000FF);
          }
          break;
        }
        case UNDEFINED: {
          if (modify_flag_field) {
            c.regs.SPSR_und &= ~0xF0000000;
            c.regs.SPSR_und |= (amount & 0xF0000000);
          }
          if (modify_control_field) {
            amount |= 0x10;
            SPDLOG_DEBUG("new control field: {:#x}", amount & 0x000000FF);
            c.regs.SPSR_und &= ~0x000000FF;
            c.regs.SPSR_und |= (amount & 0x000000FF);
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

void ARM::Instructions::STM(ARM7TDMI& c, InstructionInfo& instr) {
  u16 reg_list_v = instr.opcode & 0xFFFF;

  if (c.regs.CPSR.STATE_BIT == THUMB_MODE) { reg_list_v = instr.opcode & 0xFF; }

  std::vector<u8> reg_list;

  bool forced_writeback       = false;
  bool base_first_in_reg_list = false;
  // (void)(base_first_in_reg_list);

  for (size_t idx = 0; idx < 16; idx++) {
    if (reg_list_v & (1 << idx)) {
      if (idx == instr.Rn && reg_list.empty()) { base_first_in_reg_list = true; }

      reg_list.push_back(idx);
    }
  }

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
        c.bus->write32(c.align_address(base + (pass * 4), BOUNDARY::WORD), r_num == 15 ? c.regs.user_system_bank[r_num] + 4 : c.regs.user_system_bank[r_num]);
      } else {
        c.bus->write32(c.align_address(base + (pass * 4), BOUNDARY::WORD), r_num == 15 ? c.regs.r[r_num] + 4 : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? base : c.regs.r[r_num]);
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
        c.bus->write32((base + (pass * 4)), r_num == 15 ? c.regs.user_system_bank[r_num] + 4 : c.regs.user_system_bank[r_num]);
      } else {
        c.bus->write32((base + (pass * 4)), r_num == 15 ? c.regs.r[r_num] + 4 : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? base + (reg_list.size()) * 4 : c.regs.r[r_num]);
      }
    }

    if (forced_writeback) {
      c.regs.r[instr.Rn] += 0x40;
      base += 0x40;
      SPDLOG_DEBUG("NEW RN: {:#010x}", c.regs.r[instr.Rn]);
    }

    base += (reg_list.size() * 4);
    SPDLOG_DEBUG("STMIB BASE: {:#010x}", base);

    if (instr.W && !forced_writeback) {
      SPDLOG_DEBUG("write back is first in rlist?: {}", base_first_in_reg_list);
      SPDLOG_DEBUG("[STMIB] writing back: {:#010x}", base);
      c.regs.r[instr.Rn] = base;
    }
    return;
  }

  if (instr.P == 0 && instr.U == 0) {  // Post-decrement (DA)
    SPDLOG_DEBUG("hit post-decrement");

    base -= (reg_list.size() * 4);

    u8 pass = 0;

    if (forced_writeback) {
      c.regs.r[instr.Rn] -= 0x40;
      base -= 0x40;
      SPDLOG_DEBUG("NEW RN: {:#010x}", c.regs.r[instr.Rn]);
      reg_list.push_back(15);
    }

    for (const auto& r_num : reg_list) {
      pass++;
      if (instr.S) {
        c.bus->write32(base + (pass * 4), r_num == 15 ? c.regs.user_system_bank[r_num] + 4 : c.regs.user_system_bank[r_num]);
      } else {
        c.bus->write32(base + (pass * 4), r_num == 15 ? c.regs.r[r_num] + 4 : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? base : c.regs.r[r_num]);
      }
    }

    if (instr.W && !forced_writeback) { c.regs.r[instr.Rn] = base; }

    return;
  }

  if (instr.P == 0 && instr.U == 1) {  // Post-increment
    SPDLOG_DEBUG("[STMIA] hit post-increment");

    u8 pass = 0;
    for (const auto& r_num : reg_list) {
      if (instr.S) {
        c.bus->write32(base + (pass * 4), r_num == 15 ? c.regs.user_system_bank[r_num] + 4 : c.regs.user_system_bank[r_num]);
      } else {
        c.bus->write32(base + (pass * 4), r_num == 15 ? c.regs.r[r_num] + 4 : instr.Rn == r_num && reg_list.at(0) != instr.Rn ? (base + (reg_list.size() * 4)) : c.regs.r[r_num]);
      }
      pass++;
    }

    base += (reg_list.size() * 4);
    SPDLOG_DEBUG("STMIA BASE: {:#010x}", base);
    if (forced_writeback) { c.regs.r[instr.Rn] += 0x40; }
    if (instr.W && !forced_writeback) { c.regs.r[instr.Rn] = base; }

    return;
  }

  SPDLOG_CRITICAL("invalid STM mode");
  assert(0);
  return;
}

void ARM::Instructions::UMULL(ARM7TDMI& c, InstructionInfo& instr) {
  u64 res = static_cast<uint64_t>(c.regs.r[instr.Rm]) * c.regs.r[instr.Rs];

  // Rd = Hi
  // Rn = Lo

  c.regs.r[instr.Rd] = res >> 32;
  c.regs.r[instr.Rn] = res & 0xffffffff;

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();

    // assert(0);
  }
}

void ARM::Instructions::SMULL(ARM7TDMI& c, InstructionInfo& instr) {
  s32 rm = c.regs.r[instr.Rm];
  s32 rs = c.regs.r[instr.Rs];

  SPDLOG_DEBUG("RM: {}", rm);
  SPDLOG_DEBUG("RS: {}", rs);

  SPDLOG_DEBUG("res: {}", rm * rs);

  s64 res = rm * rs;

  // Rd = Hi
  // Rn = Lo

  c.regs.r[instr.Rd] = res >> 32;
  c.regs.r[instr.Rn] = res & 0xffffffff;

  if (instr.S) {
    (res & 0x8000000000000000) ? c.set_negative() : c.reset_negative();
    res == 0 ? c.set_zero() : c.reset_zero();

    // assert(0);
  }
}

void ARM::Instructions::SMLAL(ARM7TDMI& c, InstructionInfo& instr) {
  s64 rm = static_cast<s32>(static_cast<u32>(c.regs.r[instr.Rm]));
  s64 rs = static_cast<s32>(static_cast<u32>(c.regs.r[instr.Rs]));

  s64 big_number = (static_cast<u64>(c.regs.r[instr.Rd]) << 32) + c.regs.r[instr.Rn];

  s64 res = (rm * rs) + big_number;

  s32 hi = (big_number & 0xFFFFFFFF00000000) >> 32;
  s32 lo = big_number & 0xFFFFFFFF;

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

void ARM::Instructions::UMLAL(ARM7TDMI& c, InstructionInfo& instr) {
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

void ARM::Instructions::MLA(ARM7TDMI& c, InstructionInfo& instr) {
  c.regs.r[instr.Rd] = (c.regs.r[instr.Rm] * c.regs.r[instr.Rs]) + c.regs.r[instr.Rn];

  if (instr.S) {
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
    c.regs.r[instr.Rd] >= 0x80000000 ? c.set_negative() : c.reset_negative();
  }
}
void ARM::Instructions::MUL(ARM7TDMI& c, InstructionInfo& instr) {
  SPDLOG_DEBUG("Rm: {} - OP2: {:#x}", +instr.Rm, instr.op2);
  SPDLOG_DEBUG("Rn: {:#x} - Rn V: {:#x}", +instr.Rn, c.regs.r[instr.Rn]);

  c.regs.r[instr.Rd] = (c.regs.r[instr.Rm] * c.regs.r[instr.Rs]);

  if (instr.S) {
    c.regs.r[instr.Rd] == 0 ? c.set_zero() : c.reset_zero();
    c.regs.r[instr.Rd] >= 0x80000000 ? c.set_negative() : c.reset_negative();
  }
}

void ARM::Instructions::LDM(ARM7TDMI& c, InstructionInfo& instr) {
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

void ARM::Instructions::DIV_STUB([[gnu::unused]] ARM7TDMI& c, [[gnu::unused]] InstructionInfo& instr) {
  SPDLOG_DEBUG("running stubbed DIV bios function");

  s32 a       = static_cast<s32>(c.regs.r[0]);
  s32 b       = static_cast<s32>(c.regs.r[1]);
  s32 div     = (a / b);
  s32 mod     = (a % b);
  c.regs.r[0] = static_cast<u32>(div);
  c.regs.r[1] = static_cast<u32>(mod);
  c.regs.r[3] = abs(a / b);
}

void ARM::Instructions::SWP(ARM7TDMI& c, InstructionInfo& instr) {
  u8 misaligned_load_rotate_value = (c.regs.r[instr.Rn] & 3) * 8;

  if (misaligned_load_rotate_value) { SPDLOG_DEBUG("[SWP] misaligned rotate value: {}", misaligned_load_rotate_value); }

  bool same_dest = false;
  u32 old_rm     = c.regs.r[instr.Rm];

  if (instr.Rd == instr.Rm) {
    same_dest = true;
    SPDLOG_DEBUG("same dest, value to write: {:#010x}", old_rm);
  }

  u32 address = c.align_address(c.regs.r[instr.Rn], WORD);

  c.regs.r[instr.Rd] = std::rotr(instr.B ? c.bus->read8(address) : c.bus->read32(address), misaligned_load_rotate_value);
  instr.B ? c.bus->write8(address, same_dest ? old_rm : c.regs.r[instr.Rm]) : c.bus->write32(address, same_dest ? old_rm : c.regs.r[instr.Rm]);
}

void ARM::Instructions::BLL(ARM7TDMI& c, InstructionInfo& instr) {
  SPDLOG_DEBUG("op2: {:#010x}", (c.regs.r[15]) + (instr.op2 << 12));
  u32 offset = (instr.op2 << 12);
  SPDLOG_DEBUG("calculated offset: {:#010x}", offset);
  if (offset >= 0x400000) {
    SPDLOG_DEBUG("neg!");
    offset -= 0x800000;
  }
  c.regs.r[14] = (c.regs.r[15]) + (offset);
}

void ARM::Instructions::BLH(ARM7TDMI& c, InstructionInfo& instr) {
  u32 o_15 = c.regs.r[15];
  u32 o_14 = c.regs.r[14];

  c.regs.r[14] = (o_15 - 2) | 1;
  c.regs.r[15] = o_14 + (instr.op2 << 1);

  c.flush_pipeline();
}

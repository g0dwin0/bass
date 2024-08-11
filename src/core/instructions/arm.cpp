#include "instructions/arm.hpp"

#include "cpu.hpp"
#include "instructions/instruction.hpp"
#include "spdlog/spdlog.h"
#include "utils/bitwise.hpp"

// using namespace ARM::Instructions;
enum PSR_LOC {
  CPSR,      // CPSR
  SPSR_MODE  // SPSR of the current mode
};

void ARM::Instructions::B(ARM7TDMI& c, InstructionInfo& instr) {
  if (instr.L) { c.regs.r[14] = c.regs.r[15] - 8; }

  int x = instr.offset;

  if (x & 0x800000) { x -= 0x1000000; }
  x *= 4;

  // SPDLOG_INFO("{:#x}",c.regs.r[15] + (signed_value - 4));
  c.regs.r[15] += (x - 4);

  c.flush_pipeline();
}

void ARM::Instructions::MOV(ARM7TDMI& c, InstructionInfo& instr) {
  if (instr.S) fmt::println("should change status");
  c.regs.r[instr.Rd] = instr.op2;
}
void ARM::Instructions::ADD(ARM7TDMI& c, InstructionInfo& instr) {
  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] + instr.op2;
}

void ARM::Instructions::TST(ARM7TDMI& c, InstructionInfo& instr) {
  u8 d = c.regs.r[instr.Rn] & instr.op2;

  d == 0 ? c.set_zero() : c.reset_zero();
  // d == 0 ? c.set_zero() : c.reset_zero();
  // fmt::println("")
  // exit(-1);
}

void ARM::Instructions::STRH(ARM7TDMI& c, InstructionInfo& instr) {
  u32 address = c.regs.r[instr.Rn];

  u8 immediate_offset =
      (((instr.opcode & 0xf00) >> 8) << 4) + (instr.opcode & 0xf);

  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register
  // instr.print_params();

  if (instr.P == 0) {
    c.bus->write16(address, c.regs.r[instr.Rd] & 0xffff);

    if (instr.U) {
      c.regs.r[instr.Rn] += (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      address = c.regs.r[instr.Rn];
    } else {
      c.regs.r[instr.Rn] -= (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      address = c.regs.r[instr.Rn];
    }

  } else {
    if (instr.U) {
      if (instr.W) {
        c.regs.r[instr.Rn] += (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] +
                  (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      }
    } else {
      if (instr.W) {
        c.regs.r[instr.Rn] -= (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
        address = c.regs.r[instr.Rn];
      } else {
        address = c.regs.r[instr.Rn] -
                  (instr.I ? immediate_offset : c.regs.r[instr.Rm]);
      }
    }

    c.bus->write16(address, c.regs.r[instr.Rd] & 0xffff);
  }
}

void ARM::Instructions::STR(ARM7TDMI& c, InstructionInfo& instr) {
  (void)(c);
  (void)(instr);
  exit(-1);
}
void ARM::Instructions::LDR(ARM7TDMI& c, InstructionInfo& instr) {
  // Rn - base register
  // Rd - Source / Destination Register (where we're writing to)

  // LDR - Load data from memory into register [Rd]

  u32 c_address = 0;

  if (instr.P == 0) {
    c.regs.r[instr.Rd] = c.bus->read32(c.regs.r[instr.Rn]);

    if (instr.U) {
      c.regs.r[instr.Rn] += instr.offset;
      // address = c.regs.r[instr.Rn];
    } else {
      c.regs.r[instr.Rn] -= instr.offset;
      // address = c.regs.r[instr.Rn];
    }

  } else {
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
  }

  c.regs.r[instr.Rd] = c.bus->read32(c_address);
}

void ARM::Instructions::LDRH(ARM7TDMI& c, InstructionInfo& instr) { // TODO: implement writeback logic
  u32 c_address = c.regs.r[instr.Rn]; 

  // fmt::println("r0: {:#010x}", c.regs.r[0]);
  // fmt::println("r1: {:#010x}", c.regs.r[1]);
  // fmt::println("r2: {:#010x}", c.regs.r[2]);
  // fmt::println("r3: {:#010x}", c.regs.r[3]);
  // fmt::println("W: {:#010x}", (u8)instr.W);

  // fmt::println("Rn + Rm := {:#010x}", (c.regs.r[instr.Rn] +
  // c.regs.r[instr.Rm]) & 0xffff);

  // if(instr.P) {
  //   c_address += c.regs.r[instr.Rm];
  // }
  c.regs.r[instr.Rd] = c.bus->read32(c_address) & 0xffff;
  if (instr.P == 0) { c.regs.r[instr.Rn] += instr.Rm; }
}

void ARM::Instructions::CMP(ARM7TDMI& c, InstructionInfo& instr) {
  //  CMP - set condition codes on Op1 - Op2

  i32 r = c.regs.r[instr.Rn] - instr.op2;

  (r == 0) ? c.regs.CPSR.ZERO_FLAG = 1 : c.regs.CPSR.ZERO_FLAG = 0;
  (r < 0) ? c.regs.CPSR.SIGN_FLAG = 1 : c.regs.CPSR.SIGN_FLAG = 0;
  c.regs.CPSR.CARRY_FLAG    = (instr.op2 < c.regs.r[instr.Rn]) ? 1 : 0;
  c.regs.CPSR.OVERFLOW_FLAG = 0;
}
void ARM::Instructions::ORR(ARM7TDMI& c, InstructionInfo& instr) {
  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] | instr.op2;

  if (instr.S) {
    (c.regs.r[instr.Rd] == 0) ? c.regs.CPSR.ZERO_FLAG = 1
                              : c.regs.CPSR.ZERO_FLAG = 0;
  }
}

void ARM::Instructions::MSR(ARM7TDMI& c, InstructionInfo& instr) {
  // SPDLOG_INFO(instr.P);
  u32 amount             = (instr.opcode & 0xff);
  u8 shift_amount        = (instr.opcode & 0b111100000000) >> 8;
  bool modify_flag_field = (instr.opcode & (1 << 19));
  // bool modify_status_field    = (instr.opcode & (1 << 18));
  // bool modify_extension_field = (instr.opcode & (1 << 17));
  bool modify_control_field = (instr.opcode & (1 << 16));

  amount = rotateRight(amount, shift_amount * 2);

  // fmt::println(
  //     "{}{}{}{}", modify_flag_field ? "f" : "", modify_status_field ? "s" :
  //     "", modify_extension_field ? "x" : "", modify_control_field ? "c" :
  //     "");

  switch (instr.P) {
    case PSR_LOC::CPSR: {
      if (instr.I) {  // We use the immediate value (amount)
        u32 amount      = (instr.opcode & 0xff);
        u8 shift_amount = (instr.opcode & 0b111100000000) >> 8;

        amount = rotateRight(amount, shift_amount * 2);

        if (modify_flag_field) {
          c.regs.CPSR.value &= ~0xFF000000;
          c.regs.CPSR.value |= (amount & 0xFF000000);
        }
        if (modify_control_field) {
          c.regs.CPSR.value &= ~0x000000FF;
          c.regs.CPSR.value |= (amount & 0x000000FF);
        }
      }

      break;
    };

    case PSR_LOC::SPSR_MODE: {
      SPDLOG_CRITICAL("SPSR NOT IMPLEMENTED");
      exit(-1);
      break;
    };

    default: {
      SPDLOG_CRITICAL("INVALID RM PASSED: {}", (u8)instr.Rm);
    }
  }
}

void ARM::Instructions::STM(ARM7TDMI& c, InstructionInfo& instr) {
  u16 reg_list_v = instr.opcode & 0xFFFF;

  std::vector<u8> reg_list;

  // instr.print_params();

  for (size_t idx = 16; idx != SIZE_MAX; idx--) {
    if (reg_list_v & (1 << idx)) {
      reg_list.push_back(idx);

      if (instr.P) {
        if (idx == 15) {
          c.regs.r[instr.Rn] += instr.U ? 4 : -4;
          c.bus->write32(c.regs.r[instr.Rn], instr.loc + 12);
        } else {
          c.regs.r[instr.Rn] += instr.U ? 4 : -4;
          c.bus->write32(c.regs.r[instr.Rn], c.regs.r[idx]);
        }
      } else {
        if (idx == 15) {
          c.bus->write32(c.regs.r[instr.Rn], instr.loc + 12);
          c.regs.r[instr.Rn] += instr.U ? 4 : -4;
        } else {
          c.bus->write32(c.regs.r[instr.Rn], c.regs.r[idx]);
          c.regs.r[instr.Rn] += instr.U ? 4 : -4;
        }
      }
      fmt::println("instr loc: {:#010x}", instr.loc + 12);

      // fmt::print("r{} ", idx);
    }
  }
  SPDLOG_DEBUG("register list: r{}", fmt::join(reg_list, ",r"));

  return;
}

void ARM::Instructions::LDM(ARM7TDMI& c, InstructionInfo& instr) {
  u16 reg_list_v = instr.opcode & 0xFFFF;

  std::vector<u8> reg_list;

  // instr.print_params();

  for (size_t idx = 0; idx < 16; idx++) {
    if (reg_list_v & (1 << idx)) {
      reg_list.push_back(idx);

      if (instr.P) {
        // c.bus->write32(c.regs.r[instr.Rn], c.regs.r[idx]);

        //
        c.regs.r[instr.Rn] += instr.U ? 4 : -4;
        c.regs.r[idx] = c.bus->read32(c.regs.r[instr.Rn]);
      } else {
        c.regs.r[idx] = c.bus->read32(c.regs.r[instr.Rn]);
        c.regs.r[instr.Rn] += instr.U ? 4 : -4;
      }

      if (idx == 15) c.flush_pipeline();
    }
  }
  SPDLOG_DEBUG("register list: r{}", fmt::join(reg_list, ",r"));

  return;
}

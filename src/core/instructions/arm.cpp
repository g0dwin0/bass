#include "instructions/arm.hpp"

#include "instructions/instruction.hpp"
#include "spdlog/spdlog.h"

// using namespace ARM::Instructions;
enum PSR_LOC {
  CPSR,      // CPSR
  SPSR_MODE  // SPSR of the current mode
};

void ARM::Instructions::B(ARM7TDMI& c, InstructionInfo& instr) {
  if (instr.L) { c.regs.r[14] = c.regs.r[15]; }

  int32_t signed_value = (int32_t)(instr.offset & 0x00FFFFFF);

  if (signed_value & 0x00800000) { signed_value |= 0xFF000000; }

  signed_value <<= 2;

  signed_value = (signed_value << 8) >> 8;

  // SPDLOG_INFO("{:#x}",c.regs.r[15] + (signed_value - 4));
  c.regs.r[15] += signed_value - 4;

  c.flush_pipeline();
}

void ARM::Instructions::MOV(ARM7TDMI& c, InstructionInfo& instr) {
  c.regs.r[instr.Rd] = instr.op2;
}
void ARM::Instructions::ADD(ARM7TDMI& c, InstructionInfo& instr) {
  c.regs.r[instr.Rd] = c.regs.r[instr.Rn] + instr.op2;
}

void ARM::Instructions::STRH(ARM7TDMI& c, InstructionInfo& instr) {
  // instr.H = (instr.opcode &  0x10) ? 1 : 0;

  if (instr.I != 0) {
    SPDLOG_INFO("SHIFT TYPE: {}", 0);
    exit(-1);
  }

  //  1: STR{cond}H  Rd,<Address>  ;Store halfword   [a]=Rd
  //     2: LDR{cond}D  Rd,<Address>  ;Load Doubleword  R(d)=[a], R(d+1)=[a+4]
  //     3: STR{cond}D  Rd,<Address>  ;Store Doubleword [a]=R(d), [a+4]=R(d+1)
  // fmt::println("R0: {:#x}", regs.r[0]);
  // fmt::println("R1: {:#x}", regs.r[1]);
//   fmt::println("r0: {:#010x}", c.regs.r[0]);
// fmt::println("r1: {:#010x}", c.regs.r[1]);
// fmt::println("r2: {:#010x}", c.regs.r[2]);
// fmt::println("r3: {:#010x}", c.regs.r[3]);
  

//   fmt::println("Rm: {:#x}", c.regs.r[instr.Rm]);
//   fmt::println("Rn: {:#x}", (u8)instr.Rn);
//   fmt::println("W: {:#x}", (u8)instr.W);
//   SPDLOG_INFO("Rd: {:#x}", (u8)instr.Rd);
//  SPDLOG_INFO("P: {}", (u8)instr.P);
//   SPDLOG_INFO("W: {}", (u8)instr.W);
//   SPDLOG_INFO("offset: {:#x}", (u8)instr.Rm);
  u32 c_address = c.regs.r[instr.Rn];

  

  
  c.bus->write16(c_address, c.regs.r[instr.Rd] & 0xffff);
  if(instr.U) {
    c.regs.r[instr.Rn] += c.regs.r[instr.Rm];
  }
  //  =  = c.fetch(c_add); // TODO: make a peek function
}
void ARM::Instructions::LDRH(ARM7TDMI& c, InstructionInfo& instr) {
  u32 c_address = c.regs.r[instr.Rn];
  fmt::println("r0: {:#010x}", c.regs.r[0]);
  fmt::println("r1: {:#010x}", c.regs.r[1]);
  fmt::println("r2: {:#010x}", c.regs.r[2]);
  fmt::println("r3: {:#010x}", c.regs.r[3]);
  fmt::println("W: {:#010x}", (u8)instr.W);
  
  fmt::println("Rn + Rm := {:#010x}", (c.regs.r[instr.Rn] + c.regs.r[instr.Rm]) & 0xffff);
  

  
  // if(instr.P) {
  //   c_address += c.regs.r[instr.Rm];
  // }
  c.regs.r[instr.Rd] = c.bus->read32(c_address) & 0xffff;
  if(instr.P == 0) {
    c.regs.r[instr.Rn] += instr.Rm;
  }
}

void ARM::Instructions::CMP(ARM7TDMI& c, InstructionInfo& instr) {
  //  CMP - set condition codes on Op1 - Op2

  ((int)(instr.Rn - instr.op2) == 0) ? c.regs.CPSR.ZERO_FLAG = 1
                                     : c.regs.CPSR.ZERO_FLAG = 0;
  ((int)(instr.Rn - instr.op2) < 0) ? c.regs.CPSR.SIGN_FLAG = 1
                                    : c.regs.CPSR.SIGN_FLAG = 0;
  c.regs.CPSR.CARRY_FLAG    = 0;
  c.regs.CPSR.OVERFLOW_FLAG = 0;

  // ((instr.Rn - instr.op2) == 0) ? c.regs.CPSR.ZERO_FLAG = 1 :
  // c.regs.CPSR.ZERO_FLAG = 0;
}

void ARM::Instructions::MSR(ARM7TDMI& c, InstructionInfo& instr) {
  SPDLOG_INFO(instr.P);

  switch (instr.P) {
    case PSR_LOC::CPSR: {
      c.regs.CPSR.value = c.regs.r[instr.Rm];
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

  (void)(c);
  exit(-1);
}

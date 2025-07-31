#pragma once

#include "cpu.hpp"

enum PSR {
  CPSR,      // CPSR
  SPSR_MODE  // SPSR of the current mode
};

enum SHIFT_MODE {
  LSL,
  LSR,
  ASR,
  ROR,
};

namespace ARM::Instructions {
  // Data Processing (ALU)
  void AND(ARM7TDMI& c, InstructionInfo& instr);
  void EOR(ARM7TDMI& c, InstructionInfo& instr);
  void SUB(ARM7TDMI& c, InstructionInfo& instr);
  void RSB(ARM7TDMI& c, InstructionInfo& instr);
  void ADD(ARM7TDMI& c, InstructionInfo& instr);
  void ADC(ARM7TDMI& c, InstructionInfo& instr);
  void SBC(ARM7TDMI& c, InstructionInfo& instr);
  void RSC(ARM7TDMI& c, InstructionInfo& instr);
  void TST(ARM7TDMI& c, InstructionInfo& instr);
  void TEQ(ARM7TDMI& c, InstructionInfo& instr);
  void CMP(ARM7TDMI& c, InstructionInfo& instr);
  void CMN(ARM7TDMI& c, InstructionInfo& instr);
  void ORR(ARM7TDMI& c, InstructionInfo& instr);
  void MOV(ARM7TDMI& c, InstructionInfo& instr);
  void BIC(ARM7TDMI& c, InstructionInfo& instr);
  void MVN(ARM7TDMI& c, InstructionInfo& instr);

  // Mutiplication
  void MUL(ARM7TDMI& c, InstructionInfo& instr);
  void MLA(ARM7TDMI& c, InstructionInfo& instr);
  void UMAAL(ARM7TDMI& c, InstructionInfo& instr);
  void UMULL(ARM7TDMI& c, InstructionInfo& instr);
  void UMLAL(ARM7TDMI& c, InstructionInfo& instr);
  void SMULL(ARM7TDMI& c, InstructionInfo& instr);
  void SMLAL(ARM7TDMI& c, InstructionInfo& instr);

  // Single Data Transfer
  void LDR(ARM7TDMI& c, InstructionInfo& instr);  
  void STR(ARM7TDMI& c, InstructionInfo& instr);

  void THUMB_LDR(ARM7TDMI& c, InstructionInfo& instr);  
  void THUMB_STR(ARM7TDMI& c, InstructionInfo& instr);

  // Halfword, Signed Data Transfer
  void LDRH(ARM7TDMI& c, InstructionInfo& instr);
  void STRH(ARM7TDMI& c, InstructionInfo& instr);
  void LDRSB(ARM7TDMI& c, InstructionInfo& instr);
  void LDRSH(ARM7TDMI& c, InstructionInfo& instr);

  // Single Data Swap
  void SWP(ARM7TDMI& c, InstructionInfo& instr);

  // Branching
  void B(ARM7TDMI& c, InstructionInfo& instr);
  void BX(ARM7TDMI& c, InstructionInfo& instr);

  // Block Data Transfer
  void LDM(ARM7TDMI& c, InstructionInfo& instr);
  void STM(ARM7TDMI& c, InstructionInfo& instr);
  
  // PSR Transfer
  void MRS(ARM7TDMI& c, InstructionInfo& instr);
  void MSR(ARM7TDMI& c, InstructionInfo& instr);
  
  // Software Interrupt
  void SWI(ARM7TDMI& c, InstructionInfo& instr);

  
  void BLL(ARM7TDMI& c, InstructionInfo& instr);
  void BLH(ARM7TDMI& c, InstructionInfo& instr);
  
  void NOP(ARM7TDMI& c, InstructionInfo& instr);
  
  // Misc Helper
  void check_r15_lookahead(InstructionInfo& instr, u32& op1); 
  void single_data_transfer_writeback_str(ARM7TDMI& c, InstructionInfo& instr, u32 address, u8 added_writeback_value); 
  void single_data_transfer_writeback_ldr(ARM7TDMI& c, InstructionInfo& instr, u32 address, u8 added_writeback_value); 
  
  
}  // namespace ARM::Instructions
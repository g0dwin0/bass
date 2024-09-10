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
  void AND(ARM7TDMI& c, instruction_info& instr);
  void EOR(ARM7TDMI& c, instruction_info& instr);
  void SUB(ARM7TDMI& c, instruction_info& instr);
  void RSB(ARM7TDMI& c, instruction_info& instr);
  void ADD(ARM7TDMI& c, instruction_info& instr);
  void ADC(ARM7TDMI& c, instruction_info& instr);
  void SBC(ARM7TDMI& c, instruction_info& instr);
  void RSC(ARM7TDMI& c, instruction_info& instr);
  void TST(ARM7TDMI& c, instruction_info& instr);
  void TEQ(ARM7TDMI& c, instruction_info& instr);
  void CMP(ARM7TDMI& c, instruction_info& instr);
  void CMN(ARM7TDMI& c, instruction_info& instr);
  void ORR(ARM7TDMI& c, instruction_info& instr);
  void MOV(ARM7TDMI& c, instruction_info& instr);
  void BIC(ARM7TDMI& c, instruction_info& instr);
  void MVN(ARM7TDMI& c, instruction_info& instr);

  // Mutiplication
  void MUL(ARM7TDMI& c, instruction_info& instr);
  void MLA(ARM7TDMI& c, instruction_info& instr);
  void UMAAL(ARM7TDMI& c, instruction_info& instr);
  void UMULL(ARM7TDMI& c, instruction_info& instr);
  void UMLAL(ARM7TDMI& c, instruction_info& instr);
  void SMULL(ARM7TDMI& c, instruction_info& instr);
  void SMLAL(ARM7TDMI& c, instruction_info& instr);

  // Single Data Transfer
  void LDR(ARM7TDMI& c, instruction_info& instr);  
  void STR(ARM7TDMI& c, instruction_info& instr);

  // Halfword, Signed Data Transfer
  void LDRH(ARM7TDMI& c, instruction_info& instr);
  void STRH(ARM7TDMI& c, instruction_info& instr);
  void LDRSB(ARM7TDMI& c, instruction_info& instr);
  void LDRSH(ARM7TDMI& c, instruction_info& instr);

  // Single Data Swap
  void SWP(ARM7TDMI& c, instruction_info& instr);

  // Branching
  void B(ARM7TDMI& c, instruction_info& instr);
  void BL(ARM7TDMI& c, instruction_info& instr);
  void BX(ARM7TDMI& c, instruction_info& instr);

  // Block Data Transfer
  void LDM(ARM7TDMI& c, instruction_info& instr);
  void STM(ARM7TDMI& c, instruction_info& instr);
  
  // PSR Transfer
  void MRS(ARM7TDMI& c, instruction_info& instr);
  void MSR(ARM7TDMI& c, instruction_info& instr);

  
  // Temporary HLE BIOS functions
  void DIV_STUB(ARM7TDMI& c, instruction_info& instr);
  
  // Software Interrupt
  void SWI(ARM7TDMI& c, instruction_info& instr);

  void NOP(ARM7TDMI& c, instruction_info& instr);


  void BLL(ARM7TDMI& c, instruction_info& instr);
  void BLH(ARM7TDMI& c, instruction_info& instr);
  
}  // namespace ARM::Instructions
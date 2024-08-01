#pragma once

#include "cpu.hpp"
namespace ARM::Instructions {
  void ADC(ARM7TDMI& c, InstructionInfo& instr);
  void ADD(ARM7TDMI& c, InstructionInfo& instr);
  void AND(ARM7TDMI& c, InstructionInfo& instr);
  void ASR(ARM7TDMI& c, InstructionInfo& instr);
  void B(ARM7TDMI& c, InstructionInfo& instr);
  void BIC(ARM7TDMI& c, InstructionInfo& instr);
  void BL(ARM7TDMI& c, InstructionInfo& instr);
  void BX(ARM7TDMI& c, InstructionInfo& instr);
  void CMN(ARM7TDMI& c, InstructionInfo& instr);
  void CMP(ARM7TDMI& c, InstructionInfo& instr);
  void EOR(ARM7TDMI& c, InstructionInfo& instr);
  void LDMIA(ARM7TDMI& c, InstructionInfo& instr);
  void LDR(ARM7TDMI& c, InstructionInfo& instr);
  void LDRB(ARM7TDMI& c, InstructionInfo& instr);
  void LDRH(ARM7TDMI& c, InstructionInfo& instr);
  void LSL(ARM7TDMI& c, InstructionInfo& instr);
  void LDSB(ARM7TDMI& c, InstructionInfo& instr);
  void LDSH(ARM7TDMI& c, InstructionInfo& instr);
  void LSR(ARM7TDMI& c, InstructionInfo& instr);
  void MOV(ARM7TDMI& c, InstructionInfo& instr);
  void MUL(ARM7TDMI& c, InstructionInfo& instr);
  void MVN(ARM7TDMI& c, InstructionInfo& instr);
  void MSR(ARM7TDMI& c, InstructionInfo& instr);
  void MRS(ARM7TDMI& c, InstructionInfo& instr);
  void STRH(ARM7TDMI& c, InstructionInfo& instr);
  
  
  
}  // namespace ARM::Instructions
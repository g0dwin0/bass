#pragma once

#include "cpu.hpp"

void ADC(ARM7TDMI& c);
void ADD(ARM7TDMI& c);
void AND(ARM7TDMI& c);
void ASR(ARM7TDMI& c);
void B(ARM7TDMI& c, u32 instr);
void BIC(ARM7TDMI& c);
void BL(ARM7TDMI& c);
void BX(ARM7TDMI& c);
void CMN(ARM7TDMI& c);
void CMP(ARM7TDMI& c);
void EOR(ARM7TDMI& c);
void LDMIA(ARM7TDMI& c);
void LDR(ARM7TDMI& c);
void LDRB(ARM7TDMI& c);
void LDRH(ARM7TDMI& c);
void LSL(ARM7TDMI& c);
void LDSB(ARM7TDMI& c);
void LDSH(ARM7TDMI& c);
void LSR(ARM7TDMI& c);
void MOV(ARM7TDMI& c);
void MUL(ARM7TDMI& c);
void MVN(ARM7TDMI& c);
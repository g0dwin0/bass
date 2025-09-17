#pragma once
#include "common/defs.hpp"
struct ARM7TDMI;

enum PSR { CPSR, SPSR_MODE };

enum SHIFT_MODE {
  LSL,
  LSR,
  ASR,
  ROR,
};

void ARM_BRANCH_LINK(ARM7TDMI* cpu, u32 x);
void ARM_SWAP(ARM7TDMI* cpu, u32 x);
void ARM_HALFWORD_LOAD_STORE(ARM7TDMI* cpu, u32 x);
void ARM_SIGNED_LOAD(ARM7TDMI* cpu, u32 x);
void ARM_MULTIPLY(ARM7TDMI* cpu, u32 x);

void ARM_PSR_TRANSFER(ARM7TDMI* cpu, u32 x);
void ARM_DATA_PROCESSING(ARM7TDMI* cpu, u32 x);
void ARM_BRANCH_EXCHANGE(ARM7TDMI* cpu, u32 x);
void ARM_SINGLE_DATA_TRANSFER(ARM7TDMI* c, u32);
void ARM_BLOCK_DATA_TRANSFER(ARM7TDMI* cpu, u32 x);
void ARM_SOFTWARE_INTERRUPT(ARM7TDMI* cpu, u32 x);

void THUMB_ADD_SUB(ARM7TDMI* cpu, u32 opcode);
void THUMB_MOVE_SHIFTED_REG(ARM7TDMI* cpu, u32 opcode);
void THUMB_MOV_CMP_ADD_SUB(ARM7TDMI* cpu, u32 opcode);
void THUMB_DATA_PROC(ARM7TDMI* cpu, u32 opcode);
void THUMB_ADD_CMP_MOV_HI(ARM7TDMI* cpu, u32 opcode);
void THUMB_LDR_PC_REL(ARM7TDMI* cpu, u32 opcode);
void THUMB_CONDITIONAL_BRANCH(ARM7TDMI* cpu, u32 opcode);
void THUMB_UNCONDITIONAL_BRANCH(ARM7TDMI* cpu, u32 opcode);
void THUMB_BLX_PREFIX(ARM7TDMI* cpu, u32 offset);  // BLL
void THUMB_BL_SUFFIX(ARM7TDMI* cpu, u32 offset);   // BLH

void THUMB_HALFWORD_REG_OFFSET(ARM7TDMI* cpu, u32 offset);
void THUMB_HALFWORD_IMM_OFFSET(ARM7TDMI* cpu, u32 offset);
void THUMB_WORD_BYTE_IMM_OFFSET(ARM7TDMI* cpu, u32 opcode);
void THUMB_WORD_BYTE_REG_OFFSET(ARM7TDMI* cpu, u32 opcode);
void THUMB_ADD_SP_PC(ARM7TDMI* cpu, u32 opcode);
void THUMB_ADD_OFFSET_SP(ARM7TDMI* cpu, u32 opcode);
void THUMB_SIGN_EXTENDED_LOAD_STORE_REG_OFFSET(ARM7TDMI* cpu, u32 opcode);
void THUMB_LOAD_STORE_SP_REL(ARM7TDMI* cpu, u32 opcode);

void THUMB_PUSH_POP(ARM7TDMI* cpu, u32 offset);
void THUMB_BLOCK_DATA_TRANSFER(ARM7TDMI* cpu, u32 offset);

void THUMB_LDR_REG_OFFSET(ARM7TDMI* cpu, u32 opcode);
void THUMB_LDR_SP_REL(ARM7TDMI* cpu, u32 opcode);

void AND(ARM7TDMI* cpu, u32 opcode);
void EOR(ARM7TDMI* cpu, u32 opcode);
void SUB(ARM7TDMI* cpu, u32 opcode);
void RSB(ARM7TDMI* cpu, u32 opcode);
void ADD(ARM7TDMI* cpu, u32 opcode);
void ADC(ARM7TDMI* cpu, u32 opcode);
void SBC(ARM7TDMI* cpu, u32 opcode);
void RSC(ARM7TDMI* cpu, u32 opcode);
void TST(ARM7TDMI* cpu, u32 opcode);
void TEQ(ARM7TDMI* cpu, u32 opcode);
void CMP(ARM7TDMI* cpu, u32 opcode);
void CMN(ARM7TDMI* cpu, u32 opcode);
void ORR(ARM7TDMI* cpu, u32 opcode);
void MOV(ARM7TDMI* cpu, u32 opcode);
void BIC(ARM7TDMI* cpu, u32 opcode);
void MVN(ARM7TDMI* cpu, u32 opcode);

void LDR(ARM7TDMI* c, u32 opcode, bool pc_relative = false);
void STR(ARM7TDMI* c, u32 opcode);
void BLH(ARM7TDMI* c, u32 opcode);
void BLL(ARM7TDMI* c, u32 opcode);

void LDRSB(ARM7TDMI* c, u32 opcode);
void LDRSH(ARM7TDMI* c, u32 opcode);

void LDM(ARM7TDMI* c, u32 opcode, bool lr_bit = false, bool thumb_load_store = false);
void STM(ARM7TDMI* c, u32 opcode, bool lr_bit = false, bool thumb_load_store = false);

void OP_MSR(ARM7TDMI* c, u32 opcode);

void check_r15_lookahead(u8 Rn, bool I, bool R, u32& op1);
void single_data_transfer_writeback_str(ARM7TDMI* cpu, u8 Rn, u32 address, u8 added_writeback_value, bool P, bool W);
void single_data_transfer_writeback_ldr(ARM7TDMI* cpu, u8 Rd, u8 Rn, u32 address, u8 added_writeback_value, bool P, bool W);
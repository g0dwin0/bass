#include "instructions/arm.hpp"

#include <cassert>
#include <stdexcept>

#include "common/align.hpp"
#include "common/defs.hpp"
#include "cpu.hpp"
#include "enums.hpp"
#include "registers.hpp"
#include "spdlog/fmt/bundled/base.h"
inline bool is_negative(u32 v) { return v >= 0x80000000; }
inline bool is_zero(u32 v) { return v == 0; }

void ARM_BRANCH_LINK(ARM7TDMI* cpu, u32 x) {
  bool link  = (x & 0x1000000) != 0;
  i32 offset = x & 0xFFFFFF;

  if (offset & 0x800000) {
    offset -= 0x1000000;
  }

  offset *= 4;

  // fmt::println("ARM BRANCH LINK");

  if (link) {
    cpu->regs.get_reg(14) = cpu->regs.r[15] - 4;
  }
  // fmt::println("hi from branch");
  cpu->regs.r[15] += offset;

  cpu->regs.r[15] &= ~1;
  cpu->flush_pipeline();
  return;
}
void THUMB_ADD_CMP_MOV_HI(ARM7TDMI* cpu, u32 opcode) {
  u8 op = (opcode >> 8) & 0b11;
  // fmt::println("HI");

  u8 Rd     = (opcode & 0b111);
  u8 Rs     = ((opcode >> 3) & 0b111);
  bool MSBd = opcode & (1 << 7);
  bool MSBs = opcode & (1 << 6);
  if (MSBd) Rd += 8;
  if (MSBs) Rs += 8;
  u32 Rd_value = cpu->regs.get_reg(Rd);
  u32 Rs_value = cpu->regs.get_reg(Rs);

  switch (op) {
    case 0: {  // ADD
      cpu->regs.get_reg(Rd) += Rs_value;

      if (Rd == 15) {
        cpu->regs.get_reg(Rd) &= ~1;
        cpu->flush_pipeline();
      }
      break;
    }
    case 1: {  // CMP
      u32 result = Rd_value - Rs_value;

      u32 rn_msb  = Rd_value & 0x80000000;
      u32 rm_msb  = Rs_value & 0x80000000;
      u32 res_msb = result & 0x80000000;

      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();
      (Rd_value >= Rs_value) ? cpu->set_carry() : cpu->reset_carry();

      break;
    }
    case 2: {  // MOV R8, R8/NOP
      cpu->regs.get_reg(Rd) = Rs_value;

      if (Rd == 15) {
        cpu->regs.get_reg(Rd) &= ~1;
        cpu->flush_pipeline();
      }
      break;
    }

    case 3: {
      // fmt::println("BX - Rs value: {:#010X}", Rs_value);
      cpu->regs.get_reg(15) = (Rs_value & ~1);

      if ((Rs_value & 1) == 0) {
        cpu->regs.CPSR.STATE_BIT = ARM_MODE;
      }

      cpu->flush_pipeline();
      break;
    }
    default: assert(0);
  }
};
void THUMB_MOV_CMP_ADD_SUB(ARM7TDMI* cpu, u32 opcode) {
  u8 op        = (opcode >> 11) & 0b11;
  u8 Rd        = (opcode >> 8) & 0b111;
  u32 Rd_value = cpu->regs.get_reg(Rd);
  u8 nn        = opcode & 0xFF;

  switch (op) {
    case 0: {
      // fmt::println("MOV");
      cpu->regs.get_reg(Rd) = nn;

      is_negative(nn) ? cpu->set_negative() : cpu->reset_negative();
      nn == 0 ? cpu->set_zero() : cpu->reset_zero();

      break;
    }
    case 1: {
      // fmt::println("CMP");
      u32 result = Rd_value - nn;

      u32 rn_msb  = Rd_value & 0x80000000;
      u32 rm_msb  = nn & 0x80000000;
      u32 res_msb = result & 0x80000000;

      result == 0 ? cpu->set_zero() : cpu->reset_zero();
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();
      Rd_value >= nn ? cpu->set_carry() : cpu->reset_carry();

      break;
    }
    case 2: {
      // fmt::println("ADD");

      u64 result_64 = (u64)Rd_value + nn;
      u32 result_32 = Rd_value + nn;

      cpu->regs.get_reg(Rd) = result_32;

      result_32 == 0 ? cpu->set_zero() : cpu->reset_zero();

      u32 overflow = ~(Rd_value ^ nn) & (Rd_value ^ result_64);
      ((overflow >> 31) & 1) ? cpu->set_overflow() : cpu->reset_overflow();

      (result_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();
      is_negative(result_32) ? cpu->set_negative() : cpu->reset_negative();

      break;
    }
    case 3: {
      // fmt::println("SUB");
      u32 result = Rd_value - nn;

      cpu->regs.get_reg(Rd) = result;

      u32 rn_msb  = Rd_value & 0x80000000;
      u32 rm_msb  = nn & 0x80000000;
      u32 res_msb = result & 0x80000000;

      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      result == 0 ? cpu->set_zero() : cpu->reset_zero();
      Rd_value >= nn ? cpu->set_carry() : cpu->reset_carry();
      (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();

      break;
    }
  }

  // fmt::println("LO");
};

void THUMB_LOAD_STORE_SP_REL(ARM7TDMI* cpu, u32 opcode) {
  bool load = opcode & (1 << 11);

  u8 Rd  = (opcode >> 8) & 0b111;
  u16 nn = (opcode & 0xFF) * 4;
  //   fmt::println("HI");
  // fmt::println("nn: {}", nn);
  //   fmt::println("mode bit: {}", static_cast<u8>(cpu->regs.CPSR.MODE_BIT));
  //   fmt::println("{:#08X}", static_cast<u32>(cpu->regs.get_reg(13)));
  //   fmt::println("SP+nn = {:#010X}", cpu->regs.get_reg(13) + nn);

  if (load) {
    // fmt::println("[LDR] read result {:#010X}", cpu->bus->read32(cpu->regs.get_reg(13) + nn));
    u32 v                 = cpu->bus->read32(cpu->regs.get_reg(13) + nn);
    cpu->regs.get_reg(Rd) = std::rotr(v, (cpu->regs.get_reg(13) + nn) & 3);
    // if()
  } else {
    u32 r = cpu->regs.get_reg(13) + nn;
    // fmt::println("[STR] read result {:#010X} -> {:#010x}", cpu->regs.get_reg(13) + nn, r);

    // fmt::println("WRITING {:#010X} to {:#010X}", cpu->regs.get_reg(Rd), r);
    cpu->bus->write32(r, cpu->regs.get_reg(Rd));
  }
}

void THUMB_SIGN_EXTENDED_LOAD_STORE_REG_OFFSET(ARM7TDMI* cpu, u32 opcode) {
  //   0: STRH Rd,[Rb,Ro]  ;store 16bit data          HALFWORD[Rb+Ro] = Rd
  // 1: LDSB Rd,[Rb,Ro]  ;load sign-extended 8bit   Rd = BYTE[Rb+Ro]
  // 2: LDRH Rd,[Rb,Ro]  ;load zero-extended 16bit  Rd = HALFWORD[Rb+Ro]
  // 3: LDSH Rd,[Rb,Ro]  ;load sign-extended 16bit  Rd = HALFWORD[Rb+Ro]
  // fmt::println("ext");
  u8 op = (opcode >> 10) & 0b11;
  u8 Rd = opcode & 0b111;
  u8 Rn = (opcode >> 3) & 0b111;
  u8 Rm = (opcode >> 6) & 0b111;

  u32 value           = cpu->regs.get_reg(Rd);
  u32 address         = cpu->regs.get_reg(Rn);
  u32 register_offset = cpu->regs.get_reg(Rm);
  switch (op) {
    case 0: {
      // STRH
      // fmt::println("STRH");
      address += register_offset;
      cpu->bus->write16(address, static_cast<u16>(value));
      break;
    }
    case 1: {
      // LDSB
      // fmt::println("LDSB");
      address += register_offset;
      value = cpu->bus->read8(address);
      if (value & (1 << 7)) value |= 0xffffff00;
      cpu->regs.get_reg(Rd) = value;
      break;
    }
    case 2: {
      // LDRH
      // fmt::println("LDRH");
      address += register_offset;
      u32 value = cpu->bus->read16(address);
      if (address == 0x030000b1) fmt::println("address: {:#010x} - value: {:#010x} - {:#010x}", address, value, std::rotr(value, (address & 1) * 8));

      cpu->regs.get_reg(Rd) = std::rotr(value, (address & 1) * 8);
      break;
    }
    case 3: {
      // LDSH
      address += register_offset;
      // fmt::println("{:#010X}", address);
      u32 value = cpu->bus->read16(address);
      if ((address & 1) == 0) {  // aligned addr

        if (value & (1 << 15)) value |= 0xFFFF0000;

      } else {
        // fmt::println("odd addr");
        value = cpu->bus->read8(address);
        if (value & (1 << 7)) value |= 0xFFFFFF00;
      }

      // if ((value & 0xff) == 6) fmt::println("LDSH -- value: {:#010x} - address: {:#010x}", value, address);

      cpu->regs.get_reg(Rd) = value;
      break;
    }
  }
}

void THUMB_HALFWORD_IMM_OFFSET(ARM7TDMI* cpu, u32 opcode) {
  u8 op = (opcode >> 11) & 1;
  u8 Rd = opcode & 0b111;
  u8 Rn = (opcode >> 3) & 0b111;

  u32 address = cpu->regs.get_reg(Rn);
  u32 offset  = ((opcode >> 6) & 0b11111) * 2;
  switch (op) {
    case 0: {
      // STRH
      // fmt::println("STRH");
      u32 value = cpu->regs.get_reg(Rd);
      address += offset;
      cpu->bus->write16(address, static_cast<u16>(value));
      break;
    }
    case 1: {
      // LDRH
      // fmt::println("LDRH");
      address += offset;
      u32 value             = cpu->bus->read16(address);
      cpu->regs.get_reg(Rd) = std::rotr(value, (address & 1) * 8);
      break;
    }
  }
}

void THUMB_WORD_BYTE_IMM_OFFSET(ARM7TDMI* cpu, u32 opcode) {
  // 0: STR  Rd,[Rb,#nn]  ;store 32bit data   WORD[Rb+nn] = Rd
  // 1: LDR  Rd,[Rb,#nn]  ;load  32bit data   Rd = WORD[Rb+nn]
  // 2: STRB Rd,[Rb,#nn]  ;store  8bit data   BYTE[Rb+nn] = Rd
  // 3: LDRB Rd,[Rb,#nn]  ;load   8bit data   Rd = BYTE[Rb+nn]

  u8 op     = (opcode >> 11) & 0b11;
  u8 Rd     = opcode & 0b111;
  u8 Rn     = (opcode >> 3) & 0b111;    // Rb
  u8 offset = (opcode >> 6) & 0b11111;  // nn

  u32 value   = cpu->regs.get_reg(Rd);
  u32 address = cpu->regs.get_reg(Rn);

  switch (op) {
    case 0: {
      // STR
      address += (offset * 4);
      cpu->bus->write32(address, value);
      break;
    }
    case 1: {
      // LDR
      address += (offset * 4);
      u32 v                 = cpu->bus->read32(address);
      cpu->regs.get_reg(Rd) = std::rotr(v, (address & 3) * 8);
      break;
    }
    case 2: {
      // STRB
      address += offset;
      cpu->bus->write8(address, (u8)value);
      break;
    }
    case 3: {
      // LDRB
      address += offset;
      cpu->regs.get_reg(Rd) = cpu->bus->read8(address);
      break;
    }
  }
}

void THUMB_WORD_BYTE_REG_OFFSET(ARM7TDMI* cpu, u32 opcode) {
  // 0: STR  Rd,[Rb,#nn]  ;store 32bit data   WORD[Rb+nn] = Rd
  // 1: LDR  Rd,[Rb,#nn]  ;load  32bit data   Rd = WORD[Rb+nn]
  // 2: STRB Rd,[Rb,#nn]  ;store  8bit data   BYTE[Rb+nn] = Rd
  // 3: LDRB Rd,[Rb,#nn]  ;load   8bit data   Rd = BYTE[Rb+nn]
  u8 op               = (opcode >> 10) & 0b11;
  u8 Rd               = opcode & 0b111;
  u8 Rn               = (opcode >> 3) & 0b111;  // Rb
  u32 register_offset = cpu->regs.get_reg((opcode >> 6) & 0b111);

  u32 value   = cpu->regs.get_reg(Rd);
  u32 address = cpu->regs.get_reg(Rn) + register_offset;

  switch (op) {
    case 0: {
      // STR
      cpu->bus->write32(address, value);
      break;
    }
    case 1: {
      // STRB
      cpu->bus->write8(address, (u8)value);
      break;
    }
    case 2: {
      // LDR
      cpu->regs.get_reg(Rd) = cpu->bus->read32(address);
      break;
    }
    case 3: {
      // LDRB
      cpu->regs.get_reg(Rd) = cpu->bus->read8(address);
      break;
    }
  }
}

void THUMB_ADD_OFFSET_SP(ARM7TDMI* cpu, u32 opcode) {
  u16 offset    = (opcode & 0b1111111) * 4;
  bool negative = opcode & (1 << 7);

  if (negative) {
    cpu->regs.get_reg(13) = cpu->regs.get_reg(13) - offset;
  } else {
    cpu->regs.get_reg(13) = cpu->regs.get_reg(13) + offset;
  }
}

void THUMB_ADD_SP_PC(ARM7TDMI* cpu, u32 opcode) {  // THUMB 12
  u8 Rd      = (opcode >> 8) & 0b111;
  u16 offset = (opcode & 0xFF) * 4;
  u8 op      = (opcode & (1 << 11)) >> 11;

  switch (op) {
    case 0: {
      cpu->regs.get_reg(Rd) = (cpu->regs.r[15] & ~2) + offset;
      break;
    }

    case 1: {
      cpu->regs.get_reg(Rd) = cpu->regs.get_reg(13) + offset;
      break;
    }
  }
}

void THUMB_BLX_PREFIX(ARM7TDMI* cpu, u32 opcode) { BLL(cpu, opcode & 0b11111111111); };
void THUMB_BL_SUFFIX(ARM7TDMI* cpu, u32 opcode) { BLH(cpu, opcode & 0b11111111111); };  // BLH

void ARM_BRANCH_EXCHANGE(ARM7TDMI* cpu, u32 x) {
  u8 Rn = x & 0xF;

  cpu->regs.r[15]          = cpu->regs.get_reg(Rn);
  cpu->regs.CPSR.STATE_BIT = (CPU_MODE)(cpu->regs.get_reg(Rn) & 0b1);

  // cpu->regs.CPSR.STATE_BIT ? SPDLOG_INFO("entered THUMB mode") : (void)(0);
  cpu->regs.r[15] = cpu->align_by_current_mode(cpu->regs.r[15]);

  cpu->flush_pipeline();
  return;
}

void ARM_PSR_TRANSFER(ARM7TDMI* cpu, u32 x) {
  enum PSR_TRANSFER_TYPE : u8 { MRS, MSR };

  PSR P = (x & (1 << 22)) ? SPSR_MODE : CPSR;

  PSR_TRANSFER_TYPE transfer_type = static_cast<PSR_TRANSFER_TYPE>((x & (1 << 21)) >> 21);

  u32 psr;

  switch (transfer_type) {
    case MRS: {
      u8 Rd = (x >> 12) & 0xF;
      if (P == CPSR) {
        psr = cpu->regs.CPSR.value;
      } else {
        psr = cpu->regs.get_spsr(cpu->regs.CPSR.MODE_BIT);
      }

      cpu->regs.get_reg(Rd) = psr;

      if (Rd == 15) cpu->flush_pipeline();

      break;
    }

    case MSR: {
      // fmt::println("MSR");
      OP_MSR(cpu, x);
      break;
    }
  }

  // fmt::println("UNIMPL");
  // assert(0);
}
void ARM_MULTIPLY(ARM7TDMI* cpu, u32 x) {
  u8 Rd         = (x >> 16) & 0xF;
  u8 Rn         = (x >> 12) & 0xF;
  u8 Rs         = (x >> 8) & 0xF;
  u8 Rm         = x & 0xF;
  u8 mul_opcode = (x >> 21) & 0xF;
  bool S        = x & (1 << 20);
  // fmt::println("mull called -- {:#010X}", x);
  switch (mul_opcode) {
    case 0x0: {  // MUL
      auto _rm = Rm == 15 ? cpu->regs.get_reg(Rm) + 4 : cpu->regs.get_reg(Rm);
      auto _rs = Rs == 15 ? cpu->regs.get_reg(Rs) + 4 : cpu->regs.get_reg(Rs);

      auto result = (_rm * _rs);

      if (Rd == 15) result += 4;

      if (S) {
        result == 0 ? cpu->set_zero() : cpu->reset_zero();
        result >= 0x80000000 ? cpu->set_negative() : cpu->reset_negative();

        cpu->reset_carry();
      }
      cpu->regs.get_reg(Rd) = result;
      if (Rd == 15) cpu->flush_pipeline();
      break;
    }
    case 0x1: {  // MLA

      auto _rm = Rm == 15 ? cpu->regs.get_reg(Rm) + 4 : cpu->regs.get_reg(Rm);
      auto _rs = Rs == 15 ? cpu->regs.get_reg(Rs) + 4 : cpu->regs.get_reg(Rs);
      auto _rn = Rn == 15 ? cpu->regs.get_reg(Rn) + 4 : cpu->regs.get_reg(Rn);

      auto result = (_rm * _rs) + _rn;

      // seems to be the case for ARM mode, could be different in thumb
      if (Rd == 15) result += 4;

      if (S) {
        result == 0 ? cpu->set_zero() : cpu->reset_zero();
        result >= 0x80000000 ? cpu->set_negative() : cpu->reset_negative();
      }

      cpu->regs.get_reg(Rd) = result;
      if (Rd == 15) cpu->flush_pipeline();
      break;
    }
    case 0x2: {
      fmt::println("{:010X}", x);

      throw std::runtime_error("NO!");
      assert(0);
      break;
    }
    case 0x3: {
      throw std::runtime_error("I'm not documentd");
    }
    case 0x4: {  // UMULL
      u64 res = static_cast<uint64_t>(cpu->regs.get_reg(Rm)) * cpu->regs.get_reg(Rs);

      cpu->regs.get_reg(Rd) = res >> 32;
      cpu->regs.get_reg(Rn) = res & 0xffffffff;

      if (S) {
        (res & 0x8000000000000000) ? cpu->set_negative() : cpu->reset_negative();
        res == 0 ? cpu->set_zero() : cpu->reset_zero();
        cpu->reset_carry();
      }

      break;
    }
    case 0x5: {  // UMLAL
      // fmt::println("UMLAL");
      u64 big_number = (static_cast<u64>(cpu->regs.get_reg(Rd)) << 32) + cpu->regs.get_reg(Rn);
      u64 res        = big_number + (static_cast<u64>(cpu->regs.get_reg(Rm)) * static_cast<u64>(cpu->regs.r[Rs]));

      cpu->regs.get_reg(Rd) = res >> 32;
      cpu->regs.get_reg(Rn) = res & 0xffffffff;

      if (S) {
        (res & 0x8000000000000000) ? cpu->set_negative() : cpu->reset_negative();
        res == 0 ? cpu->set_zero() : cpu->reset_zero();

        // assert(0);
      }
      cpu->reset_carry();
      break;
    }
    case 0x6: {  // SMULL

      i32 rm  = static_cast<u32>(cpu->regs.get_reg(Rm));
      i32 rs  = static_cast<u32>(cpu->regs.r[Rs]);
      i64 res = (i64)rm * rs;

      SPDLOG_DEBUG("LO: {:#x}", lo);
      cpu->regs.get_reg(Rn) = Rn == 15 ? (u32)(res & 0xffffffff) + 4 : (u32)(res & 0xffffffff);
      cpu->regs.get_reg(Rd) = (u32)(res >> 32);

      if (S) {
        (static_cast<u64>(res) >= 0x8000000000000000) ? cpu->set_negative() : cpu->reset_negative();
        res == 0 ? cpu->set_zero() : cpu->reset_zero();
        cpu->reset_carry();
      }

      break;
    }
    case 0x7: {  // SMLAL
      i64 rdhi = Rd == 15 ? cpu->regs.get_reg(Rd) + 4 : cpu->regs.get_reg(Rd);
      i64 rdlo = Rn == 15 ? cpu->regs.get_reg(Rn) + 4 : cpu->regs.get_reg(Rn);
      u32 _rm  = Rm == 15 ? cpu->regs.get_reg(Rm) + 4 : cpu->regs.get_reg(Rm);
      u32 _rs  = Rs == 15 ? cpu->regs.get_reg(Rs) + 4 : cpu->regs.get_reg(Rs);

      i64 rm = static_cast<i32>(_rm);
      i64 rs = static_cast<i32>(_rs);

      i64 big_number = ((rdhi) << 32) + rdlo;

      i64 res = (i64)(rm * rs) + big_number;

      cpu->regs.get_reg(Rd) = res >> 32;
      cpu->regs.get_reg(Rn) = res & 0xffffffff;

      if (S) {
        (res & 0x8000000000000000) ? cpu->set_negative() : cpu->reset_negative();
        res == 0 ? cpu->set_zero() : cpu->reset_zero();

        // assert(0);
      }
      cpu->reset_carry();
      break;
    }
    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB: assert(0);
  }

  // spdlog::info("[HANDLER] called MUL handler -- opcode: {:#08x}", x);
  // fmt::println("UNIMPL");
  // assert(0);
  return;
}

void ARM_HALFWORD_LOAD_STORE(ARM7TDMI* cpu, u32 x) {
  bool P    = x & (1 << 24);
  bool U    = x & (1 << 23);
  bool I    = x & (1 << 22);
  bool W    = x & (1 << 21);
  bool load = x & (1 << 20);
  u8 Rn     = (x >> 16) & 0xF;
  u8 Rd     = (x >> 12) & 0xF;
  u8 imm    = (u8)((((x >> 8) & 0xF) << 4) | (x & 0xF));
  u8 Rm     = x & 0xF;

  // spdlog::info("[HANDLER] called HALFWORD LOAD STORE handler -- opcode: {:#08x}", x);
  // spdlog::info("immediate: {:#04X}", imm);
  // spdlog::info("P: {:#04X}", P);
  // spdlog::info("U: {:#04X}", U);
  // spdlog::info("I: {:#04X}", I);
  // spdlog::info("W: {:#04X}", W);
  // spdlog::info("L: {:#04X}", load);
  // spdlog::info("Rd: {:#04X}", Rd);
  // spdlog::info("Rn: {:#04X}", Rn);

  if (load) {
    u32 address              = cpu->regs.get_reg(Rn);
    u8 immediate_offset      = imm;
    u32 register_offset      = cpu->regs.get_reg(Rm);
    u8 added_writeback_value = 0;
    if (Rn == 15) added_writeback_value += 4;

    // On ARM7 aka ARMv4 aka NDS7/GBA:

    // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
    // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value

    u8 misaligned_load_rotate_value = 0;

    if (P == 0) {  // P == 0

      if ((address & 1) != 0) misaligned_load_rotate_value = 8;

      // fmt::println("{:#010x}", address);
      u32 value = cpu->bus->read16(address);

      value                 = std::rotr(value, misaligned_load_rotate_value);
      cpu->regs.get_reg(Rd) = value;

      if (U) {
        address += (I ? immediate_offset : register_offset);
      } else {
        address -= (I ? immediate_offset : register_offset);
      }

    } else {  // P == 1

      if (U) {
        address += (I ? immediate_offset : register_offset);
      } else {
        address -= (I ? immediate_offset : register_offset);
      }

      if ((address & 1) != 0) misaligned_load_rotate_value = 8;

      u32 value = cpu->bus->read16(address);

      value                 = std::rotr(value, misaligned_load_rotate_value);
      cpu->regs.get_reg(Rd) = value;
    }

    // writeback logic
    if (Rd == 15) {
      cpu->regs.r[15] = align(cpu->regs.r[15], cpu->regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
      cpu->flush_pipeline();
    }

    if (Rd == Rn) return;

    if (P == 0 || (P == 1 && W)) {
      // fmt::println("instr rn: {}", Rn);

      u32 writeback_addr    = address + added_writeback_value;
      cpu->regs.get_reg(Rn) = writeback_addr;

      if (Rn == 15) {
        // fmt::println("R15: {:#010x}", cpu->regs.get_reg(Rn));
        cpu->flush_pipeline();
      }
    }
  } else {  // Store
    u32 value                = cpu->regs.get_reg(Rd);
    u32 address              = cpu->regs.get_reg(Rn);
    u8 immediate_offset      = imm;
    u32 register_offset      = cpu->regs.get_reg(Rm);
    u8 added_writeback_value = 0;
    if (Rn == 15) added_writeback_value += 4;
    if (Rd == 15) value += 4;

    // fmt::println("addr: {:#010x}", address);

    if (P == 0) {  // P == 0

      cpu->bus->write16(address, (u16)value);

      if (U) {
        address += (I ? immediate_offset : register_offset);
      } else {
        address -= (I ? immediate_offset : register_offset);
      }

    } else {  // P == 1

      if (U) {
        address += (I ? immediate_offset : register_offset);
      } else {
        address -= (I ? immediate_offset : register_offset);
      }

      cpu->bus->write16(address, (u16)value);
    }

    if (P == 0 || (P == 1 && W)) {
      u32 writeback_addr    = address + added_writeback_value;
      cpu->regs.get_reg(Rn) = writeback_addr;

      // If we wrote to R15, we need to flush, as the pipeline is now invalid.
      if (Rn == 15) {
        cpu->regs.r[15] = align(writeback_addr, cpu->regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
        cpu->flush_pipeline();
      }
    }
  }

  return;
}

void ARM_SWAP(ARM7TDMI* cpu, u32 x) {
  bool B = x & (1 << 22);
  u8 Rn  = (x >> 16) & 0xF;
  u8 Rd  = (x >> 12) & 0xF;
  u8 Rm  = x & 0xF;

  spdlog::info("[HANDLER] called SWAP handler -- opcode: {:#08x}", x);
  u32 swap_value = cpu->regs.get_reg(Rm);
  u32 address    = cpu->regs.get_reg(Rn);

  if (Rm == 15) swap_value += 4;
  if (Rn == 15) address += 4;

  if (B) {
    u8 byte_at_loc        = cpu->bus->read8(address);
    cpu->regs.get_reg(Rd) = byte_at_loc;
    cpu->bus->write8(address, (u8)swap_value);
  } else {
    u32 data              = cpu->bus->read32(address);
    cpu->regs.get_reg(Rd) = std::rotr(data, (address & 3) * 8);
    cpu->bus->write32(address, swap_value);
  }

  if (Rd == 15) {
    cpu->regs.r[15] = cpu->align_by_current_mode(cpu->regs.r[15]);
    cpu->flush_pipeline();
  }
}

void ARM_SOFTWARE_INTERRUPT(ARM7TDMI* cpu, u32 x) {
  (void)x;

  auto cpsr = cpu->regs.CPSR.value;

  cpu->regs.CPSR.MODE_BIT    = SUPERVISOR;
  cpu->regs.svc_r[14]        = cpu->regs.r[15] - (cpu->regs.CPSR.STATE_BIT == THUMB_MODE ? 2 : 4);
  cpu->regs.SPSR_svc         = cpsr;
  cpu->regs.CPSR.STATE_BIT   = ARM_MODE;
  cpu->regs.CPSR.irq_disable = true;

  cpu->regs.r[15] = 0x00000008;

  cpu->flush_pipeline();
}

void ARM_DATA_PROCESSING(ARM7TDMI* cpu, u32 x) {
  (void)(cpu);
  (void)(x);

  ALU_OP data_proc_op = static_cast<ALU_OP>((x >> 21) & 0xF);

  switch (data_proc_op) {
    case ALU_OP::AND: AND(cpu, x); break;
    case ALU_OP::EOR: EOR(cpu, x); break;
    case ALU_OP::SUB: SUB(cpu, x); break;
    case ALU_OP::RSB: RSB(cpu, x); break;
    case ALU_OP::ADD: ADD(cpu, x); break;
    case ALU_OP::ADC: ADC(cpu, x); break;
    case ALU_OP::SBC: SBC(cpu, x); break;
    case ALU_OP::RSC: RSC(cpu, x); break;
    case ALU_OP::TST: TST(cpu, x); break;
    case ALU_OP::TEQ: TEQ(cpu, x); break;
    case ALU_OP::CMP: CMP(cpu, x); break;
    case ALU_OP::CMN: CMN(cpu, x); break;
    case ALU_OP::ORR: ORR(cpu, x); break;
    case ALU_OP::MOV: MOV(cpu, x); break;
    case ALU_OP::BIC: BIC(cpu, x); break;
    case ALU_OP::MVN: MVN(cpu, x); break;
  }
}
void ARM_SIGNED_LOAD(ARM7TDMI* cpu, u32 x) {
  (void)(cpu);
  (void)(x);
  u8 itr = (x >> 5) & 0b11;  // TODO: rename to useful var

  if (itr == 2) {
    LDRSB(cpu, x);
    return;
  }
  if (itr == 3) {
    LDRSH(cpu, x);
    return;
  }

  assert(0);
}

void ARM_BLOCK_DATA_TRANSFER(ARM7TDMI* cpu, u32 opcode) {
  bool load = opcode & (1 << 20);

  if (load) {
    LDM(cpu, opcode);
  } else {
    STM(cpu, opcode);
  }
}
void ARM_SINGLE_DATA_TRANSFER(ARM7TDMI* cpu, u32 opcode) {
  bool load = opcode & (1 << 20);

  if (load) {
    LDR(cpu, opcode);
  } else {
    STR(cpu, opcode);
  }
}
void THUMB_LDR_PC_REL(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd   = (opcode >> 8) & 0b111;
  u16 imm = (opcode & 0xFF) * 4;

  cpu->regs.get_reg(Rd) = cpu->bus->read32((cpu->regs.get_reg(15) & ~2) + imm);
}

void THUMB_LDR_STR_REG(ARM7TDMI* cpu, u32 opcode) { LDR(cpu, opcode, true); }

void THUMB_LDR_STR_IMM(ARM7TDMI* cpu, u32 opcode) { LDR(cpu, opcode, true); }

void THUMB_PUSH_POP(ARM7TDMI* cpu, u32 opcode) {
  bool load = opcode & (1 << 11);
  bool lr   = opcode & (1 << 8);
  if (load) {
    // fmt::println("POP");
    LDM(cpu, opcode, lr);
  } else {
    // fmt::println("PUSH");
    STM(cpu, opcode, lr);
  }
}

void THUMB_BLOCK_DATA_TRANSFER(ARM7TDMI* cpu, u32 opcode) {
  bool load = opcode & (1 << 11);

  if (load) {
    LDM(cpu, opcode, false, true);
  } else {
    STM(cpu, opcode, false, true);
  }
}

void THUMB_CONDITIONAL_BRANCH(ARM7TDMI* cpu, u32 opcode) {
  Condition c = static_cast<Condition>((opcode >> 8) & 0xF);

  if (cpu->check_condition(c)) {
    int x = (opcode & 0xff);

    if (x & 0x80) {
      x -= 0x100;
    }
    x *= 2;

    // cpu->regs.r
    cpu->regs.r[15] += x;

    cpu->regs.r[15] &= ~1;

    cpu->flush_pipeline();
  };
}

void THUMB_UNCONDITIONAL_BRANCH(ARM7TDMI* cpu, u32 opcode) {
  int x = (opcode & 0x7ff);

  if (x & 0x400) {
    x -= 0x800;
  }

  x *= 2;

  // cpu->regs.r
  cpu->regs.r[15] += x;
  cpu->flush_pipeline();
}
void THUMB_DATA_PROC(ARM7TDMI* cpu, u32 opcode) {
  THUMB_ALU_OP op = static_cast<THUMB_ALU_OP>((opcode >> 6) & 0xF);
  u8 Rd           = opcode & 0b111;
  u8 Rs           = (opcode >> 3) & 0b111;
  u32 Rd_value    = cpu->regs.get_reg(Rd);
  u32 Rs_value    = cpu->regs.get_reg(Rs);
  // fmt::println("{:#08X} dd", (opcode >> 6) & 0xF);
  switch (op) {  // handle op
    case THUMB_ALU_OP::AND: {
      u32 result = Rd_value & Rs_value;

      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::EOR: {
      u32 result = Rd_value ^ Rs_value;

      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::LSL: {
      u32 result = cpu->shift(ARM7TDMI::LSL, Rd_value, (Rs_value & 0xFF), false);
      // fmt::println("amount: {}", (Rs_value & 0xFF));
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::LSR: {
      u32 result = cpu->shift(ARM7TDMI::LSR, Rd_value, (Rs_value & 0xFF), false);

      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::ASR: {
      u32 result = cpu->shift(ARM7TDMI::ASR, Rd_value, (Rs_value & 0xFF), false);
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::ADC: {
      bool old_carry = cpu->regs.CPSR.CARRY_FLAG;

      u64 result_64 = ((u64)Rd_value + Rs_value + old_carry);
      u32 result_32 = (Rd_value + Rs_value + old_carry);

      result_32 == 0 ? cpu->set_zero() : cpu->reset_zero();

      u32 overflow = ~(Rd_value ^ Rs_value) & (Rd_value ^ result_32);
      ((overflow >> 31) & 1) ? cpu->set_overflow() : cpu->reset_overflow();

      (result_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();
      is_negative(result_32) ? cpu->set_negative() : cpu->reset_negative();

      cpu->regs.get_reg(Rd) = (Rd_value + Rs_value + old_carry);
      break;
    }
    case THUMB_ALU_OP::SBC: {
      bool old_carry = cpu->regs.CPSR.CARRY_FLAG;

      u32 result = Rd_value - Rs_value + (old_carry - 1);

      u32 rn_msb  = Rd_value & 0x80000000;
      u32 rm_msb  = Rs_value & 0x80000000;
      u32 res_msb = result & 0x80000000;

      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      (Rd_value >= ((u64)Rs_value + (1 - old_carry))) ? cpu->set_carry() : cpu->reset_carry();
      ((rn_msb != rm_msb) && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();

      cpu->regs.get_reg(Rd) = result;

      break;
    }
    case THUMB_ALU_OP::ROR: {
      u32 result = cpu->shift(ARM7TDMI::ROR, Rd_value, (Rs_value & 0xFF), false);

      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::TST: {
      u32 d = Rd_value & Rs_value;

      is_zero(d) ? cpu->set_zero() : cpu->reset_zero();
      is_negative(d) ? cpu->set_negative() : cpu->reset_negative();
      break;
    }
    case THUMB_ALU_OP::NEG: {
      u32 result            = 0 - Rs_value;
      cpu->regs.get_reg(Rd) = result;

      u32 op2_msb = Rs_value & 0x80000000;
      u32 rn_msb  = 0 & 0x80000000;
      u32 res_msb = result & 0x80000000;

      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      ((op2_msb ^ res_msb) & (op2_msb ^ rn_msb)) ? cpu->set_overflow() : cpu->reset_overflow();
      Rs_value == 0 ? cpu->set_carry() : cpu->reset_carry();
      break;
    }
    case THUMB_ALU_OP::CMP: {
      u32 result = Rd_value - Rs_value;

      u32 rn_msb  = Rd_value & 0x80000000;
      u32 rm_msb  = Rs_value & 0x80000000;
      u32 res_msb = result & 0x80000000;

      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();
      (Rd_value >= Rs_value) ? cpu->set_carry() : cpu->reset_carry();

      break;
    }
    case THUMB_ALU_OP::CMN: {
      u64 r_64 = (u64)Rd_value + Rs_value;
      u32 r    = Rd_value + Rs_value;

      u32 rn_msb  = Rd_value & 0x80000000;
      u32 rm_msb  = Rs_value & 0x80000000;
      u32 res_msb = r & 0x80000000;

      is_negative(r) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(r) ? cpu->set_zero() : cpu->reset_zero();
      (r_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();

      if (rn_msb != 0 && rm_msb != 0 && res_msb == 0) {
        cpu->set_overflow();
      } else if (rn_msb == 0 && rm_msb == 0 && res_msb != 0) {
        cpu->set_overflow();
      } else {
        cpu->reset_overflow();
      }

      break;
    }
    case THUMB_ALU_OP::ORR: {
      u32 result = Rd_value | Rs_value;

      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::MUL: {
      u32 result = Rd_value * Rs_value;

      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      cpu->reset_carry();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::BIC: {
      u32 result = Rd_value & ~Rs_value;

      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case THUMB_ALU_OP::MVN: {
      // fmt::println("MVN: {:#08X} - {:#08X}", (u32)Rs_value, (u32)~Rs_value);
      u32 result            = ~Rs_value;
      cpu->regs.get_reg(Rd) = result;
      is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
      is_zero(result) ? cpu->set_zero() : cpu->reset_zero();

      break;
    }
  }
}

void THUMB_MOVE_SHIFTED_REG(ARM7TDMI* cpu, u32 opcode) {
  u8 op     = (opcode >> 11) & 0b11;
  u8 Rs     = (opcode >> 3) & 0b111;
  u8 Rd     = (opcode) & 0b111;
  u8 offset = (opcode >> 6) & 0b11111;

  u32 source_value = cpu->regs.get_reg(Rs);
  // fmt::println("move shifted reg - Rs: {} - Rd: {} - offset: {}", Rs, Rd, offset);

  switch (op) {
    case 0: {
      source_value = cpu->shift(ARM7TDMI::SHIFT_MODE::LSL, source_value, offset, true, true, true);
      break;
    }
    case 1: {
      source_value = cpu->shift(ARM7TDMI::SHIFT_MODE::LSR, source_value, offset, true, true, true);

      break;
    }
    case 2: {
      source_value = cpu->shift(ARM7TDMI::SHIFT_MODE::ASR, source_value, offset, true, true, true);
      break;
    }
    case 3: {
      assert(0);
      break;
    }
  }

  is_zero(source_value) ? cpu->set_zero() : cpu->reset_zero();
  is_negative(source_value) ? cpu->set_negative() : cpu->reset_negative();
  if (Rd == 15) {
    cpu->regs.load_spsr_to_cpsr();
  }

  cpu->regs.get_reg(Rd) = source_value;

  if (Rd == 15) {
    cpu->regs.get_reg(Rd) = cpu->align_by_current_mode(cpu->regs.get_reg(Rd));
    cpu->flush_pipeline();
  }
}

void THUMB_ADD_SUB(ARM7TDMI* cpu, u32 opcode) {  // THUMB.2
  u8 op        = ((opcode >> 9) & 0b11);
  u32 Rs_value = cpu->regs.get_reg((opcode >> 3) & 0b111);
  u32 Rn_value = cpu->regs.get_reg((opcode >> 6) & 0b111);
  u8 reg_imm   = (opcode >> 6) & 0b111;  // both used as register operand, and immediate (Rn && nn)
  u8 Rd        = opcode & 0b111;
  // fmt::println("add sub: {}", op);
  switch (op) {
    case 0: {
      u64 result_64 = (u64)Rs_value + Rn_value;
      u32 result_32 = Rs_value + Rn_value;

      cpu->regs.get_reg(Rd) = result_32;

      result_32 == 0 ? cpu->set_zero() : cpu->reset_zero();

      u32 overflow = ~(Rs_value ^ Rn_value) & (Rs_value ^ result_64);
      ((overflow >> 31) & 1) ? cpu->set_overflow() : cpu->reset_overflow();

      (result_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();
      is_negative(result_32) ? cpu->set_negative() : cpu->reset_negative();

      break;
    }
    case 1: {
      u32 result = Rs_value - Rn_value;

      u32 rn_msb  = Rs_value & 0x80000000;
      u32 rm_msb  = Rn_value & 0x80000000;
      u32 res_msb = result & 0x80000000;

      is_negative((Rs_value - Rn_value)) ? cpu->set_negative() : cpu->reset_negative();
      is_zero((Rs_value - Rn_value)) ? cpu->set_zero() : cpu->reset_zero();

      Rs_value >= Rn_value ? cpu->set_carry() : cpu->reset_carry();
      (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
    case 2: {
      u64 result_64 = (u64)Rs_value + reg_imm;
      u32 result_32 = Rs_value + reg_imm;

      cpu->regs.get_reg(Rd) = result_32;

      result_32 == 0 ? cpu->set_zero() : cpu->reset_zero();

      u32 overflow = ~(Rs_value ^ reg_imm) & (Rs_value ^ result_64);
      ((overflow >> 31) & 1) ? cpu->set_overflow() : cpu->reset_overflow();

      (result_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();
      is_negative(result_32) ? cpu->set_negative() : cpu->reset_negative();

      break;
    }
    case 3: {
      u32 result = Rs_value - reg_imm;

      u32 rn_msb  = Rs_value & 0x80000000;
      u32 rm_msb  = reg_imm & 0x80000000;
      u32 res_msb = result & 0x80000000;

      is_negative((Rs_value - reg_imm)) ? cpu->set_negative() : cpu->reset_negative();
      is_zero((Rs_value - reg_imm)) ? cpu->set_zero() : cpu->reset_zero();

      Rs_value >= reg_imm ? cpu->set_carry() : cpu->reset_carry();
      (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();

      cpu->regs.get_reg(Rd) = result;
      break;
    }
  }
}

void AND(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  check_r15_lookahead(Rn, I, R, op1);

  u32 result = op1 & op2;

  if (S) {
    is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
    is_zero(result) ? cpu->set_zero() : cpu->reset_zero();

    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->flush_pipeline();
  }
}

void EOR(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  check_r15_lookahead(Rn, I, R, op1);

  u32 result = op1 ^ op2;

  if (S) {
    is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
    is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->flush_pipeline();
  }
}
void BIC(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  // op2 = cpu->handle_shifts(instr, instr.S);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  // check_r15_lookahead(instr, op1);

  check_r15_lookahead(Rn, I, R, op1);

  auto result = op1 & (~op2);

  if (S) {
    is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
    is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->flush_pipeline();
  }
}

void check_r15_lookahead(u8 Rn, bool I, bool R, u32& op1) {
  if (Rn == 15 && I == 0 && R && op1 != U32_MAX && op1 != 0) {
    op1 += 4;
    // fmt::println("added 4 in lookahead");
  }
  // if (instr.I == 0 && instr.shift_value_is_register && instr.Rm == 15 && instr.op2 != U32_MAX && instr.op2 != 0) instr.op2 += 4;
}

void RSB(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  // instr.op2 = cpu->handle_shifts(instr, false);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  // check_r15_lookahead(instr, op1);

  check_r15_lookahead(Rn, I, R, op1);

  u32 result = op2 - op1;

  u32 op2_msb = op2 & 0x80000000;
  u32 rn_msb  = op1 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (S) {
    is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
    is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
    ((op2_msb ^ res_msb) & (op2_msb ^ rn_msb)) ? cpu->set_overflow() : cpu->reset_overflow();
    op2 >= cpu->regs.get_reg(Rn) ? cpu->set_carry() : cpu->reset_carry();

    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->flush_pipeline();
  }
}

void SUB(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  // op2 = cpu->handle_shifts(instr, false);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  // check_r15_lookahead(instr, op1);

  check_r15_lookahead(Rn, I, R, op1);

  u32 result = op1 - op2;

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (S) {
    is_negative((op1 - op2)) ? cpu->set_negative() : cpu->reset_negative();
    is_zero((op1 - op2)) ? cpu->set_zero() : cpu->reset_zero();

    op1 >= op2 ? cpu->set_carry() : cpu->reset_carry();
    (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();

    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->regs.get_reg(Rd) = cpu->align_by_current_mode(cpu->regs.get_reg(Rd));
    cpu->flush_pipeline();
  }
}

void SBC(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode >> 16) & 0xF;
  u8 Rs           = (opcode >> 8) & 0xF;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  bool old_carry = cpu->regs.CPSR.CARRY_FLAG;
  u32 op1        = cpu->regs.get_reg(Rn);
  // op2      = cpu->handle_shifts(instr, false);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  // check_r15_lookahead(instr, op1);
  // spdlog::info("Op1 pre-lookahead: {} [Rn = {}]", op1, Rn);

  check_r15_lookahead(Rn, I, R, op1);
  // spdlog::info("Op1 post-lookahead: {} [Rn = {}]", op1, Rn);

  // fmt::println("op1: {:#010x}", op1);
  // fmt::println("op2: {:#010x}", op2);

  u32 result = op1 - op2 + (old_carry - 1);

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (S) {
    is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
    is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
    (op1 >= ((u64)op2 + (1 - old_carry))) ? cpu->set_carry() : cpu->reset_carry();
    ((rn_msb != rm_msb) && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();

    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->regs.get_reg(Rd) = cpu->align_by_current_mode(cpu->regs.get_reg(Rd));
    cpu->flush_pipeline();
  }
}

void MVN(ARM7TDMI* cpu, u32 opcode) {
  // fmt::println("is mvn");
  u8 Rd  = (opcode >> 12) & 0xF;
  bool I = opcode & (1 << 25);
  bool S = (opcode & (1 << 20));
  u8 Rm  = opcode & 0xf;
  // u8 Rn     = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode >> 8) & 0xF;
  bool R          = (opcode >> 4) & 1;
  u32 imm         = opcode & 0xff;
  u8 rotate       = (opcode >> 8) & 0xF;
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;
  // op2 = cpu->handle_shifts(instr, S);
  // fmt::println("shift type: {}", shift_type);
  // fmt::println("I: {}", I);
  // fmt::println("R: {}", R);
  // fmt::println("Rm: {}", Rm);
  // fmt::println("Rs: {}", Rs);
  // fmt::println("Rotate: {}", rotate);
  // fmt::println("Rm: {:#x}", cpu->regs.get_reg(Rm));
  // fmt::println("imm: {:#x}", imm);

  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  if (S) {
    ~op2 == 0 ? cpu->set_zero() : cpu->reset_zero();
    (~op2 & 0x80000000) != 0 ? cpu->set_negative() : cpu->reset_negative();
    if (Rd == 15 && S) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = ~op2;

  if (Rd == 15) {
    cpu->regs.get_reg(Rd) = cpu->align_by_current_mode(cpu->regs.get_reg(Rd));
    cpu->flush_pipeline();
  }
}

void ADC(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  // Carry from barrel shifter should not be used for this instruction.
  bool old_carry = cpu->regs.CPSR.CARRY_FLAG;

  u32 op1 = cpu->regs.get_reg(Rn);
  // op2 = cpu->handle_shifts(instr, false);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  u64 result_64 = ((u64)cpu->regs.get_reg(Rn) + op2 + old_carry);
  u32 result_32 = (cpu->regs.get_reg(Rn) + op2 + old_carry);

  // check_r15_lookahead(instr, op1);
  check_r15_lookahead(Rn, I, R, op1);

  if (S) {
    result_32 == 0 ? cpu->set_zero() : cpu->reset_zero();

    u32 overflow = ~(op1 ^ op2) & (op1 ^ result_32);
    ((overflow >> 31) & 1) ? cpu->set_overflow() : cpu->reset_overflow();

    (result_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();
    is_negative(result_32) ? cpu->set_negative() : cpu->reset_negative();

    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = (op1 + op2 + old_carry);

  if (Rd == 15) {
    cpu->regs.get_reg(Rd) = cpu->align_by_current_mode(cpu->regs.get_reg(Rd));
    cpu->flush_pipeline();
  }
}

void TEQ(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  // op2 = cpu->handle_shifts(instr, S);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  u32 m = cpu->regs.get_reg(Rn) ^ op2;

  u32 rn_msb  = cpu->regs.get_reg(Rn) & 0x80000000;
  u32 rm_msb  = cpu->regs.get_reg(Rm) & 0x80000000;
  u32 res_msb = m & 0x80000000;

  // (rn_msb == rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();

  if (!S) {
    if (rn_msb != 0 && rm_msb != 0 && res_msb == 0) {
      cpu->set_overflow();
    } else if (rn_msb == 0 && rm_msb == 0 && res_msb != 0) {
      cpu->set_overflow();
    } else {
      cpu->reset_overflow();
    }
  }

  is_negative(m) ? cpu->set_negative() : cpu->reset_negative();
  is_zero(m) ? cpu->set_zero() : cpu->reset_zero();

  if (Rd == 15 && S) {
    cpu->regs.load_spsr_to_cpsr();
  }
}

void MOV(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd  = (opcode & 0xf000) >> 12;
  bool I = opcode & (1 << 25);
  bool S = (opcode & (1 << 20));
  u8 Rm  = opcode & 0xf;
  // u8 Rn     = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  // op2 = cpu->handle_shifts(instr, S);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  if (S) {
    is_zero(op2) ? cpu->set_zero() : cpu->reset_zero();
    is_negative(op2) ? cpu->set_negative() : cpu->reset_negative();
    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = op2;

  if (Rd == 15) {
    cpu->regs.get_reg(Rd) = cpu->align_by_current_mode(cpu->regs.get_reg(Rd));
    cpu->flush_pipeline();
  }
}
void ADD(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  if (cpu->regs.CPSR.STATE_BIT == THUMB_MODE) {}

  u32 op1 = cpu->regs.get_reg(Rn);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  check_r15_lookahead(Rn, I, R, op1);

  // if (pc_relative) {
  //   op1 &= ~2;
  // }

  u64 result_64 = (u64)op1 + op2;
  u32 result_32 = op1 + op2;

  if (S) {
    result_32 == 0 ? cpu->set_zero() : cpu->reset_zero();

    u32 overflow = ~(op1 ^ op2) & (op1 ^ result_64);
    ((overflow >> 31) & 1) ? cpu->set_overflow() : cpu->reset_overflow();

    (result_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();
    is_negative(result_32) ? cpu->set_negative() : cpu->reset_negative();
    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result_32;

  if (Rd == 15) {
    cpu->regs.get_reg(15) = cpu->align_by_current_mode(cpu->regs.get_reg(15));
    cpu->flush_pipeline();
  }
}

void TST(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  // op2 = cpu->handle_shifts(instr, S);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  // check_r15_lookahead(instr, op1);
  check_r15_lookahead(Rn, I, R, op1);

  u32 d = op1 & op2;

  is_zero(d) ? cpu->set_zero() : cpu->reset_zero();
  is_negative(d) ? cpu->set_negative() : cpu->reset_negative();

  if (Rd == 15 && S) {
    cpu->regs.load_spsr_to_cpsr();
  }
}

void STR(ARM7TDMI* cpu, u32 opcode) {
  bool I = opcode & (1 << 25);
  bool P = opcode & (1 << 24);
  bool U = opcode & (1 << 23);
  bool B = opcode & (1 << 22);
  bool W = opcode & (1 << 21);
  // bool load = opcode & (1 << 20);

  u8 added_writeback_value = 0;

  u8 Rn      = (opcode & 0xf0000) >> 16;
  u8 Rd      = (opcode & 0xf000) >> 12;
  u32 offset = (opcode & 0xfff);

  u8 Rm = opcode & 0xf;

  u32 shift_amount = (opcode & 0xf80) >> 7;
  u32 shift_type   = (opcode & 0b1100000) >> 5;

  // u32 address = Rn == 15 ? cpu->regs.get_reg(Rn) + 4 : cpu->regs.get_reg(Rn);
  u32 value   = cpu->regs.get_reg(Rd);
  u32 address = cpu->regs.get_reg(Rn);

  if (Rn == 15) added_writeback_value = 4;
  if (Rd == 15) value += 4;

  // cpu->cpu_logger->debug("addr: {:#010x}", address);

  u32 offset_from_reg = cpu->shift((ARM7TDMI::SHIFT_MODE)shift_type, cpu->regs.get_reg(Rm), shift_amount, true, false, false);
  // print_params();
  // cpu->cpu_logger->debug("offset from reg: {:#010x}", offset_from_reg);
  // Rn - base register
  // Rm - offset register (immediate value if I is set)
  // Rd - source/destination register

  if (P == 0) {  // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

    if (B) {                                 // B - Byte/Word bit (0=transfer 32bit/word, 1=transfer 8bit/byte)
      cpu->bus->write8(address, (u8)value);  // I ended up moving the force-alignment code into my Bus class and not doing it for SRAM accesses
    } else {
      // cpu->bus->write32(cpu->align(address, WORD), value);
      cpu->bus->write32(address, value);
    }

    if (U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (I == 0 ? offset : offset_from_reg);
    } else {
      address -= (I == 0 ? offset : offset_from_reg);
    }

  } else {  // Post added offset

    if (U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (I == 0 ? offset : offset_from_reg);
    } else {
      address -= (I == 0 ? offset : offset_from_reg);
    }

    if (B) {  // B - Byte/Word bit (0=transfer 32bit/word, 1=transfer 8bit/byte)
      cpu->bus->write8(address, (u8)value);
    } else {
      cpu->bus->write32(address, value);
    }
  }

  single_data_transfer_writeback_str(cpu, Rn, address, added_writeback_value, P, W);
}

void single_data_transfer_writeback_str(ARM7TDMI* cpu, u8 Rn, u32 address, u8 added_writeback_value, bool P, bool W) {
  // Write-back is optional when P=1, forced when P=0
  // When P=1, check for writeback bit and writeback (if set)
  // Write-backs do not need the address aligned, only writes/loads do.
  // If R15 is the writeback register, add 4 (and align, obviously)
  // REFACTOR: i do believe that handling things like alignment are better handled on the side of the bus, not the instruction
  if (P == 0 || (P == 1 && W)) {
    u32 writeback_addr    = address + added_writeback_value;
    cpu->regs.get_reg(Rn) = writeback_addr;

    // If we wrote to R15, we need to flush, as the pipeline is now invalid.
    if (Rn == 15) {
      cpu->regs.r[15] = align(writeback_addr, cpu->regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
      cpu->flush_pipeline();
    }
  }
};

void single_data_transfer_writeback_ldr(ARM7TDMI* cpu, u8 Rd, u8 Rn, u32 address, u8 added_writeback_value, bool P, bool W) {
  // Write-back is optional when P=1, forced when P=0
  // When P=1, check for writeback bit and writeback (if set)
  // Write-backs do not need the address aligned, only writes/loads do.
  // If R15 is the writeback register, add 4 (and align, obviously)
  // REFACTOR: i do believe that handling things like alignment are better handled on the side of the bus, not the instruction

  if (Rd == 15) {
    cpu->regs.r[15] = align(cpu->regs.r[15], cpu->regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
    cpu->flush_pipeline();
  }

  if (Rd == Rn) return;

  if (P == 0 || (P == 1 && W)) {
    // fmt::println("instr rn: {}", instr.Rn);

    u32 writeback_addr    = address + added_writeback_value;
    cpu->regs.get_reg(Rn) = writeback_addr;

    if (Rn == 15) {
      // fmt::println("R15: {:#010x}", cpu->regs.get_reg(Rn));
      cpu->flush_pipeline();
    }
  }
}

void CMN(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  // op2 = cpu->handle_shifts(instr, false);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  u64 r_64 = (u64)op1 + op2;

  u32 r = op1 + op2;

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = op2 & 0x80000000;
  u32 res_msb = r & 0x80000000;

  is_negative(r) ? cpu->set_negative() : cpu->reset_negative();
  is_zero(r) ? cpu->set_zero() : cpu->reset_zero();
  (r_64 > U32_MAX) ? cpu->set_carry() : cpu->reset_carry();

  if (rn_msb != 0 && rm_msb != 0 && res_msb == 0) {
    cpu->set_overflow();
  } else if (rn_msb == 0 && rm_msb == 0 && res_msb != 0) {
    cpu->set_overflow();
  } else {
    cpu->reset_overflow();
  }

  if (Rd == 15 && S) {
    cpu->regs.load_spsr_to_cpsr();
  }
}

void RSC(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  bool old_carry = cpu->regs.CPSR.CARRY_FLAG;
  u32 op1        = cpu->regs.get_reg(Rn);

  // op2 = cpu->handle_shifts(instr, false);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  // check_r15_lookahead(instr, op1);
  check_r15_lookahead(Rn, I, R, op1);

  u32 result = (op2 - op1) + (old_carry - 1);

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  if (S) {
    result >= 0x80000000 ? cpu->set_negative() : cpu->reset_negative();
    result == 0 ? cpu->set_zero() : cpu->reset_zero();
    op2 >= (op1 + (1 - old_carry)) ? cpu->set_carry() : cpu->reset_carry();
    ((rn_msb != rm_msb) && (rm_msb != res_msb)) ? cpu->set_overflow() : cpu->reset_overflow();

    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->regs.get_reg(Rd) = cpu->align_by_current_mode(cpu->regs.get_reg(Rd));
    cpu->flush_pipeline();
  }
}
void LDR(ARM7TDMI* cpu, u32 opcode, bool pc_relative) {
  bool I                   = opcode & (1 << 25);
  bool P                   = opcode & (1 << 24);
  bool U                   = opcode & (1 << 23);
  bool B                   = opcode & (1 << 22);
  bool W                   = opcode & (1 << 21);
  u8 added_writeback_value = 0;
  u8 Rn                    = (opcode & 0xf0000) >> 16;
  u8 Rd                    = (opcode & 0xf000) >> 12;
  u32 offset               = (opcode & 0xfff);
  u8 Rm                    = opcode & 0xf;

  if (pc_relative) {
    Rd     = (opcode >> 8) & 0b111;
    offset = opcode & 0xff;
    Rm     = 15;
    I      = true;
  }

  u32 shift_amount = (opcode & 0xf80) >> 7;
  u32 shift_type   = (opcode & 0b1100000) >> 5;

  u32 address = cpu->regs.get_reg(Rn);

  u32 offset_from_reg = cpu->shift((ARM7TDMI::SHIFT_MODE)shift_type, cpu->regs.get_reg(Rm), shift_amount, true, false, false);
  if (Rn == 15) added_writeback_value += 4;
  u8 misaligned_by = 0;

  if (P == 0) {  // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

    misaligned_by = (address & 3) * 8;

    if (B) {
      cpu->regs.get_reg(Rd) = cpu->bus->read8(address);
    } else {
      u32 v                 = cpu->bus->read32(address);
      cpu->regs.get_reg(Rd) = std::rotr(v, misaligned_by);
    }

    if (U) {  // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
      address += (I == 0 ? offset : offset_from_reg);
    } else {
      address -= (I == 0 ? offset : offset_from_reg);
    }

  } else {
    if (U) {
      address += (I == 0 ? offset : offset_from_reg);
    } else {
      address -= (I == 0 ? offset : offset_from_reg);
    }

    misaligned_by = (address & 3) * 8;

    if (B) {
      cpu->regs.get_reg(Rd) = cpu->bus->read8(address);
    } else {
      // cpu->regs.get_reg(Rd) = cpu->bus->read32(address);
      u32 v                 = cpu->bus->read32(address);
      cpu->regs.get_reg(Rd) = std::rotr(v, misaligned_by);
    }
  }

  single_data_transfer_writeback_ldr(cpu, Rd, Rn, address, added_writeback_value, P, W);
}

void LDRSB(ARM7TDMI* cpu, u32 opcode) {  // TODO: Re-write this, too long and unnecessarily complex

  bool P = opcode & (1 << 24);
  bool U = opcode & (1 << 23);
  bool I = opcode & (1 << 22);
  bool W = opcode & (1 << 21);
  u8 Rn  = (opcode >> 16) & 0xF;
  u8 Rd  = (opcode >> 12) & 0xF;
  u8 imm = (((opcode >> 8) & 0xF) << 4) | (opcode & 0xF);
  u8 Rm  = opcode & 0xF;

  u32 address              = cpu->regs.get_reg(Rn);
  u8 immediate_offset      = imm;
  u32 register_offset      = cpu->regs.get_reg(Rm);
  u8 added_writeback_value = 0;
  if (Rn == 15) added_writeback_value += 4;

  // On ARM7 aka ARMv4 aka NDS7/GBA:

  // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value

  // u8 misaligned_load_rotate_value = 0;

  if (P == 0) {  // P == 0
    u32 value = cpu->bus->read8(address);
    if (value & (1 << 7)) value |= 0xffffff00;
    cpu->regs.get_reg(Rd) = value;

    if (U) {
      address += (I ? immediate_offset : register_offset);
    } else {
      address -= (I ? immediate_offset : register_offset);
    }

  } else {  // P == 1

    if (U) {
      address += (I ? immediate_offset : register_offset);
    } else {
      address -= (I ? immediate_offset : register_offset);
    }

    u32 value = cpu->bus->read8(address);
    if (value & (1 << 7)) value |= 0xffffff00;
    cpu->regs.get_reg(Rd) = value;
  }
  // InstructionInfo _instr = instr;
  single_data_transfer_writeback_ldr(cpu, Rd, Rn, address, added_writeback_value, P, W);
}

void LDRSH(ARM7TDMI* cpu, u32 opcode) {
  bool P = opcode & (1 << 24);
  bool U = opcode & (1 << 23);
  bool I = opcode & (1 << 22);
  bool W = opcode & (1 << 21);
  u8 Rn  = (opcode >> 16) & 0xF;
  u8 Rd  = (opcode >> 12) & 0xF;
  u8 imm = (u8)(((opcode >> 8) & 0xF) << 4) | (opcode & 0xF);
  u8 Rm  = opcode & 0xF;

  u32 address              = cpu->regs.get_reg(Rn);
  u8 immediate_offset      = imm;
  u32 register_offset      = cpu->regs.get_reg(Rm);
  u8 added_writeback_value = 0;
  if (Rn == 15) added_writeback_value += 4;

  // print_params();

  // fmt::println("addr: {:#010x}", address);

  // u32 offset = I == 1 ? immediate_offset : register_offset;
  // u32 register_offset = cpu->shift((ARM7TDMI::SHIFT_MODE)shift_type, cpu->regs.get_reg(Rm), shift_amount, true, false, false);
  // u8 rotate_by        = 0;

  // On ARM7 aka ARMv4 aka NDS7/GBA:

  // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
  // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value

  // u8 misaligned_load_rotate_value = 0;

  if (P == 0) {                // P == 0
    if ((address & 1) != 0) {  // unaligned
      u32 value = cpu->bus->read8(address);
      if (value & (1 << 7)) value |= 0xffffff00;
      cpu->regs.get_reg(Rd) = value;
    } else {
      u32 value = cpu->bus->read16(address);
      if (value & (1 << 15)) value |= 0xFFFF0000;
      cpu->regs.get_reg(Rd) = value;
    }

    // cpu->regs.get_reg(Rd) = std::rotr(cpu->regs.get_reg(Rd), misaligned_load_rotate_value);
    // fmt::println("offset {:#010X}", immediate_offset);
    // fmt::println("b4 {:#010X}", address);
    if (U) {
      address += (I ? immediate_offset : register_offset);
    } else {
      address -= (I ? immediate_offset : register_offset);
    }
    // fmt::println("after {:#010X}", address);

  } else {  // P == 1

    if (U) {
      address += (I ? immediate_offset : register_offset);
    } else {
      address -= (I ? immediate_offset : register_offset);
    }

    if ((address & 1) != 0) {  // info: this does not happen on the DS (ARM946E-S) so change if this is ever used as a core for that device
      u32 value = cpu->bus->read8(address);
      if (value & (1 << 7)) value |= 0xffffff00;
      cpu->regs.get_reg(Rd) = value;
    } else {
      u32 value = cpu->bus->read16(address);
      if (value & (1 << 15)) value |= 0xFFFF0000;
      cpu->regs.get_reg(Rd) = value;
    }
  }

  single_data_transfer_writeback_ldr(cpu, Rd, Rn, address, added_writeback_value, P, W);
}

void LDRH(ARM7TDMI* cpu, u32 opcode) {  // TODO: implement writeback logic

  bool P = opcode & (1 << 24);
  bool U = opcode & (1 << 23);
  bool I = opcode & (1 << 22);
  bool W = opcode & (1 << 21);
  u8 Rn  = (opcode >> 16) & 0xF;
  u8 Rd  = (opcode >> 12) & 0xF;
  u8 imm = (u8)((((opcode >> 8) & 0xF) << 4) | (opcode & 0xF));
  u8 Rm  = opcode & 0xF;

  u32 address              = cpu->regs.get_reg(Rn);
  u8 immediate_offset      = imm;
  u32 register_offset      = cpu->regs.get_reg(Rm);
  u8 added_writeback_value = 0;
  if (Rn == 15) added_writeback_value += 4;

  // print_params();

  // fmt::println("addr: {:#010x}", address);

  // u32 offset = I == 1 ? immediate_offset : register_offset;
  // u32 register_offset = cpu->shift((ARM7TDMI::SHIFT_MODE)shift_type, cpu->regs.get_reg(Rm), shift_amount, true, false, false);
  // u8 rotate_by        = 0;

  // On ARM7 aka ARMv4 aka NDS7/GBA:

  // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
  // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value

  u8 misaligned_load_rotate_value = 0;
  (void)misaligned_load_rotate_value;

  if (P == 0) {  // P == 0

    if ((address & 1) != 0) misaligned_load_rotate_value = 8;
    // fmt::println("aligned?: {}", (address & 1) == 0);

    u32 value = cpu->bus->read16(address);
    // fmt::println("value read: {:#010x}", value);
    value                 = std::rotr(value, misaligned_load_rotate_value);
    cpu->regs.get_reg(Rd) = value;

    // fmt::println("value after shift: {:#010x}", cpu->regs.get_reg(Rd));

    if (U) {
      address += (I ? immediate_offset : register_offset);
    } else {
      address -= (I ? immediate_offset : register_offset);
    }

  } else {  // P == 1

    if (U) {
      address += (I ? immediate_offset : register_offset);
    } else {
      address -= (I ? immediate_offset : register_offset);
    }

    if ((address & 1) != 0) misaligned_load_rotate_value = 8;
    // fmt::println("aligned?: {}", (address & 1) == 0);

    u32 value = cpu->bus->read16(address);
    // fmt::println("value read: {:#010x}", value);
    value                 = std::rotr(value, misaligned_load_rotate_value);
    cpu->regs.get_reg(Rd) = value;

    // fmt::println("value after shift: {:#010x}", cpu->regs.get_reg(Rd));
  }

  single_data_transfer_writeback_ldr(cpu, Rd, Rn, address, added_writeback_value, P, W);
}

void CMP(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  // op2 = cpu->handle_shifts(instr, false);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, false);

  u32 result = op1 - op2;

  u32 rn_msb  = op1 & 0x80000000;
  u32 rm_msb  = op2 & 0x80000000;
  u32 res_msb = result & 0x80000000;

  is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
  is_negative(result) ? cpu->set_negative() : cpu->reset_negative();
  (rn_msb != rm_msb && rn_msb != res_msb) ? cpu->set_overflow() : cpu->reset_overflow();
  (op1 >= op2) ? cpu->set_carry() : cpu->reset_carry();

  if (Rd == 15 && S) {
    cpu->regs.load_spsr_to_cpsr();
  }
}

void ORR(ARM7TDMI* cpu, u32 opcode) {
  u8 Rd           = (opcode & 0xf000) >> 12;
  bool I          = opcode & (1 << 25);
  bool S          = (opcode & (1 << 20));
  u8 Rm           = opcode & 0xf;
  u8 Rn           = (opcode & 0xf0000) >> 16;
  u8 Rs           = (opcode & 0xf00) >> 8;
  bool R          = (opcode & (1 << 4));
  u8 imm          = opcode & 0xff;
  u8 rotate       = ((opcode & 0xf00) >> 8);
  u8 shift_type   = (opcode & 0x60) >> 5;
  u8 shift_amount = (opcode & 0xf80) >> 7;

  u32 op1 = cpu->regs.get_reg(Rn);
  // op2 = cpu->handle_shifts(instr, S);
  u32 op2 = cpu->handle_shifts(I, R, Rm, Rs, shift_type, imm, rotate, shift_amount, S, S);

  // check_r15_lookahead(instr, op1);
  check_r15_lookahead(Rn, I, R, op1);

  u32 result = op1 | op2;

  if (S) {
    is_zero(result) ? cpu->set_zero() : cpu->reset_zero();
    is_negative(result) ? cpu->set_negative() : cpu->reset_negative();

    if (Rd == 15) {
      cpu->regs.load_spsr_to_cpsr();
    }
  }

  cpu->regs.get_reg(Rd) = result;

  if (Rd == 15) {
    cpu->flush_pipeline();
  }
}

void OP_MSR(ARM7TDMI* cpu, u32 opcode) {
  // TODO: refactor, this is some 4AM code

  u32 immediate_amount        = opcode & 0xff;                   // used for transfers using immediate values
  u8 shift_amount             = (opcode & 0b111100000000) >> 8;  // immediate is rotated by this amount multiplied by two.
  bool I                      = (opcode & (1 << 25));
  bool modify_flag_field      = (opcode & (1 << 19)) ? true : false;
  bool modify_status_field    = (opcode & (1 << 18)) ? true : false;
  bool modify_extension_field = (opcode & (1 << 17)) ? true : false;
  bool modify_control_field   = (opcode & (1 << 16)) ? true : false;
  u8 Rm                       = opcode & 0xf;
  u32 op_value                = 0;
  PSR P                       = opcode & (1 << 22) ? SPSR_MODE : CPSR;

  if (I == 0) {  // 0 = Register, 1 = Immediate
    op_value = cpu->regs.get_reg(Rm);
  } else {
    op_value = std::rotr(immediate_amount, shift_amount * 2);
  }

  switch (P) {  // CPSR = 0, SPSR = 1
    case PSR::CPSR: {
      if (modify_flag_field) {
        cpu->regs.CPSR.value &= ~0xFF000000;
        cpu->regs.CPSR.value |= (op_value & 0xFF000000);

        // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
      }
      if (modify_status_field && cpu->regs.CPSR.MODE_BIT != USER) {
        cpu->regs.CPSR.value &= ~0x00FF0000;
        cpu->regs.CPSR.value |= (op_value & 0x00FF0000);
      }
      if (modify_extension_field && cpu->regs.CPSR.MODE_BIT != USER) {
        cpu->regs.CPSR.value &= ~0x0000FF00;
        cpu->regs.CPSR.value |= (op_value & 0x0000FF00);
      }

      if (modify_control_field && cpu->regs.CPSR.MODE_BIT != USER) {
        op_value |= 0x10;

        // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
        auto old_mode = cpu->regs.CPSR.STATE_BIT;

        cpu->regs.CPSR.value &= ~0x000000FF;
        cpu->regs.CPSR.value |= (op_value & 0x000000FF);
        cpu->regs.CPSR.value |= 0x10;

        if (old_mode != cpu->regs.CPSR.STATE_BIT) {
          cpu->flush_pipeline();
        }
      }
      break;
    }

    case PSR::SPSR_MODE: {  // TODO: put this in handler functions, having this per case fully written out looks like dogshit
      // fmt::println("------ SPSR");
      switch (cpu->regs.CPSR.MODE_BIT) {
        case USER:
        case SYSTEM: {
          // assert(0);
          break;
        }
        case FIQ: {
          if (modify_flag_field) {
            cpu->regs.SPSR_fiq &= ~0xFF000000;
            cpu->regs.SPSR_fiq |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_fiq &= ~0x00FF0000;
            cpu->regs.SPSR_fiq |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_fiq &= ~0x0000FF00;
            cpu->regs.SPSR_fiq |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && cpu->regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = cpu->regs.CPSR.STATE_BIT;

            cpu->regs.SPSR_fiq &= ~0x000000FF;
            cpu->regs.SPSR_fiq |= (op_value & 0x000000FF);
            // cpu->regs.SPSR_fiq |= 0x10;

            // if (old_mode != cpu->regs.CPSR.STATE_BIT) { cpu->flush_pipeline(); }
          }

          break;
        }
        case IRQ: {
          // fmt::println("new value: {:#010x}", op_value);
          if (modify_flag_field) {
            cpu->regs.SPSR_irq &= ~0xFF000000;
            cpu->regs.SPSR_irq |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_irq &= ~0x00FF0000;
            cpu->regs.SPSR_irq |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_irq &= ~0x0000FF00;
            cpu->regs.SPSR_irq |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && cpu->regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("op value & 0xff: {:#010x}", op_value & 0xff);
            cpu->regs.SPSR_irq &= ~0x000000FF;
            cpu->regs.SPSR_irq |= (op_value & 0x000000FF);

            // cpu->regs.SPSR_irq |= 0x10;
          }
          break;
        }
        case SUPERVISOR: {
          if (modify_flag_field) {
            cpu->regs.SPSR_svc &= ~0xFF000000;
            cpu->regs.SPSR_svc |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_svc &= ~0x00FF0000;
            cpu->regs.SPSR_svc |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_svc &= ~0x0000FF00;
            cpu->regs.SPSR_svc |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && cpu->regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = cpu->regs.CPSR.STATE_BIT;

            cpu->regs.SPSR_svc &= ~0x000000FF;
            cpu->regs.SPSR_svc |= (op_value & 0x000000FF);
            // cpu->regs.SPSR_svc |= 0x10;

            // if (old_mode != cpu->regs.CPSR.STATE_BIT) { cpu->flush_pipeline(); }
          }
          break;
        }
        case ABORT: {
          // fmt::println("wrote to ABT");
          if (modify_flag_field) {
            cpu->regs.SPSR_abt &= ~0xFF000000;
            cpu->regs.SPSR_abt |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_abt &= ~0x00FF0000;
            cpu->regs.SPSR_abt |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_abt &= ~0x0000FF00;
            cpu->regs.SPSR_abt |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && cpu->regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = cpu->regs.CPSR.STATE_BIT;

            cpu->regs.SPSR_abt &= ~0x000000FF;
            cpu->regs.SPSR_abt |= (op_value & 0x000000FF);
            // cpu->regs.SPSR_abt |= 0x10;

            // if (old_mode != cpu->regs.CPSR.STATE_BIT) { cpu->flush_pipeline(); }
          }
          break;
        }
        case UNDEFINED: {
          if (modify_flag_field) {
            cpu->regs.SPSR_und &= ~0xFF000000;
            cpu->regs.SPSR_und |= (op_value & 0xFF000000);

            // fmt::println("[CPSR] new [f]lag field: {:#x}", op_value & 0xF0000000);
          }
          if (modify_status_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_und &= ~0x00FF0000;
            cpu->regs.SPSR_und |= (op_value & 0x00FF0000);
          }
          if (modify_extension_field && cpu->regs.CPSR.MODE_BIT != USER) {
            cpu->regs.SPSR_und &= ~0x0000FF00;
            cpu->regs.SPSR_und |= (op_value & 0x0000FF00);
          }

          if (modify_control_field && cpu->regs.CPSR.MODE_BIT != USER) {
            // op_value |= 0x10;

            // fmt::println("[CPSR] new [c]ontrol field: {:#x}", op_value & 0x000000FF);
            // auto old_mode = cpu->regs.CPSR.STATE_BIT;

            cpu->regs.SPSR_und &= ~0x000000FF;
            cpu->regs.SPSR_und |= (op_value & 0x000000FF);
            // cpu->regs.SPSR_und |= 0x10;

            // if (old_mode != cpu->regs.CPSR.STATE_BIT) { cpu->flush_pipeline(); }
          }
          break;
        }
      }
      break;
    };

    default: {
      // cpu->cpu_logger->critical("INVALID RM PASSED: {}", Rm);
      assert(0);
    }
  }
}

void STM(ARM7TDMI* cpu, u32 opcode, bool lr_bit, bool thumb_load_store) {  // REFACTOR: this could written much shorter

  bool P = opcode & (1 << 24);
  bool U = opcode & (1 << 23);
  bool S = opcode & (1 << 22);
  bool W = opcode & (1 << 21);
  u8 Rn;
  u32 base_address;

  u16 Rlist_value = 0;
  u32 value       = 0;

  if (cpu->regs.CPSR.STATE_BIT == THUMB_MODE) {
    Rlist_value = opcode & 0xFF;
    W           = true;
    S           = false;

    if (thumb_load_store) {  // THUMB.15
      Rn = (opcode >> 8) & 0b111;
      U  = true;
      P  = false;
    } else {  // THUMB.14 (PUSH/POP)
      Rn = 13;
      P  = true;
      U  = false;
    }

  } else {
    Rlist_value = opcode & 0xFFFF;
    Rn          = (opcode & 0xf0000) >> 16;
  }

  base_address = cpu->regs.get_reg(Rn);  // Base
  std::vector<u8> Rlist_vec;

  bool forced_writeback_empty_rlist = false;
  bool base_first_in_reg_list       = false;
  bool base_in_reg_list             = false;

  (void)(base_first_in_reg_list);
  (void)(forced_writeback_empty_rlist);

  for (u8 idx = 0; idx < 16; idx++) {
    if (Rlist_value & (1 << idx)) {
      // quirk: if the base register is the first register in the reglist, we writeback the old value of the base.
      if (idx == Rn && Rlist_vec.empty()) {
        base_first_in_reg_list = true;
      }
      if (idx == Rn && !Rlist_vec.empty()) {
        base_in_reg_list = true;
      }
      Rlist_vec.push_back(idx);
    }
  }

  if (lr_bit) {
    Rlist_vec.push_back(14);
  }

  if (Rlist_vec.empty()) {  // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+/-40h (ARMv4-v5)
    Rlist_vec.push_back(15);
    forced_writeback_empty_rlist = true;
    W                            = 0;
  }

  // Handle forced writeback quirk
  if (forced_writeback_empty_rlist) {
    // fmt::println("{} - {}", mnemonic, cpu->bus->cycles_elapsed);

    // fmt::println("handling forced writeback P:{} U:{}", P, U);

    if (U == 0) base_address -= 0x40;  // changed because of PUSH POP breaking

    if ((P == 0 && U == 0) || (P && U)) base_address += 0x4;

    u8 mode_dependent_offset = cpu->regs.CPSR.STATE_BIT == THUMB_MODE ? 2 : 4;

    u32 value = cpu->regs.r[15] + mode_dependent_offset;

    cpu->bus->write32(base_address, value);

    if ((P == 0 && U == 0) || (P && U)) base_address -= 0x4;

    if (P && U) base_address += 0x40;

    if (!P && U) base_address += 0x40;

    if (S) {
      cpu->regs.r[Rn] = base_address;
    } else {
      cpu->regs.get_reg(Rn) = base_address;
    }

    fmt::println("done?");
    return;
  }

  // Check mode, and execute transfer
  if (P == 1 && U == 0) {  // Pre-decrement (DB)
    base_address -= (Rlist_vec.size() * 4);

    // fmt::println("STMDB base address: {:#010x}", base_address);

    if (base_in_reg_list && W) {
      // fmt::println("pre transfer: base is in reglist, setting r{} to {:#010x}", Rn, base_address);

      if (S) {
        cpu->regs.r[Rn] = base_address;
      } else {
        cpu->regs.get_reg(Rn) = base_address;
      }
    }

    // fmt::println("r{} = {:#010x}", Rn, cpu->regs.get_reg(Rn));

    for (const auto& r_num : Rlist_vec) {
      // if (Rn == r_num) { fmt::println("Rn{} IN STMDB: {:#010x}", +Rn, cpu->regs.get_reg(Rn)); }

      if (S) {
        value = cpu->regs.r[r_num];
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      } else {
        value = cpu->regs.get_reg(r_num);
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      }

      base_address += 4;
    }

    if (W && !base_in_reg_list && U == 0) {
      if (S) {
        cpu->regs.r[Rn] = (u32)(base_address - (Rlist_vec.size() * 4));
      } else {
        cpu->regs.get_reg(Rn) = (u32)(base_address - (Rlist_vec.size() * 4));
      }
    }

    if (W && !base_in_reg_list && U == 1) {
      if (S) {
        cpu->regs.r[Rn] = base_address;
      } else {
        cpu->regs.get_reg(Rn) = base_address;
      }
    }

    // Handle forced writeback
    if (forced_writeback_empty_rlist) {}

    if (Rn == 15 && W) {
      cpu->flush_pipeline();
    }
    return;
  }

  if (P == 1 && U == 1) {  // Pre-increment (IB)

    // Writeback base immediately -- do not defer (quirk: needs reference)
    if (base_in_reg_list && W) {
      if (S) {
        cpu->regs.r[Rn] = base_address + (Rlist_vec.size() * 4);
      } else {
        cpu->regs.get_reg(Rn) = base_address + (Rlist_vec.size() * 4);
      }
    }

    for (const auto& r_num : Rlist_vec) {  // REFACTOR: we are rechecking conditional multiple times
      // fmt::println("about to write the value of: r{}", r_num);
      base_address += 4;
      if (S) {  // Force write into user bank modes
        value = cpu->regs.r[r_num];
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      } else {  // write to whatever mode we're currently in
        value = cpu->regs.get_reg(r_num);
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      }
    }

    // sanity check
    // fmt::println("0 sanity check R{} post: {:#010x}", instr.Rn, cpu->regs.get_reg(instr.Rn));

    // if (instr.W && !base_in_reg_list) {
    //   fmt::println("prong");
    //   cpu->regs.get_reg(instr.Rn) = base_address;
    // }

    if (W && !base_in_reg_list) {
      if (S) {
        cpu->regs.r[Rn] = base_address;
      } else {
        cpu->regs.get_reg(Rn) = base_address;
      }
    }

    if (W && Rn == 15) {
      cpu->flush_pipeline();
    }
    // sanity check
    // fmt::println("1 sanity check R{} post: {:#010x}", instr.Rn, cpu->regs.get_reg(instr.Rn));

    return;
  }

  if (P == 0 && U == 0) {  // Post-decrement (DA)
    // fmt::println("Rn {}: {:#010x}", Rn, cpu->regs.get_reg(Rn));

    // if (base_in_reg_list && W) {
    //   fmt::println("s0 Rn {}: {:#010x}", Rn, cpu->regs.get_reg(Rn));
    //   cpu->regs.get_reg(Rn) = base_address - (Rlist_vec.size() * 4);
    //   fmt::println("s0 Rn {}: {:#010x}", Rn, cpu->regs.get_reg(Rn));
    // }

    if (base_in_reg_list && W) {
      if (S) {
        cpu->regs.r[Rn] = base_address - (Rlist_vec.size() * 4);
      } else {
        cpu->regs.get_reg(Rn) = base_address - (Rlist_vec.size() * 4);
      }
    }

    base_address -= (Rlist_vec.size() * 4);
    u32 value = 0;

    for (const auto& r_num : Rlist_vec) {  // REFACTOR: we are rechecking conditional multiple times
      // fmt::println("about to write the value of: r{}", r_num);
      base_address += 4;
      if (S) {  // Write using values from user bank modes
        value = cpu->regs.r[r_num];
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      } else {  // write using values of whatever mode we're currently in
        value = cpu->regs.get_reg(r_num);
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      }
    }

    if (W && !base_in_reg_list) {
      if (S) {
        cpu->regs.r[Rn] = base_address - (Rlist_vec.size() * 4);
      } else {
        cpu->regs.get_reg(Rn) = base_address - (Rlist_vec.size() * 4);
      }
    }

    if (W && Rn == 15) {
      cpu->flush_pipeline();
    }
    // if (W && !base_in_reg_list) { cpu->regs.get_reg(Rn) = base_address; }

    fmt::println("s2 Rn {}: {:#010x}", Rn, cpu->regs.get_reg(Rn));

    return;
  }

  if (P == 0 && U == 1) {  // Post-increment (IA)
    if (base_in_reg_list && W) {
      if (S) {
        cpu->regs.r[Rn] = base_address + (Rlist_vec.size() * 4);
      } else {
        cpu->regs.get_reg(Rn) = base_address + (Rlist_vec.size() * 4);
      }
    }

    u32 value = 0;

    for (const auto& r_num : Rlist_vec) {  // REFACTOR: we are rechecking conditional multiple times
      // fmt::println("about to write the value of: r{}", r_num);
      if (S) {  // Force write into user bank modes
        value = cpu->regs.r[r_num];
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      } else {  // write to whatever mode we're currently in
        value = cpu->regs.get_reg(r_num);
        if (r_num == 15) {
          value = cpu->regs.r[15] + 4;
        }
        if (base_in_reg_list && Rn == 15 && r_num == 15 && W) value -= 4;
        cpu->bus->write32(base_address, value);
      }
      base_address += 4;
    }

    if (W && !base_in_reg_list) {
      if (S) {
        cpu->regs.r[Rn] = base_address;
      } else {
        cpu->regs.get_reg(Rn) = base_address;
      }
    }

    if (Rn == 15 && W) {
      cpu->flush_pipeline();
    }

    return;
  }

  cpu->cpu_logger->error("invalid STM mode");
  assert(0);
  return;
}

void LDM(ARM7TDMI* cpu, u32 opcode, bool lr_bit, bool thumb_load_store) {
  bool P = opcode & (1 << 24);
  bool U = opcode & (1 << 23);
  bool S = opcode & (1 << 22);
  bool W = opcode & (1 << 21);
  u8 Rn  = (opcode & 0xf0000) >> 16;

  u16 Rlist_value = opcode & 0xFFFF;
  u8 pass         = 0;

  if (cpu->regs.CPSR.STATE_BIT == THUMB_MODE) {
    Rlist_value = opcode & 0xFF;
    W           = true;
    S           = false;

    if (thumb_load_store) {  // THUMB.15
      // fmt::println("thumb load & store");
      Rn = (opcode >> 8) & 0b111;
      P  = false;
      U  = true;
    } else {  // THUMB.14 (PUSH/POP)
      // fmt::println("pop");
      Rn = 13;
      P  = false;
      U  = true;
    }
  }

  u32 address           = cpu->regs.get_reg(Rn);  // Base
  u32 operating_address = address;

  // operating_address = align(operating_address, WORD);

  std::vector<u8> Rlist_vec;

  bool r15_in_rlist              = false;
  bool forced_writeback          = false;
  bool base_in_reg_list          = false;
  bool pipeline_has_been_flushed = false;

  // Load Rlist values into vector, check for any conditions used at transfer.
  for (u8 idx = 0; idx < 16; idx++) {
    if (Rlist_value & (1 << idx)) {
      if (idx == Rn) {
        base_in_reg_list = true;
      }
      if (idx == 15) {
        r15_in_rlist = true;
      }
      Rlist_vec.push_back(idx);
    }
  }

  // In THUMB mode, the PC/LR-bit can be set, which means that the PC is pushed into the register list.
  if (lr_bit) {
    Rlist_vec.push_back(15);
    r15_in_rlist = true;
  }

  // If the Rlist is empty, there are some hardware quirks, set flags to be used at transfer.
  if (Rlist_vec.empty()) {
    // fmt::println("Rlist empty");
    Rlist_vec.push_back(15);
    r15_in_rlist     = true;
    forced_writeback = true;
    W                = 0;
  }

  bool use_userbanks = S && !r15_in_rlist;

  (void)base_in_reg_list;
  (void)forced_writeback;
  // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

  operating_address -= Rlist_vec.size() * 4;

  if (U == 1) operating_address += Rlist_vec.size() * 4;
  if (P && U) operating_address += 4;  // LDMIB
  if (P == 0 && U == 0) operating_address += 4;
  // if (P == 0 && U) return;  // LDMIA

  // Transfer
  for (const auto& r_num : Rlist_vec) {
    if (use_userbanks) {
      // fmt::println("using userbanks....");
      cpu->regs.r[r_num] = cpu->bus->read32(operating_address + (pass * 4));
    } else {
      cpu->regs.get_reg(r_num) = cpu->bus->read32(operating_address + (pass * 4));
    }

    pass++;
  }

  // fmt::println("base in reg list: {}", base_in_reg_list);

  if (W && !base_in_reg_list) {  // This is the deferred writeback, only executes if base register is not in register list.
    if (U == 0) {
      if (use_userbanks) {
        cpu->regs.r[Rn] = address - (Rlist_vec.size() * 4);
      } else {
        cpu->regs.get_reg(Rn) = address - (Rlist_vec.size() * 4);
      }
    }

    if (U == 1) {
      if (use_userbanks) {
        cpu->regs.r[Rn] = address + (Rlist_vec.size() * 4);
      } else {
        cpu->regs.get_reg(Rn) = address + (Rlist_vec.size() * 4);
      }
    }

    // if (U == 1) { cpu->regs.get_reg(Rn) = address + (Rlist_vec.size() * 4); }
  }

  if (forced_writeback) {
    if (use_userbanks) {
      cpu->regs.r[Rn] += 0x40;
    } else {
      cpu->regs.get_reg(Rn) += 0x40;
    }
  }

  if ((W && Rn == 15) || forced_writeback) {
    pipeline_has_been_flushed = true;
    cpu->regs.r[15]           = cpu->align_by_current_mode(cpu->regs.r[15]);
    cpu->flush_pipeline();
    // fmt::println("new PC: {:#010X}", cpu->regs.r[15]);
  }

  if (r15_in_rlist) {
    // fmt::println("hi");
    // print_params();
    if (S) {
      // fmt::println("loading SPSR!");
      cpu->regs.load_spsr_to_cpsr();
    }

    if (!pipeline_has_been_flushed) {
      // fmt::println("pipeline not flushed, flushing.");
      cpu->regs.r[15] = cpu->align_by_current_mode(cpu->regs.r[15]);
      cpu->flush_pipeline();
    }
  }
  return;

  SPDLOG_CRITICAL("invalid LDM mode");
  assert(0);
}

void BLL(ARM7TDMI* cpu, u32 offset) {
  offset <<= 12;

  if (offset >= 0x400000) {
    offset -= 0x800000;
  }
  cpu->regs.get_reg(14) = cpu->regs.r[15] + offset;
}

void BLH(ARM7TDMI* cpu, u32 offset) {
  u32 pc = cpu->regs.get_reg(15);
  u32 lr = cpu->regs.get_reg(14);

  cpu->regs.get_reg(14) = (pc - 2) | 1;
  cpu->regs.get_reg(15) = lr + (offset << 1);

  cpu->flush_pipeline();
}

#include "core/cpu.hpp"

#include <cstdlib>

#include "instructions/arm.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "shifter.cpp"
#include "spdlog/spdlog.h"
#include "utils/bitwise.hpp"
#define UNDEFINED_MASK 0x6000010
#define BX_MASK 0xffffff0
#define OPCODE_MASK 0x1e00000
enum CPU_MODE { ARM_MODE, THUMB_MODE };

ARM7TDMI::ARM7TDMI() {
  regs.r[15] = 0x08000000;
  spdlog::debug("cpu initialized");
}
void ARM7TDMI::flush_pipeline() {
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    SPDLOG_DEBUG("[ARM] flushing");
    // InstructionInfo f;
    InstructionInfo d;
    InstructionInfo e;

    pipeline = {};
    e.opcode = bus->read32(regs.r[15]);
    e.loc    = regs.r[15];
    d.opcode = bus->read32(regs.r[15] + 4);
    d.loc    = regs.r[15] + 4;

    pipeline.fetch  = d;
    pipeline.decode = decode(e);

    regs.r[15] += 4;
    SPDLOG_DEBUG("[ARM] flushed");
  } else {
    SPDLOG_DEBUG("[THUMB] flushing");
    InstructionInfo d;
    InstructionInfo e;

    pipeline = {};
    e.opcode = bus->read16(regs.r[15]);
    e.loc    = regs.r[15];
    d.opcode = bus->read16(regs.r[15] + 2);
    d.loc    = regs.r[15] + 2;

    pipeline.fetch  = d;
    pipeline.decode = decode(e);

    regs.r[15] += 2;
    SPDLOG_DEBUG("[THUMB] flushed");
  }
  return;
}

u32 ARM7TDMI::align_address(u32 address, BOUNDARY b) {
  if (b == WORD) {
    return address & ~3;
  } else {
    return address & ~1;
  }
}

u32 ARM7TDMI::handle_shifts(InstructionInfo& instr) {
  if (instr.I == 0) {
    // instr.print_params();
    // fmt::println("SHIFT set");
    // fmt::println("op2 is register: {}",instr.op2_is_register);
    if (instr.shift_value_is_register) {
      // if(instr.Rm == 15) assert(0);
      SPDLOG_DEBUG("register shift: r{}", regs.r[instr.Rs] & 0xff);
      u8 add_amount = 0;
      if (instr.Rs == instr.Rm) { add_amount += 4; }
      if (instr.Rm == 15) { add_amount += 4; }

      return shift(
          (ARM7TDMI::SHIFT_MODE)instr.shift_type,
          regs.r[instr.Rm] + add_amount,  // PC reads as PC+12 (PC already reads
                                          // +8) when used as shift register
          regs.r[instr.Rs] & 0xff, true);
    } else {
      SPDLOG_DEBUG("immediate shift: {}", instr.shift_amount);
      return shift((ARM7TDMI::SHIFT_MODE)instr.shift_type, regs.r[instr.Rm],
                   instr.shift_amount, false);
    }
  }

  SPDLOG_DEBUG("immediate value rotate");
  return shift(ROR, instr.imm, instr.rotate * 2, false, true);
}

InstructionInfo ARM7TDMI::decode(InstructionInfo& instr) {
  if (regs.CPSR.STATE_BIT == ARM_MODE) {
    instr.condition = (Condition)(instr.opcode >> 28);

    // MUL, MLA (Multiply)
    if ((instr.opcode & 0xfc000f0) == 0x90) {
      spdlog::debug("read instruction {:#010X} is a multiply", instr.opcode);

      instr.S = (instr.opcode & 1 << 20) ? 1 : 0;

      instr.Rd = (instr.opcode & 0xf0000) >> 16;
      instr.Rn = (instr.opcode & 0xf000) >> 12;
      instr.Rs = (instr.opcode & 0xf00) >> 8;
      instr.Rm = (instr.opcode & 0xf);

      u8 op = (instr.opcode & 0x1e00000) >> 21;

      switch (op) {
        case 0: {  // MUL
          instr.mnemonic = fmt::format("mul r{}, r{}, r{}, r{}", +instr.Rd,
                                       +instr.Rm, +instr.Rs, +instr.Rn);
          instr.func_ptr = ARM::Instructions::MUL;
          break;
        }
        case 1: {  // MLA
          instr.mnemonic = fmt::format("mla r{}, r{}, r{}, r{}", +instr.Rd,
                                       +instr.Rm, +instr.Rs, +instr.Rn);
          instr.func_ptr = ARM::Instructions::MLA;
          break;
        }

        default: {
          assert(0);
        }
      }
      return instr;
    }

    // MULL, MLAL (Multiply Long)
    if ((instr.opcode & 0xf8000f0) == 0x800090) {
      spdlog::debug("read instruction {:#010X} is a multiply long",
                    instr.opcode);
      instr.S = (instr.opcode & 1 << 20) ? 1 : 0;

      instr.Rd = (instr.opcode & 0xf0000) >> 16;
      instr.Rn = (instr.opcode & 0xf000) >> 12;
      instr.Rs = (instr.opcode & 0xf00) >> 8;
      instr.Rm = (instr.opcode & 0xf);
      u8 op    = (instr.opcode & 0x1e00000) >> 21;

      switch (op) {
        case 4: {  // UMULL
          instr.mnemonic = fmt::format(
              "umull{} r{}, r{}, r{}, r{}", condition_map.at(instr.condition),
              +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
          instr.func_ptr = ARM::Instructions::UMULL;

          break;
        }
        case 5: {  // UMLAL
          instr.mnemonic = fmt::format(
              "umlal{} r{} r{} r{} r{}", condition_map.at(instr.condition),
              +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
          instr.func_ptr = ARM::Instructions::UMLAL;
          break;
        }
        case 6: {  // SMULL
          instr.mnemonic = fmt::format(
              "smull{} r{}, r{}, r{}, r{}", condition_map.at(instr.condition),
              +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
          instr.func_ptr = ARM::Instructions::SMULL;
          break;
        }
        case 7: {  // SMLAL
          instr.mnemonic = fmt::format(
              "smlal{} r{}, r{}, r{}, r{}", condition_map.at(instr.condition),
              +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
          instr.func_ptr = ARM::Instructions::SMLAL;
          break;
        }
        default: {
          assert(0);
        }
      }
      return instr;
    }

    // SWP (Single Data Swap)
    if ((instr.opcode & (0xfb00ff0)) == 0x1000090) {
      spdlog::debug("read instruction {:#010X} is a single data swap",
                    instr.opcode);

      instr.B  = (instr.opcode & (1 << 22)) ? 1 : 0;
      instr.Rn = (instr.opcode & 0xf0000) >> 16;
      instr.Rd = (instr.opcode & 0xf000) >> 12;
      instr.Rm = instr.opcode & 0xf;

      assert(instr.Rm != 15);

      instr.mnemonic = fmt::format("swp{} r{}, r{}, [r{}]",
                                   condition_map.at(instr.condition), +instr.Rd,
                                   +instr.Rm, +instr.Rn);
      instr.func_ptr = ARM::Instructions::SWP;

      return instr;
    }

    // LDRH, STRH
    if ((instr.opcode & (0xe0000f0)) == 0xb0) {
      // spdlog::debug("read instruction {:#010X} is a halfword data transfer",
      //               instr.opcode);

      instr.L  = (instr.opcode & (1 << 20)) ? 1 : 0;  // Store or Load
      instr.Rn = (instr.opcode & 0xf0000) >> 16;      // base register
      instr.Rd = (instr.opcode & 0xf000) >> 12;       // destination register
      instr.U  = (instr.opcode & (1 << 23)) ? 1 : 0;  // UP DOWN BIT
      instr.P  = (instr.opcode & (1 << 24)) ? 1 : 0;  // indexing bit
      instr.W  = (instr.opcode & (1 << 21)) ? 1 : 0;  // writeback bit
      instr.I  = (instr.opcode & (1 << 22)) ? 1 : 0;  // Immediate bit
      instr.Rm = (instr.opcode & 0xf);

      u32 c_address = regs.r[instr.Rn];

      if (instr.P == 1 && instr.W) {
        if (instr.U == 1) {
          SPDLOG_INFO("UP");
          c_address += regs.r[instr.Rm];
        } else {
          SPDLOG_INFO("DOWN");
          c_address -= regs.r[instr.Rm];
        }
      }

      instr.mnemonic = fmt::format(
          "{}rh{} r{},[r{}], {}#${:#x} [#${:#x}]", instr.L ? "ld" : "st",
          condition_map.at(instr.condition), +instr.Rd, +instr.Rn,
          instr.U ? "+" : "-", +instr.Rm, c_address

      );

      if (instr.L) {
        instr.func_ptr = ARM::Instructions::LDRH;
      } else {
        instr.func_ptr = ARM::Instructions::STRH;
      }

      // SPDLOG_INFO(instr.mnemonic);

      return instr;
    }

    // LDRSB, LDRSH (Halfword and Signed Data Transfer)
    // 00000000000100000000000011010000
    if ((instr.opcode & (0xe1000d0)) == 0x1000d0) {
      spdlog::debug("read instruction {:#010X} is a signed data transfer",
                    instr.opcode);

      // spdlog::debug("read instruction {:#010X} is a halfword data transfer",
      //               instr.opcode);

      instr.L   = (instr.opcode & (1 << 20)) ? 1 : 0;  // Store or Load
      instr.Rn  = (instr.opcode & 0xf0000) >> 16;      // base register
      instr.Rd  = (instr.opcode & 0xf000) >> 12;       // destination register
      instr.U   = (instr.opcode & (1 << 23)) ? 1 : 0;  // UP DOWN BIT
      instr.P   = (instr.opcode & (1 << 24)) ? 1 : 0;  // indexing bit
      instr.W   = (instr.opcode & (1 << 21)) ? 1 : 0;  // writeback bit
      instr.I   = (instr.opcode & (1 << 22)) ? 1 : 0;  // Immediate bit
      instr.Rm  = (instr.opcode & 0xf);
      u8 opcode = (instr.opcode & 0b1100000) >> 5;

      assert(opcode != 1);
      assert(instr.L == 1);

      u32 c_address = regs.r[instr.Rn];

      if (instr.P == 1 && instr.W) {
        if (instr.U == 1) {
          // SPDLOG_INFO("UP");
          c_address += regs.r[instr.Rm];
        } else {
          // SPDLOG_INFO("DOWN");
          c_address -= regs.r[instr.Rm];
        }
      }

      SPDLOG_DEBUG("LDRSB/LDRSH");
      std::string_view mode_string;

      switch (opcode) {
        case 2: {
          mode_string    = "sb";
          instr.func_ptr = ARM::Instructions::LDRSB;
          break;
        }
        case 3: {
          mode_string    = "sh";
          instr.func_ptr = ARM::Instructions::LDRSH;
          break;
        }
        default:
          assert("invalid opcode");
      }

      instr.mnemonic =
          fmt::format("ldr{}{} r{},[r{}], {}#${:#x} [${:#x}]", mode_string,
                      condition_map.at(instr.condition), +instr.Rd, +instr.Rn,
                      instr.U ? "+" : "-", +instr.Rm, c_address

          );

      SPDLOG_DEBUG("{}", instr.mnemonic);
      // assert(0);

      // SPDLOG_INFO(instr.mnemonic);

      return instr;
    }

    // MRS
    // 00000001000000000000000000000000
    if ((instr.opcode & 0xfb000f0) == 0x1000000) {
      SPDLOG_INFO("MRS");
      instr.I = (instr.opcode & (1 << 25)) ? 1 : 0;
      instr.P = (instr.opcode & (1 << 22)) ? 1 : 0;

      assert(instr.I == 0);

      instr.Rd = (instr.opcode & 0xf000) >> 12;
      // instr.print_params();

      instr.func_ptr = ARM::Instructions::MRS;
      instr.mnemonic =
          fmt::format("mrs{} r{}, {}", condition_map.at(instr.condition),
                      +instr.Rd, instr.P == 0 ? "cpsr" : "spsr");
      return instr;
      // exit(-1);
    }

    // MSR register
    if ((instr.opcode & 0xfb000f0) == 0x1200000) {
      SPDLOG_INFO("MSR [register]");

      instr.P  = (instr.opcode & 0x400000) ? 1 : 0;
      instr.Rm = (instr.opcode & 0xf);
      instr.I  = (instr.opcode & (1 << 25)) ? 1 : 0;

      instr.func_ptr = ARM::Instructions::MSR;
      instr.mnemonic =
          fmt::format("msr{} {}, r{}", condition_map.at(instr.condition),
                      instr.P == 0 ? "cpsr" : "spsr", +instr.Rm);
      return instr;
    }

    // MSR immediate
    if ((instr.opcode & 0xfb00000) == 0x3200000) {
      SPDLOG_INFO("MSR [immediate]");

      instr.P         = (instr.opcode & (1 << 22)) ? 1 : 0;
      instr.I         = (instr.opcode & (1 << 25)) ? 1 : 0;
      u32 amount      = (instr.opcode & 0xff);
      u8 shift_amount = (instr.opcode & 0b111100000000) >> 8;

      amount = rotateRight(amount, shift_amount * 2);

      bool modify_flag_field      = (instr.opcode & (1 << 19));
      bool modify_status_field    = (instr.opcode & (1 << 18));
      bool modify_extension_field = (instr.opcode & (1 << 17));
      bool modify_control_field   = (instr.opcode & (1 << 16));

      std::string modifier_string = fmt::format(
          "_{}{}{}{}", modify_flag_field ? "f" : "",
          modify_status_field ? "s" : "", modify_extension_field ? "x" : "",
          modify_control_field ? "c" : "");

      instr.mnemonic =
          fmt::format("msr{} {}{}, #{:#x}", condition_map.at(instr.condition),
                      instr.P == 0 ? "cpsr" : "spsr", modifier_string, amount);
      instr.func_ptr = ARM::Instructions::MSR;
      return instr;
    }

    // BX
    if ((instr.opcode & BX_MASK) == 0x12fff10) {
      // spdlog::debug("read instruction {:#010X} is of branch and exchange",
      //               instr.opcode);
      instr.Rn = instr.opcode & 0xf;

      instr.func_ptr = ARM::Instructions::BX;
      instr.mnemonic =
          fmt::format("bx r{} [{:#010x}]", +instr.Rn, regs.r[instr.Rn]);
      return instr;
    }

    // LDM, STM (Block Data Transfer)
    if ((instr.opcode & 0xe000000) == 0x8000000) {
      instr.P  = (instr.opcode & (1 << 24)) ? 1 : 0;
      instr.U  = (instr.opcode & (1 << 23)) ? 1 : 0;
      instr.S  = (instr.opcode & (1 << 22)) ? 1 : 0;
      instr.W  = (instr.opcode & 0x200000) ? 1 : 0;
      instr.L  = (instr.opcode & 0x100000) ? 1 : 0;
      instr.Rn = (instr.opcode & 0xf0000) >> 16;

      u16 reg_list_v = instr.opcode & 0xFFFF;

      std::vector<u8> reg_list;

      for (size_t idx = 0; idx < 16; idx++) {
        if (reg_list_v & (1 << idx)) { reg_list.push_back(idx); }
      }
      
      if (instr.L == 0) {  // STM
        instr.func_ptr = ARM::Instructions::STM;
        instr.mnemonic =
            fmt::format("stm{} r{}{}, r{}", get_addressing_mode_string(instr),
                        +instr.Rn, instr.W ? "!" : "", fmt::join(reg_list, ",r"));
      } else {
        instr.func_ptr = ARM::Instructions::LDM;
        instr.mnemonic =
            fmt::format("ldm{} r{}{}, r{}", get_addressing_mode_string(instr),
                        +instr.Rn, instr.W ? "!" : "", fmt::join(reg_list, ",r"));
      }

      // fmt::println("LDM/STM");
      return instr;
    }

    if ((instr.opcode & 0xe000000) == 0x4000000) {
      // SPDLOG_INFO("LDR, STR [immediate]");
      instr.I = instr.opcode & (1 << 25) ? 1 : 0;
      instr.P = instr.opcode & (1 << 24) ? 1 : 0;
      instr.U = instr.opcode & (1 << 23) ? 1 : 0;
      instr.B = instr.opcode & (1 << 22) ? 1 : 0;
      instr.W = instr.opcode & (1 << 21) ? 1 : 0;
      instr.L = instr.opcode & (1 << 20) ? 1 : 0;

      instr.Rn     = (instr.opcode & 0xf0000) >> 16;
      instr.Rd     = (instr.opcode & 0xf000) >> 12;
      instr.offset = (instr.opcode & 0xfff);

      // instr.print_params();

      if (instr.L) {
        instr.func_ptr = ARM::Instructions::LDR;

        instr.mnemonic =
            fmt::format("ldr{}{} r{}, [r{}, {}#${:#06x}]", instr.B ? "b" : "",
                        condition_map.at(instr.condition), +instr.Rd, +instr.Rn,
                        instr.U ? "+" : "-", instr.offset);
        // fmt::println("{}", instr.mnemonic);
      } else {
        instr.func_ptr = ARM::Instructions::STR;
        instr.mnemonic =
            fmt::format("str{}{} r{}, [r{}, {}#${:#06x}]", instr.B ? "b" : "",
                        condition_map.at(instr.condition), +instr.Rd, +instr.Rn,
                        instr.U ? "+" : "-", instr.offset);
      }

      return instr;
    }

    // LDR, STR (Single Data Transfer)
    if ((instr.opcode & 0xe000010) == 0x6000000) {
      SPDLOG_INFO("LDR, STR [register]");
      instr.I = instr.opcode & (1 << 25) ? 1 : 0;
      instr.P = instr.opcode & (1 << 24) ? 1 : 0;
      instr.U = instr.opcode & (1 << 23) ? 1 : 0;
      instr.B = instr.opcode & (1 << 22) ? 1 : 0;
      instr.W = instr.opcode & (1 << 21) ? 1 : 0;
      instr.L = instr.opcode & (1 << 20) ? 1 : 0;

      instr.Rn = (instr.opcode & 0xf0000) >> 16;
      instr.Rd = (instr.opcode & 0xf000) >> 12;

      instr.Rm = (instr.opcode & 0xf);
      // 0000000

      // 11-7   Is - Shift amount      (1-31, 0=Special/See below)
      // 0b111110000000
      instr.shift_amount = (instr.opcode & 0xf80) >> 7;
      // 0b110000
      instr.shift_type = (instr.opcode & 0b1100000) >> 5;

      // instr.offset                  = (instr.opcode & 0xfff);
      std::string_view shift_string = get_shift_type_string(instr.shift_type);
      assert(instr.I == 1);
      if (instr.L) {
        instr.func_ptr = ARM::Instructions::LDR;

        // LDR R3, [R2, -R1, LSL #2]!
        instr.mnemonic = fmt::format(
            "ldr{}{} r{}, [r{}, {}r{}, {} #{}]", instr.B ? "b" : "",
            condition_map.at(instr.condition), +instr.Rd, +instr.Rn,
            instr.U ? "+" : "-", +instr.Rm, shift_string, instr.shift_amount);
      } else {
        instr.func_ptr = ARM::Instructions::STR;
        instr.mnemonic = fmt::format(
            "str{}{} r{}, [r{}, {}r{}, {} #{}]", instr.B ? "b" : "",
            condition_map.at(instr.condition), +instr.Rd, +instr.Rn,
            instr.U ? "+" : "-", +instr.Rm, shift_string, instr.shift_amount);
      }

      SPDLOG_DEBUG("{}", instr.mnemonic);
      return instr;
      // exit(-1);
    }

    // B (Branch)
    if ((instr.opcode & (0xe000000)) == 0xa000000) {
      // spdlog::info("read instruction {:#010X} is of branch", instr.opcode);

      instr.offset = (instr.opcode & 0xFFFFFF);
      instr.L      = (instr.opcode & 0x1000000) != 0;

      int x = instr.offset;

      if (x & 0x800000) { x -= 0x1000000; }
      x *= 4;

      // fmt::println("offset calced: {:#010x}", x);
      instr.offset = x;

      instr.mnemonic =
          fmt::format("b{}{} #{:#x}", instr.L ? "l" : "",
                      condition_map.at(instr.condition), regs.r[15] + x + 4);

      if (instr.mnemonic.starts_with(
              "bllt")) {  // TODO: when branch w/ link, lt
                          // should just be t (i think)
        instr.mnemonic.replace(0, 4, "blt");
      }

      instr.func_ptr = ARM::Instructions::B;
      return instr;
    }

    if ((instr.opcode & 0xe000010) == 0) {
      fmt::println("Data Processing (shift value is immediate)");

      u32 o_opcode = ((instr.opcode & OPCODE_MASK) >> 21);

      instr.Rd = (instr.opcode & 0xf000) >> 12;
      instr.S  = (instr.opcode & 0x100000) ? 1 : 0;
      instr.Rm = instr.opcode & 0xf;
      instr.Rn = (instr.opcode & 0xf0000) >> 16;
      instr.I  = (instr.opcode & 1 << 25) ? 1 : 0;

      assert(instr.I == 0);

      instr.shift_type   = (instr.opcode & 0x60) >> 5;
      instr.shift_amount = (instr.opcode & 0xf80) >> 7;

      instr.SHIFT                   = true;
      std::string_view shift_string = get_shift_type_string(instr.shift_type);
      // exit(-1);
      switch (o_opcode) {
        case AND: {
          instr.func_ptr = ARM::Instructions::AND;
          instr.mnemonic =
              fmt::format("and{}{} r{},r{},#{:#x} {}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2, shift_string);
          break;
        }
        case EOR: {
          break;
        }
        case SUB: {
          SPDLOG_INFO("x");
          instr.func_ptr = ARM::Instructions::SUB;
          instr.mnemonic =
              fmt::format("sub{}{} r{},r{},#{:#x} {}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2, shift_string);
          break;
        }
        case RSB: {
          break;
        }
        case ADD: {
          // ADD - Rd:= Op1 + Op2
          instr.func_ptr = ARM::Instructions::ADD;
          instr.mnemonic =
              fmt::format("add{}{} r{},r{},#{:#x} {}",
                          condition_map.at(instr.condition), instr.S ? "s" : "",
                          +instr.Rd, +instr.Rn, instr.op2, shift_string);
          break;
        }
        case ADC: {
          instr.func_ptr = ARM::Instructions::ADC;
          instr.mnemonic = fmt::format("adc{}{} r{}, r{}, {} #{}",
                                       condition_map.at(instr.condition),
                                       instr.S ? "s" : "", +instr.Rd, +instr.Rm,
                                       shift_string, instr.shift_amount);

          break;
        }
        case SBC: {
          break;
        }
        case RSC: {
          break;
        }
        case TST: {
          break;
        }
        case TEQ: {
          throw std::runtime_error("1");
          break;
        }
        case CMP: {
          // fmt::println("CMP");
          // instr.print_params();
          instr.func_ptr = ARM::Instructions::CMP;
          instr.mnemonic = fmt::format(
              "cmp{} r{}, r{}, {} #{}", condition_map.at(instr.condition),
              +instr.Rn, +instr.op2, shift_string, instr.shift_amount);

          break;
        }
        case CMN: {
          instr.func_ptr = ARM::Instructions::CMN;
          instr.mnemonic =
              fmt::format("cmn{} r{}, r{}", condition_map.at(instr.condition),
                          +instr.Rn, +instr.Rm, shift_string);
          break;
        }
        case ORR: {
          instr.func_ptr = ARM::Instructions::ORR;
          instr.mnemonic = fmt::format("orr{} r{}, #{:#x} {}",
                                       condition_map.at(instr.condition),
                                       +instr.Rn, instr.op2, shift_string);
          break;
        }
        case MOV: {
          instr.func_ptr = ARM::Instructions::MOV;
          instr.mnemonic = fmt::format("mov{}{} r{}, r{}, {} #{}",
                                       condition_map.at(instr.condition),
                                       instr.S ? "s" : "", +instr.Rd, +instr.Rm,
                                       shift_string, instr.shift_amount);
          break;
        }
        case BIC: {
          break;
        }
        case MVN: {
          break;
        }

        default: {
          SPDLOG_ERROR("could not resolve data processing opcode: {:08b}",
                       o_opcode);
          exit(-1);
        }
      }

      return instr;
    }

    if ((instr.opcode & 0xe000090) == 0x10) {
      fmt::println("Data Processing (shift value is a register)");
      u32 o_opcode                  = ((instr.opcode & OPCODE_MASK) >> 21);
      instr.Rd                      = (instr.opcode & 0xf000) >> 12;
      instr.op2                     = instr.opcode & 0xfff;
      instr.S                       = (instr.opcode & 0x100000) ? 1 : 0;
      instr.Rm                      = instr.opcode & 0xf;
      instr.Rn                      = (instr.opcode & 0xf0000) >> 16;
      instr.Rs                      = (instr.opcode & 0xf00) >> 8;
      instr.SHIFT                   = true;
      instr.shift_value_is_register = true;

      instr.shift_type = (instr.opcode & 0x60) >> 5;
      u8 shift_amount  = regs.r[instr.Rs] & 0xFF;

      std::string_view shift_string = get_shift_type_string(instr.shift_type);

      // Should be impossible.
      instr.I = (instr.opcode & 0x2000000) ? 1 : 0;
      if (instr.I == 1) throw std::runtime_error("I was set in register shift");

      switch (o_opcode) {
        case AND: {
          instr.func_ptr = ARM::Instructions::AND;
          instr.mnemonic =
              fmt::format("and{}{} r{},r{},#{:#x} {}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2, shift_string);
          break;
        }
        case EOR: {
          break;
        }
        case SUB: {
          SPDLOG_INFO("x");
          instr.func_ptr = ARM::Instructions::SUB;
          instr.mnemonic =
              fmt::format("sub{}{} r{},r{},#{:#x} {}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2, shift_string);
          break;
        }
        case RSB: {
          break;
        }
        case ADD: {
          instr.func_ptr = ARM::Instructions::ADD;
          instr.mnemonic =
              fmt::format("add{}{} r{}, r{}, {} r{} ",
                          condition_map.at(instr.condition), instr.S ? "s" : "",
                          +instr.Rd, +instr.Rn, shift_string, +instr.Rs);
          break;
        }
        case ADC: {
          instr.func_ptr = ARM::Instructions::ADC;
          instr.mnemonic =
              fmt::format("adc{}{} r{}, r{}, {} r{} ",
                          condition_map.at(instr.condition), instr.S ? "s" : "",
                          +instr.Rd, +instr.Rn, shift_string, +instr.Rs);
          break;
        }
        case SBC: {
          break;
        }
        case RSC: {
          break;
        }
        case TST: {
          break;
        }
        case TEQ: {
          throw std::runtime_error("2");
          break;
        }
        case CMP: {
          instr.func_ptr = ARM::Instructions::CMP;
          instr.mnemonic = fmt::format("cmp{} r{}, #{:#x} {}xxxx",
                                       condition_map.at(instr.condition),
                                       +instr.Rn, instr.op2, shift_string);
          break;
        }
        case CMN: {
          break;
        }
        case ORR: {
          instr.func_ptr = ARM::Instructions::ORR;
          instr.mnemonic = fmt::format("orr{} r{}, #{:#x} {}",
                                       condition_map.at(instr.condition),
                                       +instr.Rn, instr.op2, shift_string);
          break;
        }
        case MOV: {
          instr.func_ptr = ARM::Instructions::MOV;
          instr.mnemonic = fmt::format("mov{}{} r{}, r{}, {} r{} [{}]",
                                       condition_map.at(instr.condition),
                                       instr.S ? "s" : "", +instr.Rd, +instr.Rm,
                                       shift_string, +instr.Rs, shift_amount);
          fmt::println("{}", instr.mnemonic);
          // exit(-1);
          break;
        }
        case BIC: {
          break;
        }
        case MVN: {
          break;
        }

        default: {
          SPDLOG_ERROR("could not resolve data processing opcode: {:08b}",
                       o_opcode);
          exit(-1);
        }
      }

      return instr;
    }

    if ((instr.opcode & 0xfb00000) == 0x3000000) {
      SPDLOG_DEBUG("undefined instructions");

      assert(0);
      return instr;
    }

    if ((instr.opcode & 0xe000000) == 0x2000000) {
      fmt::println("Data Processing (immediate value)");

      u32 o_opcode = ((instr.opcode & OPCODE_MASK) >> 21);
      instr.Rd     = (instr.opcode & 0xf000) >> 12;
      instr.op2    = instr.opcode & 0xfff;
      instr.S      = (instr.opcode & 0x100000) ? 1 : 0;
      instr.Rm     = instr.opcode & 0xf;
      instr.Rn     = (instr.opcode & 0xf0000) >> 16;
      instr.imm    = instr.opcode & 0xff;
      instr.rotate = ((instr.opcode & 0xf00) >> 8);

      instr.I   = (instr.opcode & 0x2000000) ? 1 : 0;
      instr.op2 = rotateRight(instr.imm, instr.rotate * 2);
      // instr.print_params();
      // fmt::println("immediate value: {}, rot amount = {}", imm, rotate);

      switch (o_opcode) {
        case AND: {
          instr.func_ptr = ARM::Instructions::AND;
          instr.mnemonic =
              fmt::format("and{}{} r{},r{},#{:#x}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2);
          break;
        }
        case EOR: {
          // throw std::runtime_error("EOR");
          instr.func_ptr = ARM::Instructions::EOR;
          instr.mnemonic =
              fmt::format("eor{}{} r{},r{},#{:#x}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2);
          break;
        }
        case SUB: {
          // instr.print_params();
          instr.func_ptr = ARM::Instructions::SUB;
          instr.mnemonic =
              fmt::format("sub{}{} r{},r{},#{:#x}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2);
          break;
        }
        case RSB: {
          instr.func_ptr = ARM::Instructions::RSB;
          instr.mnemonic =
              fmt::format("rsb{}{} r{},r{},#{:#x}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2);
          break;
        }
        case ADD: {
          // ADD - Rd:= Op1 + Op2
          instr.func_ptr = ARM::Instructions::ADD;
          instr.mnemonic = fmt::format(
              "add{}{} r{},r{},#{:#x}", condition_map.at(instr.condition),
              instr.S ? "s" : "", +instr.Rd, +instr.Rn, instr.op2);
          break;
        }
        case ADC: {
          instr.func_ptr = ARM::Instructions::ADC;
          instr.mnemonic = fmt::format(
              "adc{}{} r{},r{},#{:#x}", condition_map.at(instr.condition),
              instr.S ? "s" : "", +instr.Rd, +instr.Rn, instr.op2);
          break;
        }
        case SBC: {
          instr.func_ptr = ARM::Instructions::SBC;
          instr.mnemonic =
              fmt::format("sbc{}{} r{},r{},#{:#x}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2);
          break;
        }
        case RSC: {
          instr.func_ptr = ARM::Instructions::RSC;
          instr.mnemonic =
              fmt::format("rsc{}{} r{},r{},#{:#x}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2);
          break;
        }
        case TST: {
          instr.func_ptr = ARM::Instructions::TST;
          instr.mnemonic = fmt::format("tst{} r{}, #{:#x}",
                                       condition_map.at(instr.condition),
                                       +instr.Rn, instr.op2);
          break;
        }
        case TEQ: {
          instr.func_ptr = ARM::Instructions::TEQ;
          instr.mnemonic = fmt::format("teq{} r{}, #{:#x}",
                                       condition_map.at(instr.condition),
                                       +instr.Rn, instr.op2);
          break;
        }
        case CMP: {
          instr.func_ptr = ARM::Instructions::CMP;
          instr.mnemonic = fmt::format("cmp{} r{}, #{:#x}",
                                       condition_map.at(instr.condition),
                                       +instr.Rn, instr.op2);
          break;
        }
        case CMN: {
          instr.func_ptr = ARM::Instructions::CMN;
          instr.mnemonic = fmt::format("cmn{} r{}, r{}, #${:#x}",
                                       condition_map.at(instr.condition),
                                       +instr.Rd, +instr.Rn, instr.op2);
          break;
        }
        case ORR: {
          instr.func_ptr = ARM::Instructions::ORR;
          instr.mnemonic = fmt::format("orr{} r{}, r{}, #${:#x}",
                                       condition_map.at(instr.condition),
                                       +instr.Rd, +instr.Rn, instr.op2);
          break;
        }
        case MOV: {
          instr.func_ptr = ARM::Instructions::MOV;
          instr.mnemonic = fmt::format(
              "mov{}{} r{}, #{:#x}", condition_map.at(instr.condition),
              instr.S ? "s" : "", +instr.Rd, instr.op2);
          break;
        }
        case BIC: {
          instr.func_ptr = ARM::Instructions::BIC;
          instr.mnemonic =
              fmt::format("bic{}{} r{},r{},#{:#x}", instr.S ? "s" : "",
                          condition_map.at(instr.condition), +instr.Rd,
                          +instr.Rn, instr.op2);
          break;
        }
        case MVN: {
          instr.func_ptr = ARM::Instructions::MVN;
          instr.mnemonic = fmt::format(
              "mvn{}{} r{}, #{:#x}", condition_map.at(instr.condition),
              instr.S ? "s" : "", +instr.Rd, instr.op2);
          break;
        }

        default: {
          SPDLOG_ERROR("could not resolve data processing opcode: {:08b}",
                       o_opcode);
          exit(-1);
        }
      }
      return instr;
    }

    if ((instr.opcode & 0xf000000) == 0xf000000) {
      // if (instr.opcode == 0xEF060000) {
      //   instr.func_ptr = ARM::Instructions::DIV_STUB;
      //   instr.mnemonic = "SWI stub - DIV";
      //   return instr;
      // }

      SPDLOG_ERROR("failed on {} - SWI number: {:#02x}", regs.r[0],
                   (instr.opcode & 0xff0000) >> 16);

      exit(-1);

      return instr;
    }
  } else {
    fmt::println("reached thumb decode");
    fmt::println("{:#x}", instr.opcode);

    if ((instr.opcode & 0xf800) == 0x2000) {  // ADD, SUB
      SPDLOG_INFO("ADD, SUB");
      // exit(-1);
    }

    if ((instr.opcode & 0xe000) == 0) {  // LSL, LSR, ASR, ROR
      SPDLOG_INFO("LSL, LSR, ASR, ROR");
      exit(-1);
    }

    if ((instr.opcode & 0xe000) == 0x2000) {  // Format 3
      SPDLOG_INFO("MOV, CMP, ADD, SUB");

      instr.offset = instr.opcode & 0xff;
      instr.Rd     = (instr.opcode >> 8) & 0b111;
      instr.op2    = instr.offset;
      u8 op        = (instr.opcode & 0x1800) >> 11;
      instr.I      = true;

      switch (op) {
        case 0: {  // MOV
          instr.func_ptr = ARM::Instructions::MOV;
          instr.mnemonic =
              fmt::format("mov r{}, #${:#x}", +instr.Rd, instr.offset);
          // instr.print_params();
          break;
        }
        case 1: {  // CMP
          instr.func_ptr = ARM::Instructions::CMP;
          instr.mnemonic =
              fmt::format("cmp r{}, #${}", +instr.Rd, instr.offset);
          break;
        }
        case 2: {  // ADD
          instr.func_ptr = ARM::Instructions::ADD;
          instr.mnemonic =
              fmt::format("add r{}, #${}", +instr.Rd, instr.offset);
          break;
        }
        case 3: {  // SUB
          instr.func_ptr = ARM::Instructions::SUB;
          instr.mnemonic =
              fmt::format("sub r{}, #${}", +instr.Rd, instr.offset);
          break;
        }
        default: {
          SPDLOG_DEBUG("INVALID OP: {:#x}", op);
          exit(-1);
        }
      }
      fmt::println("{:#x}", instr.opcode);
      fmt::println("{:016b}", instr.opcode);
      // exit(-1);
      return instr;
    }

    if ((instr.opcode & 0xfc00) == 0x4400) {
      SPDLOG_INFO("ADD, CMP, MOV (high registers)");
      fmt::println("{:#x}", instr.opcode);
      fmt::println("{:b}", instr.opcode);
      instr.Rd =
          ((instr.opcode & 1 << 7) ? 8 : 0) + (instr.opcode & 0b111);  // MSBd
      instr.Rs =
          ((instr.opcode & 1 << 6) ? 8 : 0) + ((instr.opcode & 0b111000) >> 3);
      instr.offset = 0xFF;

      u8 op = (instr.opcode & 0x300) >> 8;
      // fmt::println("{:#x}", op);
      switch (op) {
        case 0: {
          instr.func_ptr = ARM::Instructions::ADD;
          instr.mnemonic = fmt::format("add r{}, r{}", +instr.Rd, +instr.Rs);
          fmt::println("{}", instr.mnemonic);
          exit(-1);
          break;
        }
        case 1: {
          instr.func_ptr = ARM::Instructions::CMP;
          instr.mnemonic = fmt::format("cmp r{}, r{}", +instr.Rd, +instr.Rs);
          fmt::println("{}", instr.mnemonic);
          break;
        }
        case 2: {
          instr.func_ptr = ARM::Instructions::MOV;
          instr.mnemonic = fmt::format("mov r{}, r{}", +instr.Rd, +instr.Rs);
          fmt::println("{}", instr.mnemonic);
          break;
        }
        case 3: {
          instr.func_ptr = ARM::Instructions::BX;
          instr.Rn       = instr.Rs;
          instr.mnemonic =
              fmt::format("bx r{} [{:#010x}]", +instr.Rn, regs.r[instr.Rs]);
          fmt::println("BX");
          break;
        }
      }
      // exit(-1);
      return instr;
    }

    if ((instr.opcode & 0xf000) == 0xa000) {  // Format 12
      fmt::println("get relative address");
      u8 opcode = (instr.opcode & (1 << 11)) >> 11;
      fmt::println("opcode:{}", opcode);
      instr.Rd  = (instr.opcode & 0x700) >> 8;
      instr.Rn  = 15;
      instr.op2 = (instr.opcode & 0xff) * 255;

      switch (opcode) {
        case 0: {
          instr.func_ptr = ARM::Instructions::ADD;
          // instr.print_params();
          instr.mnemonic =
              fmt::format("add r{}, PC, #${:#x}", +instr.Rd, instr.op2);
          // fmt::println("{}", instr.mnemonic);
          // exit(-1);
          break;
        }
        case 1: {
          exit(-1);
          break;
        }
      }

      // exit(-1);
    }

    if ((instr.opcode & 0xf800) == 0xe000) {
      fmt::println("(unconditional) B");
      exit(-1);
    }

    if ((instr.opcode & 0xff00) == 0x4700) {
      fmt::println("BX out....");
      exit(-1);
    }
    // exit(-1);
    // Implement THUMB instructions
    return instr;
  }

  SPDLOG_WARN("could not resolve {} instruction {:#10X}",
              regs.CPSR.STATE_BIT == 0 ? "ARM" : "THUMB", instr.opcode);
  // exit(-1);
  return instr;
}

InstructionInfo ARM7TDMI::fetch(u32 address) {
  InstructionInfo instr;
  instr.opcode =
      regs.CPSR.STATE_BIT ? bus->read16(address) : bus->read32(address);
  instr.loc = address;
  // fmt::println("{:#x}",address);
  return instr;
}
u16 ARM7TDMI::step() {
  bus->cycles_elapsed++;
  // fmt::println("IN STEP");
  // print_pipeline();
  if (pipeline.fetch.opcode != 0) {
    pipeline.execute = pipeline.decode;
    pipeline.decode  = pipeline.fetch;
  }

  pipeline.fetch = fetch(regs.r[15]);

  if (this->pipeline.decode.opcode != 0) {
    pipeline.decode = decode(pipeline.decode);
  }

  if (this->pipeline.execute.opcode != 0) { execute(pipeline.execute); }

  // if(!this->pipeline_refilled) {

  // }

  regs.r[15] += regs.CPSR.STATE_BIT ? 2 : 4;

  // move everything up, so we can fetch a new value

  // print_pipeline();

  // if (this->pipeline.decode.opcode == 0) return 0;
  pipeline_refilled = false;

  return cycles_consumed;
}

void ARM7TDMI::print_pipeline() {
  spdlog::debug("============ PIPELINE ============");
  spdlog::debug("Fetch:  {:#010X}", this->pipeline.fetch.opcode);
  spdlog::debug("Decode  Phase Instr: {:#010X}", this->pipeline.decode.opcode);
  spdlog::debug("Execute Phase Instr: {:#010X}", this->pipeline.execute.opcode);
  spdlog::debug("PC (R15): {:#010X}", this->regs.r[15]);
  spdlog::debug("==================================");
}

void ARM7TDMI::execute(InstructionInfo& instr) {
  if (instr.func_ptr == nullptr) {
    SPDLOG_CRITICAL("could not execute instruction {:#10X}", instr.opcode);
    exit(-1);
  }
  spdlog::info("{}", instr.mnemonic);
  if (check_condition(instr)) {
    instr.func_ptr(*this, instr);
  } else {
    fmt::println("N: {} Z: {} C: {} V: {}", +regs.CPSR.SIGN_FLAG,
                 +regs.CPSR.ZERO_FLAG, +regs.CPSR.CARRY_FLAG,
                 +regs.CPSR.OVERFLOW_FLAG);
    fmt::println("failed: {}", instr.mnemonic);
  }
}

bool ARM7TDMI::check_condition(InstructionInfo& i) {
  if (regs.CPSR.STATE_BIT == THUMB_MODE) return true;
  switch (i.condition) {
    case EQ: {
      return regs.CPSR.ZERO_FLAG;
    }
    case NE: {
      return !regs.CPSR.ZERO_FLAG;
    }
    case CS: {
      return regs.CPSR.CARRY_FLAG;
    }
    case CC: {
      return !regs.CPSR.CARRY_FLAG;
    }
    case MI: {
      return regs.CPSR.SIGN_FLAG;
    }
    case PL: {
      return !regs.CPSR.SIGN_FLAG;
    }
    case VS: {
      return regs.CPSR.OVERFLOW_FLAG;
    }
    case VC: {
      return !regs.CPSR.OVERFLOW_FLAG;
    }
    case HI: {
      return regs.CPSR.CARRY_FLAG && !regs.CPSR.ZERO_FLAG;
    }
    case LS: {
      return !regs.CPSR.CARRY_FLAG || regs.CPSR.ZERO_FLAG;
    }
    case GE: {
      return regs.CPSR.SIGN_FLAG == regs.CPSR.OVERFLOW_FLAG;
    }
    case LT: {
      return regs.CPSR.SIGN_FLAG != regs.CPSR.OVERFLOW_FLAG;
    }
    case GT: {
      return !regs.CPSR.ZERO_FLAG &&
             regs.CPSR.SIGN_FLAG == regs.CPSR.OVERFLOW_FLAG;
    }
    case LE: {
      return regs.CPSR.ZERO_FLAG ||
             regs.CPSR.SIGN_FLAG != regs.CPSR.OVERFLOW_FLAG;
    }
    case AL: {
      return true;
    }
    case NV: {
      SPDLOG_WARN("instruction has NV condition.");
      return false;
    }
    default: {
      fmt::println("reached default, exiting.");
      exit(-1);
    }
  }
}
std::string ARM7TDMI::get_addressing_mode_string(const InstructionInfo& instr) {
  std::string addressing_mode =
      fmt::format("{}{}", instr.U ? "i" : "d", instr.P ? "b" : "a");

  return addressing_mode;
}

void ARM7TDMI::print_registers() {
  fmt::println("r0: {:#010x}", regs.r[0]);
  fmt::println("r1: {:#010x}", regs.r[1]);
  fmt::println("r2: {:#010x}", regs.r[2]);
  fmt::println("r3: {:#010x}", regs.r[3]);
  fmt::println("r4: {:#010x}", regs.r[4]);
  fmt::println("r5: {:#010x}", regs.r[5]);
  fmt::println("r6: {:#010x}", regs.r[6]);
  fmt::println("r7: {:#010x}", regs.r[7]);
  fmt::println("r8: {:#010x}", regs.r[8]);
  fmt::println("r9: {:#010x}", regs.r[9]);
  fmt::println("r10: {:#010x}", regs.r[10]);
  fmt::println("r11: {:#010x}", regs.r[11]);
  fmt::println("r12: {:#010x}", regs.r[12]);
  fmt::println("r13: {:#010x}", regs.r[13]);
  fmt::println("r14: {:#010x}", regs.r[14]);
  fmt::println("r15: {:#010x}", regs.r[15]);
}

std::string_view ARM7TDMI::get_shift_type_string(u8 shift_type) {
  assert(shift_type < 4);

  switch (shift_type) {
    case 0: {
      return "LSL";
    }
    case 1: {
      return "LSR";
    }
    case 2: {
      return "ASR";
    }
    case 3: {
      return "ROR";
    }
    default: {
      return "INVALID";
    }
  };
}
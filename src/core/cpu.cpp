#include "core/cpu.hpp"

#include "instructions/arm.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "spdlog/spdlog.h"
#include "utils/bitwise.hpp"
#define UNDEFINED_MASK 0x6000010
#define OPCODE_MASK 0x1e00000
#define BX_MASK 0xffffff0

ARM7TDMI::ARM7TDMI() {
  regs.r[15] = 0x08000000;
  spdlog::debug("created cpu");
}
void ARM7TDMI::flush_pipeline() {
  pipeline = {};

  // spdlog::debug("pipeline flushed");
}
InstructionInfo ARM7TDMI::decode(InstructionInfo& instr) {
  instr.condition = (Condition)(instr.opcode >> 28);

  // MUL, MLA (Multiply)
  if ((instr.opcode & 0xfc00000) == 0 && (instr.opcode & 0xf0) == 0x90) {
    spdlog::debug("read instruction {:#010X} is a multiply", instr.opcode);
    instr.Rn = (instr.opcode & 0xf000) >> 12;
    instr.Rd = (instr.opcode & 0xf0000) >> 16;
    instr.Rs = (instr.opcode & 0xf00);
    instr.Rm = (instr.opcode & 0xf);
    instr.A  = (instr.opcode & 0x200000) >> 21;
    instr.S  = (instr.opcode & 0x100000) >> 20;

    // instr.func_ptr = MUL;
    return instr;
  }

  // MULL, MLAL (Multiply)
  if ((instr.opcode & 0xf800000) == 0x800000 && (instr.opcode & 0xf0) == 0x90) {
    spdlog::debug("read instruction {:#010X} is a multiply long", instr.opcode);

    return instr;
  }

  // SWP (Single Data Swap)
  if ((instr.opcode & (0xfb00ff0)) == 0x1000090) {
    // spdlog::debug("read instruction {:#010X} is a single data swap",
    //               instr.opcode);
    return instr;
  }

  // LDRH, STRH (Halfword and Signed Data Transfer)
  if ((instr.opcode & (0xe000000)) == 0 && (instr.opcode & 0xf0) == 0xb0) {
    // spdlog::debug("read instruction {:#010X} is a halfword data transfer",
    //               instr.opcode);

    instr.L  = (instr.opcode & (1 << 20)) >> 20 ? 1 : 0;  // Store or Load
    instr.Rn = (instr.opcode & 0xf0000) >> 16;            // base register
    instr.Rd = (instr.opcode & 0xf000) >> 12;  // destination register
    instr.I  = (instr.opcode & 0x2000000) ? 1 : 0;
    instr.U  = (instr.opcode & 0x800000) ? 1 : 0;   // UP DOWN BIT
    instr.P  = (instr.opcode & 0x1000000) ? 1 : 0;  // indexing bit
    instr.W  = (instr.opcode & 0x200000) ? 1 : 0;   // writeback bit
    instr.Rm = (instr.opcode & 0xf);
    // instr.H = (instr.opcode &  0x10) ? 1 : 0;

    if (instr.I != 0) {
      SPDLOG_INFO("SHIFT TYPE: {}", 0);
      exit(-1);
    }

    //  1: STR{cond}H  Rd,<Address>  ;Store halfword   [a]=Rd
    //     2: LDR{cond}D  Rd,<Address>  ;Load Doubleword  R(d)=[a], R(d+1)=[a+4]
    //     3: STR{cond}D  Rd,<Address>  ;Store Doubleword [a]=R(d), [a+4]=R(d+1)
    // SPDLOG_INFO("R0: {:#x}", regs.r[0]);
    // SPDLOG_INFO("R1: {:#x}", regs.r[1]);
    // SPDLOG_INFO("Rm: {:#x}", (u8)instr.Rm);
    // SPDLOG_INFO("Rn: {:#x}", (u8)instr.Rn);
    // SPDLOG_INFO("W: {:#x}", (u8)instr.W);

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

    instr.mnemonic =
        fmt::format("{}rh{} r{},[r{}], +#${:#x} [#${:#x}]", instr.L ? "ld" : "st",
                    condition_map.at(instr.condition), (u8)instr.Rd, (u8)instr.Rn, (u8)instr.Rm, c_address

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
  if ((instr.opcode & (0xe100000)) == 0x100000 &&
      (instr.opcode & 0xd0) == 0xd0) {
    // spdlog::debug("read instruction {:#010X} is a signed data transfer",
    //               instr.opcode);
    return instr;
  }

  // MRS
  if ((instr.opcode & 0xfb00000) == 0x1000000 &&
      ((instr.opcode & 0xf0) == 0x0)) {
    SPDLOG_INFO("MRS");
    exit(-1);
  }

  // MSR register
  if ((instr.opcode & 0xfb00000) == 0x1200000 &&
      ((instr.opcode & 0xf0) == 0x0)) {
    SPDLOG_INFO("MSR [register]");

    instr.P  = (instr.opcode & 0x400000) ? 1 : 0;
    instr.Rm = (instr.opcode & 0xf);

    instr.func_ptr = ARM::Instructions::MSR;
    instr.mnemonic =
        fmt::format("msr{} {}, r{}", condition_map.at(instr.condition),
                    instr.P ? "cpsr" : "spsr", (u8)instr.Rm);
    return instr;
    // exit(-1);
  }

  // MSR immediate
  if ((instr.opcode & 0xfb00000) == 0x1200000) {
    SPDLOG_INFO("MSR [immediate]");
    instr.func_ptr = ARM::Instructions::MSR;
    // exit(-1);
  }

  // BX
  if ((instr.opcode & BX_MASK) == 0x12fff10) {
    // spdlog::debug("read instruction {:#010X} is of branch and exchange",
    //               instr.opcode);
    return instr;
  }

  // LDM, STM (Block Data Transfer)
  if ((instr.opcode & 0xe000000) == 0x8000000) {
    // spdlog::debug("read instruction {:#010X} is a block data transfer",
    //               instr.opcode);
    return instr;
  }

  // B (Branch)
  if ((instr.opcode & (0xe000000)) == 0xa000000) {
    // spdlog::info("read instruction {:#010X} is of branch", instr.opcode);

    instr.offset = (instr.opcode & 0xFFFFFF);
    instr.L      = (instr.opcode & 0x800000) != 0;

    int32_t value = instr.offset & 0x00FFFFFF;
    value <<= 2;
    
    // sign extend 32 bits
    if (value & 0x02000000) {
        // extend if sign bit set
        value |= 0xFC000000;
    }
    
    instr.mnemonic = fmt::format("b{}{} #{:#x}", instr.L ? "l" : "",
                                 condition_map.at(instr.condition),
                                 value + regs.r[15]);

    if(instr.mnemonic.starts_with("bllt")) { // TODO: when branch w/ link, lt should just be t (i think)
      instr.mnemonic.replace(0, 4, "blt");
    }

    instr.func_ptr = ARM::Instructions::B;
    return instr;
  }

  if ((instr.opcode & 0x2000000) != 0) {
    u32 o_opcode = ((instr.opcode & OPCODE_MASK) >> 21); // surely there's a better name for this 
    // fmt::println("instr.opcode: {:#x}", instr.opcode);
    instr.Rd      = (instr.opcode & 0xf000) >> 12;
    instr.op2     = instr.opcode & 0xfff;
    instr.S       = instr.opcode & 0x100000;
    instr.Rm      = instr.opcode & 0xf;
    instr.Rn      = (instr.opcode & 0xf0000) >> 16;
    u8 imm        = instr.opcode & 0xff;
    u8 rotate     = ((instr.opcode & 0xf00) >> 8);
    u8 shift_type = (instr.opcode & 0x30) >> 4;
    instr.I       = (instr.opcode & 0x2000000) ? 1 : 0;

    if (instr.I == 0) {
      SPDLOG_INFO("SHIFT TYPE: {}", shift_type);
      exit(-1);
    } else {
      instr.op2 = rotateImmediate(imm, rotate * 2, false);
    }


    switch (o_opcode) {
      case AND: {
        break;
      }
      case EOR: {
        break;
      }
      case SUB: {
        break;
      }
      case RSB: {
        break;
      }
      case ADD: {
        // ADD - Rd:= Op1 + Op2
        instr.func_ptr = ARM::Instructions::ADD;
        instr.mnemonic = fmt::format("add{} r{},r{},#{:#x}",
                                     condition_map.at(instr.condition),
                                     (u8)instr.Rd, (u8)instr.Rn, instr.op2);
        break;
      }
      case ADC: {
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
        break;
      }
      case CMP: {
        instr.func_ptr = ARM::Instructions::CMP;
        instr.mnemonic =
            fmt::format("cmp{} r{}, #{:#x}", condition_map.at(instr.condition),
                        (u8)instr.Rn, instr.op2);
        break;
      }
      case CMN: {
        break;
      }
      case ORR: {
        break;
      }
      case MOV: {
        instr.func_ptr = ARM::Instructions::MOV;
        instr.mnemonic =
            fmt::format("mov{} r{}, #{:#x}", condition_map.at(instr.condition),
                        (u8)instr.Rd, instr.op2);
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

  if ((instr.opcode & 0xf000000) == 0xf000000) {
    SPDLOG_CRITICAL("UNHANDLED SWI: {}", instr.opcode);
    exit(-1);

    return instr;
  }

  SPDLOG_WARN("could not resolve instruction {:#10X}", instr.opcode);
  // exit(-1);
  return instr;
}

InstructionInfo ARM7TDMI::fetch(u32 address) {
  InstructionInfo instr;
  instr.opcode = bus->read32(address);
  instr.loc    = address;
  // fmt::println("{:#x}",address);
  return instr;
}
void ARM7TDMI::step() {
  // move everything up, so we can fetch a new value
  if (pipeline.fetch.opcode != 0) {
    pipeline.execute = pipeline.decode;
    pipeline.decode  = pipeline.fetch;
  }

  pipeline.fetch = fetch(regs.r[15]);

  // print_pipeline();

  regs.r[15] += 4;

  if (this->pipeline.decode.opcode == 0) return;

  pipeline.decode = decode(pipeline.decode);

  if (this->pipeline.execute.opcode == 0) return;

  execute(pipeline.execute);
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
    // spdlog::info("{}", instr.mnemonic);
  if (check_condition(instr)) {
    instr.func_ptr(*this, instr);
  } else {
    fmt::println("N: {} Z: {} C:{} V: {}",
  +regs.CPSR.SIGN_FLAG,
  +regs.CPSR.ZERO_FLAG,
  +regs.CPSR.CARRY_FLAG,
  +regs.CPSR.OVERFLOW_FLAG);
    fmt::println("failed: {}", instr.mnemonic);
  }
}

bool ARM7TDMI::check_condition(InstructionInfo& i) {
  switch (i.condition) {
    case EQ: {
      return regs.CPSR.ZERO_FLAG;
    }
    case NE: {
      return !regs.CPSR.ZERO_FLAG;
    }
    case CS_HS: {
      return regs.CPSR.CARRY_FLAG;
    }
    case CC_LO: {
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
      return regs.CPSR.ZERO_FLAG &&
             regs.CPSR.SIGN_FLAG == regs.CPSR.OVERFLOW_FLAG;
    }
    case LE: {
      throw std::runtime_error("LE condition unimplemented");
    }
    case AL: {
      return true;
    }
    case NV: {
      fmt::println("reached never, exiting.");
      exit(-1);
    }
    default: {
      fmt::println("reached default, exiting.");
      exit(-1);
    }
  }
}

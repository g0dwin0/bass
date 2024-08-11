#include "core/cpu.hpp"

#include "instructions/arm.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "shifter.cpp"
#include "spdlog/spdlog.h"
#include "utils/bitwise.hpp"
#define UNDEFINED_MASK 0x6000010
#define BX_MASK 0xffffff0
#define OPCODE_MASK 0x1e00000

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
  if ((instr.opcode & 0xfc000f0) == 0xf0) {
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
  if ((instr.opcode & 0xf8000f0) == 0x800090) {
    spdlog::debug("read instruction {:#010X} is a multiply long", instr.opcode);

    return instr;
  }

  // SWP (Single Data Swap)
  if ((instr.opcode & (0xfb00ff0)) == 0x1000090) {
    spdlog::debug("read instruction {:#010X} is a single data swap",
                  instr.opcode);
    return instr;
  }

  // LDRH, STRH (Halfword and Signed Data Transfer)
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
    // instr.H = (instr.opcode &  0x10) ? 1 : 0;

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
        fmt::format("{}rh{} r{},[r{}], +#${:#x} [#${:#x}]",
                    instr.L ? "ld" : "st", condition_map.at(instr.condition),
                    +instr.Rd, +instr.Rn, +instr.Rm, c_address

        );

    if (instr.L) {
      instr.func_ptr = ARM::Instructions::LDRH;
    } else {
      instr.func_ptr = ARM::Instructions::STRH;
    }

    // SPDLOG_INFO(instr.mnemonic);

    return instr;
  }

  //

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
  if ((instr.opcode & 0xfb000f0) == 0x1200000) {
    SPDLOG_INFO("MSR [register]");

    instr.P  = (instr.opcode & 0x400000) ? 1 : 0;
    instr.Rm = (instr.opcode & 0xf);
    instr.I  = (instr.opcode & (1 << 25)) ? 1 : 0;

    instr.func_ptr = ARM::Instructions::MSR;
    instr.mnemonic =
        fmt::format("msr{} {}, r{}", condition_map.at(instr.condition),
                    instr.P ? "cpsr" : "spsr", +instr.Rm);
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

    // instr.print_params();

    bool modify_flag_field      = (instr.opcode & (1 << 19));
    bool modify_status_field    = (instr.opcode & (1 << 18));
    bool modify_extension_field = (instr.opcode & (1 << 17));
    bool modify_control_field   = (instr.opcode & (1 << 16));

    std::string modifier_string = fmt::format("_{}{}{}{}", modify_flag_field ? "f" : "",
                 modify_status_field ? "s" : "",
                 modify_extension_field ? "x" : "",
                 modify_control_field ? "c" : "");

    instr.mnemonic =
        fmt::format("msr{} {}{}, #{:#x}", condition_map.at(instr.condition),
                    instr.P == 0 ? "cpsr" : "spsr", modifier_string,amount);
    instr.func_ptr = ARM::Instructions::MSR;
    return instr;
  }

  // BX
  if ((instr.opcode & BX_MASK) == 0x12fff10) {
    // spdlog::debug("read instruction {:#010X} is of branch and exchange",
    //               instr.opcode);
    return instr;
  }

  // LDM, STM (Block Data Transfer)
  if ((instr.opcode & 0xe000000) == 0x8000000) {
    instr.P  = (instr.opcode & 0x1000000) ? 1 : 0;
    instr.U  = (instr.opcode & 0x800000) ? 1 : 0;
    instr.S  = (instr.opcode & 0x400000) ? 1 : 0;
    instr.W  = (instr.opcode & 0x200000) ? 1 : 0;
    instr.L  = (instr.opcode & 0x100000) ? 1 : 0;
    instr.Rn = (instr.opcode & 0xf0000) >> 16;

    // instr.print_params();

    if (instr.L == 0) {  // STM
      instr.func_ptr = ARM::Instructions::STM;
      instr.mnemonic =
          fmt::format("stm{} r{}{}, {{}}", get_addressing_mode_string(instr),
                      +instr.Rn, instr.W ? "!" : "");
    } else {
      instr.func_ptr = ARM::Instructions::LDM;
      instr.mnemonic =
          fmt::format("ldm{} r{}{}, {{}}", get_addressing_mode_string(instr),
                      +instr.Rn, instr.W ? "!" : "");
    }

    // fmt::println("LDM/STM");
    return instr;
  }


  if((instr.opcode & 0xe000000) == 0x4000000) {
    SPDLOG_INFO("LDR, STR [immediate]");
    instr.I = instr.opcode & (1 << 25) ? 1 : 0;
    instr.P = instr.opcode & (1 << 24) ? 1 : 0;
    instr.U = instr.opcode & (1 << 23) ? 1 : 0;
    instr.B = instr.opcode & (1 << 22) ? 1 : 0;
    instr.W = instr.opcode & (1 << 21) ? 1 : 0;
    instr.L = instr.opcode & (1 << 20) ? 1 : 0;
    
    instr.Rn = (instr.opcode & 0xf0000) >> 16;
    instr.Rd = (instr.opcode & 0xf000) >> 12;
    instr.offset = (instr.opcode & 0xfff);
    
    instr.print_params();


    if(instr.L) {
      instr.func_ptr = ARM::Instructions::LDR;
      instr.mnemonic = fmt::format("ldr{} r{}, [r{}, {}#${:#06x}]" , condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.U ? "+" : "-", instr.offset);
      // fmt::println("{}", instr.mnemonic);
    } else {
      instr.func_ptr = ARM::Instructions::STR;
      fmt::println("is store!");
      exit(-1);
    }

    return instr;
  }

  // 00000110000000000000000000000000
  if((instr.opcode & 0xe000010) == 0x6000000) {
    SPDLOG_INFO("LDR, STR [register]");
    exit(-1);
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

    instr.mnemonic =
        fmt::format("b{}{} #{:#x}", instr.L ? "l" : "",
                    condition_map.at(instr.condition), regs.r[15] + x);

    if (instr.mnemonic.starts_with("bllt")) {  // TODO: when branch w/ link, lt
                                               // should just be t (i think)
      instr.mnemonic.replace(0, 4, "blt");
    }

    instr.func_ptr = ARM::Instructions::B;
    return instr;
  }

  if ((instr.opcode & 0xe000010) == 0) {
    fmt::println("register data processing");

    u32 o_opcode = ((instr.opcode & OPCODE_MASK) >>
                    21);  // surely there's a better name for this
    // fmt::println("instr.opcode: {:#x}", instr.opcode);
    instr.I   = (instr.opcode & 0x2000000) ? 1 : 0;
    instr.Rd  = (instr.opcode & 0xf000) >> 12;
    instr.op2 = instr.opcode & 0xfff;
    instr.S   = (instr.opcode & 0x100000) ? 1 : 0;
    instr.Rm  = instr.opcode & 0xf;
    instr.Rn  = (instr.opcode & 0xf0000) >> 16;
    instr.Rs  = (instr.opcode & 0xf00) >> 8;
    // u8 imm        = instr.opcode & 0xff;
    // u8 rotate     = ((instr.opcode & 0xf00) >> 8);
    u8 shift_type = (instr.opcode & 0x60) >> 5;
    u8 shift_amount;

    bool is_shift_by_register =
        (instr.opcode & 1 << 4) ? true : false;  // Bit 4

    std::string_view shift_string = get_shift_type_string(shift_type);

    fmt::println("I: {}", +instr.I);
    fmt::println("opcode: {:#x}", instr.opcode);
    fmt::println("is shift by register: {:#x}", is_shift_by_register);

    if (is_shift_by_register) {
      fmt::println("Rs: {:#x}", +instr.Rs);
      shift_amount =
          ((regs.r[instr.Rs] & 0xFF));  // TODO: This is wrong... why?
    } else {
      shift_amount = (instr.opcode & 0xf80) >> 7;
      auto [retval, overflowed] =
          shift((SHIFT_MODE)shift_type, regs.r[instr.Rm], shift_amount);

      instr.op2 = retval;
      // throw std::runtime_error("w");
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
        instr.mnemonic = fmt::format(
            "add{} r{},r{},#{:#x} {}", condition_map.at(instr.condition),
            +instr.Rd, +instr.Rn, instr.op2, shift_string);
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
        instr.mnemonic = fmt::format("cmp{} r{}, #{:#x} {}",
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
        instr.mnemonic = fmt::format(
            "mov{}{} r{}, r{}, {} #{}", condition_map.at(instr.condition),instr.S ? "s" : "",
            +instr.Rd, +instr.Rm, shift_string, shift_amount);
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
    fmt::println("register shift");

    exit(-1);
    return instr;
  }

  if ((instr.opcode & 0xfb00000) == 0x3000000) {
    fmt::println("undefined instructions");
    return instr;
  }

  if ((instr.opcode & 0xe000000) == 0x2000000) {
    fmt::println("dp immediate");

    u32 o_opcode = ((instr.opcode & OPCODE_MASK) >>
                    21);  // surely there's a better name for this
    // fmt::println("instr.opcode: {:#x}", instr.opcode);
    instr.Rd  = (instr.opcode & 0xf000) >> 12;
    instr.op2 = instr.opcode & 0xfff;
    instr.S   = instr.opcode & 0x100000;
    instr.Rm  = instr.opcode & 0xf;
    instr.Rn  = (instr.opcode & 0xf0000) >> 16;
    u8 imm    = instr.opcode & 0xff;
    u8 rotate = ((instr.opcode & 0xf00) >> 8);
    // u8 shift_type = (instr.opcode & 0x60) >> 5;
    instr.I = (instr.opcode & 0x2000000) ? 1 : 0;

    instr.op2 = rotateRight(imm, rotate * 2);

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
                                     +instr.Rd, +instr.Rn, instr.op2);
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
        instr.func_ptr = ARM::Instructions::TST;
        instr.mnemonic =
            fmt::format("tst{} r{}, #{:#x}", condition_map.at(instr.condition),
                        +instr.Rn, instr.op2);
        break;
      }
      case TEQ: {
        break;
      }
      case CMP: {
        instr.func_ptr = ARM::Instructions::CMP;
        instr.mnemonic =
            fmt::format("cmp{} r{}, #{:#x}", condition_map.at(instr.condition),
                        +instr.Rn, instr.op2);
        break;
      }
      case CMN: {
        break;
      }
      case ORR: {
        instr.func_ptr = ARM::Instructions::ORR;
        instr.mnemonic =
            fmt::format("orr{} r{}, #{:#x}", condition_map.at(instr.condition),
                        +instr.Rn, instr.op2);
        break;
      }
      case MOV: {
        instr.func_ptr = ARM::Instructions::MOV;
        instr.mnemonic =
            fmt::format("mov{} r{}, #{:#x}", condition_map.at(instr.condition),
                        +instr.Rd, instr.op2);
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
    fmt::println("failed on {:#x}", regs.r[0]);
    SPDLOG_CRITICAL("UNHANDLED SWI: {:#x}", instr.opcode);
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
u16 ARM7TDMI::step() {
  cycles_consumed = 0;

  // move everything up, so we can fetch a new value
  if (pipeline.fetch.opcode != 0) {
    pipeline.execute = pipeline.decode;
    pipeline.decode  = pipeline.fetch;
  }

  pipeline.fetch = fetch(regs.r[15]);

  // print_pipeline();

  regs.r[15] += 4;

  if (this->pipeline.decode.opcode == 0) return 0;

  pipeline.decode = decode(pipeline.decode);

  if (this->pipeline.execute.opcode == 0) return 0;

  execute(pipeline.execute);

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
    fmt::println("N: {} Z: {} C:{} V: {}", +regs.CPSR.SIGN_FLAG,
                 +regs.CPSR.ZERO_FLAG, +regs.CPSR.CARRY_FLAG,
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
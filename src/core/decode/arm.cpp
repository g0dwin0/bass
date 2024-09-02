#include "instructions/arm.hpp"
#include "cpu.hpp"
#include "instructions/instruction.hpp"

#define UNDEFINED_MASK 0x6000010
#define BX_MASK 0xffffff0
#define OPCODE_MASK 0x1e00000

InstructionInfo ARM7TDMI::arm_decode(InstructionInfo& instr) {
  instr.condition = (Condition)(instr.opcode >> 28);

  // MUL, MLA (Multiply)
  if ((instr.opcode & 0xfc000f0) == 0x90) {
    // spdlog::debug("read instruction {:#010X} is a multiply", instr.opcode);

    instr.S = (instr.opcode & 1 << 20) ? 1 : 0;

    instr.Rd = (instr.opcode & 0xf0000) >> 16;
    instr.Rn = (instr.opcode & 0xf000) >> 12;
    instr.Rs = (instr.opcode & 0xf00) >> 8;
    instr.Rm = (instr.opcode & 0xf);

    u8 op = (instr.opcode & 0x1e00000) >> 21;

    switch (op) {
      case 0: {  // MUL
        instr.mnemonic = fmt::format("mul r{}, r{}, r{}, r{}", +instr.Rd, +instr.Rm, +instr.Rs, +instr.Rn);
        instr.func_ptr = ARM::Instructions::MUL;
        break;
      }
      case 1: {  // MLA
        instr.mnemonic = fmt::format("mla r{}, r{}, r{}, r{}", +instr.Rd, +instr.Rm, +instr.Rs, +instr.Rn);
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
    // spdlog::debug("read instruction {:#010X} is a multiply long", instr.opcode);
    instr.S = (instr.opcode & 1 << 20) ? 1 : 0;

    instr.Rd = (instr.opcode & 0xf0000) >> 16;
    instr.Rn = (instr.opcode & 0xf000) >> 12;
    instr.Rs = (instr.opcode & 0xf00) >> 8;
    instr.Rm = (instr.opcode & 0xf);
    u8 op    = (instr.opcode & 0x1e00000) >> 21;

    switch (op) {
      case 4: {  // UMULL
        instr.mnemonic = fmt::format("umull{} r{}, r{}, r{}, r{}", condition_map.at(instr.condition), +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
        instr.func_ptr = ARM::Instructions::UMULL;

        break;
      }
      case 5: {  // UMLAL
        instr.mnemonic = fmt::format("umlal{} r{} r{} r{} r{}", condition_map.at(instr.condition), +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
        instr.func_ptr = ARM::Instructions::UMLAL;
        break;
      }
      case 6: {  // SMULL
        instr.mnemonic = fmt::format("smull{} r{}, r{}, r{}, r{}", condition_map.at(instr.condition), +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
        instr.func_ptr = ARM::Instructions::SMULL;
        break;
      }
      case 7: {  // SMLAL
        instr.mnemonic = fmt::format("smlal{} r{}, r{}, r{}, r{}", condition_map.at(instr.condition), +instr.Rn, +instr.Rd, +instr.Rm, +instr.Rs);
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
    // spdlog::debug("read instruction {:#010X} is a single data swap", instr.opcode);

    instr.B  = (instr.opcode & (1 << 22)) ? 1 : 0;
    instr.Rn = (instr.opcode & 0xf0000) >> 16;
    instr.Rd = (instr.opcode & 0xf000) >> 12;
    instr.Rm = instr.opcode & 0xf;

    assert(instr.Rm != 15);

    instr.mnemonic = fmt::format("swp{} r{}, r{}, [r{}]", condition_map.at(instr.condition), +instr.Rd, +instr.Rm, +instr.Rn);
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
        // SPDLOG_INFO("UP");
        c_address += regs.r[instr.Rm];
      } else {
        // SPDLOG_INFO("DOWN");
        c_address -= regs.r[instr.Rm];
      }
    }

    instr.mnemonic = fmt::format("{}rh{} r{},[r{}], {}#${:#x} [#${:#x}]", instr.L ? "ld" : "st", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.U ? "+" : "-", +instr.Rm, c_address

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
  if ((instr.opcode & (0xe1000d0)) == 0x1000d0) {
    // spdlog::debug("read instruction {:#010X} is a signed data transfer", instr.opcode);

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

    // SPDLOG_DEBUG("LDRSB/LDRSH");
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
      default: assert("invalid opcode");
    }

    instr.mnemonic = fmt::format("ldr{}{} r{},[r{}], {}#${:#x} [${:#x}]", mode_string, condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.U ? "+" : "-", +instr.Rm, c_address

    );

    return instr;
  }

  // MRS
  if ((instr.opcode & 0xfb000f0) == 0x1000000) {
    // SPDLOG_INFO("MRS");
    instr.I = (instr.opcode & (1 << 25)) ? 1 : 0;
    instr.P = (instr.opcode & (1 << 22)) ? 1 : 0;

    assert(instr.I == 0);

    instr.Rd = (instr.opcode & 0xf000) >> 12;

    instr.func_ptr = ARM::Instructions::MRS;
    instr.mnemonic = fmt::format("mrs{} r{}, {}", condition_map.at(instr.condition), +instr.Rd, instr.P == 0 ? "cpsr" : "spsr");
    return instr;
  }

  // MSR register
  if ((instr.opcode & 0xfb000f0) == 0x1200000) {
    // SPDLOG_INFO("MSR [register]");

    instr.P  = (instr.opcode & 0x400000) ? 1 : 0;
    instr.Rm = (instr.opcode & 0xf);
    instr.I  = (instr.opcode & (1 << 25)) ? 1 : 0;

    instr.func_ptr = ARM::Instructions::MSR;
    instr.mnemonic = fmt::format("msr{} {}, r{}", condition_map.at(instr.condition), instr.P == 0 ? "cpsr" : "spsr", +instr.Rm);
    return instr;
  }

  // MSR immediate
  if ((instr.opcode & 0xfb00000) == 0x3200000) {
    // SPDLOG_INFO("MSR [immediate]");

    instr.P         = (instr.opcode & (1 << 22)) ? 1 : 0;
    instr.I         = (instr.opcode & (1 << 25)) ? 1 : 0;
    u32 amount      = (instr.opcode & 0xff);
    u8 shift_amount = (instr.opcode & 0b111100000000) >> 8;

    amount = std::rotr(amount, shift_amount * 2);

    bool modify_flag_field      = (instr.opcode & (1 << 19));
    bool modify_status_field    = (instr.opcode & (1 << 18));
    bool modify_extension_field = (instr.opcode & (1 << 17));
    bool modify_control_field   = (instr.opcode & (1 << 16));

    std::string modifier_string = fmt::format("_{}{}{}{}", modify_flag_field ? "f" : "", modify_status_field ? "s" : "", modify_extension_field ? "x" : "", modify_control_field ? "c" : "");

    instr.mnemonic = fmt::format("msr{} {}{}, #{:#x}", condition_map.at(instr.condition), instr.P == 0 ? "cpsr" : "spsr", modifier_string, amount);
    instr.func_ptr = ARM::Instructions::MSR;
    return instr;
  }

  // BX
  if ((instr.opcode & BX_MASK) == 0x12fff10) {
    // spdlog::debug("read instruction {:#010X} is of branch and exchange",
    //               instr.opcode);
    instr.Rn = instr.opcode & 0xf;

    instr.func_ptr = ARM::Instructions::BX;
    instr.mnemonic = fmt::format("bx r{} [{:#010x}]", +instr.Rn, regs.r[instr.Rn]);
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
      instr.mnemonic = fmt::format("stm{} r{}{}, r{}", get_addressing_mode_string(instr), +instr.Rn, instr.W ? "!" : "", fmt::join(reg_list, ",r"));
    } else {
      instr.func_ptr = ARM::Instructions::LDM;
      instr.mnemonic = fmt::format("ldm{} r{}{}, r{}", get_addressing_mode_string(instr), +instr.Rn, instr.W ? "!" : "", fmt::join(reg_list, ",r"));
    }

    return instr;
  }

  if ((instr.opcode & 0xe000000) == 0x4000000) {
    // SPDLOG_INFO("LDR, STR [immediate]");
    instr.I = (instr.opcode & (1 << 25)) ? 1 : 0;
    instr.P = (instr.opcode & (1 << 24)) ? 1 : 0;
    instr.U = (instr.opcode & (1 << 23)) ? 1 : 0;
    instr.B = (instr.opcode & (1 << 22)) ? 1 : 0;
    instr.W = (instr.opcode & (1 << 21)) ? 1 : 0;
    instr.L = (instr.opcode & (1 << 20)) ? 1 : 0;

    instr.Rn     = (instr.opcode & 0xf0000) >> 16;
    instr.Rd     = (instr.opcode & 0xf000) >> 12;
    instr.offset = (instr.opcode & 0xfff);

    if (instr.L) {
      instr.func_ptr = ARM::Instructions::LDR;

      instr.mnemonic = fmt::format("ldr{}{} r{}, [r{}, {}#${:#06x}]", instr.B ? "b" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.U ? "+" : "-", instr.offset);
      // fmt::println("{}", instr.mnemonic);
    } else {
      instr.func_ptr = ARM::Instructions::STR;
      instr.mnemonic = fmt::format("str{}{} r{}, [r{}, {}#${:#06x}]", instr.B ? "b" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.U ? "+" : "-", instr.offset);
    }

    return instr;
  }

  // LDR, STR (Single Data Transfer)
  if ((instr.opcode & 0xe000010) == 0x6000000) {
    // SPDLOG_INFO("LDR, STR [register]");
    instr.I = (instr.opcode & (1 << 25)) ? 1 : 0;
    instr.P = (instr.opcode & (1 << 24)) ? 1 : 0;
    instr.U = (instr.opcode & (1 << 23)) ? 1 : 0;
    instr.B = (instr.opcode & (1 << 22)) ? 1 : 0;
    instr.W = (instr.opcode & (1 << 21)) ? 1 : 0;
    instr.L = (instr.opcode & (1 << 20)) ? 1 : 0;

    instr.Rn = (instr.opcode & 0xf0000) >> 16;
    instr.Rd = (instr.opcode & 0xf000) >> 12;

    instr.Rm = (instr.opcode & 0xf);

    instr.shift_amount = (instr.opcode & 0xf80) >> 7;
    instr.shift_type   = (instr.opcode & 0b1100000) >> 5;

    std::string_view shift_string = get_shift_type_string(instr.shift_type);
    assert(instr.I == 1);
    if (instr.L) {
      instr.func_ptr = ARM::Instructions::LDR;

      // LDR R3, [R2, -R1, LSL #2]!
      instr.mnemonic = fmt::format("ldr{}{} r{}, [r{}, {}r{}, {} #{}]", instr.B ? "b" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.U ? "+" : "-", +instr.Rm, shift_string,
                                   instr.shift_amount);
    } else {
      instr.func_ptr = ARM::Instructions::STR;
      instr.mnemonic = fmt::format("str{}{} r{}, [r{}, {}r{}, {} #{}]", instr.B ? "b" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.U ? "+" : "-", +instr.Rm, shift_string,
                                   instr.shift_amount);
    }

    SPDLOG_DEBUG("{}", instr.mnemonic);
    return instr;
    // assert(0);
  }

  // B (Branch)
  if ((instr.opcode & (0xe000000)) == 0xa000000) {
    // spdlog::info("read instruction {:#010X} is of branch", instr.opcode);

    instr.offset = (instr.opcode & 0xFFFFFF);
    instr.L      = (instr.opcode & 0x1000000) != 0;

    // TODO: make into function
    int x = instr.offset;

    if (x & 0x800000) { x -= 0x1000000; }
    x *= 4;

    // fmt::println("offset calced: {:#010x}", x);
    instr.offset = x;

    instr.mnemonic = fmt::format("b{}{} #{:#x}", instr.L ? "l" : "", condition_map.at(instr.condition), regs.r[15] + x + 4);

    if (instr.mnemonic.starts_with("bllt")) {  // TODO: when branch w/ link, lt
                                               // should just be t (i think)
      instr.mnemonic.replace(0, 4, "blt");
    }

    instr.func_ptr = ARM::Instructions::B;
    return instr;
  }

  if ((instr.opcode & 0xe000010) == 0) {
    // fmt::println("Data Processing (shift value is immediate)");

    u32 o_opcode = ((instr.opcode & OPCODE_MASK) >> 21);

    instr.Rd = (instr.opcode & 0xf000) >> 12;
    instr.S  = (instr.opcode & 0x100000) ? 1 : 0;
    instr.Rm = instr.opcode & 0xf;
    instr.Rn = (instr.opcode & 0xf0000) >> 16;
    instr.I  = (instr.opcode & (1 << 25)) ? 1 : 0;

    assert(instr.I == 0);

    instr.shift_type   = (instr.opcode & 0x60) >> 5;
    instr.shift_amount = (instr.opcode & 0xf80) >> 7;

    instr.SHIFT                   = true;
    std::string_view shift_string = get_shift_type_string(instr.shift_type);
    assert((instr.opcode & (1 << 4)) == 0);
    // assert(0);
    switch (o_opcode) {
      case AND: {
        instr.func_ptr = ARM::Instructions::AND;
        instr.mnemonic = fmt::format("and{}{} r{},r{},#{:#x} {}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2, shift_string);
        break;
      }
      case EOR: {
        instr.func_ptr = ARM::Instructions::EOR;
        instr.mnemonic = fmt::format("eor{}{} r{},r{}, r{} {} #{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, +instr.Rm, shift_string, instr.shift_amount);
        break;
      }
      case SUB: {
        instr.func_ptr = ARM::Instructions::SUB;
        instr.mnemonic = fmt::format("sub{}{} r{},r{}, r{} {} #{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, +instr.Rm, shift_string, instr.shift_amount);
        break;
      }
      case RSB: {
        instr.func_ptr = ARM::Instructions::RSB;
        instr.mnemonic = fmt::format("rsb{}{} r{},r{}, r{} {} #{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, +instr.Rm, shift_string, instr.shift_amount);
        break;
      }
      case ADD: {
        instr.func_ptr = ARM::Instructions::ADD;
        instr.mnemonic = fmt::format("add{}{} r{},r{}, r{} {} #{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, +instr.Rm, shift_string, instr.shift_amount);
        break;
      }
      case ADC: {
        instr.func_ptr = ARM::Instructions::ADC;
        instr.mnemonic = fmt::format("adc{}{} r{},r{}, r{} {} #{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, +instr.Rm, shift_string, instr.shift_amount);

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
        instr.func_ptr = ARM::Instructions::TEQ;
        instr.mnemonic = fmt::format("teq{} r{},r{}", condition_map.at(instr.condition), +instr.Rn, +instr.Rm);
        // throw std::runtime_error("1");
        break;
      }
      case CMP: {
        instr.func_ptr = ARM::Instructions::CMP;
        instr.mnemonic = fmt::format("cmp{} r{},r{}, r{} {} #{:#x}", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, +instr.Rm, shift_string, instr.shift_amount);

        break;
      }
      case CMN: {
        instr.func_ptr = ARM::Instructions::CMN;
        instr.mnemonic = fmt::format("cmn{} r{}, r{}", condition_map.at(instr.condition), +instr.Rn, +instr.Rm, shift_string);
        break;
      }
      case ORR: {
        instr.func_ptr = ARM::Instructions::ORR;
        instr.mnemonic = fmt::format("orr{}{} r{},r{}, r{} {} #{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, +instr.Rm, shift_string, instr.shift_amount);
        break;
      }
      case MOV: {
        instr.func_ptr = ARM::Instructions::MOV;
        instr.mnemonic = fmt::format("mov{}{} r{}, r{}, {} #{}", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rm, shift_string, instr.shift_amount);
        break;
      }
      case BIC: {
        break;
      }
      case MVN: {
        break;
      }

      default: {
        SPDLOG_ERROR("could not resolve data processing opcode: {:08b}", o_opcode);
        assert(0);
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
    SPDLOG_DEBUG("OPCODE: {:#010x}", instr.opcode);
    std::string_view shift_string = get_shift_type_string(instr.shift_type);

    // Should be impossible.
    instr.I = (instr.opcode & 0x2000000) ? 1 : 0;
    if (instr.I == 1) throw std::runtime_error("I was set in register shift");

    switch (o_opcode) {
      case AND: {
        instr.func_ptr = ARM::Instructions::AND;
        instr.mnemonic = fmt::format("and{}{} r{},r{},#{:#x} {}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2, shift_string);
        break;
      }
      case EOR: {
        break;
      }
      case SUB: {
        SPDLOG_INFO("x");
        instr.func_ptr = ARM::Instructions::SUB;
        instr.mnemonic = fmt::format("sub{}{} r{},r{},#{:#x} {}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2, shift_string);
        break;
      }
      case RSB: {
        assert(0);
        // instr.func_ptr = ARM::Instructions::RSB;
        // instr.mnemonic = fmt::format("rsb{}{} r{}, r{}, {} r{} ", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rn, shift_string, +instr.Rs);
        break;
      }
      case ADD: {
        instr.func_ptr = ARM::Instructions::ADD;
        instr.mnemonic = fmt::format("add{}{} r{}, r{}, {} r{}", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rn, shift_string, +instr.Rs);
        break;
      }
      case ADC: {
        instr.func_ptr = ARM::Instructions::ADC;
        instr.mnemonic = fmt::format("adc{}{} r{}, r{}, {} r{} ", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rn, shift_string, +instr.Rs);
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
        instr.mnemonic = fmt::format("cmp{} r{}, #{:#x} {}xxxx", condition_map.at(instr.condition), +instr.Rn, instr.op2, shift_string);
        break;
      }
      case CMN: {
        break;
      }
      case ORR: {
        instr.func_ptr = ARM::Instructions::ORR;
        instr.mnemonic = fmt::format("orr{}{} r{}, r{}, {} r{} [{}]", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rm, shift_string, +instr.Rs, shift_amount);

        break;
      }
      case MOV: {
        instr.func_ptr = ARM::Instructions::MOV;
        instr.mnemonic = fmt::format("mov{}{} r{}, r{}, {} r{} [{}]", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rm, shift_string, +instr.Rs, shift_amount);
        // fmt::println("{}", instr.mnemonic);
        // assert(0);
        break;
      }
      case BIC: {
        break;
      }
      case MVN: {
        break;
      }

      default: {
        SPDLOG_ERROR("could not resolve data processing opcode: {:08b}", o_opcode);
        assert(0);
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
    // fmt::println("Data Processing (immediate value)");

    u32 o_opcode = ((instr.opcode & OPCODE_MASK) >> 21);
    instr.Rd     = (instr.opcode & 0xf000) >> 12;
    instr.op2    = instr.opcode & 0xfff;
    instr.S      = (instr.opcode & 0x100000) ? 1 : 0;
    instr.Rm     = instr.opcode & 0xf;
    instr.Rn     = (instr.opcode & 0xf0000) >> 16;
    instr.imm    = instr.opcode & 0xff;
    instr.rotate = ((instr.opcode & 0xf00) >> 8);

    instr.I   = (instr.opcode & 0x2000000) ? 1 : 0;
    instr.op2 = std::rotr(instr.imm, instr.rotate * 2);
    // instr.print_params();
    // fmt::println("immediate value: {}, rot amount = {}", imm, rotate);

    switch (o_opcode) {
      case AND: {
        instr.func_ptr = ARM::Instructions::AND;
        instr.mnemonic = fmt::format("and{}{} r{},r{},#{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case EOR: {
        // throw std::runtime_error("EOR");
        instr.func_ptr = ARM::Instructions::EOR;
        instr.mnemonic = fmt::format("eor{}{} r{},r{},#{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case SUB: {
        // instr.print_params();
        instr.func_ptr = ARM::Instructions::SUB;
        instr.mnemonic = fmt::format("sub{}{} r{},r{},#{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case RSB: {
        instr.func_ptr = ARM::Instructions::RSB;
        instr.mnemonic = fmt::format("rsb{}{} r{},r{},#{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        // assert(0);
        break;
      }
      case ADD: {
        // ADD - Rd:= Op1 + Op2
        instr.func_ptr = ARM::Instructions::ADD;
        instr.mnemonic = fmt::format("add{}{} r{},r{},#{:#x}", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case ADC: {
        instr.func_ptr = ARM::Instructions::ADC;
        instr.mnemonic = fmt::format("adc{}{} r{},r{},#{:#x}", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case SBC: {
        instr.func_ptr = ARM::Instructions::SBC;
        instr.mnemonic = fmt::format("sbc{}{} r{},r{},#{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case RSC: {
        instr.func_ptr = ARM::Instructions::RSC;
        instr.mnemonic = fmt::format("rsc{}{} r{},r{},#{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case TST: {
        instr.func_ptr = ARM::Instructions::TST;
        instr.mnemonic = fmt::format("tst{} r{}, #{:#x}", condition_map.at(instr.condition), +instr.Rn, instr.op2);
        break;
      }
      case TEQ: {
        instr.func_ptr = ARM::Instructions::TEQ;
        instr.mnemonic = fmt::format("teq{} r{}, #{:#x}", condition_map.at(instr.condition), +instr.Rn, instr.op2);
        break;
      }
      case CMP: {
        instr.func_ptr = ARM::Instructions::CMP;
        instr.mnemonic = fmt::format("cmp{} r{}, #{:#x}", condition_map.at(instr.condition), +instr.Rn, instr.op2);
        break;
      }
      case CMN: {
        instr.func_ptr = ARM::Instructions::CMN;
        instr.mnemonic = fmt::format("cmn{} r{}, r{}, #${:#x}", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case ORR: {
        instr.func_ptr = ARM::Instructions::ORR;
        instr.mnemonic = fmt::format("orr{} r{}, r{}, #${:#x}", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case MOV: {
        instr.func_ptr = ARM::Instructions::MOV;
        instr.mnemonic = fmt::format("mov{}{} r{}, #{:#x}", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, instr.op2);
        break;
      }
      case BIC: {
        instr.func_ptr = ARM::Instructions::BIC;
        instr.mnemonic = fmt::format("bic{}{} r{},r{},#{:#x}", instr.S ? "s" : "", condition_map.at(instr.condition), +instr.Rd, +instr.Rn, instr.op2);
        break;
      }
      case MVN: {
        instr.func_ptr = ARM::Instructions::MVN;
        instr.mnemonic = fmt::format("mvn{}{} r{}, #{:#x}", condition_map.at(instr.condition), instr.S ? "s" : "", +instr.Rd, instr.op2);
        break;
      }

      default: {
        SPDLOG_ERROR("could not resolve data processing opcode: {:08b}", o_opcode);
        assert(0);
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

    // instr.mnemonic = fmt::format("swi #{:#010x}", (instr.opcode & 0xff0000) >> 16);
    // instr.func_ptr = ARM::Instructions::SWI;
    SPDLOG_ERROR("failed on {} - SWI number: {:#02x}", regs.r[0], (instr.opcode & 0xff0000) >> 16);

    assert(0);

    return instr;
  }

  SPDLOG_DEBUG("[ARM] failed to decode: {:#010x} PC: {:#010x}", instr.opcode, regs.r[15]);
  return instr;

};
#include "cpu.hpp"
#include "instructions/arm.hpp"
#include "instructions/instruction.hpp"

instruction_info ARM7TDMI::thumb_decode(instruction_info& instr) {
  if ((instr.opcode & 0xf800) == 0x1800) {  // ADD, SUB
    // assert(0);
    SPDLOG_DEBUG("ADD, SUB");
    instr.Rd = instr.opcode & 0b111;
    instr.Rs = (instr.opcode & 0b111000) >> 3;  // lhs
    instr.Rn  = (instr.opcode & 0b111000000) >> 6;  // rhs
    instr.imm = (instr.opcode & 0b111000000) >> 6;

    instr.S = true;


    u8 opcode = (instr.opcode & 0b11000000000) >> 9;

    switch (opcode) {
      case 0: {
        instr.func_ptr = ARM::Instructions::ADD;
        instr.Rm       = (instr.opcode & 0b111000000) >> 6;
        instr.Rn       = instr.Rs;  // RHS
        instr.mnemonic = fmt::format("add[2] r{}, r{}, r{}", +instr.Rd, +instr.Rm, +instr.Rn);
        // fmt::println("{}", instr.mnemonic);
        break;
      }
      case 1: {
        instr.func_ptr = ARM::Instructions::SUB;
        instr.Rm       = (instr.opcode & 0b111000000) >> 6;   // LHS
        instr.Rn       = instr.Rs;  // RHS
        instr.mnemonic = fmt::format("sub r{}, r{}, r{}", +instr.Rd, +instr.Rn, +instr.Rm);
        break;
      }
      case 2: {
        instr.func_ptr = ARM::Instructions::ADD;
        instr.I        = true;
        instr.Rn       = instr.Rs;
        instr.mnemonic = fmt::format("add r{}, r{}, #{}", +instr.Rd, +instr.Rs, instr.imm);
        break;
      }
      case 3: {
        instr.func_ptr = ARM::Instructions::SUB;
        instr.I        = true;
        instr.Rn       = instr.Rs;
        instr.mnemonic = fmt::format("sub r{}, r{}, #{}", +instr.Rd, +instr.Rs, instr.imm);
        break;
      }
      default: {
        assert(0);
      }
    }

    return instr;
    // assert(0);
  }

  if ((instr.opcode & 0xe000) == 0) {  // LSL, LSR, ASR, ROR
    u8 op = (instr.opcode & 0b0001100000000000) >> 11;

    instr.shift_amount = (instr.opcode & 0b0000011111000000) >> 6;
    instr.Rm           = (instr.opcode & 0b111000) >> 3;
    instr.Rd           = (instr.opcode & 0b111);
    instr.func_ptr     = ARM::Instructions::MOV;
    instr.S            = true;

    instr.I = 0;
    // instr.SHIFT = true;

    switch (op) {
      case 0: {
        instr.mnemonic   = fmt::format("lsl{} r{}, r{}, #{:#x}", instr.S ? "s" : "", +instr.Rd, +instr.Rm, instr.shift_amount);
        instr.shift_type = LSL;
        break;
      }
      case 1: {
        instr.mnemonic   = fmt::format("lsr{} r{}, r{}, #{:#x}", instr.S ? "s" : "", +instr.Rd, +instr.Rm, instr.shift_amount);
        instr.shift_type = LSR;
        break;
      }
      case 2: {
        instr.mnemonic   = fmt::format("asr{} r{}, r{}, #{:#x}", instr.S ? "s" : "", +instr.Rd, +instr.Rm, instr.shift_amount);
        instr.shift_type = ASR;
        break;
      }
      case 3: {
        assert(0 && "Reserved");
      }
      default: {
        assert(0);
      }
    }
    return instr;
  }

  if ((instr.opcode & 0xe000) == 0x2000) {  // MOV, CMP, ADD, SUB
    instr.offset = instr.opcode & 0xff;
    instr.Rd     = (instr.opcode >> 8) & 0b111;
    instr.Rn     = instr.Rd;
    instr.op2    = instr.offset;
    u8 op        = (instr.opcode & 0x1800) >> 11;
    instr.S      = true;
    instr.imm    = instr.offset;
    instr.I      = true;

    switch (op) {
      case 0: {  // MOV
        instr.func_ptr = ARM::Instructions::MOV;
        instr.mnemonic = fmt::format("mov r{}, #${:#x}", +instr.Rd, instr.offset);
        instr.S        = true;
        // instr.print_params();
        break;
      }
      case 1: {  // CMP
        instr.func_ptr = ARM::Instructions::CMP;
        instr.mnemonic = fmt::format("cmp r{}, #${:#x}", +instr.Rd, instr.offset);
        break;
      }
      case 2: {  // ADD
        instr.func_ptr = ARM::Instructions::ADD;
        instr.mnemonic = fmt::format("add r{}, #${:#x}", +instr.Rd, instr.offset);
        instr.S        = true;
        break;
      }
      case 3: {  // SUB
        instr.func_ptr = ARM::Instructions::SUB;
        instr.mnemonic = fmt::format("sub r{}, #${:#x}", +instr.Rd, instr.offset);
        instr.S        = true;
        break;
      }
      default: {
        SPDLOG_DEBUG("INVALID OP: {:#x}", op);
        assert(0);
      }
    }

    return instr;
  }

  if ((instr.opcode & 0xfc00) == 0x4000) {  // Data Processing
    SPDLOG_DEBUG("ALU operations");
    SPDLOG_DEBUG("OPCODE: {:#08x}", instr.opcode);

    instr.Rs = (instr.opcode & 0b111000) >> 3;
    instr.Rd = instr.opcode & 0b111;

    instr.Rn = instr.Rd;
    instr.Rm = instr.Rs;
    // instr.SHIFT = true;
    instr.S = true;
    // instr.shift_amount =

    u8 op = (instr.opcode & 0x3c0) >> 6;

    switch (op) {
      case 0x0: {
        instr.func_ptr = ARM::Instructions::AND;
        instr.mnemonic = fmt::format("and r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0x1: {
        instr.func_ptr = ARM::Instructions::EOR;
        instr.mnemonic = fmt::format("eor r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0x2: {
        instr.func_ptr                = ARM::Instructions::MOV;
        instr.shift_type              = LSL;
        instr.shift_value_is_register = true;
        instr.Rm                      = instr.Rd;
        instr.mnemonic                = fmt::format("lsl r{}, r{}", +instr.Rd, +instr.Rs);
        SPDLOG_DEBUG("Rn: {}");

        // assert(0);
        break;
      }
      case 0x3: {
        instr.func_ptr                = ARM::Instructions::MOV;
        instr.shift_type              = LSR;
        instr.shift_value_is_register = true;
        instr.Rm                      = instr.Rd;
        instr.mnemonic                = fmt::format("lsr r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0x4: {
        instr.func_ptr                = ARM::Instructions::MOV;
        instr.shift_type              = ASR;
        instr.shift_value_is_register = true;
        instr.Rm                      = instr.Rd;
        instr.mnemonic                = fmt::format("asr r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0x5: {
        instr.func_ptr = ARM::Instructions::ADC;
        instr.mnemonic = fmt::format("adc r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0x6: {
        instr.func_ptr = ARM::Instructions::SBC;
        instr.mnemonic = fmt::format("sbc r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0x7: {
        instr.func_ptr                = ARM::Instructions::MOV;
        instr.shift_type              = ROR;
        instr.shift_value_is_register = true;
        instr.Rm                      = instr.Rd;
        instr.mnemonic                = fmt::format("ror r{}, r{}", +instr.Rd, +instr.Rs);
        // assert(0);
        break;
      }
      case 0x8: {
        instr.func_ptr = ARM::Instructions::TST;
        instr.mnemonic = fmt::format("tst r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0x9: {
        instr.func_ptr = ARM::Instructions::RSB;
        instr.I        = true;
        instr.imm      = 0;
        instr.rotate   = 0;
        instr.mnemonic = fmt::format("neg r{}, r{}", +instr.Rd, +instr.Rs);
        // assert(0);
        break;
      }
      case 0xA: {
        instr.func_ptr = ARM::Instructions::CMP;
        instr.mnemonic = fmt::format("cmp r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0xB: {
        instr.func_ptr = ARM::Instructions::CMN;
        instr.mnemonic = fmt::format("cmn r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0xC: {
        instr.func_ptr = ARM::Instructions::ORR;
        instr.mnemonic = fmt::format("orr r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0xD: {
        instr.func_ptr = ARM::Instructions::MUL;
        instr.Rm       = instr.Rd;
        instr.mnemonic = fmt::format("mul r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0xE: {
        instr.func_ptr = ARM::Instructions::BIC;
        instr.mnemonic = fmt::format("bic r{}, r{}", +instr.Rd, +instr.Rs);
        break;
      }
      case 0xF: {
        instr.func_ptr = ARM::Instructions::MVN;
        instr.mnemonic = fmt::format("mvn r{}, r{}", +instr.Rd, +instr.Rs);

        break;
      }
      default: {
        assert(0);
      }
    }

    return instr;
  }

  if ((instr.opcode & 0xfc00) == 0x4400) {  // ADD, CMP, MOV (high registers)
    // SPDLOG_INFO("ADD, CMP, MOV (high registers)");
    // fmt::println("{:#x}", instr.opcode);
    // fmt::println("{:b}", instr.opcode);
    instr.Rd = ((instr.opcode & 1 << 7) ? 8 : 0) + (instr.opcode & 0b111);  // MSBd
    instr.Rs = ((instr.opcode & 1 << 6) ? 8 : 0) + ((instr.opcode & 0b111000) >> 3);
    // instr.offset = 0xFF;
    instr.S = true;

    instr.Rm = instr.Rs;

    u8 op = (instr.opcode & 0x300) >> 8;
    // fmt::println("{:#x}", op);
    switch (op) {
      case 0: {
        instr.Rn       = instr.Rd;
        instr.func_ptr = ARM::Instructions::ADD;
        instr.mnemonic = fmt::format("add r{}, r{}", +instr.Rd, +instr.Rs);
        // fmt::println("{}", instr.mnemonic);
        // assert(0);
        break;
      }
      case 1: {
        instr.func_ptr = ARM::Instructions::CMP;
        instr.mnemonic = fmt::format("cmp r{}, r{}", +instr.Rd, +instr.Rs);
        // fmt::println("{}", instr.mnemonic);
        break;
      }
      case 2: {
        instr.func_ptr = ARM::Instructions::MOV;
        // instr.Rn
        instr.mnemonic = fmt::format("mov r{}, r{}", +instr.Rd, +instr.Rs);
        // fmt::println("{}", instr.mnemonic);
        break;
      }
      case 3: {
        instr.func_ptr = ARM::Instructions::BX;
        instr.Rn       = instr.Rs;
        instr.mnemonic = fmt::format("bx r{} [{:#010x}]", +instr.Rn, regs.r[instr.Rs]);
        // fmt::println("BX");
        break;
      }
    }
    // assert(0);
    return instr;
  }

  if ((instr.opcode & 0xf000) == 0xa000) {  // get relative address
    // fmt::println("get relative address");
    u8 opcode = (instr.opcode & (1 << 11)) >> 11;
    // fmt::println("opcode:{}", opcode);
    instr.Rd = (instr.opcode & 0x700) >> 8;

    instr.op2        = (instr.opcode & 0xff) * 4;
    instr.shift_type = LSL;

    // instr.SHIFT = true;
    // instr.I     = 1;
    // instr.imm =

    switch (opcode) {
      case 0: {
        instr.func_ptr = ARM::Instructions::ADD;
        instr.Rn       = 15;
        instr.mnemonic = fmt::format("add r{}, PC, #${:#x}", +instr.Rd, instr.op2);
        // assert(0);
        break;
      }
      case 1: {
        instr.func_ptr = ARM::Instructions::ADD;
        instr.Rn       = 13;
        instr.mnemonic = fmt::format("add r{}, SP, #${:#x}", +instr.Rd, instr.op2);
        break;
      }
    }

    return instr;
  }

  if ((instr.opcode & 0xf000) == 0xd000) {  // (conditional branching)
    SPDLOG_DEBUG("Bcc (conditional branching)");
    u8 condition = (instr.opcode & 0b0000111100000000) >> 8;

    instr.offset = ((instr.opcode & 0xff));
    int x        = instr.offset;

    if (x & 0x80) { x -= 0x100; }
    x *= 2;

    instr.offset = x;

    instr.func_ptr = ARM::Instructions::B;
    switch (condition) {
      case 0x0: instr.condition = EQ; break;
      case 0x1: instr.condition = NE; break;
      case 0x2: instr.condition = CS; break;
      case 0x3: instr.condition = CC; break;
      case 0x4: instr.condition = MI; break;
      case 0x5: instr.condition = PL; break;
      case 0x6: instr.condition = VS; break;
      case 0x7: instr.condition = VC; break;
      case 0x8: instr.condition = HI; break;
      case 0x9: instr.condition = LS; break;
      case 0xA: instr.condition = GE; break;
      case 0xB: instr.condition = LT; break;
      case 0xC: instr.condition = GT; break;
      case 0xD: instr.condition = LE; break;
      case 0xE: instr.condition = AL; break;
      case 0xF: instr.condition = AL; break;
      default: assert(0);
    }
    instr.mnemonic = fmt::format("b{}{} #{:#x}", instr.L ? "l" : "", condition_map.at(instr.condition), regs.r[15] + instr.offset + 2);
    return instr;
  }

  if ((instr.opcode & 0xf800) == 0xe000) {  // unconditional branch
    SPDLOG_DEBUG("(unconditional) B");
    instr.offset = (instr.opcode & 0x7ff);

    int x = instr.offset;

    if (x & 0x400) { x -= 0x800; }

    x *= 2;

    instr.offset = x;

    instr.func_ptr = ARM::Instructions::B;
    instr.mnemonic = fmt::format("b #{:#x}", regs.r[15] + instr.offset + 2);

    return instr;
  }

  if ((instr.opcode & 0xf800) == 0x4800) {  // LDR PC relative
    SPDLOG_DEBUG("LDR (PC-relative)");
    instr.Rd          = (instr.opcode & 0x700) >> 8;
    instr.offset      = (instr.opcode & 0xFF) * 4;
    instr.Rn          = 15;
    instr.P           = 1;
    instr.U           = 1;
    instr.pc_relative = true;
    instr.func_ptr    = ARM::Instructions::LDR;

    instr.mnemonic = fmt::format("ldr r{}, [PC, #${:#08x}]", +instr.Rd, ((regs.r[15] + 4) & ~2) + instr.offset);

    return instr;
    // assert(0);
  }

  // 0101011000000000
  if ((instr.opcode & 0xf600) == 0x5200 || (instr.opcode & 0xf600) == 0x5600) {  // LDRH, STRH (register offset)
    SPDLOG_DEBUG("LDRH, STRH (register offset)");
    instr.Rd = instr.opcode & 0x7;
    instr.Rn = (instr.opcode & 0x38) >> 3;

    instr.Rm = (instr.opcode & 0x1c0) >> 6;  // offset

    instr.Rs = instr.Rm;

    u8 opcode = (instr.opcode & 0b110000000000) >> 10;
    SPDLOG_DEBUG("OPCODE: {:#x}", instr.opcode);
    SPDLOG_DEBUG("OPCODE: {:#x}", opcode);
    instr.P = 1;
    instr.I = 0;
    instr.U = 1;
    switch (opcode) {
      case 0: {
        instr.func_ptr = ARM::Instructions::STRH;
        instr.mnemonic = fmt::format("strh r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rs);
        break;
      }
      case 1: {
        instr.func_ptr = ARM::Instructions::LDRSB;
        instr.B        = true;
        instr.mnemonic = fmt::format("ldsb r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rm);
        break;
      }
      case 2: {
        instr.func_ptr = ARM::Instructions::LDRH;
        instr.mnemonic = fmt::format("ldrh r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rs);
        break;
      }
      case 3: {
        instr.func_ptr = ARM::Instructions::LDRSH;
        instr.B        = true;
        instr.mnemonic = fmt::format("ldrsh r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rs);
        break;
      }
      default: {
        SPDLOG_DEBUG("INVALID OPCODE: {}", opcode);
        assert(0);
      }
    }

    return instr;
    // SPDLOG_DEBUG("{}", instr.mnemonic);
    // assert(0);
  }

  if ((instr.opcode & 0xf600) == 0x5000 || (instr.opcode & 0xf600) == 0x5400) {  // LDR, STR (register offset)
    instr.Rd = instr.opcode & 0x7;
    instr.Rn = (instr.opcode & 0x38) >> 3;

    instr.Rm = (instr.opcode & 0x1c0) >> 6;  // offset

    instr.Rs = instr.Rm;

    u8 opcode = (instr.opcode & 0b110000000000) >> 10;
    SPDLOG_DEBUG("OPCODE: {:#x}", instr.opcode);
    SPDLOG_DEBUG("OPCODE: {:#x}", opcode);
    instr.P = 1;
    instr.I = 1;
    instr.U = 1;
    switch (opcode) {
      case 0: {
        instr.func_ptr = ARM::Instructions::STR;
        instr.mnemonic = fmt::format("str r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rs);
        break;
      }
      case 1: {
        instr.func_ptr = ARM::Instructions::STR;
        instr.B        = true;
        instr.mnemonic = fmt::format("strb r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rs);
        break;
      }
      case 2: {
        instr.func_ptr = ARM::Instructions::LDR;
        instr.mnemonic = fmt::format("ldr r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rs);
        break;
      }
      case 3: {
        instr.func_ptr = ARM::Instructions::LDR;
        instr.B        = true;
        instr.mnemonic = fmt::format("ldrb r{}, [r{}, r{}]", +instr.Rd, +instr.Rn, +instr.Rs);
        break;
      }
      default: {
        SPDLOG_DEBUG("INVALID OPCODE: {}", opcode);
        assert(0);
      }
    }

    return instr;
    // SPDLOG_DEBUG("{}", instr.mnemonic);
    // assert(0);
  }

  // 0111000000000000
  if ((instr.opcode & 0xf000) == 0x6000 || (instr.opcode & 0xf000) == 0x7000) {  // LDR, STR (immediate offset)
    // instr.mnemonic
    u8 opcode    = (instr.opcode & 0b1100000000000) >> 11;
    instr.Rd     = instr.opcode & 0b111;
    instr.Rn     = (instr.opcode & 0b111000) >> 3;
    instr.offset = (instr.opcode & 0b11111000000) >> 6;

    instr.P = 1;
    instr.U = 1;
    switch (opcode) {
      case 0: {
        instr.I = 0;
        instr.offset *= 4;
        instr.func_ptr = ARM::Instructions::STR;
        instr.mnemonic = fmt::format("str r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      case 1: {
        instr.I = 0;
        instr.offset *= 4;
        instr.func_ptr = ARM::Instructions::LDR;
        instr.mnemonic = fmt::format("ldr r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      case 2: {
        instr.I        = 0;
        instr.B        = true;
        instr.func_ptr = ARM::Instructions::STR;
        instr.mnemonic = fmt::format("strb r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      case 3: {
        instr.I        = 0;
        instr.B        = true;
        instr.func_ptr = ARM::Instructions::LDR;
        instr.mnemonic = fmt::format("ldrb r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      default: {
        assert(0);
      }
    }
    return instr;

    // SPDLOG_DEBUG("{}", instr.mnemonic);
    // assert(0);
  }

  if ((instr.opcode & 0xf000) == 0x8000) {  // LDRH, STRH (immediate offset)
    // instr.mnemonic
    u8 opcode    = (instr.opcode & (1 << 11)) >> 11;
    instr.Rd     = instr.opcode & 0b111;
    instr.Rn     = (instr.opcode & 0b111000) >> 3;
    instr.offset = ((instr.opcode & 0b11111000000) >> 6) * 2;

    instr.P = 1;
    instr.U = 1;
    switch (opcode) {
      case 0: {
        instr.I        = 1;
        instr.func_ptr = ARM::Instructions::STRH;
        instr.mnemonic = fmt::format("strh r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      case 1: {
        instr.I        = 1;
        instr.func_ptr = ARM::Instructions::LDRH;
        instr.mnemonic = fmt::format("ldrh r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      default: {
        assert(0);
      }
    }
    return instr;
  }

  // 1001000000000000
  if ((instr.opcode & 0xf000) == 0x9000) {  // LDR, STR (SP-relative)
    u8 opcode    = (instr.opcode & (1 << 11)) >> 11;
    instr.Rd     = (instr.opcode & 0b11100000000) >> 8;
    instr.offset = (instr.opcode & 0xFF) * 4;
    instr.Rn     = 13;

    instr.P = 1;
    instr.U = 1;
    switch (opcode) {
      case 0: {
        instr.I        = 0;
        instr.func_ptr = ARM::Instructions::STR;
        instr.mnemonic = fmt::format("str r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      case 1: {
        instr.I        = 0;
        instr.func_ptr = ARM::Instructions::LDR;
        instr.mnemonic = fmt::format("ldr r{}, [r{}, #{:#x}]", +instr.Rd, +instr.Rn, instr.offset);
        break;
      }
      default: {
        assert(0);
      }
    }
    return instr;

    // SPDLOG_DEBUG("{}", instr.mnemonic);
    // assert(0);
  }

  if ((instr.opcode & 0xf000) == 0xa000) {  // ADD (SP or PC) aka Load Address
    assert(0 && " ADD (SP or PC) aka Load Address");
  }

  if ((instr.opcode & 0xff00) == 0xb000) {  // ADD, SUB (SP)
    // assert(0 && "ADD, SUB (SP)");
    u8 opcode      = (instr.opcode & (1 << 7)) >> 7;
    instr.func_ptr = ARM::Instructions::ADD;
    instr.Rd       = 13;
    instr.Rn       = 13;
    instr.I        = true;

    instr.imm = (instr.opcode & 0b111111) * 4;
    instr.op2 = instr.imm;

    if (opcode == 0) {
      instr.mnemonic = fmt::format("ADD SP, #{:#x}", instr.imm);
    } else {
      instr.func_ptr = ARM::Instructions::SUB;
      instr.mnemonic = fmt::format("ADD SP, #-{:#x}", instr.imm);
    }

    return instr;
  }
  if ((instr.opcode & 0xf600) == 0xb400) {  // POP PUSH
    SPDLOG_DEBUG("PUSH, POP");

    u8 op        = (instr.opcode & (1 << 11)) >> 11;
    u8 pc_lr_bit = (instr.opcode & (1 << 8)) >> 8;
    u8 rlist     = instr.opcode & 0xff;
    instr.Rn     = 13;
    instr.W      = true;

    std::vector<u8> reg_list;

    for (size_t idx = 0; idx < 8; idx++) {
      if (rlist & (1 << idx)) { reg_list.push_back(idx); }
    }

    switch (op) {
      case 0: {  // PUSH
        instr.P = 1;
        instr.U = 0;
        if (pc_lr_bit) { instr.LR_BIT = 1; }
        instr.func_ptr = ARM::Instructions::STM;
        instr.mnemonic = fmt::format("PUSH r{}, r{}", fmt::join(reg_list, ",r"), +instr.Rn);
        break;
      }
      case 1: {  // POP
        instr.P = 0;
        instr.U = 1;
        if (pc_lr_bit) { instr.PC_BIT = 1; }
        instr.func_ptr = ARM::Instructions::LDM;
        instr.mnemonic = fmt::format("POP <r{}>, r{}", fmt::join(reg_list, ",r"), +instr.Rn);
        break;
      }
      default: assert(0);
    }

    return instr;
  }

  if ((instr.opcode & 0xf000) == 0xc000) {  // LDM, STM
    SPDLOG_DEBUG("LDM, STM");

    u8 is_ldm = (instr.opcode & (1 << 11)) ? 1 : 0;

    u16 reg_list_v = instr.opcode & 0xFF;

    instr.Rn = (instr.opcode & 0b11100000000) >> 8;

    std::vector<u8> reg_list;

    instr.W = true;

    for (size_t idx = 0; idx < 8; idx++) {
      if (reg_list_v & (1 << idx)) { reg_list.push_back(idx); }
    }

    if (is_ldm) {
      instr.P        = 0;
      instr.U        = 1;
      instr.func_ptr = ARM::Instructions::LDM;
      instr.mnemonic = fmt::format("ldm{} r{}{}, r{}", get_addressing_mode_string(instr), +instr.Rn, instr.W ? "!" : "", fmt::join(reg_list, ",r"));
    } else {
      instr.P        = 0;
      instr.U        = 1;
      instr.func_ptr = ARM::Instructions::STM;
      instr.mnemonic = fmt::format("stm{} r{}{}, r{}", get_addressing_mode_string(instr), +instr.Rn, instr.W ? "!" : "", fmt::join(reg_list, ",r"));
    }

    // assert(0);
    return instr;
  }

  if ((instr.opcode & 0xff00) == 0xdf00) {  // SWI
    SPDLOG_DEBUG("SWI");
    assert(0);
    return instr;
  }

  if ((instr.opcode & 0xf800) == 0xf000) {  // BL, BLX prefix
    SPDLOG_DEBUG("BL, BLX prefix");

    u32 lo    = (instr.opcode & 0b11111111111);
    instr.op2 = lo;

    instr.mnemonic = fmt::format("BLL #${:010x}", lo << 12);
    instr.func_ptr = ARM::Instructions::BLL;
    SPDLOG_DEBUG(instr.mnemonic);
    // assert(0);
    return instr;
  }

  if ((instr.opcode & 0xf800) == 0xf800) {  // BL suffix
    SPDLOG_DEBUG("BL suffix");

    u32 hi    = (instr.opcode & 0b11111111111);
    instr.op2 = hi;

    instr.mnemonic = fmt::format("BLH #${:08x}", hi);
    instr.func_ptr = ARM::Instructions::BLH;

    return instr;
    // assert(0);
  }

  SPDLOG_DEBUG("[THUMB] failed to decode: {:#010x} PC: {:#010x}", instr.opcode, regs.r[15]);
  assert(0);
  //
  // Implement THUMB instructions
  return instr;
}
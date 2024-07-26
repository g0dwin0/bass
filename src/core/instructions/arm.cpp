#include "instructions/arm.hpp"

void B(ARM7TDMI &c, u32 instr) {
  bool link = ((instr & 0x800000) != 0);

  if (link) { c.regs.r14 = c.regs.r15; }

  u32 offset = (instr & 0xFFFFFF);

  s32 new_offset = static_cast<s32>(offset << 2);
  spdlog::debug("[B{}] calculated offset: {}", link ? "L" : "", new_offset);

  c.regs.r15 += new_offset;
}
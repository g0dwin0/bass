#include "core/cpu.hpp"

#include <stdexcept>

#include "instructions/arm.hpp"
#include "instructions/thumb.hpp"
#include "spdlog/spdlog.h"

void ARM7TDMI::decode() {
  // Instruction instruction;

  // if ((instr & 0b10010000) == 0b10010000) {
  //   // Either a multiply/multiply long, halfword data transfer or single data
  //   // swap

  //   if ((instr & (0b11111 << 23)) == 0) {
  //     spdlog::debug("read instruction {:#010X} is a multiply", instr);
  //     return;
  //   }

  //   if ((instr & (0b00001 << 23)) == 0x800000) {
  //     spdlog::debug("read instruction {:#010X} is a multiply long", instr);
  //     return;
  //   }
  // }

  // if ((instr & (0x40090)) == 0x40090) {
  //   spdlog::debug("read instruction {:#010X} is a single data swap", instr);
  //   return;
  // }

  // if ((instr & (0x12fff1 << 4)) == 0x12fff10) {
  //   spdlog::debug("read instruction {:#010X} is of branch and exchange",
  //   instr);

  //   return;
  // }

  // if((instr & (0xa000000)) == 0xa000000) {
  //   spdlog::debug("read instruction {:#010X} is of branch", instr);

  //   u32 offset = 0xffffff;

  //   typedef void (*func_ptr)(void);

  /*
Is there even a point in decoding in the decode stage?
What if the pipeline gets cleared by an instruction in the execute stage, atp we
just wasted CPU cycles decoding an instruction we don't even execute.
*/

  return;
}


void ARM7TDMI::execute(u32 instr) {
  // Instruction instruction;

  if ((instr & 0b10010000) == 0b10010000) {
    // Either a multiply/multiply long, halfword data transfer or single data
    // swap

    if ((instr & (0b11111 << 23)) == 0) {
      spdlog::debug("read instruction {:#010X} is a multiply", instr);
      return;
    }

    if ((instr & (0b00001 << 23)) == 0x800000) {
      spdlog::debug("read instruction {:#010X} is a multiply long", instr);
      return;
    }
  }

  if ((instr & (0x40090)) == 0x40090) {
    spdlog::debug("read instruction {:#010X} is a single data swap", instr);
    return;
  }

  if ((instr & (0x12fff1 << 4)) == 0x12fff10) {
    spdlog::debug("read instruction {:#010X} is of branch and exchange", instr);

    return;
  }

  if ((instr & (0xa000000)) == 0xa000000) {
    spdlog::debug("read instruction {:#010X} is of branch", instr);

    B(*this, instr);
 
    return;
  }
}
u32 ARM7TDMI::fetch(const u32 address) { return bus->read32(address); }
void ARM7TDMI::step() {
  // move everything up, so we can fetch a new value
  if (pipeline.fetch != 0) {
    pipeline.execute = pipeline.decode;
    pipeline.decode  = pipeline.fetch;
  }

  pipeline.fetch = fetch(regs.r15);

  fmt::println("============ PIPELINE ============");
  fmt::println("Fetch:  {:#010X}", this->pipeline.fetch);
  fmt::println("Decode  Phase Instr: {:#010X}", this->pipeline.decode);
  fmt::println("Execute Phase Instr: {:#010X}", this->pipeline.execute);
  fmt::println("PC (R15): {:#010X}", this->regs.r15);
  fmt::println("==================================");

  regs.r15 += 4;

  if (this->pipeline.decode == 0) return;

  if (this->pipeline.execute == 0) return;

  execute(pipeline.execute);
}
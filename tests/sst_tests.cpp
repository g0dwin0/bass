#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../include/common/stopwatch.hpp"
#include "../include/core/agb.hpp"
#include "../lib/json/json.hpp"
#include "common/align.hpp"
#include "common/test_structs.hpp"
#include "instructions/arm.hpp"
#include "instructions/instruction.hpp"
#include "registers.hpp"
#include "spdlog/fmt/bundled/core.h"

#define DIVIDER() printf("\n============================\n\n");
using json = nlohmann::json;

static std::vector<std::string> completed            = {};
static std::vector<std::string> multiplication_tests = {"arm_mul_mla.json", "arm_mull_mlal.json"};

bool is_multiplication_test(const std::string& s) {
  for (const auto& name : multiplication_tests) {
    if (s == name) return true;
  }

  return false;
}

std::vector<std::string> list_files(const std::string& path) {
  std::vector<std::string> files;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path().filename().string());
    }
  }
  return files;
}

void compare_states(AGB* b_ptr, Registers expected, bool is_multiplication_test) {
  Registers actual = b_ptr->cpu.regs;

  for (const auto& transaction : b_ptr->bus.transactions) {
    if (transaction.kind == WRITE && !transaction.accessed) {
      fmt::println("failed write on transaction:");
      transaction.print();

      exit(-1);
      assert(-1);
    }
  }

  for (u8 i = 0; i < 16; i++) {
    if (i == 15) {
      expected.r[i] = align(expected.r[i], b_ptr->cpu.regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
      actual.r[i]   = align(actual.r[i], b_ptr->cpu.regs.CPSR.STATE_BIT == THUMB_MODE ? HALFWORD : WORD);
    }

    if (actual.r[i] != expected.r[i]) {
      fmt::println("gpr mismatch in r{}", i);
      fmt::println("expected:  {:#010X}", expected.r[i]);
      fmt::println("actual:    {:#010X}", actual.r[i]);

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }

      exit(1);
      assert(-1);
    }

    if (actual.fiq_r[i] != expected.fiq_r[i]) {
      fmt::println("fiq mismatch in r{}", i);
      fmt::println("expected:  {:#010X}", expected.fiq_r[i]);
      fmt::println("actual:    {:#010X}", actual.fiq_r[i]);

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }
      exit(1);
      assert(-1);
    }

    if (actual.svc_r[i] != expected.svc_r[i]) {
      fmt::println("svc mismatch in r{}", i);
      fmt::println("expected:  {:#010X}", expected.svc_r[i]);
      fmt::println("actual:    {:#010X}", actual.svc_r[i]);

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }
      exit(1);
      assert(-1);
    }

    if (actual.abt_r[i] != expected.abt_r[i]) {
      fmt::println("abt mismatch in r{}", i);
      fmt::println("expected:  {:#010X}", expected.abt_r[i]);
      fmt::println("actual:    {:#010X}", actual.abt_r[i]);

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }
      exit(1);
      assert(-1);
    }

    if (actual.und_r[i] != expected.und_r[i]) {
      fmt::println("und mismatch in r{}", i);
      fmt::println("expected:  {:#010X}", expected.und_r[i]);
      fmt::println("actual:    {:#010X}", actual.und_r[i]);

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }
      exit(1);
      assert(-1);
    }

    if (actual.irq_r[i] != expected.irq_r[i]) {
      fmt::println("irq mismatch in r{}", i);
      fmt::println("expected:  {:#010X}", expected.irq_r[i]);
      fmt::println("actual:    {:#010X}", actual.irq_r[i]);

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }
      exit(1);
      assert(-1);
    }
  }

  if (actual.CPSR.value != expected.CPSR.value) {
    if ((actual.CPSR.CARRY_FLAG != expected.CPSR.CARRY_FLAG) && is_multiplication_test) return;
    // if (b_ptr->cpu.pipeline.execute.mnemonic.starts_with("mul")) return;

    fmt::println("cpsr mismatch");
    fmt::println("expected: {:#010X}", expected.CPSR.value);
    fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +expected.CPSR.SIGN_FLAG, +expected.CPSR.ZERO_FLAG, +expected.CPSR.CARRY_FLAG, +expected.CPSR.OVERFLOW_FLAG,
                 expected.get_mode_string(expected.CPSR.MODE_BIT));
    fmt::println("actual: {:#010X}", actual.CPSR.value);
    fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +actual.CPSR.SIGN_FLAG, +actual.CPSR.ZERO_FLAG, +actual.CPSR.CARRY_FLAG, +actual.CPSR.OVERFLOW_FLAG,
                 actual.get_mode_string(expected.CPSR.MODE_BIT));

    for (const auto& test_file : completed) {
      fmt::println("{}", test_file);
    }
    exit(1);
    assert(-1);
  }

  if (actual.SPSR_abt != expected.SPSR_abt) {
    // if (actual.CPSR.CARRY_FLAG != expected.CPSR.CARRY_FLAG && is_multiplication_test) return;
    // if (b_ptr->cpu.pipeline.execute.mnemonic.starts_with("mul")) return;

    fmt::println("SPSR ABT mismatch");
    fmt::println("expected: {:#010X}", expected.SPSR_abt);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +expected.CPSR.SIGN_FLAG, +expected.CPSR.ZERO_FLAG, +expected.CPSR.CARRY_FLAG, +expected.CPSR.OVERFLOW_FLAG,
    //              expected.get_mode_string(expected.CPSR.MODE_BIT));
    fmt::println("actual: {:#010X}", actual.SPSR_abt);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +actual.CPSR.SIGN_FLAG, +actual.CPSR.ZERO_FLAG, +actual.CPSR.CARRY_FLAG, +actual.CPSR.OVERFLOW_FLAG,
    //              actual.get_mode_string(expected.CPSR.MODE_BIT));

    for (const auto& test_file : completed) {
      fmt::println("{}", test_file);
    }
    exit(1);
    assert(-1);
  }

  if (actual.SPSR_irq != expected.SPSR_irq) {
    // if (actual.CPSR.CARRY_FLAG != expected.CPSR.CARRY_FLAG && is_multiplication_test) return;
    // if (b_ptr->cpu.pipeline.execute.mnemonic.starts_with("mul")) return;

    fmt::println("SPSR IRQ mismatch");
    fmt::println("expected: {:#010X}", expected.SPSR_irq);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +expected.CPSR.SIGN_FLAG, +expected.CPSR.ZERO_FLAG, +expected.CPSR.CARRY_FLAG, +expected.CPSR.OVERFLOW_FLAG,
    //              expected.get_mode_string(expected.CPSR.MODE_BIT));
    fmt::println("actual: {:#010X}", actual.SPSR_irq);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +actual.CPSR.SIGN_FLAG, +actual.CPSR.ZERO_FLAG, +actual.CPSR.CARRY_FLAG, +actual.CPSR.OVERFLOW_FLAG,
    //              actual.get_mode_string(expected.CPSR.MODE_BIT));

    for (const auto& test_file : completed) {
      fmt::println("{}", test_file);
    }
    exit(1);
    assert(-1);
  }

  if (actual.SPSR_svc != expected.SPSR_svc) {
    // if (actual.CPSR.CARRY_FLAG != expected.CPSR.CARRY_FLAG && is_multiplication_test) return;
    // if (b_ptr->cpu.pipeline.execute.mnemonic.starts_with("mul")) return;

    fmt::println("SPSR SVC mismatch");
    fmt::println("expected: {:#010X}", expected.SPSR_svc);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +expected.CPSR.SIGN_FLAG, +expected.CPSR.ZERO_FLAG, +expected.CPSR.CARRY_FLAG, +expected.CPSR.OVERFLOW_FLAG,
    //              expected.get_mode_string(expected.CPSR.MODE_BIT));
    fmt::println("actual: {:#010X}", actual.SPSR_svc);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +actual.CPSR.SIGN_FLAG, +actual.CPSR.ZERO_FLAG, +actual.CPSR.CARRY_FLAG, +actual.CPSR.OVERFLOW_FLAG,
    //              actual.get_mode_string(expected.CPSR.MODE_BIT));

    for (const auto& test_file : completed) {
      fmt::println("{}", test_file);
    }
    exit(1);
    assert(-1);
  }

  if (actual.SPSR_und != expected.SPSR_und) {
    // if (actual.CPSR.CARRY_FLAG != expected.CPSR.CARRY_FLAG && is_multiplication_test) return;
    // if (b_ptr->cpu.pipeline.execute.mnemonic.starts_with("mul")) return;

    fmt::println("SPSR UND mismatch");
    fmt::println("expected: {:#010X}", expected.SPSR_und);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +expected.CPSR.SIGN_FLAG, +expected.CPSR.ZERO_FLAG, +expected.CPSR.CARRY_FLAG, +expected.CPSR.OVERFLOW_FLAG,
    //              expected.get_mode_string(expected.CPSR.MODE_BIT));
    fmt::println("actual: {:#010X}", actual.SPSR_und);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +actual.CPSR.SIGN_FLAG, +actual.CPSR.ZERO_FLAG, +actual.CPSR.CARRY_FLAG, +actual.CPSR.OVERFLOW_FLAG,
    //              actual.get_mode_string(expected.CPSR.MODE_BIT));

    for (const auto& test_file : completed) {
      fmt::println("{}", test_file);
    }
    exit(1);
    assert(-1);
  }

  if (actual.SPSR_fiq != expected.SPSR_fiq) {
    // if (actual.CPSR.CARRY_FLAG != expected.CPSR.CARRY_FLAG && is_multiplication_test) return;
    // if (b_ptr->cpu.pipeline.execute.mnemonic.starts_with("mul")) return;

    fmt::println("SPSR FIQ mismatch");
    fmt::println("expected: {:#010X}", expected.SPSR_fiq);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +expected.CPSR.SIGN_FLAG, +expected.CPSR.ZERO_FLAG, +expected.CPSR.CARRY_FLAG, +expected.CPSR.OVERFLOW_FLAG,
    //              expected.get_mode_string(expected.CPSR.MODE_BIT));
    fmt::println("actual: {:#010X}", actual.SPSR_fiq);
    // fmt::println("N: {} Z: {} C: {} V: {} - mode: {}", +actual.CPSR.SIGN_FLAG, +actual.CPSR.ZERO_FLAG, +actual.CPSR.CARRY_FLAG, +actual.CPSR.OVERFLOW_FLAG,
    //              actual.get_mode_string(expected.CPSR.MODE_BIT));

    for (const auto& test_file : completed) {
      fmt::println("{}", test_file);
    }
    exit(1);
    assert(-1);
  }

  for (const auto& transaction : b_ptr->bus.transactions) {
    if (transaction.kind == WRITE && !transaction.accessed) {
      fmt::println("failed write on transaction:");
      transaction.print();

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }

      exit(-1);
      assert(-1);
    }
  }
  for (const auto& transaction : b_ptr->bus.transactions) {
    if (transaction.kind == GENERAL_READ && !transaction.accessed) {
      fmt::println("failed general read on transaction:");
      transaction.print();

      for (const auto& test_file : completed) {
        fmt::println("{}", test_file);
      }

      exit(-1);
      assert(-1);
    }
  }
}
Registers get_result_state(auto& initial_json) {
  Registers regs = {};

  for (u8 r = 0; r < 16; r++) {
    regs.r[r] = initial_json["final"]["R"][r];
    // fmt::println("FINAL R{}: {:#010X}", r, regs.r[r], regs.r[r]);
  }
  // DIVIDER();
  // Load FIQ banked regs (8 -> 14)
  for (u8 i = 0, r = 8; r < 15; r++, i++) {
    regs.fiq_r[r] = initial_json["final"]["R_fiq"][i];
    // fmt::println("FINAL R_fiq{}: {:#010X}", r, regs.fiq_r[r]);
  }

  // DIVIDER();
  // Load SVC banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    regs.svc_r[r] = initial_json["final"]["R_svc"][i];
    // fmt::println("FINAL R_svc{}: {:#010X}", r, regs.svc_r[r]);
  }

  // DIVIDER();
  // Load ABORT banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    regs.abt_r[r] = initial_json["final"]["R_abt"][i];
    // fmt::println("FINAL R_abt{}: {:#010X}", r, regs.abt_r[r]);
  }

  // DIVIDER();
  // Load IRQ banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    regs.irq_r[r] = initial_json["final"]["R_irq"][i];
    // fmt::println("FINAL R_irq{}: {:#010X}", r, regs.irq_r[r]);
  }

  // DIVIDER();
  // Load UND banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    regs.und_r[r] = initial_json["final"]["R_und"][i];
    // fmt::println("FINAL R_und{}: {:#010X}", r, regs.und_r[r]);
  }

  // DIVIDER();

  regs.CPSR.value = initial_json["final"]["CPSR"];
  regs.SPSR_fiq   = initial_json["final"]["SPSR"][0];
  regs.SPSR_svc   = initial_json["final"]["SPSR"][1];
  regs.SPSR_abt   = initial_json["final"]["SPSR"][2];
  regs.SPSR_irq   = initial_json["final"]["SPSR"][3];
  regs.SPSR_und   = initial_json["final"]["SPSR"][4];
  // regs.pipeline.f = initial_json["final"]["pipeline"][0];
  regs.pipeline.d = initial_json["final"]["pipeline"][0];
  regs.pipeline.e = initial_json["final"]["pipeline"][1];
  

  fmt::println("FINAL CPSR: {:#010X}", regs.CPSR.value);

  fmt::println("N: {} Z: {} C: {} V: {} - mode: {} - sb: {}", +regs.CPSR.SIGN_FLAG, +regs.CPSR.ZERO_FLAG, +regs.CPSR.CARRY_FLAG, +regs.CPSR.OVERFLOW_FLAG, regs.get_mode_string(regs.CPSR.MODE_BIT),
               regs.CPSR.STATE_BIT ? "THUMB" : "ARM");
  fmt::println("FINAL SPSR_fiq: {:#010X}", regs.SPSR_fiq);
  fmt::println("FINAL SPSR_svc: {:#010X}", regs.SPSR_svc);
  fmt::println("FINAL SPSR_abt: {:#010X}", regs.SPSR_abt);
  fmt::println("FINAL SPSR_irq: {:#010X}", regs.SPSR_irq);
  fmt::println("FINAL SPSR_und: {:#010X}", regs.SPSR_und);

  return regs;
}

void get_cpu_from_state(const json& initial_json, AGB* bass) {
  // Load GPR
  for (u8 r = 0; r < 16; r++) {
    bass->cpu.regs.r[r] = initial_json["initial"]["R"][r];
    // fmt::println("INITIAL R{}: {:#010X}", r, bass->cpu.regs.r[r]);
  }
  // DIVIDER();
  // Load FIQ banked regs (8 -> 14)
  for (u8 i = 0, r = 8; r < 15; r++, i++) {
    bass->cpu.regs.fiq_r[r] = initial_json["initial"]["R_fiq"][i];
    // fmt::println("INITIAL R_fiq{}: {:#010x}", r, bass->cpu.regs.fiq_r[r]);
  }

  // DIVIDER();
  // Load SVC banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    bass->cpu.regs.svc_r[r] = initial_json["initial"]["R_svc"][i];
    // fmt::println("INITIAL R_svc{}: {:#010x}", r, bass->cpu.regs.svc_r[r]);
  }

  // DIVIDER();
  // Load ABORT banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    bass->cpu.regs.abt_r[r] = initial_json["initial"]["R_abt"][i];
    // fmt::println("INITIAL R_abt{}: {:#010x}", r, bass->cpu.regs.abt_r[r]);
  }

  // DIVIDER();
  // Load IRQ banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    bass->cpu.regs.irq_r[r] = initial_json["initial"]["R_irq"][i];
    // fmt::println("INITIAL R_irq{}: {:#010x}", r, bass->cpu.regs.irq_r[r]);
  }

  // DIVIDER();
  // Load UND banked regs (13 -> 14)
  for (u8 i = 0, r = 13; r < 15; i++, r++) {
    bass->cpu.regs.und_r[r] = initial_json["initial"]["R_und"][i];
    // fmt::println("INITIAL R_und{}: {:#010x}", r, bass->cpu.regs.und_r[r]);
  }

  // DIVIDER();

  bass->cpu.regs.CPSR.value = initial_json["initial"]["CPSR"];
  bass->cpu.regs.SPSR_fiq   = initial_json["initial"]["SPSR"][0];
  bass->cpu.regs.SPSR_svc   = initial_json["initial"]["SPSR"][1];
  bass->cpu.regs.SPSR_abt   = initial_json["initial"]["SPSR"][2];
  bass->cpu.regs.SPSR_irq   = initial_json["initial"]["SPSR"][3];
  bass->cpu.regs.SPSR_und   = initial_json["initial"]["SPSR"][4];
  fmt::println("INITIAL CPSR: {:#010X}", bass->cpu.regs.CPSR.value);
  fmt::println("N: {} Z: {} C: {} V: {} - mode: {} - sb: {}", +bass->cpu.regs.CPSR.SIGN_FLAG, +bass->cpu.regs.CPSR.ZERO_FLAG, +bass->cpu.regs.CPSR.CARRY_FLAG, +bass->cpu.regs.CPSR.OVERFLOW_FLAG,
               bass->cpu.regs.get_mode_string(bass->cpu.regs.CPSR.MODE_BIT), bass->cpu.regs.CPSR.STATE_BIT ? "THUMB" : "ARM");
  fmt::println("INITIAL SPSR_fiq: {:#010X}", bass->cpu.regs.SPSR_fiq);
  fmt::println("INITIAL SPSR_svc: {:#010X}", bass->cpu.regs.SPSR_svc);
  fmt::println("INITIAL SPSR_abt: {:#010X}", bass->cpu.regs.SPSR_abt);
  fmt::println("INITIAL SPSR_irq: {:#010X}", bass->cpu.regs.SPSR_irq);
  fmt::println("INITIAL SPSR_und: {:#010X}", bass->cpu.regs.SPSR_und);

  bass->cpu.pipeline.decode  = initial_json["initial"]["pipeline"][0];
  bass->cpu.pipeline.execute = initial_json["initial"]["pipeline"][1];

  bass->bus.transactions.clear();

  for (u8 i = 0; i < initial_json["transactions"].size(); i++) {
    u32 kind, size, addr, data, opcode, base_addr = 0;
    kind          = initial_json["transactions"][i]["kind"];
    size          = initial_json["transactions"][i]["size"];
    addr          = initial_json["transactions"][i]["addr"];
    data          = initial_json["transactions"][i]["data"];
    opcode        = initial_json["opcode"];
    base_addr     = initial_json["base_addr"];
    Transaction t = {};
    t.id          = i;
    t.kind        = (ACCESS_TYPE)kind;
    t.size        = (u8)size;
    t.addr        = addr;
    t.data        = data;
    t.opcode      = opcode;
    t.base_addr   = base_addr;

    t.print();
    bass->bus.transactions.push_back(t);
  }
}

void run_tests() {
  // Load tests, then put them into a struct.
  std::string directory_path = "tests/v1/";
  auto test_files            = list_files(directory_path);

  if (test_files.empty()) {
    fmt::println("No tests loaded.");
    return;
  }

  AGB bass = {};

  for (const auto& test_file : test_files) {
    printf("Test file: %s\n", test_file.c_str());

    std::ifstream f(directory_path + test_file);
    json data = json::parse(f);

    for (size_t idx = 0; idx < data.size(); idx++) {
      fmt::println("Test: {}/{}", idx + 1, data.size());

      DIVIDER();

      bass.cpu.pipeline.decode  = {};
      bass.cpu.pipeline.execute = {};

      get_cpu_from_state(data[idx], &bass);

      // construct result struct

      auto result_state = get_result_state(data[idx]);

      // // Run test
      // fmt::println("{:#08X}", bass.cpu.pipeline.execute.opcode);
      // bass.bus.read8 = 0;
      bool is_mul = ((bass.cpu.pipeline.decode & 0b111111'1111'000000) == 0b010000'1101'000000) && bass.cpu.regs.CPSR.STATE_BIT == THUMB_MODE;

      is_mul |= is_multiplication_test(test_file);
      // fmt::println("is mul? : {}", is_mul);

      // fmt::println("{}", (bass.cpu.pipeline.decode & 0b111111'1111'000000) == 0b010000'1101'000000);
      // fmt::println("{:#010B}", bass.cpu.pipeline.decode);
      // fmt::println("{}", bass.cpu.regs.CPSR.STATE_BIT == THUMB_MODE);

      bass.cpu.step();
      fmt::println("{}", test_file);
      // fmt::println("========= instruction params  =========");
      // bass.cpu.pipeline.execute.print_params();
      // fmt::println("========================================");

      fmt::println("M: {}", bass.cpu.regs.get_mode_string(bass.cpu.regs.CPSR.MODE_BIT));
      compare_states(&bass, result_state, is_mul);
    }
    fmt::println("Completed tests. all good.");
    completed.push_back(test_file);
  }
};

int main() {
  run_tests();
  for (const auto& test_file : completed) {
    fmt::println("{}", test_file);
  }
  return 0;
}

#pragma once

#include <string_view>
#include <unordered_map>

#include "common.hpp"
#include "spdlog/spdlog.h"

enum BANK_MODE : u8 { USER = 0x10, FIQ = 0x11, IRQ = 0x12, SUPERVISOR = 0x13, ABORT = 0x17, UNDEFINED = 0x1B, SYSTEM = 0x1F };

enum Condition : u8 { EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL, NV };

inline const std::unordered_map<Condition, std::string_view> condition_map = {
    {EQ, "eq"},
    {NE, "ne"},
    {CS, "cs"},
    {CC, "cc"},
    {MI, "mi"},
    {PL, "pl"},
    {VS, "vs"},
    {VC, "vc"},
    {HI, "hi"},
    {LS, "ls"},
    {GE, "ge"},
    {LT, "lt"},
    {GT, "gt"},
    {LE, "le"},
    {AL,   ""},
    {NV, "nv"},
};
inline const std::unordered_map<BANK_MODE, std::string_view> mode_map = {
    {      USER,             "USER"},
    {       FIQ,              "FIQ"},
    {       IRQ,              "IRQ"},
    {SUPERVISOR, "SUPERVISOR (SWI)"},
    {     ABORT,            "ABORT"},
    { UNDEFINED,        "UNDEFINED"},
    {    SYSTEM,           "SYSTEM"}
};
enum CPU_MODE : u8 { ARM_MODE, THUMB_MODE };
struct Registers {
  Registers() {
    CPSR.MODE_BITS = SYSTEM;
    CPSR.STATE_BIT = ARM_MODE;
    r[14]      = 0x08000000;

    // initialize stack pointers
    r[13]     = 0x03007F00;
    irq_r[13] = 0x03007FA0;
    svc_r[13] = 0x03007FE0;
  }

  void copy(BANK_MODE);
  void load_spsr_to_cpsr();
  u32 get_spsr(BANK_MODE m);
  struct {
    // R13 (SP)
    // R14 (LR)
    // R15 (PC)

    u32 r[16] = {};

    u32 fiq_r[16]            = {};
    u32 svc_r[16]            = {};
    u32 abt_r[16]            = {};
    u32 und_r[16]            = {};
    u32 irq_r[16]            = {};
    u32 user_system_bank[16] = {};

    u32 SPSR_fiq = 0;
    u32 SPSR_svc = 0;
    u32 SPSR_abt = 0;
    u32 SPSR_irq = 0;
    u32 SPSR_und = 0;

    union {
      u32 value;
      struct {
        BANK_MODE MODE_BITS : 5;
        CPU_MODE STATE_BIT    : 1;
        u8 FIQ_DISABLE        : 1;
        u8 IRQ_DISABLE        : 1;
        u8 ABORT_DISABLE      : 1;
        u8 ENDIAN             : 1;
        u32                   : 14;
        u8 JAZELLE_MODE       : 1;
        u8                    : 2;
        u8 STICKY_OVERFLOW    : 1;
        bool OVERFLOW_FLAG    : 1;
        bool CARRY_FLAG       : 1;
        bool ZERO_FLAG        : 1;
        bool SIGN_FLAG        : 1;
      };
    } CPSR;
  };
};

#pragma once

#include <unordered_map>

#include "common.hpp"

enum REGISTER_MODE { SYSTEM_USER, FIQ, SUPERVISOR, ABORT, IRQ, UNDEFINED };

enum Condition : u8 {
  EQ,
  NE,
  CS_HS,
  CC_LO,
  MI,
  PL,
  VS,
  VC,
  HI,
  LS,
  GE,
  LT,
  GT,
  LE,
  AL,
  NV
};

inline const std::unordered_map<Condition, std::string_view> condition_map = {
    {   EQ,    "eq"},
    {   NE,    "ne"},
    {CS_HS, "cs_hs"},
    {CC_LO, "cc_lo"},
    {   MI,    "mi"},
    {   PL,    "pl"},
    {   VS,    "vs"},
    {   VC,    "vc"},
    {   HI,    "hi"},
    {   LS,    "ls"},
    {   GE,    "ge"},
    {   LT,    "lt"},
    {   GT,    "gt"},
    {   LE,    "le"},
    {   AL,      ""},
    {   NV,    "nv"},
};
inline const std::unordered_map<u8, std::string_view> mode_map = {
    { 0x0,         "OLD USER"},
    { 0x1,          "OLD FIQ"},
    { 0x2,          "OLD IRQ"},
    { 0x3,   "OLD SUPERVISOR"},
    {0x10,             "USER"},
    {0x11,              "FIQ"},
    {0x12,              "IRQ"},
    {0x13, "SUPERVISOR (SWI)"},
    {0x17,            "ABORT"},
    {0x1B,        "UNDEFINED"},
    {0x1F,           "SYSTEM"}
};

struct Registers {
  Registers() {
    CPSR.value = 0x00000003;
    r[13]      = 0x03007f00;
  }

  struct {
    // R13 (SP)
    // R14 (LR)
    // R15 (PC)

    u32 r[16] = {};

    u32 r8_fiq;
    u32 r9_fiq;
    u32 r10_fiq;
    u32 r11_fiq;
    u32 r12_fiq;
    u32 r13_fiq;
    u32 r14_fiq;

    u32 r13_svc;
    u32 r14_svc;

    u32 r13_abt;
    u32 r14_abt;

    u32 r13_irq;
    u32 r14_irq;

    u32 r13_und;
    u32 r14_und;

    u32 SPSR_fiq;
    u32 SPSR_svc;
    u32 SPSR_abt;
    u32 SPSR_irq;
    u32 SPSR_und;

    union {
      u32 value;
      struct {
        u8 MODE_BITS       : 5;
        // Designates if CPU is in THUMB mode (1) or ARM mode (0)
        u8 STATE_BIT       : 1;
        u8 FIQ_DISABLE     : 1;
        u8 IRQ_DISABLE     : 1;
        u8 ABORT_DISABLE   : 1;
        u8 ENDIAN          : 1;
        u32                : 14;
        u8 JAZELLE_MODE    : 1;
        u8                 : 2;
        u8 STICKY_OVERFLOW : 1;
        bool OVERFLOW_FLAG : 1;
        bool CARRY_FLAG    : 1;
        bool ZERO_FLAG     : 1;
        bool SIGN_FLAG     : 1;
      };
    } CPSR;
  };
};

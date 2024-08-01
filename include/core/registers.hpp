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

inline std::unordered_map<Condition, std::string> condition_map = {
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
    {   AL,    ""},
    {   NV,    "nv"},
};

struct Registers {
  struct {
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
        u8 STATE_BIT       : 1;
        u8 FIQ_DISABLE     : 1;
        u8 IRQ_DISABLE     : 1;
        u8 ABORT_DISABLE   : 1;
        u8 ENDIAN          : 1;
        u32                : 14;
        u8 JAZELLE_MODE    : 1;
        u8                 : 2;
        u8 STICKY_OVERFLOW : 1;
        u8 OVERFLOW_FLAG   : 1;
        u8 CARRY_FLAG      : 1;
        u8 ZERO_FLAG       : 1;
        u8 SIGN_FLAG       : 1;
      };
    } CPSR;
  };
};

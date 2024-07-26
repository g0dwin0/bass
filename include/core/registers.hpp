#pragma once

#include "common.hpp"
enum CPU_MODE {
	ARM,
	THUMB
};

enum REGISTER_MODE {
	SYSTEM_USER,
	FIQ,
	SUPERVISOR,
	ABORT,
	IRQ,
	UNDEFINED
};

enum CONDITION {
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

struct Registers {
	struct {
		u32 r0;
		u32 r1;
		u32 r2;
		u32 r3;
		u32 r4;
		u32 r5;
		u32 r6;
		u32 r7;
		u32 r8;
		u32 r9;
		u32 r10;
		u32 r11;
		u32 r12;
		u32 r13; // SP
		u32 r14; // LR
		u32 r15 = 0x08000000; // PC

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
				u8 MODE_BITS : 5;
				u8 STATE_BIT : 1;
				u8 FIQ_DISABLE : 1;
				u8 IRQ_DISABLE : 1;
				u8 ABORT_DISABLE : 1;
				u8 ENDIAN : 1;
				u32 : 14;
				u8 JAZELLE_MODE : 1;
				u8 : 2;
				u8 STICKY_OVERFLOW : 1;
				u8 OVERFLOW_FLAG : 1;
				u8 CARRY_FLAG: 1;
				u8 ZERO_FLAG : 1;
				u8 SIGN_FLAG: 1;
			};
		} CPSR;
	} ;
};


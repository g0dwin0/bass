#pragma once

#include "common/defs.hpp"
enum ALIGN_TO { HALFWORD, WORD };

enum PRESCALER_SEL : u8 { F_1, F_64, F_256, F_1024 };

enum class INTERRUPT_TYPE : u8 {
  LCD_VBLANK       = 0,
  LCD_HBLANK       = 1,
  LCD_VCOUNT_MATCH = 2,
  TIMER0_OVERFLOW  = 3,
  TIMER1_OVERFLOW  = 4,
  TIMER2_OVERFLOW  = 5,
  TIMER3_OVERFLOW  = 6,
  SERIAL_COMM      = 7,
  DMA0             = 8,
  DMA1             = 9,
  DMA2             = 10,
  DMA3             = 11,
  KEYPAD           = 12,
  GAMEPAK_EX       = 13
};

enum CartridgeType { EEPROM, SRAM, FLASH, FLASH512, FLASH1M, UNKNOWN };
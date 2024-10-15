#pragma once

#include <unordered_map>

#include "common.hpp"
#include "pak.hpp"
struct Bus;
struct PPU;
#include "dma.hpp"
#include "ppu.hpp"

struct Bus {
  enum InterruptType { LCD_VBLANK, LCD_HBLANK, LCD_VCOUNT_MATCH, TIMER0_OVERFLOW, TIMER1_OVERFLOW, TIMER2_OVERFLOW, TIMER3_OVERFLOW, SERIAL_COMM, DMA0, DMA1, DMA2, DMA3, KEYPAD, GAMEPAK_EXT };
  // memory/devices
  std::vector<u8> BIOS;
  std::vector<u8> IWRAM;
  std::vector<u8> EWRAM;
  std::vector<u8> VRAM;
  std::vector<u8> PALETTE_RAM;
  std::vector<u8> OAM;
  std::vector<u8> SRAM;



  Bus() : BIOS(0x4000), IWRAM(0x8000), EWRAM(0x40000), VRAM(0x18000), PALETTE_RAM(0x400), OAM(0x400), SRAM(0x10000) { BIOS = read_file("roms/gba_bios.bin"); }

  Pak* pak = nullptr;
  PPU* ppu = nullptr;
  // DMA_Controller* dma = nullptr;

  unsigned long long fun_value      = 0;
  unsigned long long cycles_elapsed = 0;

  [[nodiscard]] u8 read8(u32 address);
  [[nodiscard]] u16 read16(u32 address);
  [[nodiscard]] u32 read32(u32 address);

  void request_interrupt(InterruptType type);
  void handle_interrupts();

  void write8(u32 address, u8 value);
  void write16(u32 address, u16 value);
  void write32(u32 address, u32 value);

  inline void transfer16(const u32 src, const u32 dst, u32 word_count) {
    for (size_t idx = 0; idx < word_count; idx++) {
      fmt::println("copying to {:#010x}", dst + (idx * 2));
      write16(dst + (idx * 2), read16(src + (idx * 2)));
    }
    fmt::println("dma copied successfully");
  }
  inline void transfer32(const u32 src, const u32 dst, u32 word_count) {
    for (size_t idx = 0; idx < word_count; idx++) {
      fmt::println("copying to {:#010x}", dst + (idx * 4));
      write32(dst + (idx * 4), read32(src + (idx * 4)));
    }
    fmt::println("dma copied successfully");
  }

  [[nodiscard]] u8 io_read(u32 address);
  void io_write(u32 address, u8 value);

  struct {
    union {
      u16 v = 0;
      struct {
        u8 BG_MODE                 : 3;
        u8                         : 1;
        u8 DISPLAY_FRAME_SELECT    : 1;
        u8 HBLANK_INTERVAL_FREE    : 1;
        u8 OBJ_CHAR_VRAM_MAPPING   : 1;
        u8 FORCED_BLANK            : 1;
        u8 SCREEN_DISPLAY_BG0      : 1;
        u8 SCREEN_DISPLAY_BG1      : 1;
        u8 SCREEN_DISPLAY_BG2      : 1;
        u8 SCREEN_DISPLAY_BG3      : 1;
        u8 SCREEN_DISPLAY_OBJ      : 1;
        u8 WINDOW_0_DISPLAY_FLAG   : 1;
        u8 WINDOW_1_DISPLAY_FLAG   : 1;
        u8 OBJ_WINDOW_DISPLAY_FLAG : 1;
      };
    } DISPCNT;

    union {
      u16 v = 0;
      struct {
        u8 GREEN_SWAP : 1;
        u16           : 15;
      };
    } GREEN_SWAP;

    union {
      u16 v = 0;
      struct {
        u8 VBLANK_FLAG          : 1;
        u8 HBLANK_FLAG          : 1;
        u8 V_COUNTER_FLAG       : 1;
        u8 VBLANK_IRQ_ENABLE    : 1;
        u8 HBLANK_IRQ_ENABLE    : 1;
        u8 V_COUNTER_IRQ_ENABLE : 1;
        u8                      : 2;
        u8 VCOUNT_SETTING;
      };
    } DISPSTAT;

    union {
      u16 v = 0;

      struct {
        u8 LY;
        u8 : 8;
      };
    } VCOUNT;

    union {
      u16 v = 0;

      struct {
        u8 BG_PRIORITY       : 2;
        u8 CHAR_BASE_BLOCK   : 2;
        u8                   : 2;
        u8 MOSAIC            : 1;
        u8 COLOR_MODE        : 1;  // 0 = 4bpp (16 colors) 1 = 8bpp (256 colors)
        u8 SCREEN_BASE_BLOCK : 5;
        u8 BG_WRAP           : 1;
        u8 SCREEN_SIZE       : 2;
      };
    } BG0CNT, BG1CNT;

    union {
      u16 v = 0;

      struct {
        u8 BG_PRIORITY           : 2;
        u8 CHAR_BASE_BLOCK       : 2;
        u8                       : 2;
        u8 MOSAIC                : 1;
        u8 COLORS_PALETTES       : 1;
        u8 SCREEN_BASE_BLOCK     : 5;
        u8 DISPLAY_AREA_OVERFLOW : 1;
        u8 SCREEN_SIZE           : 2;
      };
    } BG2CNT, BG3CNT;

    union {
      u16 v = 0;

      struct {
        u16 OFFSET : 9;
        u8         : 7;
      };
    } BG0HOFS, BG0VOFS, BG1HOFS, BG1VOFS, BG2HOFS, BG2VOFS, BG3HOFS, BG3VOFS;

    union {
      u16 v = 0;

      struct {
        u8 FRACTIONAL_PORT;
        u32 INTEGER_PORT : 7;
        u8 SIGN          : 1;
      };
    } BG2PA, BG2PB, BG2PC, BG2PD;

    union {
      u32 v = 0;
      struct {
        u8 FRACTIONAL_PORT;
        u32 INTEGER_PORT : 19;
        u8 SIGN          : 1;
        u8               : 4;
      };
    } BG2X_L, BG2X_H, BG2Y_L, BG2Y_H;

    union {
      u16 v = 0;
      struct {
        u8 FRACTIONAL_PORT;
        u32 INTEGER_PORT : 7;
        u8 SIGN          : 1;
      };
    } BG3PA, BG3PB, BG3PC, BG3PD;

    union {
      u32 v = 0;
      struct {
        u8 FRACTIONAL_PORT;
        u32 INTEGER_PORT : 19;
        u8 SIGN          : 1;
        u8               : 4;
      };
    } BG3X_L, BG23_H, BG3Y_L, BG3Y_H;

    union {
      u32 v = 0;
      struct {
        u8 X2;
        u8 X1;
      };
    } WIN0H, WIN1H;

    union {
      u32 v = 0;
      struct {
        u8 Y2;
        u8 Y1;
      };
    } WIN0V, WIN1V;

    union {
      u16 v;
      struct {
        u8 WIN0_BG_ENABLE_BITS   : 4;
        u8 WIN0_OBJ_ENABLE_BIT   : 1;
        u8 WIN0_COLOR_SPECIAL_FX : 1;
        u8 RESERVED              : 2;
        u8 WIN1_BG_ENABLE_BITS   : 4;
        u8 WIN1_OBJ_ENABLE_BIT   : 1;
        u8 WIN1_COLOR_SPECIAL_FX : 1;
        u8                       : 2;
      };
    } WININ;

    union {
      u16 v = 0;
      struct {
        u8 OUTSIDE_BG_ENABLE_BITS      : 4;
        u8 OUTSIDE_OBJ_ENABLE_BIT      : 1;
        u8 OUTSIDE_COLOR_SPECIAL_FX    : 1;
        u8 RESERVED                    : 2;
        u8 OBJ_WINDOW_BG_ENABLE_BITS   : 4;
        u8 OBJ_WINDOW_OBJ_ENABLE_BIT   : 1;
        u8 OBJ_WINDOW_COLOR_SPECIAL_FX : 1;
        u8                             : 2;
      };
    } WINOUT;

    union {
      u32 v = 0;
      struct {
        u8 BG_MOSAIC_H_SIZE  : 4;
        u8 BG_MOSAIC_V_SIZE  : 4;
        u8 OBJ_MOSAIC_H_SIZE : 4;
        u8 OBJ_MOSAIC_V_SIZE : 4;
        u16 RESERVED;
      };
    } MOSAIC;

    u16 RESERVED : 16 = 0;

    union {
      u16 v = 0;
      struct {
        u8 BG0_1ST_TARGET_PIXEL : 1;
        u8 BG1_1ST_TARGET_PIXEL : 1;
        u8 BG2_1ST_TARGET_PIXEL : 1;
        u8 BG3_1ST_TARGET_PIXEL : 1;
        u8 OBJ_1ST_TARGET_PIXEL : 1;
        u8 BD_1ST_TARGET_PIXEL  : 1;
        u8 COLOR_SPECIAL_FX     : 2;

        u8 BG0_2ND_TARGET_PIXEL : 1;
        u8 BG1_2ND_TARGET_PIXEL : 1;
        u8 BG2_2ND_TARGET_PIXEL : 1;
        u8 BG3_2ND_TARGET_PIXEL : 1;
        u8 OBJ_2ND_TARGET_PIXEL : 1;
        u8 BD_2ND_TARGET_PIXEL  : 1;

        u8 RESERVED             : 2;
      };
    } BLDCNT;

    union {
      u16 v = 0;
      struct {
        u8 EVA_COEFFICIENT_FIRST_TARGET : 5;
        u8 OBJ_MOSAIC_H_SIZE            : 3;
        u8 OBJ_MOSAIC_V_SIZE            : 5;
        u8 RESERVED                     : 3;
      };
    } BLDALPHA;

    union {
      u32 v = 0;
      struct {
        u8 EVY_COEFFICIENT : 5;
        u32 RESERVED       : 27;
      };
    } BLDY;

    // u8 RESERVED[218];
  } display_fields = {};

  struct {
    union {
      u32 v = 0;
      struct {
        u8 enabled : 1;
        u32        : 31;
      };
    } IME;

    union {
      u16 v = 0;
      struct {
        u8 LCD_VBLANK          : 1;
        u8 LCD_HBLANK          : 1;
        u8 LCD_V_COUNTER_MATCH : 1;
        u8 TIMER0_OVERFLOW     : 1;
        u8 TIMER1_OVERFLOW     : 1;
        u8 TIMER2_OVERFLOW     : 1;
        u8 TIMER3_OVERFLOW     : 1;
        u8 SERIAL_COM          : 1;
        u8 DMA0                : 1;
        u8 DMA1                : 1;
        u8 DMA2                : 1;
        u8 DMA3                : 1;
        u8 KEYPAD              : 1;
        u8 GAMEPAK             : 1;

        u32                    : 2;
      };
    } IE;

    union {
      u16 v = 0;
      struct {
        u8 LCD_VBLANK          : 1;
        u8 LCD_HBLANK          : 1;
        u8 LCD_V_COUNTER_MATCH : 1;
        u8 TIMER0_OVERFLOW     : 1;
        u8 TIMER1_OVERFLOW     : 1;
        u8 TIMER2_OVERFLOW     : 1;
        u8 TIMER3_OVERFLOW     : 1;
        u8 SERIAL_COM          : 1;
        u8 DMA0                : 1;
        u8 DMA1                : 1;
        u8 DMA2                : 1;
        u8 DMA3                : 1;
        u8 KEYPAD              : 1;
        u8 GAMEPAK             : 1;

        u32                    : 2;
      };
    } IF;
  } interrupt_control = {};

  struct {
    union {
      u32 v = 0;
      struct {
        u8 SRAM_WAIT_CONTROL       : 2;
        u8 WS0_FIRST_ACCESS        : 2;
        u8 WS0_SECOND_ACCESS       : 1;
        u8 WS1_FIRST_ACCESS        : 2;
        u8 WS1_SECOND_ACCESS       : 1;
        u8 WS2_FIRST_ACCESS        : 2;
        u8 WS2_SECOND_ACCESS       : 1;
        u8 PHI_TERMINAL_OUTPUT     : 2;
        u8                         : 1;
        u8 GAMEPAK_PREFETCH_BUFFER : 1;
        u8 GAMEPAK_TYPE_FLAG       : 1;
        u16                        : 16;
      };
    } WAITCNT;
    u32 sound_bias = 0;

  } system_control = {};

  struct {
    union {
      u32 v = 0;
      struct {
        u32 src : 28;
        u8      : 4;
      };
    } DMA0SAD, DMA1SAD, DMA2SAD, DMA3SAD;

    union {
      u32 v = 0;
      struct {
        u32 dst : 28;
        u8      : 4;
      };
    } DMA0DAD, DMA1DAD, DMA2DAD, DMA3DAD;

    union {
      u16 v = 0;
      struct {
        u8                          : 5;
        DST_CONTROL dst_control     : 2;
        SRC_CONTROL src_control     : 2;
        u8 dma_repeat               : 1;
        TRANSFER_TYPE transfer_type : 1;
        u8 game_pak_drq             : 1;
        DMA_TIMING start_timing     : 2;
        u8 irq_at_end               : 1;
        u8 dma_enable               : 1;
      };
    } DMA0CNT_H, DMA1CNT_H, DMA2CNT_H, DMA3CNT_H;

    union {
      u16 v = 0;
      struct {
        u16 word_count;
      };
    } DMA0CNT_L, DMA1CNT_L, DMA2CNT_L, DMA3CNT_L;

    void print_dma_info(u8 id) {
      switch (id) {
        case 0: {
          fmt::println("============ DMA0 INFO ============");
          fmt::println("src: {:#010x}", +DMA0SAD.src);
          fmt::println("dst: {:#010x}", +DMA0DAD.dst);
          fmt::println("timing: {}", TIMING_MAP.at(DMA0CNT_H.start_timing));
          fmt::println("word count: {:#010x}", DMA0CNT_L.word_count);
          fmt::println("word size: {}", DMA0CNT_H.transfer_type == TRANSFER_TYPE::HALFWORD ? "16bits" : "32bits");
          fmt::println("===================================");
          break;
        }
        case 1: {
          fmt::println("============ DMA1 INFO ============");
          fmt::println("src: {:#010x}", +DMA1SAD.src);
          fmt::println("dst: {:#010x}", +DMA1DAD.dst);
          fmt::println("timing: {}", TIMING_MAP.at(DMA1CNT_H.start_timing));
          fmt::println("word count: {:#010x}", DMA1CNT_L.word_count);
          fmt::println("word size: {}", DMA1CNT_H.transfer_type == TRANSFER_TYPE::HALFWORD ? "16bits" : "32bits");
          fmt::println("===================================");
          break;
        }
        case 2: {
          fmt::println("============ DMA2 INFO ============");
          fmt::println("src: {:#010x}", +DMA2SAD.src);
          fmt::println("dst: {:#010x}", +DMA2DAD.dst);
          fmt::println("timing: {}", TIMING_MAP.at(DMA2CNT_H.start_timing));
          fmt::println("word count: {:#010x}", DMA2CNT_L.word_count);
          fmt::println("word size: {}", DMA2CNT_H.transfer_type == TRANSFER_TYPE::HALFWORD ? "16bits" : "32bits");
          fmt::println("===================================");
          break;
        }
        case 3: {
          fmt::println("============ DMA3 INFO ============");
          fmt::println("src: {:#010x}", +DMA3SAD.src);
          fmt::println("dst: {:#010x}", +DMA3DAD.dst);
          fmt::println("timing: {}", (u8)DMA3CNT_H.start_timing);
          fmt::println("timing: {}", TIMING_MAP.at(DMA3CNT_H.start_timing));
          fmt::println("word count: {:#010x}", DMA3CNT_L.word_count);
          fmt::println("word size: {}", DMA3CNT_H.transfer_type == TRANSFER_TYPE::HALFWORD ? "16bits" : "32bits");
          fmt::println("===================================");
          break;
        }
        default: {
          assert(0);
        }
      }
    }
  } dma_control;

  struct {
    union {
      u16 v = 0xFFFF;
      struct {
        u8 A      : 1;
        u8 B      : 1;
        u8 SELECT : 1;
        u8 START  : 1;
        u8 RIGHT  : 1;
        u8 LEFT   : 1;
        u8 UP     : 1;
        u8 DOWN   : 1;
        u8 R      : 1;
        u8 L      : 1;
        u8        : 6;
      };
    } KEYINPUT;

  } keypad_input = {};
};

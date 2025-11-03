#pragma once

#include "apu.hpp"
#include "common.hpp"
#include "common/defs.hpp"
#include "common/test_structs.hpp"
#include "enums.hpp"
#include "labels.hpp"
#include "pak.hpp"
#include "timer.hpp"

struct PPU;
#include "ppu.hpp"

struct DMAContext;
#include "dma.hpp"
struct ARM7TDMI;

inline u64 cycles_elapsed;

enum class ACCESS_TYPE { SEQUENTIAL, NON_SEQUENTIAL };
enum class WAITSTATE { WS0, WS1, WS2 };
struct Bus {
  enum class REGION : u8 {
    BIOS           = 0x0,
    EWRAM          = 0x2,
    IWRAM          = 0x3,
    IO             = 0x4,
    BG_OBJ_PALETTE = 0x5,
    VRAM           = 0x6,
    OAM            = 0x7,
    PAK_WS0_0      = 0x8,
    PAK_WS0_1      = 0x9,

    PAK_WS1_0 = 0xA,
    PAK_WS1_1 = 0xB,

    PAK_WS2_0 = 0xC,
    PAK_WS2_1 = 0xD,

    SRAM_0 = 0xE,
    SRAM_1 = 0xF,

  };

  // memory/devices
  std::vector<u8> BIOS;
  std::vector<u8> IWRAM;
  std::vector<u8> EWRAM;
  std::vector<u8> PALETTE_RAM;
  std::vector<u8> OAM;
  std::vector<u8> WAVE_RAM;
  std::vector<Transaction> transactions;

  u32 bios_open_bus = 0;

  Bus() : BIOS(0x4000), IWRAM(0x8000), EWRAM(0x40000), PALETTE_RAM(0x400), OAM(0x400), WAVE_RAM(0x20) {
    if (!std::filesystem::exists("./roms/magic.bin")) {
      spdlog::error("Running this emulator requires a valid GBA BIOS. Rename your BIOS to magic.bin, and place it in the roms/ folder.");
      assert(0);
    }

    BIOS = read_file("roms/magic.bin");
    bus_logger->set_level(spdlog::level::debug);
    mem_logger->set_level(spdlog::level::debug);
  }

  std::shared_ptr<spdlog::logger> bus_logger = spdlog::stdout_color_mt("BUS");
  std::shared_ptr<spdlog::logger> mem_logger = spdlog::stdout_color_mt("MEM");
  std::shared_ptr<spdlog::logger> dma_logger = spdlog::stdout_color_mt("DMA");

  ARM7TDMI* cpu = nullptr;
  Pak* pak      = nullptr;
  PPU* ppu      = nullptr;
  std::shared_ptr<DMAContext> ch0, ch1, ch2, ch3;
  Timer *tm0, *tm1, *tm2, *tm3 = nullptr;
  APU* apu = nullptr;

  void request_interrupt(INTERRUPT_TYPE type);

  [[nodiscard]] u8 read8(u32 address, ACCESS_TYPE access_type = ACCESS_TYPE::NON_SEQUENTIAL);
  [[nodiscard]] u16 read16(u32 address, ACCESS_TYPE access_type = ACCESS_TYPE::NON_SEQUENTIAL);
  [[nodiscard]] u32 read32(u32 address, ACCESS_TYPE access_type = ACCESS_TYPE::NON_SEQUENTIAL);

  void write8(u32 address, u8 value);
  void write16(u32 address, u16 value);
  void write32(u32 address, u32 value);

  [[nodiscard]] u8 io_read(u32 address);
  void io_write(u32 address, u8 value);

  u8 get_rom_cycles_by_waitstate(ACCESS_TYPE access_type, WAITSTATE ws);
  u8 get_wram_waitstates();

  enum MAPPING_MODE : u8 { ONE_DIMENSIONAL = 1, TWO_DIMENSIONAL = 0 };

  struct {
    union {
      u32 v = 0;
      struct {
        u8 enabled : 1;
        u32        : 31;
      };
    } IME = {};

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
        u8                     : 2;
      };
    } IE = {};

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

    u8 POSTFLG = 0;

    union {
      struct {
        u8        : 7;
        bool halt : 1;
      };
      u8 HALTCNT;
    };

    union {
      struct {
        u32                  : 24;
        u8 wait_control_wram : 4;
        u8                   : 4;
      };
      u32 IMC;
    };
  } system_control = {};

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

  struct {
    u16 TM0CNT_L, TM1CNT_L, TM2CNT_L, TM3CNT_L = 0;

    union {
      u16 v = 0;
      struct {
        PRESCALER_SEL prescaler_selection : 2;
        bool countup_timing               : 1;
        u8                                : 3;
        bool timer_irq                    : 1;
        bool timer_start                  : 1;
        u8                                : 8;
      };
    } TM0CNT_H, TM1CNT_H, TM2CNT_H, TM3CNT_H;

  } timer_fields = {};
};
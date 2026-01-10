#pragma once
#include <SDL3/SDL_audio.h>

#include <queue>

#include "common/defs.hpp"
#include "enums.hpp"

struct APU {
  APU() : audio_buf(3072) {}
  std::queue<i8> FIFO_A, FIFO_B;
  std::vector<i8> audio_buf;
  i16 sample_rate;
  void tick(u32 cycles);
  SDL_AudioStream* stream = nullptr;

  u16 write_pos = 0;

  void write_to_fifo(FIFO_CHANNEL channel, u8 value);
  void sample(FIFO_CHANNEL);

  struct {
    enum class SWEEP_FREQ_DIRECTION : u8 { INCREASE = 0, DECREASE = 1 };
    enum class ENVELOPE_DIRECTION : u8 { INCREASE = 0, DECREASE = 1 };

    union {
      u16 v;

      struct {
        u8 num_sweep_shift                        : 3;
        SWEEP_FREQ_DIRECTION sweep_freq_direction : 1;
        u8 sweep_time                             : 3;
        u16                                       : 9;
      };

    } SOUND1CNT_L, SOUND2CNT_L;

    union {
      u16 v;

      struct {
        u8 sound_length                       : 6;
        u8 wave_duty_pattern                  : 2;
        u8 envelope_step_time                 : 3;
        ENVELOPE_DIRECTION envelope_direction : 1;
        u8 initial_volume                     : 4;
      };

    } SOUND1CNT_H, SOUND2CNT_H;

    union {
      u32 v;

      struct {
        u16 frequency  : 11;
        u8             : 3;
        u8 length_flag : 1;
        u8 initial     : 1;
        u16            : 16;
      };

    } SOUND1CNT_X;
    enum WAVE_RAM_DIMENSION : u8 { ONE_BANK, TWO_BANKS };
    enum SOUND_CHANNEL_STATUS : u8 { STOP, PLAYBACK };
    union {
      u16 v;

      struct {
        u8                                  : 5;
        WAVE_RAM_DIMENSION bank_amount      : 1;
        u8 bank_number                      : 1;
        SOUND_CHANNEL_STATUS sound_ch_3_off : 1;
        u8                                  : 8;
      };

    } SOUND3CNT_L = {};

    union {
      u16 v;

      struct {
        u8 sound_length;
        u8              : 5;
        u8 sound_volume : 2;
        u8 force_volume : 1;
      };

    } SOUND3CNT_H = {};

    union {
      u32 v;

      struct {
        u16 sample_rate : 11;
        u8              : 3;
        u8 length_flag  : 1;
        u8 initial      : 1;
        u16             : 16;
      };

    } SOUND3CNT_X;

    union {
      u32 v;

      struct {
        u8 sound_length              : 6;
        u8                           : 2;
        u8 envelope_step_time        : 3;
        ENVELOPE_DIRECTION direction : 1;
        u8 initial_volume            : 4;
        u16                          : 16;
      };

    } SOUND4CNT_L;

    union {
      u32 v;

      struct {
        u8 dividing_ratio        : 3;
        u8 counter_step_width    : 1;
        u8 shift_clock_frequency : 3;
        u8                       : 6;
        u8 length_flag           : 1;
        u8 initial               : 1;
        u16                      : 16;
      };

    } SOUND4CNT_H;

    union {
      u16 v;

      struct {
        u8 sound_master_vol_right : 3;
        u8                        : 1;
        u8 sound_master_vol_left  : 3;
        u8                        : 1;
        u8 sound_enable_right     : 4;
        u8 sound_enable_left      : 4;
      };

    } SOUNDCNT_L;

    union {
      u16 v;

      struct {
        u8 psg_master_vol                                : 2;
        u8 dma_sound_a_vol                               : 1;
        u8 dma_sound_b_vol                               : 1;
        u8                                               : 4;

        u8 dma_sound_a_enable_right                      : 1;
        u8 dma_sound_a_enable_left                       : 1;
        SELECTED_FIFO_DMA_TIMER dma_sound_a_timer_select : 1;
        u8 dma_sound_a_reset_fifo                        : 1;

        u8 dma_sound_b_enable_right                      : 1;
        u8 dma_sound_b_enable_left                       : 1;
        SELECTED_FIFO_DMA_TIMER dma_sound_b_timer_select : 1;
        u8 dma_sound_b_reset_fifo                        : 1;
      };

    } SOUNDCNT_H;

    union {
      u32 v;

      struct {
        u8 sound_1_on_flag        : 1;
        u8 sound_2_on_flag        : 1;
        u8 sound_3_on_flag        : 1;
        u8 sound_4_on_flag        : 1;
        u8                        : 3;
        u8 psg_fifo_master_enable : 1;
        u32                       : 24;
      };

    } SOUNDCNT_X;

  } sound_registers = {};
};
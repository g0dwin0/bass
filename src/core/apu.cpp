#include "apu.hpp"

#include <algorithm>
#include <cassert>

#include "bus.hpp"
#include "common.hpp"
#include "frontend/window.hpp"

static constexpr i32 SAMPLE_RATE = 350;

// void APU::write_to_fifo(FIFO_CHANNEL ch, u8 value) {
//   // switch (ch) {
//     // case FIFO_CHANNEL::FIFO_A: {
//     //   if (FIFO_A.size() <= 16) break;
//     // }
//     // case FIFO_CHANNEL::FIFO_B: break;
//   // }
// };

#ifndef SYSTEM_TEST_MODE
// void APU::tick(u32 cycles) {
// regs.channel_1.frequency_timer -= cycles;
// regs.channel_2.frequency_timer -= cycles;
// regs.channel_3.frequency_timer -= cycles;
// regs.channel_4.frequency_timer -= cycles;
//
// sample_rate -= cycles;
//
// if (regs.channel_1.frequency_timer <= 0) {
//   regs.channel_1.duty_step = (regs.channel_1.duty_step + 1) & 7;
//   regs.channel_1.frequency_timer += (2048 - regs.channel_1.frequency) * 4;
// }
//
// if (regs.channel_2.frequency_timer <= 0) {
//   regs.channel_2.duty_step = (regs.channel_2.duty_step + 1) & 7;
//   regs.channel_2.frequency_timer += (2048 - regs.channel_2.frequency) * 4;
// }
//
// if (regs.channel_3.frequency_timer <= 0) {
//   regs.channel_3.frequency_timer += (2048 - regs.channel_3.frequency) * 2;
//   regs.channel_3.sample_index = (regs.channel_3.sample_index + 1) & 31;
// }
//
// if (regs.channel_4.frequency_timer <= 0) {
//   regs.channel_4.frequency_timer = (regs.channel_4.NR43.clock_divider > 0 ? (regs.channel_4.NR43.clock_divider << 4) : 8) << regs.channel_4.NR43.clock_shift;
//
//   u8 xor_result = (regs.channel_4.lfsr & 0b01) ^ ((regs.channel_4.lfsr & 0b10) >> 1);
//
//   regs.channel_4.lfsr = (regs.channel_4.lfsr >> 1) | (xor_result << 14);
//
//   if (regs.channel_4.NR43.lfsr_width == 1) {
//     regs.channel_4.lfsr &= ~(1 << 6);
//     regs.channel_4.lfsr |= (xor_result << 6);
//   }
// }

// if (sample_rate <= 0) {
// sample();
// sample_rate += SAMPLE_RATE;
// }
// }

// f32 APU::sample_ch1() {
//   if (regs.channel_1.channel_volume == 0 || !regs.channel_1.dac_enabled() || !regs.channel_1.channel_enabled) return 0.0f;
//
//   u8 duty_cycle_output = SQUARE_DUTY_WAVEFORMS[regs.channel_1.NR11.WAVE_DUTY][regs.channel_1.duty_step];
//   return dac(duty_cycle_output * regs.channel_1.channel_volume);
// };
//
// f32 APU::sample_ch2() {
//   if (!regs.channel_2.channel_enabled || regs.channel_2.channel_volume == 0 || !regs.channel_2.dac_enabled()) return 0.0f;
//   u8 duty_cycle_output = SQUARE_DUTY_WAVEFORMS[regs.channel_2.NR21.wave_duty_pattern][regs.channel_2.duty_step];
//
//   return dac(duty_cycle_output * regs.channel_2.channel_volume);
// };
//
// f32 APU::sample_ch3() {
//   if (!regs.channel_3.channel_enabled || !regs.channel_3.dac_enabled() || regs.channel_3.NR32.output_level == 0) return 0.0f;
//
//   return dac(get_sample_nibble(regs.channel_3.sample_index) >> (regs.channel_3.NR32.output_level - 1));
// };
// f32 APU::sample_ch4() {
//   if (!regs.channel_4.channel_enabled || regs.channel_4.channel_volume == 0 || !regs.channel_4.dac_enabled()) return 0.0f;
//
//   return dac((~(regs.channel_4.lfsr) & 0x01) * regs.channel_4.channel_volume);
// };
//
void APU::tick(u32 cycles) {
  //   regs.channel_1.frequency_timer -= cycles;
  //   regs.channel_2.frequency_timer -= cycles;
  //   regs.channel_3.frequency_timer -= cycles;
  //   regs.channel_4.frequency_timer -= cycles;
  //
  sample_rate -= cycles;
  //
  //   if (regs.channel_1.frequency_timer <= 0) {
  //     regs.channel_1.duty_step = (regs.channel_1.duty_step + 1) & 7;
  //     regs.channel_1.frequency_timer += (2048 - regs.channel_1.frequency) * 4;
  //   }
  //
  //   if (regs.channel_2.frequency_timer <= 0) {
  //     regs.channel_2.duty_step = (regs.channel_2.duty_step + 1) & 7;
  //     regs.channel_2.frequency_timer += (2048 - regs.channel_2.frequency) * 4;
  //   }
  //
  //   if (regs.channel_3.frequency_timer <= 0) {
  //     regs.channel_3.frequency_timer += (2048 - regs.channel_3.frequency) * 2;
  //     regs.channel_3.sample_index = (regs.channel_3.sample_index + 1) & 31;
  //   }
  //
  //   if (regs.channel_4.frequency_timer <= 0) {
  //     regs.channel_4.frequency_timer = (regs.channel_4.NR43.clock_divider > 0 ? (regs.channel_4.NR43.clock_divider << 4) : 8) << regs.channel_4.NR43.clock_shift;
  //
  //     u8 xor_result = (regs.channel_4.lfsr & 0b01) ^ ((regs.channel_4.lfsr & 0b10) >> 1);
  //
  //     regs.channel_4.lfsr = (regs.channel_4.lfsr >> 1) | (xor_result << 14);
  //
  //     if (regs.channel_4.NR43.lfsr_width == 1) {
  //       regs.channel_4.lfsr &= ~(1 << 6);
  //       regs.channel_4.lfsr |= (xor_result << 6);
  //     }
  //   }
  //
  if (sample_rate <= 0) {
    sample(FIFO_CHANNEL::FIFO_A);
    sample(FIFO_CHANNEL::FIFO_B);

    sample_rate += SAMPLE_RATE;
  }
}
void APU::sample(FIFO_CHANNEL ch) {
  // assert(stream != nullptr);
  i8 l = 0;
  i8 r = 0;

  switch (ch) {
    case FIFO_CHANNEL::FIFO_A: {
      if (FIFO_A.empty()) {
        l = 0;
        r = 0;
      } else {
        l = FIFO_A.front();
        r = l;
        l = sound_registers.SOUNDCNT_H.dma_sound_a_enable_left ? l : 0;
        r = sound_registers.SOUNDCNT_H.dma_sound_a_enable_right ? r : 0;

        FIFO_A.pop();
      }
      break;
    }
    case FIFO_CHANNEL::FIFO_B: {
      if (FIFO_B.empty()) {
        l = 0;
        r = 0;
      } else {
        l = FIFO_B.front();
        r = l;
        l = sound_registers.SOUNDCNT_H.dma_sound_b_enable_left ? l : 0;
        r = sound_registers.SOUNDCNT_H.dma_sound_b_enable_right ? r : 0;
        FIFO_B.pop();
      }
      break;
    }
  }

  audio_buf[write_pos++] = l;
  audio_buf[write_pos++] = r;

  if (write_pos == 1024) {
    SDL_PutAudioStreamData(stream, audio_buf.data(), 1024 * sizeof(i8));
    write_pos = 0;
  }
}
// f32 APU::dac(u8 in) {
//   assert(in <= 0xF);
//   // if (!dac_enabled) return 0.0f;
//
//   return ((in / 7.5) - 1.0);
// };
#endif
// void APU::ch1_trigger_event() {
//   // fmt::println("triggered ch1");
//
//   // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Trigger_Event -- length ctr quirks
//   if ((seq_current_step & 1) && regs.channel_1.NR14.length_enable && regs.channel_1.length_timer == 0) {
//     regs.channel_1.length_timer = 63;
//   }
//
//   if (regs.channel_1.dac_enabled()) {
//     regs.channel_1.channel_enabled = true;
//     // fmt::println("[APU] triggered channel 1 (ON)");
//   }
//
//   if (regs.channel_1.length_timer == 0) {
//     regs.channel_1.length_timer = 64;
//   }
//
//   // ====================== sweep specific trigger events ======================
//
//   regs.channel_1.sweep_calc_in_negate = false;
//
//   if (regs.channel_1.NR10.sweep_period != 0 || regs.channel_1.NR10.sweep_shift != 0) {
//     regs.channel_1.sweep_enabled = true;
//   } else {
//     regs.channel_1.sweep_enabled = false;
//   }
//
//   // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
//   // reload sweep timer: the volume envelope and sweep timers treat a period of 0 as 8
//   regs.channel_1.sweep_timer = regs.channel_1.NR10.sweep_period != 0 ? regs.channel_1.NR10.sweep_period : 8;
//
//   regs.channel_1.sweep_freq_shadow_reg = regs.channel_1.frequency;
//
//   if (regs.channel_1.NR10.sweep_shift != 0) {
//     if (regs.channel_1.NR10.direction_negate == SUBTRACTION) regs.channel_1.sweep_calc_in_negate = true;
//
//     u32 new_frequency = calculate_sweep_freq();
//
//     if (new_frequency > 2047) {
//       fmt::println("freq: {:#010x} -- disabled channel", new_frequency);
//       regs.channel_1.channel_enabled = false;
//     }
//   }
//
//   regs.channel_1.frequency_timer = regs.channel_1.frequency;
//   regs.channel_1.period_timer    = regs.channel_1.NR12.sweep_pace ? regs.channel_1.NR12.sweep_pace : 8;
//   regs.channel_1.channel_volume  = regs.channel_1.NR12.initial_volume;
// };
// void APU::ch2_trigger_event() {
//   // fmt::println("triggered channel 2");
//   // Obscure behaviour
//   if ((seq_current_step & 1) && regs.channel_2.NR24.length_enable && regs.channel_2.length_timer == 0) {
//     regs.channel_2.length_timer = 63;
//   }
//
//   if (regs.channel_2.dac_enabled()) {
//     regs.channel_2.channel_enabled = true;
//     // fmt::println("[APU] triggered channel 2 (ON) -- loaded length timer: {}", regs.channel_2.length_timer);
//   }
//
//   if (regs.channel_2.length_timer == 0) {
//     regs.channel_2.length_timer = 64;
//     // fmt::println("[APU] post reload length timer: {}", regs.CHANNEL_2.length_timer);
//   }
//
//   regs.channel_2.frequency_timer = regs.channel_2.frequency;
//   regs.channel_2.period_timer    = regs.channel_2.NR22.sweep_pace ? regs.channel_2.NR22.sweep_pace : 8;
//   regs.channel_2.channel_volume  = regs.channel_2.NR22.initial_volume;
// };
// void APU::ch3_trigger_event() {  // Obscure behaviour
//   if ((seq_current_step & 1) == 1 && regs.channel_3.NR34.length_enable && regs.channel_3.length_timer == 0) {
//     regs.channel_3.length_timer = 255;
//   }
//
//   if (regs.channel_3.length_timer == 0) {
//     regs.channel_3.length_timer = 256;
//   }
//
//   if (regs.channel_3.dac_enabled()) {
//     regs.channel_3.channel_enabled = true;
//     // fmt::println("[APU] triggered channel 3 (ON) -- loaded length timer: {}", regs.channel_3.length_timer);
//   }
//
//   regs.channel_3.sample_index    = 0;
//   regs.channel_3.frequency_timer = (2048 - regs.channel_3.frequency) * 2;
// };
// void APU::ch4_trigger_event() {
//   // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Trigger_Event
//   if ((seq_current_step & 1) == 1 && regs.channel_4.NR44.length_enable && regs.channel_4.length_timer == 0) {
//     regs.channel_4.length_timer = 63;
//   }
//
//   if (regs.channel_4.length_timer == 0) {
//     regs.channel_4.length_timer = 64;
//   }
//
//   if (regs.channel_4.dac_enabled()) {
//     regs.channel_4.channel_enabled = true;
//     // fmt::println("[APU] triggered channel 4 (ON)");
//   }
//
//   regs.channel_4.frequency_timer = (regs.channel_4.NR43.clock_divider > 0 ? (regs.channel_4.NR43.clock_divider << 4) : 8) << regs.channel_4.NR43.clock_shift;
//   regs.channel_4.lfsr            = 0xFFFF;
//   regs.channel_4.period_timer    = regs.channel_4.NR42.sweep_pace ? regs.channel_4.NR42.sweep_pace : 8;
//   regs.channel_4.channel_volume  = regs.channel_4.NR42.initial_volume;
// }
//
// inline void APU::tick_vol_env_ch1() {
//   if (regs.channel_1.NR12.sweep_pace != 0) {
//     if (regs.channel_1.period_timer > 0) {
//       regs.channel_1.period_timer -= 1;
//     }
//
//     if (regs.channel_1.period_timer == 0) {
//       regs.channel_1.period_timer = regs.channel_1.NR12.sweep_pace;
//       bool is_upwards             = regs.channel_1.NR12.envelope_direction == INCREASE;
//
//       if ((regs.channel_1.channel_volume < 0xF && is_upwards) || regs.channel_1.channel_volume > 0x0 && !is_upwards) {
//         if (is_upwards) {
//           assert(regs.channel_1.channel_volume != 0xF);
//           regs.channel_1.channel_volume += 1;
//         } else {
//           assert(regs.channel_1.channel_volume != 0);
//           regs.channel_1.channel_volume -= 1;
//         }
//       }
//     }
//   }
// }
// inline void APU::tick_vol_env_ch2() {
//   if (regs.channel_2.NR22.sweep_pace != 0) {
//     if (regs.channel_2.period_timer > 0) {
//       regs.channel_2.period_timer -= 1;
//     }
//
//     if (regs.channel_2.period_timer == 0) {
//       regs.channel_2.period_timer = regs.channel_2.NR22.sweep_pace;
//       bool is_upwards             = regs.channel_2.NR22.envelope_direction == INCREASE;
//
//       if ((regs.channel_2.channel_volume < 0xF && is_upwards) || regs.channel_2.channel_volume > 0x0 && !is_upwards) {
//         if (is_upwards) {
//           regs.channel_2.channel_volume += 1;
//         } else {
//           regs.channel_2.channel_volume -= 1;
//         }
//       }
//     }
//   }
// }
// inline void APU::tick_vol_env_ch4() {
//   if (regs.channel_4.NR42.sweep_pace != 0) {
//     if (regs.channel_4.period_timer > 0) {
//       regs.channel_4.period_timer -= 1;
//     }
//
//     if (regs.channel_4.period_timer == 0) {
//       regs.channel_4.period_timer = regs.channel_4.NR42.sweep_pace;
//       bool is_upwards             = regs.channel_4.NR42.envelope_direction == INCREASE;
//
//       if ((regs.channel_4.channel_volume < 0xF && is_upwards) || regs.channel_4.channel_volume > 0x0 && !is_upwards) {
//         if (is_upwards) {
//           regs.channel_4.channel_volume += 1;
//         } else {
//           regs.channel_4.channel_volume -= 1;
//         }
//       }
//     }
//   }
// }

// u8 APU::get_sample_nibble(u8 sample_index) {
//   bool upper      = sample_index % 2 == 0;
//   u8 wave_ram_idx = sample_index / 2;
//   u8 nibble       = bus->wave_ram.at(wave_ram_idx);
//
//   if (upper) {
//     nibble = nibble >> 4;
//   } else {
//     nibble &= 0x0F;
//   }
//
//   return nibble;
// }
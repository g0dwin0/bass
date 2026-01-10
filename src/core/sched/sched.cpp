#include "sched/sched.hpp"

#include "agb.hpp"
#include "bus.hpp"
#include "common/stopwatch.hpp"

void Scheduler::schedule(EventType e, u64 when) { event_queue.push({e, when}); }

i64 Scheduler::get_diff_adjusted_timestamp(const Event& e, const u64 current_timestamp, const u64 target_timestamp) {
  return (current_timestamp + (e.timestamp - current_timestamp) + target_timestamp);
}
void Scheduler::step(u32 cycles) {
  cycles_elapsed += cycles;
  while (cycles_elapsed >= event_queue.top().timestamp) {
    const Event& event = event_queue.top();
    switch (event.type) {
      case EventType::VBLANK: {
        agb->ppu.draw_window();
        agb->ppu.display_fields.DISPSTAT.set_vblank();
        agb->ppu.db.swap_buffers();
        agb->ppu.reset_sprite_layer();
        Stopwatch::end();
        Stopwatch::start();

        if (agb->ppu.display_fields.DISPSTAT.VBLANK_IRQ_ENABLE) {
          agb->bus.request_interrupt(INTERRUPT_TYPE::LCD_VBLANK);
        }

        schedule(EventType::VBLANK, event.timestamp + 197120);
        break;
      }
      case EventType::HBLANK_START: {
        agb->ppu.display_fields.DISPSTAT.set_hblank();

        if (agb->ppu.display_fields.DISPSTAT.HBLANK_IRQ_ENABLE) {
          agb->bus.request_interrupt(INTERRUPT_TYPE::LCD_HBLANK);
        }
        schedule(EventType::HBLANK_END, event.timestamp + 226);
        break;
      }
      case EventType::HBLANK_END: {
        agb->ppu.display_fields.DISPSTAT.reset_hblank();
        agb->ppu.step();
        if (agb->ppu.display_fields.VCOUNT.LY == 227) {
          agb->ppu.display_fields.VCOUNT.LY = 0;
          agb->ppu.display_fields.DISPSTAT.reset_vblank();
        } else {
          agb->ppu.display_fields.VCOUNT.LY++;
          if (agb->ppu.display_fields.VCOUNT.LY == 227) {
            // latch BG2 & BG3 internal reference pointers
            agb->ppu.latched_bg2x = agb->ppu.display_fields.BG2X.INTEGER_PORT;
            agb->ppu.latched_bg2y = agb->ppu.display_fields.BG2Y.INTEGER_PORT;

            agb->ppu.latched_bg3x = agb->ppu.display_fields.BG3X.INTEGER_PORT;
            agb->ppu.latched_bg3y = agb->ppu.display_fields.BG3Y.INTEGER_PORT;
          }
        }

        agb->ppu.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = agb->ppu.display_fields.VCOUNT.LY == agb->ppu.display_fields.DISPSTAT.LYC;

        if (agb->ppu.display_fields.DISPSTAT.V_COUNTER_IRQ_ENABLE && agb->ppu.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG) {
          agb->bus.request_interrupt(INTERRUPT_TYPE::LCD_VCOUNT_MATCH);
        }

        schedule(EventType::HBLANK_START, event.timestamp + 1006);
        break;
      }
      case EventType::TIMER0_OVERFLOW: {
        // agb->timers[0].ctrl.timer_irq_enable = true;
        // agb->timers[0].ctrl.timer_start_stop = true;
        // agb->timers[0].tick(cycles - 2);
        // agb->timers[0].tick_this_cycle = false;
        // agb->timers

        // if (agb->timers[0].ctrl.timer_irq_enable) agb->bus.request_interrupt(agb->timers[0].get_timer_interrupt());
        // agb->timers[0].counter     = agb->timers[0].reload_value;
        // agb->timers[0].start_time  = event.timestamp;
        // agb->timers[0].start_value = agb->timers[0].reload_value + (cycles_elapsed - event.timestamp);
        // u32 diff_to_overflow       = 0x10000 - agb->timers[0].start_value;
        //
        // diff_to_overflow = agb->timers[0].get_divider_val() * diff_to_overflow;
        // schedule(EventType::TIMER0_OVERFLOW, event.timestamp + diff_to_overflow);

        // if (agb->apu.sound_registers.SOUNDCNT_H.dma_sound_a_timer_select == SELECTED_FIFO_DMA_TIMER::TIMER0) {
        //   // agb->apu.sample(FIFO_CHANNEL::FIFO_B);
        //   agb->dma_channels[1]->dmacnt_h.dst_control = DST_CONTROL::FIXED;
        //   agb->dma_channels[1]->transfer32(agb->dma_channels[1]->src, FIFO_A, 4);
        //   break;
        // }

        // if (agb->apu.sound_registers.SOUNDCNT_H.dma_sound_b_timer_select == SELECTED_FIFO_DMA_TIMER::TIMER0) {
        //   // agb->apu.sample(FIFO_CHANNEL::FIFO_B);
        //   agb->dma_channels[2]->dmacnt_h.dst_control = DST_CONTROL::FIXED;
        //   agb->dma_channels[2]->transfer32(agb->dma_channels[2]->src, FIFO_B, 4);
        //   break;
        // }

        // if (agb->apu.sound_registers.SOUNDCNT_H.dma_sound_a_timer_select == SELECTED_FIFO_DMA_TIMER::TIMER0) {
        //   agb->apu.sample(FIFO_CHANNEL::FIFO_A);
        //
        // if (agb->apu.FIFO_A.size() <= 4) {
        //   agb->dma_channels
        //   break;
        // }
        // }
      }
    }

    event_queue.pop();
  }
}

void Scheduler::print_scheduled_events() {
  while (!event_queue.empty()) {
    fmt::println("timestamp: {}", event_queue.top().timestamp);
    event_queue.pop();
  }
}

// u64 Scheduler::get_next_event() {
//   // print_scheduled_events();
//   return event_queue.top().timestamp;
// }

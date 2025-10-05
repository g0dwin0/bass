#include "sched/sched.hpp"

#include "agb.hpp"
#include "bus.hpp"
#include "common/stopwatch.hpp"

void Scheduler::schedule(EventType e, u64 when) { event_queue.push({e, when}); }

i64 Scheduler::get_diff_adjusted_timestamp(const Event& e, const u64 current_timestamp, const u64 target_timestamp) {
  return (current_timestamp + (e.timestamp - current_timestamp) + target_timestamp);
}
void Scheduler::step(AGB& agb, u32 cycles) {
  cycles_elapsed += cycles;
  while (cycles_elapsed >= event_queue.top().timestamp) {
    const Event& event = event_queue.top();
    switch (event.type) {
      case EventType::VBLANK: {
        agb.bus.display_fields.DISPSTAT.set_vblank();
        agb.ppu.db.swap_buffers();
        agb.ppu.reset_sprite_layer();
        Stopwatch::end();
        Stopwatch::start();

        if (agb.bus.display_fields.DISPSTAT.VBLANK_IRQ_ENABLE) {
          agb.bus.request_interrupt(INTERRUPT_TYPE::LCD_VBLANK);
        }

        schedule(EventType::VBLANK, get_diff_adjusted_timestamp(event, cycles_elapsed, 197120));
        break;
      }
      case EventType::HBLANK_START: {
        agb.bus.display_fields.DISPSTAT.set_hblank();

        if (agb.bus.display_fields.DISPSTAT.HBLANK_IRQ_ENABLE) {
          agb.bus.request_interrupt(INTERRUPT_TYPE::LCD_HBLANK);
        }
        schedule(EventType::HBLANK_END, get_diff_adjusted_timestamp(event, cycles_elapsed, 226));
        break;
      }
      case EventType::HBLANK_END: {
        agb.bus.display_fields.DISPSTAT.reset_hblank();
        agb.ppu.step();
        if (agb.bus.display_fields.VCOUNT.LY == 227) {
          agb.bus.display_fields.VCOUNT.LY = 0;
        } else {
          agb.bus.display_fields.VCOUNT.LY++;
          if (agb.bus.display_fields.VCOUNT.LY == 227) {
            agb.bus.display_fields.DISPSTAT.reset_vblank();
          }
        }

        if (agb.bus.display_fields.VCOUNT.LY == agb.bus.display_fields.DISPSTAT.LYC) {
          agb.bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = true;

          if (agb.bus.display_fields.DISPSTAT.V_COUNTER_IRQ_ENABLE) {
            agb.bus.request_interrupt(INTERRUPT_TYPE::LCD_VCOUNT_MATCH);
          }

        } else {
          agb.bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = false;
        }

        schedule(EventType::HBLANK_START, get_diff_adjusted_timestamp(event, cycles_elapsed, 1232));
        break;
      }
      case EventType::TIMER0_START: {
        agb.timers[0].ctrl.timer_irq_enable = true;
        agb.timers[0].counter               = agb.timers[0].reload_value;
        // agb.timers
        break;
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

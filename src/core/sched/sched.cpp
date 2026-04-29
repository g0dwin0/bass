#include "sched/sched.hpp"

#include "agb.hpp"
#include "bus.hpp"
#include "common/stopwatch.hpp"

void Scheduler::schedule(EventType e, u64 when) { event_queue.push({e, when}); }

void Scheduler::step([[maybe_unused]] u32 w) {
  // cycles_elapsed += cycles;
  while (cycles_elapsed >= event_queue.top().timestamp) {
    const auto& [type, timestamp] = event_queue.top();
    switch (type) {
      case EventType::VBLANK: {
        // agb->ppu.draw_window();
        // fmt::println("VBLANK EVENT LY: {}", agb->ppu.display_fields.VCOUNT.LY);
        // if (agb->ppu.display_fields.VCOUNT.LY < 159) assert(0);
        agb->ppu.display_fields.DISPSTAT.set_vblank();
        agb->ppu.db.swap_buffers();
        agb->ppu.reset_sprite_layer();
        Stopwatch::end();
        Stopwatch::start();

        if (agb->ppu.display_fields.DISPSTAT.VBLANK_IRQ_ENABLE) {
          agb->bus.request_interrupt(INTERRUPT_TYPE::LCD_VBLANK);
        }
        agb->check_for_dma();

        schedule(EventType::VBLANK, timestamp + (197120 + (1232 * 68)));
        break;
      }
      case EventType::HBLANK_START: {
        agb->ppu.display_fields.DISPSTAT.set_hblank();

        if (agb->ppu.display_fields.DISPSTAT.HBLANK_IRQ_ENABLE) {
          agb->bus.request_interrupt(INTERRUPT_TYPE::LCD_HBLANK);
        }
        agb->check_for_dma();
        schedule(EventType::HBLANK_END, timestamp + 226);
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
          agb->check_for_dma();
        }

        schedule(EventType::HBLANK_START, timestamp + 1006);
        break;
      }
      default: assert(0);
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

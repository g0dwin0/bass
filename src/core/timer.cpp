#include "timer.hpp"

#include "bus.hpp"
#include "enums.hpp"
#include "spdlog/fmt/bundled/base.h"

static u8 timer_id = 0;
Timer::Timer() {
  id             = timer_id++;
  counter        = 0;
  reload_value   = 0;
  ctrl.prescaler = F_1;
  fmt::println("created timer {}", id);
}
void Timer::reload_counter() { counter = reload_value; }

void Timer::tick(u64 cycles) {
  if (!ctrl.timer_start_stop) return;
  // fmt::println("delta: {} -- cycle request: {}", (cycles_elapsed - (start_time + 2)), cycles);

  if (!(start_time < cycles_elapsed)) {
    // fmt::println("calculated delta: {}, start time: {}, cycles_elapsed: {}, cycles requested: {}", (start_time) - cycles_elapsed, start_time, cycles_elapsed, cycles);
    return;
  }

  for (u64 i = 0; i < cycles; i++) {
    if (((cycles_elapsed + cycles) % get_divider_val()) == 0) {
      if (counter == 0xFFFF) {
        counter = reload_value;
        if (ctrl.timer_irq_enable) bus->request_interrupt(get_timer_interrupt());

      } else {
        counter++;
      }
    }
  }
}

INTERRUPT_TYPE Timer::get_timer_interrupt() const {
  switch (id) {
    case 0: return INTERRUPT_TYPE::TIMER0_OVERFLOW;
    case 1: return INTERRUPT_TYPE::TIMER1_OVERFLOW;
    case 2: return INTERRUPT_TYPE::TIMER2_OVERFLOW;
    case 3: return INTERRUPT_TYPE::TIMER3_OVERFLOW;
  };

  throw std::runtime_error("bad id");
  return INTERRUPT_TYPE::KEYPAD;
}

std::string Timer::get_prescaler_string() const {
  switch (ctrl.prescaler) {
    case F_1: return "F/1";
    case F_64: return "F/64";
    case F_256: return "F/256";
    case F_1024: return "F/1024";
  }

  exit(-1);
}

u16 Timer::get_divider_val() const {
  switch (ctrl.prescaler) {
    case F_1: return 1;
    case F_64: return 64;
    case F_256: return 256;
    case F_1024: return 1024;
    default: assert(0);
  }
}
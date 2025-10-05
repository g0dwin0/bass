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
  divider        = 0;
  fmt::println("created timer {}", id);
}

void Timer::tick(u64 cycles) {
  if (!ctrl.timer_start_stop) return;

  for (u64 i = 0; i < cycles; i++) {
    divider -= 1;
    if (divider <= 0) {
      if (counter == 0xFFFF) {
        reload_counter();
        if (ctrl.timer_irq_enable) bus->request_interrupt(get_timer_interrupt());
      } else {
        counter++;
      }

      reload_divider();
    }
  }
}

void Timer::reload_counter() { counter = reload_value; };

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

void Timer::reload_divider() {
  switch (ctrl.prescaler) {
    case F_1: divider = 1; break;
    case F_64: divider = 64; break;
    case F_256: divider = 256; break;
    case F_1024: divider = 1024; break;
  }
}

std::string Timer::get_prescaler_string() const {
  switch (ctrl.prescaler) {
    case F_1: return "F/1";
    case F_64: return "F/64";
    case F_256: return "F/256";
    case F_1024: return "F/1024";
  }

  exit(-1);
  return "";
}
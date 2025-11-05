#pragma once

#include <string>

#include "common/defs.hpp"
#include "enums.hpp"

struct Bus;

struct Timer {
  Timer();

  u64 start_time  = 0;
  u16 start_value = 0;
  bool use_delta = false;
  
  union {
    u16 v;
    struct {
      PRESCALER_SEL prescaler : 2;
      bool count_up_timing    : 1;
      u8                      : 3;
      bool timer_irq_enable   : 1;
      bool timer_start_stop   : 1;
      u8                      : 8;
    };
  } ctrl = {};

  Bus* bus = nullptr;

  u8 id;

  u16 counter      = 0;
  u16 reload_value = 0;

  void tick(u64 cycles);
  void reload_counter();
  [[nodiscard]] u16 get_divider_val() const;

  [[nodiscard]] INTERRUPT_TYPE get_timer_interrupt() const;
  [[nodiscard]] std::string get_prescaler_string() const;
};
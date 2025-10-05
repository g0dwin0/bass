#pragma once

#include <functional>
#include <queue>

#include "agb.hpp"
#include "common/defs.hpp"
#include "spdlog/fmt/bundled/base.h"
namespace Scheduler {
  enum class EventType { VBLANK, HBLANK_START, HBLANK_END, TIMER0_START };

  struct Event {
    EventType type;
    u64 timestamp;  // cycle count
  };

  struct EventCompare {
    bool operator()(const Event& l, const Event& r) const { return l.timestamp > r.timestamp; };
  };

  static std::priority_queue<Event, std::vector<Event>, EventCompare> event_queue;

  // Returns timestamp adjusted by the difference of the timestamp when an event is processed, and when it was scheduled.
  i64 get_diff_adjusted_timestamp(const Event& e, u64 current_timestamp, u64 target_timestamp);

  void cycle_by(u32 cycles);
  void schedule(EventType e, u64 when);
  void step(AGB& agb, u32 cycles);

  void print_scheduled_events();
}  // namespace Scheduler
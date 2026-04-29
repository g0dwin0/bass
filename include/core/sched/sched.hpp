#pragma once

#include <queue>

#include "agb.hpp"
#include "common/defs.hpp"

namespace Scheduler {
  enum class EventType { VBLANK, HBLANK_START, HBLANK_END, TIMER0_OVERFLOW };

  inline AGB* agb = nullptr;

  struct Event {
    EventType type;
    u64 timestamp;  // cycle count
  };

  struct EventCompare {
    bool operator()(const Event& l, const Event& r) const { return l.timestamp > r.timestamp; };
  };

  static std::priority_queue<Event, std::vector<Event>, EventCompare> event_queue;

  void schedule(EventType e, u64 when);
  void step(u32 cycles);

  void print_scheduled_events();
}  // namespace Scheduler
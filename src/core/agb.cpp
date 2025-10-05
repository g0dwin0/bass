#include "agb.hpp"

#include "../../include/core/flash.hpp"
#include "bus.hpp"
#include "common/defs.hpp"
#include "enums.hpp"
#include "sched/sched.hpp"
AGB::AGB() {
  bus.pak                   = &pak;
  bus.cpu                   = &cpu;
  cpu.bus                   = &bus;
  bus.ppu                   = &ppu;
  ppu.bus                   = &bus;
  pak.flash_controller.SRAM = &pak.SRAM;

  for (u8 i = 0; i < 4; i++) {
    dma_channels[i] = new DMAContext(&bus);
    timers[i].bus   = &bus;
  }

  bus.ch0 = dma_channels[0];
  bus.ch1 = dma_channels[1];
  bus.ch2 = dma_channels[2];
  bus.ch3 = dma_channels[3];

  bus.tm0 = &timers[0];
  bus.tm1 = &timers[1];
  bus.tm2 = &timers[2];
  bus.tm3 = &timers[3];
}

AGB::~AGB() {
  for (u8 i = 0; i < 4; i++) {
    delete dma_channels[i];
  }
  fmt::println("destroyed dma channels");
}

void AGB::check_for_dma() const {
  for (DMAContext* ch : dma_channels) {
    if (ch->enabled() && start_timing_cond_met(ch)) ch->process();
  }
};

bool AGB::start_timing_cond_met(DMAContext* ch) const {
  switch (ch->dmacnt_h.start_timing) {
    case DMA_START_TIMING::IMMEDIATELY: return true;
    case DMA_START_TIMING::VBLANK: return bus.display_fields.DISPSTAT.VBLANK_FLAG;
    case DMA_START_TIMING::HBLANK: return bus.display_fields.DISPSTAT.HBLANK_FLAG;
    case DMA_START_TIMING::SPECIAL: return false;  // TODO: implement checking for special behaviour
  }

  assert(0);
}

void AGB::tick_timers(u64 cycles) {
  for (Timer& t : timers) {
    t.tick(cycles);
  }
}

void AGB::system_loop() {
  Scheduler::schedule(Scheduler::EventType::HBLANK_START, 1006);
  Scheduler::schedule(Scheduler::EventType::VBLANK, 197120);
  while (active) {
    auto cycles = cpu.step();
    Scheduler::step(*this, cycles);
    tick_timers(cycles);
    check_for_dma();
  }
}

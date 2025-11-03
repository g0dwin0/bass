#include "agb.hpp"

#include "../../include/core/flash.hpp"
#include "bus.hpp"
#include "common/defs.hpp"
#include "enums.hpp"
#include "sched/sched.hpp"
AGB::AGB() {
  bus.pak        = &pak;
  bus.cpu        = &cpu;
  cpu.bus        = &bus;
  bus.ppu        = &ppu;
  ppu.bus        = &bus;
  bus.apu        = &apu;
  Scheduler::agb = this;

  pak.flash_controller.SRAM = &pak.SRAM;

  for (u8 i = 0; i < 4; i++) {
    dma_channels[i] = std::make_shared<DMAContext>(&bus);
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

  // Scheduler::schedule(Scheduler::EventType::HBLANK_START, 1006);
  // Scheduler::schedule(Scheduler::EventType::VBLANK, 197120);
}

void AGB::check_for_dma() const {
  for (auto& ch : dma_channels) {
    if (ch->enabled() && start_timing_cond_met(ch.get())) ch->process();
  }
};

bool AGB::start_timing_cond_met(DMAContext* ch) const {
  switch (ch->dmacnt_h.start_timing) {
    case DMA_START_TIMING::IMMEDIATELY: return true;
    case DMA_START_TIMING::VBLANK: return ppu.display_fields.DISPSTAT.VBLANK_FLAG;
    case DMA_START_TIMING::HBLANK: return ppu.display_fields.DISPSTAT.HBLANK_FLAG;
    case DMA_START_TIMING::SPECIAL: {
      return false;
      switch (ch->id) {
        case 0: return false;
        case 1: return false;  // FIFO req
        case 2: return false;  // FIFO req
        case 3: return false;
        default: assert(0);
      }
    }
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
    u32 cycles = 0;

    cycles += cpu.step();

    check_for_dma();
    Scheduler::step(cycles);
    // tick_timers(cycles);
  }
}

void AGB::step() {
  u32 cycles = 0;

  cycles += cpu.step();

  Scheduler::step(cycles);
  // tick_timers(cycles);
  check_for_dma();
}
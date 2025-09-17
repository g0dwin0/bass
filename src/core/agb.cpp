#include "agb.hpp"

#include "bus.hpp"
#include "common/defs.hpp"
#include "enums.hpp"

AGB::AGB() {
  bus.pak = &pak;
  bus.cpu = &cpu;
  cpu.bus = &bus;
  bus.ppu = &ppu;
  ppu.bus = &bus;

  for (u8 i = 0; i < 4; i++) {
    dma_channels[i] = new DMAContext(&bus);
  }
  

  bus.ch0 = dma_channels[0];
  bus.ch1 = dma_channels[1];
  bus.ch2 = dma_channels[2];
  bus.ch3 = dma_channels[3];
}

AGB::~AGB() {
  for (u8 i = 0; i < 4; i++) {
    delete dma_channels[i];
  }
  fmt::println("destroyed dma channels");
}

void AGB::set_ppu_interrupts() {
  // GFX Interrupt Logic

  if (((bus.cycles_elapsed % 197120) == 0 && bus.cycles_elapsed != 0)) {  // Check for VBLANK
    bus.display_fields.DISPSTAT.set_vblank();
    bus.ppu->db.swap_buffers();

    // stopwatch.end();
    // fmt::println("Frametime: {}ms", (stopwatch.duration.count()));

    if (bus.display_fields.DISPSTAT.VBLANK_IRQ_ENABLE) {
      bus.request_interrupt(INTERRUPT_TYPE::LCD_VBLANK);
    }
  }

  if ((bus.cycles_elapsed % 1006) == 0 && bus.cycles_elapsed != 0) {
    bus.display_fields.DISPSTAT.set_hblank();

    if (bus.display_fields.DISPSTAT.HBLANK_IRQ_ENABLE) {
      bus.request_interrupt(INTERRUPT_TYPE::LCD_HBLANK);
    }
  }

  if ((bus.cycles_elapsed % 1232) == 0 && bus.cycles_elapsed != 0) {  // check if we're on a new scanline
    bus.display_fields.DISPSTAT.reset_hblank();
    ppu.step();
    if (bus.display_fields.VCOUNT.LY == 227) {
      bus.display_fields.VCOUNT.LY = 0;
    } else {
      bus.display_fields.VCOUNT.LY++;
      if (bus.display_fields.VCOUNT.LY == 227) {
        bus.display_fields.DISPSTAT.reset_vblank();
      }
    }

    if (bus.display_fields.VCOUNT.LY == bus.display_fields.DISPSTAT.LYC) {
      bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = 1;

      if (bus.display_fields.DISPSTAT.V_COUNTER_IRQ_ENABLE) {
        bus.request_interrupt(INTERRUPT_TYPE::LCD_VCOUNT_MATCH);
      }

    } else {
      bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = 0;
    }
  }
}

void AGB::check_for_dma() {
  for (DMAContext* ch : dma_channels) {
    // assert(ch != nullptr);
    if (ch->enabled() && start_timing_cond_met(ch)) ch->process();
  }
};

bool AGB::start_timing_cond_met(DMAContext* ch) {
  switch (ch->dmacnt_h.start_timing) {
    case DMA_START_TIMING::IMMEDIATELY: return true;
    case DMA_START_TIMING::VBLANK: return bus.display_fields.DISPSTAT.VBLANK_FLAG;
    case DMA_START_TIMING::HBLANK: return bus.display_fields.DISPSTAT.HBLANK_FLAG;
    case DMA_START_TIMING::SPECIAL: return false;  // TODO: implement checking for special behaviour
  }

  assert(0);
}

void AGB::system_loop() {
  while (active) {
    cpu.step();
    set_ppu_interrupts();
    check_for_dma();
  }
}

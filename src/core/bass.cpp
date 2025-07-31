#include "bass.hpp"

#include "bus.hpp"
#include "common/defs.hpp"
#include "enums.hpp"

Bass::Bass() {
  bus.pak = &pak;
  bus.cpu = &cpu;
  cpu.bus = &bus;
  bus.ppu = &ppu;
  ppu.bus = &bus;
}

void Bass::set_ppu_interrupts() {
  // GFX Interrupt Logic

  if ((bus.cycles_elapsed % 197120 && bus.cycles_elapsed != 0) == 0) {  // Check for VBLANK
    bus.display_fields.DISPSTAT.set_vblank();
    // bus.ppu->db.swap_bufs();
    if (bus.display_fields.DISPSTAT.VBLANK_IRQ_ENABLE) { bus.request_interrupt(InterruptType::LCD_VBLANK); }
  }

  if ((bus.cycles_elapsed % 960 && bus.cycles_elapsed != 0) == 0) {
    
    bus.display_fields.DISPSTAT.set_hblank();
    // fmt::println("hblank set");

    if (bus.display_fields.DISPSTAT.HBLANK_IRQ_ENABLE) { bus.request_interrupt(InterruptType::LCD_HBLANK); }
  }

  if ((bus.cycles_elapsed % 1232) == 0 && bus.cycles_elapsed != 0) {  // Check if we're on a new scanline
    bus.display_fields.DISPSTAT.reset_hblank();
    ppu.step();
    if (bus.display_fields.VCOUNT.LY == 227) {
      bus.display_fields.VCOUNT.LY = 0;
      bus.display_fields.DISPSTAT.reset_vblank();
    } else {
      bus.display_fields.VCOUNT.LY++;
    }

    if (bus.display_fields.VCOUNT.LY == bus.display_fields.DISPSTAT.LYC) {
      bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = 1;

      if (bus.display_fields.DISPSTAT.V_COUNTER_IRQ_ENABLE) { bus.request_interrupt(InterruptType::LCD_VCOUNT_MATCH); }
    } else {
      bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = 0;
    }
  }
}

void Bass::system_loop() {
  while (active) {
    cpu.step();
    set_ppu_interrupts();
  }
}
#include "bass.hpp"

#include "bus.hpp"
#include "common/defs.hpp"

Bass::Bass() {
  bus.pak = &pak;
  cpu.bus = &bus;
  bus.ppu = &ppu;
  ppu.bus = &bus;

  cpu.initialize_lookup_table();
}


void Bass::check_and_handle_interrupts() {
  // GFX Interrupt Logic

  if ((bus.cycles_elapsed % 197120) == 0) {  // Check for VBLANK
    bus.display_fields.DISPSTAT.SET_VBLANK();
    // bus.ppu->db.swap_bufs();
    // if (bus.display_fields.DISPSTAT.VBLANK_IRQ_ENABLE) { bus.request_interrupt(Bus::LCD_VBLANK); }
  }

  if ((bus.cycles_elapsed % 960) == 0) {
    
    bus.display_fields.DISPSTAT.SET_HBLANK();
    // fmt::println("hblank set");

    if (bus.display_fields.DISPSTAT.HBLANK_IRQ_ENABLE) { bus.request_interrupt(Bus::LCD_HBLANK); }
  }

  if ((bus.cycles_elapsed % 1232) == 0) {  // Check if we're on a new scanline
    bus.display_fields.DISPSTAT.RESET_HBLANK();
    ppu.step();
    if (bus.display_fields.VCOUNT.LY == 227) {
      bus.display_fields.VCOUNT.LY = 0;
      bus.display_fields.DISPSTAT.RESET_VBLANK();
    } else {
      bus.display_fields.VCOUNT.LY++;
    }

    if (bus.display_fields.VCOUNT.LY == bus.display_fields.DISPSTAT.LYC) {
      bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = 1;

      if (bus.display_fields.DISPSTAT.V_COUNTER_IRQ_ENABLE) { bus.request_interrupt(Bus::LCD_VCOUNT_MATCH); }
    } else {
      bus.display_fields.DISPSTAT.VCOUNT_MATCH_FLAG = 0;
    }
  }
}

void Bass::system_loop() {
  while (active) {
    cpu.step();
    check_and_handle_interrupts();
  }
}
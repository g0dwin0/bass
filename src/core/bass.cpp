#include "bass.hpp"

Bass::Bass() {
  bus.pak = &pak;
  cpu.bus = &bus;
  bus.ppu = &ppu;
  ppu.bus = &bus;
}

void Bass::check_and_handle_interrupts() {

  // GFX Interrupt Logic
  
  if (bus.cycles_elapsed == 197120) { // Check for VBLANK
    ppu.draw();
    bus.display_fields.DISPSTAT.VBLANK_FLAG = 1;
  }

  if (bus.cycles_elapsed == 280896) { bus.display_fields.DISPSTAT.VBLANK_FLAG = 0; } // End of VBLANK, unset flag

  if (bus.cycles_elapsed % 1232 == 0) { // Check if we're on a new scanline
    if (bus.display_fields.VCOUNT.LY == 228) {
      bus.display_fields.VCOUNT.LY = 0;
    } else {
      bus.display_fields.VCOUNT.LY++;
    }
  }



}
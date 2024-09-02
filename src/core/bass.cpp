#include "bass.hpp"
#include "spdlog/spdlog.h"

Bass::Bass() {
    bus.pak = &pak;
    cpu.bus = &bus;
    bus.ppu = &ppu;
    ppu.bus = &bus;
}
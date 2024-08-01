#include "bass.hpp"
#include "spdlog/spdlog.h"

Bass::Bass() {
    bus.pak = &pak;
    cpu.bus = &bus;
    bus.ppu = &ppu;
    spdlog::debug("pointers to device set");
}
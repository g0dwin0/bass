#include "bass.hpp"
#include "spdlog/spdlog.h"

Bass::Bass() {
    spdlog::debug("setting pak ptr");
    bus.pak = &pak;
    spdlog::debug("setting bus ptr");
    cpu.bus = &bus;
}
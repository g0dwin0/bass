#pragma once

// #include "bus.hpp"
#include "common.hpp"

enum class DST_CONTROL { INCREMENT = 0, DECREMENT, FIXED, INCREMENT_RELOAD };
enum class SRC_CONTROL  { INCREMENT = 0, DECREMENT, FIXED, PROHIBITED };
enum class TRANSFER_TYPE { HALFWORD = 0, WORD };
enum class DMA_TIMING { IMMEDIATELY = 0, VBLANK, HBLANK, SPECIAL };

inline std::unordered_map<DMA_TIMING, std::string> TIMING_MAP = {
    {DMA_TIMING::IMMEDIATELY, "IMMEDIATELY"},
    {     DMA_TIMING::VBLANK,      "VBLANK"},
    {     DMA_TIMING::HBLANK,      "HBLANK"},
    {    DMA_TIMING::SPECIAL,     "SPECIAL"},
};

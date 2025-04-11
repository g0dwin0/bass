#pragma once
#include <spdlog/common.h>

#include <string>
#include <unordered_map>

#include "common/defs.hpp"
// DMA
enum class DST_CONTROL { INCREMENT = 0, DECREMENT, FIXED, INCREMENT_RELOAD };
enum class SRC_CONTROL { INCREMENT = 0, DECREMENT, FIXED, PROHIBITED };
enum class TRANSFER_TYPE { HALFWORD = 0, WORD };
enum DMA_TIMING { IMMEDIATELY = 0, VBLANK, HBLANK, SPECIAL };

inline std::unordered_map<u8, std::string> TIMING_MAP = {
    {DMA_TIMING::IMMEDIATELY, "IMMEDIATELY"},
    {     DMA_TIMING::VBLANK,      "VBLANK"},
    {     DMA_TIMING::HBLANK,      "HBLANK"},
    {    DMA_TIMING::SPECIAL,     "SPECIAL"},
};

union DMACNT_L {
  u16 v = 0;
  struct {
    u16 word_count;
  };
};

union DMACNT_H {
  u16 v = 0;
  struct {
    u8                          : 5;
    DST_CONTROL dst_control     : 2;
    SRC_CONTROL src_control     : 2;
    u8 dma_repeat               : 1;
    TRANSFER_TYPE transfer_type : 1;
    u8 game_pak_drq             : 1;
    u8 start_timing             : 2;
    u8 irq_at_end               : 1;
    u8 dma_enable               : 1;
  };
};

union DMADAD {
  u32 v = 0;
  struct {
    u32 dst : 28;
    u8      : 4;
  };
};

union DMASAD {
  u32 v = 0;
  struct {
    u32 src : 28;
    u8      : 4;
  };
};

struct DMAContext {
  DMADAD dma_dad;
  DMASAD dma_sad;
  DMACNT_L dmacnt_l;
  DMACNT_H dmacnt_h;

  DMAContext() = delete;
  void print_dma_info() {
    fmt::println("============ DMA0 INFO ============");
    fmt::println("src: {:#010x}", +dma_sad.src);
    fmt::println("dst: {:#010x}", +dma_dad.dst);
    fmt::println("timing: {}", TIMING_MAP.at(dmacnt_h.start_timing));
    fmt::println("word count: {:#010x}", dmacnt_l.word_count);
    fmt::println("word size: {}", dmacnt_h.transfer_type == TRANSFER_TYPE::HALFWORD ? "16bits" : "32bits");
    fmt::println("===================================");
    // break;
  }
};
#pragma once

#include "bus.hpp"

enum class DST_CONTROL : u8 { INCREMENT = 0, DECREMENT, FIXED, INCREMENT_RELOAD };
enum class SRC_CONTROL : u8 { INCREMENT = 0, DECREMENT, FIXED, PROHIBITED };
enum class TRANSFER_TYPE : u8 { HALFWORD = 0, WORD };
enum class DMA_START_TIMING : u8 { IMMEDIATELY = 0, VBLANK, HBLANK, SPECIAL };

const std::unordered_map<DMA_START_TIMING, std::string> TIMING_MAP = {
    {DMA_START_TIMING::IMMEDIATELY, "IMMEDIATELY"},
    {     DMA_START_TIMING::VBLANK,      "VBLANK"},
    {     DMA_START_TIMING::HBLANK,      "HBLANK"},
    {    DMA_START_TIMING::SPECIAL,     "SPECIAL"},
};

union DMACNT_L {
  u32 v = 0;
  struct {
    u32 word_count;
  };
};

struct DMACNT_H {  // packed manually because of alignment issues -> dmacnt_h_values
  u8                            : 4;
  DST_CONTROL dst_control       : 2;
  SRC_CONTROL src_control       : 2;
  u8 dma_repeat                 : 1;
  TRANSFER_TYPE transfer_type   : 1;
  u8 game_pak_drq               : 1;
  DMA_START_TIMING start_timing : 2;
  u8 irq_at_end                 : 1;
  bool dma_enable               : 1;
};

static u8 dma_ctx_id = 0;

struct DMAContext {
  u8 id    = 0;
  Bus* bus = nullptr;

  u32 src = 0;  // DMASAD - forward facing
  u32 dst = 0;  // DMADAD - forward facing

  u32 internal_src = 0;  // internal -- reload depending on addr control bit
  u32 internal_dst = 0;  // internal -- reload depending on addr control bit

  DMACNT_L dmacnt_l = {};
  DMACNT_H dmacnt_h = {};

  DMAContext(Bus* bus_ptr) : id(dma_ctx_id++), bus(bus_ptr) {
    assert(bus != nullptr);
    assert(dma_ctx_id <= 4);
    fmt::println("created dma channel {}", id);
  };

  std::shared_ptr<spdlog::logger> dma_logger = spdlog::stdout_color_mt(fmt::format("DMA{}", id));

  void print_dma_info();
  void process();
  bool enabled();

  void set_values_cnt_h(u8 byte_index, u16 value);
  u8 get_values_cnt_h(u8 byte_index);

  void transfer16(const u32 src, const u32 dst, u32 word_count);
  void transfer32(const u32 src, const u32 dst, u32 word_count);
};

#pragma once

struct Bus;
#include "bus.hpp"
#include "common/defs.hpp"

enum class SRC_CONTROL : u16 { INCREMENT = 0, DECREMENT, FIXED, PROHIBITED };
enum class DST_CONTROL : u16 { INCREMENT = 0, DECREMENT, FIXED, INCREMENT_RELOAD };
enum class TRANSFER_SIZE : u16 { HALFWORD = 0, WORD };
enum class DMA_START_TIMING : u16 { IMMEDIATELY = 0, VBLANK, HBLANK, SPECIAL };

const std::unordered_map<DMA_START_TIMING, std::string> TIMING_MAP = {
    {DMA_START_TIMING::IMMEDIATELY, "IMMEDIATELY"},
    {     DMA_START_TIMING::VBLANK,      "VBLANK"},
    {     DMA_START_TIMING::HBLANK,      "HBLANK"},
    {    DMA_START_TIMING::SPECIAL,     "SPECIAL"},
};

const std::unordered_map<SRC_CONTROL, std::string> SRC_CTRL_MAP = {
    { SRC_CONTROL::INCREMENT,  "INCREMENT"},
    { SRC_CONTROL::DECREMENT,  "DECREMENT"},
    {     SRC_CONTROL::FIXED,      "FIXED"},
    {SRC_CONTROL::PROHIBITED, "PROHIBITED"},
};

const std::unordered_map<DST_CONTROL, std::string> DST_CTRL_MAP = {
    {       DST_CONTROL::INCREMENT,        "INCREMENT"},
    {       DST_CONTROL::DECREMENT,        "DECREMENT"},
    {           DST_CONTROL::FIXED,            "FIXED"},
    {DST_CONTROL::INCREMENT_RELOAD, "INCREMENT RELOAD"},
};

const std::unordered_map<u8, std::string> SPECIAL_COND_MAP = {
    {0,    "Prohibited"},
    {1,    "Sound FIFO"},
    {2,    "Sound FIFO"},
    {3, "Video Capture"},
};

union DMACNT_L {
  u32 v = 0;
  struct {
    u32 word_count;
  };
};

union DMACNT_H {
  u16 v;
  struct {
    u16                           : 5;
    DST_CONTROL dst_control       : 2;
    SRC_CONTROL src_control       : 2;
    bool dma_repeat               : 1;
    TRANSFER_SIZE transfer_type   : 1;
    bool game_pak_drq             : 1;
    DMA_START_TIMING start_timing : 2;
    bool irq_at_end               : 1;
    bool dma_enable               : 1;
  };
};

static u8 dma_ctx_id;

struct DMAContext {
  u8 id            = 0;
  Bus* bus         = nullptr;
  u32 dma_open_bus = 0;
  enum class OPEN_BUS_WIDTH { HALFWORD, WORD } open_bus_size;
  u32 src = 0;  // DMASAD - forward facing (this value gets loaded into internal)
  u32 dst = 0;  // DMADAD - forward facing (this value gets loaded into internal)

  u32 internal_src       = 0;  // internal -- reload depending on addr control bit
  u32 internal_dst       = 0;  // internal -- reload depending on addr control bit
  u32 internal_word_size = 0;

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

  void transfer16(u32 src, u32 dst, u32 word_count);
  void transfer32(u32 src, u32 dst, u32 word_count);

  static constexpr std::array<u32, 4> DMA_SRC_MASK    = {0x07FFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF};
  static constexpr std::array<u32, 4> DMA_DST_MASK    = {0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x0FFFFFFF};
  static constexpr std::array<u32, 4> WORD_COUNT_MASK = {0x3FFF, 0x3FFF, 0x3FFF, 0xFFFF};
};
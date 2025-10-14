#include "dma.hpp"

#include "common/align.hpp"

void DMAContext::print_dma_info() {
  dma_logger->info("============ DMA{} INFO ============", id);
  dma_logger->info("src: {:#010x}", src);
  dma_logger->info("dst: {:#010x}", dst);
  dma_logger->info("dst address control: {}", DST_CTRL_MAP.at(dmacnt_h.dst_control));
  dma_logger->info("src address control: {}", SRC_CTRL_MAP.at(dmacnt_h.src_control));
  dma_logger->info("timing: {} {}", TIMING_MAP.at(static_cast<DMA_START_TIMING>(dmacnt_h.start_timing)), dmacnt_h.start_timing == DMA_START_TIMING::SPECIAL ? SPECIAL_COND_MAP.at(id) : "");
  dma_logger->info("word count: {:#010x}", dmacnt_l.word_count);
  dma_logger->info("word size: {}", dmacnt_h.transfer_type == TRANSFER_SIZE::HALFWORD ? "16bits" : "32bits");
  dma_logger->info("repeating?: {}", dmacnt_h.dma_repeat ? "Yes" : "No");
  dma_logger->info("===================================");
}

void DMAContext::process() {
  // print_dma_info();

  if (this->dmacnt_h.dma_repeat) {
    // internal_src = src;
    // internal_dst = dst;
    internal_word_size &= WORD_COUNT_MASK[id];
    if (this->dmacnt_h.dst_control == DST_CONTROL::INCREMENT_RELOAD) {
      internal_dst = dst;
    }
  }

  if (dmacnt_h.transfer_type == TRANSFER_SIZE::HALFWORD) {
    transfer16(internal_src, internal_dst, internal_word_size);
  } else {
    transfer32(internal_src, internal_dst, internal_word_size);
  }

  dmacnt_h.dma_enable = dmacnt_h.dma_repeat;

  if (dmacnt_h.irq_at_end) {
    switch (id) {
      case 0: bus->request_interrupt(INTERRUPT_TYPE::DMA0); break;
      case 1: bus->request_interrupt(INTERRUPT_TYPE::DMA1); break;
      case 2: bus->request_interrupt(INTERRUPT_TYPE::DMA2); break;
      case 3: bus->request_interrupt(INTERRUPT_TYPE::DMA3); break;
      default: assert(0);
    }
  }
};

bool DMAContext::enabled() { return dmacnt_h.dma_enable; }

void DMAContext::transfer16(u32 _src, u32 _dst, u32 word_count) {
  if (word_count == 0 && id != 3) word_count = 0x4000;
  if (word_count == 0 && id == 3) word_count = 0x10000;

  u16 value = 0;

  internal_src = align(_src, HALFWORD);
  internal_dst = align(_dst, HALFWORD);

  auto access_type = ACCESS_TYPE::NON_SEQUENTIAL;

  for (size_t idx = 0; idx < word_count; idx++) {
    idx == 0 ? access_type = ACCESS_TYPE::NON_SEQUENTIAL : access_type = ACCESS_TYPE::SEQUENTIAL;
    if ((internal_src & DMA_SRC_MASK[id]) <= 0x1FFFFFF) {
      value = dma_open_bus;
    } else {
      value = bus->read16(internal_src & DMA_SRC_MASK[id], access_type);

      dma_open_bus  = value;
      open_bus_size = OPEN_BUS_WIDTH::HALFWORD;
    }

    if (!((src & DMA_SRC_MASK[id]) >= 0X0E000000 && id == 0)) {
      bus->write16(internal_dst & DMA_DST_MASK[id], value);
    }

    switch (dmacnt_h.dst_control) {
      case DST_CONTROL::INCREMENT: internal_dst += 2; break;
      case DST_CONTROL::DECREMENT: internal_dst -= 2; break;
      case DST_CONTROL::FIXED: break;
      case DST_CONTROL::INCREMENT_RELOAD: internal_dst += 2; break;
    }

    if (src >= 0x08000000 && src < 0x0E000000) {
      if (dmacnt_h.src_control == SRC_CONTROL::FIXED) internal_src += 2;
      if (dmacnt_h.src_control == SRC_CONTROL::DECREMENT) internal_src += 4;
    }

    switch (dmacnt_h.src_control) {
      case SRC_CONTROL::INCREMENT: internal_src += 2; break;
      case SRC_CONTROL::DECREMENT: internal_src -= 2; break;
      case SRC_CONTROL::FIXED: break;
      case SRC_CONTROL::PROHIBITED: assert(0);
    }
  }
}

void DMAContext::transfer32(u32 _src, u32 _dst, u32 word_count) {
  if (word_count == 0 && id != 3) word_count = 0x4000;
  if (word_count == 0 && id == 3) word_count = 0x10000;

  internal_src = align(_src, WORD);
  internal_dst = align(_dst, WORD);

  u32 value = 0;

  for (size_t idx = 0; idx < word_count; idx++) {
    if ((internal_src & DMA_SRC_MASK[id]) <= 0x1FFFFFF) {
      if (open_bus_size == OPEN_BUS_WIDTH::HALFWORD) {
        value = (dma_open_bus << 16) | dma_open_bus;
      } else {
        value = dma_open_bus;
      }

    } else {
      value = bus->read32(internal_src & DMA_SRC_MASK[id]);

      dma_open_bus  = value;
      open_bus_size = OPEN_BUS_WIDTH::WORD;
    }

    if (!((internal_src & DMA_SRC_MASK[id]) >= 0X0E000000 && id == 0)) {
      bus->write32(internal_dst & DMA_DST_MASK[id], value);
    }

    switch (dmacnt_h.dst_control) {
      case DST_CONTROL::INCREMENT: internal_dst += 4; break;
      case DST_CONTROL::DECREMENT: internal_dst -= 4; break;
      case DST_CONTROL::FIXED: break;
      case DST_CONTROL::INCREMENT_RELOAD: break;
    }

    if (src >= 0x08000000 && src < 0x0E000000) {
      if (dmacnt_h.src_control == SRC_CONTROL::FIXED) internal_src += 4;
      if (dmacnt_h.src_control == SRC_CONTROL::DECREMENT) internal_src += 8;
    }

    switch (dmacnt_h.src_control) {
      case SRC_CONTROL::INCREMENT: internal_src += 4; break;
      case SRC_CONTROL::DECREMENT: internal_src -= 4; break;
      case SRC_CONTROL::FIXED: break;
      case SRC_CONTROL::PROHIBITED: assert(0);
    }
  }
}
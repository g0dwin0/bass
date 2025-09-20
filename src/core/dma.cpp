#include "dma.hpp"

#include "common/align.hpp"

void DMAContext::print_dma_info() {
  dma_logger->info("============ DMA{} INFO ============", id);
  dma_logger->info("src: {:#010x}", src);
  dma_logger->info("dst: {:#010x}", dst);
  dma_logger->info("dst address control: {}", DST_CTRL_MAP.at(dmacnt_h.dst_control));
  dma_logger->info("src address control: {}", SRC_CTRL_MAP.at(dmacnt_h.src_control));
  dma_logger->info("timing: {}", TIMING_MAP.at(static_cast<DMA_START_TIMING>(dmacnt_h.start_timing)));
  dma_logger->info("word count: {:#010x}", dmacnt_l.word_count);
  dma_logger->info("word size: {}", dmacnt_h.transfer_type == TRANSFER_TYPE::HALFWORD ? "16bits" : "32bits");
  dma_logger->info("===================================");
}

void DMAContext::process() {
  // print_dma_info();

  internal_src = src;
  internal_dst = dst;

  dmacnt_l.word_count &= WORD_COUNT_MASK[id];

  if (dmacnt_h.transfer_type == TRANSFER_TYPE::HALFWORD) {
    transfer16(internal_src, internal_dst, dmacnt_l.word_count);
  } else {
    transfer32(internal_src, internal_dst, dmacnt_l.word_count);
  }

  dmacnt_h.dma_enable = false;

  // if (dmacnt_h.dma_repeat) {
  //   // reload cnt_l & dad (if increment+reload is set)
  // }

  if (dmacnt_h.irq_at_end) {
    switch (id) {
      case 0: bus->request_interrupt(INTERRUPT_TYPE::DMA0); break;
      case 1: bus->request_interrupt(INTERRUPT_TYPE::DMA1); break;
      case 2: bus->request_interrupt(INTERRUPT_TYPE::DMA2); break;
      case 3: bus->request_interrupt(INTERRUPT_TYPE::DMA3); break;
      default: assert(0);
    }
  }
  // }
};

bool DMAContext::enabled() { return dmacnt_h.dma_enable; }

void DMAContext::transfer16(u32 src, u32 dst, u32 word_count) {
  if (word_count == 0 && id != 3) word_count = 0x4000;
  if (word_count == 0 && id == 3) word_count = 0x10000;

  u16 value = 0;

  src = align(src, HALFWORD);
  dst = align(dst, HALFWORD);

  for (size_t idx = 0; idx < word_count; idx++) {
    if ((src & DMA_SRC_MASK[id]) < 0x02000000) {
      value = dma_open_bus;
    } else {
      if (src > 0x07FFFFFF && id == 0) {
        value = 0;
      } else {
        value = bus->read16(src & DMA_SRC_MASK[id]);
      }

      dma_open_bus = value;
    }

    bus->write16(dst & DMA_DST_MASK[id], value);

    switch (dmacnt_h.dst_control) {
      case DST_CONTROL::INCREMENT: dst += 2; break;
      case DST_CONTROL::DECREMENT: dst -= 2; break;
      case DST_CONTROL::FIXED: break;
      case DST_CONTROL::INCREMENT_RELOAD: break; ;
    }

    if (src >= 0x08000000 && dmacnt_h.src_control != SRC_CONTROL::INCREMENT) src += 2;

    switch (dmacnt_h.src_control) {
      case SRC_CONTROL::INCREMENT: src += 2; break;
      case SRC_CONTROL::DECREMENT: src -= 2; break;
      case SRC_CONTROL::FIXED: break;
      case SRC_CONTROL::PROHIBITED: assert(0);
    }
  }
}

void DMAContext::transfer32(u32 src, u32 dst, u32 word_count) {
  if (word_count == 0 && id != 3) word_count = 0x4000;
  if (word_count == 0 && id == 3) word_count = 0x10000;

  src = align(src, WORD);
  dst = align(dst, WORD);

  u32 value = 0;

  for (size_t idx = 0; idx < word_count; idx++) {
    if ((src & DMA_SRC_MASK[id]) <= 0x1FFFFFF) {
      value = dma_open_bus;
    } else {
      value = bus->read32(src & DMA_SRC_MASK[id]);

      if (src > 0x07FFFFFF && id == 0) {  // DMA0 (27 bits) cannot read this high
        value = 0;
      } else {
        dma_open_bus = value;
      }
    }

    bus->write32(dst & DMA_DST_MASK[id], value);

    switch (dmacnt_h.src_control) {
      case SRC_CONTROL::INCREMENT: src += 4; break;
      case SRC_CONTROL::DECREMENT: src -= 4; break;
      case SRC_CONTROL::FIXED: break;
      case SRC_CONTROL::PROHIBITED: assert(0);
    }

    switch (dmacnt_h.dst_control) {
      case DST_CONTROL::INCREMENT: dst += 4; break;
      case DST_CONTROL::DECREMENT: dst -= 4; break;
      case DST_CONTROL::FIXED: break;
      case DST_CONTROL::INCREMENT_RELOAD: assert(0);
    }
  }
}
#include "dma.hpp"

void DMAContext::print_dma_info() {
  dma_logger->info("============ DMA{} INFO ============", id);
  dma_logger->info("src: {:#010x}", src);
  dma_logger->info("dst: {:#010x}", dst);
  dma_logger->info("timing: {}", TIMING_MAP.at(static_cast<DMA_START_TIMING>(dmacnt_h.start_timing)));
  dma_logger->info("word count: {:#010x}", dmacnt_l.word_count);
  dma_logger->info("word size: {}", dmacnt_h.transfer_type == TRANSFER_TYPE::HALFWORD ? "16bits" : "32bits");
  dma_logger->info("===================================");
}

void DMAContext::process() {
  if (dmacnt_h.dma_enable) {
    // fmt::println("dma{} fired", id);

    print_dma_info();

    if (dmacnt_h.transfer_type == TRANSFER_TYPE::HALFWORD) {
      transfer16(src, dst, dmacnt_l.word_count);
    } else {
      transfer32(src, dst, dmacnt_l.word_count);
    }

    if (!dmacnt_h.dma_repeat) dmacnt_h.dma_enable = false;
    if (dmacnt_h.dma_repeat) {
      // reload cnt_l & dad (if increment+reload is set)
    }

    if (dmacnt_h.irq_at_end) {
      switch (id) {
        case 0: bus->request_interrupt(InterruptType::DMA0); break;
        case 1: bus->request_interrupt(InterruptType::DMA1); break;
        case 2: bus->request_interrupt(InterruptType::DMA2); break;
        case 3: bus->request_interrupt(InterruptType::DMA3); break;
        default: assert(0);
      }
    }
  }
};

bool DMAContext::enabled() { return dmacnt_h.dma_enable; }

void DMAContext::set_values_cnt_h(u8 byte_index, u16 value) {
  if (byte_index == 0) {
    dmacnt_h.dst_control = static_cast<DST_CONTROL>((value & 0b1100000) >> 5);
    dmacnt_h.src_control = static_cast<SRC_CONTROL>(((value & 0b10000000) >> 7) | (u8)dmacnt_h.src_control);

  } else {
    dmacnt_h.src_control   = static_cast<SRC_CONTROL>(((value & 1)) | ((u8)dmacnt_h.src_control << 1));
    value <<= 8;

    dmacnt_h.dma_repeat    = static_cast<u8>((value & (1 << 9)) >> 9);
    dmacnt_h.transfer_type = static_cast<TRANSFER_TYPE>((value & (1 << 10)) >> 10);

    if(id == 3) dmacnt_h.game_pak_drq  = static_cast<bool>((value & (1 << 11)) >> 11); // game pak drq is dma3 only
    dmacnt_h.start_timing  = static_cast<DMA_START_TIMING>((value & (0b11 << 12)) >> 12);
    dmacnt_h.irq_at_end    = static_cast<bool>((value & (1 << 14)) >> 14);
    dmacnt_h.dma_enable    = static_cast<bool>((value & (1 << 15)) >> 15) ? true : false;
  }
}

u8 DMAContext::get_values_cnt_h(u8 byte_index) {
  u16 v = 0;

  v |= ((u8)dmacnt_h.dst_control << 5);
  v |= ((u8)dmacnt_h.src_control << 7);
  v |= ((u8)dmacnt_h.dma_repeat << 9);
  v |= ((u8)dmacnt_h.transfer_type << 10);
  v |= ((u8)dmacnt_h.game_pak_drq << 11);
  v |= ((u8)dmacnt_h.start_timing << 12);
  v |= ((u8)dmacnt_h.irq_at_end << 14);
  v |= ((u8)dmacnt_h.dma_enable << 15);

  if (byte_index == 0) {
    return v & 0xFF;
  } else {
    return (v & 0xFF00) >> 8;
  }

}

void DMAContext::transfer16(const u32 src, const u32 dst, u32 word_count) {
  if (word_count == 0 && id != 3) word_count = 0x4000;
  if (word_count == 0 && id == 3) word_count = 0x10000;

  for (size_t idx = 0; idx < word_count; idx++) {
    bus->write16(dst + (idx * 2), bus->read16(src + (idx * 2)));
  }
}

void DMAContext::transfer32(const u32 src, const u32 dst, u32 word_count) {
  if (word_count == 0 && id != 3) word_count = 0x4000;
  if (word_count == 0 && id == 3) word_count = 0x10000;

  for (size_t idx = 0; idx < word_count; idx++) {
    bus->write32(dst + (idx * 4), bus->read32(src + (idx * 4)));
  }
}
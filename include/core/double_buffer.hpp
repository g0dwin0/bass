#pragma once

#include <condition_variable>
#include <mutex>
#include "common/defs.hpp"

struct DoubleBuffer {
    u32* write_buf = nullptr;
    u32* disp_buf  = nullptr;

   public:
    DoubleBuffer() = delete;
    DoubleBuffer(u32* _write_buf, u32* _disp_buf) : write_buf(_write_buf), disp_buf(_disp_buf) {}


    void write(size_t idx, u32 value);
    void swap_buffers();
  };
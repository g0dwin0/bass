#include "double_buffer.hpp"
#include <cassert>
#include <cstring>

void DoubleBuffer::write(size_t idx, u32 value) {
  assert(idx < 241*160);
  write_buf[idx] = value;
}
void DoubleBuffer::swap_buffers() {
  std::swap(write_buf, disp_buf);
};
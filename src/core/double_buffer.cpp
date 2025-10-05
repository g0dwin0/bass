#include "double_buffer.hpp"

#include <cassert>
#include <utility>

void DoubleBuffer::write(const size_t idx, const u32 value) {
  assert(idx < 241 * 160);
  write_buf[idx] = value;
}
void DoubleBuffer::swap_buffers() { std::swap(write_buf, disp_buf); };
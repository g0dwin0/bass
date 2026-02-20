#include "double_buffer.hpp"

#include <cassert>
#include <utility>

void DoubleBuffer::write(const size_t idx, const u32 value) {
  assert(idx < 240 * 160);
  write_buf[idx] = value;
}
// TODO(multithread): needs a lock guard, pointers could be swapped while frontend thread is 
// executing SDL_RenderCopy leading to tearing
void DoubleBuffer::swap_buffers() { std::swap(write_buf, disp_buf); };
#pragma once
#include <queue>

#include "common/defs.hpp"

struct APU {
  std::queue<i8> FIFO_A, FIFO_B;

  // u8 get_queue_size() {
  //   FIFO_A.size();
  // }
};
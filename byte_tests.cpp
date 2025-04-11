
#include <array>
#include <cstdio>
#include <tuple>
#include "common/bytes.hpp"

// template <typename T>
// void set_byte(T& r, u8 byte_idx, u8 v) {
//   static_assert(std::is_arithmetic<T>::value, "Invalid type");

//   r &= ~(0xFF << byte_idx * 8);
//   r |= (v << byte_idx * 8);
// };

std::array<std::tuple<u16, u16, u8, u8>, 8> UNSIGNED_SHORTS = {
    {
     {0xFFFFFFFF, 0xFFFFFF00, 0, 0},
     {0xFFFFFFFF, 0xFFFF00FF, 1, 0},
     {0xFFFFFFFF, 0xFF00FFFF, 2, 0},
     {0xFFFFFFFF, 0x00FFFFFF, 3, 0},

     {0xFFFFFFFF, 0xFFFFFF12, 0, 0x12},
     {0xFFFFFFFF, 0xFFFF12FF, 1, 0x12},
     {0xFFFFFFFF, 0xFF12FFFF, 2, 0x12},
     {0xFFFFFFFF, 0x55FFFFFF, 3, 0x55},

     },
};

std::array<std::tuple<u32, u32, u32, u32>, 8> UNSIGNED_INTEGERS = {
    {
     {0xFFFFFFFF, 0xFFFFFF00, 0, 0},
     {0xFFFFFFFF, 0xFFFF00FF, 1, 0},
     {0xFFFFFFFF, 0xFF00FFFF, 2, 0},
     {0xFFFFFFFF, 0x00FFFFFF, 3, 0},

     {0xFFFFFFFF, 0xFFFFFF12, 0, 0x12},
     {0xFFFFFFFF, 0xFFFF12FF, 1, 0x12},
     {0xFFFFFFFF, 0xFF12FFFF, 2, 0x12},
     {0xFFFFFFFF, 0x55FFFFFF, 3, 0x55},

     },
};

std::array<std::tuple<u32, u8, u8>, 4> UNSIGNED_INTEGERS_READ = {
    {
     {0xFFFFFF02, 0x02, 0},
     {0xFFFF00FF, 0xFF, 0},
     {0xAB0000FF, 0xAB, 3},
     {0x0012FFFF, 0x12, 2},
     },
};

int main() {
  // for (size_t i = 0; i < UNSIGNED_INTEGERS.size(); i++) {
  //   const auto [initial, result, byte_index, value] = UNSIGNED_INTEGERS[i];
  //   printf("[IDX: %zu]\n", i);
  //   printf("\ninitial:    %08X\n", initial);
  //   auto c = initial;
  //   set_byte(c, byte_index, value);

  //   printf("ACTUAL:     %08X\n", c);
  //   printf("TARGET:     %08X\n", result);

  //   assert(c == result);
  // }
  // printf("all good.\n");

  for (size_t i = 0; i < UNSIGNED_INTEGERS_READ.size(); i++) {
    const auto [initial, result, byte_index] = UNSIGNED_INTEGERS_READ[i];
    printf("[IDX: %zu]\n", i);
    printf("\ninitial:    %08X\n", initial);
    printf("byte index: %d\n", byte_index);
    // auto c = initial;
    
    auto c = read_byte(initial, byte_index);

    printf("ACTUAL:     %08X\n", c);
    printf("TARGET:     %08X\n", result);

    assert(c == result);
  }

  return 0;
}

// auto b = set_byte<u16>(1, 2);
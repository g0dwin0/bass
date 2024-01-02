#include "core/cart.hpp"

#include "common.hpp"
#include "io.hpp"

using namespace Bass;
void Cart::load_data(File& f) {
  std::copy(f.data.begin(), f.data.end(), data.memory);
};
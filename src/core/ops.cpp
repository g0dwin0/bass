#include "ops.hpp"
#include <stdexcept>
#define UNIMPL throw std::runtime_error(fmt::format("UNIMPL {}: {}", __func__, __LINE__));

void Bass::ARM7::Instructions::AND(ARM7TDMI&) {
    
    UNIMPL
};

/* Single Data Swap */
void SWP() {
  UNIMPL  
};

void SWI() {
    throw std::runtime_error("unhandled SWI");
};
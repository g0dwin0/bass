#pragma once

#include "common.hpp"
#include "io.hpp"

namespace Bass {
  struct Cart {

    union {
      u8 memory[0xC000000];
      struct {
        u32 entry_point; 
        u8 logo[156];
        char game_title[12];
        char game_code[4];
        char maker_code[2];
        u8 unused_0;
        u8 unit_code;
        u8 device_type;
        u8 reserved[7];
        u8 software_version;
        u8 complement_check;
        u8 reserved_[2];
      } info;
    } data;


    void load_data(File&);
  };
};  // namespace Bass
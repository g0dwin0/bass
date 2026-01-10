#include "eeprom.hpp"

#include <algorithm>
#include <cstdlib>

void EEPROMController::handle_write(u8 v) {
  switch (mode) {
    case EEPROM_MODE::READ_REQUEST: {
      cmd_bit_vec.push_back(v & 1);

      switch (read_submode) {
        case EEPROM_READ_SUB_MODE::WAIT_FOR_ADDRESS: {
          eeprom_logger->info(": {}", cmd_bit_vec.size());
          if (cmd_bit_vec.size() == ADDRESS_SIZE) {
            selected_block = 0;
            u8 idx         = ADDRESS_SIZE;  // 6 or 14 bits (ambiguous till first DMA)
            for (const auto& bit : cmd_bit_vec) {
              idx--;
              selected_block |= bit << idx;
            }
            
            eeprom_logger->info("idx:{}", idx);
            assert(idx == 0);

            eeprom_logger->info("read req: set selected block: {}", selected_block);
            cmd_bit_vec.clear();
            read_submode = EEPROM_READ_SUB_MODE::WAIT_FOR_END_BIT;
          }
          break;
        }
        case EEPROM_READ_SUB_MODE::WAIT_FOR_END_BIT: {
          eeprom_logger->info("v: {}", v);
          // assert(v == 0);
          eeprom_logger->info("read req finished, going back to mode NONE");

          bits_to_read = 68;

          cmd_bit_vec.clear();
          mode         = EEPROM_MODE::NONE;
          read_submode = EEPROM_READ_SUB_MODE::WAIT_FOR_ADDRESS;
          break;
        }
      }

      break;
    }
    case EEPROM_MODE::WRITE_REQUEST: {
      cmd_bit_vec.push_back(v & 1);
      switch (submode) {
        case EEPROM_WRITE_SUB_MODE::WAIT_FOR_ADDRESS: {
          eeprom_logger->info("submode: wait for address cmd bit vec sz: {}", cmd_bit_vec.size());
         
          if (cmd_bit_vec.size() != ADDRESS_SIZE) return;

          selected_block = 0;
          u8 idx         = ADDRESS_SIZE;
          for (const auto& bit : cmd_bit_vec) {
            idx--;
            selected_block |= bit << idx;
          }
          
          selected_block &= 0x3ff;

          assert(idx == 0);
          cmd_bit_vec.clear();

          eeprom_logger->info("write req: set selected block: {} - {:#08x}", selected_block, selected_block);
          submode = EEPROM_WRITE_SUB_MODE::WAIT_FOR_DATA;

          assert(cmd_bit_vec.size() == 0);
          break;
        }
        case EEPROM_WRITE_SUB_MODE::WAIT_FOR_DATA: {
          write_val |= (u64)(v & 1) << (64 - cmd_bit_vec.size());
          eeprom_logger->info("new write val: {} - {}", write_val, cmd_bit_vec.size());
          if (cmd_bit_vec.size() != 64) return;

          eeprom_logger->info("hit 64 bits written");
          eeprom_logger->info("copying data to block sector: {}", selected_block);
          data.at(selected_block) = write_val;
          eeprom_logger->info("copied, setting mode to wait for end bit -- clearing cmd bit vec");
          cmd_bit_vec.clear();
          write_val = 0;
          submode   = EEPROM_WRITE_SUB_MODE::WAIT_FOR_END_BIT;
          break;
        }
        case EEPROM_WRITE_SUB_MODE::WAIT_FOR_END_BIT: {
          // assert(v == 0);
          eeprom_logger->info("write end bit received, setting submode to wait and mode to none");
          mode    = EEPROM_MODE::NONE;
          submode = EEPROM_WRITE_SUB_MODE::WAIT_FOR_ADDRESS;
          cmd_bit_vec.clear();
          return;
        }
        default: assert(0); break;
      }
      break;
    }
    case EEPROM_MODE::NONE: {
      cmd_bit_vec.push_back(v & 1);
      // eeprom_logger->info("pushed back: {} none: {}", v & 1, cmd_bit_vec.size());
      if (cmd_bit_vec.size() == 2) {
        EEPROM_MODE cmd;

        cmd = static_cast<EEPROM_MODE>(cmd_bit_vec.at(0) << 1 | cmd_bit_vec.at(1));
        eeprom_logger->info("cmd: {}", (u8)cmd);
        eeprom_logger->info("setting mode to: {}", eeprom_str.at(cmd));
        read_submode = EEPROM_READ_SUB_MODE::WAIT_FOR_ADDRESS;
        submode      = EEPROM_WRITE_SUB_MODE::WAIT_FOR_ADDRESS;

        mode = cmd;
        cmd_bit_vec.clear();
        break;
      }
      break;
    }
    default: assert(0);
  }
}

u8 EEPROMController::handle_read() {
  bits_to_read--;
  if (bits_to_read > 63) {
    eeprom_logger->info("skipped");
    return 0;
  }



  eeprom_logger->info("getting data from selected block: {}", selected_block);
  eeprom_logger->info("bits to read: {}", bits_to_read);
  u64 data_val = data.at(selected_block);
  u8 retval    = (data_val >> bits_to_read) & 1;
  eeprom_logger->info("data val: {}", data_val);
  eeprom_logger->info("retval: {}", retval);
  eeprom_logger->info("iteration: {}", bits_to_read);

  return retval;
}
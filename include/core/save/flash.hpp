#pragma once

struct SRAM;

struct FlashController {
  SRAM *sram = nullptr;
  void handle_write();
  void handle_read();
};
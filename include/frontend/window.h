#pragma once

#include <SDL2/SDL.h>

#include "bass.hpp"
#include "common.hpp"
#include "imgui.h"
struct State {
  SDL_Texture* ppu_texture = nullptr;
  // std::array<u8, (240 * 160) * 2>* frame_buf_ptr = nullptr;
  // std::array<u32, (240 * 160)>* frame_buf_ptr = nullptr;
  u32* frame_buf_ptr = nullptr;
  
  
  

  const u8* keyboardState = SDL_GetKeyboardState(nullptr);
  bool running            = true;

  bool debugger_open = true;
  bool cpu_info_open = true;
  ImGuiIO* io        = nullptr;
};
struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  Bass* bass = nullptr;

  char const* patterns[1] = {"*.gba"};

  void handle_events();
  void render_frame();

  void show_menubar();

  void shutdown();
  void show_debugger();
  void show_cpu_info();

  explicit Frontend(Bass*);
};

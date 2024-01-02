#pragma once

#include <SDL2/SDL.h>

#include "imgui.h"
struct State {
  SDL_Texture* viewport    = nullptr;

  bool active             = true;
  
  ImGuiIO* io = nullptr;
};
struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  char const* patterns[1] = {"*.*"};

  void handle_events();
  void render_frame();

  void show_menubar();

  void shutdown();

  explicit Frontend();
};

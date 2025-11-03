#pragma once

#include <unordered_map>

#include "SDL3/SDL.h"
#include "SDL3/SDL_video.h"
#include "agb.hpp"
#include "imgui.h"
// TODO: add breakpoints in debugger

struct State {
  SDL_Texture* ppu_texture = nullptr;
  SDL_Texture* backdrop    = nullptr;

  SDL_Texture* tile_set_texture = nullptr;
  SDL_Texture* tile_map_texture = nullptr;
  SDL_Texture* obj_texture      = nullptr;

  std::array<SDL_Texture*, 4> background_textures{};
  std::array<SDL_Texture*, 4> background_affine_textures{};

  const bool* keyboard_state = SDL_GetKeyboardState(nullptr);
  std::atomic<bool> running  = true;

  bool memory_viewer_open   = true;
  bool cpu_info_open        = true;
  bool ppu_info_open        = true;
  bool controls_window_open = true;
  bool tile_window_open     = true;
  bool window_info_open     = true;
  bool timer_window_open    = true;
  bool apu_window_open      = true;
  bool show_dma_menu        = true;

  bool backgrounds_window_open = true;
  bool halted                  = false;
  int step_amount              = 0;
  bool blend_info_open         = true;
  ImGuiIO* io                  = nullptr;
};

enum KEY { A, B, SELECT, START, RIGHT, LEFT, UP, DOWN, L, R, NONE = 0xFF };

struct Keymap_State {
  SDL_Scancode current_key;
  bool being_remapped;
};
struct Settings {
  struct Keybinds {
    std::unordered_map<KEY, Keymap_State> control_map = {
        {     KEY::A,         {SDL_SCANCODE_Z, false}},
        {     KEY::B,         {SDL_SCANCODE_X, false}},
        {     KEY::L,         {SDL_SCANCODE_A, false}},
        {     KEY::R,         {SDL_SCANCODE_S, false}},
        {KEY::SELECT, {SDL_SCANCODE_BACKSPACE, false}},
        { KEY::START,    {SDL_SCANCODE_RETURN, false}},
        { KEY::RIGHT,     {SDL_SCANCODE_RIGHT, false}},
        {  KEY::LEFT,      {SDL_SCANCODE_LEFT, false}},
        {    KEY::UP,        {SDL_SCANCODE_UP, false}},
        {  KEY::DOWN,      {SDL_SCANCODE_DOWN, false}},
    };

  } keybinds;
};

struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  AGB* agb = nullptr;
  std::thread sdlThread;
  SDL_AudioStream* stream = nullptr;
  SDL_FRect rect{0, 0, 240, 160};

  constexpr static const SDL_DialogFileFilter filters[] = {
      {"ROMs", "gba"},
  };

  void init_sdl();

  void handle_events();

  void render_frame();

  void show_menu_bar();
  void show_backgrounds();
  void show_viewport();
  void show_audio_info();

  void show_dma_info();
  void show_window_info();
  void show_blend_info();
  void init_audio_device();
  void shutdown();
  void show_memory_viewer();
  void show_cpu_info();
  void show_ppu_info();
  void show_controls_menu(bool*);
  void show_irq_status();
  void show_tiles();
  void show_obj();
  void show_timer_window();

  explicit Frontend(AGB*);

  Settings settings;
};

const inline std::unordered_map<KEY, std::string> buttons = {
    {     A,      "A"},
    {     B,      "B"},
    {     L,      "L"},
    {     R,      "R"},
    { START,  "START"},
    {SELECT, "SELECT"},
    { RIGHT,  "RIGHT"},
    {  LEFT,   "LEFT"},
    {    UP,     "UP"},
    {  DOWN,   "DOWN"},
};

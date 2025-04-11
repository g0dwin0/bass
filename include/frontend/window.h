#pragma once

#include <SDL2/SDL.h>

#include <unordered_map>

#include "bass.hpp"
#include "imgui.h"

// TODO: add breakpoints in debugger

struct State {
  SDL_Texture* ppu_texture      = nullptr;
  SDL_Texture* backdrop         = nullptr;
  SDL_Texture* tile_set_texture = nullptr;
  SDL_Texture* tile_map_texture = nullptr;

  std::array<SDL_Texture*, 4> background_textures{};

  u32* frame_buf_ptr = nullptr;

  const u8* keyboardState   = SDL_GetKeyboardState(nullptr);
  std::atomic<bool> running = true;

  bool debugger_open        = true;
  bool cpu_info_open        = true;
  bool ppu_info_open        = true;
  bool controls_window_open = true;
  bool tile_window_open     = true;

  bool backgrounds_window_open = true;
  bool halted                  = false;
  int step_amount              = 0;

  ImGuiIO* io = nullptr;
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
  Bass* bass = nullptr;
  std::thread sdlThread;

  Uint32 frameStartTime = 0;
  Uint32 frameCount     = 0;
  Uint32 fpsStartTime   = SDL_GetTicks();  // Start time for FPS calculation
  float fps             = 0.0f;

  SDL_Rect rect{0, 0, 240, 160};

  const char* patterns[1] = {"*.gba"};

  void init_sdl();

  void handle_events();

  void render_frame();

  void show_menubar();
  void show_backgrounds();

  void shutdown();
  void show_debugger();
  void show_cpu_info();
  void show_ppu_info();
  void show_controls_menu(bool*);
  void show_irq_status();
  void show_tiles();

  explicit Frontend(Bass*);

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
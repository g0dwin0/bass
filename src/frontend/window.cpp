#include "frontend/window.h"

#include <SDL2/SDL.h>

#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "spdlog/spdlog.h"
#include "tinyfiledialogs.h"

void Frontend::handle_events() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
      case SDL_QUIT: {
        SPDLOG_INFO("SDL QUIT CALLED");

        state.running = false;
        break;
      }

      case SDL_KEYDOWN: {
        // const u8* keyboardState = SDL_GetKeyboardState(nullptr);

        // fmt::println("key pressed: {}",
        //              SDL_GetKeyName(SDL_GetKeyFromScancode(event.key.keysym.scancode)));

        break;
      }

      case SDL_KEYUP: {
        // const u8* keyboardState = SDL_GetKeyboardState(nullptr);
        // fmt::println("key released: {}",
        //              SDL_GetKeyName(SDL_GetKeyFromScancode(event.key.keysym.scancode)));

        break;
      }
    }
  }
}

void Frontend::show_debugger() {
  ImGui::Begin("DEBUGGER", &state.debugger_open, 0);
  
  ImGui::End();
}


void Frontend::show_cpu_info() {
  ImGui::Begin("CPU INFO", &state.cpu_info_open, 0);
  if (ImGui::Button("STEP")) {  bass->cpu.step(); }
  ImGui::End();
}

void Frontend::show_menubar() {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::Separator();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load ROM")) {
        auto path = tinyfd_openFileDialog("Load ROM", "./", 2, patterns,
                                          "Gameboy Advance ROMs", 0);
        if (path != nullptr) {
          // TODO: add cart loading logic
        }
      }
      if (ImGui::MenuItem("Reset")) {
          
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("State")) {
      if (ImGui::MenuItem("Load state..")) {}
      if (ImGui::MenuItem("Reset")) {}
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}
void Frontend::shutdown() {
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_Quit();
}

void Frontend::render_frame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  show_menubar();
  bool t = true;
  ImGui::ShowDemoWindow(&t);
  show_cpu_info();
  show_debugger();
   // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x,
                     state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);
  SDL_Rect rect{0, 0, 256, 256};
  SDL_RenderCopy(renderer, state.ppu_texture, &rect, NULL);

  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}

Frontend::Frontend(Bass* c) {
  bass = c; // maybe a smart pointer could be better?

  SPDLOG_DEBUG("constructed frontend with instance pointer");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) !=
      0) {
    fmt::println("ERROR: failed initialize SDL");
  }
  
  auto window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);

  this->window =
      SDL_CreateWindow("bass", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
  this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  
  // io.ConfigFlags |=
  //     ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  this->state.io = &io;

  ImGui::StyleColorsDark();

  this->state.ppu_texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_TARGET, 256, 256);

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}
#include "frontend/window.h"

#include <SDL2/SDL.h>

#include "common.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "io.hpp"
#include "tinyfiledialogs.h"

void Frontend::handle_events() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
      case SDL_QUIT: {
        state.active = false;
        break;
      }

      case SDL_KEYDOWN: {
        // const u8* keyboardState = SDL_GetKeyboardState(nullptr);

        fmt::println("key pressed: {}",
                     SDL_GetKeyName(SDL_GetKeyFromScancode(event.key.keysym.scancode)));

        break;
      }

      case SDL_KEYUP: {
        // const u8* keyboardState = SDL_GetKeyboardState(nullptr);
        fmt::println("key released: {}",
                     SDL_GetKeyName(SDL_GetKeyFromScancode(event.key.keysym.scancode)));

        break;
      }
    }
  }
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
          // TODO: add cart reset logic
        // if (!gb->bus.cart.info.path.empty()) {
        // }
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
  // show_menubar();

  // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x,
                     state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);

  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}

Frontend::Frontend() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fmt::println("ERROR: failed initialize SDL");
    exit(-1);
  }

  auto window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);

  this->window =
      SDL_CreateWindow("template", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 600, 600, window_flags);
  this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  this->state.io = &io;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}
#include "frontend/window.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>

#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_memory_edit.h"
#include "spdlog/spdlog.h"
#include "tinyfiledialogs.h"
// #include "utils.h"

// static MemoryEditor iwram_view;
// static MemoryEditor vram_view;
// static MemoryEditor palette_view;
// static MemoryEditor pak_view;
// static MemoryEditor frame_buffer_view;
static MemoryEditor editor_instance;

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
  const char* regions[] = { // TODO: add mirrors & other pak wait states
      "Region: [0x00000000 - 0x00003FFF] BIOS",
      "Region: [0x02000000 - 0x0203FFFF] EWRAM",
      "Region: [0x03000000 - 0x03007FFF] IWRAM",
      "Region: [0x04000000 - 0x040003FE] IO",
      "Region: [0x05000000 - 0x050003FF] BG/OBJ Palette RAM",
      "Region: [0x06000000 - 0x06017FFF] VRAM",
      "Region: [0x07000000 - 0x070003FF] OAM",
      "Region: [0x08000000 - 0x09FFFFFF] Game Pak ROM (WS0)",
      "Region: [0x0E000000 - 0x0E00FFFF] Game Pak SRAM",
  };
  const std::vector<u8>* memory_partitions[] = {
    &bass->bus.BIOS,
    &bass->bus.EWRAM,
    &bass->bus.IWRAM,
    &bass->bus.IO,
    &bass->bus.PALETTE_RAM,
    &bass->bus.VRAM,
    &bass->bus.OAM,
    &bass->bus.pak->data,
    &bass->bus.SRAM,
    
  };

  static int SelectedItem = 0;


  if(ImGui::Combo("Regions", &SelectedItem, regions, IM_ARRAYSIZE(regions))) {
    SPDLOG_DEBUG("switched to: {}", regions[SelectedItem]);
  }
  // editor_instance.HighlightFn
  editor_instance.OptShowAscii = false;
  editor_instance.DrawContents((void*)memory_partitions[SelectedItem]->data(), memory_partitions[SelectedItem]->size());
  ImGui::End();
}

void Frontend::show_cpu_info() {
  ImGui::Begin("CPU INFO", &state.cpu_info_open, 0);
  ImGui::Columns(4, "Registers");
  for (u8 i = 0; i < 16; i++) {
    if (i % 4 == 0 && i != 0) ImGui::NextColumn();
    ImGui::Text("%s",
                fmt::format("r{:d}: {:#010x}", i, bass->cpu.regs.r[i]).c_str());
  }
  ImGui::Columns(1);
  ImGui::Separator();
  bool zero     = bass->cpu.regs.CPSR.ZERO_FLAG;
  bool negative = bass->cpu.regs.CPSR.SIGN_FLAG;
  bool carry    = bass->cpu.regs.CPSR.CARRY_FLAG;
  bool overflow = bass->cpu.regs.CPSR.OVERFLOW_FLAG;

  ImGui::BeginDisabled(true);
  ImGui::Spacing();
  ImGui::SameLine();
  ImGui::Checkbox("Zero", &zero);
  ImGui::SameLine();
  ImGui::Checkbox("Negative", &negative);
  ImGui::SameLine();
  ImGui::Checkbox("Carry", &carry);
  ImGui::SameLine();
  ImGui::Checkbox("Overflow", &overflow);
  ImGui::EndDisabled();

  ImGui::BeginDisabled(true);
  ImGui::Text("CPSR: %#010x", bass->cpu.regs.CPSR.value);
  ImGui::Text("SPSR: %#010x",
              bass->cpu.regs.get_spsr(bass->cpu.regs.CPSR.MODE_BITS));

  ImGui::Text("CPU MODE: %s", bass->cpu.regs.CPSR.STATE_BIT ? "THUMB" : "ARM");
  ImGui::Text(
      "OPERATING MODE: %s",
      fmt::format("{}", mode_map.at(bass->cpu.regs.CPSR.MODE_BITS)).c_str());

  ImGui::EndDisabled();

  ImGui::Separator();

  ImGui::Text("FIQ DISABLED: 0x%02x\n", bass->cpu.regs.CPSR.FIQ_DISABLE);
  ImGui::Text("IRQ DISABLED: 0x%02x\n", bass->cpu.regs.CPSR.IRQ_DISABLE);

  if (ImGui::Button("STEP")) { bass->cpu.step(); }
  ImGui::End();
}
void Frontend::show_ppu_info() {
  ImGui::Begin("PPU INFO", &state.cpu_info_open, 0);

  ImGui::Text("BG MODE: %#010x", bass->ppu.DISPCNT.BG_MODE);
  if (ImGui::Button("Set VBLANK")) { bass->ppu.DISPSTAT.VBLANK_FLAG = 1; }
  if (ImGui::Button("Reset VBLANK")) { bass->ppu.DISPSTAT.VBLANK_FLAG = 0; }
  if (ImGui::Button("Draw")) { bass->ppu.draw(); }
  ImGui::Separator();
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
      if (ImGui::MenuItem("Reset")) {}
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
  // bool t = true;
  // ImGui::ShowDemoWindow(&t);
  show_cpu_info();
  show_ppu_info();
  show_debugger();

  // iwram_view.DrawWindow("IWRAM View", bass->bus.IWRAM.data(),
  //                       bass->bus.IWRAM.size());

  // vram_view.DrawWindow("VRAM View", bass->bus.VRAM.data(),
  //                      bass->bus.VRAM.size());

  // palette_view.DrawWindow("Palette RAM View", bass->bus.PALETTE_RAM.data(),
  //                         bass->bus.PALETTE_RAM.size());
  // pak_view.DrawWindow("Pak View", bass->bus.pak->data.data(),
  //                     bass->bus.pak->data.size());
  // frame_buffer_view.DrawWindow("Frame Buffer View", bass->bus.ppu->frame_buffer,
  //                              38400);

  // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x,
                     state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);
  SDL_Rect rect{0, 0, 240, 160};
  SDL_UpdateTexture(state.ppu_texture, nullptr, state.frame_buf_ptr, 240 * 4);
  SDL_RenderCopy(renderer, state.ppu_texture, &rect, NULL);
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}

Frontend::Frontend(Bass* c) {
  bass = c;  // maybe a smart pointer could be better?

  state.frame_buf_ptr = bass->ppu.frame_buffer;

  SPDLOG_DEBUG("constructed frontend with instance pointer");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) !=
      0) {
    fmt::println("ERROR: failed initialize SDL");
  }

  auto window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);

  this->window =
      SDL_CreateWindow("bass", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       240 * 5, 160 * 5, window_flags);
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
      renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 240, 160);

  // this->state.ppu_texture = SDL_CreateTexture(
  //     renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_TARGET, 240, 160);

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}
#include "frontend/window.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <spdlog/common.h>

#include <thread>

#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_memory_edit.h"
#include "tinyfiledialogs.h"

static MemoryEditor editor_instance;

void Frontend::handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT: {
        SPDLOG_INFO("SDL QUIT CALLED");

        state.running = false;
        break;
      }

      case SDL_KEYDOWN: {
        if (state.keyboardState[settings.keybinds.control_map.at(RIGHT).current_key]) { bass->bus.keypad_input.KEYINPUT.RIGHT = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(LEFT).current_key]) { bass->bus.keypad_input.KEYINPUT.LEFT = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(UP).current_key]) { bass->bus.keypad_input.KEYINPUT.UP = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(DOWN).current_key]) { bass->bus.keypad_input.KEYINPUT.DOWN = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(A).current_key]) { bass->bus.keypad_input.KEYINPUT.A = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(B).current_key]) { bass->bus.keypad_input.KEYINPUT.B = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(L).current_key]) { bass->bus.keypad_input.KEYINPUT.L = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(R).current_key]) { bass->bus.keypad_input.KEYINPUT.R = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(SELECT).current_key]) { bass->bus.keypad_input.KEYINPUT.SELECT = 0; }
        if (state.keyboardState[settings.keybinds.control_map.at(START).current_key]) { bass->bus.keypad_input.KEYINPUT.START = 0; }

        break;
      }

      case SDL_KEYUP: {
        if (!state.keyboardState[settings.keybinds.control_map.at(RIGHT).current_key]) { bass->bus.keypad_input.KEYINPUT.RIGHT = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(LEFT).current_key]) { bass->bus.keypad_input.KEYINPUT.LEFT = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(UP).current_key]) { bass->bus.keypad_input.KEYINPUT.UP = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(DOWN).current_key]) { bass->bus.keypad_input.KEYINPUT.DOWN = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(A).current_key]) { bass->bus.keypad_input.KEYINPUT.A = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(B).current_key]) { bass->bus.keypad_input.KEYINPUT.B = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(L).current_key]) { bass->bus.keypad_input.KEYINPUT.L = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(R).current_key]) { bass->bus.keypad_input.KEYINPUT.R = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(SELECT).current_key]) { bass->bus.keypad_input.KEYINPUT.SELECT = 1; }
        if (!state.keyboardState[settings.keybinds.control_map.at(START).current_key]) { bass->bus.keypad_input.KEYINPUT.START = 1; }

        break;
      }
    }
    ImGui_ImplSDL2_ProcessEvent(&event);
  }
}

void Frontend::show_debugger() {
  ImGui::Begin("DEBUGGER", &state.debugger_open, 0);
  const char* regions[] = {
      "Region: [0x00000000 - 0x00003FFF] BIOS",
      "Region: [0x02000000 - 0x0203FFFF] EWRAM",
      "Region: [0x03000000 - 0x03007FFF] IWRAM",
      // "Region: [0x04000000 - 0x040003FE] IO",
      "Region: [0x05000000 - 0x050003FF] BG/OBJ Palette RAM",
      "Region: [0x06000000 - 0x06017FFF] VRAM",
      "Region: [0x07000000 - 0x070003FF] OAM",
      "Region: [0x08000000 - 0x09FFFFFF] Game Pak ROM",
      "Region: [0x0E000000 - 0x0E00FFFF] Game Pak SRAM",
  };
  // ImGui::Text("KEYCNT: {:%#010X}", bass->bus.keypad.KEYINPUT.value);
  // ImGui::Text("KEYCNT: {:%10s}", +bass->bus.keypad.KEYINPUT.trv_ptr);
  // ImGui::Text("keys pressed: %s", bass->bus.keypad.KEYINPUT.get_keys_pressed().c_str());

  // ImGui::Text("Cycles Consumed: UNIMPL");
  const std::vector<u8>* memory_partitions[] = {
      &bass->bus.BIOS,
      &bass->bus.EWRAM,
      &bass->bus.IWRAM,
      // &bass->bus.IO,
      &bass->bus.PALETTE_RAM,
      &bass->bus.VRAM,
      &bass->bus.OAM,
      &bass->bus.pak->data,
      &bass->bus.SRAM,

  };

  static int SelectedItem = 0;

  if (ImGui::Combo("Regions", &SelectedItem, regions, IM_ARRAYSIZE(regions))) { SPDLOG_DEBUG("switched to: {}", regions[SelectedItem]); }
  editor_instance.OptShowAscii = false;
  // editor_instance.ReadOnly = true;

  editor_instance.DrawContents((void*)memory_partitions[SelectedItem]->data(), memory_partitions[SelectedItem]->size());

  ImGui::End();
}
void Frontend::show_irq_status() {
  ImGui::Begin("IRQ STATUS");
  ImGui::Text("%s", fmt::format("IME: {}", bass->bus.interrupt_control.IME.v ? 1 : 0).c_str());
  ImGui::Separator();
  ImGui::Text("IE - IF");
  ImGui::Text("%s", fmt::format("LCD VBLANK:        {} - {}", bass->bus.interrupt_control.IE.LCD_VBLANK ? 1 : 0, bass->bus.interrupt_control.IF.LCD_VBLANK ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("LCD HBLANK:        {} - {}", bass->bus.interrupt_control.IE.LCD_HBLANK ? 1 : 0, bass->bus.interrupt_control.IF.LCD_HBLANK ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("LCD VCOUNT_MATCH:  {} - {}", bass->bus.interrupt_control.IE.LCD_V_COUNTER_MATCH ? 1 : 0, bass->bus.interrupt_control.IF.LCD_V_COUNTER_MATCH ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER0 OVERFLOW:   {} - {}", bass->bus.interrupt_control.IE.TIMER0_OVERFLOW ? 1 : 0, bass->bus.interrupt_control.IF.TIMER0_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER1 OVERFLOW:   {} - {}", bass->bus.interrupt_control.IE.TIMER1_OVERFLOW ? 1 : 0, bass->bus.interrupt_control.IF.TIMER1_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER2 OVERFLOW:   {} - {}", bass->bus.interrupt_control.IE.TIMER2_OVERFLOW ? 1 : 0, bass->bus.interrupt_control.IF.TIMER2_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER3 OVERFLOW:   {} - {}", bass->bus.interrupt_control.IE.TIMER3_OVERFLOW ? 1 : 0, bass->bus.interrupt_control.IF.TIMER3_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("SERIAL COMM:       {} - {}", bass->bus.interrupt_control.IE.SERIAL_COM ? 1 : 0, bass->bus.interrupt_control.IF.SERIAL_COM ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA0:              {} - {}", bass->bus.interrupt_control.IE.DMA0 ? 1 : 0, bass->bus.interrupt_control.IF.DMA0 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA1:              {} - {}", bass->bus.interrupt_control.IE.DMA1 ? 1 : 0, bass->bus.interrupt_control.IF.DMA1 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA2:              {} - {}", bass->bus.interrupt_control.IE.DMA2 ? 1 : 0, bass->bus.interrupt_control.IF.DMA2 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA3:              {} - {}", bass->bus.interrupt_control.IE.DMA3 ? 1 : 0, bass->bus.interrupt_control.IF.DMA3 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("KEYPAD:            {} - {}", bass->bus.interrupt_control.IE.KEYPAD ? 1 : 0, bass->bus.interrupt_control.IF.KEYPAD ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("GAMEPAK_EXT:       {} - {}", bass->bus.interrupt_control.IE.GAMEPAK ? 1 : 0, bass->bus.interrupt_control.IF.GAMEPAK ? 1 : 0).c_str());
  ImGui::End();
}
void Frontend::show_tiles() {
  ImGui::Begin("Tile Window");
  // SDL_Rect sz{240,160};
  ImGui::Text("Tile Set");
  ImGui::Image(state.tile_set_texture, ImVec2(256, 256));
  ImGui::Separator();
  ImGui::Text("Tilemap");
  ImGui::Image(state.tile_map_texture, ImVec2(256 * 2, 256 * 2));
  ImGui::End();
}

void Frontend::show_backgrounds() {
  ImGui::Begin("Backgrounds", &state.backgrounds_window_open, 0);
  const char* backgrounds[] = {
      "BG0",
      "BG1",
      "BG2",
      "BG3",
  };

  static int SelectedItem = 0;

  if (ImGui::Combo("Regions", &SelectedItem, backgrounds, IM_ARRAYSIZE(backgrounds))) { fmt::println("switched to: {}", backgrounds[SelectedItem]); }
  // ImGui::Image(bass->ppu.tile_map_texture_buffer_arr[SelectedItem], {512, 512});
  ImGui::End();
}
void Frontend::show_cpu_info() {
  ImGui::Begin("CPU INFO", &state.cpu_info_open, 0);
  ImGui::Columns(4, "Registers");
  for (u8 i = 0; i < 16; i++) {
    if (i % 4 == 0 && i != 0) ImGui::NextColumn();
    ImGui::Text("%s", fmt::format("r{:d}: {:#010x}", i, bass->cpu.regs.r[i]).c_str());
  }
  ImGui::Columns(1);
  ImGui::Separator();
  ImGui::Text("%s", fmt::format("SPSR_fiq: {:#010x}", bass->cpu.regs.SPSR_fiq).c_str());
  ImGui::Text("%s", fmt::format("SPSR_irq: {:#010x}", bass->cpu.regs.SPSR_irq).c_str());
  ImGui::Text("%s", fmt::format("SPSR_svc: {:#010x}", bass->cpu.regs.SPSR_svc).c_str());
  ImGui::Text("%s", fmt::format("SPSR_abt: {:#010x}", bass->cpu.regs.SPSR_abt).c_str());
  ImGui::Text("%s", fmt::format("SPSR_und: {:#010x}", bass->cpu.regs.SPSR_und).c_str());
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
  ImGui::Text("SPSR: %#010x", bass->cpu.regs.get_spsr(bass->cpu.regs.CPSR.MODE_BITS));

  ImGui::Text("CPU MODE: %s", bass->cpu.regs.CPSR.STATE_BIT ? "THUMB" : "ARM");
  ImGui::Text("OPERATING MODE: %s", fmt::format("{}", mode_map.at(bass->cpu.regs.CPSR.MODE_BITS)).c_str());
  ImGui::Text("CYCLES ELAPSED: %s", fmt::format("{}", bass->bus.cycles_elapsed).c_str());

  ImGui::EndDisabled();

  ImGui::Separator();

  ImGui::Text("FIQ DISABLED: 0x%02x\n", bass->cpu.regs.CPSR.FIQ_DISABLE);
  ImGui::Text("IRQ DISABLED: 0x%02x\n", bass->cpu.regs.CPSR.IRQ_DISABLE);
  ImGui::Text("KEYINPUT: 0x%02x\n", bass->bus.keypad_input.KEYINPUT.v);
  ImGui::InputInt("step amount", &state.step_amount, 0, 0, 0);
  if (ImGui::Button("STEP")) { bass->cpu.step(); }
  if (ImGui::Button("STEP AMOUNT")) {
    for (int i = 0; i < state.step_amount; i++) {
      bass->cpu.step();
    }
  }

  ImGui::Separator();
  if (ImGui::Button("Enable Logging")) { spdlog::set_level(spdlog::level::trace); }
  if (ImGui::Button("Disable Logging")) { spdlog::set_level(spdlog::level::off); }
  if (ImGui::Button("UNHALT")) { state.halted = false; }

  ImGui::End();
}

void Frontend::show_ppu_info() {
  ImGui::Begin("PPU INFO", &state.cpu_info_open, 0);

  ImGui::Text("BG0 PRIORITY: %d", bass->bus.display_fields.BG0CNT.BG_PRIORITY);
  ImGui::Text("BG0 CHAR_BASE_BLOCK: %d", bass->bus.display_fields.BG0CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG0 MOSAIC: %d", bass->bus.display_fields.BG0CNT.MOSAIC);
  ImGui::Text("BG0 COLOR MODE: %s", bass->bus.display_fields.BG0CNT.COLOR_MODE ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG0 SCREEN_BASE_BLOCK: %d", bass->bus.display_fields.BG0CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG0 SCREEN_SIZE: %d (%s)", bass->bus.display_fields.BG0CNT.SCREEN_SIZE, bass->ppu.screen_sizes.at(+bass->bus.display_fields.BG0CNT.SCREEN_SIZE).c_str());

  ImGui::Text("BG1 PRIORITY: %d", bass->bus.display_fields.BG1CNT.BG_PRIORITY);
  ImGui::Text("BG1 CHAR_BASE_BLOCK: %d", bass->bus.display_fields.BG1CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG1 MOSAIC: %d", bass->bus.display_fields.BG1CNT.MOSAIC);
  ImGui::Text("BG1 COLOR MODE: %s", bass->bus.display_fields.BG1CNT.COLOR_MODE ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG1 SCREEN_BASE_BLOCK: %d", bass->bus.display_fields.BG1CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG1 SCREEN_SIZE: %d (%s)", bass->bus.display_fields.BG1CNT.SCREEN_SIZE, bass->ppu.screen_sizes.at(+bass->bus.display_fields.BG1CNT.SCREEN_SIZE).c_str());

  ImGui::Text("BG0HOFS: %d", bass->bus.display_fields.BG0HOFS.v);
  ImGui::Text("BG0VOFS: %d", bass->bus.display_fields.BG0VOFS.v);
  ImGui::Separator();

  ImGui::Text("BG MODE: %#010x", bass->bus.display_fields.DISPCNT.BG_MODE);
  if (ImGui::Button("Set VBLANK")) { bass->bus.display_fields.DISPSTAT.VBLANK_FLAG = 1; }
  if (ImGui::Button("Reset VBLANK")) { bass->bus.display_fields.DISPSTAT.VBLANK_FLAG = 0; }
  if (ImGui::Button("Draw")) { bass->ppu.draw(); }
  if (ImGui::Button("Draw Tileset")) {
    bass->ppu.draw(true);
    // SDL_UpdateTexture(state.tile_set_texture, nullptr, bass->ppu.tile_set_texture, 240 * 4);
    // SDL_UpdateTexture(state.tile_map_texture, nullptr, bass->ppu.tile_map_texture_buffer, 512 * 4);
  }
  ImGui::Separator();
  ImGui::End();
}

void Frontend::show_controls_menu(bool* p_open) {
  ImGui::Begin("Controls", p_open);
  ImGui::Text("Remap your controls here.");
  ImGui::Separator();

  bool invalid_keybind = false;

  for (SDL_Scancode current_scancode = SDL_SCANCODE_A; current_scancode < SDL_SCANCODE_AUDIOFASTFORWARD; current_scancode = (SDL_Scancode)(current_scancode + 1)) {
    if (!state.keyboardState[current_scancode]) continue;
    ImGui::SameLine();

    // refactor: hard to read; use structured bindings
    for (auto& current_key : settings.keybinds.control_map) {
      // Actual key represents the key on GBA (L, R, START etc...);
      auto& actual_key = current_key.first;

      // This indicates the scancode (the key on your keyboard) of the corresponding actual key (key on the GBA)
      auto& [scancode, being_remapped] = current_key.second;

      // If button is set to be remapped, remap with next key input
      if (being_remapped) {
        being_remapped = false;
        for (auto& [diff_key, keymap_state] : settings.keybinds.control_map) {
          if (diff_key != actual_key && keymap_state.current_key == current_scancode) { invalid_keybind = true; }
        }
        if (!invalid_keybind) {
          scancode       = current_scancode;
          being_remapped = false;  // unset remap flag
        }
      }
    }
  }

  ImGui::Separator();
  ImGui::Spacing();

  ImGui::SameLine();

  for (auto& entry : settings.keybinds.control_map) {
    ImGui::PushID(&entry);
    auto& [key, keymap_state]        = entry;
    auto& [scancode, being_remapped] = keymap_state;

    ImGui::Text("%s: ", buttons.at(key).c_str());
    ImGui::SameLine();
    if (ImGui::Button(being_remapped ? "Waiting for input..." : SDL_GetScancodeName(scancode))) {
      being_remapped = true;  // Being remapped
    }
    ImGui::PopID();
  }
  ImGui::End();
}

void Frontend::show_menubar() {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::Separator();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load ROM")) {
        auto path = tinyfd_openFileDialog("Load ROM", "./", 2, patterns, "Gameboy Advance ROMs", 0);
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
  show_cpu_info();
  show_ppu_info();
  show_debugger();
  // show_irq_status();
  show_tiles();
  show_backgrounds();

  // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x, state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);
  SDL_Rect rect{0, 0, 240, 160};
  SDL_UpdateTexture(state.ppu_texture, nullptr, state.frame_buf_ptr, 240 * 4);
  SDL_RenderCopy(renderer, state.ppu_texture, &rect, NULL);
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}
void Frontend::init_sdl() {
  // bass = c;  // maybe a smart pointer could be better?
  state.frame_buf_ptr = bass->ppu.frame_buffer;

  SPDLOG_DEBUG("constructed frontend with instance pointer");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) { fmt::println("ERROR: failed initialize SDL"); }

  auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  this->window   = SDL_CreateWindow("bass", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 240 * 5, 160 * 5, window_flags);
  this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  fmt::println("initializing SDL");
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  this->state.io = &io;

  ImGui::StyleColorsDark();

  this->state.ppu_texture      = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 240, 160);
  this->state.tile_set_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 240, 160);
  this->state.tile_map_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 512, 512);

  //   for (size_t bg_buffer = 0; bg_buffer < 4; bg_buffer++) {
  //     this->state.tile_maps_textures[bg_buffer] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 512, 512);
  //   }

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  // GFX Loop

  while (state.running) {
    handle_events();
    render_frame();
  }

  stop();  // Joins the thread and then executes cleanup code
};

void Frontend::run() {
  sdlThread     = std::thread(&Frontend::init_sdl, this);
  state.running = true;
}
void Frontend::stop() {
  state.running = false;
  if (sdlThread.joinable()) { sdlThread.join(); }
}

Frontend::Frontend(Bass* c) {
  bass = c;
  run();
}
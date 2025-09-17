#include "frontend/window.hpp"

#include <spdlog/common.h>

#include <atomic>
#include <mutex>

#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_render.h"
#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_memory_edit.h"

static MemoryEditor editor_instance;

void Frontend::handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT: {
        SPDLOG_INFO("SDL QUIT CALLED");
        state.running = false;
        break;
      }

      case SDL_EVENT_KEY_DOWN: {
        if (state.keyboard_state[settings.keybinds.control_map.at(RIGHT).current_key]) {
          agb->bus.keypad_input.KEYINPUT.RIGHT = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(LEFT).current_key]) {
          agb->bus.keypad_input.KEYINPUT.LEFT = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(UP).current_key]) {
          agb->bus.keypad_input.KEYINPUT.UP = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(DOWN).current_key]) {
          agb->bus.keypad_input.KEYINPUT.DOWN = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(A).current_key]) {
          agb->bus.keypad_input.KEYINPUT.A = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(B).current_key]) {
          agb->bus.keypad_input.KEYINPUT.B = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(L).current_key]) {
          agb->bus.keypad_input.KEYINPUT.L = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(R).current_key]) {
          agb->bus.keypad_input.KEYINPUT.R = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(SELECT).current_key]) {
          agb->bus.keypad_input.KEYINPUT.SELECT = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(START).current_key]) {
          agb->bus.keypad_input.KEYINPUT.START = 0;
        }

        break;
      }

      case SDL_EVENT_KEY_UP: {
        if (!state.keyboard_state[settings.keybinds.control_map.at(RIGHT).current_key]) {
          agb->bus.keypad_input.KEYINPUT.RIGHT = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(LEFT).current_key]) {
          agb->bus.keypad_input.KEYINPUT.LEFT = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(UP).current_key]) {
          agb->bus.keypad_input.KEYINPUT.UP = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(DOWN).current_key]) {
          agb->bus.keypad_input.KEYINPUT.DOWN = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(A).current_key]) {
          agb->bus.keypad_input.KEYINPUT.A = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(B).current_key]) {
          agb->bus.keypad_input.KEYINPUT.B = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(L).current_key]) {
          agb->bus.keypad_input.KEYINPUT.L = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(R).current_key]) {
          agb->bus.keypad_input.KEYINPUT.R = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(SELECT).current_key]) {
          agb->bus.keypad_input.KEYINPUT.SELECT = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(START).current_key]) {
          agb->bus.keypad_input.KEYINPUT.START = 1;
        }

        break;
      }
    }
    ImGui_ImplSDL3_ProcessEvent(&event);
  }
}

void Frontend::show_memory_viewer() {
  ImGui::Begin("Memory Viewer", &state.memory_viewer_open, 0);
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

  const std::vector<u8>* memory_partitions[] = {
      &agb->bus.BIOS,
      &agb->bus.EWRAM,
      &agb->bus.IWRAM,
      // &agb->bus.IO,
      &agb->bus.PALETTE_RAM,
      &agb->bus.VRAM,
      &agb->bus.OAM,
      &agb->bus.pak->data,
      &agb->bus.SRAM,

  };

  static int SelectedItem = 0;

  if (ImGui::Combo("Regions", &SelectedItem, regions, IM_ARRAYSIZE(regions))) {
    SPDLOG_DEBUG("switched to: {}", regions[SelectedItem]);
  }
  editor_instance.OptShowAscii = false;
  // editor_instance.ReadOnly = true;

  editor_instance.DrawContents((void*)memory_partitions[SelectedItem]->data(), memory_partitions[SelectedItem]->size());

  ImGui::End();
}
void Frontend::show_irq_status() {
  ImGui::Begin("IRQ STATUS");
  ImGui::Text("%s", fmt::format("IME: {}", agb->bus.interrupt_control.IME.enabled ? 1 : 0).c_str());
  ImGui::Separator();
  ImGui::Text("IE - IF");
  ImGui::Text("%s", fmt::format("LCD VBLANK:        {} - {}", agb->bus.interrupt_control.IE.LCD_VBLANK ? 1 : 0, agb->bus.interrupt_control.IF.LCD_VBLANK ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("LCD HBLANK:        {} - {}", agb->bus.interrupt_control.IE.LCD_HBLANK ? 1 : 0, agb->bus.interrupt_control.IF.LCD_HBLANK ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("LCD VCOUNT_MATCH:  {} - {}", agb->bus.interrupt_control.IE.LCD_V_COUNTER_MATCH ? 1 : 0, agb->bus.interrupt_control.IF.LCD_V_COUNTER_MATCH ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER0 OVERFLOW:   {} - {}", agb->bus.interrupt_control.IE.TIMER0_OVERFLOW ? 1 : 0, agb->bus.interrupt_control.IF.TIMER0_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER1 OVERFLOW:   {} - {}", agb->bus.interrupt_control.IE.TIMER1_OVERFLOW ? 1 : 0, agb->bus.interrupt_control.IF.TIMER1_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER2 OVERFLOW:   {} - {}", agb->bus.interrupt_control.IE.TIMER2_OVERFLOW ? 1 : 0, agb->bus.interrupt_control.IF.TIMER2_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("TIMER3 OVERFLOW:   {} - {}", agb->bus.interrupt_control.IE.TIMER3_OVERFLOW ? 1 : 0, agb->bus.interrupt_control.IF.TIMER3_OVERFLOW ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("SERIAL COMM:       {} - {}", agb->bus.interrupt_control.IE.SERIAL_COM ? 1 : 0, agb->bus.interrupt_control.IF.SERIAL_COM ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA0:              {} - {}", agb->bus.interrupt_control.IE.DMA0 ? 1 : 0, agb->bus.interrupt_control.IF.DMA0 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA1:              {} - {}", agb->bus.interrupt_control.IE.DMA1 ? 1 : 0, agb->bus.interrupt_control.IF.DMA1 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA2:              {} - {}", agb->bus.interrupt_control.IE.DMA2 ? 1 : 0, agb->bus.interrupt_control.IF.DMA2 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("DMA3:              {} - {}", agb->bus.interrupt_control.IE.DMA3 ? 1 : 0, agb->bus.interrupt_control.IF.DMA3 ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("KEYPAD:            {} - {}", agb->bus.interrupt_control.IE.KEYPAD ? 1 : 0, agb->bus.interrupt_control.IF.KEYPAD ? 1 : 0).c_str());
  ImGui::Text("%s", fmt::format("GAMEPAK_EXT:       {} - {}", agb->bus.interrupt_control.IE.GAMEPAK ? 1 : 0, agb->bus.interrupt_control.IF.GAMEPAK ? 1 : 0).c_str());
  ImGui::End();
}
void Frontend::show_tiles() {
  ImGui::Begin("Tile Window");

  ImGui::Text("Tile Set");
  ImGui::Image(state.tile_set_texture, ImVec2(256, 256));
  ImGui::Separator();
  ImGui::Text("Tilemap");
  ImGui::Image(state.tile_map_texture, ImVec2(256 * 2, 256 * 2));
  ImGui::End();
}

void Frontend::show_obj() {
  ImGui::Begin("Sprite/OBJ Window");

  ImGui::Text("Sprite Texture -- relative to BG");
  ImGui::Image(state.obj_texture, ImVec2(256, 256));
  ImGui::Separator();

  ImGui::End();
}

void Frontend::show_backgrounds() {
  ImGui::Begin("Backgrounds", &state.backgrounds_window_open, 0);
  const char* backgrounds[] = {"BG0", "BG1", "BG2", "BG3", "viewport", "backdrop"};

  static int SelectedItem = 0;
  ImGui::Text("Enabled BG(s)");
  for (int bg_id = 0; bg_id < 4; bg_id++) {
    if (!agb->ppu.background_enabled(bg_id)) continue;
    ImGui::Text("BG %d", bg_id);
  }
  if (ImGui::Combo("Regions", &SelectedItem, backgrounds, IM_ARRAYSIZE(backgrounds))) {
    fmt::println("switched to: {}", backgrounds[SelectedItem]);
  }

  if (SelectedItem < 4) {
    ImGui::Image(state.background_textures[SelectedItem], {512, 512});
  }

  switch (SelectedItem) {
    case 0:
    case 1:
    case 2:
    case 3: {
      ImGui::Image(state.background_textures[SelectedItem], {512, 512});
      break;
    }
    case 4: {
      ImGui::Image(state.ppu_texture, {512, 512});
      break;
    }
    case 5: {
      ImGui::Image(state.backdrop, {512, 512});
      break;
    }
  }

  ImGui::End();
}
void Frontend::show_cpu_info() {
  ImGui::Begin("CPU INFO", &state.cpu_info_open, 0);
  ImGui::Columns(4, "Registers");
  for (u8 i = 0; i < 16; i++) {
    if (i % 4 == 0 && i != 0) ImGui::NextColumn();
    ImGui::Text("%s", fmt::format("r{:d}: {:#010x}", i, agb->cpu.regs.get_reg(i)).c_str());
  }
  ImGui::Columns(1);
  ImGui::Separator();
  ImGui::Text("%s", fmt::format("SPSR_fiq: {:#010x}", agb->cpu.regs.SPSR_fiq).c_str());
  ImGui::Text("%s", fmt::format("SPSR_irq: {:#010x}", agb->cpu.regs.SPSR_irq).c_str());
  ImGui::Text("%s", fmt::format("SPSR_svc: {:#010x}", agb->cpu.regs.SPSR_svc).c_str());
  ImGui::Text("%s", fmt::format("SPSR_abt: {:#010x}", agb->cpu.regs.SPSR_abt).c_str());
  ImGui::Text("%s", fmt::format("SPSR_und: {:#010x}", agb->cpu.regs.SPSR_und).c_str());
  ImGui::Separator();
  bool zero     = agb->cpu.regs.CPSR.ZERO_FLAG;
  bool negative = agb->cpu.regs.CPSR.SIGN_FLAG;
  bool carry    = agb->cpu.regs.CPSR.CARRY_FLAG;
  bool overflow = agb->cpu.regs.CPSR.OVERFLOW_FLAG;

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
  ImGui::Text("CPSR: %#010X", agb->cpu.regs.CPSR.value);
  ImGui::Text("SPSR: %#010X", agb->cpu.regs.get_spsr(agb->cpu.regs.CPSR.MODE_BIT));

  ImGui::Text("CPU MODE: %s", agb->cpu.regs.CPSR.STATE_BIT ? "THUMB" : "ARM");
  ImGui::Text("OPERATING MODE: %s", mode_map.at(agb->cpu.regs.CPSR.MODE_BIT).c_str());
  ImGui::Text("CYCLES ELAPSED: %lu", agb->bus.cycles_elapsed);

  ImGui::EndDisabled();

  ImGui::Separator();

  ImGui::Separator();

  ImGui::Text("FIQ DISABLED: 0x%02x\n", agb->cpu.regs.CPSR.fiq_disable);
  ImGui::Text("IRQ DISABLED: 0x%02x\n", agb->cpu.regs.CPSR.irq_disable);
  ImGui::Text("KEYINPUT: 0x%02x\n", agb->bus.keypad_input.KEYINPUT.v);
  ImGui::InputInt("step amount", &state.step_amount, 0, 0, 0);
  if (ImGui::Button("STEP")) {
    agb->cpu.step();
  }
  if (ImGui::Button("STEP AMOUNT")) {
    for (int i = 0; i < state.step_amount; i++) {
      agb->cpu.step();
      agb->set_ppu_interrupts();
    }
  }

  ImGui::Separator();
  if (ImGui::Button("Enable Logging")) {
    spdlog::set_level(spdlog::level::trace);

    agb->cpu.cpu_logger->set_level(spdlog::level::trace);
  }
  if (ImGui::Button("Disable Logging")) {
    spdlog::set_level(spdlog::level::off);
    agb->cpu.cpu_logger->set_level(spdlog::level::off);
  }
  if (ImGui::Button("FORCE NEW TILE LOAD")) {
    // agb->ppu.repopulate_objs();

    agb->ppu.load_tiles(0, agb->bus.display_fields.BG0CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
    agb->ppu.load_tiles(1, agb->bus.display_fields.BG1CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
    agb->ppu.load_tiles(2, agb->bus.display_fields.BG2CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
    agb->ppu.load_tiles(3, agb->bus.display_fields.BG3CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
  }

  ImGui::End();
}

void Frontend::show_window_info() {
  ImGui::Begin("PPU INFO", &state.window_info_open, 0);
  ImGui::Text("WINDOW 0 ENABLED: %d", agb->bus.display_fields.DISPCNT.WINDOW_0_DISPLAY_FLAG);
  ImGui::Text("WINDOW 1 ENABLED: %d", agb->bus.display_fields.DISPCNT.WINDOW_1_DISPLAY_FLAG);
  ImGui::Text("OBJ WINDOW ENABLED: %d", agb->bus.display_fields.DISPCNT.OBJ_WINDOW_DISPLAY_FLAG);

  ImGui::Text("WINDOW 0: X2: %d", agb->bus.display_fields.WIN0H.X2);
  ImGui::Text("WINDOW 0: Y2: %d", agb->bus.display_fields.WIN0V.Y2);

  ImGui::Text("WINDOW 1: X2: %d", agb->bus.display_fields.WIN1H.X2);
  ImGui::Text("WINDOW 1: Y2: %d", agb->bus.display_fields.WIN1V.Y2);

  
  ImGui::SeparatorText("WININ");
  ImGui::Text("W0 BG0 ENABLED: %d", agb->bus.display_fields.WININ.WIN0_BG_ENABLE_BITS & 1);
  ImGui::Text("W0 BG1 ENABLED: %d", agb->bus.display_fields.WININ.WIN0_BG_ENABLE_BITS >> 1 & 1);
  ImGui::Text("W0 BG2 ENABLED: %d", agb->bus.display_fields.WININ.WIN0_BG_ENABLE_BITS >> 2 & 1);
  ImGui::Text("W0 BG3 ENABLED: %d", agb->bus.display_fields.WININ.WIN0_BG_ENABLE_BITS >> 3 & 1);

  ImGui::SeparatorText("WINOUT");

  

  
  

  ImGui::End();
}
void Frontend::show_ppu_info() {
  ImGui::Begin("PPU INFO", &state.cpu_info_open, 0);

  // ImGui::Text("FPS: %.2f", (1 / agb->stopwatch.duration.count()) * 1000.0f);

  ImGui::Text("BG0 PRIORITY: %d", agb->bus.display_fields.BG0CNT.BG_PRIORITY);
  ImGui::Text("BG0 CHAR_BASE_BLOCK: %d", agb->bus.display_fields.BG0CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG0 MOSAIC: %d", agb->bus.display_fields.BG0CNT.MOSAIC);
  ImGui::Text("BG0 COLOR MODE: %s", agb->bus.display_fields.BG0CNT.COLOR_MODE ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG0 SCREEN_BASE_BLOCK: %d", agb->bus.display_fields.BG0CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG0 SCREEN_SIZE: %d (%s)", agb->bus.display_fields.BG0CNT.SCREEN_SIZE, agb->ppu.screen_sizes.at(+agb->bus.display_fields.BG0CNT.SCREEN_SIZE).data());

  ImGui::Text("BG1 PRIORITY: %d", agb->bus.display_fields.BG1CNT.BG_PRIORITY);
  ImGui::Text("BG1 CHAR_BASE_BLOCK: %d", agb->bus.display_fields.BG1CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG1 MOSAIC: %d", agb->bus.display_fields.BG1CNT.MOSAIC);
  ImGui::Text("BG1 COLOR MODE: %s", agb->bus.display_fields.BG1CNT.COLOR_MODE ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG1 SCREEN_BASE_BLOCK: %d", agb->bus.display_fields.BG1CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG1 SCREEN_SIZE: %d (%s)", agb->bus.display_fields.BG1CNT.SCREEN_SIZE, agb->ppu.screen_sizes.at(+agb->bus.display_fields.BG1CNT.SCREEN_SIZE).data());

  ImGui::Text("BG2 PRIORITY: %d", agb->bus.display_fields.BG2CNT.BG_PRIORITY);
  ImGui::Text("BG2 CHAR_BASE_BLOCK: %d", agb->bus.display_fields.BG2CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG2 MOSAIC: %d", agb->bus.display_fields.BG2CNT.MOSAIC);
  ImGui::Text("BG2 COLOR MODE: %s", agb->bus.display_fields.BG2CNT.COLOR_MODE ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG2 SCREEN_BASE_BLOCK: %d", agb->bus.display_fields.BG2CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG2 SCREEN_SIZE: %d (%s)", agb->bus.display_fields.BG2CNT.SCREEN_SIZE, agb->ppu.screen_sizes.at(+agb->bus.display_fields.BG2CNT.SCREEN_SIZE).data());

  ImGui::Text("BG3 PRIORITY: %d", agb->bus.display_fields.BG3CNT.BG_PRIORITY);
  ImGui::Text("BG3 CHAR_BASE_BLOCK: %d", agb->bus.display_fields.BG3CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG3 MOSAIC: %d", agb->bus.display_fields.BG3CNT.MOSAIC);
  ImGui::Text("BG3 COLOR MODE: %s", agb->bus.display_fields.BG3CNT.COLOR_MODE ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG3 SCREEN_BASE_BLOCK: %d", agb->bus.display_fields.BG3CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG3 SCREEN_SIZE: %d (%s)", agb->bus.display_fields.BG3CNT.SCREEN_SIZE, agb->ppu.screen_sizes.at(+agb->bus.display_fields.BG3CNT.SCREEN_SIZE).data());

  ImGui::Text("BG0HOFS: %d", agb->bus.display_fields.BG0HOFS.OFFSET);
  ImGui::Text("BG0VOFS: %d", agb->bus.display_fields.BG0VOFS.OFFSET);

  ImGui::Text("BG1HOFS: %d", agb->bus.display_fields.BG1HOFS.OFFSET);
  ImGui::Text("BG1VOFS: %d", agb->bus.display_fields.BG1VOFS.OFFSET);

  ImGui::Text("LY: %d", agb->bus.display_fields.VCOUNT.LY);
  // ImGui::Text("Scanline Cycle: %d", agb->ppu.scanline_cycle);

  ImGui::Separator();

  ImGui::Text("BG MODE: %#04x", agb->bus.display_fields.DISPCNT.BG_MODE);
  ImGui::Text("OBJ VRAM MAPPING: %s", agb->bus.display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING == 0 ? "2D" : "1D");
  ImGui::Text("OBJ Window: %#010x", agb->bus.display_fields.DISPCNT.OBJ_WINDOW_DISPLAY_FLAG);

  if (ImGui::Button("Set VBLANK")) {
    agb->bus.display_fields.DISPSTAT.VBLANK_FLAG = 1;
  }
  if (ImGui::Button("Reset VBLANK")) {
    agb->bus.display_fields.DISPSTAT.VBLANK_FLAG = 0;
  }
  if (ImGui::Button("Draw")) {
    agb->ppu.step();
  }
  if (ImGui::Button("Draw Tileset")) {
    agb->ppu.step(true);
    // SDL_UpdateTexture(state.tile_set_texture, nullptr, agb->ppu.tile_set_texture, 240 * 4);
    // SDL_UpdateTexture(state.tile_map_texture, nullptr, agb->ppu.tile_map_texture_buffer, 512 * 4);
  }
  ImGui::Separator();
  ImGui::End();
}

void Frontend::show_controls_menu(bool* p_open) {
  ImGui::Begin("Controls", p_open);
  ImGui::Text("Remap your controls here.");
  ImGui::Separator();

  bool invalid_keybind = false;

  for (SDL_Scancode current_scancode = SDL_SCANCODE_A; current_scancode < SDL_SCANCODE_MEDIA_FAST_FORWARD; current_scancode = (SDL_Scancode)(current_scancode + 1)) {
    if (!state.keyboard_state[current_scancode]) continue;
    ImGui::SameLine();

    // REFACTOR: hard to read; use structured bindings
    for (auto& current_key : settings.keybinds.control_map) {
      // Actual key represents the key on GBA (L, R, START etc...);
      auto& actual_key = current_key.first;

      // This indicates the scancode (the key on your keyboard) of the corresponding actual key (key on the GBA)
      auto& [scancode, being_remapped] = current_key.second;

      if (being_remapped) {
        being_remapped = false;
        for (auto& [diff_key, keymap_state] : settings.keybinds.control_map) {
          if (diff_key != actual_key && keymap_state.current_key == current_scancode) {
            invalid_keybind = true;
          }
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
      being_remapped = true;
    }
    ImGui::PopID();
  }
  ImGui::End();
}

void Frontend::show_menu_bar() {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::Separator();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load ROM")) {
        SDL_ShowOpenFileDialog(
            [](void* userdata, const char* const* filelist, int filter) {
              if (filter == -1) {
                spdlog::info("no filters");
              }

              auto agb = static_cast<AGB*>(userdata);
              (void)(agb);
              fmt::println("{}", *filelist);

              // agb->pak.load_data(std::vector<u8> &)
            },
            this, window, filters, 1, "./", false);
      }
      if (ImGui::MenuItem("Reset")) {}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("State")) {
      if (ImGui::MenuItem("Save state...")) {}
      if (ImGui::MenuItem("Load state...")) {}
      if (ImGui::MenuItem("Reset")) {}
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}
void Frontend::shutdown() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_Quit();
}

void Frontend::render_frame() {
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // show_menu_bar();
  if (state.cpu_info_open) {
    show_cpu_info();
  }
  if (state.ppu_info_open) {
    show_ppu_info();
  }
  if (state.window_info_open) {
    show_window_info();
  }

  if (state.memory_viewer_open) {
    show_memory_viewer();
  }
  show_irq_status();
  show_tiles();
  show_backgrounds();
  show_obj();

  ImGui::Render();
  SDL_SetRenderScale(renderer, state.io->DisplayFramebufferScale.x, state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);

  SDL_UpdateTexture(state.background_textures[0], nullptr, agb->ppu.tile_map_texture_buffer_0, 512 * 4);
  SDL_UpdateTexture(state.background_textures[1], nullptr, agb->ppu.tile_map_texture_buffer_1, 512 * 4);
  SDL_UpdateTexture(state.background_textures[2], nullptr, agb->ppu.tile_map_texture_buffer_2, 512 * 4);
  SDL_UpdateTexture(state.background_textures[3], nullptr, agb->ppu.tile_map_texture_buffer_3, 512 * 4);
  SDL_UpdateTexture(state.obj_texture, nullptr, agb->ppu.obj_texture_buffer, 256 * 4);

  // SDL_UpdateTexture(state.backdrop, nullptr, agb->ppu.backdrop, 512 * 4);
  SDL_UpdateTexture(state.ppu_texture, nullptr, agb->ppu.db.disp_buf, 240 * 4);
  SDL_RenderTexture(renderer, state.ppu_texture, &rect, NULL);

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

  SDL_RenderPresent(renderer);
}
void Frontend::init_sdl() {
  SPDLOG_DEBUG("constructed frontend with instance pointer");

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
    fmt::println("ERROR: failed initialize SDL");
  }

  auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE);

  this->window = SDL_CreateWindow("bass", 240 * 5, 160 * 5, window_flags);
  if (this->window == NULL) {
    fmt::println("failed to create window");
    assert(0);
  }
  this->renderer = SDL_CreateRenderer(window, NULL);

  if (this->renderer == NULL) {
    fmt::println("failed to create renderer");
    assert(0);
  }

  fmt::println("initializing SDL");
  printf("window ptr: %p\n", this->window);
  printf("render ptr: %p\n", this->renderer);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  this->state.io = &io;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);
  this->state.ppu_texture      = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 240, 160);
  this->state.tile_set_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 240, 160);
  this->state.tile_map_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 512, 512);
  this->state.backdrop         = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 512, 512);
  this->state.obj_texture      = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 256, 256);

  for (size_t bg_id = 0; bg_id < 4; bg_id++) {
    this->state.background_textures[bg_id] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 512, 512);
  }
};

Frontend::Frontend(AGB* c) : agb(c) { init_sdl(); }

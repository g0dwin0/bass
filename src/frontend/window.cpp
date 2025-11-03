#include "frontend/window.hpp"

#include <spdlog/common.h>

#include <atomic>

#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_render.h"
#include "common/stopwatch.hpp"
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
  const char* regions[] = {"Region: [0x00000000 - 0x00003FFF] BIOS", "Region: [0x02000000 - 0x0203FFFF] EWRAM", "Region: [0x03000000 - 0x03007FFF] IWRAM",
                           // "Region: [0x04000000 - 0x040003FE] IO",
                           "Region: [0x05000000 - 0x050003FF] BG/OBJ Palette RAM", "Region: [0x06000000 - 0x06017FFF] VRAM", "Region: [0x07000000 - 0x070003FF] OAM",
                           "Region: [0x08000000 - 0x09FFFFFF] Game Pak ROM", "Region: [0x0E000000 - 0x0E00FFFF] Game Pak SRAM", "Region: BGMAP1"};

  const std::vector<u8>* memory_partitions[] = {
      &agb->bus.BIOS,
      &agb->bus.EWRAM,
      &agb->bus.IWRAM,
      // &agb->bus.IO,
      &agb->bus.PALETTE_RAM,
      &agb->bus.ppu->VRAM,
      &agb->bus.OAM,
      &agb->pak.data,
      &agb->pak.SRAM,

  };

  static int SelectedItem = 0;

  if (ImGui::Combo("Regions", &SelectedItem, regions, IM_ARRAYSIZE(regions))) {
    SPDLOG_DEBUG("switched to: {}", regions[SelectedItem]);
  }
  editor_instance.OptShowAscii = false;
  // editor_instance.ReadOnly = true;

  if (SelectedItem == 8) {
    // agb->ppu.tile_map_texture_buffer_1,
    editor_instance.DrawContents((void*)agb->ppu.tile_map_texture_buffer_arr[0].data(), sizeof(u32) * 512 * 512);
  } else {
    editor_instance.DrawContents((void*)memory_partitions[SelectedItem]->data(), memory_partitions[SelectedItem]->size());
  }

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

void Frontend::show_timer_window() {
  ImGui::Begin("Show Timer Window", &state.timer_window_open);

  ImGui::SeparatorText("Timer 0");
  // bool enabled_0     = agb->timers[0].ctrl.timer_start_stop;
  // bool irq_enabled_0 = agb->timers[0].ctrl.timer_irq_enable;

  ImGui::Text("Ticking Enabled: %d", agb->timers[0].ctrl.timer_start_stop);
  ImGui::Text("IRQ Enabled: %d", agb->timers[0].ctrl.timer_irq_enable);
  ImGui::Text("Counter: %d", agb->timers[0].counter);
  ImGui::Text("Prescaler: %s", agb->timers[0].get_prescaler_string().c_str());

  // =========================

  ImGui::SeparatorText("Timer 1");

  ImGui::Text("Ticking Enabled: %d", agb->timers[1].ctrl.timer_start_stop);
  ImGui::Text("IRQ Enabled: %d", agb->timers[1].ctrl.timer_irq_enable);
  ImGui::Text("Counter: %d", agb->timers[1].counter);
  ImGui::Text("Prescaler: %s", agb->timers[1].get_prescaler_string().c_str());

  // =========================
  ImGui::SeparatorText("Timer 2");
  bool enabled_2     = agb->timers[2].ctrl.timer_start_stop;
  bool irq_enabled_2 = agb->timers[2].ctrl.timer_irq_enable;

  ImGui::Checkbox("Ticking Enabled##l2", &enabled_2);
  ImGui::Checkbox("IRQ Enabled##il2", &irq_enabled_2);
  ImGui::Text("Counter: %d", agb->timers[2].counter);
  ImGui::Text("Prescaler: %s", agb->timers[2].get_prescaler_string().c_str());

  // =========================
  ImGui::SeparatorText("Timer 3");
  bool enabled_3     = agb->timers[3].ctrl.timer_start_stop;
  bool irq_enabled_3 = agb->timers[3].ctrl.timer_irq_enable;

  ImGui::Checkbox("Ticking Enabled##l3", &enabled_3);
  ImGui::Checkbox("IRQ Enabled##il3", &irq_enabled_3);

  ImGui::Text("Counter: %d", agb->timers[3].counter);
  ImGui::Text("Prescaler: %s", agb->timers[3].get_prescaler_string().c_str());

  ImGui::End();
}
void Frontend::show_viewport() {
  ImGui::Begin("Viewport");
  ImGui::Image(state.ppu_texture, {ImGui::GetWindowWidth() - 10, ImGui::GetWindowHeight() - 10});

  ImGui::End();
}
void Frontend::show_audio_info() {
  ImGui::Begin("Audio", &state.apu_window_open, 0);
  ImGui::Text("Write position: %d", agb->apu.write_pos);
  ImGui::Text("FIFO A size: %lu", agb->apu.FIFO_A.size());
  ImGui::Text("FIFO B size: %lu", agb->apu.FIFO_B.size());

  ImGui::End();
}
void Frontend::show_dma_info() {
  ImGui::Begin("DMA Info");

  ImGui::SeparatorText("DMA0");
  auto& ch0 = agb->dma_channels[0];
  ImGui::Text("SRC: %08X", ch0->src);
  ImGui::Text("DST: %08X", ch0->dst);
  ImGui::Text("INT SRC: %08X", ch0->internal_src);
  ImGui::Text("INT DST: %08X", ch0->internal_dst);
  ImGui::Text("ACTIVE: %01X", ch0->dmacnt_h.dma_enable ? 1 : 0);
  ImGui::Text("COND: %s %s", TIMING_MAP.at(static_cast<DMA_START_TIMING>(ch0->dmacnt_h.start_timing)).c_str(),
              ch0->dmacnt_h.start_timing == DMA_START_TIMING::SPECIAL ? SPECIAL_COND_MAP.at(ch0->id).c_str() : "");
  ImGui::Text("repeat: %d", ch0->dmacnt_h.dma_repeat);
  ImGui::SeparatorText("DMA1");
  ch0 = agb->dma_channels[1];
  ImGui::Text("SRC: %08X", ch0->src);
  ImGui::Text("DST: %08X", ch0->dst);
  ImGui::Text("INT SRC: %08X", ch0->internal_src);
  ImGui::Text("INT DST: %08X", ch0->internal_dst);
  ImGui::Text("ACTIVE: %01X", ch0->dmacnt_h.dma_enable ? 1 : 0);
  ImGui::Text("COND: %s %s", TIMING_MAP.at(static_cast<DMA_START_TIMING>(ch0->dmacnt_h.start_timing)).c_str(),
              ch0->dmacnt_h.start_timing == DMA_START_TIMING::SPECIAL ? SPECIAL_COND_MAP.at(ch0->id).c_str() : "");
  ImGui::Text("repeat: %d", ch0->dmacnt_h.dma_repeat);
  ImGui::SeparatorText("DMA2");
  ch0 = agb->dma_channels[2];
  ImGui::Text("SRC: %08X", ch0->src);
  ImGui::Text("DST: %08X", ch0->dst);
  ImGui::Text("INT SRC: %08X", ch0->internal_src);
  ImGui::Text("INT DST: %08X", ch0->internal_dst);
  ImGui::Text("ACTIVE: %01X", ch0->dmacnt_h.dma_enable ? 1 : 0);
  ImGui::Text("COND: %s %s", TIMING_MAP.at(static_cast<DMA_START_TIMING>(ch0->dmacnt_h.start_timing)).c_str(),
              ch0->dmacnt_h.start_timing == DMA_START_TIMING::SPECIAL ? SPECIAL_COND_MAP.at(ch0->id).c_str() : "");
  ImGui::Text("repeat: %d", ch0->dmacnt_h.dma_repeat);
  ImGui::End();
}
void Frontend::show_backgrounds() {
  ImGui::Begin("Backgrounds", &state.backgrounds_window_open, 0);
  const char* backgrounds[] = {"BG0", "BG1", "BG2", "BG3", "viewport", "backdrop", "BG2 (rot scal)"};

  static int SelectedItem = 0;
  ImGui::Text("Enabled BG(s)");
  for (int bg_id = 0; bg_id < 4; bg_id++) {
    if (!agb->ppu.background_enabled(bg_id)) continue;
    ImGui::Text("BG %d", bg_id);
  }
  if (ImGui::Combo("Regions", &SelectedItem, backgrounds, IM_ARRAYSIZE(backgrounds))) {
    fmt::println("switched to: {}", backgrounds[SelectedItem]);
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
    case 6: {
      ImGui::Image(state.background_affine_textures[2], {1024, 1024});
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
  ImGui::Text("CYCLES ELAPSED: %lu", cycles_elapsed);

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
  // if (ImGui::Button("FORCE NEW TILE LOAD")) {
  //   // agb->ppu.repopulate_objs();
  //
  //   agb->ppu.load_tiles(0, agb->ppu.display_fields.BG0CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
  //   agb->ppu.load_tiles(1, agb->ppu.display_fields.BG1CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
  //   agb->ppu.load_tiles(2, agb->ppu.display_fields.BG2CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
  //   agb->ppu.load_tiles(3, agb->ppu.display_fields.BG3CNT.COLOR_MODE);  // only gets called when dispstat corresponding to bg changes
  // }

  ImGui::End();
}

void Frontend::show_window_info() {
  ImGui::Begin("PPU INFO", &state.window_info_open, 0);
  ImGui::Text("WINDOW 0 ENABLED: %d", agb->ppu.display_fields.DISPCNT.WINDOW_0_DISPLAY_FLAG);
  ImGui::Text("WINDOW 1 ENABLED: %d", agb->ppu.display_fields.DISPCNT.WINDOW_1_DISPLAY_FLAG);
  ImGui::Text("OBJ WINDOW ENABLED: %d", agb->ppu.display_fields.DISPCNT.OBJ_WINDOW_DISPLAY_FLAG);

  ImGui::Text("WINDOW 0: X2: %d", agb->ppu.display_fields.WIN0H.X2);
  ImGui::Text("WINDOW 0: Y2: %d", agb->ppu.display_fields.WIN0V.Y2);

  ImGui::Text("WINDOW 1: X2: %d", agb->ppu.display_fields.WIN1H.X2);
  ImGui::Text("WINDOW 1: Y2: %d", agb->ppu.display_fields.WIN1V.Y2);

  ImGui::SeparatorText("WININ");
  ImGui::Text("W0 BG0 ENABLED: %d", agb->ppu.display_fields.WININ.WIN0_BG_ENABLE_BITS & 1);
  ImGui::Text("W0 BG1 ENABLED: %d", agb->ppu.display_fields.WININ.WIN0_BG_ENABLE_BITS >> 1 & 1);
  ImGui::Text("W0 BG2 ENABLED: %d", agb->ppu.display_fields.WININ.WIN0_BG_ENABLE_BITS >> 2 & 1);
  ImGui::Text("W0 BG3 ENABLED: %d", agb->ppu.display_fields.WININ.WIN0_BG_ENABLE_BITS >> 3 & 1);

  ImGui::SeparatorText("WINOUT");

  ImGui::End();
}
void Frontend::show_blend_info() {
  ImGui::Begin("BLEND INFO", &state.blend_info_open, 0);
  ImGui::Text("COLOR SPECIAL FX: %s", agb->ppu.special_fx_str_map.at(agb->ppu.display_fields.BLDCNT.COLOR_SPECIAL_FX).c_str());

  ImGui::Text("BG0 1ST TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG0_1ST_TARGET_PIXEL);
  ImGui::Text("BG1 1ST TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG1_1ST_TARGET_PIXEL);
  ImGui::Text("BG2 1ST TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG2_1ST_TARGET_PIXEL);
  ImGui::Text("BG3 1ST TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG3_1ST_TARGET_PIXEL);
  ImGui::Text("OBJ 1ST TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.OBJ_1ST_TARGET_PIXEL);
  ImGui::Text("BD 1ST TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BD_1ST_TARGET_PIXEL);
  // ImGui::Text("COLOR SPECIAL FX: %d", agb->ppu.display_fields.BLDCNT.COLOR_SPECIAL_FX);
  ImGui::Separator();
  ImGui::Text("BG0 2ND TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG0_2ND_TARGET_PIXEL);
  ImGui::Text("BG1 2ND TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG1_2ND_TARGET_PIXEL);
  ImGui::Text("BG2 2ND TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG2_2ND_TARGET_PIXEL);
  ImGui::Text("BG3 2ND TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BG3_2ND_TARGET_PIXEL);
  ImGui::Text("OBJ 2ND TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.OBJ_2ND_TARGET_PIXEL);
  ImGui::Text("BD 2ND TARGET PIXEL: %d", agb->ppu.display_fields.BLDCNT.BD_2ND_TARGET_PIXEL);

  ImGui::End();
}
void Frontend::show_ppu_info() {
  ImGui::Begin("PPU INFO", &state.cpu_info_open, 0);

  // ImGui::Text("FPS: %.2f", (1 / agb->stopwatch.duration.count()) * 1000.0f);

  ImGui::Text("BG0 PRIORITY: %d", agb->ppu.display_fields.BG0CNT.BG_PRIORITY);
  ImGui::Text("BG0 CHAR_BASE_BLOCK: %d", agb->ppu.display_fields.BG0CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG0 MOSAIC: %d", agb->ppu.display_fields.BG0CNT.MOSAIC);
  ImGui::Text("BG0 COLOR MODE: %s", agb->ppu.display_fields.BG0CNT.color_depth == COLOR_DEPTH::BPP8 ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG0 SCREEN_BASE_BLOCK: %d", agb->ppu.display_fields.BG0CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG0 SCREEN_SIZE: %d (%s)", agb->ppu.display_fields.BG0CNT.SCREEN_SIZE, agb->ppu.screen_sizes_str_map.at(+agb->ppu.display_fields.BG0CNT.SCREEN_SIZE).data());

  ImGui::Text("BG1 PRIORITY: %d", agb->ppu.display_fields.BG1CNT.BG_PRIORITY);
  ImGui::Text("BG1 CHAR_BASE_BLOCK: %d", agb->ppu.display_fields.BG1CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG1 MOSAIC: %d", agb->ppu.display_fields.BG1CNT.MOSAIC);
  ImGui::Text("BG1 COLOR MODE: %s", agb->ppu.display_fields.BG1CNT.color_depth == COLOR_DEPTH::BPP8 ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG1 SCREEN_BASE_BLOCK: %d", agb->ppu.display_fields.BG1CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG1 SCREEN_SIZE: %d (%s)", agb->ppu.display_fields.BG1CNT.SCREEN_SIZE, agb->ppu.screen_sizes_str_map.at(+agb->ppu.display_fields.BG1CNT.SCREEN_SIZE).data());

  ImGui::Text("BG2 PRIORITY: %d", agb->ppu.display_fields.BG2CNT.BG_PRIORITY);
  ImGui::Text("BG2 CHAR_BASE_BLOCK: %d", agb->ppu.display_fields.BG2CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG2 MOSAIC: %d", agb->ppu.display_fields.BG2CNT.MOSAIC);
  ImGui::Text("BG2 COLOR MODE: %s", agb->ppu.display_fields.BG2CNT.color_depth == COLOR_DEPTH::BPP8 ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG2 SCREEN_BASE_BLOCK: %d", agb->ppu.display_fields.BG2CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG2 SCREEN_SIZE: %d (%s)", agb->ppu.display_fields.BG2CNT.SCREEN_SIZE, agb->ppu.screen_sizes_str_map.at(+agb->ppu.display_fields.BG2CNT.SCREEN_SIZE).data());

  ImGui::Text("BG3 PRIORITY: %d", agb->ppu.display_fields.BG3CNT.BG_PRIORITY);
  ImGui::Text("BG3 CHAR_BASE_BLOCK: %d", agb->ppu.display_fields.BG3CNT.CHAR_BASE_BLOCK);
  ImGui::Text("BG3 MOSAIC: %d", agb->ppu.display_fields.BG3CNT.MOSAIC);
  ImGui::Text("BG3 COLOR MODE: %s", agb->ppu.display_fields.BG3CNT.color_depth == COLOR_DEPTH::BPP8 ? "8bpp (256 colors)" : "4bpp (16 colors)");
  ImGui::Text("BG3 SCREEN_BASE_BLOCK: %d", agb->ppu.display_fields.BG3CNT.SCREEN_BASE_BLOCK);
  ImGui::Text("BG3 SCREEN_SIZE: %d (%s)", agb->ppu.display_fields.BG3CNT.SCREEN_SIZE, agb->ppu.screen_sizes_str_map.at(+agb->ppu.display_fields.BG3CNT.SCREEN_SIZE).data());

  ImGui::Text("BG0HOFS: %d", agb->ppu.display_fields.BG0HOFS.OFFSET);
  ImGui::Text("BG0VOFS: %d", agb->ppu.display_fields.BG0VOFS.OFFSET);
  ImGui::Text("BG1HOFS: %d", agb->ppu.display_fields.BG1HOFS.OFFSET);
  ImGui::Text("BG1VOFS: %d", agb->ppu.display_fields.BG1VOFS.OFFSET);
  ImGui::Text("BG2HOFS: %d", agb->ppu.display_fields.BG2HOFS.OFFSET);
  ImGui::Text("BG2VOFS: %d", agb->ppu.display_fields.BG2VOFS.OFFSET);
  ImGui::Text("BG3HOFS: %d", agb->ppu.display_fields.BG3HOFS.OFFSET);
  ImGui::Text("BG3VOFS: %d", agb->ppu.display_fields.BG3VOFS.OFFSET);

  ImGui::Text("BG2X: %d", agb->ppu.display_fields.BG2HOFS.OFFSET);
  ImGui::Text("BG2VOFS: %d", agb->ppu.display_fields.BG2VOFS.OFFSET);
  ImGui::Text("BG3HOFS: %d", agb->ppu.display_fields.BG3HOFS.OFFSET);
  ImGui::Text("BG3VOFS: %d", agb->ppu.display_fields.BG3VOFS.OFFSET);

  ImGui::Text("LY: %d", agb->ppu.display_fields.VCOUNT.LY);
  // ImGui::Text("Scanline Cycle: %d", agb->ppu.scanline_cycle);
  ImGui::Separator();

  ImGui::Text("BG MODE: %#02x", agb->ppu.display_fields.DISPCNT.BG_MODE);
  ImGui::Text("OBJ DRAW: %#02x", agb->ppu.display_fields.DISPCNT.SCREEN_DISPLAY_OBJ);

  ImGui::Text("OBJ VRAM MAPPING: %s", agb->ppu.display_fields.DISPCNT.OBJ_CHAR_VRAM_MAPPING == 0 ? "2D" : "1D");
  ImGui::Text("OBJ Window: %#010x", agb->ppu.display_fields.DISPCNT.OBJ_WINDOW_DISPLAY_FLAG);

  if (ImGui::Button("Set VBLANK")) {
    agb->ppu.display_fields.DISPSTAT.VBLANK_FLAG = true;
  }
  if (ImGui::Button("Reset VBLANK")) {
    agb->ppu.display_fields.DISPSTAT.VBLANK_FLAG = false;
  }
  if (ImGui::Button("Draw")) {
    agb->ppu.step();
  }
  // if (ImGui::Button("Draw Tileset")) {
  //   agb->ppu.step(true);
  //   // SDL_UpdateTexture(state.tile_set_texture, nullptr, agb->ppu.tile_set_texture, 240 * 4);
  //   // SDL_UpdateTexture(state.tile_map_texture, nullptr, agb->ppu.tile_map_texture_buffer, 512 * 4);
  // }
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
  // if (state.cpu_info_open) {
  //   show_cpu_info();
  // }
  // if (state.ppu_info_open) {
  //   show_ppu_info();
  // }
  // if (state.window_info_open) {
  //   show_window_info();
  // }

  // if (state.apu_window_open) {
  //   show_audio_info();
  // }
  // if (state.memory_viewer_open) {
  //   show_memory_viewer();
  // }

  // if (state.show_dma_menu) {
  //   show_dma_info();
  // }
  // if (state.timer_window_open) {
  //   show_timer_window();
  // }
  // if (state.blend_info_open) {
  //   show_blend_info();
  // }

  // show_irq_status();
  // show_tiles();
  // show_backgrounds();
  // show_viewport();

  SDL_SetWindowTitle(window, fmt::format("{}fps", 1000 / Stopwatch::duration.count()).c_str());

  ImGui::Render();
  SDL_SetRenderScale(renderer, state.io->DisplayFramebufferScale.x, state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);

  SDL_UpdateTexture(state.background_textures[0], nullptr, agb->ppu.tile_map_texture_buffer_arr[0].data(), 512 * 4);
  SDL_UpdateTexture(state.background_textures[1], nullptr, agb->ppu.tile_map_texture_buffer_arr[1].data(), 512 * 4);
  SDL_UpdateTexture(state.background_textures[2], nullptr, agb->ppu.tile_map_texture_buffer_arr[2].data(), 512 * 4);
  SDL_UpdateTexture(state.background_textures[3], nullptr, agb->ppu.tile_map_texture_buffer_arr[3].data(), 512 * 4);
  SDL_UpdateTexture(state.background_affine_textures[0], nullptr, agb->ppu.tile_map_affine_texture_buffer_arr[0].data(), 1024 * 4);
  SDL_UpdateTexture(state.background_affine_textures[1], nullptr, agb->ppu.tile_map_affine_texture_buffer_arr[1].data(), 1024 * 4);
  SDL_UpdateTexture(state.background_affine_textures[2], nullptr, agb->ppu.tile_map_affine_texture_buffer_arr[2].data(), 1024 * 4);
  SDL_UpdateTexture(state.background_affine_textures[3], nullptr, agb->ppu.tile_map_affine_texture_buffer_arr[3].data(), 1024 * 4);

  SDL_UpdateTexture(state.backdrop, nullptr, agb->ppu.backdrop.data(), 512 * 4);

  SDL_UpdateTexture(state.ppu_texture, nullptr, agb->ppu.db.disp_buf, 240 * 4);
  SDL_RenderTexture(renderer, state.ppu_texture, &rect, NULL);

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

  SDL_RenderPresent(renderer);
}

void audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int in_total) {
  (void)(in_total);
  assert(userdata != nullptr);
  u32 samples = ((additional_amount) / sizeof(float));
  AGB* agb    = (AGB*)userdata;

  if (additional_amount / sizeof(float) < 4096) fmt::println("need more than 4K samples");

  // fmt::println("SDL wants: {} bytes --- {} samples", additional_amount, additional_amount / sizeof(float));
  // fmt::println("SDL in_total: {} bytes --- {} samples", in_total, in_total / sizeof(float));

  // fmt::println("pre-stream: {}", SDL_GetAudioStreamAvailable(stream));
  agb->apu.write_pos = 0;

  while (agb->apu.write_pos < samples) {
    agb->step();
  }

  // fmt::println("post write pos: {} - samples again: {}", gb->apu.write_pos, samples);
  if (!SDL_PutAudioStreamData(stream, agb->apu.audio_buf.data(), additional_amount)) {
    fmt::println("failed to put data");
    assert(0);
  }
  // fmt::println("post-stream: {}", SDL_GetAudioStreamAvailable(stream));
};

void Frontend::init_sdl() {
  spdlog::info("initializing SDL");

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
    spdlog::error("ERROR: failed initialize SDL");
    exit(-1);
  }

  auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE);

  this->window = SDL_CreateWindow("bass", 640, 480, window_flags);
  if (this->window == nullptr) {
    spdlog::error("failed to create window");
    assert(0);
  }
  // SDL_SetRenderVSync(renderer, 1);

  this->renderer = SDL_CreateRenderer(window, nullptr);
  init_audio_device();
  if (this->renderer == nullptr) {
    spdlog::error("failed to create renderer");
    assert(0);
  }

  spdlog::info("window ptr: {}", fmt::ptr(this->window));
  spdlog::info("render ptr: {}", fmt::ptr(this->renderer));

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
  SDL_SetTextureScaleMode(state.ppu_texture, SDL_SCALEMODE_NEAREST);

  for (size_t bg_id = 0; bg_id < 4; bg_id++) {
    this->state.background_textures[bg_id] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 512, 512);
  }
  for (size_t bg_id = 0; bg_id < 4; bg_id++) {
    this->state.background_affine_textures[bg_id] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_TARGET, 1024, 1024);
  }
};
void Frontend::init_audio_device() {
  SDL_AudioSpec spec = {};
  spec.freq          = 48000;
  spec.format        = SDL_AUDIO_S8;
  spec.channels      = 2;
  // stream             = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, agb);
  stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);

  if (!stream) {
    fmt::println("Couldn't create audio stream: {}", SDL_GetError());
    exit(-1);
  }

  if (!SDL_ResumeAudioStreamDevice(stream)) {
    fmt::println("failed to unpause audio device: {}", SDL_GetError());
    assert(0);
  }

  spdlog::info("initialized audio device");
}

Frontend::Frontend(AGB* c) : agb(c) { init_sdl(); }

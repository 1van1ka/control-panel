#include "../include/ui.h"
#include <stdbool.h>

void create_ui(struct App *a) {
  layout_add_button(a, ButtonPlayerPrev, "Prev", "Prev", "Prev", true);
  layout_add_button(a, ButtonPlayerPlayPause, "Play", "Play", "Pause", true);
  layout_add_button(a, ButtonPlayerNext, "Next", "Next", "Next", false);
  layout_new_row();

  layout_add_button(a, ButtonWiFi, get_state_wifi() ? "Wi-Fi:On" : "Wi-Fi:Off",
                    "Wi-Fi:On", "Wi-Fi:Off", false);
  layout_add_button(a, ButtonBluetooth,
                    get_state_bluetooth() ? "BT:On" : "BT:Off", "BT:On",
                    "BT:Off", false);
  layout_new_row();

  layout_add_button(a, ButtonNotify, get_state_dunst() ? "Notify" : "Silence",
                    "Notify", "Silence", true);
  layout_new_row();

  layout_add_label(a, "Brn", false);
  layout_add_slider(a, SliderBrightness, get_level_brightness(), 100, true);
  layout_new_row();

  layout_add_button(a, ButtonVolumeMute, get_state_audio_mute() ? "Mut" : "Vol",
                    "Mut", "Vol", false);
  layout_add_slider(a, SliderVolume, get_level_audio(), 120, true);

  spawn_state_thread(a, SliderBrightness, WIDGET_SLIDER, get_level_brightness);
  spawn_state_thread(a, SliderVolume, WIDGET_SLIDER, get_level_audio);
  spawn_state_thread(a, ButtonVolumeMute, WIDGET_BUTTON, get_state_audio_mute);
}

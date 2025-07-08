#include "../include/ui.h"
#include <stdbool.h>
#include <string.h>

XftColor color_alloc(struct App *a, XRenderColor *color_src) {
  XftColor color_dest;
  XftColorAllocValue(dpy, a->visual, a->colormap, color_src, &color_dest);
  return color_dest;
}

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

void layout_add_button(struct App *a, enum WidgetId id, const char *text,
                       char *valid_data, char *invalid_data, bool full_width) {
  XGlyphInfo extents;
  XftTextExtents8(dpy, a->font, (FcChar8 *)text, strlen(text), &extents);

  int width = extents.xOff;
  int height = a->font->ascent + a->font->descent;

  a->widgets[a->widget_count++] =
      (struct Widget){.type = WIDGET_BUTTON,
                      .valid_label = strdup(valid_data),
                      .invalid_label = strdup(invalid_data),
                      .id = id,
                      .x = layout_x,
                      .y = layout_y + height,
                      .text = strdup(text),
                      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
                      .normal_back = color_alloc(a, &ColorsSrc[NormBg]),
                      .hover_color = color_alloc(a, &ColorsSrc[HoverFg]),
                      .hover_back = color_alloc(a, &ColorsSrc[HoverBg]),
                      .hovered = false,
                      .full_width = full_width};

  layout_x += width + 2 * PADDING + layout_spacing_x;
  if (height + 2 * PADDING > layout_row_height)
    layout_row_height = height + 2 * PADDING;
}

void layout_add_label(struct App *a, const char *text, bool full_width) {
  XGlyphInfo extents;
  XftTextExtents8(dpy, a->font, (FcChar8 *)text, strlen(text), &extents);

  int width = extents.xOff;
  int height = a->font->ascent + a->font->descent;

  a->widgets[a->widget_count++] =
      (struct Widget){.type = WIDGET_LABEL,
                      .id = IdNone,
                      .x = layout_x,
                      .y = layout_y + height,
                      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
                      .text = strdup(text),
                      .full_width = full_width};

  layout_x += width + 2 * PADDING + layout_spacing_x;
  if (height + 2 * PADDING > layout_row_height)
    layout_row_height = height + 2 * PADDING;
}

void layout_add_slider(struct App *a, enum WidgetId id, int value,
                       int max_value, bool full_width) {
  int height = 14;
  int width = 200;
  int slider_y = layout_y + a->font->ascent;

  a->widgets[a->widget_count++] =
      (struct Widget){.type = WIDGET_SLIDER,
                      .id = id,
                      .x = layout_x,
                      .y = layout_y + a->font->ascent,
                      .width = width,
                      .knob = height,
                      .height = height,
                      .normal_color = color_alloc(a, &ColorsSrc[NormFgSlider]),
                      .normal_back = color_alloc(a, &ColorsSrc[NormBg]),
                      .hover_color = color_alloc(a, &ColorsSrc[HoverFg]),
                      .hover_back = color_alloc(a, &ColorsSrc[HoverBg]),
                      .slider_value = value,
                      .max_value = max_value,
                      .full_width = full_width};

  layout_x += width + layout_spacing_x + PADDING;

  int total_height = (slider_y - layout_y) + height / 2 + 2 * PADDING;
  if (total_height > layout_row_height)
    layout_row_height = total_height;
}

void layout_new_row(void) {
  layout_x = 20;
  layout_y += layout_row_height + layout_spacing_y;
  layout_row_height = 0;
}

#include "../include/ui.h"
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

XftColor color_alloc(struct App *a, XRenderColor *color_src) {
  XftColor color_dest;
  XftColorAllocValue(dpy, a->visual, a->colormap, color_src, &color_dest);
  return color_dest;
}

void create_ui(struct App *a) {
  layout_add_button(a, ButtonPlayerPrev, NULL, "Prev", "Prev", true);
  layout_add_button(a, ButtonPlayerPlayPause, NULL, "Play", "Paus", true);
  layout_add_button(a, ButtonPlayerNext, NULL, "Next", "Next", false);
  layout_new_row();

  layout_add_button(a, ButtonWiFi, get_state_wifi, "Wi-Fi:On", "Wi-Fi:Off",
                    false);
  layout_add_button(a, ButtonBluetooth, get_state_bluetooth, "BT:On", "BT:Off",
                    false);
  layout_new_row();

  layout_add_button(a, ButtonNotify, get_state_dunst, "Notify", "Silence",
                    true);
  layout_new_row();

  layout_add_label(a, LabelBrightness, NULL, "Brn", "Brn", false);
  layout_add_slider(a, SliderBrightness, get_level_brightness(), 100, true);
  layout_new_row();

  layout_add_button(a, ButtonVolumeMute, get_state_audio_mute, "Mute", "Vol",
                    false);
  layout_add_slider(a, SliderVolume, get_level_audio(), 120, true);

  spawn_state_thread(a, SliderBrightness, WIDGET_SLIDER, get_level_brightness);
  spawn_state_thread(a, SliderVolume, WIDGET_SLIDER, get_level_audio);
  spawn_state_thread(a, ButtonVolumeMute, WIDGET_BUTTON, get_state_audio_mute);
}

void layout_add_button(struct App *a, enum WidgetId id, int (*state)(),
                       char *valid_data, char *invalid_data, bool full_width) {
  XGlyphInfo extents_valid, extents_invalid;
  XftTextExtents8(dpy, a->font, (FcChar8 *)valid_data, strlen(valid_data),
                  &extents_valid);
  XftTextExtents8(dpy, a->font, (FcChar8 *)invalid_data, strlen(invalid_data),
                  &extents_invalid);

  int width = extents_valid.xOff > extents_invalid.xOff ? extents_valid.xOff
                                                        : extents_invalid.xOff;
  int height = a->font->ascent + a->font->descent;

  a->widgets[id] = (struct Widget){
      .id = id,
      .type = WIDGET_BUTTON,
      .valid_label = strdup(valid_data),
      .invalid_label = strdup(invalid_data),
      .x = layout_x,
      .y = layout_y + height,
      .text = strdup((state ? state() : 1) ? valid_data : invalid_data),
      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
      .normal_back = color_alloc(a, &ColorsSrc[NormBg]),
      .hover_color = color_alloc(a, &ColorsSrc[HoverFg]),
      .hover_back = color_alloc(a, &ColorsSrc[HoverBg]),
      .hovered = false,
      .full_width = full_width,
      .width = width,
      .height = height};

  layout_x += width + 2 * PADDING + layout_spacing_x;
  if (height + 2 * PADDING > layout_row_height)
    layout_row_height = height + 2 * PADDING;
}

void layout_add_label(struct App *a, enum WidgetId id, int (*state)(),
                      char *valid_data, char *invalid_data, bool full_width) {
  XGlyphInfo extents_valid, extents_invalid;
  XftTextExtents8(dpy, a->font, (FcChar8 *)valid_data, strlen(valid_data),
                  &extents_valid);
  XftTextExtents8(dpy, a->font, (FcChar8 *)invalid_data, strlen(invalid_data),
                  &extents_invalid);

  int width = extents_valid.xOff > extents_invalid.xOff ? extents_valid.xOff
                                                        : extents_invalid.xOff;
  int height = a->font->ascent + a->font->descent;

  a->widgets[id] = (struct Widget){
      .type = WIDGET_LABEL,
      .id = id,
      .x = layout_x,
      .y = layout_y + height,
      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
      .text = strdup((state ? state() : 1) ? valid_data : invalid_data),
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

  a->widgets[id] =
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

#include "../include/ui.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

const int PADDING = 7;
const int layout_spacing_x = 10;
const int layout_spacing_y = 10;
int layout_x = layout_spacing_x;
int layout_y = layout_spacing_y;
int layout_row_height = 0;

XftColor color_alloc(struct App *a, XRenderColor *color_src) {
  XftColor color_dest;
  XftColorAllocValue(dpy, a->visual, a->colormap, color_src, &color_dest);
  return color_dest;
}

void create_ui(struct App *a) {

  // struct Layout layout[][3] = {
  //     {{.type = WIDGET_BUTTON,
  //       .button = {a, ButtonPlayerPrev, NULL, "Prev", "Prev", false}},
  //      {.type = WIDGET_BUTTON,
  //       .button = {a, ButtonPlayerPlayPause, NULL, "Play", "Paus", true}},
  //      {.type = WIDGET_BUTTON,
  //       .button = {a, ButtonPlayerNext, NULL, "Next", "Next", false}}},
  //
  //     {{.type = WIDGET_BUTTON,
  //       .button = {a, ButtonWiFi, get_state_wifi, "Wi-Fi:On", "Wi-Fi:Off",
  //                  false}},
  //      {.type = WIDGET_BUTTON,
  //       .button = {a, ButtonBluetooth, get_state_bluetooth, "BT:On", "BT:Off",
  //                  false}}},
  //
  //     {{.type = WIDGET_BUTTON,
  //       .button = {a, ButtonNotify, get_state_dunst, "Notify", "Silence",
  //                  false}}},
  //     {{.type = WIDGET_LABEL,
  //       .label = {a, LabelBrightness, NULL, "Brgh", "Brgh", false}},
  //      {.type = WIDGET_SLIDER,
  //       .slider = {a, SliderBrightness, get_level_brightness(), 100, true}}},
  //
  //     {{.type = WIDGET_BUTTON,
  //       .button = {a, ButtonVolumeMute, get_state_audio_mute, "Mute", "Vol",
  //                  false}},
  //      {.type = WIDGET_SLIDER,
  //       .slider = {a, SliderVolume, get_level_audio(), 120, true}}}};

  struct Layout layout[][3] = {
      {{.type = WIDGET_BUTTON,
        .button = {a, ButtonPlayerPrev, NULL, "Prev", "Prev", true}},
       {.type = WIDGET_BUTTON,
        .button = {a, ButtonPlayerPlayPause, NULL, "Play", "Paus", true}},
       {.type = WIDGET_BUTTON,
        .button = {a, ButtonPlayerNext, NULL, "Next", "Next", true}}},

      {{.type = WIDGET_BUTTON,
        .button = {a, ButtonWiFi, get_state_wifi, "Wi-Fi:On", "Wi-Fi:Off",
                   true}},
       {.type = WIDGET_BUTTON,
        .button = {a, ButtonBluetooth, get_state_bluetooth, "BT:On",
        "BT:Off",
                   true}}},

      {{.type = WIDGET_BUTTON,
        .button = {a, ButtonNotify, get_state_dunst, "Notify", "Silence",
                   true}}},

      {{.type = WIDGET_LABEL,
        .label = {a, LabelBrightness, NULL, "Brgh", "Brgh", false}},
       {.type = WIDGET_SLIDER,
        .slider = {a, SliderBrightness, get_level_brightness(), 100, true}}},

      {{.type = WIDGET_BUTTON,
        .button = {a, ButtonVolumeMute, get_state_audio_mute, "Mute", "Vol",
                   false}},
       {.type = WIDGET_SLIDER,
        .slider = {a, SliderVolume, get_level_audio(), 120, true}}}};

  int rows = sizeof(layout) / sizeof(layout[0]);
  int layout_cols[] = {3, 2, 1, 2, 2};

  for (int i = 0; i < rows; ++i) {
    int num_cols = layout_cols[i];
    int fixed_width = 0;
    int dynamic_count = 0;

    for (int j = 0; j < num_cols; j++) {
      struct Layout *l = &layout[i][j];

      bool is_dynamic = false;
      int width = 0;

      switch (l->type) {
      case WIDGET_BUTTON: {
        is_dynamic = l->button.full_width;
        width =
            get_text_width(a, l->button.valid_data, l->button.invalid_data) +
            2 * PADDING;
        break;
      }
      case WIDGET_LABEL: {
        is_dynamic = l->label.full_width;
        width = get_text_width(a, l->label.valid_data, l->label.invalid_data) +
                2 * PADDING;
        break;
      }
      case WIDGET_SLIDER: {
        is_dynamic = l->slider.full_width;
        width = 100;
        break;
      }
      default:
        break;
      }

      if (is_dynamic)
        dynamic_count++;
      else
        fixed_width += width;
    }

    int total_width = a->width_app - 2 * layout_x;
    fixed_width += (num_cols - 1) * (layout_spacing_x);
    int dynamic_width =
        dynamic_count > 0 ? (total_width - fixed_width) / dynamic_count : 0;

    for (int j = 0; j < num_cols; j++) {
      bool is_last = (j == num_cols - 1);
      struct Layout *l = &layout[i][j];

      switch (l->type) {
      case WIDGET_BUTTON:
        layout_add_button(a, l->button.id, l->button.state,
                          l->button.valid_data, l->button.invalid_data,
                          l->button.full_width ? dynamic_width : 0, is_last);
        break;

      case WIDGET_LABEL:
        layout_add_label(a, l->label.id, l->label.state, l->label.valid_data,
                         l->label.invalid_data,
                         l->label.full_width ? dynamic_width : 0, is_last);
        break;

      case WIDGET_SLIDER:
        layout_add_slider(a, l->slider.id, l->slider.value, l->slider.max_value,
                          l->slider.full_width ? dynamic_width : 0, is_last);
        break;
      default:
        break;
      }
    }

    layout_new_row();
  }

  spawn_state_thread(a, SliderBrightness, WIDGET_SLIDER, get_level_brightness);
  spawn_state_thread(a, SliderVolume, WIDGET_SLIDER, get_level_audio);
  spawn_state_thread(a, ButtonVolumeMute, WIDGET_BUTTON, get_state_audio_mute);
}

int get_text_width(struct App *a, const char *valid, const char *invalid) {
  XGlyphInfo extents_valid, extents_invalid;
  XftTextExtents8(dpy, a->font, (FcChar8 *)valid, strlen(valid),
                  &extents_valid);
  XftTextExtents8(dpy, a->font, (FcChar8 *)invalid, strlen(invalid),
                  &extents_invalid);
  return extents_valid.xOff > extents_invalid.xOff ? extents_valid.xOff
                                                   : extents_invalid.xOff;
}

void layout_add_button(struct App *a, enum WidgetId id, int (*state)(),
                       char *valid_data, char *invalid_data, int override_width,
                       bool is_last) {
  int width = override_width > 0
                  ? override_width
                  : get_text_width(a, valid_data, invalid_data) + 2 * PADDING;
  int height = a->font->ascent + a->font->descent + 2 * PADDING;

  widgets[id] = (struct Widget){
      .id = id,
      .type = WIDGET_BUTTON,
      .valid_label = strdup(valid_data),
      .invalid_label = strdup(invalid_data),
      .x = layout_x,
      .y = layout_y + a->font->ascent,
      .text = strdup((state ? state() : 1) ? valid_data : invalid_data),
      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
      .normal_back = color_alloc(a, &ColorsSrc[NormBg]),
      .hover_color = color_alloc(a, &ColorsSrc[HoverFg]),
      .hover_back = color_alloc(a, &ColorsSrc[HoverBg]),
      .hovered = false,
      .width = width,
      .height = height};

  layout_x += width;

  if (!is_last)
    layout_x += layout_spacing_x;
  if (height > layout_row_height)
    layout_row_height = height;
}

void layout_add_label(struct App *a, enum WidgetId id, int (*state)(),
                      char *valid_data, char *invalid_data, int override_width,
                      bool is_last) {
  int width = override_width > 0
                  ? override_width
                  : get_text_width(a, valid_data, invalid_data) + 2 * PADDING;
  int height = a->font->ascent + a->font->descent + 2 * PADDING;

  widgets[id] = (struct Widget){
      .type = WIDGET_LABEL,
      .id = id,
      .x = layout_x,
      .y = layout_y + a->font->ascent,
      .width = width,
      .height = height,
      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
      .text = strdup((state ? state() : 1) ? valid_data : invalid_data)};

  layout_x += width;
  if (!is_last)
    layout_x += layout_spacing_x;
  if (height > layout_row_height)
    layout_row_height = height;
}

void layout_add_slider(struct App *a, enum WidgetId id, int value,
                       int max_value, int override_width, bool is_last) {
  int height = 14;
  int width = override_width > 0 ? override_width : 200;
  int slider_y = layout_y + (a->font->ascent + a->font->descent) / 2 + PADDING;

  widgets[id] =
      (struct Widget){.type = WIDGET_SLIDER,
                      .id = id,
                      .x = layout_x,
                      .y = slider_y,
                      .width = width,
                      .knob = height,
                      .height = height,
                      .normal_color = color_alloc(a, &ColorsSrc[NormFgSlider]),
                      .normal_back = color_alloc(a, &ColorsSrc[NormBg]),
                      .hover_color = color_alloc(a, &ColorsSrc[HoverFg]),
                      .hover_back = color_alloc(a, &ColorsSrc[HoverBg]),
                      .slider_value = value,
                      .max_value = max_value};

  layout_x += width;
  if (!is_last)
    layout_x += layout_spacing_x;

  int total_height = (slider_y - layout_y) + height;
  if (total_height > layout_row_height)
    layout_row_height = total_height;
}

void layout_new_row(void) {
  layout_x = layout_spacing_x;
  layout_y += layout_row_height + layout_spacing_y;
  layout_row_height = 0;
}

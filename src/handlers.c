#include "../include/handlers.h"
#include <stdio.h>
#include <time.h>

void handler_button(struct Widget *w) {
  switch (w->id) {
  case ButtonVolumeMute:
    on_volume_button_click(w);
    break;
  case ButtonWiFi:
    on_wifi_clicked(w);
    break;
  case ButtonBluetooth:
    on_bluetooth_clicked(w);
    break;
  case ButtonNotify:
    on_notify_clicked(w);
    break;
  case ButtonPlayerPrev:
    on_music_prev_clicked(w);
    break;
  case ButtonPlayerPlayPause:
    on_music_playpause_clicked(w);
    break;
  case ButtonPlayerNext:
    on_music_next_clicked(w);
    break;
  default:
    break;
  }
}

void handler_slider(struct Widget *w) {
  switch (w->id) {
  case SliderBrightness:
    on_brightness_changed(w->slider_value);
    break;
  case SliderVolume:
    on_volume_changed(w->slider_value);
    break;
  default:
    break;
  }
}

int handle_button_press(struct App *a, XEvent *ev) {
  if (ev->xbutton.x_root >= a->width_app || ev->xbutton.x_root <= POS_X ||
      ev->xbutton.y_root <= POS_Y || ev->xbutton.y_root >= a->width_app)
    return 1;
  for (int i = 0; i < a->widget_count; ++i) {
    struct Widget *w = &a->widgets[i];
    if (w->hovered)
      switch (w->type) {
      case WIDGET_BUTTON:
        a->dragging_id = w->id;
        a->dragging_type = w->type;
        handler_button(w);
        break;
      case WIDGET_SLIDER: {
        a->dragging_id = w->id;
        a->dragging_type = w->type;
        int knob_x =
            w->x + (w->slider_value * (w->width - w->knob)) / w->max_value;
        if (ev->xbutton.x >= knob_x && ev->xbutton.x <= knob_x + w->knob) {
          w->drag_offset_x = ev->xbutton.x - knob_x;
        } else {
          int new_val = ((ev->xbutton.x - w->x - w->knob / 2) * w->max_value) /
                        (w->width - w->knob);
          if (new_val < 0)
            new_val = 0;
          if (new_val > w->max_value)
            new_val = w->max_value;
          w->slider_value = new_val;
          handler_slider(w);
          w->drag_offset_x = w->knob / 2;
        }
        break;
      }
      default:
        break;
      }
  }
  return 0;
}

int handle_motion_notify(struct App *a, XEvent *ev) {
  for (int i = 0; i < a->widget_count; i++) {
    struct Widget *w = &a->widgets[i];
    if (w->type == WIDGET_SLIDER && w->id == a->dragging_id) {
      int new_val = ((ev->xmotion.x - w->drag_offset_x - w->x) * w->max_value) /
                    (w->width - w->knob);
      new_val = new_val < 0              ? 0
                : new_val > w->max_value ? w->max_value
                                         : new_val;
      w->slider_value = new_val;
      static struct timespec last_update = {0, 0};
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);

      long delta_ms = (now.tv_sec - last_update.tv_sec) * 1000 +
                      (now.tv_nsec - last_update.tv_nsec) / 1000000;

      if (delta_ms > 20) {
        draw_widget(a, w);
        handler_slider(w);
        last_update = now;
      }
    }
  }
  return 0;
}

void on_bluetooth_clicked(struct Widget *w) {
  int state = get_state_bluetooth();

  if (state == 1) {
    char *argv[] = {"rfkill", "block", "bluetooth", NULL};
    execute_command_args(argv);
    set_value(w, "BT:Off");
  } else if (state == 0) {
    char *argv[] = {"rfkill", "unblock", "bluetooth", NULL};
    execute_command_args(argv);
    set_value(w, "BT:On");
  }
}

void on_brightness_changed(int value) {
  char arg[16];
  snprintf(arg, sizeof(arg), "%d%%", value);

  char *argv[] = {
      "brightnessctl", "--class=backlight", "--min-value=1", "set", arg, NULL};
  execute_command_args(argv);
}

void on_music_next_clicked(__attribute__((unused)) struct Widget *w) {
  char *argv[] = {"playerctl", "next", NULL};
  execute_command_args(argv);
}

void on_music_playpause_clicked(__attribute__((unused)) struct Widget *w) {
  char *argv[] = {"playerctl", "play-pause", NULL};
  execute_command_args(argv);
}

void on_music_prev_clicked(__attribute__((unused)) struct Widget *w) {
  char *argv[] = {"playerctl", "previous", NULL};
  execute_command_args(argv);
}

void on_notify_clicked(struct Widget *w) {
  char *argv1[] = {"dunstctl", "set-paused", "toggle", NULL};
  char *argv2[] = {"pkill", "-RTMIN+3", "dwmblocks", NULL};
  execute_command_args(argv1);
  execute_command_args(argv2);
  set_value(w, get_state_dunst() ? "Notify" : "Silence");
}

void on_volume_changed(int value) {
  char arg[16];
  snprintf(arg, sizeof(arg), "%d%%", value);

  char *argv1[] = {"wpctl", "set-volume", "-l", "1.2", "@DEFAULT_AUDIO_SINK@",
                   arg,     NULL};
  char *argv2[] = {"pkill", "-RTMIN+1", "dwmblocks", NULL};
  execute_command_args(argv1);
  execute_command_args(argv2);
}

void on_volume_button_click(struct Widget *w) {
  char *argv1[] = {"wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle", NULL};
  char *argv2[] = {"pkill", "-RTMIN+1", "dwmblocks", NULL};
  execute_command_args(argv1);
  execute_command_args(argv2);
  set_value(w, get_state_audio_mute() ? "Mut" : "Vol");
}

void on_wifi_clicked(struct Widget *w) {
  char *argv[] = {"nmcli", "radio", "wifi", get_state_wifi() ? "off" : "on",
                  NULL};
  execute_command_args(argv);
  set_value(w, get_state_wifi() ? "Wi-Fi:On" : "Wi-Fi:Off");
}

void update_hover_state(struct App *a, int mouse_x, int mouse_y) {
  for (int i = 0; i < a->widget_count; ++i) {
    struct Widget *w = &a->widgets[i];
    switch (w->type) {
    case WIDGET_BUTTON: {
      int top = w->y - a->font->ascent - PADDING;
      int bottom = w->y - a->font->ascent - PADDING + w->height + 2 * PADDING;
      int left = w->x - PADDING;
      int right = w->x - PADDING + w->width + 2 * PADDING;
      w->hovered =
          (a->dragging_id == w->id && a->dragging_type == w->type) ||
          ((a->dragging_id == IdNone) && (mouse_x >= left && mouse_x <= right &&
                                          mouse_y >= top && mouse_y <= bottom));
      break;
    }
    case WIDGET_SLIDER: {
      int top = w->y - w->height / 2;
      int bottom = w->y + w->height / 2;
      int left = w->x;
      int right = w->x + w->width;
      w->hovered =
          (a->dragging_id == w->id && a->dragging_type == w->type) ||
          ((a->dragging_id == IdNone) && (mouse_x >= left && mouse_x <= right &&
                                          mouse_y >= top && mouse_y <= bottom));
      break;
    }
    default:
      break;
    }
  }
}

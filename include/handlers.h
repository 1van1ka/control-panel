#ifndef HANDLERS_H
#define HANDLERS_H

#include "../config.h"
#include "../include/control_center.h"
#include "../include/getters.h"
#include "../include/utils.h"
#include "utils.h"
#include <X11/Xlib.h>

void on_bluetooth_clicked(struct Widget *w);
void on_brightness_changed(int value);
void on_music_prev_clicked(struct Widget *w);
void on_music_next_clicked(struct Widget *w);
void on_music_playpause_clicked(struct Widget *w);
void on_notify_clicked(struct Widget *w);
void on_volume_changed(int value);
void on_volume_button_click(struct Widget *w);
void on_wifi_clicked(struct Widget *w);
void handler_slider(struct Widget *w);
void handler_button(struct Widget *w);

int handle_motion_notify(struct App *a, XEvent *ev);
int handle_button_press(struct App *a, XEvent *ev);
// void handle_button_release(App *a, XMotionEvent *ev);
// void handle_key_press(App *a, XMotionEvent *ev);
void update_hover_state(struct App *a, int mouse_x, int mouse_y);

#endif

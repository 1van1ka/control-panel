#ifndef CONTROL_CENTER_H
#define CONTROL_CENTER_H

#include "../config.h"
#include "../include/getters.h"
#include "../include/handlers.h"
#include "../include/utils.h"

void cleanup(struct App *a);
void create_ui(struct App *a);
void draw_widget(struct App *a, struct Widget *w);
void draw_widgets(struct App *a);
void grab_focus(void);
void grab_keyboard(void);
void kill_panel(Display *dpy, struct App *a, XClassHint ch);
void layout_add_button(struct App *a, enum WidgetId id, const char *text);
void layout_add_label(struct App *a, const char *text);
void layout_add_slider(struct App *a, enum WidgetId id, int value, int max_value);
void layout_new_row(void);
void run(struct App *a);
void setup(struct App *a);
void spawn_state_thread(struct App *a, int id, enum WidgetType type,
                        int (*getter)(void));
void *state_updater_thread(void *arg);

#endif

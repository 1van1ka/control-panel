#ifndef CONTROL_CENTER_H
#define CONTROL_CENTER_H

#include "config.h"
#include "getters.h"
#include "handlers.h"
#include "utils.h"
#include "ui.h"
#include <stdbool.h>

extern const int PADDING;

void cleanup(struct App *a);
void create_ui(struct App *a);
void draw_widgets(struct App *a);
void grab_focus(void);
void grab_keyboard(void);
void kill_panel(Display *dpy, struct App *a, XClassHint ch);
void redraw_widget(struct App *a, struct Widget *w);
void run(struct App *a);
void setup(struct App *a);
void spawn_state_thread(struct App *a, enum WidgetId id, enum WidgetType type,
                        int (*getter)(void));
void *state_updater_thread(void *arg);
void widget_to_buffer(struct App *a, struct Widget *w);

#endif

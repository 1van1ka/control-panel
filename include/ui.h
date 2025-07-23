#ifndef UI_H
#define UI_H

#include "config.h"
#include "control_center.h"
#include "getters.h"

extern Display *dpy;
extern int layout_x;
extern int layout_y;
extern int layout_row_height;
extern const int layout_spacing_x;
extern const int layout_spacing_y;

void create_ui(struct App *a);
void layout_add_button(struct App *a, enum WidgetId id, int (*state)(),
                       char *valid_data, char *invalid_data, int override_width,
                       bool is_last);
void layout_add_label(struct App *a, enum WidgetId id, int (*state)(),
                      char *valid_data, char *invalid_data, int override_width,
                      bool is_last);
void layout_add_slider(struct App *a, enum WidgetId id, int value,
                       int max_value, int override_width, bool is_last);
void layout_new_row(void);

#endif

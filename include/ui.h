#ifndef UI_H
#define UI_H

#include "../include/control_center.h"
#include "../config.h"
#include "../include/getters.h"

extern Display *dpy;
extern int layout_x;
extern int layout_y;
extern int layout_row_height;
extern const int layout_spacing_x;
extern const int layout_spacing_y;

void create_ui(struct App *a);
void layout_add_button(struct App *a, enum WidgetId id, int (*state)(),
                       char *valid_data, char *invalid_data, bool full_width);
void layout_add_label(struct App *a, enum WidgetId id, int (*state)(),
                       char *valid_data, char *invalid_data, bool full_width);
void layout_add_slider(struct App *a, enum WidgetId id, int value,
                       int max_value, bool full_width);
void layout_new_row(void);

#endif

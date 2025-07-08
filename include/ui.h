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

#endif

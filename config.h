#ifndef CONFIG_H
#define CONFIG_H

// ID TYPE
// BUTTONS:
// 0 bthn
// 1 volume
// 2 wifi
// 3 bt 
// 4 notify
// 5 prev mus
// 6 play/pause mus
// 7 next mus
//
// SLIDERS:
// 0 bthn
// 1 volume

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#define WIDTH 350
#define HEIGHT 400
#define POS_X 25
#define POS_Y 25
#define FONT "JetBrains Mono-14"
#define BG "#282828"
#define MAX_WIDGETS 32

enum WidgetType{
  WIDGET_LABEL,
  WIDGET_BUTTON,
  WIDGET_SLIDER,
};

struct Widget {
  enum WidgetType type;
  int id;
  int x, y;
  int width, height;
  int knob;
  char *text;
  XftColor normal_color;
  XftColor normal_back;
  XftColor hover_color;
  XftColor hover_back;
  int hovered;
  int slider_value;
  int max_value;
  int drag_offset_x;
};

struct App {
  XftDraw *xftdraw;
  XftColor bg, fg;
  XftFont *font;
  Visual *visual;
  Colormap colormap;
  struct Widget widgets[MAX_WIDGETS];
  pthread_mutex_t lock;
  int widget_count;
  int dragging_id;
  enum WidgetType dragging_type;
};

struct ThreadArgs {
  int id;
  enum WidgetType type;
  struct App *app;
  pthread_t thread;
  int (*getter)(void);
} ;

enum { NormBg, HoverBg, NormFg, HoverFg, NormFgSlider };

extern XRenderColor ColorsSrc[5];
extern const int PADDING;

#endif

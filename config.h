#ifndef CONFIG_H
#define CONFIG_H

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#define POS_X 25
#define POS_Y 25
#define FONT "JetBrains Mono-14"
#define BG "#282828"
#define MAX_WIDGETS 32

enum WidgetType {
  TypeNone,
  WIDGET_LABEL,
  WIDGET_BUTTON,
  WIDGET_SLIDER,
  TypeCount
};

enum WidgetId {
  IdNone,
  ButtonVolumeMute,
  SliderVolume,
  SliderBrightness,
  ButtonPlayerPrev,
  ButtonPlayerPlayPause,
  ButtonPlayerNext,
  ButtonNotify,
  ButtonWiFi,
  ButtonBluetooth,
  LabelBrightness,
  IdCount
};

struct Widget {
  enum WidgetType type;
  enum WidgetId id;
  int x, y;
  int width, height;
  int knob;
  char *text;
  XftColor normal_color;
  XftColor normal_back;
  XftColor hover_color;
  XftColor hover_back;
  bool hovered;
  bool full_width;
  int slider_value;
  int max_value;
  int drag_offset_x;
  char *valid_label;
  char *invalid_label;
};

struct App {
  int height_app;
  int width_app;
  XftDraw *xftdraw;
  XftColor bg, fg;
  XftFont *font;
  Visual *visual;
  Colormap colormap;
  struct Widget widgets[MAX_WIDGETS];
  pthread_mutex_t lock;
  enum WidgetId dragging_id;
  enum WidgetType dragging_type;
  Pixmap buffer;
};

struct ThreadArgs {
  enum WidgetId id;
  enum WidgetType type;
  struct App *app;
  pthread_t thread;
  int (*getter)(void);
};

enum { NormBg, HoverBg, NormFg, HoverFg, NormFgSlider };

extern XRenderColor ColorsSrc[5];
extern const int PADDING;

#endif

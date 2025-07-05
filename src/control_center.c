#include "../include/control_center.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static Display *dpy;
static Window win;

static int running = 1;
const int PADDING = 5;

int layout_x = 20;
int layout_y = 20;
int layout_row_height = 0;
const int layout_spacing_x = 10;
const int layout_spacing_y = 10;

XRenderColor ColorsSrc[5] = {
    [NormFg] = {0xaaaa, 0xaaaa, 0xaaaa, 0xffff},
    [NormFgSlider] = {0x8888, 0x8888, 0x8888, 0xffff},
    [HoverFg] = {0xffff, 0x0000, 0x0000, 0xffff},
    [HoverBg] = {0x2222, 0x2222, 0x2222, 0xffff},
    [NormBg] = {0x3838, 0x3838, 0x3838, 0xffff},
};

XftColor color_alloc(struct App *a, XRenderColor *color_src) {
  XftColor color_dest;
  XftColorAllocValue(dpy, a->visual, a->colormap, color_src, &color_dest);
  return color_dest;
}

void create_ui(struct App *a) {
  layout_add_button(a, ButtonPlayerPrev, "Prev");
  layout_add_button(a, ButtonPlayerPlayPause, "Play");
  layout_add_button(a, ButtonPlayerNext, "Next");
  layout_new_row();

  layout_add_button(a, ButtonWiFi, get_state_wifi() ? "Wi-Fi:On" : "Wi-Fi:Off");
  layout_add_button(a, ButtonBluetooth,
                    get_state_bluetooth() ? "BT:On" : "BT:Off");
  layout_new_row();

  layout_add_button(a, ButtonNotify, get_state_dunst() ? "Notify" : "Silence");
  layout_new_row();

  layout_add_label(a, "Brn");
  layout_add_slider(a, SliderBrightness, get_level_brightness(), 100);
  layout_new_row();

  layout_add_button(a, ButtonVolumeMute,
                    get_state_audio_mute() ? "Mut" : "Vol");
  layout_add_slider(a, SliderVolume, get_level_audio(), 120);

  spawn_state_thread(a, SliderBrightness, WIDGET_SLIDER, get_level_brightness);
  spawn_state_thread(a, SliderVolume, WIDGET_SLIDER, get_level_audio);
}

void draw_widget(struct App *a, struct Widget *w) {
  switch (w->type) {
  case WIDGET_LABEL: {
    XGlyphInfo extents;
    XftTextExtents8(dpy, a->font, (FcChar8 *)w->text, strlen(w->text),
                    &extents);

    w->width = extents.xOff;
    w->height = a->font->ascent + a->font->descent;

    XftDrawStringUtf8(a->xftdraw, &w->normal_color, a->font, w->x, w->y,
                      (FcChar8 *)w->text, strlen(w->text));
    break;
  }

  case WIDGET_BUTTON: {
    XGlyphInfo extents;
    XftTextExtents8(dpy, a->font, (FcChar8 *)w->text, strlen(w->text),
                    &extents);

    w->width = extents.xOff;
    w->height = a->font->ascent + a->font->descent;

    int rect_x = w->x - PADDING;
    int rect_y = w->y - a->font->ascent - PADDING;
    int rect_width = w->width + 2 * PADDING;
    int rect_height = w->height + 2 * PADDING;

    XClearArea(dpy, win, rect_x, rect_y, rect_width, rect_height, False);

    GC gc = XCreateGC(dpy, win, 0, NULL);
    XSetForeground(dpy, gc,
                   w->hovered ? w->hover_back.pixel : w->normal_back.pixel);
    XFillRectangle(dpy, win, gc, rect_x, rect_y, rect_width, rect_height);
    XFreeGC(dpy, gc);

    XftDrawStringUtf8(a->xftdraw,
                      w->hovered ? &w->hover_color : &w->normal_color, a->font,
                      w->x, w->y, (FcChar8 *)w->text, strlen(w->text));
    break;
  }

  case WIDGET_SLIDER: {
    XClearArea(dpy, win, w->x, w->y - w->knob / 2, w->width, w->knob, False);

    GC gc = XCreateGC(dpy, win, 0, NULL);

    XSetForeground(dpy, gc,
                   w->hovered ? w->hover_back.pixel : w->normal_back.pixel);
    XFillRectangle(dpy, win, gc, w->x, w->y - 3, w->width, 6);

    int knob_x = w->x + (w->slider_value * (w->width - w->knob)) / w->max_value;
    int knob_y = w->y - w->knob / 2;

    XSetForeground(dpy, gc,
                   w->hovered ? w->hover_color.pixel : w->normal_color.pixel);
    XFillRectangle(dpy, win, gc, knob_x, knob_y, w->knob, w->knob);

    XFreeGC(dpy, gc);
    break;
  }
  default:
    break;
  }
  XFlush(dpy);
}

void draw_widgets(struct App *a) {
  XClearWindow(dpy, win);
  for (int i = 0; i < a->widget_count; ++i) {
    struct Widget *w = &a->widgets[i];
    draw_widget(a, w);
  }
}

void grab_focus(void) {
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000};
  for (int i = 0; i < 1000; ++i) {
    if (XGrabPointer(dpy, win, True,
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                     GrabModeAsync, GrabModeAsync, None, None,
                     CurrentTime) == GrabSuccess)
      return;
    nanosleep(&ts, NULL);
  }
  die("cannot grab focus");
}

void grab_keyboard(void) {
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000};
  for (int i = 0; i < 1000; i++) {
    if (XGrabKeyboard(dpy, DefaultRootWindow(dpy), True, GrabModeAsync,
                      GrabModeAsync, CurrentTime) == GrabSuccess)
      return;
    nanosleep(&ts, NULL);
  }
  die("cannot grab keyboard");
}

void kill_panel(Display *dpy, struct App *a, XClassHint ch) {
  Window root = DefaultRootWindow(dpy);
  Window *children;
  unsigned int nchildren;

  if (!XQueryTree(dpy, root, &root, &root, &children, &nchildren))
    return;

  for (unsigned int i = 0; i < nchildren; i++) {
    XClassHint class_hint;
    if (XGetClassHint(dpy, children[i], &class_hint)) {
      if (!strcmp(class_hint.res_name, ch.res_name) &&
          !strcmp(class_hint.res_class, ch.res_class)) {
        XKillClient(dpy, children[i]);
        cleanup(a);
        die("already opened");
        break;
      }
      XFree(class_hint.res_name);
      XFree(class_hint.res_class);
    }
  }
  XFree(children);
}

void layout_add_button(struct App *a, enum WidgetId id, const char *text) {
  XGlyphInfo extents;
  XftTextExtents8(dpy, a->font, (FcChar8 *)text, strlen(text), &extents);

  int width = extents.xOff;
  int height = a->font->ascent + a->font->descent;

  a->widgets[a->widget_count++] = (struct Widget){
      .type = WIDGET_BUTTON,
      .id = id,
      .x = layout_x,
      .y = layout_y + height,
      .text = strdup(text),
      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
      .normal_back = color_alloc(a, &ColorsSrc[NormBg]),
      .hover_color = color_alloc(a, &ColorsSrc[HoverFg]),
      .hover_back = color_alloc(a, &ColorsSrc[HoverBg]),
      .hovered = 0,
  };

  layout_x += width + 2 * PADDING + layout_spacing_x;
  if (height + 2 * PADDING > layout_row_height)
    layout_row_height = height + 2 * PADDING;
}

void layout_add_label(struct App *a, const char *text) {
  XGlyphInfo extents;
  XftTextExtents8(dpy, a->font, (FcChar8 *)text, strlen(text), &extents);

  int width = extents.xOff;
  int height = a->font->ascent + a->font->descent;

  a->widgets[a->widget_count++] = (struct Widget){
      .type = WIDGET_LABEL,
      .id = IdNone,
      .x = layout_x,
      .y = layout_y + height,
      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
      .text = strdup(text),
  };

  layout_x += width + 2 * PADDING + layout_spacing_x;
  if (height + 2 * PADDING > layout_row_height)
    layout_row_height = height + 2 * PADDING;
}

void layout_add_slider(struct App *a, enum WidgetId id, int value, int max_value) {
  int height = 14;
  int width = 200;

  a->widgets[a->widget_count++] =
      (struct Widget){.type = WIDGET_SLIDER,
                      .id = id,
                      .x = layout_x,
                      .y = layout_y + a->font->ascent,
                      .width = width,
                      .knob = height,
                      .height = height,
                      .normal_color = color_alloc(a, &ColorsSrc[NormFg]),
                      .normal_back = color_alloc(a, &ColorsSrc[NormBg]),
                      .hover_color = color_alloc(a, &ColorsSrc[HoverFg]),
                      .hover_back = color_alloc(a, &ColorsSrc[HoverBg]),
                      .slider_value = value,
                      .max_value = max_value};

  layout_x += width + layout_spacing_x;
  if (height > layout_row_height)
    layout_row_height = height;
}

void layout_new_row(void) {
  layout_x = 20;
  layout_y += layout_row_height + layout_spacing_y;
  layout_row_height = 0;
}

void run(struct App *a) {
  XEvent ev;
  while (running) {
    draw_widgets(a);
    update_hover_state(a, ev.xmotion.x, ev.xmotion.y);
    XNextEvent(dpy, &ev);

    switch (ev.type) {
    case EnterNotify:
    case LeaveNotify:
    case MotionNotify:
      if (handle_motion_notify(a, &ev))
        running = 0;
      break;

    case KeyPress:
      running = 0;
      break;

    case ButtonPress:
      if (handle_button_press(a, &ev))
        running = 0;
      break;

    case ButtonRelease:
      a->dragging_id = IdNone;
      a->dragging_type = TypeNone;
      break;
    }
  }
}

void setup(struct App *a) {
  a->dragging_id = IdNone;
  a->dragging_type = TypeNone;
  a->widget_count = 0;

  pthread_mutex_init(&a->lock, NULL);
  XClassHint ch = {"center", "center"};

  if (!(dpy = XOpenDisplay(NULL))) {
    die("Can't open display\n");
  }

  kill_panel(dpy, a, ch);

  int screen = DefaultScreen(dpy);
  a->visual = DefaultVisual(dpy, screen);
  a->colormap = DefaultColormap(dpy, screen);

  if (!XftColorAllocName(dpy, a->visual, a->colormap, BG, &a->bg)) {
    cleanup(a);
    die("Can't allocate colors\n");
  }

  XSetWindowAttributes attrs = {
      .override_redirect = True,
      .background_pixel = a->bg.pixel,
      .event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
                    ButtonReleaseMask | PointerMotionMask | EnterWindowMask |
                    LeaveWindowMask};

  win = XCreateWindow(dpy, RootWindow(dpy, screen), POS_X, POS_Y, WIDTH, HEIGHT,
                      0, CopyFromParent, InputOutput, CopyFromParent,
                      CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);

  XSetClassHint(dpy, win, &ch);
  if (!(a->font = XftFontOpenName(dpy, screen, FONT))) {
    cleanup(a);
    die("Can't load font\n");
  }

  a->xftdraw = XftDrawCreate(dpy, win, a->visual, a->colormap);

  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  Atom net_wm_state_above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty(dpy, win, net_wm_state, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&net_wm_state_above, 1);
}

void spawn_state_thread(struct App *a, int id, enum WidgetType type,
                        int (*getter)(void)) {
  pthread_t thread = 0;
  struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
  *args = (struct ThreadArgs){
      .id = id, .type = type, .app = a, .thread = thread, .getter = getter};

  if (pthread_create(&thread, NULL, state_updater_thread, args) != 0) {
    fprintf(stderr, "Failed to create thread\ntype: %u id: %d\n", type, id);
    free(args);
  }
}

void *state_updater_thread(void *arg) {
  struct ThreadArgs *args = (struct ThreadArgs *)arg;
  struct Widget *w = NULL;

  for (int i = 0; i < args->app->widget_count; ++i) {
    if (args->app->widgets[i].id == args->id &&
        args->app->widgets[i].type == args->type) {
      w = &args->app->widgets[i];
      break;
    }
  }

  if (!w || !args->getter)
    return NULL;

  while (running) {
    switch (w->type) {
    case WIDGET_SLIDER: {
      int new_value = args->getter();
      if (new_value != w->slider_value) {
        w->slider_value = new_value;
        pthread_mutex_lock(&args->app->lock);
        w->slider_value = new_value;
        draw_widget(args->app, w);
        pthread_mutex_unlock(&args->app->lock);
      }
      break;
    }
    case WIDGET_BUTTON: {
      __attribute__((unused)) int state = args->getter();
      break;
    }
    default:
      break;
    }
    usleep(450000);
  }

  free(args);
  return NULL;
}

void cleanup(struct App *a) {
  if (!a)
    return;

  for (int i = 0; i < a->widget_count; i++) {
    free(a->widgets[i].text);
  }

  if (a->xftdraw)
    XftDrawDestroy(a->xftdraw);
  if (a->font)
    XftFontClose(dpy, a->font);
  if (win) {
    XUnmapWindow(dpy, win);
    XDestroyWindow(dpy, win);
  }
  if (dpy)
    XCloseDisplay(dpy);
}

int main(void) {
  struct App *a = calloc(1, sizeof(struct App));
  setup(a);
  grab_keyboard();

  XMapRaised(dpy, win);
  create_ui(a);
  XFlush(dpy);

  grab_focus();
  run(a);

  XUngrabPointer(dpy, CurrentTime);
  XUngrabKeyboard(dpy, CurrentTime);

  cleanup(a);
  return 0;
}

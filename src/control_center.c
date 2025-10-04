#include "../include/control_center.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

Display *dpy;
Window win;
struct Widget widgets[IdCount];

static int running = 1;

XRenderColor ColorsSrc[5] = {
    [NormFg] = {0xaaaa, 0xaaaa, 0xaaaa, 0xffff},
    [NormFgSlider] = {0x8888, 0x8888, 0x8888, 0xffff},
    [HoverFg] = {0xffff, 0x0000, 0x0000, 0xffff},
    [HoverBg] = {0x2222, 0x2222, 0x2222, 0xffff},
    [NormBg] = {0x3838, 0x3838, 0x3838, 0xffff},
};

void draw_widgets(struct App *a) {
  GC gc = XCreateGC(dpy, win, 0, NULL);
  XSetForeground(dpy, gc, a->bg.pixel);
  XFillRectangle(dpy, a->buffer, gc, 0, 0, a->width_app, a->height_app);

  for (int i = 0; i < IdCount; ++i) {
    struct Widget *w = &widgets[i];
    pthread_mutex_lock(&a->lock);
    widget_to_buffer(a, w);
    pthread_mutex_unlock(&a->lock);
  }

  XCopyArea(dpy, a->buffer, win, gc, 0, 0, a->width_app, a->height_app, 0, 0);
  XFreeGC(dpy, gc);
  XFlush(dpy);
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

// void kill_panel(Display *dpy, struct App *a, XClassHint ch) {
//   Window root = DefaultRootWindow(dpy);
//   Window *children;
//   unsigned int nchildren;
//
//   if (!XQueryTree(dpy, root, &root, &root, &children, &nchildren))
//     return;
//
//   for (unsigned int i = 0; i < nchildren; i++) {
//     XClassHint class_hint;
//     if (XGetClassHint(dpy, children[i], &class_hint)) {
//       if (!strcmp(class_hint.res_name, ch.res_name) &&
//           !strcmp(class_hint.res_class, ch.res_class)) {
//         XKillClient(dpy, children[i]);
//         cleanup(a);
//         die("already opened");
//         break;
//       }
//       XFree(class_hint.res_name);
//       XFree(class_hint.res_class);
//     }
//   }
//   XFree(children);
// }

void redraw_widget(struct App *a, struct Widget *w) {
  if (!a || !w)
    return;

  int update_x, update_y, update_width, update_height;

  switch (w->type) {
  case WIDGET_SLIDER:
    update_x = w->x - 2;
    update_y = w->y - w->knob / 2 - 2;
    update_width = w->width + 4;
    update_height = w->knob + 4;
    break;

  case WIDGET_BUTTON:
  case WIDGET_LABEL:
    update_x = w->x;
    update_y = w->y - a->font->ascent;
    update_width = w->width;
    update_height = w->height;
    break;

  default:
    return;
  }

  pthread_mutex_lock(&a->lock);
  widget_to_buffer(a, w);
  pthread_mutex_unlock(&a->lock);

  GC gc = XCreateGC(dpy, win, 0, NULL);
  XCopyArea(dpy, a->buffer, win, gc, update_x, update_y, update_width,
            update_height, update_x, update_y);
  XFreeGC(dpy, gc);
  XFlush(dpy);
}

void run(struct App *a) {
  XEvent ev;
  while (running) {
    XNextEvent(dpy, &ev);

    switch (ev.type) {
    case EnterNotify:
    case LeaveNotify:
    case MotionNotify:
      update_hover_state(a, ev.xmotion.x, ev.xmotion.y);
      if (handle_motion_notify(a, &ev))
        running = 0;
      draw_widgets(a);
      break;

    case KeyPress:
      running = 0;
      break;

    case ButtonPress:
      if (handle_button_press(a, &ev))
        running = 0;
      draw_widgets(a);
      break;

    case ButtonRelease:
      a->dragging_id = IdNone;
      a->dragging_type = TypeNone;
      update_hover_state(a, ev.xbutton.x, ev.xbutton.y);
      draw_widgets(a);
      break;

    case Expose:
      break;

    default:
      break;
    }
  }
}

void setup(struct App *a) {
  a->width_app = 320;
  a->height_app = 400;

  pthread_mutex_init(&a->lock, NULL);
  XClassHint ch = {"center", "center"};

  if (!XInitThreads()) {
    die("Xlib thread initialization failed.\n");
  }

  if (!(dpy = XOpenDisplay(NULL))) {
    die("Can't open display\n");
  }

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

  win = XCreateWindow(dpy, RootWindow(dpy, screen), POS_X, POS_Y, a->width_app,
                      a->height_app, 0, CopyFromParent, InputOutput,
                      CopyFromParent,
                      CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);

  XSetClassHint(dpy, win, &ch);
  if (!(a->font = XftFontOpenName(dpy, screen, FONT))) {
    cleanup(a);
    die("Can't load font\n");
  }

  a->buffer = XCreatePixmap(dpy, win, a->width_app, a->height_app,
                            DefaultDepth(dpy, DefaultScreen(dpy)));

  a->xftdraw = XftDrawCreate(dpy, a->buffer, a->visual, a->colormap);

  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  Atom net_wm_state_above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty(dpy, win, net_wm_state, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&net_wm_state_above, 1);

  XMapRaised(dpy, win);

  create_ui(a);
  draw_widgets(a);

  XFlush(dpy);

  grab_keyboard();
  grab_focus();
}

void spawn_state_thread(struct App *a, enum WidgetId id, enum WidgetType type,
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
  struct Widget *w = &widgets[args->id];

  if (!w || !args->getter) {
    free(args);
    return NULL;
  }

  while (running) {
    if (w->id != args->app->dragging_id) {
      pthread_mutex_lock(&args->app->lock);

      switch (w->type) {
      case WIDGET_SLIDER:
        w->slider_value = args->getter();
        break;

      case WIDGET_BUTTON:
        set_value(w, args->getter);
        break;

      default:
        break;
      }

      pthread_mutex_unlock(&args->app->lock);
      redraw_widget(args->app, w);
      usleep(300000);
    }
  }

  free(args);

  return NULL;
}

void widget_to_buffer(struct App *a, struct Widget *w) {
  GC gc = XCreateGC(dpy, a->buffer, 0, NULL);
  switch (w->type) {
  case WIDGET_LABEL: {
    XGlyphInfo extents;
    XftTextExtents8(dpy, a->font, (FcChar8 *)w->text, strlen(w->text),
                    &extents);

    int text_x = w->x + (w->width - extents.xOff) / 2;
    int text_y = w->y + (w->height - a->font->ascent - a->font->descent) / 2;

    XftDrawStringUtf8(a->xftdraw, &w->normal_color, a->font, text_x, text_y,
                      (FcChar8 *)w->text, strlen(w->text));
    break;
  }

  case WIDGET_BUTTON: {
    XGlyphInfo extents;
    XftTextExtents8(dpy, a->font, (FcChar8 *)w->text, strlen(w->text),
                    &extents);

    int rect_x = w->x;
    int rect_y = w->y - a->font->ascent;
    int rect_width = w->width;
    int rect_height = w->height;

    XSetForeground(dpy, gc,
                   w->hovered ? w->hover_back.pixel : w->normal_back.pixel);
    XFillRectangle(dpy, a->buffer, gc, rect_x, rect_y, rect_width, rect_height);

    int text_x = w->x + (w->width - extents.xOff) / 2;
    int text_y = w->y + (w->height - a->font->ascent - a->font->descent) / 2;

    XftDrawStringUtf8(a->xftdraw,
                      w->hovered ? &w->hover_color : &w->normal_color, a->font,
                      text_x, text_y, (FcChar8 *)w->text, strlen(w->text));
    break;
  }

  case WIDGET_SLIDER: {
    int full_x = w->x - 2;
    int full_y = w->y - w->knob / 2 - 2;
    int full_width = w->width + 4;
    int full_height = w->knob + 4;

    XSetForeground(dpy, gc, a->bg.pixel);
    XFillRectangle(dpy, a->buffer, gc, full_x, full_y, full_width, full_height);

    XSetForeground(dpy, gc,
                   w->hovered ? w->hover_back.pixel : w->normal_back.pixel);
    XFillRectangle(dpy, a->buffer, gc, w->x, w->y - 4, w->width, 8);

    int knob_x = w->x + (w->slider_value * (w->width - w->knob)) / w->max_value;
    int knob_y = w->y - w->knob / 2;

    XSetForeground(dpy, gc,
                   w->hovered ? w->hover_color.pixel : w->normal_color.pixel);
    XFillRectangle(dpy, a->buffer, gc, knob_x, knob_y, w->knob, w->knob);
    break;
  }
  default:
    break;
  }
  XFreeGC(dpy, gc);
}

void cleanup(struct App *a) {
  if (!a)
    return;

  if (dpy) {
    XUngrabPointer(dpy, CurrentTime);
    XUngrabKeyboard(dpy, CurrentTime);
  }

  for (int i = 0; i < IdCount; i++) {
    free(widgets[i].text);
    free(widgets[i].valid_label);
    free(widgets[i].invalid_label);
  }

  if (a->buffer)
    XFreePixmap(dpy, a->buffer);

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
  if (a)
    free(a);
}

int main(void) {
  struct App *a = calloc(1, sizeof(struct App));
  setup(a);
  run(a);

  cleanup(a);
  return 0;
}

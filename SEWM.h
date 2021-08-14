#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

// this is the core of the WM

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

struct{
  Display *dpy;
  XWindowAttributes attrs;
  XButtonEvent bevent;
  XEvent event;
  KeySym keysym;
  Window focused; // current focused window
  XftFont **fonts;

  int dx, dy; // mouse delta value
  int gapSize, borderSize, windowOffset;
  int wc, fc, tc, mc, ad;
  bool on;
  bool tiling;
  bool border;
} wm;

typedef struct{
  char *s;
  int num, x, y, w, h;
  Window win;
} arg;

struct{
  int width, height, screen;
} monitorInfo;

typedef struct{
  Window win;
  int isFloating, hasBorder, isFullScreen, isFocused;
} Windows;

struct{
  Windows *w;
  int wc, fc, tc, mc;
} d[9];

struct{
  Window win;
  GC gc;

  int x, y, w, h, borderSize;
} bar;

struct{
  Window win;
  GC gc;

  int x, y, w, h;
  arg args;
  void (*func)();
} button[9];
XWindowAttributes attrs;

const char *buttonNames[] = {"", "", "", "", "", "", "", "", ""};
const char *fontNames[] = {"monospace:size=14", "fontawesome:size=14"};

const char *colors[]     = {"#1a2026", "#0077cc", "#888888"};
const char *fontColors[] = {"#FFFFFF", "#0077cc", "#888888"};
unsigned long *xColors;
XftColor *xFontColors;

int launchWM();
void handleEvents();

void fetchMonitorInfo();
void tileWindows();
void focusWindow(Window w);
void changeMode(arg args);
void manageWindow(Window w);
void addToWins(Window w);
void remFromWins(Window w);
void removeFromArray(int pos, int ind);
void drawBar();
void createButtons(int w, int h);
void updateButtons();
int decode_code_point(char **s);

void loadColors();
void loadFonts();
void drawText(char *string, XftColor xcolor, Window win, int x, int y, int clear);

int getWindowIndex(Window w);
int howManyFloating();

#include "binds.h"
void handleEvents(){
  XNextEvent(wm.dpy, &wm.event);
  XSync(wm.dpy, false);
  switch(wm.event.type){
    case Expose:
      XClearWindow(wm.dpy, bar.win);
      updateButtons();
      break;
    case KeyPress:
      printf("KeyPress event called.\n");
      wm.keysym = XKeycodeToKeysym(wm.dpy, (KeyCode)wm.event.xkey.keycode, 0);
      for(int i = 0; i < SIZEOF(binds); i++)
        if(wm.keysym == binds[i].key && (binds[i].mod & ~(LockMask) & (ShiftMask|ControlMask)) == (wm.event.xkey.state & ~(LockMask) & (ShiftMask|ControlMask)))
          (*binds[i].func)(binds[i].args);
      break;
    case ButtonPress:
      printf("ButtonPress event called.\n");
      for(int i = 0; i < SIZEOF(button); i++){
        if(button[i].win == wm.event.xbutton.window){
          (*button[i].func)(button[i].args);
          break;
        }
      }

      if(wm.event.xkey.subwindow && wm.event.xkey.subwindow != bar.win){
        XGetWindowAttributes(wm.dpy, wm.event.xkey.subwindow, &wm.attrs);
        wm.bevent = wm.event.xbutton;
        XRaiseWindow(wm.dpy, wm.event.xkey.subwindow);
      }
      break;
    case MotionNotify:
      printf("MotionNotify event called.\n");
      if(wm.bevent.subwindow){
        wm.dx = wm.event.xbutton.x_root - wm.bevent.x_root;
        wm.dy = wm.event.xbutton.y_root - wm.bevent.y_root;
        XMoveResizeWindow(wm.dpy, wm.bevent.subwindow, wm.attrs.x + (wm.bevent.button==1 ? wm.dx : 0), wm.attrs.y + (wm.bevent.button==1 ? wm.dy : 0), MAX(100, wm.attrs.width + (wm.bevent.button==3 ? wm.dx : 0)), MAX(100, wm.attrs.height + (wm.bevent.button==3 ? wm.dy : 0)));
        if(wm.tiling)
          changeMode((arg){.win = wm.event.xkey.subwindow, .num = 1});
      }
      break;
    case ButtonRelease:
      printf("ButtonRelease event called.\n");
      wm.bevent.subwindow = None;
      break;
    case EnterNotify:
      printf("EnterNotify event called.\n");
      focusWindow(wm.event.xcrossing.window);
      break;
    case MapNotify:
      printf("mapNotify event called.\n");
      if(wm.event.xmap.window == bar.win)
        break;

      XGetWindowAttributes(wm.dpy, wm.event.xmap.window, &attrs);
      if(attrs.class != InputOnly && attrs.map_state == 2 && attrs.override_redirect == false){
        addToWins(wm.event.xmap.window);
        manageWindow(wm.event.xmap.window);
        tileWindows();
        if(d[wm.ad].wc)
          focusWindow(d[wm.ad].w[d[wm.ad].wc-1].win);
      }
      break;
    case UnmapNotify:
      printf("UnmapNotify event called.\n");
      remFromWins(wm.event.xunmap.window);
      tileWindows();
      if(d[wm.ad].wc)
        focusWindow(d[wm.ad].w[d[wm.ad].wc-1].win);
      break;
    default:
      printf("Unknown/Ignored event called, (%d)\n", wm.event.type);
      break;
  }
}

void tileWindows(){
  if(!wm.tiling)
    return;
  d[wm.ad].fc = howManyFloating();
  d[wm.ad].tc = d[wm.ad].wc - d[wm.ad].fc;
  int c=d[wm.ad].tc, h=0, g=wm.gapSize+wm.borderSize, ii=0, amc=d[wm.ad].mc;
  if(d[wm.ad].mc > d[wm.ad].wc-1 && d[wm.ad].wc-1 > 0)
    amc = d[wm.ad].mc - (d[wm.ad].mc - d[wm.ad].wc);
  for(int i = 0; i < d[wm.ad].wc; i++){
    if(!d[wm.ad].w[i].isFloating && !d[wm.ad].w[i].isFullScreen){
      if(ii == c || !c)
        break;

      if(c == 1){
        XMoveResizeWindow(wm.dpy, d[wm.ad].w[i].win, wm.gapSize, wm.gapSize+bar.h, monitorInfo.width-g*2, (monitorInfo.height-g*2)-bar.h);
        break;
      }

      if(ii+amc < c){
        h = (monitorInfo.height-(bar.h+g)) / (c - amc);
        XMoveResizeWindow(wm.dpy, d[wm.ad].w[i].win, 
            ((amc?monitorInfo.width/2:0)+wm.gapSize)+wm.windowOffset*(amc?1:0), 
            (h*((d[wm.ad].wc-(amc+1))-(ii+++d[wm.ad].fc)))+wm.gapSize+bar.h, 
            ((monitorInfo.width/(amc?2:1))-g*2)-wm.windowOffset*(amc?1:0), 
            h-(g+(wm.borderSize)*(ii?1:0)));
      }else{
        h = amc==c?0:1;
        XMoveResizeWindow(wm.dpy, d[wm.ad].w[i].win, 
            wm.gapSize, 
            ((c-++ii)*(monitorInfo.height-(bar.h+wm.gapSize))/amc)+wm.gapSize+bar.h, 
            ((monitorInfo.width/(amc==c?1:2))-(g+wm.borderSize*h)*(amc==c?2:1))+wm.windowOffset*h, 
            ((monitorInfo.height-(bar.h+g))/amc)-g);
      }
    }
  }
}

void manageWindow(Window w){
  if(wm.border)
    XSetWindowBorderWidth(wm.dpy, w, wm.borderSize);

  XSelectInput(wm.dpy, w, EnterWindowMask);
}

int howManyFloating(){
  int result=0;
  for(int i = 0; i < d[wm.ad].wc; i++){
    if(d[wm.ad].w[i].isFloating)
      result++;
  }
  return result;
}

void focusWindow(Window w){
  int i = getWindowIndex(w), ii = getWindowIndex(wm.focused);
  if(i==-1)
    return;

  // unfocuse prev one
  if(ii!=-1){
    d[wm.ad].w[ii].isFocused = 0;
    XSetWindowBorder(wm.dpy, d[wm.ad].w[ii].win, xColors[2]);
  }

  wm.focused = w;
  d[wm.ad].w[i].isFocused = 1;
  XSetWindowBorder(wm.dpy, w, xColors[1]);
  XSetInputFocus(wm.dpy, w, RevertToPointerRoot, CurrentTime);
}

void changeMode(arg args){
  int i = getWindowIndex(args.win);

  if(args.x){
    int ii = getWindowIndex(wm.focused);
    if(ii==-1){return;}
    d[wm.ad].w[ii].isFloating = !d[wm.ad].w[ii].isFloating;
    tileWindows();
    XRaiseWindow(wm.dpy, wm.focused);
    return;
  }
  if(i!=-1){
    d[wm.ad].w[i].isFloating = args.num;
    tileWindows();
  }
}

void loadColors(){
  Visual *visual = DefaultVisual(wm.dpy, DefaultScreen(wm.dpy));
  Colormap map   = DefaultColormap(wm.dpy, DefaultScreen(wm.dpy));
  XColor color;
  XftColor xcolor;

  // loading default colors
  xColors = (unsigned long *)calloc(1, sizeof(*xColors));
  for(int i = 0; i < SIZEOF(colors); i++){
    xColors = (unsigned long *)realloc(xColors, sizeof(*xColors) * (i+1));

    XAllocNamedColor(wm.dpy, map, colors[i], &color, &color);
    xColors[i] = color.pixel;
  }

  // loading font colors
  xFontColors = (XftColor *)calloc(1, sizeof(*xFontColors));
  for(int i = 0; i < SIZEOF(fontColors); i++){
    xFontColors = (XftColor *)realloc(xFontColors, sizeof(*xFontColors) * (i+1));

    XftColorAllocName(wm.dpy, visual, map, fontColors[i], &xcolor);
    xFontColors[i] = xcolor;
  }
}

void loadFonts(){
  wm.fonts = (XftFont **)calloc(1, sizeof(*wm.fonts));
  for(int i = 0; i < SIZEOF(fontNames); i++){
    wm.fonts = (XftFont **)realloc(wm.fonts, sizeof(*wm.fonts) * (i+1));
    wm.fonts[i] = XftFontOpenName(wm.dpy, DefaultScreen(wm.dpy), fontNames[i]);
  }
}

// utf8 decoder: https://gist.github.com/tylerneylon/9773800
int decode_code_point(char **s) {
  int k = **s ? __builtin_clz(~(**s << 24)) : 0;  // Count # of leading 1 bits.
  int mask = (1 << (8 - k)) - 1;                  // All 1's with k leading 0's.
  int value = **s & mask;
  for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {  // Note that k = #total bytes, or 0.
    value <<= 6;
    value += (**s & 0x3F);
  }
  return value;
}

void drawText(char *string, XftColor xcolor, Window win, int x, int y, int clear){
  Visual *visual = DefaultVisual(wm.dpy, DefaultScreen(wm.dpy));
  Colormap map   = DefaultColormap(wm.dpy, DefaultScreen(wm.dpy));
  XftDraw *draw  = XftDrawCreate(wm.dpy, win, visual, map);

  if(clear)
    XClearWindow(wm.dpy, win);
 
  for(int i = 0; i < SIZEOF(fontNames); i++){
    char *s = string;
    if(XftCharExists(wm.dpy, wm.fonts[i], (FcChar32)decode_code_point(&s))){
      XftDrawStringUtf8(draw, &xcolor, wm.fonts[i], x, y, (const FcChar8 *)string, strlen(string));
      break;
    }
  }
  XftDrawDestroy(draw);
}

void updateButtons(){
  for(int i = 0; i < SIZEOF(button); i++){
    if(i == wm.ad){
      drawText((char *)buttonNames[i], xFontColors[1], button[i].win, 6, 14, 1);
    }else if(d[i].wc){
      drawText((char *)buttonNames[i], xFontColors[2], button[i].win, 6, 14, 1);
    }else{
      drawText((char *)buttonNames[i], xFontColors[0], button[i].win, 6, 14, 1);
    }
  }
}

int getWindowIndex(Window w){
  for(int i = 0; i < d[wm.ad].wc; i++){
    if(d[wm.ad].w[i].win == w)
      return i;
  }
  return -1;
}

void addToWins(Window w){
  d[wm.ad].w = (Windows *)realloc(d[wm.ad].w, sizeof(*d[wm.ad].w) * (d[wm.ad].wc+1));

  d[wm.ad].w[d[wm.ad].wc].win = w;
  d[wm.ad].w[d[wm.ad].wc].hasBorder = 1;
  d[wm.ad].w[d[wm.ad].wc].isFloating = 0;
  d[wm.ad].w[d[wm.ad].wc].isFullScreen = 0;

  d[wm.ad].wc++;
  focusWindow(w);
}

void removeFromArray(int pos, int ind){
  for(int i = pos; i < d[ind].wc; i++){
    d[ind].w[i] = d[ind].w[i+1];
  }
  d[ind].wc--;
}

void remFromWins(Window w){
  if(wm.focused == w)
    wm.focused = None;

  for(int i = 0; i < SIZEOF(d); i++)
    for(int j = 0; j < d[i].wc; j++)
      if(d[i].w[j].win == w)
        removeFromArray(j, i);
}

void drawBar(){
  bar.win = XCreateSimpleWindow(wm.dpy, DefaultRootWindow(wm.dpy),
      bar.x, bar.y, bar.w-(bar.borderSize*2), bar.h, bar.borderSize,
      xColors[1], xColors[0]
      );

  XMapWindow(wm.dpy, bar.win);
  XRaiseWindow(wm.dpy, bar.win);
  XSetClassHint(wm.dpy, bar.win, &(XClassHint){"sewm", "sewm"});
  //XSelectInput(wm.dpy, bar.win, ExposureMask);

  XChangeProperty(wm.dpy, bar.win, XInternAtom(wm.dpy, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *)&bar.win, 1);
  XChangeProperty(wm.dpy, bar.win, XInternAtom(wm.dpy, "_NET_WM_NAME", False), XInternAtom(wm.dpy, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char *)"sewm", 4);
  XChangeProperty(wm.dpy, DefaultRootWindow(wm.dpy), XInternAtom(wm.dpy, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *)&bar.win, 1);
}

void createButtons(int w, int h){
  for(int i = 0; i < SIZEOF(button); i++){
    button[i].x = i*w;
    button[i].y = 0;
    button[i].w = w;
    button[i].h = h;
    button[i].args = (arg){.num = i};
    button[i].func = changeDesktop;

    button[i].win = XCreateSimpleWindow(wm.dpy, bar.win,
        button[i].x, button[i].y, button[i].w, button[i].h, 0,
        xColors[0], xColors[0]
        );

    XMapWindow(wm.dpy, button[i].win);
    XRaiseWindow(wm.dpy, button[i].win);
    XSelectInput(wm.dpy, button[i].win, ButtonPressMask|ExposureMask);
  }
  updateButtons();
}

void fetchMonitorInfo(){
  monitorInfo.screen = DefaultScreen(wm.dpy);
  monitorInfo.width  = DisplayWidth (wm.dpy, monitorInfo.screen);
  monitorInfo.height = DisplayHeight(wm.dpy, monitorInfo.screen);
}

int launchWM(){
  Colormap map;
  XColor color;

  printf("Starting SEWM...\n");
  // opening display
  wm.dpy = XOpenDisplay(NULL);
  if(!wm.dpy){
    printf("ERROR: cannot open display! EXITING...\n");
    exit(EXIT_FAILURE);
  }
  fetchMonitorInfo();

  XUngrabKey(wm.dpy, AnyKey, AnyModifier, DefaultRootWindow(wm.dpy));
  unsigned int masks[] = {0, LockMask};
  for(int i = 0; i < sizeof(binds) / sizeof(binds[0]); i++)
    for(int v = 0; v < 2; v++)
      XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, binds[i].key), binds[i].mod|masks[v], DefaultRootWindow(wm.dpy), true, GrabModeAsync, GrabModeAsync);

  // move and resize
  XGrabButton(wm.dpy, 1, Mod1Mask, DefaultRootWindow(wm.dpy), true, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(wm.dpy, 3, Mod1Mask, DefaultRootWindow(wm.dpy), true, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

  XSelectInput(wm.dpy, DefaultRootWindow(wm.dpy), SubstructureNotifyMask);

  wm.mc = 1; // master count
  for(int i = 0; i < SIZEOF(d); i++)
    d[i].mc = wm.mc;
  d[wm.ad].w = (Windows *)calloc(0, sizeof(*d[wm.ad].w));

  loadColors();
  loadFonts();

  bar.x = 0;
  bar.y = 0;
  bar.w = monitorInfo.width;
  bar.h = 18;
  bar.borderSize = 0;

  drawBar();
  createButtons(26, bar.h);

  wm.borderSize = (wm.border ? wm.borderSize : 0);
  wm.bevent.subwindow = None;
  wm.on = true;

  while(wm.on)
    handleEvents();

  return 1;
}

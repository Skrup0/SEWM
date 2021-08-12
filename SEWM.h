#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

// this is the core of the WM

#define MAX(a,b) ((a) > (b) ? (a) : (b))

struct{
  Display *dpy;
  XWindowAttributes attrs;
  XButtonEvent bevent;
  XEvent event;
  KeySym keysym;
  Window focused; // current focused window

  int dx, dy; // mouse delta value
  int gapSize, borderSize, windowOffset;
  int wc, fc, tc, mc, ad;
  char *focusedColor, *notFocusedColor;
  unsigned long fcol, nfcol;
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
  unsigned long borderColor;
} Windows;

struct{
  Windows *w;
  int wc, fc, tc, mc;
} d[9];

struct{
  Window win;
  GC gc;

  int x, y, w, h, borderSize;
  char *color, *borderColor;
  unsigned long col, borderCol;
} bar;

struct{
  Window win;
  GC gc;

  int x, y, w, h;
  char *color, *activeColor, *fontColor;
  unsigned long col, activeCol, fontCol;
} button[9];

XWindowAttributes attrs;
const char *buttonNames[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};

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

int getWindowIndex(Window w);
int howManyFloating();

#include "binds.h"
void handleEvents(){
  XNextEvent(wm.dpy, &wm.event);
  XSync(wm.dpy, false);
  switch(wm.event.type){
    case Expose:
      updateButtons();
      break;
    case KeyPress:
      printf("KeyPress event called.\n");
      wm.keysym = XKeycodeToKeysym(wm.dpy, (KeyCode)wm.event.xkey.keycode, 0);
      for(int i = 0; i < sizeof(binds) / sizeof(binds[0]); i++)
        if(wm.keysym == binds[i].key && (binds[i].mod & ~(LockMask) & (ShiftMask|ControlMask)) == (wm.event.xkey.state & ~(LockMask) & (ShiftMask|ControlMask)))
          (*binds[i].func)(binds[i].args);
      break;
    case ButtonPress:
      printf("ButtonPress event called.\n");
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
  int c=d[wm.ad].tc, h=0, g=wm.gapSize+wm.borderSize, ii=0;
  for(int i = 0; i < d[wm.ad].wc; i++){
    if(!d[wm.ad].w[i].isFloating && !d[wm.ad].w[i].isFullScreen){
      if(ii == c || !c)
        break;

      if(c == 1){
        XMoveResizeWindow(wm.dpy, d[wm.ad].w[i].win, wm.gapSize, wm.gapSize+bar.h, monitorInfo.width-g*2, (monitorInfo.height-g*2)-bar.h);
        break;
      }

      if(ii+d[wm.ad].mc < c){
        h = monitorInfo.height / (c - d[wm.ad].mc);
        XMoveResizeWindow(wm.dpy, d[wm.ad].w[i].win, 
            ((d[wm.ad].mc ? monitorInfo.width/2 : 0)+wm.gapSize)+wm.windowOffset*(d[wm.ad].mc ? 1 : 0), 
            (h*((d[wm.ad].wc-(d[wm.ad].mc+1))-(ii+++d[wm.ad].fc)))+wm.gapSize+bar.h, 
            ((monitorInfo.width/(d[wm.ad].mc ? 2 : 1))-g*2)-wm.windowOffset*(d[wm.ad].mc ? 1 : 0), 
            (h-g*2)-bar.h);
      }else{
        XMoveResizeWindow(wm.dpy, d[wm.ad].w[i].win, 
            wm.gapSize, 
            ((c-++ii)*monitorInfo.height/d[wm.ad].mc)+wm.gapSize+bar.h, 
            ((monitorInfo.width/(d[wm.ad].mc==c ? 1 : 2))-g*2)+wm.windowOffset*(d[wm.ad].mc==c ? 0 : 1), 
            ((monitorInfo.height/d[wm.ad].mc)-g*2)-bar.h);
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
    d[wm.ad].w[ii].borderColor = wm.nfcol;
    XSetWindowBorder(wm.dpy, d[wm.ad].w[ii].win, d[wm.ad].w[ii].borderColor);
  }

  wm.focused = w;
  d[wm.ad].w[i].isFocused = 1;
  d[wm.ad].w[i].borderColor = wm.fcol;
  XSetWindowBorder(wm.dpy, w, d[wm.ad].w[i].borderColor);
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

void updateButtons(){
  for(int i = 0; i < 9; i++){
    XSetForeground(wm.dpy, button[i].gc, i == wm.ad ? button[i].activeCol : button[i].fontCol);
    XDrawString(wm.dpy, button[i].win, button[i].gc, 6, button[i].y+13, buttonNames[i], 1);
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

  for(int i = 0; i < 9; i++)
    for(int j = 0; j < d[i].wc; j++)
      if(d[i].w[j].win == w)
        removeFromArray(j, i);
}

void drawBar(){
  XColor color;
  Colormap map = DefaultColormap(wm.dpy, DefaultScreen(wm.dpy));

  XAllocNamedColor(wm.dpy, map, bar.color, &color, &color);
  bar.col = color.pixel;
  XAllocNamedColor(wm.dpy, map, bar.borderColor, &color, &color);
  bar.borderCol = color.pixel;

  bar.win = XCreateSimpleWindow(wm.dpy, DefaultRootWindow(wm.dpy),
      bar.x, bar.y, bar.w-(bar.borderSize*2), bar.h, bar.borderSize,
      bar.borderCol, bar.col
      );

  XMapWindow(wm.dpy, bar.win);
  XRaiseWindow(wm.dpy, bar.win);
  XSetClassHint(wm.dpy, bar.win, &(XClassHint){"sewm", "sewm"});
  //XSelectInput(wm.dpy, bar.win, ExposureMask);

  XChangeProperty(wm.dpy, bar.win, XInternAtom(wm.dpy, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *)&bar.win, 1);
  XChangeProperty(wm.dpy, bar.win, XInternAtom(wm.dpy, "_NET_WM_NAME", False), XInternAtom(wm.dpy, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char *)"sewm", 4);
  XChangeProperty(wm.dpy, DefaultRootWindow(wm.dpy), XInternAtom(wm.dpy, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *)&bar.win, 1);

  bar.gc = XCreateGC(wm.dpy, bar.win, 0, 0);
  XAllocNamedColor(wm.dpy, map, "#FFFFFF", &color, &color);
  XSetForeground(wm.dpy, bar.gc, color.pixel);
  XSetBackground(wm.dpy, bar.gc, bar.col);
}

void createButtons(int w, int h){
  XColor color;
  Colormap map = DefaultColormap(wm.dpy, DefaultScreen(wm.dpy));

  for(int i = 0; i < 9; i++){
    button[i].color = "#1a2026";
    button[i].activeColor = "#0077cc";
    button[i].fontColor = "#FFFFFF";

    XAllocNamedColor(wm.dpy, map, button[i].color, &color, &color);
    button[i].col = color.pixel;
    XAllocNamedColor(wm.dpy, map, button[i].activeColor, &color, &color);
    button[i].activeCol = color.pixel;

    button[i].x = i*w;
    button[i].y = 0;
    button[i].w = w;
    button[i].h = h;

    button[i].win = XCreateSimpleWindow(wm.dpy, bar.win,
        button[i].x, button[i].y, button[i].w, button[i].h, 0,
        button[i].col, button[i].col
        );

    XMapWindow(wm.dpy, button[i].win);
    XRaiseWindow(wm.dpy, button[i].win);
    XSelectInput(wm.dpy, button[i].win, ExposureMask);

    button[i].gc = XCreateGC(wm.dpy, button[i].win, 0, 0);
    XAllocNamedColor(wm.dpy, map, button[i].fontColor, &color, &color);
    button[i].fontCol = color.pixel;
    XSetForeground(wm.dpy, button[i].gc, color.pixel);
    XSetBackground(wm.dpy, button[i].gc, button[i].col);
  }
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

  for(int i = 0; i < 9; i++)
    d[i].mc = wm.mc;
  d[wm.ad].w = (Windows *)calloc(0, sizeof(*d[wm.ad].w));

  bar.x = 0;
  bar.y = 0;
  bar.w = monitorInfo.width;
  bar.h = 18;
  bar.borderSize = 0;
  bar.color = "#1a2026";
  bar.borderColor = "#0077cc";

  drawBar();
  createButtons(18, bar.h);

  map = DefaultColormap(wm.dpy, DefaultScreen(wm.dpy));
  XAllocNamedColor(wm.dpy, map, wm.focusedColor, &color, &color);
  wm.fcol = color.pixel;
  XAllocNamedColor(wm.dpy, map, wm.notFocusedColor, &color, &color);
  wm.nfcol = color.pixel;

  wm.borderSize = (wm.border ? wm.borderSize : 0);
  wm.bevent.subwindow = None;
  wm.on = true;

  while(wm.on)
    handleEvents();

  return 1;
}

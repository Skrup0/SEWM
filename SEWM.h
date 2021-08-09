#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

// made by skippy#6250
// this is the core of the WM

// headers
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
    int winCount, floatingCount, tiliedCount;
    char *focusedColor, *notFocusedColor;
    unsigned long fc, nfc;
    bool on;
    bool tiling;
    bool border;
} wm;

typedef struct{
    char *s;
    int num, x, y, w, h;
    Window win;
    Display *d;
} arg;

struct{
    int width, height, screen;
} monitorInfo;

typedef struct{
    Window win;
    int isFloating, hasBorder, isFullScreen, isFocused;
    unsigned long borderColor;
} Windows;
Windows windows[512];

char buffer[28];
Window *visibleChildren = NULL;
XWindowAttributes attrs;
unsigned int visibleCount = 0;
unsigned int tileCount = 0;
int tileSide = 0; // right: left, 1: bottom

int launchWM();
void handleEvents();

void fetchMonitorInfo();
void tileWindows();
void focusWindow(Window w);
void changeMode(arg args);
void manageWindow(Window w);
void addToWins(Window w);
void remFromWins(Window w);

int getWindowIndex(Window w);
int howManyFloating();

#include "binds.h" // i put it under the wm structure so it can access it
// implementations
void handleEvents(){
    XNextEvent(wm.dpy, &wm.event);
    XSync(wm.dpy, false);
    switch(wm.event.type){
        case KeyPress:
            printf("KeyPress event called.\n");
            XLookupString(&wm.event.xkey, buffer, 28, &wm.keysym, NULL);
            for(int i = 0; i < sizeof(binds) / sizeof(binds[0]); i++){
                if(wm.keysym == binds[i].key && binds[i].mod == wm.event.xkey.state)
                  (*binds[i].func)(binds[i].args);
            }
        break;
        case ButtonPress:
            printf("ButtonPress event called.\n");
            if(wm.event.xkey.subwindow != None){
                XGetWindowAttributes(wm.dpy, wm.event.xkey.subwindow, &wm.attrs);
                wm.bevent = wm.event.xbutton;
                //focusWindow(wm.event.xkey.subwindow); // already focused
                XRaiseWindow(wm.dpy, wm.event.xkey.subwindow);
            }
        break;
        case MotionNotify:
            printf("MotionNotify event called.\n");
            if(wm.bevent.subwindow != None){
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
            XGetWindowAttributes(wm.dpy, wm.event.xmap.window, &attrs);
            if(attrs.class != InputOnly && attrs.map_state == 2 && attrs.override_redirect == false){
                addToWins(wm.event.xmap.window);
                manageWindow(wm.event.xmap.window);
                tileWindows();
                if(wm.winCount)
                  focusWindow(windows[wm.winCount-1].win);
            }
        break;
        case UnmapNotify:
            printf("UnmapNotify event called.\n");
            remFromWins(wm.event.xunmap.window);
            tileWindows();
            if(wm.winCount)
              focusWindow(windows[wm.winCount-1].win);
        break;
        default:
            printf("Unknown/Ignored event called, (%d)\n", wm.event.type);
        break;
    }
}

void tileWindows(){
    if(!wm.tiling)
      return;
    wm.floatingCount = howManyFloating();
    wm.tiliedCount = wm.winCount - wm.floatingCount;
    int c=wm.tiliedCount, h=0, g=wm.gapSize+wm.borderSize, ii=0;
    for(int i = 0; i < wm.winCount; i++){
        if(!windows[i].isFloating && !windows[i].isFullScreen){
            if(ii == c || !c)
              break;

            if(c == 1){
              XMoveResizeWindow(wm.dpy, windows[i].win, wm.gapSize, wm.gapSize, monitorInfo.width-g*2, monitorInfo.height-g*2);
              break;
            }

            if(ii+1 != c){
              h = monitorInfo.height / (c - 1);
              XMoveResizeWindow(wm.dpy, windows[i].win, ((monitorInfo.width/2)+wm.gapSize)+wm.windowOffset, (h*((wm.winCount-2)-(ii+++wm.floatingCount)))+wm.gapSize, ((monitorInfo.width/2)-g*2)-wm.windowOffset, h-g*2);
            }else{
              XMoveResizeWindow(wm.dpy, windows[i].win, wm.gapSize, wm.gapSize, ((monitorInfo.width/2)-g*2)+wm.windowOffset, monitorInfo.height-g*2);
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
  for(int i = 0; i < wm.winCount; i++){
    if(windows[i].isFloating)
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
    windows[ii].isFocused = 0;
    windows[ii].borderColor = wm.nfc;
    XSetWindowBorder(wm.dpy, windows[ii].win, windows[ii].borderColor);
  }

  wm.focused = w;
  windows[i].isFocused = 1;
  windows[i].borderColor = wm.fc;
  XSetWindowBorder(wm.dpy, w, windows[i].borderColor);
  XSetInputFocus(wm.dpy, w, RevertToPointerRoot, CurrentTime);
}

void changeMode(arg args){
  int i = getWindowIndex(args.win);
 
  if(args.x){
    int ii = getWindowIndex(wm.focused);
    if(ii==-1){return;}
    windows[ii].isFloating = !windows[ii].isFloating;
    tileWindows();
    XRaiseWindow(wm.dpy, wm.focused);
    return;
  }
  if(i!=-1){
    windows[i].isFloating = args.num;
    tileWindows();
  }
}

int getWindowIndex(Window w){
  for(int i = 0; i < wm.winCount; i++){
    if(windows[i].win == w)
      return i;
  }
  return -1;
}

void addToWins(Window w){
    //windows = (Windows *)realloc(windows, wm.winCount+10);
    windows[wm.winCount].win = w;
    windows[wm.winCount].hasBorder = 1;
    windows[wm.winCount].isFloating = 0;
    windows[wm.winCount].isFullScreen = 0;

    wm.winCount++;
    focusWindow(w);
}

void removeFromArray(int pos){
  for(int i = pos; i < wm.winCount; i++){
      windows[i] = windows[i+1];
  }
  wm.winCount--;
}

void remFromWins(Window w){
    if(wm.focused == w)
      wm.focused = None;
    int i = getWindowIndex(w);
    if(i!=-1){
      removeFromArray(i);
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
    for(int i = 0; i < sizeof(binds) / sizeof(binds[0]); i++)
      XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, binds[i].key), binds[i].mod, DefaultRootWindow(wm.dpy), true, GrabModeAsync, GrabModeAsync);

    // move and resize
    XGrabButton(wm.dpy, 1, Mod1Mask, DefaultRootWindow(wm.dpy), true, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(wm.dpy, 3, Mod1Mask, DefaultRootWindow(wm.dpy), true, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    XSelectInput(wm.dpy, DefaultRootWindow(wm.dpy), SubstructureNotifyMask);

    map = DefaultColormap(wm.dpy, DefaultScreen(wm.dpy));
    XAllocNamedColor(wm.dpy, map, wm.focusedColor, &color, &color);
    wm.fc = color.pixel;
    XAllocNamedColor(wm.dpy, map, wm.notFocusedColor, &color, &color);
    wm.nfc = color.pixel;

    //windows = (Windows *)malloc(0);
    wm.borderSize = (wm.border ? wm.borderSize : 0);
    wm.bevent.subwindow = None;
    wm.on = true;
    while(wm.on){
        handleEvents();
    }

    return 1;
}

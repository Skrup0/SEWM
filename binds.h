// the bind system is almost exactly the same as dwm's
// but soon I will write an actual config file
// so that you don't have to edit the source everytime
// you want to edit something.

typedef struct{
    unsigned int mod;
    KeySym key;
    void (*func)();
    arg args;
} bind;

#define AltKey   Mod1Mask
#define ShiftKey ShiftMask

void setupBinds();
void quitWM();
void closeProgram();
void makeFullscreen();
void execApp(arg args);
void toggleTilingMode();
void changeFocus(arg args);
void changeGapSize(arg args);
void changeWindowOffset(arg args);
void switchMaster();

bind binds[] = {
    {AltKey|ShiftKey,    XK_Return, execApp,               {"st"}               },
    {AltKey,             XK_e,      execApp,               {"qutebrowser"}      },
    {AltKey,             XK_d,      execApp,               {"dmenu_run"}        },
    {AltKey,             XK_s,      execApp,               {"maim -s image.png"}},
    {AltKey|ShiftKey,    XK_s,      execApp,               {"sxiv image.png"}   },
    {AltKey|ShiftKey,    XK_space,  changeMode,            {.x = 1}             }, // toggle window mode between floating or tiling
    {AltKey|ShiftKey,    XK_c,      closeProgram                                },
    {AltKey|ShiftKey,    XK_f,      makeFullscreen                              },
    {AltKey|ShiftKey,    XK_t,      toggleTilingMode                            }, // turn tiling on/off
    {AltKey,             XK_Return, switchMaster,                               }, // switch positions between master window and focused window
    {AltKey,             XK_k,      changeFocus,           {.num = 0}           }, // change window focus
    {AltKey,             XK_j,      changeFocus,           {.num = 1}           },
    {AltKey,             XK_h,      changeWindowOffset,    {.num = 0}           }, // change the tiling offset
    {AltKey,             XK_l,      changeWindowOffset,    {.num = 1}           },
    {AltKey,             XK_minus,  changeGapSize,         {.num = 0}           }, // change the gap size
    {AltKey,             XK_equal,  changeGapSize,         {.num = 1}           },
    {AltKey|ShiftKey,    XK_q,      quitWM                                      },
};

void execApp(arg args){
    char buffer[256];
    sprintf(buffer, "setsid -f %s", args.s);
    system(buffer);
}

void quitWM(){
    wm.on = false;
}

void closeProgram(){
    if(wm.focused != None){
        // XDestroyWindow(wm.dpy, wm.focused);
        XEvent event;
        event.xclient.type = ClientMessage;
        event.xclient.window = wm.focused;
        event.xclient.message_type = XInternAtom(wm.dpy, "WM_PROTOCOLS", true);
        event.xclient.format = 32;
        event.xclient.data.l[0] = XInternAtom(wm.dpy, "WM_DELETE_WINDOW", false);
        event.xclient.data.l[1] = CurrentTime;

        XSendEvent(wm.dpy, wm.focused, false, NoEventMask, &event);
    }
}

void changeFocus(arg args){
  int i = getWindowIndex(wm.focused);
  if(i==-1)
    return;
  switch(args.num){
      case 0: // left
          focusWindow(i == 0 ? windows[wm.winCount-1].win : windows[i-1].win);
      break;
      case 1: // right
          focusWindow(i == wm.winCount-1 ? windows[0].win : windows[i+1].win);
      break;
  }
}

void changeWindowOffset(arg args){
  switch(args.num){
    case 0: // left
      wm.windowOffset = wm.windowOffset - 25;
      tileWindows();
    break;
    case 1: // right
      wm.windowOffset = wm.windowOffset + 25;
      tileWindows();
    break;
  }
}

void switchMaster(){
  int i = getWindowIndex(wm.focused);
  if(i==-1 || !wm.winCount)
    return;

  Windows w;
  w = windows[wm.winCount-1];

  windows[wm.winCount-1] = windows[i];
  windows[i] = w;
  tileWindows();
}

void changeGapSize(arg args){
  switch(args.num){
    case 0: // decrement
      if(wm.gapSize > 0){
        wm.gapSize--;
        tileWindows();
      }
    break;
    case 1: // increment
      wm.gapSize++;
      tileWindows();
    break;
  }
}

void makeFullscreen(){
    int i = getWindowIndex(wm.focused);
    if(wm.focused != None && i!=-1){
        Atom atoms[2] = {XInternAtom(wm.dpy, "_NET_WM_STATE_FULLSCREEN", false), None};   
        XChangeProperty(wm.dpy, wm.focused, XInternAtom(wm.dpy, "_NET_WM_STATE", false), XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, 1);
        XMoveResizeWindow(wm.dpy, wm.focused, -wm.borderSize, -wm.borderSize, monitorInfo.width+wm.borderSize, monitorInfo.height+wm.borderSize);

        windows[i].isFocused = 1;
    }
}

void toggleTilingMode(){
    wm.tiling = !wm.tiling;
    tileWindows();
}

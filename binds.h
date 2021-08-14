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

void quitWM();
void closeProgram();
void makeFullscreen();
void execApp(arg args);
void toggleTilingMode();
void changeFocus(arg args);
void changeGapSize(arg args);
void changeWindowOffset(arg args);
void switchMaster();
void changeMasterCount(arg args);

void changeWindowMap(arg args);
void changeDesktop(arg args);

bind binds[] = {
  {AltKey|ShiftKey,    XK_Return, execApp,               {"st"}               },
  {AltKey,             XK_e,      execApp,               {"qutebrowser"}      },
  {AltKey,             XK_p,      execApp,               {"dmenu_run"}        },
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
  {AltKey,             XK_d,      changeMasterCount,     {.num = 0}           }, // change the master window count
  {AltKey,             XK_i,      changeMasterCount,     {.num = 1}           },

  {AltKey,             XK_1,      changeDesktop,         {.num = 0}           },
  {AltKey,             XK_2,      changeDesktop,         {.num = 1}           },
  {AltKey,             XK_3,      changeDesktop,         {.num = 2}           },
  {AltKey,             XK_4,      changeDesktop,         {.num = 3}           },
  {AltKey,             XK_5,      changeDesktop,         {.num = 4}           },
  {AltKey,             XK_6,      changeDesktop,         {.num = 5}           },
  {AltKey,             XK_7,      changeDesktop,         {.num = 6}           },
  {AltKey,             XK_8,      changeDesktop,         {.num = 7}           },
  {AltKey,             XK_9,      changeDesktop,         {.num = 8}           },

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
      focusWindow(i == 0 ? d[wm.ad].w[d[wm.ad].wc-1].win : d[wm.ad].w[i-1].win);
      break;
    case 1: // right
      focusWindow(i == d[wm.ad].wc-1 ? d[wm.ad].w[0].win : d[wm.ad].w[i+1].win);
      break;
  }
}

void changeWindowOffset(arg args){
  switch(args.num){
    case 0: // left
      if(wm.windowOffset >= (-monitorInfo.width/2)+150){
        wm.windowOffset = wm.windowOffset - 50;
        tileWindows();
      }
      break;
    case 1: // right
      if(wm.windowOffset <= (monitorInfo.width/2)-150){
        wm.windowOffset = wm.windowOffset + 50;
        tileWindows();
      }
      break;
  }
}

void changeWindowMap(arg args){
  XWindowAttributes winAttrs;

  XGrabServer(wm.dpy);
  XGetWindowAttributes(wm.dpy, DefaultRootWindow(wm.dpy), &attrs);
  XGetWindowAttributes(wm.dpy, args.win, &winAttrs);

  XSelectInput(wm.dpy, DefaultRootWindow(wm.dpy), attrs.your_event_mask & ~SubstructureNotifyMask);
  XSelectInput(wm.dpy, args.win, winAttrs.your_event_mask & ~StructureNotifyMask);
  if(args.num){
    XUnmapWindow(wm.dpy, args.win);
  }else{
    XMapWindow(wm.dpy, args.win);
  }
  XSelectInput(wm.dpy, DefaultRootWindow(wm.dpy), attrs.your_event_mask);
  XSelectInput(wm.dpy, args.win, winAttrs.your_event_mask);
  XUngrabServer(wm.dpy);
}

void changeDesktop(arg args){
  if(args.num == wm.ad)
    return;

  // unfocus the prev focused desktop
  for(int i = 0; i < d[wm.ad].wc; i++){
    changeWindowMap((arg){.win = d[wm.ad].w[i].win, .num = 1});
  }

  // focus the new one
  wm.ad = args.num;
  for(int i = 0; i < d[args.num].wc; i++){
    changeWindowMap((arg){.win = d[args.num].w[i].win, .num = 0});
  }

  updateButtons();
  // focus the window in master
  if(d[args.num].wc)
    focusWindow(d[args.num].w[d[args.num].wc-1].win);
}

void changeMasterCount(arg args){
  if(!d[wm.ad].wc)
    return;
  switch(args.num){
    case 0:
      if(d[wm.ad].mc < d[wm.ad].tc){
        d[wm.ad].mc++;
        tileWindows();
      }
      break;
    case 1:
      if(d[wm.ad].mc > 0){
        d[wm.ad].mc--;
        tileWindows();
      }
      break;
  }
}

void switchMaster(){
  int i = getWindowIndex(wm.focused);
  if(i==-1 || !d[wm.ad].wc)
    return;

  Windows w;
  w = d[wm.ad].w[d[wm.ad].wc-1];

  d[wm.ad].w[d[wm.ad].wc-1] = d[wm.ad].w[i];
  d[wm.ad].w[i] = w;
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
  if(wm.focused && i!=-1){
    Atom atoms[2] = {XInternAtom(wm.dpy, "_NET_WM_STATE_FULLSCREEN", false), None};   
    XChangeProperty(wm.dpy, wm.focused, XInternAtom(wm.dpy, "_NET_WM_STATE", false), XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, 1);
    XMoveResizeWindow(wm.dpy, wm.focused, -wm.borderSize, -wm.borderSize, monitorInfo.width+wm.borderSize, monitorInfo.height+wm.borderSize);

    d[wm.ad].w[i].isFocused = 1;
    d[wm.ad].w[i].isFloating = 1;
  }
}

void toggleTilingMode(){
  wm.tiling = !wm.tiling;
  tileWindows();
}

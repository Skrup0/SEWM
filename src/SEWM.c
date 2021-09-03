#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/cursorfont.h>

#include "SEWM.h"
#include "binds.h"
#include "util.h"
#include "calls.h"

// this is the core of the WM

void handleEvents(){
	XNextEvent(wm.dpy, &wm.event);
	XSync(wm.dpy, false);
	switch(wm.event.type){
		case Expose:
			exposeCall();
			break;
		case KeyPress:
			keyPressCall();
			break;
		case ButtonPress:
			buttonPressCall();
			break;
		case MotionNotify:
			motionNotifyCall();
			break;
		case ButtonRelease:
			buttonReleaseCall();
			break;
		case EnterNotify:
			enterNotifyCall();
			break;
		case MapNotify:
			mapNotifyCall();
			break;
		case UnmapNotify:
			unmapNotifyCall();
		default:
			printf("Unknown/Ignored event called, (%d)\n", wm.event.type);
			break;
	}
}

static void *updateBarWidgets(){
	XEvent exppp;
	while(wm.on){
		sleep(1);
    memset(&exppp, 0, sizeof(exppp));
    exppp.type = Expose;
    XSendEvent(wm.dpy, DefaultRootWindow(wm.dpy), False, ExposureMask, &exppp);
    XFlush(wm.dpy);
	}
}

// TODO: make better tiling
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
	if(!wm.tiling)
		XMoveWindow(wm.dpy, w, (monitorInfo.width/2)-(attrs.width/2), (monitorInfo.height/2)-(attrs.height/2) + bar.h);

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
	wm.fonts = (XftFont **)calloc(1, sizeof(wm.fonts));
	for(int i = 0; i < SIZEOF(fontNames); i++){
		wm.fonts = (XftFont **)realloc(wm.fonts, sizeof(wm.fonts) * (i+1));
		wm.fonts[i] = XftFontOpenName(wm.dpy, DefaultScreen(wm.dpy), fontNames[i]);
	}
}

void drawText(char *string, XftColor xcolor, Window win, int x, int y, int clear){
	static const char lengths[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
	};
	Visual *visual = DefaultVisual(wm.dpy, DefaultScreen(wm.dpy));
	Colormap map   = DefaultColormap(wm.dpy, DefaultScreen(wm.dpy));
	XftDraw *draw  = XftDrawCreate(wm.dpy, win, visual, map);
	char *t = NULL, *tmp = NULL;
	int j = 0;

	if(clear)
		XClearWindow(wm.dpy, win);

	for(t = string; t-string < strlen(string); t = tmp){
		unsigned char *s = t;
		int len = lengths[s[0] >> 3];
		char *next = s + len + !len;
		for(int i = 0; i < SIZEOF(fontNames); i++){
			tmp = t;
			if(XftCharExists(wm.dpy, wm.fonts[i], (FcChar32)decode_code_point(&tmp))){
				XftDrawStringUtf8(draw, &xcolor, wm.fonts[i], x+(10*j), y, (const FcChar8 *)t, next-t);
				j++;
				break;
			}
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

void addToWins(Window w, int i){
	d[i].w = (Windows *)realloc(d[i].w, sizeof(*d[i].w) * (d[i].wc+1));

	d[i].w[d[i].wc].win = w;
	d[i].w[d[i].wc].hasBorder = 1;
	d[i].w[d[i].wc].isFloating = 0;
	d[i].w[d[i].wc].isFullScreen = 0;

	d[i].wc++;
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
			xColors[1], xColors[bar.bg]
			);

	XMapWindow(wm.dpy, bar.win);
	XRaiseWindow(wm.dpy, bar.win);
	XSetClassHint(wm.dpy, bar.win, &(XClassHint){"sewm", "sewm"});

	XChangeProperty(wm.dpy, bar.win, XInternAtom(wm.dpy, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *)&bar.win, 1);
	XChangeProperty(wm.dpy, bar.win, XInternAtom(wm.dpy, "_NET_WM_NAME", False), XInternAtom(wm.dpy, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char *)"sewm", 4);
	XChangeProperty(wm.dpy, DefaultRootWindow(wm.dpy), XInternAtom(wm.dpy, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char *)&bar.win, 1);
}

void renderBarWidgets(){
	int j = 0, k = 0;
	char *tmp = NULL;
	for(int i = 0; i < SIZEOF(rightBarWidgets); i++){
		if(rightBarWidgets[i].func){
			tmp = (char *)malloc(strlen(rightBarWidgets[i].func()) + strlen(rightBarWidgets[i].content) + 1);
			strcpy(tmp, rightBarWidgets[i].content);
			strcat(tmp, rightBarWidgets[i].func());
		}else{
			tmp = rightBarWidgets[i].content;
		}

		k = getActualSize(tmp);
		j += k;
		if(rightBarWidgets[i].bg != bar.bg){
			if(rightBarWidgets[i].bgWin)
				XDestroyWindow(wm.dpy, rightBarWidgets[i].bgWin);
			rightBarWidgets[i].bgWin = XCreateSimpleWindow(wm.dpy, bar.win,
					(bar.w-(j*10))-1, 0, (k*10), bar.h+1, 0,
					xColors[1], xColors[rightBarWidgets[i].bg]
					);
			XMapWindow(wm.dpy, rightBarWidgets[i].bgWin);
			drawText(tmp, xFontColors[rightBarWidgets[i].fg], rightBarWidgets[i].bgWin, 0, 14, 0);
		}else{
			drawText(tmp, xFontColors[rightBarWidgets[i].fg], bar.win, bar.w-(j*10), 14, 0);
		}
	}
}

void createButtons(int w, int h, int bg){
	for(int i = 0; i < SIZEOF(button); i++){
		button[i].x = i*w;
		button[i].y = 0;
		button[i].w = w;
		button[i].h = h;
		button[i].args = (arg){.num = i};
		button[i].func = changeDesktop;

		button[i].win = XCreateSimpleWindow(wm.dpy, bar.win,
				button[i].x, button[i].y, button[i].w, button[i].h, 0,
				xColors[1], xColors[bg]
				);

		XSelectInput(wm.dpy, button[i].win, ButtonPressMask|ExposureMask);
		XMapWindow(wm.dpy, button[i].win);
		XRaiseWindow(wm.dpy, button[i].win);
	}
	updateButtons();
}

void fetchMonitorInfo(){
	monitorInfo.screen = DefaultScreen(wm.dpy);
	monitorInfo.width  = DisplayWidth (wm.dpy, monitorInfo.screen);
	monitorInfo.height = DisplayHeight(wm.dpy, monitorInfo.screen);
}

int launchWM(){
	XSetWindowAttributes mainAttrs;
	Colormap map;
	XColor color;
  pthread_t t1 = 0;
	setlocale(LC_CTYPE, "");

	printf("Starting SEWM...\n");
	wm.dpy = XOpenDisplay(NULL);
	if(!wm.dpy)
		die("ERROR: cannot open display! EXITING...\n");
	fetchMonitorInfo();

	XUngrabKey(wm.dpy, AnyKey, AnyModifier, DefaultRootWindow(wm.dpy));
	unsigned int masks[] = {0, LockMask};
	for(int i = 0; i < sizeof(binds) / sizeof(binds[0]); i++)
		for(int v = 0; v < 2; v++)
			XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, binds[i].key), binds[i].mod|masks[v], DefaultRootWindow(wm.dpy), true, GrabModeAsync, GrabModeAsync);

	// move and resize
	XGrabButton(wm.dpy, 1, Mod1Mask, DefaultRootWindow(wm.dpy), true, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(wm.dpy, 3, Mod1Mask, DefaultRootWindow(wm.dpy), true, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

	XSelectInput(wm.dpy, DefaultRootWindow(wm.dpy), SubstructureNotifyMask | ExposureMask);

	wm.mc = 1; // master count
	for(int i = 0; i < SIZEOF(d); i++)
		d[i].mc = wm.mc;
	d[wm.ad].w = (Windows *)calloc(0, sizeof(*d[wm.ad].w));

	loadColors();
	loadFonts();
	bar.x	 = 0;
	bar.y  = 0;
	bar.w  = monitorInfo.width;
	bar.h  = 18;
	bar.bg = 0;
	drawBar();
	createButtons(26, bar.h, bar.bg);
	renderBarWidgets();

	mainAttrs.cursor = XCreateFontCursor(wm.dpy, XC_left_ptr);
	XChangeWindowAttributes(wm.dpy, DefaultRootWindow(wm.dpy), CWCursor, &mainAttrs);

	wm.borderSize = (wm.border ? wm.borderSize : 0);
	wm.bevent.subwindow = None;
	wm.on = true;

  pthread_create(&t1, NULL, updateBarWidgets, NULL);

	while(wm.on)
		handleEvents();

	return 1;
}

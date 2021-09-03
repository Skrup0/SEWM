#include "calls.h"

void exposeCall(){
	XClearWindow(wm.dpy, bar.win);
	renderBarWidgets();
	updateButtons();
}

void keyPressCall(){
	wm.keysym = XKeycodeToKeysym(wm.dpy, (KeyCode)wm.event.xkey.keycode, 0);
	for(int i = 0; i < SIZEOF(binds); i++)
		if(wm.keysym == binds[i].key 
		&& (binds[i].mod & ~(LockMask) & (ShiftMask|ControlMask)) == (wm.event.xkey.state & ~(LockMask) & (ShiftMask|ControlMask)) 
		&& binds[i].mod == wm.event.xkey.state)
			(*binds[i].func)(binds[i].args);
}

void buttonPressCall(){
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
}

void motionNotifyCall(){
	if(wm.bevent.subwindow){
		wm.dx = wm.event.xbutton.x_root - wm.bevent.x_root;
		wm.dy = wm.event.xbutton.y_root - wm.bevent.y_root;
		XMoveResizeWindow(wm.dpy, wm.bevent.subwindow, wm.attrs.x + (wm.bevent.button==1 ? wm.dx : 0), wm.attrs.y + (wm.bevent.button==1 ? wm.dy : 0), MAX(100, wm.attrs.width + (wm.bevent.button==3 ? wm.dx : 0)), MAX(100, wm.attrs.height + (wm.bevent.button==3 ? wm.dy : 0)));
		if(wm.tiling)
			changeMode((arg){.win = wm.event.xkey.subwindow, .num = 1});
	}
}

void buttonReleaseCall(){
	wm.bevent.subwindow = None;
}

void enterNotifyCall(){
	focusWindow(wm.event.xcrossing.window);
}

void mapNotifyCall(){
	if(wm.event.xmap.window == bar.win)
		return;

	XGetWindowAttributes(wm.dpy, wm.event.xmap.window, &attrs);
	if(attrs.class != InputOnly && attrs.map_state == 2 && attrs.override_redirect == false){
		addToWins(wm.event.xmap.window, wm.ad);
		tileWindows();
		manageWindow(wm.event.xmap.window);
		focusWindow(wm.event.xmap.window);
		if(d[wm.ad].wc)
			focusWindow(d[wm.ad].w[d[wm.ad].wc-1].win);
	}
}

void unmapNotifyCall(){
	printf("UnmapNotify event called.\n");
	remFromWins(wm.event.xunmap.window);
	tileWindows();
	if(d[wm.ad].wc)
		focusWindow(d[wm.ad].w[d[wm.ad].wc-1].win);
}


#include "barWidgets.h"

char *getTime(){
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char *currentTime = (char *)malloc(28);
	strftime(currentTime, 28, "%I:%M%p", timeinfo);

	return currentTime;
}

char *getMemUsage(){
	char *result = malloc(28);
	char *line = NULL;
	size_t len;

	int t, s, f, b, c, r;
	FILE *meminfo = fopen("/proc/meminfo", "r");
	while(getline(&line, &len, meminfo) != EOF){
		sscanf(line, "MemTotal: %d", &t);
		sscanf(line, "Shmem: %d", &s);
		sscanf(line, "MemFree: %d", &f);
		sscanf(line, "Buffers: %d", &b);
		sscanf(line, "Cached: %d", &c);
		sscanf(line, "SReclaimable: %d", &r);
	}

	snprintf(result, 28, "%dMB", (t + (s-f-b-c-r))/1024);
	fclose(meminfo);
	free(line);
	return result;
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


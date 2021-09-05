#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

time_t rawtime;
struct tm *timeinfo;

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

typedef struct{
	char *content;
	int fg;
	int bg;
	char *(*func)();
	Window bgWin;
} BarWidgets;

BarWidgets rightBarWidgets[] = {
	{" | SEWM v0.1", 0, 0},
	{" | ", 0, 0, getTime},
	{"", 0, 0, getMemUsage},
};

struct{
	Window win;
	GC gc;

	int x, y, w, h, bg, borderSize, widgetSpacing;
} bar;

struct{
	Window win;
	GC gc;

	int x, y, w, h;
	arg args;
	void (*func)();
} button[9];
XWindowAttributes attrs;

const char *buttonNames[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
const char *fontNames[] = {"monospace:size=14"}; 
const char *languages[] = {"us"};

const char *colors[] = {
	"#1a2026", // bar color
	"#0077cc", // border color
	"#888888", // inactive border color
};
const char *fontColors[] = {
	"#FFFFFF", // default font color
	"#0077cc", // current desktop font color
	"#888888", // non-empty desktop font color
};
unsigned long *xColors;
XftColor *xFontColors;
int lang = 0;

int launchWM();
void die(const char *msg);
void handleEvents();

void fetchMonitorInfo();
void tileWindows();
void focusWindow(Window w);
void changeMode(arg args);
void manageWindow(Window w);
void addToWins(Window w, int i);
void remFromWins(Window w);
void removeFromArray(int pos, int ind);
void drawBar();
void createButtons(int w, int h, int bg);
void updateButtons();
static void *updateBarWidgets();

void loadColors();
void loadFonts();
void drawText(char *string, XftColor xcolor, Window win, int x, int y, int clear);
void renderBarWidgets();

int getWindowIndex(Window w);
int howManyFloating();

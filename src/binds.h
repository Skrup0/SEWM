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
#define SuperKey Mod4Mask
#define desktopTags(m,k,i) \
{m,                  k,         changeDesktop,         {.num = i}           }, \
{m|ShiftKey,         k,         moveToOtherDesktop,    {.num = i}           },

void quitWM();
void closeProgram();
void makeFullscreen();
void execApp(arg args);
void toggleTilingMode();
void changeFocus(arg args);
void changeGapSize(arg args);
void changeWindowOffset(arg args);
void switchMaster();
void raiseFocusedWindow();
void changeLang();
void changeMasterCount(arg args);
void moveToOtherDesktop(arg args);

void changeWindowMap(arg args);
void changeDesktop(arg args);

bind binds[] = {
	{AltKey|ShiftKey,    XK_Return, execApp,               {"st"}               },
	{AltKey,             XK_q,      execApp,               {"qutebrowser"}      },
	{AltKey,             XK_p,      execApp,               {"dmenu_run"}        },
	{AltKey|ShiftKey,    XK_space,  changeMode,            {.x = 1}             }, // toggle window mode between floating or tiling
	{AltKey|ShiftKey,    XK_c,      closeProgram                                },
	{AltKey|ShiftKey,    XK_f,      makeFullscreen                              },
	{AltKey|ShiftKey,    XK_t,      toggleTilingMode                            }, // turn tiling on/off
	{AltKey,             XK_Return, switchMaster,                               }, // switch positions between master window and focused window
	{AltKey,             XK_j,      changeFocus,           {.num = 0}           }, // change window focus
	{AltKey,             XK_k,      changeFocus,           {.num = 1}           },
	{AltKey,             XK_h,      changeWindowOffset,    {.num = 0}           }, // change the tiling offset
	{AltKey,             XK_l,      changeWindowOffset,    {.num = 1}           },
	{AltKey,             XK_minus,  changeGapSize,         {.num = 0}           }, // change the gap size
	{AltKey,             XK_equal,  changeGapSize,         {.num = 1}           },
	{AltKey,             XK_d,      changeMasterCount,     {.num = 0}           }, // change the master window count
	{AltKey,             XK_i,      changeMasterCount,     {.num = 1}           },
	{AltKey,             XK_f,      raiseFocusedWindow,                         },
	{SuperKey,					 XK_space,  changeLang                                   },
	{AltKey|ShiftKey,    XK_q,      quitWM                                      },

	{SuperKey,           XK_q,			execApp,               {"mpc play"}         },
	{SuperKey,           XK_w,  		execApp,               {"mpc pause"}        },
	{SuperKey,           XK_e,  		execApp,               {"mpc stop"}         },
	{SuperKey,           XK_equal,  execApp,               {"mpc volume +5"}    },
	{SuperKey,           XK_minus,  execApp,               {"mpc volume -5"}    },
	{SuperKey,           XK_h,			execApp,               {"mpc seek -10"}     },
	{SuperKey,           XK_l,			execApp,               {"mpc seek +10"}     },
 
	desktopTags(AltKey, XK_1, 0)
		desktopTags(AltKey, XK_2, 1)
		desktopTags(AltKey, XK_3, 2)
		desktopTags(AltKey, XK_4, 3)
		desktopTags(AltKey, XK_5, 4)
		desktopTags(AltKey, XK_6, 5)
		desktopTags(AltKey, XK_7, 6)
		desktopTags(AltKey, XK_8, 7)
		desktopTags(AltKey, XK_9, 8)
};


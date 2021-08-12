#include "SEWM.h"
#include <X11/Xlib.h>

// SEWM (skippy's epic window manager)
// made by Skippy#6250
// compile with 
// ``make install``

// how to use:
// put ``exec sewm`` in your .xinitrc file
// then restart X

int main(int argc, char *argv[]){
  // some settings
  wm.borderSize = 2;
  wm.gapSize = 16;
  wm.tiling = true;
  wm.border = true;
  wm.focusedColor = "#0077cc";
  wm.notFocusedColor = "#aaaaaa";

  launchWM();

  XFree(d);
  return 0;
}

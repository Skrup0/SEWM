#include "SEWM.h"

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
  wm.gapSize = 0;
  wm.tiling = true;
  wm.border = true;
  wm.focusedColor = "#0077cc";
  wm.notFocusedColor = "#aaaaaa";
  wm.masterCount = 1;

  launchWM();

  return 0;
}

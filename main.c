#include "SEWM.h"

// SEWM (skippy's epic window manager)
// made by Skippy#6250
// compile with 
// ``gcc -o wm main.c -L/usr/X11/lib -lX11``
//
// how to use:
// put ``exec wm`` in your .xinitrc file
// by using the command ``echo "exec wm" >> .xinitrc``
// or by writing it manually.
// make sure you specified the right location of wm
// and your .xinitrc file.

int main(int argc, char *argv[]){
    // some settings
    wm.borderSize = 2;
    wm.gapSize = 0;
    wm.tiling = true;
    wm.border = true;
    wm.focusedColor = "#0077cc";
    wm.notFocusedColor = "#aaaaaa";

    launchWM();

    return 0;
}

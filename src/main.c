#include "SEWM.c"
#include "binds.c"
#include "util.c"
#include "calls.c"

int main(int argc, char *argv[]){
  wm.borderSize = 2;
  wm.gapSize = 16;
  wm.tiling = true;
  wm.border = true;

  launchWM();

  XFree(d);
  return 0;
}

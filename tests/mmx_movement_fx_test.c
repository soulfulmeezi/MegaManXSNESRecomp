#include "mmx_movement_fx.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  unsigned dust=0,change=0,landing=0;
  for(int y=-25;y<30;y++)for(int x=-55;x<55;x++) {
    int a=MmxMovementFxPixel(0x14,1,1,0,x,y);
    int b=MmxMovementFxPixel(0x14,1,1,5,x,y);
    assert(MmxMovementFxPixel(0,1,1,0,x,y)==-1);
    assert(MmxMovementFxPixel(0x0a,0,1,0,x,y)==-1);
    if(a>=0){dust++;assert(a<=0x7fff && x<0);}
    change+=a!=b;
    landing+=MmxMovementFxPixel(0x0a,1,0,0,x,y)>=0;
  }
  assert(dust>10 && dust<100 && change>10 && landing==8);
  puts("movement: bounded animated dash dust, landing arcs and inactive states passed");
}

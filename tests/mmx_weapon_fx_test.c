#include "mmx_weapon_fx.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  int earth=0,jupiter=0,motion=0;
  assert(MmxFxWeaponDetailPixel(MMX_JUPITER_STORM,0,0,0)==-1);
  assert(MmxFxWeaponDetailPixel(MMX_EARTH_WAVE,48,0,0)==-1);
  for(int y=-36;y<=30;++y)for(int x=-40;x<=40;++x) {
    int a=MmxFxWeaponDetailPixel(MMX_EARTH_WAVE,8,x,y);
    int b=MmxFxWeaponDetailPixel(MMX_JUPITER_STORM,8,x,y);
    int c=MmxFxWeaponDetailPixel(MMX_JUPITER_STORM,18,x,y);
    if(a>=0) {++earth;assert(a<=0x7fff);}
    if(b>=0) {++jupiter;assert(b<=0x7fff);}
    if(b!=c)++motion;
  }
  assert(earth>10 && jupiter>10 && motion>20);
  printf("CosmicFX weapon accents: earth=%d storm=%d changing=%d\n",
         earth,jupiter,motion);
  return 0;
}

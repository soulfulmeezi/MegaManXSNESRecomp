#include "mmx_environment_fx.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  uint16_t sky=MmxFxRgb(10,19,27);
  assert(!MmxFxBlueSky(MmxFxRgb(8,9,9)));
  assert(!MmxFxBlueSky(MmxFxRgb(2,5,12)));
  assert(MmxFxAtmospherePixel(MmxFxRgb(8,9,9),100,70,0,0)==-1);
  assert(MmxFxAtmospherePixel(sky,100,160,0,0)==-1);
  assert(MmxFxAtmospherePixel(sky,100,0,0,0)==-1);
  assert(MmxFxLerpRgb(sky,sky,16)==sky);
  assert(MmxFxLerpRgb(sky,MmxFxRgb(31,31,31),0)==sky);
  int cloud=0, moving=0, parallax=0;
  for(int y=16;y<152;++y) for(int x=0;x<342;++x) {
    int a=MmxFxAtmospherePixel(sky,x,y,0,0);
    int b=MmxFxAtmospherePixel(sky,x,y,0,120);
    int c=MmxFxAtmospherePixel(sky,x,y,240,0);
    if(a>=0) {
      ++cloud;
      assert(a<=0x7fff);
    }
    if(a!=b) ++moving;
    if(a!=c) ++parallax;
  }
  assert(cloud>800 && cloud<342*136);
  assert(moving>300 && parallax>300);
  /* Visuals and simulation are deterministic from camera and saved tick. */
  for(int t=0;t<200;t+=17)
    assert(MmxFxAtmospherePixel(sky,76,72,246,t)==
           MmxFxAtmospherePixel(sky,76,72,246,t));
  printf("CosmicFX clouds: %d pixels, %d moving, %d parallax\n",
         cloud,moving,parallax);
  return 0;
}

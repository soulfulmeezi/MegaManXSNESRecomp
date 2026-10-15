#include "mmx_environment_fx.h"
#include "mmx_orbital_background.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  unsigned orbital_motion=0, orbital_parallax=0;
  for(int y=0;y<224;y++)for(int x=-512;x<768;x++) {
    int a=MmxOrbitalBackgroundPixel(x,y,0,0);
    assert(a>=0 && a<=0x7fff);
    orbital_motion+=a!=MmxOrbitalBackgroundPixel(x,y,0,240);
    orbital_parallax+=a!=MmxOrbitalBackgroundPixel(x,y,320,0);
  }
  assert(orbital_motion>20 && orbital_parallax>10000);
  uint16_t sky=MmxFxRgb(10,19,27);
  assert(!MmxFxBlueSky(MmxFxRgb(8,9,9)));
  assert(!MmxFxBlueSky(MmxFxRgb(2,5,12)));
  assert(MmxFxAtmospherePixel(MmxFxRgb(8,9,9),100,70,0,0)==-1);
  assert(MmxFxAtmospherePixel(sky,100,160,0,0)==-1);
  assert(MmxFxAtmospherePixel(sky,100,0,0,0)==-1);
  uint16_t night=MmxFxRgb(12,8,11);
  assert(MmxFxHighwaySky(night));
  assert(!MmxFxHighwaySky(MmxFxRgb(6,12,18)));
  assert(MmxFxHighwayCloudPixel(night,100,180,0,0)==-1);
  assert(MmxFxLerpRgb(sky,sky,16)==sky);
  assert(MmxFxLerpRgb(sky,MmxFxRgb(31,31,31),0)==sky);
  int cloud=0, moving=0, parallax=0, night_cloud=0, night_motion=0;
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
    int n=MmxFxHighwayCloudPixel(night,x,y,0,0);
    int m=MmxFxHighwayCloudPixel(night,x,y,0,120);
    if(n>=0)++night_cloud;
    if(n!=m)++night_motion;
  }
  assert(cloud>800 && cloud<342*136);
  assert(moving>300 && parallax>300);
  assert(night_cloud>100 && night_motion>100);
  /* Visuals and simulation are deterministic from camera and saved tick. */
  for(int t=0;t<200;t+=17)
    assert(MmxFxAtmospherePixel(sky,76,72,246,t)==
           MmxFxAtmospherePixel(sky,76,72,246,t));
  printf("CosmicFX clouds: %d pixels, %d moving, %d parallax; highway=%d moving=%d\n",
         cloud,moving,parallax,night_cloud,night_motion);
  return 0;
}

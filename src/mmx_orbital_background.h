#pragma once
#include <stdint.h>
#include "mmx_background_fleet.h"
/* Original integer pixel art. BG2 only; no collision or actor state changes. */
static inline int MmxOrbitRGB(int r,int g,int b) { return r|(g<<5)|(b<<10); }
static inline unsigned MmxOrbitHash(unsigned n) { n^=n>>16;n*=0x7feb352du;n^=n>>15;return n; }
static inline int MmxOrbitalBackgroundPixel(int x,int y,int camera,unsigned tick) {
  int sy=y<0?0:y>223?223:y;
  int color=MmxOrbitRGB(2+sy/56,2+sy/40,8+sy/28);
  int far=x+camera/16;
  unsigned cell=MmxOrbitHash((unsigned)(far/24)*71u+(unsigned)(sy/20)*977u);
  if((unsigned)(far%24)==cell%24 && (unsigned)(sy%20)==(cell>>8)%20)
    color=MmxOrbitRGB(18,22,28);
  /* Broad overlapping cloud lobes breathe over a 24-second cycle.
   * Smooth intensity changes replace hatching and on/off particles. */
  unsigned phase=tick%1440;
  int breath=phase<720?(int)phase:1440-(int)phase;
  int cloud=0;
  for(int i=0;i<3;i++) {
    int cx=90+i*153+(int)(tick/24%512);
    int dx=((far-cx+2048)%512)-256;
    int dy=sy-(42+i*18);
    int density=64-dx*dx/90-dy*dy/12;
    if(density>0)cloud+=density;
  }
  if(cloud>64)cloud=64;
  int glow=cloud*(200+breath)/920/10;
  if(cloud>0 && color==MmxOrbitRGB(2+sy/56,2+sy/40,8+sy/28))
    color=MmxOrbitRGB(2+sy/56+glow,2+sy/40+glow/2,8+sy/28+glow);
  int px=(far%512+512)%512-188,py=sy-72;
  int radius=px*px+py*py;
  if(radius<65*65) {
    if(radius>62*62) color=MmxOrbitRGB(11,17,25);
    else {
      int band=(py+65+(px*px/380))/7;
      int shade=(px+64)/18;
      int r=band%3==0?22:29,g=band%3==0?12:21,b=band%3==0?9:15;
      if(((px+22)*(px+22)*2+(py-17)*(py-17)*5)<210) {r=26;g=9;b=6;}
      color=MmxOrbitRGB(r-shade,g-shade,b-shade);
    }
  }
  /* Two depths of orbital architecture. Deterministic building silhouettes. */
  for(int layer=0;layer<2;layer++) {
    int wx=x+camera/(layer?3:7),col=(wx+8192)/19;
    unsigned h=MmxOrbitHash((unsigned)col+layer*991u);
    int roof=(layer?169:143)-(int)(h%38),local=(wx+8192)%19;
    if(sy>=roof && local<16) {
      color=layer?MmxOrbitRGB(3,7,12):MmxOrbitRGB(5,10,16);
      if(sy==roof)color=MmxOrbitRGB(8,18,23);
      if(sy>roof+5 && local%5==2 && sy%7==2)
        color=((h>>(sy/14%16))&1)?MmxOrbitRGB(7,23,26):MmxOrbitRGB(7,12,17);
    }
  }
  int ship=MmxFleetScenePixel(x,y,camera,tick);
  if(ship>=0)color=ship;
  return color;
}

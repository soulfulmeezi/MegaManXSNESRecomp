#pragma once
#include <stdint.h>
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
  int ship=(int)((tick/2)%640)-96;
  if(y>=104 && y<=106 && x>=ship && x<ship+14)
    color=y==105?MmxOrbitRGB(24,27,30):MmxOrbitRGB(6,16,23);
  if(y==105 && x>=ship-9 && x<ship)color=MmxOrbitRGB(7,20,28);
  return color;
}

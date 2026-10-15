#ifndef MMX_WEAPON_FX_H
#define MMX_WEAPON_FX_H
#include <stdint.h>
#include "mmx_cosmic.h"

/* Cosmetic pixel weapons, independent of damage actors and physics.
 * Works at the 60-tick game clock and survives save/capture replay. */
static inline int MmxFxWeaponDetailPixel(unsigned kind,unsigned age,int x,int y) {
  if(!age || age>=48 || x<-40 || x>40 || y<-36 || y>30) return -1;
  if(kind==MMX_EARTH_WAVE) {
    /* Staggered wake motes accent the original animated rolling crest.
     * Sparks travel backward around the broad silhouette on each tick. */
    for(unsigned i=0;i<5;++i) {
      int wx=-33+(int)(i*9)+((int)(age*2+i*3)%15);
      int wy=-20+(int)(i*6)+((int)(age+i*7)%7)-3;
      int dx=x-wx,dy=y-wy;
      if(dx>=-1 && dx<=1 && dy>=-1 && dy<=1 &&
         (dx==0 || dy==0)) return (i&1) ?
           MmxCosmicRgb(30,25,10) : MmxCosmicRgb(26,30,28);
    }
    /* Thin disappearing ground wake beneath the moving wave. */
    if(y>=5 && y<=7 && x>=-35 && x<=-4 &&
       (((x+(int)age*3)&7)<4) && (x+y+age)%3!=0)
      return MmxCosmicRgb(20,25,17);
  } else if(kind==MMX_JUPITER_STORM) {
    /* Broken, growing electrical halo: not one static crescent/triangle.
     * Its holes, sparks and arc thickness shift with projectile age. */
    int radius=22+(int)((age/4)%8);
    int dist=x*x+y*y;
    if(dist>=(radius-2)*(radius-2) && dist<=radius*radius &&
       ((x*3+y*5+(int)age)%9+9)%9<5)
      return ((x+y+(int)age)&3) ?
          MmxCosmicRgb(11,26,31) : MmxCosmicRgb(30,31,31);
    for(unsigned i=0;i<4;++i) {
      int sx=((int)(age*5+i*19)%65)-32;
      int sy=((int)(age*3+i*13)%47)-24;
      if(x==sx && y>=sy && y<=sy+3)
        return MmxCosmicRgb(29,30,31);
    }
  }
  return -1;
}
#endif

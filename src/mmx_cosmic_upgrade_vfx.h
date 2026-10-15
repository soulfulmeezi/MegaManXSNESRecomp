#pragma once
#include <stdint.h>
#include "mmx_cosmic.h"

/* Deterministic visual-only spark orbit derived from saved projectile age.
 * Opt-in with MMX_COSMIC_UPGRADE_VFX=1 in the custom renderer.
 * This code does not change hitboxes, damage, physics, input or save data.
 */
static inline int MmxCosmicUpgradeSparkPixel(unsigned kind,unsigned age,int x,int y) {
  if((kind!=MMX_EARTH_WAVE && kind!=MMX_JUPITER_STORM) ||
     age<3 || age>44) return -1;
  static const int8_t dx[16]={14,13,10,5,0,-5,-10,-13,-14,-13,-10,-5,0,5,10,13};
  static const int8_t dy[16]={0,5,10,13,14,13,10,5,0,-5,-10,-13,-14,-13,-10,-5};
  unsigned phase=(age/2u)&15u;
  for(unsigned i=0;i<2;++i) {
    unsigned pos=(phase+i*8u)&15u;
    int px=(int)dx[pos]-(kind==MMX_EARTH_WAVE?4:0);
    int py=(int)dy[pos]-(kind==MMX_EARTH_WAVE?5:0);
    int ax=x-px,ay=y-py;
    unsigned distance=(unsigned)(ax<0?-ax:ax)+(unsigned)(ay<0?-ay:ay);
    if(distance==0) return kind==MMX_EARTH_WAVE?
      MmxCosmicRgb(24,31,27):MmxCosmicRgb(30,28,20);
    if(distance==1 && (age&1u)) return kind==MMX_EARTH_WAVE?
      MmxCosmicRgb(9,23,19):MmxCosmicRgb(19,14,8);
  }
  return -1;
}

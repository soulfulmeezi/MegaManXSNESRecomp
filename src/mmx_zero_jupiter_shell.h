#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "mmx_future_zero_color.h"

/* Jupiter shell: color is sourced from cloud belts, not from SNES metallic
 * light/shadow ramps. Original Zero sprite opacity, outlines, animation,
 * hair, facial skin and dark undersuit remain untouched. */

static inline unsigned MmxJovianMax3(unsigned a,unsigned b,unsigned c) {
  unsigned ab=a>b?a:b; return ab>c?ab:c;
}
static inline unsigned MmxJovianMin3(unsigned a,unsigned b,unsigned c) {
  unsigned ab=a<b?a:b; return ab<c?ab:c;
}
static inline bool MmxJovianFaceSkin(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return r>=18 && g>=10 && b>=7 && r>=g+4 && g>=b+2;
}
static inline bool MmxJovianGoldenHair(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return r>=20 && g>=15 && b<=11 && r>=g && g>=b+8;
}
static inline bool MmxJovianMaterial(uint16_t c,
    unsigned row,unsigned face_bottom) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  unsigned hi=MmxJovianMax3(r,g,b),lo=MmxJovianMin3(r,g,b);
  if(MmxJovianFaceSkin(c) || hi<=12) return false;
  /* Hair remains golden above the face, including moving ponytail poses.
   * The same original yellows below the neck belong to boots/arm accents. */
  if(MmxJovianGoldenHair(c)) return row>face_bottom+14;
  if(MmxFutureZeroIsRedArmor(c)) return true;
  /* Bright silver and white were NOT affected by Pass 1's red-only palette
   * conversion; these are responsible for Zero's persistent metal shine. */
  if(lo>=13 && hi-lo<=8) return true;
  /* Convert the green chest jewel and other brightly colored body details
   * into cloud matter, as requested. Blue face/visor pixels are protected. */
  if(g>=14 && g>=r+5 && g>=b+4) return true;
  /* Small warm metallic shades, excluding all skin and golden hair. */
  if(r>=17 && g>=8 && b<=10 && r>=g+4) return true;
  return false;
}

/* Fixed-pixel SNES gas bands. We deliberately discard the base armor shade
 * for eligible material, so metallic panels become one continuous cloudy
 * surface contained inside the ORIGINAL Zero silhouette. Coordinates are
 * pose-local; no camera/world-position dependence or randomized flicker. */
static inline uint16_t MmxJovianShellColor(
    uint16_t original,uint16_t base,unsigned x,unsigned y,
    unsigned shell_top,unsigned face_bottom) {
  if(x>=128 || y>=128 || y<shell_top ||
     !MmxJovianMaterial(original,y,face_bottom)) return base;
  /* Curved 1–2px perturbation: belts undulate rather than forming rigid,
   * identical horizontal ruler lines. */
  static const int8_t curve[12]={0,0,1,1,2,1,0,0,-1,-1,-2,-1};
  int relative=(int)y-(int)shell_top+curve[(x/5u)%12u];
  if(relative<0) relative=0;
  unsigned phase=(unsigned)relative%28u;
  if(phase<5) return MmxFutureZeroBgr555(29,28,26); /* cloud-white */
  if(phase<9) return MmxFutureZeroBgr555(27,25,22); /* ivory */
  if(phase<14) return MmxFutureZeroBgr555(25,21,16);/* sand */
  if(phase<20) return MmxFutureZeroBgr555(23,16,10);/* orange belt */
  if(phase<24) return MmxFutureZeroBgr555(19,12,8); /* storm bronze */
  return MmxFutureZeroBgr555(27,25,22);             /* cream transition */
}

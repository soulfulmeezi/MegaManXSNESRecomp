#pragma once
#include <stdint.h>

/* +175% = 11/4 of the resolved hit. Native HP is integral: round to the
 * nearest point, retaining immunity, special responses and the HP limit. */
static inline unsigned MmxEarthDamageBoost(unsigned damage, int earth_x,
                                           unsigned enemy, unsigned shot) {
  if (!earth_x || !damage || damage >= 128 || enemy < 0xe68 ||
      enemy >= 0x1228 || (enemy & 63) != 0x28 || shot < 0x1228 ||
      shot >= 0x1428 || (shot & 63) != 0x28) return damage;
  unsigned result = (damage * 11 + 2) / 4;
  return result > 127 ? 127 : result;
}
enum { MMX_COSMIC_PAGE = 3, MMX_EARTH_WAVE = 1, MMX_JUPITER_STORM = 2 };
static inline uint16_t MmxCosmicRgb(unsigned r,unsigned g,unsigned b) {
  return (uint16_t)(r | (g<<5) | (b<<10));
}
/* Authored, opaque SNES pixels. -1 leaves the scene transparent. */
static inline int MmxCosmicShotPixel(unsigned kind,unsigned age,int x,int y) {
  if(kind==MMX_EARTH_WAVE) {
    if(x < -10 || x > 10 || y < -10 || y > 8) return -1;
    int ax=x<0?-x:x;
    int crest=-(9-ax/2)+(int)((age/3+(unsigned)(x+10)/4)&1);
    if(y==crest || y==crest+1) return MmxCosmicRgb(27,31,30);
    if(y>crest+1 && y<crest+5) return MmxCosmicRgb(5,22,18);
    if(y>=4 && y<=7 && ((x+(int)age/2)&7)<3) return MmxCosmicRgb(19,17,9);
  } else if(kind==MMX_JUPITER_STORM) {
    if(x < -9 || x > 9 || y < -16 || y > 16) return -1;
    int ay=y<0?-y:y;
    int arc=7-ay*ay/32;
    if(x==arc || x==arc-1) return MmxCosmicRgb(31,30,26);
    if(x==arc-2 || x==arc-3) return MmxCosmicRgb(30,15,6);
    int bolt=(y+18)/4%2? -3:1;
    if(x==bolt && ((age/2)&1)) return MmxCosmicRgb(26,29,31);
  }
  return -1;
}
/* Distant Highway ambience. The caller applies this only to the original
 * BG2 layer, behind terrain, actors and HUD, before native fades/windows. */
static inline int MmxCityActivityPixel(int x,int y,unsigned tick,uint16_t base) {
  unsigned r=base&31,g=(base>>5)&31,b=(base>>10)&31;
  int u=(x+(int)(tick/8))&255;
  int sky=r>=8 && r<=14 && g>=5 && g<=10 && b>=7 && b<=14 && r>g;
  if(sky && y>=16 && y<100) {
    int band=(y-16)/28, row=(y-16)%28;
    int drift=(u+band*83)&127;
    int curve=3+(drift<48?drift:96-drift)/12;
    if(drift<96 && row>=curve && row<=curve+2 && ((drift+row)&3)!=0)
      return MmxCosmicRgb(r+3,g+3,b+3);
  }
  /* Tiny pairs of headlights follow two distant traffic lanes. */
  int lane=y==115 || y==141;
  int traffic=(x+(y==115?(int)(tick/2):-(int)(tick/3)))&127;
  if(lane && sky && (traffic==12 || traffic==15 || traffic==73 || traffic==76))
    return MmxCosmicRgb(21,20,13);
  /* Sparse building windows change slowly; never paint the open sky. */
  if(!sky && y>=70 && y<164 && r<12 && g<14 && b<16 &&
      (x&31)>=9 && (x&31)<=11 && (y&15)==6 &&
      (((unsigned)x/32+(unsigned)y/16+tick/90)&3)==0)
    return MmxCosmicRgb(16,19,14);
  return -1;
}

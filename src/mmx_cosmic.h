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
    if(x < -24 || x > 12 || y < -28 || y > 9) return -1;
    /* A breaking ridge of stone, a blue-white rim and a trailing dust wake.
     * x points forward; the renderer mirrors the entire effect with travel. */
    if(x>=-10 && x<=10) {
      static const int8_t ridge[21]={-1,-4,-3,-7,-10,-9,-15,-14,-20,-26,-25,-20,-18,-21,-16,-11,-9,-8,-4,-5,-1};
      int crest=ridge[x+10]+(int)((age/4)&1);
      if(y>=crest && y<=crest+2) return MmxCosmicRgb(28,31,31);
      if(y>crest+2 && y<crest+6) return MmxCosmicRgb(4,23,25);
      if(y>=crest+6 && y<=7) {
        if(((x+32)/4+(y+32)/5)%3==0) return MmxCosmicRgb(5,12,15);
        return y<0?MmxCosmicRgb(14,20,15):MmxCosmicRgb(21,19,12);
      }
    }
    if(x<-8) {
      int rise=(int)((age+(unsigned)(-x)*3)%16),dust=-x-8;
      if(y==5-rise && dust%5<3) return MmxCosmicRgb(25,26,20);
      if(y>=6 && y<=8 && ((x+(int)age)&7)<5) return MmxCosmicRgb(9,18,19);
    }
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
/* Quantized SNES lighting, with no timer independent of saved combat state. */
static inline unsigned MmxCosmicLightStrength(int dx,int dy,unsigned power) {
  unsigned ax=(unsigned)(dx<0?-dx:dx),ay=(unsigned)(dy<0?-dy:dy);
  unsigned distance=ax>ay?ax+ay/2:ay+ax/2;
  unsigned falloff=distance/8;
  return power>falloff?power-falloff:0;
}
static inline uint16_t MmxCosmicLitColor(uint16_t base,unsigned earth,unsigned storm,unsigned shade) {
  unsigned values[3];
  if(earth>10) earth=10;if(storm>10) storm=10;if(shade>4) shade=4;
  for(unsigned i=0;i<3;++i) {
    unsigned c=(base>>(i*5))&31;
    c=c*(16-shade)/16 + earth*(i==0?1:2) + storm*(i==2?1:2);
    values[i]=c>31?31:c;
  }
  return MmxCosmicRgb(values[0],values[1],values[2]);
}
/* Distant Highway ambience. The caller applies this only to the original
 * BG2 layer, behind terrain, actors and HUD, before native fades/windows. */
static inline int MmxCityActivityPixel(int x,int y,unsigned tick,uint16_t base) {
  unsigned r=base&31,g=(base>>5)&31,b=(base>>10)&31;
  int u=(x+(int)(tick/8))&255;
  int sky=r>=8 && r<=14 && g>=5 && g<=10 && b>=7 && b<=14 && r>g;
  if(y>=164 && y<224) {
    /* Distant lower streets: emergency traffic, localized blasts, smoke and
     * falling sparks. These pixels belong to BG2, behind highway pillars. */
    unsigned cell=(unsigned)x>>7,phase=(tick+cell*83)%240;
    int local=x&127,dx=local-58,dy=y-192;
    int ax=dx<0?-dx:dx,ay=dy<0?-dy:dy;
    if(phase<96) {
      int radius=phase<24?2+(int)phase/2:14-(int)(phase-24)/6;
      if(ax+ay<radius) return ax+ay<radius/2?MmxCosmicRgb(31,29,18):MmxCosmicRgb(28,12,4);
      if(phase>12 && dy<0 && dy>-(int)(phase/2) && ax<3+(192-y)/6 &&
          ((x+y+(int)phase/6)&3)!=0) return MmxCosmicRgb(8,8,10);
    }
    if(phase>=24 && phase<112 && local>=45 && local<=71) {
      int fall=164+(int)((phase+local*7)%52);
      if(y==fall && (local%7)==0) return MmxCosmicRgb(30,18,5);
    }
    if(y>=188 && y<=191) {
      int car=(x+(int)(tick/2))&127;
      if(car>=12 && car<=23) {
        if(y==188) return (tick/10)&1?MmxCosmicRgb(27,5,4):MmxCosmicRgb(7,17,31);
        return y==191?MmxCosmicRgb(8,10,14):MmxCosmicRgb(20,21,20);
      }
    }
  }
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
/* The lower expressways use dark green asphalt. Bright support pillars and
 * lane paint must occlude traffic instead of receiving painted-on cars. */
static inline int MmxLowerRoadColor(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return r>=3 && r<=10 && g>=6 && g<=14 && b>=3 && b<=10 && g>=r+2 && g>b;
}
static inline int MmxLowerCarPixel(int x,int y,unsigned depth,unsigned reverse,unsigned variant) {
  int half=2+(int)depth/20,height=7+(int)depth/10;
  if(x < -half || x>half || y<0 || y>=height) return -1;
  if(reverse) y=height-1-y;
  int ax=x<0?-x:x;
  if((y==0 || y==height-1) && ax==half) return -1;
  if(ax==half || y==0 || y==height-1) return MmxCosmicRgb(2,4,5);
  if(y==height-2 && ax==half-1) return MmxCosmicRgb(30,30,22);
  if(y==1 && ax==half-1) return MmxCosmicRgb(27,4,3);
  if(y==height-4 || y==height-3) return MmxCosmicRgb(15,24,26);
  static const uint16_t bodies[]={9|(14<<5)|(23<<10),23|(13<<5)|(6<<10),20|(22<<5)|(20<<10)};
  return bodies[variant%3];
}

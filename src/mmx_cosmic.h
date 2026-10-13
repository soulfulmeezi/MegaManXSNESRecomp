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
    /* A crest rolls backward through the advancing wave. The silhouette,
     * interior ribbons and ballistic fragments all animate independently. */
    static const uint8_t swell[16]={0,1,3,6,10,15,20,24,26,24,20,15,10,6,3,1};
    unsigned phase=age&31;
    if(x>=-18 && x<=10) {
      unsigned index=(unsigned)(x+32+(int)phase)/2&15;
      int crest=5-(int)swell[index];
      if(y>=crest && y<crest+2) return MmxCosmicRgb(27,31,31);
      if(y>=crest+2 && y<crest+5) return MmxCosmicRgb(3,23,24);
      /* Open moving ribbons replace the solid triangular slab. */
      unsigned stream=(unsigned)(y*2+x+96+(int)age*3)&15;
      if(y>=crest+5 && y<=7 && stream<4) return MmxCosmicRgb(5,16,18);
      if(y>=crest+5 && y<=7 && stream==4) return MmxCosmicRgb(15,27,24);
    }
    for(unsigned i=0;i<5;++i) {
      int t=(int)((age+i*7)%20),px=10-t*2+(int)(i&1)*3;
      int py=5-t*2+t*t/12+(int)(i&1)*2;
      int size=t<10?2:1;
      if(x>=px && x<px+size && y>=py && y<py+size)
        return (t&2)?MmxCosmicRgb(23,21,14):MmxCosmicRgb(13,16,11);
    }
    if(y>=6 && y<=8 && ((x+(int)age*2)&7)<5) return MmxCosmicRgb(9,22,21);
  } else if(kind==MMX_JUPITER_STORM) {
    if(x < -20 || x > 13 || y < -19 || y > 19) return -1;
    static const int8_t radius[8]={13,15,17,18,17,15,13,12};
    static const int8_t lean[8]={-2,-1,0,1,2,1,0,-1};
    unsigned phase=(age/2)&7;
    int yy=y-lean[phase],ay=yy<0?-yy:yy;
    int arc=8-ay*ay/(radius[phase]+9)+lean[phase];
    unsigned flow=(unsigned)(y+32+(int)age*2)&15;
    int thickness=2+(int)((phase+2)%3);
    if(ay<=radius[phase]) {
      if(x==arc || (x==arc-1 && flow<11)) return MmxCosmicRgb(31,30,26);
      if(x<arc && x>=arc-thickness) return flow<6?MmxCosmicRgb(28,20,12):MmxCosmicRgb(30,13,5);
      if(x==arc-thickness-3 && flow<9) return MmxCosmicRgb(18,20,24);
      int bolt=-5+(int)((unsigned)(y+24)/3+phase)%3*3;
      if(x==bolt && flow<7) return MmxCosmicRgb(23,28,31);
    }
    /* Shed sparks peel backward from the changing upper/lower tips. */
    int t=(int)(age%8);
    if(x==-7-t && (y==radius[phase]-t || y==-radius[phase]+t))
      return MmxCosmicRgb(31,24,16);
  }
  return -1;
}
/* Quantized SNES lighting, with no timer independent of saved combat state. */
static inline unsigned MmxCosmicLightStrength(int dx,int dy,unsigned power) {
  unsigned ax=(unsigned)(dx<0?-dx:dx)*2,ay=(unsigned)(dy<0?-dy:dy);
  unsigned distance=ax>ay?ax+ay/2:ay+ax/2;
  unsigned falloff=distance/4;
  if(power>6) power=6;
  return power>falloff?(power-falloff+1)/2:0;
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

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
static inline int MmxWrapPositive(int value,int period) {
  value%=period;
  return value<0?value+period:value;
}
static inline int MmxDistantTrafficPixel(int x,int y,unsigned tick) {
  /* Tiny skyline hovercars replace isolated headlight dots. */
  static const int lane_y[2]={115,141};
  for(unsigned lane=0;lane<2;++lane) {
    int ly=y-lane_y[lane];
    if(ly<-1 || ly>1) continue;
    int traffic=(x+(lane==0?(int)(tick/2):-(int)(tick/3)))&127;
    for(unsigned i=0;i<2;++i) {
      int dx=traffic-(i?73:12);
      if(dx<0 || dx>5) continue;
      if(ly==0) {
        if(dx==0 || dx==5) return MmxCosmicRgb(23,22,16);
        if(dx==1 || dx==4) return MmxCosmicRgb(13,19,24);
        return dx==2?MmxCosmicRgb(18,21,23):MmxCosmicRgb(8,11,14);
      }
      if(ly==-1 && dx>=1 && dx<=4) return (dx==2||dx==3)?MmxCosmicRgb(10,14,18):MmxCosmicRgb(6,8,11);
      if(ly==1 && dx>=1 && dx<=4) return (5| (7u<<5) | (10u<<10));
    }
  }
  return -1;
}

static inline int MmxLowerRoadColor(uint16_t c);

static inline int MmxLowerStreetPixel(int x,int y,unsigned tick,uint16_t base) {
  if(!MmxLowerRoadColor(base) || y<164 || y>223) return -1;
  /* Background highway lanes: opposing traffic on straightaways. */
  struct Lane { int y0,h,w,period,speed,offset,count; uint16_t roof,body,light,shadow; };
  static const struct Lane lanes[] = {
    {170,3,8,96,  1, 17, 4, 19023, 15720, 5398,  8388},
    {187,5,14,96, -2, 43, 4, 21170, 16777, 4283, 9445},
    {206,3,9,96,   2, 71, 4, 21038, 12583,  32295, 7332},
    {219,3,9,96,  -1, 29, 4, 21072, 13640, 6398,  7332}
  };
  for(unsigned li=0;li<sizeof(lanes)/sizeof(lanes[0]);++li) {
    const struct Lane *L=&lanes[li];
    int row=y-L->y0;
    if(row<0 || row>=L->h) continue;
    int scroll=x + (int)((long long)tick * (long long)L->speed / 2);
    int local=MmxWrapPositive(scroll + L->offset,L->period);
    for(int car=0;car<L->count;++car) {
      int dx=local - (8 + car*(L->period/L->count));
      if(dx<0 || dx>=L->w) continue;
      if(L->h==5) {
        if(row==0 && dx>=2 && dx<=11) return dx<4 || dx>9?L->body:L->roof;
        if(row==1 && dx>=1 && dx<=12) {
          if(dx==1 || dx==12) return L->light;
          if(dx>=4 && dx<=9) return L->roof;
          return L->body;
        }
        if(row==2 && dx>=2 && dx<=11) return dx<4 || dx>9?L->shadow:L->body;
        if(row==3 && dx>=3 && dx<=10) return L->shadow;
        if(row==4 && dx>=5 && dx<=8 && ((dx+(int)tick)&1)==0) return MmxCosmicRgb(3,8,12);
      } else {
        if(row==0 && dx>=1 && dx<L->w-1) return dx==L->w-2?L->light:L->roof;
        if(row==1 && dx>=0 && dx<L->w) return (dx==0 || dx==L->w-1)?L->light:L->body;
        if(row==2 && dx>=1 && dx<L->w-1) return dx>2 && dx<L->w-3?L->shadow:L->body;
      }
    }
  }
  /* Curved on/off-ramps: traffic enters/exits along the bend instead of sliding flat. */
  struct Ramp { int range,speed,offset,dir,base_y,slope,curve; uint16_t roof,body,light,shadow; };
  static const struct Ramp ramps[] = {
    /* lower-left entry curve -> flows left to right */
    {160, 2, 13,  1, 216, -1, -3, 21137, 15720, 4283, 8388},
    /* central diagonal ramp -> flows right to left */
    {148, 2, 57, -1, 188, -1,  0, 24275, 17834,31304, 10469},
    /* right-side exit curve -> flows right to left */
    {152, 2, 91, -1, 176,  1,  3, 23218, 16777, 6398,  8388}
  };
  for(unsigned ri=0;ri<sizeof(ramps)/sizeof(ramps[0]);++ri) {
    const struct Ramp *R=&ramps[ri];
    for(int car=0;car<3;++car) {
      int t=MmxWrapPositive((int)(tick*R->speed) + R->offset + car*(R->range/3), R->range);
      int px,py;
      if(ri==0) {
        px=-24 + t;
        py=R->base_y - t/6 + (t*t)/220;
      } else if(ri==1) {
        px=260 - t;
        py=R->base_y - t/4;
      } else {
        px=340 - t;
        int q=152-t;
        py=R->base_y + q/8 + (q*q)/260;
      }
      int dx=x-px,dy=y-py;
      if(dx<0 || dx>=12 || dy<0 || dy>=5) continue;
      if(dy==0 && dx>=3 && dx<=9) return dx<5 || dx>7?R->body:R->roof;
      if(dy==1 && dx>=1 && dx<=10) return (dx==1 || dx==10)?R->light:(dx>=4 && dx<=7?R->roof:R->body);
      if(dy==2 && dx>=2 && dx<=10) return dx<4 || dx>8?R->shadow:R->body;
      if(dy==3 && dx>=3 && dx<=9) return R->shadow;
      if(dy==4 && dx>=5 && dx<=7 && (((int)tick+dx+ri)&1)==0) return MmxCosmicRgb(3,8,12);
    }
  }
  if(y>=176 && y<=179) {
    int column=(x+32)&127;
    if((column==22 || column==86) && (((tick/20)+x/64)&1)==0)
      return y==176?MmxCosmicRgb(23,16,6):MmxCosmicRgb(11,8,6);
  }
  return -1;
}

static inline int MmxMidRoadColor(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return r>=7 && r<=20 && g>=9 && g<=22 && b>=11 && b<=26 && b>=g && g+2>=r;
}

static inline int MmxNearRoadTrafficPixel(int x,int y,unsigned tick,uint16_t base) {
  if(!MmxMidRoadColor(base) || y<108 || y>162) return -1;
  struct FlatLane { int y0,h,w,period,speed,offset; uint16_t roof,body,light,shadow; };
  static const struct FlatLane flats[] = {
    {117,5,16,112, 2, 19, (20| (23u<<5) | (23u<<10)), (11| (14u<<5) | (18u<<10)), (29| (8u<<5) | (7u<<10)), (5| (7u<<5) | (10u<<10))},
    {146,5,16,104,-2, 41, (19| (22u<<5) | (24u<<10)), (10| (13u<<5) | (18u<<10)), (8| (18u<<5) | (30u<<10)), (5| (7u<<5) | (10u<<10))}
  };
  for(unsigned li=0;li<sizeof(flats)/sizeof(flats[0]);++li) {
    const struct FlatLane *L=&flats[li];
    int row=y-L->y0;
    if(row<0 || row>=L->h) continue;
    int scroll=L->speed>=0 ? x + (int)(tick*(unsigned)L->speed/2) : x - (int)(tick*(unsigned)(-L->speed)/2);
    int local=(scroll + L->offset) & (L->period-1);
    for(unsigned car=0;car<3;++car) {
      int dx=local - (car? (int)(car*(L->period/3) + 11) : 11);
      if(dx<0 || dx>=L->w) continue;
      if(row==0 && dx>=3 && dx<=12) return dx<5 || dx>10?L->body:L->roof;
      if(row==1 && dx>=1 && dx<=14) {
        if(dx==1 || dx==14) return L->light;
        if(dx>=4 && dx<=11) return L->roof;
        return L->body;
      }
      if(row==2 && dx>=2 && dx<=13) return dx<4 || dx>11?L->shadow:L->body;
      if(row==3 && dx>=3 && dx<=12) return L->shadow;
      if(row==4 && dx>=5 && dx<=9 && ((dx+(int)tick)&1)==0) return MmxCosmicRgb(3,8,12);
    }
  }
  for(unsigned car=0;car<3;++car) {
    int cx=((int)(tick*2) + 38 + (int)car*92)%336 - 24;
    int cy=116 + cx/4;
    int dx=x-cx,dy=y-cy;
    if(dx>=0 && dx<12 && dy>=0 && dy<5) {
      static const uint16_t roof=(20| (22u<<5) | (23u<<10)), body=(10| (13u<<5) | (17u<<10)),
                            light=(28| (8u<<5) | (7u<<10)), shadow=(5| (7u<<5) | (10u<<10));
      if(dy==0 && dx>=3 && dx<=9) return dx<5 || dx>7?body:roof;
      if(dy==1 && dx>=1 && dx<=10) return (dx==1 || dx==10)?light:(dx>=4 && dx<=7?roof:body);
      if(dy==2 && dx>=2 && dx<=10) return dx<4 || dx>8?shadow:body;
      if(dy==3 && dx>=3 && dx<=9) return shadow;
      if(dy==4 && dx>=5 && dx<=7) return MmxCosmicRgb(3,8,12);
    }
  }
  return -1;
}

static inline int MmxCityActivityPixel(int x,int y,unsigned tick,uint16_t base) {

  unsigned r=base&31,g=(base>>5)&31,b=(base>>10)&31;
  int u=(x+(int)(tick/8))&255;
  int sky=r>=8 && r<=14 && g>=5 && g<=10 && b>=7 && b<=14 && r>g;
  if(y>=88 && y<164) {
    int nearer=MmxNearRoadTrafficPixel(x,y,tick,base);
    if(nearer>=0) return nearer;
  }
  if(y>=164 && y<224) {
    int lower=MmxLowerStreetPixel(x,y,tick,base);
    if(lower>=0) return lower;
  }
  if(sky && y>=16 && y<100) {
    int band=(y-16)/28, row=(y-16)%28;
    int drift=(u+band*83)&127;
    int curve=3+(drift<48?drift:96-drift)/12;
    if(drift<96 && row>=curve && row<=curve+2 && ((drift+row)&3)!=0)
      return MmxCosmicRgb(r+3,g+3,b+3);
  }
  /* Skyline traffic should read like actual tiny hovercars, not static dots. */
  if(sky) {
    int distant=MmxDistantTrafficPixel(x,y,tick);
    if(distant>=0) return distant;
  }
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
  /* Only the darker lower-highway asphalt should qualify; exclude bright bridge deck/support colors. */
  return r>=2 && r<=8 && g>=5 && g<=11 && b>=7 && b<=15 &&
         g>=r+2 && b>=g-1 && (r+g+b)<=28;
}

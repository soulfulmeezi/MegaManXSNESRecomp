#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "mmx_zero_jupiter_shell.h"

/* Per-pose armor regions for Jupiter Zero. Every connected plate gets its
 * own local band origin and wave seed. The original indexed sprite remains
 * untouched: transparency, outlines, face, hair, visor and undersuit keep
 * their original draw path and pixel edges. */
enum {
  MMX_JUPITER_POSE_WIDTH=128,
  MMX_JUPITER_POSE_PIXELS=16384,
  MMX_JUPITER_MAX_SECTIONS=255,
  MMX_JUPITER_SECTION_CHEST=1,
  MMX_JUPITER_SECTION_BOOT=2,
  MMX_JUPITER_SECTION_SWIRL=4
};

typedef struct {
  uint8_t left,top,right,bottom;
  uint8_t center_x,center_y,pattern_seed,flags;
  uint16_t area;
} MmxJupiterPlanetSection;

static inline bool MmxJupiterPlanetBlueFace(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return b>=15 && b>=r+5 && b>=g+3;
}
static inline bool MmxJupiterPlanetBrightArmor(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  unsigned hi=MmxJovianMax3(r,g,b);
  return hi>=14 && !MmxJovianFaceSkin(c) &&
      !MmxJupiterPlanetBlueFace(c);
}
static inline bool MmxJupiterPlanetPaintable(unsigned pixel,
    const uint16_t palette[128]) {
  return pixel>=16 && pixel<32 &&
      MmxJupiterPlanetBrightArmor(palette[pixel]);
}

/* The mask stores 0 for protected pixels and a 1-based connected armor
 * section ID for every painted pixel. Small seam pixels are first bridged
 * into the source armor material, then included in their plate's section. */
static inline void MmxJupiterPlanetMask(
    const uint8_t source[MMX_JUPITER_POSE_PIXELS],
    const uint16_t palette[128],
    uint8_t mask[MMX_JUPITER_POSE_PIXELS],
    MmxJupiterPlanetSection sections[MMX_JUPITER_MAX_SECTIONS],
    unsigned *section_count) {
  uint8_t protected_hair[MMX_JUPITER_POSE_PIXELS]={0};
  uint16_t queue[MMX_JUPITER_POSE_PIXELS];
  unsigned top=128,bottom=0,face_x=0,face_y=0,faces=0;
  for(unsigned y=0;y<128;++y) for(unsigned x=0;x<128;++x) {
    unsigned pixel=source[y*128+x];
    if(pixel>=16 && pixel<32) {
      if(y<top) top=y;
      if(y>bottom) bottom=y;
    }
  }
  if(top==128) {
    for(unsigned i=0;i<MMX_JUPITER_POSE_PIXELS;++i) mask[i]=0;
    *section_count=0;return;
  }
  for(unsigned y=top;y<128 && y<top+40;++y) for(unsigned x=0;x<128;++x) {
    unsigned pixel=source[y*128+x];
    if(pixel>=16 && pixel<32 && MmxJovianFaceSkin(palette[pixel])) {
      face_x+=x;face_y+=y;++faces;
    }
  }
  face_x=faces?face_x/faces:64;
  face_y=faces?face_y/faces:top+18;

  /* Preserve connected golden hair close to and above the face. Gold lower
   * on the sprite remains eligible for the boot plates. */
  for(unsigned y=top;y<128 && y<top+45;++y) for(unsigned x=0;x<128;++x) {
    unsigned p=y*128+x,pixel=source[p];
    if(protected_hair[p] || pixel<16 || pixel>=32 ||
        !MmxJovianGoldenHair(palette[pixel]) ||
        (int)x<(int)face_x-24 || x>face_x+24 || y>face_y+2) continue;
    unsigned head=0,tail=0;queue[tail++]=(uint16_t)p;
    protected_hair[p]=1;
    while(head<tail) {
      unsigned q=queue[head++],qx=q%128,qy=q/128;
      for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
        if(!dx && !dy) continue;
        int nx=(int)qx+dx,ny=(int)qy+dy;
        if(nx<0 || ny<0 || nx>=128 || ny>=128) continue;
        unsigned next=(unsigned)ny*128+(unsigned)nx,v=source[next];
        if(!protected_hair[next] && v>=16 && v<32 &&
            MmxJovianGoldenHair(palette[v])) {
          protected_hair[next]=1;queue[tail++]=(uint16_t)next;
        }
      }
    }
  }

  /* Candidate pass: bright metal, red/gold armor and vivid plate details
   * become cloud material, independent of the source's panel shading. */
  for(unsigned i=0;i<MMX_JUPITER_POSE_PIXELS;++i) {
    unsigned pixel=source[i];
    mask[i]=(uint8_t)(MmxJupiterPlanetPaintable(pixel,palette) &&
                       !protected_hair[i]);
  }
  /* Bridge only short, enclosed metallic seams. This uses the original
   * candidate mask, so the gap never grows into the black undersuit. */
  for(unsigned y=2;y<126;++y) for(unsigned x=2;x<126;++x) {
    unsigned i=y*128+x,pixel=source[i];
    if(mask[i] || pixel<16 || pixel>=32 || protected_hair[i])continue;
    uint16_t c=palette[pixel];
    unsigned hi=MmxJovianMax3(c&31,(c>>5)&31,(c>>10)&31);
    if(hi<10 || hi>19 || MmxJovianFaceSkin(c) ||
       MmxJupiterPlanetBlueFace(c)) continue;
    bool across=(mask[i-1] && mask[i+1]) ||
        (mask[i-2] && mask[i+2]) ||
        (mask[i-128] && mask[i+128]) ||
        (mask[i-256] && mask[i+256]);
    if(across)mask[i]=1;
  }

  /* Flood-fill each plate independently. Eight-way connectivity follows
   * diagonal SNES pixel contours while black outlines keep armor sections
   * separate. A bounded final slot safely absorbs pathological tiny islands. */
  unsigned count=0;
  for(unsigned i=0;i<MMX_JUPITER_POSE_PIXELS;++i) {
    if(mask[i]!=1) continue;
    unsigned label=count<MMX_JUPITER_MAX_SECTIONS?count+1:
        MMX_JUPITER_MAX_SECTIONS;
    unsigned head=0,tail=0;
    queue[tail++]=(uint16_t)i;mask[i]=255;
    unsigned left=127,right=0,top_y=127,bottom_y=0;
    while(head<tail) {
      unsigned q=queue[head++],qx=q%128,qy=q/128;
      if(qx<left)left=qx;
      if(qx>right)right=qx;
      if(qy<top_y)top_y=qy;
      if(qy>bottom_y)bottom_y=qy;
      for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
        if(!dx && !dy)continue;
        int nx=(int)qx+dx,ny=(int)qy+dy;
        if(nx<0 || ny<0 || nx>=128 || ny>=128)continue;
        unsigned next=(unsigned)ny*128+(unsigned)nx;
        if(mask[next]==1) { mask[next]=255;queue[tail++]=(uint16_t)next; }
      }
    }
    bool new_section=count<MMX_JUPITER_MAX_SECTIONS;
    if(new_section) ++count;
    MmxJupiterPlanetSection *s=&sections[label-1];
    if(new_section) {
      s->left=(uint8_t)left;s->right=(uint8_t)right;
      s->top=(uint8_t)top_y;s->bottom=(uint8_t)bottom_y;
      s->center_x=(uint8_t)((left+right)/2);
      s->center_y=(uint8_t)((top_y+bottom_y)/2);
      s->area=(uint16_t)(tail>65535?65535:tail);
      s->pattern_seed=(uint8_t)(((left/4u)*3u+(top_y/4u)*5u)%16u);
      s->flags=0;
    } else {
      /* More than 255 islands share the last texture region safely. */
      if(left<s->left)s->left=(uint8_t)left;
      if(right>s->right)s->right=(uint8_t)right;
      if(top_y<s->top)s->top=(uint8_t)top_y;
      if(bottom_y>s->bottom)s->bottom=(uint8_t)bottom_y;
      if((unsigned)(UINT16_MAX-s->area)<tail)s->area=UINT16_MAX;
      else s->area=(uint16_t)(s->area+tail);
      s->center_x=(uint8_t)((s->left+s->right)/2);
      s->center_y=(uint8_t)((s->top+s->bottom)/2);
    }
    for(unsigned n=0;n<tail;++n)mask[queue[n]]=(uint8_t)label;
  }

  /* Find the most plausible central torso patch relative to the animated
   * face. Only it receives Jupiter's Great Red Spot. */
  unsigned chest=0;int chest_score=-2147483647;
  for(unsigned n=0;n<count;++n) {
    MmxJupiterPlanetSection *s=&sections[n];
    int dx=(int)s->center_x-(int)face_x;
    int dy=(int)s->center_y-(int)face_y;
    if(dx<0)dx=-dx;
    if(dy<7 || dy>34 || dx>22 || s->area<18)continue;
    int width=(int)s->right-s->left+1;
    int height=(int)s->bottom-s->top+1;
    int score=(int)s->area*3+width*height-dx*12-dy*3;
    if(score>chest_score) { chest_score=score;chest=n+1; }
  }
  if(!chest) {
    for(unsigned n=0;n<count;++n) {
      MmxJupiterPlanetSection *s=&sections[n];
      int dx=(int)s->center_x-(int)face_x;
      int dy=(int)s->center_y-((int)face_y+17);
      if(dx<0)dx=-dx;
      if(dy<0)dy=-dy;
      int score=(int)s->area*2-dx*12-dy*4;
      if(score>chest_score) { chest_score=score;chest=n+1; }
    }
  }
  for(unsigned n=0;n<count;++n) {
    MmxJupiterPlanetSection *s=&sections[n];
    unsigned width=s->right-s->left+1,height=s->bottom-s->top+1;
    if(n+1==chest) s->flags|=MMX_JUPITER_SECTION_CHEST;
    if((unsigned)s->bottom+12>=bottom && (unsigned)s->top+22>=bottom)
      s->flags|=MMX_JUPITER_SECTION_BOOT;
    if(n+1!=chest && width>=8 && height>=6 && s->area>=28)
      s->flags|=MMX_JUPITER_SECTION_SWIRL;
  }
  *section_count=count;
}

static inline bool MmxJupiterPlanetSwirlPixel(int dx,int dy) {
  /* A compact, hard-edged spiral in a 7x5 SNES pixel-art stamp. */
  switch(dy) {
    case -2:return dx==0 || dx==1;
    case -1:return dx==-1 || dx==2;
    case 0:return dx==-2 || dx==1;
    case 1:return dx==-1 || dx==0;
    case 2:return dx==1;
    default:return false;
  }
}

/* Discrete cloud belts are evaluated in each connected plate's own bounds.
 * The deterministic seed varies both wave shape and phase between sections;
 * no camera position, interpolation or anti-aliasing enters the color path. */
static inline uint16_t MmxJupiterPlanetColor(
    uint16_t original,unsigned section_id,unsigned x,unsigned y,
    const MmxJupiterPlanetSection sections[MMX_JUPITER_MAX_SECTIONS]) {
  if(!section_id || section_id>MMX_JUPITER_MAX_SECTIONS || x>=128 || y>=128)
    return original;
  const MmxJupiterPlanetSection *s=&sections[section_id-1];
  if(x<s->left || x>s->right || y<s->top || y>s->bottom)return original;
  static const int8_t curve[16]={0,0,1,1,2,1,0,0,-1,-1,-2,-1,0,1,1,0};
  unsigned local_x=x-s->left,local_y=y-s->top;
  unsigned height=s->bottom-s->top+1;
  unsigned period=height<14?height:14;
  unsigned wave=(local_x+s->pattern_seed)%16u;
  int phase=(int)local_y+curve[wave]+(int)(s->pattern_seed%7u);
  phase%=(int)period;
  if(phase<0)phase+=(int)period;
  unsigned band=(unsigned)phase*14u/period;
  bool boot=(s->flags&MMX_JUPITER_SECTION_BOOT)!=0;
  uint16_t cloud;
  if(boot) {
    cloud=band<1?MmxFutureZeroBgr555(27,26,24):
        band<3?MmxFutureZeroBgr555(24,20,15):
        band<6?MmxFutureZeroBgr555(22,13,7):
        band<9?MmxFutureZeroBgr555(18,10,6):
               MmxFutureZeroBgr555(11,6,4);
  } else {
    cloud=band<2?MmxFutureZeroBgr555(29,28,26):
        band<5?MmxFutureZeroBgr555(26,22,17):
        band<8?MmxFutureZeroBgr555(24,16,10):
        band<10?MmxFutureZeroBgr555(20,12,7):
        band<12?MmxFutureZeroBgr555(15,9,5):
                MmxFutureZeroBgr555(26,22,17);
  }

  if(s->flags&MMX_JUPITER_SECTION_CHEST) {
    unsigned width=s->right-s->left+1;
    int rx=width>=17?4:width>=11?3:2;
    int ry=height>=12?2:1;
    int sx=(int)s->center_x+(width>=13?1:0),sy=s->center_y;
    int dx=(int)x-sx,dy=(int)y-sy;
    int ellipse=dx*dx*ry*ry+dy*dy*rx*rx;
    int limit=rx*rx*ry*ry;
    if(ellipse<=limit) {
      if(MmxJupiterPlanetSwirlPixel(dx,dy))
        return MmxFutureZeroBgr555(30,22,14); /* pale storm swirl */
      return MmxFutureZeroBgr555(28,10,4); /* Great Red Spot */
    }
    if(ellipse<=limit+rx*ry*2)
      cloud=MmxFutureZeroBgr555(25,16,10); /* storm's rust edge */
  } else if((s->flags&MMX_JUPITER_SECTION_SWIRL) &&
      MmxJupiterPlanetSwirlPixel((int)x-(int)s->center_x,
                                 (int)y-(int)s->center_y)) {
    cloud=MmxFutureZeroBgr555(30,29,27);
  }
  return cloud;
}

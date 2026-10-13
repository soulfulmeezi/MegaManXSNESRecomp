#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "mmx_zero_jupiter_shell.h"

/* Jupiter Planet Shell v17. A pixel-ART material, not another palette swap.
 * A 128x128 mask is built ONCE per Zero animation pose from its unmodified
 * indexed sprite and original palette. Preserve transparency, hair, facial
 * skin, blue visor and dark undersuit. Paint bright armor + small enclosed
 * metallic seams with the SAME Jupiter cloud texture, suppressing original
 * panel shading. No CPU movement, hitboxes or SNES ROM graphics change. */
enum { MMX_JUPITER_POSE_WIDTH=128, MMX_JUPITER_POSE_PIXELS=16384 };

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

/* Build a texture mask for ONE source pose, using its original colors.
 * 0 = preserve original, 1 = armor cloud surface, 2 = narrow internal seam.
 * Does not modify source sprite indices or original palette.
 *
 * The ponytail is located by a golden connected component above/behind
 * Zero's face, not by forbidding all gold globally (boots use warm gold).
 * Recolor isolated bright armor regions, preserving outlines by default. */
static inline void MmxJupiterPlanetMask(
    const uint8_t source[MMX_JUPITER_POSE_PIXELS],
    const uint16_t palette[128],
    uint8_t mask[MMX_JUPITER_POSE_PIXELS],
    unsigned *anchor_top, unsigned *storm_x,unsigned *storm_y) {
  uint8_t protected_hair[MMX_JUPITER_POSE_PIXELS]={0};
  uint16_t queue[MMX_JUPITER_POSE_PIXELS];
  unsigned top=128,face_x=0,face_y=0,faces=0;
  for(unsigned y=0;y<128;++y) for(unsigned x=0;x<128;++x) {
    unsigned pixel=source[y*128+x];
    if(pixel>=16 && pixel<32) {
      if(y<top) top=y;
    }
  }
  if(top==128) { /* No body in an empty animation pose. */
    for(unsigned i=0;i<MMX_JUPITER_POSE_PIXELS;++i) mask[i]=0;
    *anchor_top=0;*storm_x=64;*storm_y=64;return;
  }
  for(unsigned y=top;y<128 && y<top+40;++y) for(unsigned x=0;x<128;++x) {
    unsigned pixel=source[y*128+x];
    if(pixel>=16 && pixel<32 && MmxJovianFaceSkin(palette[pixel])) {
      face_x+=x;face_y+=y;++faces;
    }
  }
  face_x=faces?face_x/faces:64;
  face_y=faces?face_y/faces:top+18;

  /* Preserve each connected patch of golden ponytail that is near the
   * face/head. The mask follows jumping/attacking poses automatically. */
  for(unsigned y=top;y<128 && y<top+45;++y) for(unsigned x=0;x<128;++x) {
    unsigned p=y*128+x,pixel=source[p];
    if(protected_hair[p] || pixel<16 || pixel>=32 ||
        !MmxJovianGoldenHair(palette[pixel]) ||
        (int)x<(int)face_x-24 || x>face_x+24 ||
        y>face_y+2) continue;
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
  /* Colored armor becomes one painted gas material, independent of
   * whether the source rendered it white metal, gold, red or green. */
  for(unsigned i=0;i<MMX_JUPITER_POSE_PIXELS;++i) {
    unsigned pixel=source[i];
    mask[i]=(uint8_t)(MmxJupiterPlanetPaintable(pixel,palette) &&
                       !protected_hair[i]);
  }
  /* Bridge only SHORT interior metal seam fragments. Deep black stays
   * black. The mask is not expanded into large gaps between limbs or body
   * regions, and exterior silhouette boundaries are not recolored. */
  for(unsigned y=2;y<126;++y) for(unsigned x=2;x<126;++x) {
    unsigned i=y*128+x,pixel=source[i];
    if(mask[i] || pixel<16 || pixel>=32 || protected_hair[i])continue;
    uint16_t c=palette[pixel];
    unsigned hi=MmxJovianMax3(c&31,(c>>5)&31,(c>>10)&31);
    if(hi<10 || hi>19 || MmxJovianFaceSkin(c) ||
       MmxJupiterPlanetBlueFace(c)) continue;
    /* Use the ORIGINAL material mask, not a recursively growing region. */
    bool across=(mask[i-1]==1 && mask[i+1]==1) ||
        (mask[i-2]==1 && mask[i+2]==1) ||
        (mask[i-128]==1 && mask[i+128]==1) ||
        (mask[i-256]==1 && mask[i+256]==1);
    if(across)mask[i]=2;
  }
  *anchor_top=top;
  *storm_x=face_x;
  *storm_y=face_y+13<128?face_y+13:127;
}

/* Broad organic SNES Jupiter belts with a tiny rusty oval storm.
 * Pattern is entirely independent of the original metal detail pixels.
 * The texture stays in animation-local coordinates and never scrolls
 * with the game camera. Transparent pixels keep their original alpha. */
static inline uint16_t MmxJupiterPlanetColor(
    uint16_t original,unsigned mask_value,
    unsigned x,unsigned y,unsigned top,unsigned storm_x,unsigned storm_y) {
  if(!mask_value || x>=128 || y>=128) return original;
  static const int8_t drift[16]={0,0,1,1,2,2,1,0,0,-1,-1,-2,-2,-1,0,0};
  int phase=(int)y-(int)top+drift[(x/3)%16];
  if(phase<0) phase=0;
  unsigned band=(unsigned)phase%19u;
  uint16_t cloud=band<3?MmxFutureZeroBgr555(29,28,26):
      band<6?MmxFutureZeroBgr555(27,25,22):
      band<9?MmxFutureZeroBgr555(25,21,16):
      band<14?MmxFutureZeroBgr555(23,16,10):
      band<16?MmxFutureZeroBgr555(19,12,8):
              MmxFutureZeroBgr555(27,25,22);
  int dx=(int)x-(int)storm_x,dy=(int)y-(int)storm_y;
  int ellipse=9*dx*dx+25*dy*dy;
  if(ellipse<=225) cloud=MmxFutureZeroBgr555(22,13,8);
  else if(ellipse<=360) cloud=MmxFutureZeroBgr555(27,25,22);
  return cloud;
}

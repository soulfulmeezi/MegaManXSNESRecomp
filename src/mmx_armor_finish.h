#pragma once
#include "mmx_x_earth_sections.h"
#include "mmx_zero_jupiter_planet.h"

/* Discrete RGB555 finishing only. Never create pixels outside the existing
 * armor mask, move material boundaries, animate texture coordinates, or
 * repaint skin/hair/eyes/undersuit. */
static inline uint16_t MmxArmorShift(uint16_t color,int dr,int dg,int db) {
  int r=(color&31)+dr,g=((color>>5)&31)+dg,b=((color>>10)&31)+db;
  r=r<0?0:r>31?31:r;g=g<0?0:g>31?31:g;b=b<0?0:b>31?31:b;
  return (uint16_t)(r|(g<<5)|(b<<10));
}
static inline uint16_t MmxEarthFinishedPixel(uint16_t original,unsigned section,int x,int y) {
  uint16_t base=MmxEarthXArmorPixel(original,section,x,y);
  if(section>=MMX_EARTH_SECTION_COUNT || !MmxEarthIsBlueArmor(original))return base;
  char material=MmxEarthXMaterial(section,x,y);
  /* Shallow turquoise shelves touch land; deep oceans keep their dark
   * ramp. A two-channel lift keeps water blue and continents green. */
  if(material=='.') {
    char coast=MmxEarthXMaterial(section,x,y-1);
    if(coast=='G' || coast=='T')return MmxArmorShift(base,0,3,2);
  }
  if(material=='G' && MmxEarthXMaterial(section,x,y-1)=='.')
    return MmxArmorShift(base,1,2,0);
  /* Clouds cast one pixel of cool relief onto the material below them. */
  if(material!='C' && MmxEarthXMaterial(section,x,y-1)=='C')
    return MmxArmorShift(base,-1,-1,0);
  return base;
}
static inline uint16_t MmxJupiterFinishedPixel(uint16_t original,unsigned id,
    unsigned x,unsigned y,const MmxJupiterPlanetSection sections[MMX_JUPITER_MAX_SECTIONS]) {
  uint16_t base=MmxJupiterPlanetColor(original,id,x,y,sections);
  if(!id || id>MMX_JUPITER_MAX_SECTIONS || x>=128 || y>=128)return base;
  const MmxJupiterPlanetSection *s=&sections[id-1];
  if(x<s->left || x>s->right || y<s->top || y>s->bottom)return base;
  unsigned w=s->right-s->left+1,h=s->bottom-s->top+1;
  if(w<5 || h<5)return base; /* Don't crush tiny seam details. */
  unsigned lx=x-s->left,ly=y-s->top;
  /* Keep the Great Red Spot's authored center/swirl readable. Broad curved
   * edge lighting gives each plate its own volume, not a global gold wash. */
  if(ly==0 && lx>0 && lx+1<w)return MmxArmorShift(base,2,2,2);
  if(lx==0 && ly<h/2)return MmxArmorShift(base,1,1,2);
  if(ly+1==h || (lx+1==w && ly>=h/2))return MmxArmorShift(base,-3,-2,-1);
  return base;
}

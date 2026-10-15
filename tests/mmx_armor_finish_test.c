#include "mmx_armor_finish.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  const uint16_t protected_colors[]={0,0x1084,MmxEarthRgb(24,16,11),MmxEarthRgb(27,27,29)};
  unsigned changed=0;
  for(unsigned s=0;s<MMX_EARTH_SECTION_COUNT;s++)for(int y=-30;y<20;y++)for(int x=-15;x<16;x++) {
    for(unsigned c=0;c<4;c++)assert(MmxEarthFinishedPixel(protected_colors[c],s,x,y)==protected_colors[c]);
    uint16_t original=MmxEarthRgb(4,10,27);
    uint16_t a=MmxEarthXArmorPixel(original,s,x,y),b=MmxEarthFinishedPixel(original,s,x,y);
    changed+=a!=b;assert(b<=0x7fff);
    char m=MmxEarthXMaterial(s,x,y);
    if(m=='.')assert((b>>10)>((b>>5)&31));
    if(m=='G')assert(((b>>5)&31)>(b&31));
  }
  assert(changed>100);
  MmxJupiterPlanetSection sections[MMX_JUPITER_MAX_SECTIONS]={{20,20,39,39,30,30,3,MMX_JUPITER_SECTION_CHEST,400}};
  for(unsigned y=0;y<60;y++)for(unsigned x=0;x<60;x++) {
    assert(MmxJupiterFinishedPixel(1234,0,x,y,sections)==1234);
    uint16_t a=MmxJupiterPlanetColor(1234,1,x,y,sections);
    uint16_t b=MmxJupiterFinishedPixel(1234,1,x,y,sections);
    assert(b<=0x7fff);
    if(x>20 && x<39 && y>20 && y<39)assert(a==b); /* Belts and storm unchanged. */
    if(x<20 || x>39 || y<20 || y>39)assert(b==1234);
  }
  assert(MmxJupiterFinishedPixel(0,1,25,20,sections)!=MmxJupiterPlanetColor(0,1,25,20,sections));
  puts("armor finish: protected materials, coast colors, plate edges and storm identity passed");
}

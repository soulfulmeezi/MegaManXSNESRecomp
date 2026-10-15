#include <assert.h>
#include "mmx_near_road_debug.h"
static void near_road_cases(void) {
 assert(MmxNearRoadScene(0,4,0x500,2));assert(MmxNearRoadScene(0,4,0x1500,2));
 assert(!MmxNearRoadScene(0,4,0x4ff,2));assert(!MmxNearRoadScene(0,4,0x1501,2));
 assert(!MmxNearRoadScene(1,4,0x600,2));assert(!MmxNearRoadScene(0,3,0x600,2));
 assert(!MmxNearRoadScene(0,4,0x600,12));
 assert(MmxNearRoadPixel(1454,468,0x118e,0x8044)==0x03ff);
 assert(MmxNearRoadPixel(1454,477,0x116a,0x8044)==0x7fe0);
 assert(MmxNearRoadPixel(1454,468,0x518e,0x8044)==0x03ff);
 assert(MmxNearRoadPixel(1454,468,0x118e,0)==-1);
 assert(MmxNearRoadPixel(1454,469,0x118e,0x8044)==-1);
 const unsigned blockers[]={0x898,0x897,0x4897,0x852,0x896,0x899,0x16dc,0x96d9,0xa6b,0x804};
 for(unsigned i=0;i<sizeof(blockers)/sizeof(blockers[0]);i++) {
  assert(MmxNearRoadPixel(1454,468,(uint16_t)blockers[i],0x8044)==-1);
  assert(MmxNearRoadPixel(1454,477,(uint16_t)blockers[i],0x8044)==-1);
 }
 for(int x=0x5ff;x<=0x601;x++)assert(MmxNearRoadY(0,x)==468);
 for(int x=0xfff;x<=0x1001;x++)assert(MmxNearRoadY(1,x)==477);
 for(unsigned lane=2;lane<4;lane++)for(int x=0x688;x<=0x6c7;x++) {
  assert(MmxNearRoadY(lane,x)==MmxNearRoadY(lane,x+0xa00));
  if(x>0x688){int delta=MmxNearRoadY(lane,x)-MmxNearRoadY(lane,x-1);assert(delta>=0 && delta<=1);}
 }
 assert(MmxNearRoadPixel(0x688,468,0x1179,0x8044)==0x7c1f);
 assert(MmxNearRoadPixel(0x688,477,0x116b,0x8044)==0x03e0);
 assert(MmxNearRoadY(2,0x687)==-1);assert(MmxNearRoadY(2,0x6c8)==-1);
}

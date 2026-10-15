#include "mmx_near_road_cars.h"
#include <assert.h>
#include <stdio.h>
static void test_visibility(void) {
  for(unsigned lane=0;lane<2;++lane)
    for(unsigned slot=0;slot<MMX_NEAR_CAR_SLOTS;++slot)
      for(unsigned tick=0;tick<5500;tick+=73) {
        MmxNearCar car=MmxNearCarAt(lane,slot,tick);
        unsigned tile=lane?0x116a:0x118e;
        int width=MmxNearCarWidth(car.model);
        for(int y=462;y<=481;++y)
          for(int x=car.x-width/2-2;x<car.x+width/2+4;++x) {
            if(!MmxNearCarMayDraw(&car,x,y,1)) {
              assert(MmxNearCarRoadPixel(&car,x,y,(uint16_t)tile,0x8044)==-1);
              assert(MmxNearCarHaloPixel(&car,x,y,(uint16_t)tile,0x8044)==-1);
            }
            if(!MmxNearCarMayDraw(&car,x,y,0))
              assert(MmxNearCarRoadPixel(&car,x,y,(uint16_t)tile,0x8044)==-1);
            assert(MmxNearCarRoadPixel(&car,x,y,0x0898,0x8044)==-1);
          }
        assert(MmxNearCarAt(lane,slot,tick).x==car.x);
        assert(car.phase<4 && car.palette<5 && car.model<3);
      }
}
int main(void) {test_visibility();puts("Near-road raster cull tests passed");return 0;}

#ifndef MMX_NEAR_ROAD_DEBUG_H
#define MMX_NEAR_ROAD_DEBUG_H
#include <stdint.h>
/* v33g: grey near-road geometry from v33f1 BG1 tile captures.
 * Full BG1 coordinates, including Y=256 camera origin. No palette tests.
 * A/B are flat road centers; C/D are the two measured descending bends.
 * Green perspective ramps are a different tile family and are excluded. */
typedef struct MmxNearRoadKnot {int x,y;} MmxNearRoadKnot;
static const MmxNearRoadKnot mmx_near_road_bend[]={
 {0,0},{8,2},{16,4},{24,7},{32,11},{40,16},{48,22},{56,29},{63,36}};
static const int mmx_near_road_curve_origins[]={0x0688,0x1088};
static const uint16_t mmx_near_road_colors[]={0x03ff,0x7fe0,0x7c1f,0x03e0};
static inline int MmxNearRoadScene(unsigned stage,unsigned phase,unsigned camera,unsigned mode) {
 return stage==0 && phase==4 && mode==2 && camera>=0x0500 && camera<=0x1500;
}
static inline int MmxNearRoadY(unsigned lane,int wx) {
 if(lane>3 || wx<0x0500 || wx>0x1600)return -1;
 int base=(lane&1)?477:468;
 if(lane<2)return base;
 for(unsigned c=0;c<2;c++) {
  int dx=wx-mmx_near_road_curve_origins[c];
  if(dx<0 || dx>63)continue;
  for(unsigned i=1;i<sizeof(mmx_near_road_bend)/sizeof(mmx_near_road_bend[0]);i++) {
   MmxNearRoadKnot a=mmx_near_road_bend[i-1],b=mmx_near_road_bend[i];
   if(dx<=b.x)return base+a.y+((dx-a.x)*(b.y-a.y)+(b.x-a.x)/2)/(b.x-a.x);
  }
 }
 return -1;
}
static inline int MmxNearRoadTile(unsigned lane,uint16_t entry) {
 /* Keep tile number AND palette bank, discard only priority/flip flags.
  * Exact native words observed on the road, not color-based filtering. */
 unsigned key=entry&0x1fff;
 if(lane==0)return key==0x118e;
 if(lane==1)return key==0x116a;
 if(lane>3)return 0;
 return key==0x116b || key==0x116c ||
        (key>=0x1179 && key<=0x1185);
}
static inline int MmxNearRoadPixel(int wx,int wy,uint16_t tile,unsigned bg1_pixel) {
 if(!bg1_pixel)return -1;
 for(int lane=3;lane>=0;lane--) {
  int ly=MmxNearRoadY((unsigned)lane,wx);
  if(ly>=0 && wy==ly && MmxNearRoadTile((unsigned)lane,tile))
   return mmx_near_road_colors[lane];
 }
 return -1;
}
#endif

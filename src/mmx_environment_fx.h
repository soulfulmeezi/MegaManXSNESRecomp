#ifndef MMX_ENVIRONMENT_FX_H
#define MMX_ENVIRONMENT_FX_H
#include <stdint.h>

/* CosmicFX v1: deterministic, depth-separated 16-bit-style cloud motion.
 * The SNES/guest PPU remains untouched. This is a pure host-renderer BG2
 * color layer, applied only to verified blue-sky colors behind native BG1.
 * Tick is a saved simulation tick, never a wall-clock timestamp.
 * No asset ownership, collision, AI, gameplay timing or save layout changes.
 */
typedef struct MmxFxCloudPlane {
  int period, base_y, camera_div, wind_div, cloud_x;
  int radius_x, radius_y;
  unsigned opacity;
} MmxFxCloudPlane;

static inline int MmxFxPositiveMod(int v, int m) {
  int n = v % m;
  return n < 0 ? n + m : n;
}
static inline uint16_t MmxFxRgb(unsigned r,unsigned g,unsigned b) {
  return (uint16_t)(r | (g<<5) | (b<<10));
}
static inline uint16_t MmxFxLerpRgb(uint16_t source,uint16_t tint,unsigned weight) {
  if(weight > 16) weight=16;
  unsigned a[3] = {source&31u,(source>>5)&31u,(source>>10)&31u};
  unsigned b[3] = {tint&31u,(tint>>5)&31u,(tint>>10)&31u};
  return MmxFxRgb((a[0]*(16-weight)+b[0]*weight+8)/16,
                  (a[1]*(16-weight)+b[1]*weight+8)/16,
                  (a[2]*(16-weight)+b[2]*weight+8)/16);
}
static inline int MmxFxBlueSky(uint16_t color) {
  unsigned r=color&31u,g=(color>>5)&31u,b=(color>>10)&31u;
  /* Reject machinery, roads, stars, windows, UI, dark interiors and water.
   * Even in a suitable stage, sky tint only appears on airy blue pixels. */
  return b>=17 && g>=13 && r>=6 && r<=23 && b>r+3 && g>r+2;
}
static inline int MmxFxPuff(int dx,int dy,int rx,int ry) {
  if(dx<-rx || dx>rx || dy<-ry || dy>ry) return 0;
  /* Avoid floating point / sqrt per pixel. */
  int x=dx*ry,y=dy*rx;
  return x*x+y*y <= rx*rx*ry*ry;
}
static inline int MmxFxCloudCoverage(int x,int y,int camera_x,
                                    unsigned tick,unsigned plane) {
  static const MmxFxCloudPlane clouds[3] = {
    {233, 38, 12, 20,  33, 24, 7,  7}, /* distant wisps: slow camera/wind */
    {197, 75,  6, 11,  82, 34, 9,  9}, /* midrange cloud bank */
    {271,119,  3,  7, 119, 46,12, 11}  /* larger foreground clouds */
  };
  const MmxFxCloudPlane *p=&clouds[plane];
  int local=MmxFxPositiveMod(x+camera_x/p->camera_div+
                            (int)(tick/p->wind_div),p->period);
  /* Three offset puffs create an irregular silhouette, rather than a flat
   * sine-wave band. Matching positions on different planes never line up. */
  static const int shift_x[3]={-21,7,28};
  static const int shift_y[3]={3,-3,2};
  int coverage=0;
  for(unsigned i=0;i<3;++i) {
    int cx=p->cloud_x+shift_x[i],cy=p->base_y+shift_y[i];
    int dx=local-cx,dy=y-cy,rx=p->radius_x+(i==1?8:-3);
    if(dx>p->period/2) dx-=p->period;
    if(dx<-p->period/2) dx+=p->period;
    if(MmxFxPuff(dx,dy,rx,p->radius_y+(i==1?2:0))) {
      int weight=(int)p->opacity;
      if(dy>=p->radius_y-2) weight-=3; /* soft lower shadow rim */
      if(weight>coverage) coverage=weight;
    }
  }
  return coverage;
}
/* -1 = no replacement. The native BG2 depth, palette fade, PPU windows,
 * actor priorities and color math remain controlled by mmx_renderer.c. */
static inline int MmxFxAtmospherePixel(uint16_t source,int x,int y,
                                       int camera_x,unsigned tick) {
  if(y<16 || y>=152 || !MmxFxBlueSky(source)) return -1;
  int selected=-1;
  for(unsigned layer=0;layer<3;++layer) {
    int cover=MmxFxCloudCoverage(x,y,camera_x,tick,layer);
    if(cover>0) {
      static const uint16_t tint[3]={
        (21u|(26u<<5)|(30u<<10)),
        (26u|(29u<<5)|(31u<<10)),
        (29u|(30u<<5)|(31u<<10))
      };
      selected=(int)MmxFxLerpRgb(source,tint[layer],(unsigned)cover);
    }
  }
  return selected;
}
#endif /* MMX_ENVIRONMENT_FX_H */

#ifndef MMX_NEAR_ROAD_CARS_H
#define MMX_NEAR_ROAD_CARS_H
#include <stdint.h>
#include "mmx_near_road_debug.h"
/* Original compact road vehicles: panel shading, framed cabins and animated
 * wheel hubs. Existing lane geometry and native-road occlusion are retained. */
enum {
 MMX_NEAR_CAR_SLOTS=30, MMX_NEAR_CAR_LANES=2,
 MMX_NEAR_CAR_MAX=MMX_NEAR_CAR_SLOTS*MMX_NEAR_CAR_LANES,
 MMX_NEAR_CAR_START=0x0500, MMX_NEAR_CAR_SPAN=0x1100,
 MMX_NEAR_CAR_SEDAN_W=12, MMX_NEAR_CAR_TRUCK_W=15,
 MMX_NEAR_CAR_SPORTS_W=14, MMX_NEAR_CAR_W_MAX=15,
 MMX_NEAR_CAR_H=8, MMX_NEAR_CAR_MODELS=3,
 MMX_NEAR_CAR_PALETTES=5, MMX_NEAR_CAR_FRAMES=4
};
/* Color indices 0 transparent, 1..6 common, 7..12 model palette:
 * 1 outline, 2 window, 3 window shine, 4 headlight, 5 taillight,
 * 6 dim taillight, 7 body shadow, 8 body, 9 highlight,
 * 10 tire, 11 wheel hub, 12 metal trim (fixed colors in MmxNearCarColor). */
static const uint16_t mmx_tier1_palette_rgb555[5][13]={
  { 0x0000, 0x2082, 0x51C7, 0x6ACD, 0x7FDC, 0x251F, 0x1893, 0x3D04, 0x61E7, 0x7B0C, 0x4582, 0x6EC3, 0x7FCA }, /* Blue_Cyan */
  { 0x0000, 0x2082, 0x51C7, 0x6ACD, 0x7FDC, 0x251F, 0x1893, 0x1C8C, 0x18F8, 0x1DFF, 0x08AC, 0x09D9, 0x231F }, /* Red_Orange */
  { 0x0000, 0x2082, 0x51C7, 0x6ACD, 0x7FDC, 0x251F, 0x1893, 0x40AB, 0x5D34, 0x79FC, 0x286B, 0x6D39, 0x7EBF }, /* Purple_Magenta */
  { 0x0000, 0x2082, 0x51C7, 0x6ACD, 0x7FDC, 0x251F, 0x1893, 0x2943, 0x4265, 0x5B6B, 0x2941, 0x5703, 0x77EE }, /* Green_Teal */
  { 0x0000, 0x2082, 0x51C7, 0x6ACD, 0x7FDC, 0x251F, 0x1893, 0x4DEC, 0x7356, 0x7FFD, 0x3900, 0x7EC4, 0x7FF1 }, /* White_Blue */
};
/* Facing right; 0 transparent, A tire, B hub, C metal bumper/trim.
 * Original hand-authored pixels inspired by the highway's mechanical shapes. */
static const char mmx_road_detail[3][8][16]={
 {"000019991000", "000123221000", "001922128910", "158888188884",
  "1877881787C1", "11AA1111AA11", "01ABA001ABA0", "001A10001A10"},
 {"019999991999100", "018888811232100", "018787811222910", "018888818888884",
  "1577777178777C1", "11AA1AA1111AA11", "01ABABA1001ABA0", "001A1A10001A100"},
 {"00000199910000", "09901232291000", "01899221289910", "15888818888884",
  "187778177887C1", "11AA111111AA11", "01ABA00001ABA0", "001A1000001A10"}
};
typedef struct MmxNearCar {
 int x;unsigned lane,model,palette,phase;
} MmxNearCar;
static inline int MmxNearCarWidth(unsigned model) {
 static const unsigned w[3]={MMX_NEAR_CAR_SEDAN_W,MMX_NEAR_CAR_TRUCK_W,MMX_NEAR_CAR_SPORTS_W};
 return model<MMX_NEAR_CAR_MODELS?(int)w[model]:0;
}
static inline int MmxNearCarLaneTop(unsigned lane) {return lane?472:464;}
/* Deterministic lane-relative world motion: upper A => +X; lower B => -X.
 * A speeds 2 or 1.333 px/frame; B speeds 1.333 or 1 px/frame.
 * Anchor and mask always use full world coordinates independent of camera. */
static inline MmxNearCar MmxNearCarAt(unsigned lane,unsigned slot,unsigned tick) {
 MmxNearCar c;
 c.lane=lane&1u;
 c.model=(slot+c.lane)%MMX_NEAR_CAR_MODELS; /* transport reserved */
 c.palette=(slot+2*c.lane)%MMX_NEAR_CAR_PALETTES;
 c.phase=(tick/5+slot+2*c.lane)%MMX_NEAR_CAR_FRAMES;
 unsigned seed=(slot%MMX_NEAR_CAR_SLOTS)*MMX_NEAR_CAR_SPAN/MMX_NEAR_CAR_SLOTS;
 unsigned divisor=(c.lane?3u:2u)+((slot+c.lane)&1u);
 unsigned progress=(seed+64+(tick*4)/divisor)%MMX_NEAR_CAR_SPAN;
 c.x=MMX_NEAR_CAR_START+(int)(c.lane?MMX_NEAR_CAR_SPAN-1u-progress:progress);
 return c;
}
/* Returns 0 transparent or 1..12 indexed color.
 * px/py are the absolute full BG1 world-space pixel coordinates. */
static inline unsigned MmxNearCarIndex(const MmxNearCar *c,int wx,int wy) {
 if(c->model>=MMX_NEAR_CAR_MODELS || c->lane>=MMX_NEAR_CAR_LANES ||
    c->palette>=MMX_NEAR_CAR_PALETTES || c->phase>=MMX_NEAR_CAR_FRAMES)return 0;
 int width=MmxNearCarWidth(c->model);
 int ix=wx-(c->x-width/2),iy=wy-MmxNearCarLaneTop(c->lane);
 if(ix<0 || ix>=width || iy<0 || iy>=MMX_NEAR_CAR_H)return 0;
 if(c->lane==1)ix=width-1-ix;
 char symbol=mmx_road_detail[c->model][iy][ix];
 unsigned v=symbol>='A'?(unsigned)(symbol-'A'+10):(unsigned)(symbol-'0');
 /* Subtle two-frame wheel rotation; no flickering body or glow strip. */
 if(v==11 && (c->phase&1))v=12;

  return v;
}
static inline int MmxNearCarColor(const MmxNearCar *c,int wx,int wy) {
 unsigned pixel=MmxNearCarIndex(c,wx,wy);
 if(pixel==10)return 0x0c63; /* rubber */
 if(pixel==11)return 0x39ce; /* steel hub */
 if(pixel==12)return 0x56b5; /* bright metal */
 return pixel?mmx_tier1_palette_rgb555[c->palette][pixel]:-1;
}
/* Preserve v33h native BG1 per-pixel occlusion. Extra Y clamp ensures that
 * no sprite paint escapes lane A 464..471 or B 472..479 on road-edge pixels.
 * All 8px-high sprite cells fit; no 18px transport in this build. */
static inline int MmxNearCarRoadPixel(const MmxNearCar *c,int wx,int wy,
                                     uint16_t native_tile,unsigned native_bg1_pixel){
 if(c->lane>=MMX_NEAR_CAR_LANES || wy<MmxNearCarLaneTop(c->lane) ||
    wy>=MmxNearCarLaneTop(c->lane)+MMX_NEAR_CAR_H ||
    !native_bg1_pixel || !MmxNearRoadTile(c->lane,native_tile))return -1;
 return MmxNearCarColor(c,wx,wy);
}
/* Wheeled traffic has no thruster halo. Keep the renderer interface inert. */
static inline int MmxNearCarHaloPixel(const MmxNearCar *c,int wx,int wy,
                                     uint16_t native_tile,unsigned native_bg1_pixel){
 (void)c;(void)wx;(void)wy;(void)native_tile;(void)native_bg1_pixel;
 return -1;
}
#endif

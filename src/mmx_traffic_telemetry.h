#ifndef MMX_TRAFFIC_TELEMETRY_H
#define MMX_TRAFFIC_TELEMETRY_H
/* Small, opt-in NATIVE-PIXEL raster readout for stage A road debugging.
 * Pure host presentation: no SNES RAM, OAM, PPU, gameplay, or lane mutations. */
#include <stdint.h>
#include <stdio.h>

typedef struct MmxTrafficTelemetry {
  unsigned stage,phase,mode,gate;
  unsigned camera_x,camera_y;
  unsigned bg2_stream_x,bg2_stream_y;
  unsigned ppu_bg2_x,ppu_bg2_y;
  unsigned bg2_world_x,bg2_world_y;
} MmxTrafficTelemetry;

/* 3x5 native pixels per hex numeral/label. Never smooth, scale, or filter. */
static inline unsigned MmxTrafficTelemetryGlyph(char c, unsigned row) {
  static const char alphabet[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ: -";
  static const uint8_t pixels[][5]={
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7},
    {5,5,7,1,1}, {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1},
    {7,5,7,5,7}, {7,5,7,1,7},
    {7,5,7,5,5}, {6,5,6,5,6}, {7,4,4,4,7}, {6,5,5,5,6},
    {7,4,7,4,7}, {7,4,7,4,4}, {7,4,5,5,7}, {5,5,7,5,5},
    {7,2,2,2,7}, {1,1,1,5,7}, {5,5,6,5,5}, {4,4,4,4,7},
    {5,7,7,5,5}, {5,7,7,7,5}, {7,5,5,5,7}, {7,5,7,4,4},
    {7,5,5,7,3}, {7,5,7,6,5}, {7,4,7,1,7}, {7,2,2,2,2},
    {5,5,5,5,7}, {5,5,5,5,2}, {5,5,7,7,5}, {5,5,2,5,5},
    {5,5,2,2,2}, {7,1,2,4,7},
    {0,2,0,2,0}, {0,0,0,0,0}, {0,0,7,0,0}
  };
  if(row>=5) return 0;
  for(unsigned i=0; i<sizeof(alphabet)-1; ++i)
    if(alphabet[i]==c) return pixels[i][row];
  return 0;
}

static inline void MmxTrafficTelemetryText(char out[5][40],const MmxTrafficTelemetry *t) {
  snprintf(out[0],40,"ST:%02X PH:%02X MD:%02X G:%u",t->stage&255,t->phase&255,t->mode&255,t->gate?1u:0u);
  snprintf(out[1],40,"CAM X:%04X Y:%04X",t->camera_x&65535,t->camera_y&65535);
  snprintf(out[2],40,"BG2 RAM:%04X %04X",t->bg2_stream_x&65535,t->bg2_stream_y&65535);
  snprintf(out[3],40,"PPU BG2:%04X %04X",t->ppu_bg2_x&65535,t->ppu_bg2_y&65535);
  snprintf(out[4],40,"BG2 WLD:%04X %04X",t->bg2_world_x&65535,t->bg2_world_y&65535);
}

static inline void MmxTrafficTelemetryDraw(uint32_t *out,unsigned width,
                                             const MmxTrafficTelemetry *t) {
  if(!out || width<112 || !t) return;
  char lines[5][40]; MmxTrafficTelemetryText(lines,t);
  const unsigned x0=width-112,y0=2;
  for(unsigned y=0;y<39;++y) for(unsigned x=0;x<110;++x) {
    uint32_t color = (x==0 || x==109 || y==0 || y==38)?0x19647c:0x071521;
    out[(y+y0)*width+x+x0]=color;
  }
  for(unsigned row=0;row<5;++row) {
    const char *s=lines[row];
    for(unsigned ci=0;s[ci] && ci<26;++ci) {
      for(unsigned gy=0;gy<5;++gy) {
        unsigned bits=MmxTrafficTelemetryGlyph(s[ci],gy);
        for(unsigned gx=0;gx<3;++gx) if(bits & (1u<<(2-gx)))
          out[(y0+3+row*7+gy)*width+x0+4+ci*4+gx]=
               row==0?(t->gate?0x9bf5a6:0xffb979):0xb9ecff;
      }
    }
  }
}
#endif

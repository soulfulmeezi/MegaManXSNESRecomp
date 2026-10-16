#ifndef MMX_HOVER_TRAFFIC_H
#define MMX_HOVER_TRAFFIC_H
#include <stdint.h>
#include "mmx_cosmic.h"
#include "mmx_near_road_debug.h"
/* Intro highway world coordinates, traced against the unmodified BG1 map.
 * Anchors never depend on fragmented visible road spans or on the camera. */
typedef struct MmxHoverLane { int fx,fy,nx,ny; } MmxHoverLane;
static const MmxHoverLane mmx_hover_lanes[] = {
  {984,398,900,526}, {1003,398,963,526},
  {1045,398,1109,526}, {1080,398,1304,526},
  {1873,411,1777,526}, {1893,411,1843,526},
  {1942,411,2001,526}, {1978,411,2192,526}
};
/* v25: Keep the exact eight v24 lanes as the two reference corridors.
 * Additional entrance pairs use their same measured near/far geometry in
 * later intro-highway world sections. The native road-material mask in the
 * renderer hides cars behind asphalt edges, supports and foreground tiles.
 * These added world placements need an in-game visual road-map review. */
typedef struct MmxHoverSection { int world_shift; unsigned reference; } MmxHoverSection;
static const MmxHoverSection mmx_hover_sections[] = {
  {0,0}, {0,1},             /* original central corridors */
  {-896,0}, {-896,1},       /* stage opening / early back lanes */
  {896,0}, {896,1},         /* first extended roadway set */
  {1792,0}, {1792,1},       /* mid-stage repeated road group */
  {2688,0}, {2688,1},       /* under-bridge middle stretches */
  {3584,0}, {3584,1},       /* later road sections */
  {4480,0}, {4480,1}        /* far-late roadway / exit stretch */
};
enum {
  MMX_HOVER_BASE_LANES=8, MMX_HOVER_SECTION_LANES=4,
  MMX_HOVER_LANES=sizeof(mmx_hover_sections)/sizeof(mmx_hover_sections[0])*MMX_HOVER_SECTION_LANES,
  MMX_HOVER_PER_LANE=3, MMX_HOVER_CYCLE=1152,
  MMX_HOVER_MAX_VISIBLE=96
};
static inline MmxHoverLane MmxHoverLaneAt(unsigned lane) {
  lane%=MMX_HOVER_LANES;
  const MmxHoverSection *section=&mmx_hover_sections[lane/MMX_HOVER_SECTION_LANES];
  MmxHoverLane road=mmx_hover_lanes[section->reference*MMX_HOVER_SECTION_LANES+
                                   lane%MMX_HOVER_SECTION_LANES];
  road.fx+=section->world_shift;road.nx+=section->world_shift;
  return road;
}
typedef struct MmxHoverCar { int x,y; unsigned depth,model,direction,phase,state; } MmxHoverCar;
static inline MmxHoverCar MmxHoverPosition(unsigned lane,unsigned slot,unsigned tick) {
  MmxHoverLane lane_path=MmxHoverLaneAt(lane);
  const MmxHoverLane *p=&lane_path;
  unsigned t=(tick*12+lane*137+slot*(MMX_HOVER_CYCLE/MMX_HOVER_PER_LANE))%MMX_HOVER_CYCLE;
  MmxHoverCar c;
  c.x=p->fx+(p->nx-p->fx)*(int)t/MMX_HOVER_CYCLE;
  c.y=p->fy+(p->ny-p->fy)*(int)t/MMX_HOVER_CYCLE;
  c.depth=t; c.model=(lane+slot)%4;
  int dx=p->nx-p->fx,dy=p->ny-p->fy;
  c.direction=dx<0?(dx*2 < -dy?0:1):dx==0?2:dx>dy?4:3;
  c.phase=(tick/4+lane+slot*3)&7;
  c.state=t<96?0:t>1056?2:1; /* entering, cruise, exiting */
  return c;
}
/* Four models x five headings (two left, straight, two right).
 * Each is a separate hard-edged 25x25 sprite.
 * h/t are neon lamps, e is plasma; r is lit top, s is shaded side. */
static const char mmx_hover_sprites[20][25][26] = {
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "..........o.oe...........",
    "..........oteo...........",
    ".........osssbooee.......",
    ".........osbbbbbb........",
    "........osbrrbbbsoo......",
    ".......ossbrrrrbbto......",
    ".......osbbrrrrbsoo......",
    ".......rbbgbrrbsso.......",
    ".......orgggbbbso........",
    ".......obggggbsoo........",
    "........hbgbbbso.........",
    "........rbrbrro..........",
    ".......oorrho............",
    ".........o.o.............",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "...........e.............",
    ".........ooeo..e.........",
    "........ossbboobe........",
    "........osbbbbbso........",
    "........osbrrbbbto.......",
    ".......osbbrrrrbso.......",
    ".......osbrrrrbsso.......",
    ".......osbbbrrbso........",
    "......orbbggbbbso........",
    ".......orggggbsso........",
    "........obgggbro.........",
    ".........hbbbroo.........",
    "........orrrhho..........",
    ".........oooo............",
    "...........o.............",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........e...e...........",
    ".........e...e...........",
    ".......otsbbbbsto........",
    ".......otsbbbbsto........",
    ".......osbrrrrbso........",
    ".......osbrrrrbso........",
    ".......osbrrrrbso........",
    ".......osbrrrrbso........",
    ".......osbggggbso........",
    ".......osbggggbso........",
    "........orbggbro.........",
    "........orbggbro.........",
    "..........orrro..........",
    "..........orrro..........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "..........eeoo...........",
    ".........oobsto..........",
    ".......eobbbbsoo.........",
    "......osbbbbrbso.........",
    ".....ottbbrrrbsoo........",
    "......osbrrrrrbso........",
    "......ossbrrrbbsoo.......",
    ".......osbrbbggbr........",
    ".......ossbggggro........",
    "........osbggggrho.......",
    "........orrrbbbro........",
    "..........oohbrro........",
    ".............ooo.........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "..........o..............",
    "........eotoo............",
    ".......ebsbssoo..........",
    ".......obbbbssoo.........",
    ".......obbrrbbsso........",
    ".....eobbbrrrbbbro.......",
    "......esbrrrrbggro.......",
    ".....otbbrrrbggbbho......",
    "......ossbbbggggbro......",
    ".......oossbbgbbbro......",
    "........oossbbbhro.......",
    "..........ooroo.o........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "..........oooe...........",
    "..........ossoo..........",
    ".........ossbbboo.e......",
    ".........osrrbbboe.......",
    "........osrbrrbbsto......",
    ".......ossrrrbrrbso......",
    ".......osrbrrrrbsoo......",
    "......osbbrrbrrsso.......",
    "......rrbbbrrrbso........",
    "......obgggbbrsoo........",
    "......orhgggbbso.........",
    ".......rrrggbso..........",
    "......oorbbhroo..........",
    "........oobboo...........",
    "...........o.............",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "...........e.............",
    "..........oe.............",
    ".........otsooo.e........",
    "........osbbbbboeo.......",
    "........osrrbbbsto.......",
    "........osrrrrrbso.......",
    ".......osrrbrbrrso.......",
    ".......osrbrrrbsso.......",
    ".......osrrrbrrso........",
    "......osbbbrrrbso........",
    "......osbggbbbsso........",
    ".......rbggggbso.........",
    ".......orhgggbso.........",
    ".......orrrbhbro.........",
    "........oobrbo...........",
    "..........oo.o...........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........e....e..........",
    "........oeooooeo.........",
    ".......otsbbbbsto........",
    ".......osbbbbbbso........",
    ".......osrrrrrrso........",
    ".......osrbrbrbso........",
    ".......osrrrrrrso........",
    ".......osrbrbrbso........",
    ".......osrrrrrrso........",
    ".......osbbbbbbso........",
    ".......osbggggbso........",
    ".......orbggggbro........",
    "........orhrrbho.........",
    ".........orrbrbo.........",
    "..........oooo...........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "...........eo............",
    "..........oeto...........",
    "......e.oobbbso..........",
    "......oebbbbbsoo.........",
    "......tsbbbrrbso.........",
    ".....otsbrrbrrsoo........",
    "......osrbrrrrbso........",
    "......ossrrrbrrsoo.......",
    ".......osrbrrrbbso.......",
    ".......ossrrbbgbro.......",
    "........osbbggggbo.......",
    "........ossbgggbho.......",
    ".........orbgrrbr........",
    ".........ooorrroo........",
    "............ooo..........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "........o.o..............",
    ".......eetsoo............",
    ".......obsbssoo..........",
    ".......obbrbssoo.........",
    "......obbbrrrbssoo.......",
    "....eeobbrbrrrrbsr.......",
    ".....osbrrrrbrbgbroo.....",
    ".....otbrbrrrrbgghb......",
    ".....oossrrbrbgggbr......",
    ".......oossrrbggrbo......",
    "........oossbbghrro......",
    "..........oossbrro.......",
    "............ooo.o........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "..........o.e............",
    "..........oe.............",
    "..........ttoo...........",
    "..........osbboo.........",
    "........ossbbbbooee......",
    ".......ossbbrbbboeo......",
    ".......orbgrrrbstbo......",
    ".......orbgggrsoo.o......",
    "........obggggbs.........",
    "........hbgggbsoo........",
    "........ohbbrbro.........",
    "........ohobho...........",
    ".........o...............",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........oe..............",
    "........otooo............",
    ".........oobboooe........",
    ".........osbbbboeo.......",
    ".......osbsbrbbstbo......",
    ".......osbrrrbsoo........",
    ".......orbgggrbs.........",
    ".......orbggggbso........",
    "........obgggbrso........",
    ".........hbbbroo.........",
    ".........ohhbho..........",
    ".........oo..............",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "........e......e.........",
    "........e......e.........",
    ".......otobbbbotbo.......",
    ".......otobbbbotbo.......",
    "........osbrrbso.........",
    "........osbrrbso.........",
    ".......osbggggbso........",
    ".......osbggggbso........",
    "........orbggbro.........",
    "........orbggbro.........",
    "..........ohho...........",
    "..........ohho...........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "............eo.o.........",
    "...........ooebo.........",
    ".........oooot...........",
    "......eooobbbso..........",
    ".....oeeobbbbssoo........",
    "......otsbrrrrbso........",
    ".......oosrrggbro........",
    "........sbrggggro........",
    ".......ossbgggbho........",
    "........orbbgbbo.........",
    "........oooohhho.........",
    "............ohoo.........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "..........ooo............",
    ".........eeb.............",
    ".........oooo.o..........",
    "........ooososoo.........",
    "........obbbsbsr.........",
    ".......obbbrrgbroo.......",
    ".......obbrrgggbh........",
    ".....eeosbbrgggbb........",
    "......otosbggggbh........",
    ".........osbbbhhho.......",
    ".........osrrrhoo........",
    "..........ooo............",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "...........oee...........",
    "..........ooeo...........",
    "..........ossoo..........",
    ".........ossbbboo........",
    ".........osrrbbbooe......",
    "........osrrrbbbsteo.....",
    ".......ossrrbbrrbso......",
    ".......osrrbbbrrsoo......",
    "......osrrrbbrrsso.......",
    "......rrbrbbrrrso........",
    "......obgbbbrrsoo........",
    "......orhggbbrso.........",
    ".......rrgggbso..........",
    "......oorrbhroo..........",
    "........oooooo...........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "..........e..............",
    ".........oeo.............",
    ".........otsooo..e.......",
    "........osbbbbbooe.......",
    "........osrrbbbstoo......",
    "........osrrbbrbso.......",
    ".......osrrrbbrrso.......",
    ".......osrrbbrrsso.......",
    ".......osrrbbrrso........",
    "......osrrrbbrrso........",
    "......osbbbbrrsso........",
    ".......rbgbbbbso.........",
    ".......orhgggbso.........",
    ".......orrgbhbro.........",
    "........oorroo...........",
    "..........oo.............",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "........e......e.........",
    ".......oeooooooeo........",
    ".......otsbbbbsto........",
    ".......osbbbbbbso........",
    ".......osrrbbrrso........",
    ".......osrrbbrrso........",
    ".......osrrbbrrso........",
    ".......osrrbbrrso........",
    ".......osrrbbrrso........",
    ".......osrrbbrrso........",
    ".......osbbbbbbso........",
    ".......orbggggbro........",
    "........orhggbho.........",
    ".........orrrro..........",
    "..........oooo...........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    "............eo...........",
    "..........ooto...........",
    "........oobbbso..........",
    ".....eeobbbbbsoo.........",
    ".....otsbbbbrrso.........",
    ".....otsbrbbrrsoo........",
    "......osrrbbbrrso........",
    "......ossrrbbrrsoo.......",
    ".......osrrbbbrrso.......",
    ".......ossrrbbbbro.......",
    "........osrrbbggbo.......",
    "........ossbgggbh........",
    ".........orbgggrr........",
    ".........ooorrroo........",
    "............ooo..........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
  {
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".......eeoo..............",
    "........otsoo............",
    ".......obsbssoo..........",
    ".......obbrrssoo.........",
    "......obbbrrrrssoo.......",
    "......obbbbbrrrrsr.......",
    "....eesbrrbbbrrbbro......",
    "....oetbrrrbbbbbgho......",
    ".....oossrrrrbbggbr......",
    ".......oossrrrbggro......",
    "........oossrbghgro......",
    "..........oossbrro.......",
    "............ooo.o........",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
    ".........................",
  },
};
static inline int MmxHoverPixel(const MmxHoverCar *c,int x,int y) {
  /* Continuous fixed-point depth scale, sampled on native SNES pixels. */
  int scale=128+(int)c->depth*216/MMX_HOVER_CYCLE;
  int sx=(x*256+(x<0?-scale/2:scale/2))/scale+12;
  int sy=(y*256+(y<0?-scale/2:scale/2))/scale+12;
  if(sx<0 || sx>=25 || sy<0 || sy>=25) return -1;
  char material=mmx_hover_sprites[c->model*5+c->direction][sy][sx];
  static const uint16_t tops[4]={18|(24<<5)|(27<<10),23|(16<<5)|(9<<10),20|(10<<5)|(23<<10),14|(23<<5)|(19<<10)};
  static const uint16_t bodies[4]={8|(15<<5)|(21<<10),15|(8<<5)|(5<<10),12|(5<<5)|(17<<10),6|(14<<5)|(12<<10)};
  static const unsigned pulse[8]={15,18,22,27,31,28,23,18};
  switch(material) {
  case 'o':return MmxCosmicRgb(2,3,5);
  case 's':return MmxCosmicRgb(4,7,10);
  case 'b':return bodies[c->model];
  case 'r':return tops[c->model];
  case 'g':return sy<12?MmxCosmicRgb(12,24,27):MmxCosmicRgb(4,10,16);
  case 'h':return MmxCosmicRgb(25,30,31);
  case 't':return MmxCosmicRgb(31,6,7);
  case 'e':return MmxCosmicRgb(5,pulse[c->phase],31);
  default:break;
  }
  /* Eight-phase exhaust: a travelling bright packet and a tapered wake.
   * The body anchor remains on the lane; only the plasma animates. */
  int row=sy+1+(int)(c->phase%4);
  if(row<25 && material=='.' && mmx_hover_sprites[c->model*5+c->direction][row][sx]=='e')
    return MmxCosmicRgb(3,13+c->phase,23);
  return -1;
}
/* Cosmic Stage A: native BG2 layer-space debug paths, NO traffic sprites.
 * Coordinates are authored in the highway BG2 map's *world* pixel space,
 * never in the player's BG1/camera space or 1024px wrapped VRAM pages.
 * A/B cover the verified lower-road BG2 columns in the four telemetry shots.
 * C/D have two explicitly located curved connectors; they do not repeat
 * every VRAM page.  All knots still require visual road-alignment approval. */
typedef struct MmxTrafficPoint { int x, y; } MmxTrafficPoint;
typedef struct MmxTrafficDebugLane {
  char id;
  uint16_t color;
  unsigned direction; /* A/C/D: +X, B: -X */
  unsigned count;
  MmxTrafficPoint points[6];
} MmxTrafficDebugLane;
enum { MMX_TRAFFIC_DEBUG_LANES=4, MMX_TRAFFIC_BG2_X_PERIOD=1024,
       MMX_TRAFFIC_BG2_Y_PERIOD=256,
       MMX_TRAFFIC_ROAD_X_MIN=0x0200, MMX_TRAFFIC_ROAD_X_MAX=0x0c00,
       MMX_TRAFFIC_CURVE_SECOND_X_SHIFT=0x04b0 };
static const MmxTrafficDebugLane mmx_traffic_debug_lanes[4]={
  /* Straight lanes follow a single BG2-space line behind ALL pillar tiles.
   * Small 2px descent across 2560 road pixels avoids the old y>=224 cutoff. */
  {'A', 31u|(31u<<5), 1, 2, {{MMX_TRAFFIC_ROAD_X_MIN,209},{MMX_TRAFFIC_ROAD_X_MAX,211}}},
  {'B', (31u<<5)|(31u<<10), 0, 2, {{MMX_TRAFFIC_ROAD_X_MIN,218},{MMX_TRAFFIC_ROAD_X_MAX,220}}},
  /* First curve in BG2 source columns $0380..$03E0, second shifted $04B0.
   * Each follows the user's ramp centerline, not a camera-fixed screen arc. */
  {'C', 31u|(31u<<10), 1, 6,
    {{0x0380,197},{0x0392,197},{0x03a8,200},{0x03bc,206},{0x03d0,213},{0x03e0,220}}},
  {'D', (15u<<5), 1, 5,
    {{0x0380,210},{0x038e,209},{0x039e,211},{0x03ae,215},{0x03bd,221}}},
};

/* Floor division is essential at negative widescreen view margins. */
static inline int MmxTrafficWrap(int n,int period) {
  int r=n%period;
  return r<0?r+period:r;
}
/* v33d diagnostic gate: camera positions observed in the four user-provided
 * stage captures: $05B7, $0718, $0FCE, $148F.  Narrow to active BG2 follow
 * mode $02 so stage phase transitions/airship do not draw false highways.
 * These endpoints are TEST LIMITS until the actual road bounds are captured. */
enum { MMX_TRAFFIC_CAMERA_X_MIN=0x0500, MMX_TRAFFIC_CAMERA_X_MAX=0x1500 };
static inline int MmxTrafficDebugScene(uint8_t stage,uint8_t phase,
                                       unsigned camera_x,uint8_t bg2_mode) {
  return stage==0 && phase==4 && bg2_mode==2 &&
         camera_x>=MMX_TRAFFIC_CAMERA_X_MIN &&
         camera_x<=MMX_TRAFFIC_CAMERA_X_MAX;
}

/* Reconstruct the full BG2 scroll near streamed tilemap-page boundaries.
 * ExpandStageScroll supplies low-byte phase continuity; the BG2 streaming
 * register is authoritative for which 1024-pixel VRAM page is resident.
 * Match background()'s exact page offset, even when it differs by 256. */
static inline int MmxTrafficBg2Scroll(unsigned stream, unsigned ppu_scroll) {
  int expanded=MmxDisplay_ExpandStageScroll((uint16_t)stream,(uint16_t)ppu_scroll);
  int delta=(((int)ppu_scroll-(int)stream+512)&1023)-512;
  int source=(int)stream+delta;
  int page_delta=source-expanded;
  /* Both scrolls share the same pixel phase, so adjustment is whole pages. */
  return expanded+(page_delta/256)*256;
}
/* The straight lanes are continuous across 256/1024px BG2 page seams:
 * no modulo is allowed on their x-coordinate.  C/D are two *authored*
 * world-space curve regions (no periodic wrap across irrelevant tiles). */
static inline int MmxTrafficLaneY(unsigned lane,int bg2_world_x) {
  if(lane>=MMX_TRAFFIC_DEBUG_LANES) return -1;
  const MmxTrafficDebugLane *p=mmx_traffic_debug_lanes+lane;
  int px=bg2_world_x;
  if(lane>=2 &&
     px>=p->points[0].x+MMX_TRAFFIC_CURVE_SECOND_X_SHIFT &&
     px<=p->points[p->count-1].x+MMX_TRAFFIC_CURVE_SECOND_X_SHIFT)
    px-=MMX_TRAFFIC_CURVE_SECOND_X_SHIFT;
  if(px<p->points[0].x || px>p->points[p->count-1].x) return -1;
  for(unsigned i=1;i<p->count;++i) {
    MmxTrafficPoint a=p->points[i-1],b=p->points[i];
    if(px<a.x || px>b.x) continue;
    if(a.x==b.x) return a.y;
    int dx=px-a.x,dy=b.y-a.y,den=b.x-a.x;
    return a.y+(dy>=0?(dy*dx+den/2)/den:
             -((-dy*dx+den/2)/den));
  }
  return -1;
}

static inline int MmxTrafficDebugLayerPixel(int bg2_x,int bg2_y) {
  int by=MmxTrafficWrap(bg2_y,MMX_TRAFFIC_BG2_Y_PERIOD);
  /* C/D first only where their curves actually occupy the road. */
  for(int lane=MMX_TRAFFIC_DEBUG_LANES-1;lane>=0;--lane) {
    int line_y=MmxTrafficLaneY((unsigned)lane,bg2_x);
    if(line_y>=0 && by==line_y) return mmx_traffic_debug_lanes[lane].color;
  }
  return -1;
}
static inline int MmxTrafficDebugScreenPixel(int screen_x,int screen_y,
                                             int bg2_scroll_x,int bg2_scroll_y) {
  return MmxTrafficDebugLayerPixel(screen_x+bg2_scroll_x,
                                   screen_y+1+bg2_scroll_y);
}
/* A native BG1 bridge tile (deck or pillar) ALWAYS occludes BG2 debug ink. */
static inline int MmxTrafficDebugCompositedPixel(int bg2_x,int bg2_y,
                                                 unsigned bg1_pixel,unsigned bg2_pixel) {
  if(bg1_pixel || !bg2_pixel) return -1;
  return MmxTrafficDebugLayerPixel(bg2_x,bg2_y);
}

#endif

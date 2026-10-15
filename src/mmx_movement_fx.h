#pragma once
#include "mmx_cosmic.h"
/* Original draw-only accents, in player-local coordinates (+X = facing).
 * No hitboxes, floor probes, new state, or guest timing changes. */
static inline int MmxMovementFxPixel(unsigned action, int grounded, int zero,
                                     unsigned tick, int x, int y) {
  if(x < -42 || x>24 || y < -18 || y>19)return -1;
  if(action==0x14) {
    for(unsigned i=0;i<5;i++) {
      unsigned phase=(tick+i*5)%18;
      int px=-14-(int)phase-(int)i*2,py=grounded?13-(int)(phase/5):-3+(int)i*3;
      int dx=x-px,dy=y-py;
      if(dx>=-2 && dx<=2 && dy>=-1 && dy<=1 && (dx==0 || dy==0))
        return zero?MmxCosmicRgb(26,22,14):MmxCosmicRgb(13,25,19);
    }
  } else if(action==0x0a && grounded) {
    /* Native landing recovery controls the duration; broken paired arcs
     * stay below the body and never become a full-screen light flash. */
    int radius=7+(int)(tick%4)*3;
    int ax=x<0?-x:x;
    if(ax>=radius && ax<=radius+3 && y==15-(ax-radius)/2)
      return zero?MmxCosmicRgb(26,21,13):MmxCosmicRgb(16,26,19);
  }
  return -1;
}

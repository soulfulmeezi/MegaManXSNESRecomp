#pragma once
#include <stdint.h>
#include "mmx_cosmic.h"

/* Highway BG2 palette-only sky overlay. Original native clouds and their
 * parallax stay intact. Storm-front wisps move at 1-3 pixels per 60Hz tick,
 * with a second faster slipstream; all coordinates are recomputed from the
 * existing saved combat tick (deterministic on rewind/co-op).
 *
 * Color gate deliberately limits this to the verified purple Highway sky;
 * no HUD, actors, buildings, road pixels or arbitrary stages are altered.
 * -1 means transparent; renderer controls opt-in via MMX_COSMIC_SKY_MOTION.
 */
static inline int MmxCosmicSkyImpactPixel(int x,int y,unsigned tick,uint16_t base) {
  unsigned r=base&31u,g=(base>>5)&31u,b=(base>>10)&31u;
  if(y<16 || y>=100 || r<8 || r>14 || g<5 || g>10 ||
     b<7 || b>14 || r<=g) return -1;
  unsigned band=(unsigned)(y-16)/21u;
  unsigned row=(unsigned)(y-16)%21u;
  unsigned speed=band<2?3u:(band==2?2u:1u);
  unsigned drift=((unsigned)x + tick*speed + band*47u)&127u;
  /* Animated rolling cloud caps; varied lip thickness and shearing wind.
   * Keep broad shadow breaks so native cloud silhouettes still read. */
  unsigned shoulder=drift<64u?drift:127u-drift;
  unsigned crest=3u+shoulder/12u;
  unsigned thickness=3u+((drift>>4)+band)%4u;
  if(drift<108u && row>=crest && row<=crest+thickness) {
    unsigned wake=(drift+row*3u+(tick>>2))&15u;
    if(wake<3u) return MmxCosmicRgb(r+4u>31u?31u:r+4u,
                                    g+4u>31u?31u:g+4u,
                                    b+5u>31u?31u:b+5u);
    if(wake<12u) return MmxCosmicRgb(r+2u,g+2u,b+3u);
    return MmxCosmicRgb(r>2u?r-2u:0u,g>1u?g-1u:0u,b);
  }
  /* Fast wind streaks between cloud fronts: thin and intermittent, not
   * a full-screen flash that would obscure enemies or the original artwork. */
  unsigned gust=((unsigned)x+tick*4u+band*71u)&255u;
  if((row==2u || row==12u) && gust<80u && (gust&7u)<5u)
    return MmxCosmicRgb(r+2u,g+2u,b+3u);
  return -1;
}

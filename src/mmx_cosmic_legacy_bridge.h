#pragma once
#include <stdbool.h>
#include "mmx_cosmic.h"
#include "mmx_cosmic_core.h"

/* Pure versioned mapping between unrelated native and progression IDs.
 * Existing Earth Wave and Jupiter Storm remain available without this bridge.
 */
enum { MMX_COSMIC_LEGACY_BRIDGE_VERSION = 1 };

static inline unsigned MmxCosmicAspectForNative(unsigned page,unsigned kind) {
  if(page!=MMX_COSMIC_PAGE) return MMX_COSMIC_NONE;
  if(kind==MMX_EARTH_WAVE) return MMX_COSMIC_EARTH;
  if(kind==MMX_JUPITER_STORM) return MMX_COSMIC_STORM;
  return MMX_COSMIC_NONE;
}
static inline bool MmxCosmicNativeForAspect(unsigned aspect,
                                          unsigned *page,unsigned *kind) {
  if(!page || !kind) return false;
  unsigned native;
  if(aspect==MMX_COSMIC_EARTH) native=MMX_EARTH_WAVE;
  else if(aspect==MMX_COSMIC_STORM) native=MMX_JUPITER_STORM;
  else return false; /* Hypothetical Gravity is NOT Earth Wave. */
  *page=MMX_COSMIC_PAGE;
  *kind=native;
  return true;
}

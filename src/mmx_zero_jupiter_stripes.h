#pragma once
#include <stdint.h>
#include "mmx_future_zero_color.h"

/* Pixel-level stripes (not CGRAM/palette swapping). The caller must pass
 * the ORIGINAL palette entry so hair, face, gem, undersuit, attacks and
 * neutral outlines are never treated as armor. Stripe coordinates are
 * pose-local so the pattern follows Zero through movement animations.
 *
 * This is a conservative color-only texture overlay: existing red armor
 * pixels alone receive thin wavy horizontal Jupiter belts. It does not
 * change pose indices, hitboxes, ROM, or sprite animations. */
static inline uint16_t MmxZeroJupiterStripePixel(
    uint16_t original, uint16_t base,
    unsigned local_x, unsigned local_y, unsigned armor_top) {
  if (!MmxFutureZeroIsRedArmor(original) ||
      (original & 31) < 18 ||
      local_x >= 128 || local_y >= 128 ||
      local_y < armor_top) return base;

  /* One-pixel horizontal undulation prevents ruler-straight bands.
   * The period is 17 pixels, giving several cream/tan/rust belts across
   * the helmet, torso and leg armor without inventing new geometry. */
  unsigned ripple = (local_x / 10u) % 3u == 1u ? 1u : 0u;
  unsigned line = (local_y - armor_top + ripple) % 17u;
  uint16_t cloud;
  if (line < 3) cloud = MmxFutureZeroBgr555(29,28,26);      /* cream */
  else if (line < 6) cloud = MmxFutureZeroBgr555(25,21,16); /* sand */
  else if (line < 10) cloud = MmxFutureZeroBgr555(23,16,10);/* orange */
  else if (line < 13) cloud = MmxFutureZeroBgr555(19,12,8); /* bronze */
  else cloud = MmxFutureZeroBgr555(27,25,22);               /* cloud */

  /* Leave a quarter of the original Jupiter shade to preserve volume. */
  unsigned red = (((base & 31u) + 3u*(cloud & 31u) + 2u) / 4u);
  unsigned green = ((((base >> 5) & 31u) +
                     3u*((cloud >> 5) & 31u) + 2u) / 4u);
  unsigned blue = ((((base >> 10) & 31u) +
                    3u*((cloud >> 10) & 31u) + 2u) / 4u);
  return MmxFutureZeroBgr555(red,green,blue);
}

#pragma once
#include <stdbool.h>
#include <stdint.h>
/* SNES BGR555 armor remap. Only saturated, strongly red-dominant pixels
 * are transformed. Gold hair, skin, green gems, neutral outlines and
 * unmodified X palettes remain untouched. Applied to Zero's host art only. */
static inline bool MmxFutureZeroIsRedArmor(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return r>=11 && g<=12 && r>=g+8 && r>=b+8;
}
static inline uint16_t MmxFutureZeroYellowBlack(uint16_t c) {
  if(!MmxFutureZeroIsRedArmor(c)) return c;
  unsigned r=c&31;
  if(r>=17) {
    /* Two levels of yellow/gold, preserving the existing lightness. */
    unsigned g=r>=3 ? r-3 : 0;
    return (uint16_t)(r | (g<<5) | (2u<<10));
  }
  /* The dark-red armor shade becomes charcoal black rather than red. */
  unsigned d=r/3;
  return (uint16_t)(d | (d<<5) | (d<<10));
}

#pragma once
#include <stdbool.h>
#include <stdint.h>

/* SNES 15-bit BGR555: red in bits 0-4, green in 5-9, blue in 10-14.
 * Recolor only Zero's clearly red-dominant *armor* palette entries. A
 * palette-only first pass cannot paint new physical Jupiter stripes; the
 * seven tonal bands follow the armor's existing pixel shading. Hair,
 * face, green gems, dark neutral undersuit and X's art stay intact. */
static inline bool MmxFutureZeroIsRedArmor(uint16_t c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return r>=11 && g<=12 && r>=g+8 && r>=b+8;
}
static inline uint16_t MmxFutureZeroBgr555(unsigned r,unsigned g,unsigned b) {
  return (uint16_t)((r&31)|((g&31)<<5)|((b&31)<<10));
}

/* Jupiter's clouds: storm-white, cream, sandy tan, rusty belt, bronze,
 * deep brown and charcoal. Original red brightness chooses the tone;
 * the pixel positions are NEVER altered, so every animation stays valid.
 *
 * Upper bands (brightest armor) are atmospheric clouds; lower bands map
 * the dark red armor shadows to Jupiter's weathered brown depths. */
static inline uint16_t MmxFutureZeroJupiter(uint16_t original) {
  if(!MmxFutureZeroIsRedArmor(original)) return original;
  unsigned red=original&31;
  if(red>=30) return MmxFutureZeroBgr555(29,28,26); /* #E9E1D2 cream white */
  if(red>=27) return MmxFutureZeroBgr555(27,25,22); /* #D9C9B3 pale cloud */
  if(red>=24) return MmxFutureZeroBgr555(25,21,16); /* #C9A77F sand */
  if(red>=21) return MmxFutureZeroBgr555(23,16,10); /* #B98252 orange belt */
  if(red>=18) return MmxFutureZeroBgr555(19,12,8);  /* #9A643F bronze */
  if(red>=15) return MmxFutureZeroBgr555(13,9,6);   /* #694632 deep brown */
  return MmxFutureZeroBgr555(6,5,4);                 /* #2E2623 charcoal */
}

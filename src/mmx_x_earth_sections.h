#ifndef MMX_X_EARTH_SECTIONS_H
#define MMX_X_EARTH_SECTIONS_H

#include <stdint.h>

/* Section ids are deliberately stable across X's pose and OAM tile changes. */
enum {
  MMX_EARTH_HELMET = 0,
  MMX_EARTH_CHEST,
  MMX_EARTH_SHOULDER,
  MMX_EARTH_FOREARM,
  MMX_EARTH_THIGH,
  MMX_EARTH_BOOT,
  MMX_EARTH_SECTION_COUNT
};

static inline int MmxEarthXFaceDetail(unsigned object, unsigned section,
                                      int x, int y) {
  /* X's blue eye/visor shares his armor ramp. Leave the tiny front-of-helmet
   * facial detail native while the surrounding helmet plate gets the map. */
  return object == 0xba8 && section == MMX_EARTH_HELMET &&
      x >= 5 && x <= 10 && y >= -27 && y <= -12;
}

static inline unsigned MmxEarthMod(int value, unsigned modulus) {
  int result = value % (int)modulus;
  return (unsigned)(result < 0 ? result + (int)modulus : result);
}

static inline int MmxEarthFloorDiv(int value, int divisor) {
  int result = value / divisor;
  return value < 0 && value % divisor ? result - 1 : result;
}

static inline uint16_t MmxEarthRgb(unsigned r, unsigned g, unsigned b) {
  return (uint16_t)((r & 31) | ((g & 31) << 5) | ((b & 31) << 10));
}

/* Repaint only saturated blue armor shades. Skin, hair, black undersuit,
 * white trims and the small neutral/green details stay in their native ramp. */
static inline int MmxEarthIsBlueArmor(uint16_t color) {
  unsigned r = color & 31, g = (color >> 5) & 31, b = (color >> 10) & 31;
  return b >= 13 && b >= g + 2 && b >= r + 6;
}

/* A plate-local, hard-edged Earth surface: blue ocean, blocky green/brown
 * continents, curved pale cloud streaks and a tiny pixel storm curl. Coordinates
 * are anchored to X's body rather than the camera or the OAM tile origin. */
static inline uint16_t MmxEarthXArmorPixel(uint16_t original,
                                          unsigned section,
                                          int x, int y) {
  if (section >= MMX_EARTH_SECTION_COUNT || !MmxEarthIsBlueArmor(original))
    return original;

  static const uint8_t phase[MMX_EARTH_SECTION_COUNT] = {2, 7, 11, 15, 20, 25};
  unsigned p = phase[section];
  unsigned r = original & 31, g = (original >> 5) & 31, b = (original >> 10) & 31;
  unsigned brightness = (r + g + b) / 3;
  uint16_t ocean = brightness < 15 ? MmxEarthRgb(2, 8, 19) :
      brightness < 22 ? MmxEarthRgb(3, 13, 25) : MmxEarthRgb(6, 18, 30);

  /* Continents are laid down in coarse square pixels so the pattern keeps
   * the native sprite's crisp SNES edges. Each plate has a different map. */
  int u = (int)MmxEarthMod(MmxEarthFloorDiv(x + (int)p * 3, 4), 16);
  int v = (int)MmxEarthMod(MmxEarthFloorDiv(y + (int)p * 5, 4), 12);
  int ax = u - 4, ay = v - 4;
  int bx = u - 12, by = v - 8;
  int cx = u - 10, cy = v - 2;
  int d1 = ax * ax * 2 + ay * ay * 3;
  int d2 = bx * bx * 2 + by * by * 3;
  int d3 = cx * cx + cy * cy * 4;
  int land = (d1 <= 28 || d2 <= 19 || d3 <= 10) &&
      !((u == 4 || u == 5) && v >= 3 && v <= 5);
  if (land) {
    unsigned shade = (unsigned)MmxEarthMod((int)brightness + (int)p + u + v, 3);
    return shade == 0 ? MmxEarthRgb(5, 15, 8) :
        shade == 1 ? MmxEarthRgb(8, 22, 10) : MmxEarthRgb(16, 17, 9);
  }

  /* A short cloud band bends over each plate instead of repeating as a flat
   * palette stripe. Keep the cloud colors distinct from the ocean highlights. */
  unsigned wx = MmxEarthMod(MmxEarthFloorDiv(x + (int)p, 3), 18);
  unsigned wave = MmxEarthMod(y + (int)p * 2 + (int)(wx < 9 ? wx : 18 - wx) / 2, 16);
  if (wave == 1 || wave == 2)
    return wave == 1 ? MmxEarthRgb(27, 29, 30) : MmxEarthRgb(17, 25, 30);

  /* Small white, stepped storm curls: an eye and a three-sided spiral. */
  unsigned sx = MmxEarthMod(MmxEarthFloorDiv(x + (int)p * 2, 2), 16);
  unsigned sy = MmxEarthMod(MmxEarthFloorDiv(y + (int)p, 2), 12);
  int swirl = (sx == 5 && sy == 4) || (sx == 6 && sy == 4) ||
      (sx == 7 && sy == 4) || (sx == 7 && sy == 5) ||
      (sx == 7 && sy == 6) || (sx == 6 && sy == 6) ||
      (sx == 5 && sy == 6) || (sx == 4 && sy == 6) ||
      (sx == 4 && sy == 5);
  if (swirl) return MmxEarthRgb(29, 30, 31);
  return ocean;
}

#endif

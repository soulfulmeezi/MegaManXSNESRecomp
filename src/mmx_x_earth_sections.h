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

static inline uint16_t MmxEarthRgb(unsigned r, unsigned g, unsigned b) {
  return (uint16_t)((r & 31) | ((g & 31) << 5) | ((b & 31) << 10));
}

/* Repaint only saturated blue armor shades. Skin, hair, black undersuit,
 * white trims and the small neutral/green details stay in their native ramp. */
static inline int MmxEarthIsBlueArmor(uint16_t color) {
  unsigned r = color & 31, g = (color >> 5) & 31, b = (color >> 10) & 31;
  return b >= 13 && b >= g + 2 && b >= r + 6;
}

/* Each chart is twelve native pixels across: a complete coastline fits on a
 * plate instead of sampling a fragment of a sprite-sized globe. G/T form one
 * connected continent; T is a small dry interior and C a thin cloud arc.
 * No noise, interpolated pixels, or randomly alternating terrain shades. */
static inline char MmxEarthXMaterial(unsigned section, int x, int y) {
  static const char maps[MMX_EARTH_SECTION_COUNT][12][13] = {
    { /* Helmet: broad northern land, tapering into a southern peninsula. */
      "......CC....", "..GGGG..CC..", ".GGTTGG.....", ".GGTTGGG....",
      "..GGGGGG....", "...GGGG.....", "....GGG.CC..", ".....GGG..C.",
      "......GG....", ".CC...GG....", "...CC.......", "............"
    },
    { /* Chest: the largest connected land silhouette is the focal point. */
      ".......CC...", "..GGGG...CC.", ".GGTTGGG....", ".GGTTGGG....",
      "..GGGGG.....", ".CCGGG......", "...CGGGG....", "....GTTGG...",
      ".....GGGG...", "......GG....", ".CC....G.CC.", "...C........"
    },
    { /* Shoulder: a wide continent wraps over the cap. */
      "............", "....CC......", "..CC........", "..GGGGGG....",
      ".GGGTTGGG...", ".GGTTTGGGG..", "..GGGGGGG...", "...GGG.GG...",
      "....GG...CC.", ".....G.CC...", "..CC........", "....C......."
    },
    { /* Forearm: a single long coast, with a short offshore cloud. */
      "............", "...GGGG.....", "..GGTGGG....", "..GTTGGG....",
      "..GGGGGG.CC.", "...GGGG.C...", ".CC.GGG.....", "...CGGGG....",
      ".....GG.....", "......G.CC..", ".......C....", "............"
    },
    { /* Thigh: a rounded landmass and a narrow southern tip. */
      ".CC.........", "...CGGG.....", "...GGTTG....", "..GGTTTGG...",
      "..GGGGGGG.CC", "...GGGGGGC..", "....GGGG....", ".....GGG....",
      "......GG....", ".CC....G....", "...CC.......", "............"
    },
    { /* Boot: deep ocean around a continuous coast, no white ice blocks. */
      "......CC....", "..GGG...CC..", ".GGTTGG.....", ".GGTTGGG....",
      "..GGGGGG....", ".CCGGGGG....", "...CGGGG....", ".....GGGG...",
      "......GGG...", ".......GG...", "..CC....G...", "....C......."
    }
  };
  static const int8_t origin_x[] = {-6, -6, 6, 8, 0, 0};
  static const int8_t origin_y[] = {-28, -16, -23, -10, -6, 5};
  if (section >= MMX_EARTH_SECTION_COUNT) return '.';
  /* Paired limbs wrap the same map around their outer surface. Facing is
   * normalized by the renderer before reaching this body-local chart. */
  if (section >= MMX_EARTH_SHOULDER && x < 0) x = -x;
  return maps[section][MmxEarthMod(y - origin_y[section], 12)]
                      [MmxEarthMod(x - origin_x[section], 12)];
}

/* Preserve native lighting within each material. The coastline never depends
 * on palette brightness, tile boundaries, camera position, or animation time. */
static inline uint16_t MmxEarthXArmorPixel(uint16_t original,
                                          unsigned section,
                                          int x, int y) {
  if (section >= MMX_EARTH_SECTION_COUNT || !MmxEarthIsBlueArmor(original))
    return original;

  unsigned r = original & 31, g = (original >> 5) & 31, b = (original >> 10) & 31;
  unsigned brightness = (r + g + b) / 3;
  unsigned shade = brightness < 11 ? 0 : brightness < 18 ? 1 : 2;
  switch (MmxEarthXMaterial(section, x, y)) {
    case 'G': return shade == 0 ? MmxEarthRgb(4, 12, 6) :
        shade == 1 ? MmxEarthRgb(6, 19, 9) : MmxEarthRgb(11, 24, 12);
    case 'T': return shade == 0 ? MmxEarthRgb(13, 12, 6) :
        shade == 1 ? MmxEarthRgb(20, 18, 9) : MmxEarthRgb(25, 23, 13);
    case 'C': return shade == 0 ? MmxEarthRgb(21, 25, 27) :
        shade == 1 ? MmxEarthRgb(27, 29, 30) : MmxEarthRgb(30, 31, 31);
    default: return shade == 0 ? MmxEarthRgb(1, 5, 13) :
        shade == 1 ? MmxEarthRgb(2, 10, 21) : MmxEarthRgb(5, 16, 26);
  }
}

#endif

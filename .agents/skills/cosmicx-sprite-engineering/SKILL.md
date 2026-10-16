---
name: cosmicx-sprite-engineering
description: Use when changing Cosmic X 16-bit fighter sprites, palette conversion, heading rotation, perspective scaling, pivots, or sprite-sheet generation.
---

# Fighter art engineering
- Preserve the exact 36x20 source fighter pixel grid and both distinct palettes. Convert '.' to transparent alpha, not background color. Never use approximate/AI-generated replacement designs.
- The native image pivot is center (18,10); source draws sprite at (-18,-10). Round angle to 16 steps using `k=PI/8`, `a=round(heading/k)*k`. When `cos(a)<0`, invert local Y as the reference does, then scale. Preserve flame placement relative to pivot.
- Depth is a genuine 0.25x–2.6x range. At the max 2.6x, the unrotated 36-pixel image spans about 94 native reference pixels, not 10–20 px. Scale world-to-screen coordinates from source reference width 256 and height 156; validate viewport clipping and layer depth.
- Inspect native renderer transforms and image sampling first. If direct draw meets frame budget, use nearest-neighbor scaling/rotation. If it does not, generate a no-substitution atlas from original pixels: 2 teams x 16 angles x ~5 scale bands spanning both endpoints, plus any additional bands required for close-pass smoothness. Correct alpha bounds, pivots and atlas metadata are mandatory.
- Validate at 0, 90, 180 and 270 degrees and intermediate orientations; ensure left-going craft do not visually flip belly-up.
- Produce palette hash and pixel-diff checks and a contact sheet, including 0.25x, 1x and 2.6x. Preserve cruiser hulls at their actual canvas sizes in Round 2.

Do not claim this skill generated or tested an atlas unless the assets, build, and captures actually exist.

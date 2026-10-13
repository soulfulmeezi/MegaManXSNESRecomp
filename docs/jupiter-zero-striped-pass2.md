# Jupiter Zero: horizontal cloud belts (experimental Pass 2A)

Base: `experiment/zero-jupiter-palette-pass1-20261012`. All existing
AI, camera separation, recovery logging and game physics remain unchanged.

## What actually changed

- `src/mmx_zero_jupiter_stripes.h` is a real, pixel-level SNES BGR555
  recolor helper. Jupiter's cream, sand, orange/rust, bronze and cloud-white
  belts alternate horizontally with a 1-pixel gentle wave.
- `src/mmx_zero.c` retains Zero's original 128 source palette entries and
  calculates the uppermost red-armor pixel for each of his **152 animation
  poses**. The ripple is relative to that pose's original sprite pixels,
  rather than the viewport, so it moves with Zero.
- `src/mmx_renderer.c` applies the result to main-player Zero **and**
  CPU/partner Zero as each pixel is submitted. Pixel-index art, hitboxes,
  animation timelines and CPU pad inputs are never modified.
- Strongly red-dominant source armor pixels only; keep dark armor edges,
  skin, hair, gem, undersuit, saber, charged particles and HUD unstriped.
- Seven-color Jupiter palette from Pass 1 is still the base.

These are color bands drawn on the existing armor pixels. This is NOT a
hand-painted pixel-art mask that outlines helmet/chest/boot pieces precisely.
Some short fragments and animation poses may vary; assess the actual game
screenshot before a more expensive hand-masked Pass 2B.

## Compare stripe intensity

**Default:** stripes ON in this branch.

- `MMX_ZERO_NO_STRIPES=1` (on launch) shows Pass 1 Jupiter palette but
  *without* stripes.
- `MMX_ZERO_CLASSIC_RED=1` restores the original extracted red armor and
  also disables the stripes.

## Windows MSYS2 MINGW64

```bash
cd ~/MegaManXSNESRecomp
git fetch origin
git switch --track origin/experiment/zero-jupiter-striped-pass2-20261012
cmake --build build-recompui -j 4
ctest --test-dir build-recompui -R mmx_future_zero --output-on-failure
MMX_CPU_FREE_FOLLOW=1 MMX_CPU_LEARN=1 MMX_CPU_TRACE=1 MMX_CPU_CRUMBLE=1 MMX_CPU_PIT_WALL=1 MMX_CPU_WALL_ARC=1 SDL_AUDIO_DRIVER=directsound ./build-recompui/MegaManXSNESRecomp.exe 2>&1 | tee cpu-jupiter-stripes-v14.log
```

If already on this branch, use `git pull --ff-only` instead of the first
`git switch --track`. The experimental patch hasn't been verified by playing
with the SNES ROM while authoring, so send a standing, jumping and wall-kick
screenshot if rendering is incorrect.

The unit test `tests/mmx_future_zero_test.c` now covers stripe band
changes, pose-relative offsets, non-armor protection and existing safe-follow.

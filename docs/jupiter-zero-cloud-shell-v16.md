# Jupiter Zero: Cloud Shell v16 (experimental)

Branch: `experiment/zero-jupiter-cloud-shell-v16-20261012`

This version preserves the CPU's improved jump and wall recovery AI and
changes ONLY host-side Zero colors. It is based on the working v14 striped
Jupiter experiment.

## What's different

The prior variants recolored saturated red armor. Zero's metal-like white /
silver plates were still largely untouched. This experiment adds a
pose-relative pixel-color material mapping to `src/mmx_zero_jupiter_shell.h`.
It replaces **eligible bright gray, white, original red, green jewel and warm
metallic armor pixels** with cream, tan, sand, rusty orange and bronze Jupiter
belts. It ignores original metallic highlight/shadow differences within each
cloud belt: the source sprite silhouette is retained, but its colored surface
is a unified Jupiter material.

The original face/skin colors, hair above the face, dark/black undersuit,
blue facial details and particles remain protected. The renderer's existing
per-pixel hook already handles **both** CPU Zero and player Zero, so the new
material feeds through the same function without changing the co-op AI.
The helmet/jump/wall sprite art still has its original SNES outlines and
internal lines; color mapping alone cannot erase those without mask/asset edits.

## How to try it (MSYS2 MINGW64)

```bash
cd ~/MegaManXSNESRecomp
git fetch origin
git switch --track origin/experiment/zero-jupiter-cloud-shell-v16-20261012
cmake --build build-recompui -j 4
ctest --test-dir build-recompui -R mmx_future_zero --output-on-failure
MMX_CPU_FREE_FOLLOW=1 MMX_CPU_LEARN=1 MMX_CPU_TRACE=1 MMX_CPU_CRUMBLE=1 MMX_CPU_PIT_WALL=1 MMX_CPU_WALL_ARC=1 SDL_AUDIO_DRIVER=directsound ./build-recompui/MegaManXSNESRecomp.exe 2>&1 | tee cpu-jupiter-cloud-shell-v16.log
```

Optional launch flags for comparisons:
- `MMX_ZERO_NO_SHELL=1`: uses the existing striped Jupiter texture instead.
- `MMX_ZERO_NO_SHELL=1 MMX_ZERO_NO_STRIPES=1`: Jupiter palette Pass 1.
- `MMX_ZERO_CLASSIC_RED=1`: removes all Jupiter recoloring.

The `mmx_future_zero_test` test now covers silver plate conversion,
green jewel conversion, golden hair protection, facial skin protection,
dark undersuit protection and replacement of metallic shade differences.

**Known limitation:** This is a pixel-color texture overlay in the custom
renderer, not newly drawn/tiled art. Inspect standing and jumping images
before calling it a successful Jupiter material. It has not been played
in-game by the author.

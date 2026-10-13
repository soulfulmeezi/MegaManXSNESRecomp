# Jupiter Zero v17 — Planet Shell (experimental)

This branch starts from `experiment/zero-jupiter-cloud-shell-v16-20261012`.
**No gameplay, AI, camera, physics, netplay, ROM or collision changes.**

## What is new

- Real **per-pose material mask** for each of Zero's 152 indexed 128×128
  animation images. Compiled from the source artwork at load time, once per
  session. `src/mmx_zero_jupiter_planet.h`.
- The source palette identifies **bright silver, pale metal, red, gold,
  and colored details**, while individual skin-colored and blue face pixels,
  the connected golden ponytail, transparent silhouette, and deep black
  undersuit are left untouched.
- Eligible original metal/colored areas are filled with Jupiter atmosphere
  from a **single shared set of soft, curved cream, sand, orange and rust
  gas bands**. The output is independent of the source armor highlight
  shade. Short isolated metallic seam lines are filled where both sides
  contain cloud material, minimizing the painted-armor appearance.
- A tiny bronze oval "Great Red Spot" accent sits near the upper chest
  in each pose when there is source shell material there.
- It uses the same renderer hook for the main playable Zero and offline
  CPU companion Zero. The animation and silhouette pixels are unchanged.
- If memory allocation for the per-pose masks fails, previous v16 shell
  rendering remains the fallback.

## Run in Windows MSYS2 MINGW64

```bash
cd ~/MegaManXSNESRecomp
git fetch origin
git switch --track origin/experiment/zero-jupiter-planet-shell-v17-20261012
cmake --build build-recompui -j 4
ctest --test-dir build-recompui -R mmx_future_zero --output-on-failure
MMX_CPU_FREE_FOLLOW=1 MMX_CPU_LEARN=1 MMX_CPU_TRACE=1 MMX_CPU_CRUMBLE=1 MMX_CPU_PIT_WALL=1 MMX_CPU_WALL_ARC=1 SDL_AUDIO_DRIVER=directsound ./build-recompui/MegaManXSNESRecomp.exe 2>&1 | tee cpu-planet-v17.log
```

If the local branch already exists, use `git switch experiment/zero-jupiter-planet-shell-v17-20261012 && git pull --ff-only`.

## Visual comparison flags

Restart the game after changing launch flags:
- Default: v17 Planet Shell pixel masks and cloud material.
- `MMX_ZERO_NO_PLANET=1`: v16 Cloud Shell.
- `MMX_ZERO_NO_PLANET=1 MMX_ZERO_NO_SHELL=1`: v14 Jupiter stripes.
- Add `MMX_ZERO_NO_STRIPES=1` to see Jupiter palette-only.
- `MMX_ZERO_CLASSIC_RED=1`: original extracted red.

## Test coverage and limitations

`mmx_future_zero_test` contains a synthetic SNES sprite pose and checks:
hair / facial skin / blue eye / deep black untouched; original silver,
gray and green armor all converted; narrow internal shading seam removed;
a source gray and a source white plate producing identical gas colors when
drawn at the same coordinate; actual y-dependent clouds. Previously
verified safe-follow arithmetic remains covered.

**Visual verification is still required**: the exact original Zero sprite
palettes are loaded from the game asset on the player's PC and were not
available here for visual inspection. The artifact is a *pixel-color
material mask*, not a new hand-painted sheet. Especially check ponytail,
helmet, black undersuit and Great Red Spot on standing and jump poses.
If some internal outlines remain, that is intentional—truly deleting all
sprite pixel edges would require replacing pose artwork, which we can plan
after testing this safer experiment.

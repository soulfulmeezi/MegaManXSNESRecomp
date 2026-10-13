# Jupiter Zero v18 — section bands

## Artwork changes

- Every connected armor plate receives its own pose-local, wavy Jupiter cloud
  pattern. Short plates such as the helmet and forearms use four broad lanes
  so cream, tan, rust and brown remain visible at SNES sprite scale.
- All original red armor shades, including the two darkest ones, enter the
  armor mask. Face, hair, blue face details, outlines and black undersuit stay
  protected; the sprite silhouette and animation pixels are unchanged.
- The Great Red Spot is a larger red-orange chest crest with a pale swirl.
  Boots retain their darker storm palette and available plates get small white
  storm swirls.

## Solo co-op control

The co-op feature now defaults Player 2 to CPU Companion for solo play. It
keeps the existing companion behavior and enables L2 on Player 1's controller
to switch control between X and Zero. Choose Human (Player 2 controller) for
two-player co-op. Startup diagnostics state which control mode was selected.

No companion AI, wall recovery, camera, gameplay or hitbox logic changed.

## Build and launch from MSYS2 MINGW64

From the repository folder:

```bash
git fetch origin
git switch --track origin/experiment/zero-jupiter-section-bands-v18-20261012
cmake --build build-recompui --parallel 4
./build-recompui/MegaManXSNESRecomp.exe
```

If the first CMake configuration reports that `src/gen` is empty, stage the
verified X1 ROM and regenerate the local generated code before configuring:

```bash
cp "/path/to/Mega Man X (USA Rev 1).sfc" mmx.sfc
bash tools/regen.sh usa --no-tests
cmake -S . -B build-recompui -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/mingw64
cmake --build build-recompui --parallel 4
```

The launcher also needs the verified X3 ROM selected for its Zero sprite
assets.

## Checks

`mmx_future_zero_test` verifies dark red armor coverage, protected pixels,
separate local patterns, short-plate colors, chest emblem, swirls and dark
boots. `mmx_zero_test` continues to cover Zero asset and state behavior.

# Jupiter Zero — color-only first pass

This experimental branch inherits all working CPU navigation, camera-separation
and teleport diagnostics from `experiment/cpu-teleport-diagnostics-20261012`.

**Reference:** Jupiter's cream, sandy tan, orange/rust and brown horizontal cloud belts.

## Armor remap

`src/mmx_future_zero_color.h` applies `MmxFutureZeroJupiter` only to
**strongly red-dominant** colors in Zero's source palettes, selecting a new
SNES BGR555 tone from the source red channel:

| Original red (0–31) | Jupiter tone | Visual design target |
| --- | --- | --- |
| 30–31 | Storm-cloud white | `#E9E1D2` |
| 27–29 | Ivory cloud | `#D9C9B3` |
| 24–26 | Sandy tan | `#C9A77F` |
| 21–23 | Rusty orange belt | `#B98252` |
| 18–20 | Bronze | `#9A643F` |
| 15–17 | Deep storm brown | `#694632` |
| 11–14 | Charcoal | `#2E2623` |

The HTML hex colors are design references; SNES 5-bit channels approximate them.
Other palette entries remain unchanged. Hair, skin, green gem, neutral undersuit,
sprite poses, physics, partner camera, CPU navigation and co-op behavior stay as is.
Zero's HUD palette uses the same map. The charged armor-color palette stays
Jupiter-colored while preserving effects and non-armor colors.

**Important:** This pass uses only the existing sprite shading. It **does not**
create new horizontal stripes in texture regions that lack stripes already.
If the first screenshot needs stronger Jupiter cloud bands, the next pass
should use a tightly masked sprite/texture edit scoped to helmet, torso,
shoulders and boots.

## Build and play

From MSYS2 MINGW64:

```bash
cd ~/MegaManXSNESRecomp
git fetch origin
git switch --track origin/experiment/zero-jupiter-palette-pass1-20261012
cmake --build build-recompui -j 4
ctest --test-dir build-recompui -R 'mmx_(future_zero|cpu_jump_learning|cpu_deep_descent|cpu_antistuck)' --output-on-failure
MMX_CPU_FREE_FOLLOW=1 MMX_CPU_LEARN=1 MMX_CPU_TRACE=1 MMX_CPU_CRUMBLE=1 MMX_CPU_PIT_WALL=1 MMX_CPU_WALL_ARC=1 SDL_AUDIO_DRIVER=directsound ./build-recompui/MegaManXSNESRecomp.exe 2>&1 | tee cpu-jupiter-zero-v13.log
```

A/B: `MMX_ZERO_CLASSIC_RED=1` on a new game launch restores the extracted
classic-red palette, without altering the AI. To compare to the previous
yellow/black palette, switch back to the previous experimental branch.

## Testing status

The added ROM-free test in `tests/mmx_future_zero_test.c` asserts all
seven shade mappings and that hair, skin, neutral and gem colors are unchanged.
The branch has **not been run with a ROM in-game** during implementation.
Review an actual Zero screenshot before judging stripe quality.

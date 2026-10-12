# Future Zero: yellow/black + cautious CPU follow experiment

Branch: `experiment/cpu-future-zero-yellow-safe-separation-20261012`

## Scope

- Recolor the *host-side Zero artwork* by converting saturated red armor colors into yellow/gold highlights and charcoal shadows. Existing dark armor, skin, blond hair, green gem and animations are left unchanged. The recolor also covers Zero's HUD icon and keeps red armor pixels yellow through the three X3 charged-flash palettes.
- Fix CPU defensive vertical jump: when direction is 0, hold the jump button **without** secretly holding LEFT. Native player movement/physics are unchanged.
- **Optional, offline CPU-only:** `MMX_CPU_FREE_FOLLOW=1` extends the shared-camera separation limit from 224 to 288 world pixels, moving the camera more toward the human-controlled character when separated by over 160 pixels. Both human co-op and online/netplay keep the original camera and 224px limit.
- With that same opt-in, Zero waits for a human airborne route demonstration only when already **safely grounded on stable, non-collapsing ground**, has no committed jump/drop/wall route and is not attempting a health pickup. The existing crumble-response and live landing checks remain in place.
- Existing ground-dash catch-up policy continues to trigger on verified terrain. This experiment does not add a new dash or alter jump physics.

## Build and test (MSYS2 MINGW64)

```bash
cd ~/MegaManXSNESRecomp
git fetch origin
git switch --track origin/experiment/cpu-future-zero-yellow-safe-separation-20261012
cmake --build build-recompui -j 4
ctest --test-dir build-recompui -R 'mmx_(future_zero|cpu_jump_learning|cpu_deep_descent|cpu_antistuck)' --output-on-failure
```

## Run and compare

```bash
MMX_CPU_FREE_FOLLOW=1 MMX_CPU_LEARN=1 MMX_CPU_TRACE=1 MMX_CPU_CRUMBLE=1 MMX_CPU_PIT_WALL=1 MMX_CPU_WALL_ARC=1 SDL_AUDIO_DRIVER=directsound ./build-recompui/MegaManXSNESRecomp.exe 2>&1 | tee cpu-future-zero-v11.log

grep -E '\[cpu-(follow-limit|safe-wait|dash|jump|death|nav|air-route|wall|crumble)|\[human-learn' cpu-future-zero-v11.log | tail -n 180
```

Look for `[cpu-safe-wait]` when Zero waits on verified footing, `[cpu-dash]` when safely catching up, and `[cpu-follow-limit]` when X is still blocked at the expanded limit. Save your logs near the previously dangerous Highway collapse at x=4327–4479.

## A/B and limitations

- Leave out `MMX_CPU_FREE_FOLLOW=1` to keep the old **224px separation and midpoint camera**, while retaining the vertical-jump fix and yellow recolor.
- Set `MMX_ZERO_CLASSIC_RED=1` before launch to keep the original extracted red palette for side-by-side comparison; it is loaded at startup.
- This is **not** an unlimited separation/camera redesign. The offscreen companion may still be culled or encounter stage-specific camera-driven terrain/objects. Test cautiously on the Highway before using a long playthrough.
- Local ROM-free unit tests validate the pure color transform, camera/separation arithmetic, stable-wait gate, and defensive zero-direction B-pad logic. **No full ROM gameplay run was performed while authoring this branch**. Yellow/black appearance needs a screenshot to tune the exact Zero palette colors.
- Story dialogue, quest events, save structure and netplay behavior are intentionally unchanged.

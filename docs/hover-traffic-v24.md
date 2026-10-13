# Hover traffic — v24, Task 1

Branch: `experiment/hover-traffic-v24-20261013`.

## Frames, states and layers

- Four original models: sedan, cargo truck, sports car and transport.
- Twenty directional pixel sprites: each model has shallow/steep left,
  straight, and shallow/steep right headings. The diagonal roads select the
  heading closest to their path; the straight sprite is available for future lanes.
- Eight engine phases: pulsing underbody plasma and travelling exhaust packets.
- Enter/cruise/exit intervals, deterministic from the saved combat clock.
- Existing BG1 lower-road composition, masked by the original asphalt material.
  Bridge supports, native object priority and HUD composition still apply.

## Placement and spacing

Eight explicit world-space far/near paths are traced against the original
intro-stage road art at its two diagonal junctions. Cars use fixed-point
interpolation on these paths. The camera only transforms their final position;
visible road fragments never choose their trajectories.

Three equally spaced cars share each lane's speed. The smallest separation is
38 world pixels vertically; their maximum scaled 25-pixel sprite canvas is
34 pixels. Tests check this bound over two cycles, including wraparound.
Scale varies continuously from 0.5 to about 1.34 and is rasterized with nearest
pixel sampling. Neon lamps, cyan engines, lit roofs and shaded sides use a
limited palette. No wheel sprites, gradients or anti-aliasing are used.

## Verification

- MinGW desktop build succeeded.
- `mmx_custom_renderer` passed, including lane-line rounding tolerance,
  same-lane separation, bounds, and animation checks for all twenty sprites.
- Private headless game run completed 3,571 frames and exited normally.
- In-game frame captures reviewed at the first diagonal junction; an animated
  six-second capture is supplied separately for visual feedback.
- Gameplay, player art, damage, sword timing, controller setup and AI are unchanged.

## Quality assessment

**7/10 against the X4–X6 target, subjective and provisional.** The traffic now
tracks the diagonal roads and has perspective, directional silhouettes and
multi-phase engines. It does not yet match those games' polished sprite animation:
body silhouettes remain rigid during cruise, distant models lose detail at
native resolution, and depth scaling changes in whole-pixel increments.
The conservative material mask also clips cars at lane paint as well as supports.
The paths are straight approximations of the roads' perspective, with no
animated steering/banking transitions. These are the remaining visual gaps.

Task 2 (ships) and Task 3 (dodge behavior) await feedback on Task 1.

## Run locally

Close the previous game and double-click **Start Cosmic v24.cmd** from the
task's outputs folder. This launches the versioned executable with the existing
controller/runtime and companion-recovery environment.

MSYS2 MINGW64, from the existing checkout with generated ROM code:

```bash
cd /c/Users/MEEzi/Documents/Codex/2026-10-12/continue-developing-my-mega-man-x/work/MegaManXSNESRecomp
git switch experiment/hover-traffic-v24-20261013
cmake --build build-recompui --parallel 4 && bash tools/launch_cosmic.sh
```

The branch is local until a GitHub push succeeds; fetching cannot retrieve an
unpublished branch. No merge into main is part of this iteration.

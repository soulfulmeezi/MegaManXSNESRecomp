# Cosmic X and Zero v20

Branch: `experiment/cosmic-powers-v20-20261013`, based on the approved
Earth X armor in `ddaf459`. No main-branch merge.

## Controls

- X: hold Down and tap Shoot while grounded with the buster selected to send
  an Earth shockwave along the floor. It stops at gaps and walls.
- Zero, X3 behavior: Down + Shoot starts the Jupiter sword combo. Tap Shoot
  again during each swing to queue the next. Three swings increase in strength;
  the third releases a traveling storm crescent. Holding Shoot does not chain.
- Zero, Modern behavior: Shoot starts the same three-swing combo directly.
- L2 still switches the controlled character in CPU companion mode.

## Changes

- X's resolved attack damage increases by 175% (2.75 times), rounded to the
  nearest whole HP and capped at 127. Immunity and special response values are
  preserved. Zero retains his previous 3x modifier.
- Each storm sword swing hits a victim once. Base contact strengths are 3, 4,
  and 6 before Zero's modifier and native enemy immunity. New traveling effects
  use the existing projectile pool, collision hooks and per-owner save snapshots.
  Two cosmic projectiles can coexist per character.
- Earth armor gains thin white cloud streaks; its connected continents remain.
- Highway BG2 gains drifting cloud wisps, distant headlights and changing
  windows. Native foreground, actor and HUD priorities still apply.
- The launchers enable the existing `MMX_CPU_CRUMBLE`, `MMX_CPU_PIT_WALL` and
  `MMX_CPU_WALL_ARC` recovery routes. Their omission from the recent Windows
  launcher was found while investigating Zero waiting after a collapse.
  The reported collapse still needs a player retest; this does not establish
  that every edge case is resolved. No new camera or companion algorithm.

## Build and launch in the existing MSYS2 MINGW64 checkout

```bash
cd /c/Users/MEEzi/Documents/Codex/2026-10-12/continue-developing-my-mega-man-x/work/MegaManXSNESRecomp
git switch experiment/cosmic-powers-v20-20261013
cmake --build build-recompui --parallel 4 && bash tools/launch_cosmic.sh
```

The existing generated code and verified local ROM are required. No ROM or
extracted sprite data is committed. On a fresh checkout, first follow the
repository's ROM regeneration/setup instructions. Fetching this branch from
another computer is possible only after it has been pushed successfully.

## Verification

- MinGW build and generated-code hook verification.
- Nine ROM-free CTest suites, including combo timing, cancellation, restore,
  finisher emission, immunity, damage rounding, projectile ownership, ground
  gaps/walls, slot limits, cloud masks and city animation predicates.
- Private headless visual smoke checks for X and Zero using isolated config
  and save paths. Controller input and the reported collapsed-platform route
  need normal desktop playtesting.

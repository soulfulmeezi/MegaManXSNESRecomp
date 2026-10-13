# Cosmic impact v21

Branch: `experiment/cosmic-impact-v21-20261013`, continuing v20.

- Earth shockwave: a taller jagged stone ridge, cyan-white energy rim and
  trailing dust/debris. The expanded trail is cosmetic; the damaging core and
  terrain behavior retain v20's collision bounds.
- Attack lighting: Earth casts cool light; Jupiter casts warm light. Scenery
  dims briefly around each active attack, with a stepped local light radius
  and fade-out. Both co-op actors contribute. All timing comes from saved
  attack state. HUD, menus, native fades and color windows retain their rules.
- Zero's combo has three visual choreographies: forward cut, rising backswing,
  and heavy overhead cut. They reuse the original ground/air body and blade
  frames with different sequences. Damage, contact timing and controls remain
  as in v20; these are three variations, not newly extracted game animations.
- Highway BG2 includes moving emergency vehicles, staggered small explosions,
  smoke and falling sparks below the road. Native pillars and terrain obscure
  these background events. They do not spawn enemies or change stage geometry.

## Play

X: Down + Shoot on the ground. Zero: Down + Shoot, then a fresh Shoot press
during each swing to queue the next. Modern Zero starts directly with Shoot.
Hold L2 to switch characters with CPU companion enabled.

The supplied **Start Cosmic v21.cmd** uses a separate v21 executable in the
existing build directory so a locked v20 executable need not be overwritten.
Close the previous game normally before starting v21 to avoid sharing inputs
or saves between two running instances.

## Rebuild in MSYS2 MINGW64 after closing the game

```bash
cd /c/Users/MEEzi/Documents/Codex/2026-10-12/continue-developing-my-mega-man-x/work/MegaManXSNESRecomp
git switch experiment/cosmic-impact-v21-20261013
cmake --build build-recompui --parallel 4 && bash tools/launch_cosmic.sh
```

The existing verified ROM, source cache and generated code are reused. No
controller, companion AI, camera, damage or gameplay rules are changed by v21.

## Checks

The Zero character and custom-renderer suites cover distinct combo frame
sequences, expanded effect pixels, bounded lighting, lower-street animation,
and existing combat behavior. The Windows executable is linked using the
normal CMake response file and flags, then checked with isolated headless
X and Zero runs. The original output executable was locked at link time.

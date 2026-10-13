# Animated cosmic waves — v23

Branch: `experiment/cosmic-flow-v23-20261013`.

Earth's projectile now has a rolling crest, moving interior ribbons, tumbling
fragments and a flowing ground wake. The solid, fixed triangular mass is gone.
Jupiter's crescent expands, bends and contracts through eight shape phases,
with streaming highlights, a broken secondary trail and sparks peeling away.
Both effects change their silhouettes at a fixed origin while also traveling.
Animation is driven by the saved projectile age, so pauses and rewind stay
consistent. The collision cores, damage, travel speed and v22 combo are retained.

Lighting is limited to a small elliptical footprint (up to 12 pixels sideways
and 24 vertically from its center), with reduced intensity. Earth's light is
centered up inside the crest. Global scene dimming has been removed.

## References and approach

Online references located during this pass:

- [X4 animated-sprite archive](https://megaman.retropixel.net/mmx/4/sprites.php)
- [X5 Grizzly Slash projectile sheet](https://www.spriters-resource.com/playstation/mmx5/asset/46009/)
- [X5 Mattrex effects sheet](https://www.spriters-resource.com/playstation/mmx5/asset/86323/)

Direct GIF playback/download was unavailable in this environment; these are
reference links, not a claim of frame-accurate X4–X6 reproduction. The new
effects are original procedural pixel animations; no web sprites are bundled.

## Build and review

Standard MinGW build and the Zero/custom-renderer suites pass. New assertions
check changing silhouettes, deterministic frames and the tight lighting limit.
Private X and Zero smoke runs check the actual game renderer. The supplied
`Cosmic flow v23 animation.gif` renders the real effect functions at fixed
origins, making their internal animation visible without projectile movement.

Close the previous game, then run **Start Cosmic v23.cmd**. Controls are the
same: X Down + Shoot; Zero Down + Shoot followed by fresh Shoot presses.

```bash
cd /c/Users/MEEzi/Documents/Codex/2026-10-12/continue-developing-my-mega-man-x/work/MegaManXSNESRecomp
git switch experiment/cosmic-flow-v23-20261013
cmake --build build-recompui --parallel 4 && bash tools/launch_cosmic.sh
```

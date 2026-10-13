# Lower-road traffic and fast saber — v22

Branch: `experiment/cosmic-traffic-v22-20261013`.

- Cars now travel on the dark green lower expressways, including diagonal
  road sections. The renderer samples actual road spans at each vehicle's
  depth, sizes vehicles with perspective, and clips them behind lane paint,
  bright support pillars and native foreground objects. There are opposing
  flows, colored bodies, windshields, headlights and red rear lights.
- Zero's storm sword timeline advances twice per simulation tick. Startup
  reaches the first contact in three updates instead of six. Fresh follow-up
  presses can buffer earlier. This is an X5-inspired speed adjustment using
  the existing SNES poses, not an exact X5 frame-data reproduction.
- Three swing variations, finisher emission, damage values and one-hit-per-
  victim rules remain. Holding Shoot does not automatically chain swings.
- Existing Down + Shoot and L2 switching controls are unchanged.

Build: the standard MinGW CMake target succeeds. The Zero character and
custom-renderer suites pass, including startup/chain timing, road selection
and directional car pixels. Private X/Zero smoke runs use isolated configs.

Use **Start Cosmic v22.cmd** for the supplied versioned executable. Close
the previous game normally before launching another instance.

To rebuild in the existing MSYS2 MINGW64 checkout:

```bash
cd /c/Users/MEEzi/Documents/Codex/2026-10-12/continue-developing-my-mega-man-x/work/MegaManXSNESRecomp
git switch experiment/cosmic-traffic-v22-20261013
cmake --build build-recompui --parallel 4 && bash tools/launch_cosmic.sh
```

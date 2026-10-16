# Cosmic X Reference Evolution — experimental build

This branch combines the preserved v33 work, sky/Earth Wave correction,
CosmicFX atmosphere/weapon accents, and Zero saber choreography. It adds
original synthesized sound and movement feedback. X Earth and Zero Jupiter
base designs are preserved, with a new optional material finish; native collision, SNES timing, save layouts,
controller ownership and dependency pins are unchanged.

## Included

- Armor finish: turquoise Earth coastlines, cloud relief shadows, and
  Jupiter plate-edge lighting. Planet maps, storm centers, silhouettes,
  skin, hair, eyes and undersuits retain their existing identity.

- Three depth-separated cloud planes with faster independent drift, plus
  Highway gusts. Background effects stay behind native terrain and actors.
- Existing road-masked lower Highway cars enabled; debug-colored lane lines
  disabled. Existing measured straight-lane geometry is retained. Curved-lane
  expansion is not implemented in this update.
- Both sets of original Earth/Jupiter projectile accents enabled; native
  projectile hitboxes and the exclusive Earth Wave input fix are retained.
- Zero's saber-only choreography, distinct ground/aerial/downward poses,
  visible arcs and preserved aerial steering from the existing experiment.
- Original PCM energy whoosh at the active swing edge, with combo-dependent
  pitch; original impact on a sustained-airtime landing. Independent co-op
  cue state; no per-frame sound retriggering or teleport landing sound.
- Draw-only dash dust and paired landing accents for X and Zero, behind
  the character sprites. No new damaging objects or physics changes.

## Reference provenance

Analyzed user-supplied `Mega Man X8 16-bit 1.0.0.9.exe`, SHA-256
`513b7d0c0fc3e32f33f56e76394603a2dc5dc7e2f817465a9dea1bd0aae397d7`.
Its embedded Godot 3.5.3 pack has 5592 checksum-verified resources.
MetalValley's authored Sun/City/Sand/Cliffs parallax scales were
(0.025,0), (0.15,0.05), (0.2,0.05), (0.5,0.25); its destructible-rock
scene combines one-shot particles and positional audio. Those architectural
ideas informed this integration. No extracted artwork, sound, bytecode,
scene layout, ROM or generated game source is included. This update does not
claim to convert the Godot levels into the SNES engine or reproduce their
runtime behavior. New collision layouts and complete replacement stages
still require separate authoring and gameplay validation.

## Build on Windows

Run in MSYS2 MinGW64, using a NEW destination:

```bash
git clone --branch upgrade/cosmic-x-reference-evolution-20261015 --recurse-submodules https://github.com/soulfulmeezi/MegaManXSNESRecomp.git ~/CosmicX_Evolution
cd ~/CosmicX_Evolution
bash tools/build_cosmic_evolution.sh ~/CosmicFX_Test
cd build-evolution
./MegaManXSNESRecomp.exe
```

The helper reads your existing X1 ROM, generates game code locally, builds,
runs CTest and copies available configuration/cache files only when absent.
It never edits the reference folder or existing v33 build. If launcher source
paths need updating, select your own X1 and X3 ROMs. No Windows EXE is supplied
by this change; the source must be compiled on your machine.

## A/B controls

All these enhancements default on; set an individual variable to `0` before
launching to compare: `MMX_CLOUD_FX`, `MMX_COSMIC_SKY_MOTION`, `MMX_WEAPON_FX`,
`MMX_COSMIC_UPGRADE_VFX`, `MMX_MOVEMENT_FX`, `MMX_NEAR_ROAD_CARS`, `MMX_ZERO_SFX`, `MMX_ARMOR_FINISH`.
Set the armor toggle before starting the game so both character paths agree.
`MMX_TRAFFIC_DEBUG_LANES=1` restores diagnostic lane lines. The host audio
mixer shares master volume/mute. New Zero audio is offline-only pending
confirmed-event scheduling for online rollback. These cues do not change
serialized state. A discontinuity, reset or source change primes the observer
silently rather than manufacturing a hit or landing.

## Validation

Local ROM-free GCC tests (13 programs passed): renderer/combat (including Earth Wave exclusivity),
Zero asset/state/combo/choreography tests, Cosmic core, cloud planes, weapon
accents, movement effects and PCM cue timing. Armor tests cover material protection, coastline hue, section bounds and
unchanged storm centers. The full renderer test also passes with the finish.
Main host source syntax checks pass for USA and JP using SDL headers.
The new audio test checks
waveform bounds/endpoints, repeated frames, rapid combos, second actor,
airborne landing, grounding jitter, mute, stage changes and reset generation.
The inherited Zero test needed its old ceiling-query fixture cleared before
starting the open-space saber test; collision code was not weakened.

Full local desktop build is blocked by absent SDL/CMake dependencies and
restricted system package installation. CI includes the new test targets.
Windows gameplay, road visibility, actual sound mix, shader behavior and
60 FPS frame pacing are NOT yet verified. No frame-rate claim follows from
unit tests.

Test the Intro Highway at camera X 0x05B7, 0x0718, 0x0FCE, 0x148F. Check
lower-car visibility and pillar occlusion; compare standing still and dashing.
Test Zero misses/hits, three-hit combos, aerial down slash, landing while
attacking, pause/resume and save/load. Hold normal fire while Earth Wave is
active, then verify it resumes after the wave ends. Keep a matched recording
of baseline and candidate; inspect frame-time spikes with effects overlapping.


## Jupiter Skyline r2

The highway travel section now uses an original procedural BG2 scene: a large
banded Jupiter, star field, two parallax city depths and a moving
shuttle. Native foreground terrain, sprites and HUD retain their priority.
The airship/final arena and windowed dialogue retain native backgrounds.
`MMX_ORBITAL_BACKGROUND=0` restores the earlier backdrop.
Earth X and Jupiter Zero armor identities remain intact.

Swing synthesis now has more noise/body and 100% voice gain (previously 75%).
`MMX_AUDIO_TRACE=1` prints actor/frame/clip/voice for each triggered swing.
`MMX_ZERO_SFX=0` still mutes these cues; online custom audio remains disabled.
The window title includes **Jupiter Skyline r2** to identify the executable.

Validation: renderer regression suite, environment tests, feedback tests, and
USA main syntax check passed. A harness linked against the real pinned PCM
mixer produced a swing peak of 11364/32767. This verifies generated PCM and
mixer delivery, not the user's Windows device or music balance. Full Windows
build/gameplay listening remains unverified; no confirmed cause for the
recording's missing swing audio has been established.

Update the existing evolution checkout from MSYS2 MinGW64:

```bash
cd ~/CosmicX_Evolution
git pull --ff-only origin upgrade/cosmic-x-reference-evolution-20261015
bash tools/build_cosmic_evolution.sh ~/CosmicFX_Test
cd build-evolution
MMX_ZERO_SFX=1 MMX_AUDIO_TRACE=1 ./MegaManXSNESRecomp.exe
```

### Follow-up visual fixes

Removed the diagonal purple nebula hatching after gameplay review. The orbital
background now stays present through all six character-swap phases; palette
fades use the common compositor instead of revealing the original background.
Renderer regression tests cover swap continuity with synthetic Zero assets.

### Detailed lower-road vehicles

Replaced the luminous strip sprites with original compact sedan, cargo truck,
and sports car pixels: framed cabins, shaded panels, steel bumpers and rotating
hub highlights. Five body palettes, both directions and existing lane motion
remain. Sprite bounds stay 12/15/14 by 8 pixels to preserve road masks.
The thruster halo is inert for these wheeled vehicles. Renderer tests passed;
an enlarged sprite preview was inspected. In-game scale/occlusion review remains.

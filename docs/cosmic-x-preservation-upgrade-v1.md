# Cosmic X preservation-first upgrade, Phase 1

Status: standalone C11 prototype and upgrade plan, **not yet a playable game change**.

## Why the branch starts here
This branch derives from published experiment/zero-jupiter-planet-shell-v17-20261012, not main. That published branch contains advanced companion work and Jupiter Zero material-mask options. Preserve its complete source history.

**Critical local-work safeguard:** the owner's more recent local v23/v24 art/highway experiments do not appear to be published on GitHub. The v23 X wave, Zero crescent and localized glow and the v24 four moving vehicles, lane alignment, spacing and remaining clipping work should not be lost. A previously reported local short commit 611e316 was not found by the GitHub commit API. These local results have not been revalidated here.

Before a merge or local checkout reset, preserve the owner's current PC checkout. On the PC inspect git status --short, git branch --show-current, git log -5 --oneline --decorate and git submodule status --recursive. Review any uncommitted edits, make a deliberate backup commit on a safe branch if needed, and publish the completed v24 branch to a UNIQUE remote backup ref. Do not force-push, run git reset --hard or clean untracked files. Never upload ROMs, extracted artwork, private saves, recordings, generated game code or keys. Only then compare its graph against v17 and this new branch; reconcile on a new integration branch.

## Preserve these systems

| Existing investment | Acceptance requirement |
| --- | --- |
| Native player physics and X/Zero combat | No changes to default inputs, guest timing, collisions or damage from this PR |
| CPU companion, anti-stuck, wall and pit recovery | Retain prior fixes and owner-validated routes; new combat cannot override survival |
| Adaptive widescreen and stage compositor | Keep the existing renderer, world-column colors, HUD and shaders; avoid palette/scroll regressions |
| Jupiter Zero v17 | Retain its 152-pose material masks, preserved face/hair, visuals/fallback flags |
| Local v23/v24 artwork, cars and road tests | Treat local content as potentially newer than GitHub; backup and compare before porting |
| Weapon inventory and multiplayer save/rollback | No MmxCoopState ABI changes, new save formats or silent controller rewrites |
| Graphics pacing and audio | Preserve decompressor-yield fix, native guest frame clock and matched-route tests |

## Actual first implementation

- src/mmx_cosmic_core.h / src/mmx_cosmic_core.c: original deterministic, ROM-independent Constellation core: 12 proposed Aspect identifiers, unlock bitmask, optional specialization, two equipped aspects, event-driven Resonance and the first curated Storm+Gravity fusion gate with cooldown.
- tests/mmx_cosmic_core_test.c: six synthetic unit scenarios.
- CMakeLists.txt: registers only an isolated mmx_cosmic_core_test / CTest target. Neither USA nor JP playable executable compiles or links the new core yet. Existing game logic, rendering, sprites, controls and save layouts are untouched.

The exact three C files committed on GitHub matched local Git blob hashes. An isolated test using C11, warnings-as-errors, AddressSanitizer and UndefinedBehaviorSanitizer passed: Cosmic Constellation prototype: 6 synthetic tests passed. This validates only self-authored unit logic, NOT playable powers, full game compilation, old-save compatibility or native runtime pacing. Full repository CTest, Windows builds and owner playtests remain pending.

## Next stages and approval gates

**Gate 0 — local safety:** recover and backup the latest local v24 branch; compare both the lane placements and the original art/companion behavior. Do NOT merge this draft first.

**Gate 1 — one working power:** on the correct reconciled gameplay branch, opt-in Storm shot + Gravity field + a finite original Thunderwell attack. First write tests for attack ownership, hit cap, energy, cooldown, input edges, actor swap, save compatibility and rewind. Only after that wire it into gameplay. Reference original skills in draft PR #4 (https://github.com/soulfulmeezi/MegaManXSNESRecomp/pull/4).

**Gate 2 — Highway upgrade:** retain the latest local moving-car geometry and v23 glow. Measure reference footage rather than guessing scroll speeds. Prioritize the LOWER road near camera X 0x05B7, 0x0718, 0x0FCE, 0x148F, and check curved-road centerlines, pillar masking, clipping and widescreen. Original placeholder cars first.

**Gate 3 — intelligence and art:** improve grounded Zero AI reactions to unstable platforms and safe wall routes without changing existing physics; create original stage/sprite detail. Keep integer 5x display scaling separate from truly new artwork.

**Gate 4 — presentation and acceptance:** compare the owner-valid Highway movement route with separate simulation/render/presentation/audio timing. Record real p50/p95/p99/max and missed 60Hz intervals. Test the original rendering path, co-op, AI survival, old saves and Jupiter poses against actual gameplay.

## Limits
This PR does not enable an in-game power, install a package, process a ROM, touch copyrighted assets, push local v24, claim 60fps, or merge or deploy anything. PolyForm Noncommercial and third-party IP require separate review before commercial distribution.

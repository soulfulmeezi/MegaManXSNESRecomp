# Cosmic X specialized agent skill library

This branch contains five original, non-executable game-development skills. They are instructions for compatible coding agents, not installed programs, art assets, a new renderer or proof of improved gameplay. Root AGENTS.md provides routing when supported; any agent can be explicitly asked to read the individual SKILL.md.

| Order | Skill | Use for | Output |
| --- | --- | --- | --- |
| P0 | [Video reference analyzer](../.agents/skills/cosmic-x-video-reference/SKILL.md) | User-provided reference footage | Timestamped vehicle and background-motion measurements |
| P0 | [Highway traffic/parallax](../.agents/skills/cosmic-x-highway-traffic/SKILL.md) | Cars on both Highway roads, lower-road priority | World-coordinate lanes and original placeholder vehicle design/tests |
| P1 | [Smooth frame pacing](../.agents/skills/cosmic-x-frame-pacing/SKILL.md) | Visible stutter and 60Hz presentation | Reproducible p50/p95/p99 timing and bottleneck validation |
| P2 | [HD art pipeline](../.agents/skills/cosmic-x-hd-art-pipeline/SKILL.md) | 5x pixel scaling and more visual detail | Separate upscale vs redesigned-original-art plan |
| P2 | [Zero companion AI](../.agents/skills/cosmic-x-companion-ai/SKILL.md) | Follow, jump, wall climb, collapsing floors, combat | Deterministic controller-decision regressions |

## How to use immediately
Ask a compatible agent to read a path and apply its instructions. For example:

**Reference video:** Read .agents/skills/cosmic-x-video-reference/SKILL.md and analyze the recording I attach. Report timestamped measured car speeds and parallax, not contact-sheet guesses.

**Road traffic:** Read .agents/skills/cosmic-x-highway-traffic/SKILL.md. Start with the lower road, validate road lane geometry near cameras 0x05B7, 0x0718, 0x0FCE and 0x148F, and propose draw-only original vehicle fixtures. No gameplay edits yet.

**Frame pacing:** Read .agents/skills/cosmic-x-frame-pacing/SKILL.md and the historical Highway report. Diagnose only from the checked-out branch and propose instrumented acceptance checks.

**Art:** Read .agents/skills/cosmic-x-hd-art-pipeline/SKILL.md and specify one original traffic car sprite strip while preserving consistent anchors and existing native scaling.

**Companion:** Read .agents/skills/cosmic-x-companion-ai/SKILL.md on the actual companion branch. Reproduce a collapsing-platform failure and write test cases before modifying AI behavior.

## Ordered integration plan
1. Establish the exact active project branch and receive authorized reference media. Run video measurement before hard-coding arbitrary car speeds.
2. Prepare a separate gameplay branch and obtain user approval. Verify world-space road masks/curves and lower-road visibility using synthetic debug overlays.
3. Introduce draw-only placeholder cars, tune parallax and test both original and adaptive-width renderers with moving captures.
4. Compare matched Highway gameplay runs, preserve guest timing and measure host presentation intervals. Investigate before applying performance fixes.
5. Iterate original stage art, maintaining a nearest-neighbor fallback; separately develop Zero companion fixes on the branch with existing Zero AI.

## Existing research and external candidates
The other [Cosmic X research draft PR #3](https://github.com/soulfulmeezi/MegaManXSNESRecomp/pull/3) separately provides a cosmic-x-analysis skill and [researched tooling/license notes](https://github.com/soulfulmeezi/MegaManXSNESRecomp/blob/codex/cosmic-x-research-skills/docs/cosmic-x-tooling-plan.md). Those are not on main yet and are not required for these five skills. Generic upstream systematic-debugging, test-driven-development, SNES debugger and profiler projects are **not copied, installed or run** by this change.

## Scope and validation
This change intentionally consists exclusively of Markdown instruction files. No game code, scripts, assets, ROMs, submodules, pipeline configuration or dependencies are modified. No video was opened, no performance number was measured and no game build was run. All proposed acceptance tests still need explicit implementation and validation.

The game repository is licensed under PolyForm Noncommercial 1.0.0; review commercialization and third-party IP separately. A tool license does not authorize copied proprietary game content. Future additions require exact version and license verification before installing or adapting any third-party skill/tool.


## Powers and bosses: four more skills from the additional reference footage

The supplied ~79.6-second 30fps gameplay montage was reviewed using individual frames and contact sheets; the [combat reference evidence log](cosmic-x-combat-reference-2026-10-14.md) lists scene timestamps, observations and unknowns.

| Priority | Skill | Trigger | Expected result |
| --- | --- | --- | --- |
| P1 | [Power systems](../.agents/skills/cosmic-x-power-systems/SKILL.md) | Charged shots, projectile volleys, new attacks | Original power contract, energy/damage behavior and state-based tests |
| P1 | [Combat mobility](../.agents/skills/cosmic-x-combat-mobility/SKILL.md) | Running, dashing, jumping and shooting together | Explicit character action-state transitions and timing tests |
| P1 | [Boss patterns](../.agents/skills/cosmic-x-boss-patterns/SKILL.md) | Aerial enemies, sword moves, projectile fields, phase shifts | Fair telegraphed attacks, deterministic phase/encounter checks |
| P2 | [Combat VFX](../.agents/skills/cosmic-x-combat-vfx/SKILL.md) | Lightning, fire, energy glow, slash trails, impact flashes | Original VFX with z-order, readability, accessibility and performance budgets |

Suggested combat vertical slice after separate gameplay authorization: prototype one original regular/charged energy power and one synthetic training enemy with a readable, avoidable telegraph; add lightweight impact VFX, then profile the resulting frame pacing. Do not add exact attacks, code, or assets copied from the reference game.

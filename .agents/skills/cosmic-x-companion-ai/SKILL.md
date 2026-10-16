---
name: cosmic-x-companion-ai
description: Plan and test deterministic Zero companion safety, collapsing-platform escape, wall climbing, following and context-sensitive combat using native collision and controller input.
---

# Cosmic X Zero companion intelligence

Use on the actual branch containing CPU Zero companion behavior, potentially feature/cpu-companion and pit/wall traversal experiments. Do not assume main includes those changes. Read src/mmx_coop.c, src/mmx_coop.h, src/mmx_coop_trace.c, src/mmx_zero.c, tests/mmx_coop_test.inc and the tests specific to the current branch. Review PR #2's pit/wall traversal evidence before extending it.

## Decision loop
1. Observe X and Zero world position, velocity, facing, grounded state, nearby collision shapes, safe landing headroom, wall contact, platform stability, attack threats and character variant. Treat sensor visibility as a coded feature to verify, not an invented capability.
2. Predict bounded trajectories for follow, jump, wall climb, retreat or safe landing using the game's collision/physics rules. A breakable platform that still looks solid needs a detectable triggered/cracking/collapsing state or a documented conservative proxy.
3. Prioritize avoiding imminent death, then maintaining safe footing, recovering/following X, fighting and facing animation. Do not let aggressive attack logic override ledge survival.
4. Output native controller actions (including jump release/repress edges), not arbitrary teleport, health writes, forced movement velocity or actor ownership changes.
5. Bound repeated rescue attempts, stuck detection and wall loops. Distinguish X3 charge-buster behavior from optional Modern saber/air-dash capabilities and boss-specific charge choices.

## Required regression scenarios
- Platform breaks when X steps on it; when Zero steps on it; already broken; nearby stable platform.
- Valid wall jump, left/right wall, blocked headroom, no valid landing, one-way floor, slide toward a pit, jump held versus released.
- Following across separated platforms, sudden X direction change, camera separation, miniboss/boss attack choice, ordinary enemy quick shots, threats during charge.
- Death/respawn and player handoff, deterministic save/load/rewind, no impact on human seat controls.

Log frame, position, observed ground/collision state, chosen button state, intended landing, result and failure classification. Mark whether a scenario was statically reviewed, synthetically tested or actually witnessed in gameplay. Keep experimental changes opt-in until owner-validated. No gameplay files should change merely by loading this skill.

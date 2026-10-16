# Cosmic X agent routing

Read CONTRIBUTING.md before writing game code. Select the most relevant original agent skill from .agents/skills/; see docs/cosmic-x-skill-library.md for paths, prompts and the recommended order. When the separate research PR is available on your branch, consult its cosmic-x-analysis skill as well.

Respect the checked-out branch: experimental Zero AI and graphics work are not necessarily merged into main. Preserve native guest timing, co-op controller ownership, collision, rollback and save layouts, shader support, and pinned framework submodules.

A previous conversation's video reference does not imply its file is accessible. Never invent measured traffic speeds, road geometry, sprite dimensions or 60fps results. Clearly label proposed tests versus tested behavior.

For skill/documentation work, never access, extract, commit, download or redistribute copyrighted ROMs, game artwork, generated game code or private save states. Use original synthetic fixtures. Do not install or execute unknown third-party code or run build/extraction scripts without explicit scope approval. Keep gameplay edits, deployments and merges behind user approval.

Make changes on a dedicated branch/draft PR; report exact paths, license considerations, test outcomes and tests not performed.

## User-directed visual quality baseline

LaMarr requests an 8/10 quality baseline once achieved and verified in gameplay.
Do not label an unreviewed implementation 8/10 from code tests or a still preview.
Evaluate readable 16-bit artwork, purposeful ability motion, gradual effect
entry/exit, terrain/foreground placement, audiovisual timing, stage storytelling,
and measured frame pacing. Once the user accepts a qualifying build, preserve
its exact commit/build as the reference. Future mods must meet or improve that
baseline; keep unfinished experiments separate and disclose regressions.

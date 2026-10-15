# Cosmic X tooling and agent-skill integration

Research date: 2026-10-14 (America/Los_Angeles). Scope: source/document review only; no third-party execution, ROM access, extraction, runtime validation, or deployment.

## Repository identification and inspected baseline

[soulfulmeezi/MegaManXSNESRecomp](https://github.com/soulfulmeezi/MegaManXSNESRecomp) is the strong match for Cosmic X: it contains the owner's CPU-companion, traversal, and palette experiments. No separate repository named Cosmic X was visible in the signed-in account's repository list. The product name itself is not confirmed by the main README.

Inspected main: `fc6f9ad5209307e29528951c09513dea9b149caa`. The research branch starts here and does not combine experimental gameplay branches. Relevant work: [CPU companion PR #1](https://github.com/soulfulmeezi/MegaManXSNESRecomp/pull/1), based on main, and [pit/wall traversal PR #2](https://github.com/soulfulmeezi/MegaManXSNESRecomp/pull/2), based on `feature/cpu-companion`. Their descriptions explicitly distinguish authored tests from gameplay validation. Do not treat open experiments as accepted fixes.

Existing integration points:

| Area | Existing paths | Recommendation |
|---|---|---|
| Build and recompilation | `CMakeLists.txt`, `recomp/bank*.cfg`, `tools/regen.sh`, `.gitmodules` | Keep the pinned framework; do not regenerate code in a tooling-only PR. |
| Asset formats | `tools/extract_zero.py`, `tools/extract_x_weapons.py`, `src/mmx_source_assets.cpp`, `src/mmx_render_assets.h` | Extend existing parsers with synthetic fixtures; no parallel asset pipeline. |
| Tiles and palettes | `tools/oam_parse.py`, `tools/vram_region.py`, `src/mmx_render_assets.h` | Track palette ownership, tile layout, transparency, and VRAM word/byte units explicitly. |
| Background motion | `src/mmx_renderer.c`, `src/mmx_wide_policy.c`, `docs/custom-renderer-spike.md` | Preserve native PPU/compositor boundaries and world-column palette selection. |
| Physics and companion | `src/mmx_coop.c`, `src/mmx_zero.c`, `tests/mmx_coop_test.inc` | Extend native controller decisions and collision queries on the companion branch. |
| Performance | `docs/highway-pacing-2026-09-30.md`, `tools/scripts/highway-coop-pacing.txt` | Reuse the gameplay-gated route and separate simulation, render, and presentation timing. |
| Tests and CI | `.github/workflows/build.yml`, CTest targets for display, widescreen, renderer, saves, Zero | Existing CI configures a ROM-free setup host; it is not full gameplay coverage. |

Framework gitlink: `7931b53c4968d9cf3fffeabf3e7f5f21d2ecf3a0`; UI gitlink: `7e884a227accea91ddb378671bd49aaeeea13371`. The pinned framework already contains GLSL presets and a shader loader. No new graphics dependency is justified yet.

## Stand-alone tools: ranked candidates

These are applications/libraries, not agent skills. Maintenance dates below are default-branch commit dates in UTC, not certification of security. Exact reviewed heads and metadata are in `cosmic-x-tool-evidence.json`.

| Priority | Tool and license | Evidence / fit / limitation |
|---|---|---|
| P1 | [MesenCE](https://github.com/nesdev-org/MesenCE), GPL-3.0-or-later | Community successor to archived SourMesen/Mesen2; head 2026-10-07. Windows-compatible SNES debugger reference. Keep external to the game; establish a trace adapter before assuming log compatibility. |
| P1 | [DiztinGUIsh](https://github.com/DizTools/DiztinGUIsh), GPL-3.0 | Head 2026-08-09. SNES disassembly, code/data annotation and trace import. Useful for 65816 M/X-width and bank-state analysis. Its documented integrations include BSNES/BizHawk, not a promise of direct MesenCE import. |
| P1 | [Tracy](https://github.com/wolfpld/tracy), BSD-3-Clause | Head 2026-10-15 UTC (Oct 14 Pacific). C/C++ frame profiling fits this native host. Add optional zones only after baseline timing identifies the bottleneck; keep profiling disabled in normal builds. |
| P2 | [LibreSprite](https://github.com/LibreSprite/LibreSprite), GPL-2.0 | Head 2026-10-13. External editor for original sprite/palette fixtures. It does not identify game compression or perform SNES extraction automatically. |
| P2 | [Kaitai compiler](https://github.com/kaitai-io/kaitai_struct_compiler), GPL-3.0; [C++ STL runtime](https://github.com/kaitai-io/kaitai_struct_cpp_stl_runtime), MIT | Compiler head 2026-05-04; runtime head 2026-08-22. Optional declarative format documentation when an existing extractor becomes difficult to maintain. Compiler, runtime, and format definitions have distinct licensing scopes. |
| P3 | [Asar](https://github.com/RPGHacker/asar), LGPL-3.0-or-later | Default head 2025-09-21, repository push activity 2026-08-20. Slower maintenance signal; useful only for synthetic 65816 assembly round-trip experiments. Not needed to build the existing C recompilation. |
| Hold | [libretro GLSL shaders](https://github.com/libretro/glsl-shaders) | Head 2026-10-11. Upscaling/filter research only: no blanket repository license was established. Inspect the selected shader's license and host GLSL/preset support before copying anything. Existing sharp/CRT presets and integer scaling come first. |

The [SNESRecomp framework](https://github.com/RetroPortingToolKit/snesrecomp) is already a dependency, not an additional skill to install. The title's [LICENSE](https://github.com/soulfulmeezi/MegaManXSNESRecomp/blob/main/LICENSE) specifies PolyForm Noncommercial 1.0.0; do not describe the game repository as permissively licensed open source. A tool license does not grant rights to ROMs, extracted assets, or translated game code. Keep GPL editors/debuggers as separate development tools; copying/linking their implementation requires a separate compatibility review. Preserve notices for any distributed dependency.

## Actual agent skills

Reviewed Trail of Bits head: `e1f86a390d4caf28bf1c377a7d86707b2898669c`, commit 2026-10-14 UTC, [CC-BY-SA-4.0](https://github.com/trailofbits/skills/blob/e1f86a390d4caf28bf1c377a7d86707b2898669c/LICENSE).

- [property-based-testing](https://github.com/trailofbits/skills/blob/e1f86a390d4caf28bf1c377a7d86707b2898669c/plugins/property-based-testing/skills/property-based-testing/SKILL.md): best candidate for decoder invariants and synthetic palette tests. Entry point reviewed; its referenced resources still require review before vendoring. Preserve attribution/share-alike for adaptations. Do not add a testing dependency without a concrete test need.
- [audit-context-building](https://github.com/trailofbits/skills/blob/e1f86a390d4caf28bf1c377a7d86707b2898669c/plugins/audit-context-building/skills/audit-context-building/SKILL.md): useful architecture discipline, but directly invokes Claude plugin workflows and subagents. Not portable as a lone SKILL.md; defer installation.
- [dwarf-expert](https://github.com/trailofbits/skills/blob/e1f86a390d4caf28bf1c377a7d86707b2898669c/plugins/dwarf-expert/skills/dwarf-expert/SKILL.md): useful only for host DWARF builds. Does not decode SNES machine code and does not replace Windows PDB debugging.
- [Mesen2-Diz](https://github.com/danielburgess/Mesen2-Diz) advertises a separate SNES reverse-engineering skill but states it will be released separately when ready. The experimental emulator fork is available; a released, vetted skill was not established. Do not present it as installable.

This change supplies an original, repository-specific `cosmic-x-analysis` skill rather than copying third-party instructions. It has no executable helpers, hooks, installers, remote agents, or network service requirements. Third-party entry points were inspected as data, not followed as instructions. This is a scoped review, not a complete supply-chain audit.

## Implementation sequence and acceptance checks

1. **P0: land the research baseline separately.** Review this plan and the original skill. Keep experimental branch histories separate. Record the actual gameplay branch and target platform before code changes. No dependency updates in this PR.
2. **P1: establish synthetic regression fixtures.** Test 2/4/8-bpp tile interpretation as applicable, BGR555 palette conversion, transparent index behavior, flip flags, bank boundaries, truncated streams and maximum decompressed sizes. Include invalid inputs that must fail predictably. Never use extracted artwork as a fixture.
3. **P1: companion and collision fixes.** Work from `feature/cpu-companion`; preserve native physics and output controller inputs. Cover wall-jump button release/repress, blocked headroom, one-way surfaces, ledges, failed jumps, death handoff and X3 versus Modern Zero capabilities. Verify deterministic decisions and bound recovery attempts. Pit-wall experiments remain opt-in until real gameplay validation is separately authorized.
4. **P1: 60fps pacing.** Reuse the Highway route with gameplay detection and warm-up. Record host/platform, renderer, VSync, audio, build and submodule SHA. Report p50/p95/p99/max and missed presentation intervals, not just average or uncapped FPS. A 60 Hz display budget is 16.67 ms; preserve the configured guest clock rather than forcing SNES simulation to exactly 60 Hz. Compare HLE against LLE for correctness, then profile CPU/render/present separately if necessary.
5. **P2: parallax.** Build an original synthetic background with marked tile columns. Test bidirectional camera motion, wrap seams, fractional camera movement, palette boundaries, menu transitions and adaptive aspect ratios. Preserve per-layer scroll behavior and palette phase without changing collision coordinates.
6. **P2: upscaling.** Start with integer nearest-neighbor output and the existing sharp preset. Compare shimmer, transparency edges, HUD legibility and GPU cost on synthetic artwork. Only then evaluate a specific licensed shader. No AI upscaler dependency or model-weight download is justified in the frame loop.
7. **P3: deeper analysis.** Use MesenCE/Diz externally only when source and existing traces cannot answer a specific question. Record mapping, ROM revision identifier, bank:address, CPU width state, trace provenance and confidence. ROM-backed analysis remains outside the current authorization.

Before any later installation, pin a release/commit, read the selected package and scripts, record its license and expected hash, and inspect download/build hooks and transitive dependencies. Avoid floating install commands. No setup script is included because no external tool needs to execute for this initial documentation change.

## Validation boundary

The source inventory, build definitions, selected headers/scripts, license files and three third-party skill entry points were read. No game build, third-party tool, existing repository script, extractor, ROM, or asset was executed/opened. The repository's full contribution checks need a complete approved development checkout and cannot be represented as passed here. Validate this change with skill-frontmatter checks and `git diff --check`; report later CI results separately from actual playtesting.

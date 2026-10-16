---
name: cosmicx-reverse-engineering
description: Use when studying permitted game executables, disassembly, symbol tables, gameplay recordings, and original game feel to improve Cosmic X behavior without copying proprietary art.
---

# Evidence-led executable/behavior study
- Confirm what is available and legitimately usable: executable and version/hash, permitted inspection scope, debug symbols/logs, screen recordings, and user-owned ROMs/assets when required. Never upload or redistribute game ROMs, copyrighted extracted sprites, or proprietary binaries.
- Begin with metadata, PE sections, imports, strings, symbols, disassembly, call graph and build logs. Use read-only tools (objdump, nm, strings, dumpbin, Ghidra or equivalent) on a copy. Do not claim source is recoverable from a binary without analysis.
- Create a behavior notebook: input -> state transitions -> time in frames -> camera/world coordinates -> observed output. For reference footage, measure native viewport, speed, offsets, parallax layers, animation cycles, hit reactions and scene density. Separate measurements from estimates.
- For the current recomp, anchor findings to symbols and source files actually present (renderer, hotkey handling, stage renderer, special effects, save state). Do not conflate a base GitHub revision with the user's latest Windows folder.
- Use black-box comparison to reproduce techniques, not directly port proprietary code or assets from third-party games. For each hypothesis, provide the evidence and a small repeatable experiment.
- Do not disable security or anti-cheat controls to extract protected content.

Output table: observed behavior, file/frame evidence, hypothesis, native implementation approach, confidence, measured status.

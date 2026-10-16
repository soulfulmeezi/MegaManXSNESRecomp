---
name: cosmicx-level-design
description: Use for Cosmic X highway stage road traffic, city skyline, parallax, stage backgrounds, and reference-footage analysis.
---

# Stage and environment choreography
1. Distinguish user-provided `space-battle-sky.js` city elements from the existing SNES highway tilemap and foreground. Source city consists of four skyline layers with neon windows/billboards, a deck, rooftop beacons, and exactly 14 flying traffic lights. Port `buildCity` behavior, not a vaguely similar replacement.
2. Use exact stage camera positions from the user's test brief when available (0x05B7, 0x0718, 0x0FCE, 0x148F). Verify on-road Y positions through captures and tile boundaries; avoid hard-coded y without proof.
3. Cars on lower roads below the highway are a separate first-class feature from the JS's airborne city traffic. Speed and density should be measured against supplied video, not guessed. Cars must visibly follow correct lanes, curves, and foreground occlusion.
4. Design background depth: sky/Jupiter, city, native highway, battleships, parallax fighters, flying traffic, road traffic, effects; define exactly which elements should be behind vs in front of stage art. A design map does not replace runtime captures.
5. Collect 60fps clips before and after, measure object speeds in native px/tick, count on-screen entities and look for traffic dropouts, seams, and clipping at pillars/ramps.
6. If stage layout or occlusion cannot support a feature, identify offending tiles/layer API and label `BLOCKED` before choosing a visual reduction.

Output a camera-X/Y annotated scene map, timings, verified road lanes, before/after screenshots, and `UNTESTED` notes.

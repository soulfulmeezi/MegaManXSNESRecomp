---
name: cosmicx-renderer-integration
description: Use when mapping the Cosmic X JavaScript battle into the native host renderer, choosing direct sprite transforms vs faithful pre-rendered atlases, or integrating debug keys.
---

# Native renderer integration
1. Identify the user's ACTUAL active Cosmic X build, code revision and renderer entry points. The GitHub repository may be behind the local Windows build. Check native foreground and sky-layer composition, viewport clipping, frame buffer ownership, and double buffering before changes.
2. Verify whether arbitrary alpha sprites support scale, nearest-neighbor rotation, target resolution, and stable pivots. Test a full 36x20 fighter at 0.25, 1 and 2.6, not just a tiny existing fleet sprite.
3. If real-time 16-angle transforms exceed budget, prerender from source pixels only. Atlas must cover 2 teams x 16 angles x ~5+ scales including both endpoints; ensure transform orientation, alpha borders, origins, and brightness are preserved. No small fixed sprite replacement.
4. Layer appropriately with original scene semantics: sky bands, stars, Jupiter, skyline/highway, far cruiser, near cruiser, fighters, bolts, particles/explosions; check foreground occlusion against gameplay stage for the live composition.
5. Reserve debug control keys after reading launcher's existing hotkeys, gamepad mappings and modifier behavior; never pick a conflicting binding based on guesswork. Document map and always preserve normal gameplay.
6. Overlay build label, fighters actually visible, cruisers actually visible, real frame-time milliseconds and test-result indicators; measure outside inner sprite loops.

Return source paths touched, build command, real launch command, before/after capture steps, and marked `UNTESTED` results for unavailable tests.

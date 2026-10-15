---
name: cosmic-x-hd-art-pipeline
description: Design crisp 5x display scaling and optional original higher-detail vehicle, stage, sprite and effect art for Cosmic X without mixing art resolution with native gameplay physics.
---

# Cosmic X HD pixel art and scaling pipeline

Use for higher resolution, visually richer stage backgrounds, original cars, HD pixel art, animated effects or consistent new character art. Read src/mmx_render_assets.h, src/mmx_render_assets.c, src/mmx_source_assets.cpp, src/mmx_renderer.c and tests/mmx_renderer_test.c. Inspect the existing shader loader and presets in the pinned framework before introducing anything.

## Choose the output honestly
- Integer or nearest-neighbor display scaling makes the current pixels larger and crisper; a 5x output does not add native graphic detail.
- True enhanced detail requires new original assets or explicitly supported higher-resolution overlays, with separate design, memory and frame-pacing costs.
- Do not stretch sprites independently of the camera's pixel-aspect/viewport rules; preserve the existing adaptive widescreen compositor.

## Sprite and scene workflow
1. Create a subject brief from authorized references: size at gameplay scale, silhouette, outline, color/palette, texture, lighting, number of frames and required motions. Define whether it is strict SNES-style pixel art or optional HD artwork.
2. Approve one original master pose first. Keep one shared animation scale and anchor (character feet or car wheel-ground contact); specify frame size, duration and loop seam.
3. Keep art appearance separate from collision boxes and camera position. Review animations at game speed, not just on a sprite-sheet contact sheet.
4. Explicitly distinguish native indexed tiles and transparency, BGR555 palettes, VRAM offsets and compressed source assets from modern RGBA textures. Do not force HD RGBA data into a SNES tile/palette buffer.
5. Introduce one synthetic car/prop/FX fixture before a broad art pipeline. Track upload costs, texture memory and compositing time as part of the 60Hz budget.
6. Only after licensing review consider external editors, shader packs or third-party skill workflows; do not execute their scripts or download weights implicitly.

## QA checklist
Check pixel crispness, edge shimmer in motion, transparency halos, HUD readability, consistent sprite proportions, wheel alignment, animation pacing, wide-view clipping, palette transitions and frame-time impact. Keep a default original-art/integer-scale fallback.

Do not commit copyrighted ROM art, modified source-game sprites, captured gameplay frames or unlicensed art to the public repository. Record provenance and licensing for every third-party asset. This skill does not install an image model or generate an art pack automatically.

# Cosmic X cinematic showcase — development preview

This branch preserves the v33 game, original Zero/physics/save ABI, and all player combat. Opt-in by setting `MMX_COSMIC_SHOWCASE=1` in the environment before launching an approved test build.

Showcase enables existing Tier 1 near-road cars and the new Earth/Jupiter projectile spark overlay, disables colored road-debug lanes, and doubles ambient cloud drift and the road cars' visual progress. No guest/simulation tick, collisions, enemy logic or controller timing is changed. Cars remain draw-only, masked to approved BG1 road tiles and their original eight-pixel A/B lanes.

Each flag overrides the preset when explicitly set to `0` or `1`: `MMX_NEAR_ROAD_CARS`, `MMX_TRAFFIC_DEBUG_LANES`, `MMX_COSMIC_UPGRADE_VFX`, `MMX_COSMIC_TRAFFIC_FAST`, `MMX_COSMIC_SKY_FAST`. Halo stays OFF by default. With showcase OFF, all previous defaults are retained.

To reduce per-frame work, the renderer now checks the only valid BG1 world-Y road band (464–479), the native road tile, lane and conservative sprite/halo rectangle BEFORE calling exact existing per-pixel artwork. The original pixel/foreground masking functions are unchanged. The ROM-free test proves rejected candidate pixels were already transparent, sampling normal cars and optional halo across different ticks.

**Acceptance requires real playtesting.** Compare OFF vs ON near camera X 0x05B7, 0x0718, 0x0FCE and 0x148F. Check road A/B curves/occlusion, widescreen, stage phase gating, v33 fallback, Zero, co-op and frame delivery (p95/p99) before enabling any visual option by default. Curved C/D cars or aircraft are not implemented here.

No game assets, external packages, ROMs, saves or deployment changes. Keep this PR draft.

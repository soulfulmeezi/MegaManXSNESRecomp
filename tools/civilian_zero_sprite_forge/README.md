# Civilian Zero — Agent Sprite Forge integration

Upstream: https://github.com/0x0funky/agent-sprite-forge
Pinned revision: `5324342fdfab72430f4e9604f3fe427009f8596d` (2026-10-06).
Upstream license: MIT © 2026 0x0funky.

## Installation in CosmicX_Evolution (MSYS2 MinGW64)

From the game root:

```bash
bash tools/civilian_zero_sprite_forge/install.sh
```

This installs the upstream skills for **Codex** (not ChatGPT), makes no changes to game logic, and does not invoke image/video generation or incur API charges. Restart Codex before using the new skills. The script pins the source revision; review upstream source before running locally. Python 3.10+, Git, numpy, Pillow, scipy and optionally ffmpeg for video routes are required.

## Art and gameplay contract

1. Preserve source artwork at **288×288 RGBA** per pose with the original 3× detail and sharp pixel edges. Never upscale low-res frames and call them HD.
2. Keep identity, face, hair, jacket, palette, hip pivot, baseline and proportions consistent.
3. Side-view limbs must be labeled **NEAR/FAR**, with the NEAR leg leading frames 0–3 and FAR leading frames 4–7. The two contact poses must reach equally recognizable but opposite trailing-leg extensions. Do not create the second half by repeating the first.
4. X5-inspired motion: low torso, forward lean, forceful trailing-leg push, clearly advancing free arm, saber hand guarded closer to the hip.
5. First use upstream `generate2dsprite` to inspect and QC real existing frames, and `video2dsprite` only if an image-to-video route has been explicitly selected and costs understood. For a generated sheet use `sheet_qc.py frames --cycle run`; inspect the contact and passing poses visually.
6. A QA report is not proof of playable animation. The prior Run R1 passes basic duplicate/size checks but fails the user's opposite stride review. Require slowed actual game footage and a side-by-side native/HD comparison.
7. Runtime code is in `src/mmx_zero.c` (`MmxZeroCivilianRunPose()`); HD pixels are in `src/mmx_zero_civilian_detail_art.h`. The **skill installation does not edit these**.

Any later self-applying game patch must commit a backup, create a patch branch, offer a one-line rollback, build to `build_log.txt`, write `report.txt` on failure, and provide a feature toggle, fired log, overlay entry and title revision bump.

For the fuller offline installer, QA runner and instructions, see the downloadable integration ZIP provided in ChatGPT.

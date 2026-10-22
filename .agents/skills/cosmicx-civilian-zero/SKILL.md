---
name: cosmicx-civilian-zero
description: Preserve and improve Cosmic X's HD Civilian Zero running, jumping and dash animation using Agent Sprite Forge QA, registered frames and strict source-art fidelity.
---

# Civilian Zero HD animation workflow

Read `tools/civilian_zero_sprite_forge/README.md` before editing. Use the upstream Agent Sprite Forge `generate2dsprite` and `video2dsprite` skills. Video or paid image generation is NEVER automatic.

- Inventory accepted 288×288 RGBA source poses, the original X/Zero native run cycle, the animation selector, and the Boss Powers R4 source baseline.
- Plan 8 **distinct** NEAR/FAR gait poses: 0 near contact, 1 down, 2 passing, 3 up; 4 far contact, 5 down, 6 passing, 7 up. The far-contact leg must physically lead on frame 4, not just relabel an otherwise repeated silhouette.
- X5-style Zero: low forward lean, explicit trailing-leg push, free left-arm lead, guarded saber-side arm; hair/jacket respond with subtle delayed motion.
- Preserve 288×288 actual detail, face, colors, hair strands, clothes, boot size, root and foot anchoring. Never substitute low-res resizes or new character designs.
- Preflight original candidate PNGs with `sheet_qc.py frames --cycle run`. Validate visual *anatomy*, not just silhouette IoU/metrics. The prior X5 R1 passes simple pixel QC but is visibly incomplete.
- After **art review** only: patch the detailed art and run selector, keep old behavior switchable, include `[FEATURE] fired`, overlay and revision title. Commit backup first; provide rollback command; build with `build_log.txt`, fail with `report.txt` and surrounding code; obtain actual in-game slow-motion proof of both stride extremes.

Do not claim the stride is fixed until a gameplay clip shows both opposing push-offs.

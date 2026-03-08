# StickBrawl — Done

- **B1.** **Particle coordinate space mismatch** — Fixed by converting particle velocities to world-space (divided by PPM at spawn) and changing gravity from 200 (pixel-scale) to 10 m/s² (world-scale). Positions stay in world coords, `toScreen()` converts at draw time.

- **B2.** **Dismember always hides head regardless of detached part** — Added `dismemberedPart` field to `DeathEffect`. `spawnDismember()` stores the choice (0=head, 1=left arm, 2=right arm). `drawCollapse()` now only hides the specific detached part and draws a blood stump at the correct joint.

- **B3.** **Fade-out alpha computed but unused** — Added `effectAlpha` parameter to `drawGib()`, `drawCollapse()`, and `drawParticles()`. The overall effect fade-out (last 0.5s) now multiplies into all sub-element alphas. Removed `drawCollapse`'s redundant internal alpha calculation.

- **B4.** **drawGib crash on destroyed physics body** — Added `b2Body_IsValid()` guard at the top of `drawGib()`, returning early if the body has been destroyed.

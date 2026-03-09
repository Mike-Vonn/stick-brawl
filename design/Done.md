# StickBrawl — Done

- **B1.** **Particle coordinate space mismatch** — Fixed by converting particle velocities to world-space (divided by PPM at spawn) and changing gravity from 200 (pixel-scale) to 10 m/s² (world-scale). Positions stay in world coords, `toScreen()` converts at draw time.

- **B2.** **Dismember always hides head regardless of detached part** — Added `dismemberedPart` field to `DeathEffect`. `spawnDismember()` stores the choice (0=head, 1=left arm, 2=right arm). `drawCollapse()` now only hides the specific detached part and draws a blood stump at the correct joint.

- **B3.** **Fade-out alpha computed but unused** — Added `effectAlpha` parameter to `drawGib()`, `drawCollapse()`, and `drawParticles()`. The overall effect fade-out (last 0.5s) now multiplies into all sub-element alphas. Removed `drawCollapse`'s redundant internal alpha calculation.

- **B4.** **drawGib crash on destroyed physics body** — Added `b2Body_IsValid()` guard at the top of `drawGib()`, returning early if the body has been destroyed.

- **F1.** **Dragon character with fire breath** — Added quadruped Dragon character (CHARACTER_TYPE_COUNT now 7) with `drawDragon()` featuring wings, spines, horned head, tail, and fire breath visual. Fire Breath weapon: 5-pellet cone, 30° spread, no gravity, burn DOT (6 DPS for 3s). Burn DOT system added parallel to poison (separate timers, can stack). Terrain fire system: Wood and Roof platforms catch fire from fire projectiles, fire spreads to adjacent flammable platforms after 1.5s, platforms collapse after 5s of burning. Players standing on burning platforms also catch fire.

- **F3.** **Incinerate death animation** — New `DeathAnimType::Incinerate` for fire/burn deaths. Three phases: (1) character darkens from player color to charcoal black (0-0.8s), (2) charcoal figure shrinks/crumbles with rising ash/ember particles (0.8-1.5s), (3) bone-colored skeleton silhouette appears and collapses with fading embers (1.5-3s). Triggered by "Fire Breath" or "Burn" DOT kills via `getDeathAnimType()`. Burn DOT now tracks "Burn" as `m_lastDamageWeapon` so burn kills correctly trigger Incinerate.

- **F4/B5.** **Character-aware death animations** — Collapse and Dismember death animations now render body shapes appropriate to the dying character's type. Added `BodyPlan` enum (Humanoid, Quadruped, Serpentine) and `CharacterType` field to `DeathEffect`. `drawCollapse()` dispatches to `drawCollapseHumanoid()` (Stick, StickLady), `drawCollapseQuadruped()` (Cat, Unicorn, Crocodile, Dragon), or `drawCollapseSerpentine()` (Cobra). Incinerate also uses body-plan drawers for charcoal phase and per-plan skeleton drawings (with ribs, skull, vertebral details).

- **I1.** **Magic number cleanup** — Replaced all `3.14159f` and `6.283f` magic numbers with named `PI` and `TWO_PI` constants in `DeathAnimation.cpp`, `StickFigure.cpp`, and `Game.cpp`.

- **I2.** **`BloodParticle` renamed to `Particle`** — Renamed the struct in `DeathAnimation.h` and all usage in `DeathAnimation.cpp` since it's used for ash, bone fragments, embers, and other non-blood particles.

- **I3.** **Weapon death_anim tag replaces string matching** — Added `deathAnim` field to `WeaponData`, parsed from `"death_anim"` in weapon JSON. `getDeathAnimType()` now checks the weapon's tag (`dismember`, `disintegrate`, `incinerate`, `explode`, `collapse`) instead of matching weapon names. Also added `m_lastDamageDeathAnim` tracking on `StickFigure` passed through `takeDamage()`. Updated katana.json, jaw_snap.json, nuke_grenade.json, and fire_breath.json with their tags.

- **I4.** **Gib physics body leak fix** — Added `cleanupAll(Physics&)` method to `DeathAnimationSystem` that destroys all gib bodies and clears all effects. Called on round end (`checkRoundEnd`), round restart (R key), and return to character select (Backspace).

- **I5.** **`m_wasAlive` defensive resize** — Added a resize guard at the top of `checkPlayerDeaths()` ensuring `m_wasAlive` always matches the player count. Removed the silent `i < m_wasAlive.size()` guards that would skip untracked players.

- **F5.** **Dragon glide ability**
- **F6.** **Unique weapon pickup icons** — Each weapon now has a distinct visual icon when spawned as a pickup instead of a generic gold box. Katana: angled blade with guard and handle. Pistol: L-shaped gun silhouette with trigger guard. Shotgun: long barrel with wide muzzle, wooden stock and grip. Grenade Launcher: military green tube with grenade at tip. Nuclear Hand Grenade: glowing aura with grey shell and spinning radiation trefoil. Unknown weapons fall back to the old gold box. — Dragon can glide by holding the jump button while airborne. Downward velocity is clamped to -2.0 m/s for a slow descent. Wings spread wider with a gentle flap animation during glide. Glide state resets each frame and only activates for the Dragon character type.

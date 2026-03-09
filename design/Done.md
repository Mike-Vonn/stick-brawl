# StickBrawl — Done

## From dev branch

- **B1.** **Particle coordinate space mismatch** — Fixed by converting particle velocities to world-space (divided by PPM at spawn) and changing gravity from 200 (pixel-scale) to 10 m/s² (world-scale). Positions stay in world coords, `toScreen()` converts at draw time.

- **B2.** **Dismember always hides head regardless of detached part** — Added `dismemberedPart` field to `DeathEffect`. `spawnDismember()` stores the choice (0=head, 1=left arm, 2=right arm). `drawCollapse()` now only hides the specific detached part and draws a blood stump at the correct joint.

- **B3.** **Fade-out alpha computed but unused** — Added `effectAlpha` parameter to `drawGib()`, `drawCollapse()`, and `drawParticles()`. The overall effect fade-out (last 0.5s) now multiplies into all sub-element alphas. Removed `drawCollapse`'s redundant internal alpha calculation.

- **B4.** **drawGib crash on destroyed physics body** — Added `b2Body_IsValid()` guard at the top of `drawGib()`, returning early if the body has been destroyed.

- **F1.** **Dragon character with fire breath** — Added quadruped Dragon character with `drawDragon()` featuring wings, spines, horned head, tail, and fire breath visual. Fire Breath weapon: 5-pellet cone, 30° spread, no gravity, burn DOT (6 DPS for 3s). Burn DOT system added parallel to poison. Terrain fire system: Wood and Roof platforms catch fire, fire spreads, platforms collapse after 5s.

- **F3.** **Incinerate death animation** — New `DeathAnimType::Incinerate` for fire/burn deaths. Three phases: darkening, crumbling with ash, skeleton collapse.

- **F4/B5.** **Character-aware death animations** — Collapse and Dismember death animations render body shapes appropriate to the dying character's type via `BodyPlan` (Humanoid, Quadruped, Serpentine).

- **I1.** **Magic number cleanup** — Replaced `3.14159f` and `6.283f` with named `PI` and `TWO_PI` constants.

- **I2.** **`BloodParticle` renamed to `Particle`** — Generic name since it's used for ash, bone fragments, embers, etc.

- **I3.** **Weapon death_anim tag replaces string matching** — Added `deathAnim` field to `WeaponData`, parsed from `"death_anim"` in weapon JSON.

- **I4.** **Gib physics body leak fix** — Added `cleanupAll(Physics&)` method to `DeathAnimationSystem`.

- **I5.** **`m_wasAlive` defensive resize** — Resize guard at top of `checkPlayerDeaths()`.

- **F5.** **Dragon glide ability** — Hold jump while airborne for slow descent. Wings spread wider during glide.

- **F6.** **Unique weapon pickup icons** — Each weapon has a distinct visual icon when spawned as a pickup.

- **F7.** **Weapon icon above characters** — `drawWeaponIcon()` at 0.6x scale above alive players.

- **F8.** **Weapon inventory, switching & death drops** — Multiple weapon slots, swap key, death drops.

## From big-cats merge review

- **I6.** **Preserve Cat character type** — Restored Cat as housecat (0.8x health, 10.0 speed, 13.0 jump, wall climb, Cat Scratch weapon). Distinct from Jaguar via size, speed, and weapon. `canWallClimb()` includes Cat, Jaguar, Panther. Cat is NOT in `isBigCat()`.

- **I7.** **Hardcoded weapon rejection list** — Resolved by spawnable flag in weapon JSON. `WeaponFactory::getRandomSpawnableWeapon()` replaces the do/while rejection loop. Innate weapons default to `spawnable: false`.

- **I8.** **Restore stripped comments** — Restored inline comments for `m_aimAngle`, `teleportTo`, section headers (Poison DOT, Animation, Character select state), ExplosionEffect fields, and Game.h section comments.

- **I9.** **Big cats bypass RulesEngine health** — Replaced hardcoded health with `m_healthMultiplier` per character. `startGame()` applies `rules.maxHealth * getHealthMultiplier()` for ALL characters. Multipliers: Lion 1.3x, Tiger 1.1x, Jaguar 1.0x, Panther 0.95x, Cheetah 0.75x, Cat 0.8x.

- **I10.** **damageMultiplier scoped to melee only** — Removed `ownerDamageMultiplier` from Projectile struct. Tiger's 1.25x only applies in `handleMeleeAttack()`.

- **I11.** **Wall climb input changed** — Wall climb now triggers by move-toward-wall while airborne. `isTouchingWall()` replaced with `wallSide()` returning -1/0/1.

- **I12.** **Respawn body hide fix** — All 6 physics bodies moved offscreen during respawn timer.

# Done

- **I1.** **Preserve Cat character type** — Restored Cat as housecat (0.8x health, 10.0 speed, 13.0 jump, wall climb, Cat Scratch weapon). Distinct from Jaguar via size, speed, and weapon. `canWallClimb()` includes Cat, Jaguar, Panther. Cat is NOT in `isBigCat()`.
- **I2.** **Hardcoded weapon rejection list** — Resolved by spawnable flag in weapon JSON. `WeaponFactory::getRandomSpawnableWeapon()` replaces the do/while rejection loop. Innate weapons default to `spawnable: false`.
- **I3.** **Restore stripped comments** — Restored inline comments for `m_aimAngle`, `teleportTo`, section headers (Poison DOT, Animation, Character select state), ExplosionEffect fields, and Game.h section comments. Reverted em dash cosmetic changes.
- **I4.** **Big cats bypass RulesEngine health** — Replaced hardcoded health with `m_healthMultiplier` per character. `startGame()` applies `rules.maxHealth * getHealthMultiplier()` for ALL characters. Multipliers: Lion 1.3x, Tiger 1.1x, Jaguar 1.0x, Panther 0.95x, Cheetah 0.75x, Cat 0.8x.
- **I5.** **damageMultiplier scoped to melee only** — Removed `ownerDamageMultiplier` from Projectile struct. Tiger's 1.25x only applies in `handleMeleeAttack()`, affecting innate and picked-up melee weapons.
- **I6.** **Wall climb input changed** — Wall climb now triggers by move-toward-wall while airborne. `isTouchingWall()` replaced with `wallSide()` returning -1/0/1. Aim-up no longer conflicts with wall climbing.
- **I7.** **Respawn body hide fix** — Kept from big-cats commit as-is. All 6 physics bodies moved offscreen during respawn timer.

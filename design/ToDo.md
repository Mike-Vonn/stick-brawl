# StickBrawl — ToDo

## Bugs

*Next: B5*

## Improvements

- **I1.** **Magic number `6.283f` used for 2*PI** — Appears 3 times in `DeathAnimation.cpp`. Replace with a named constant or `M_PI * 2.0f`.

- **I2.** **`BloodParticle` naming is misleading** — Also used for ash, bone fragments, and embers in the Disintegrate/Explode effects. Consider renaming to `Particle`.

- **I3.** **Weapon-name string matching is fragile** — `getDeathAnimType()` in `Game.cpp:1064-1082` matches weapon names with string literals (`"Katana"`, `"Jaw Snap"`, `"Nuclear Hand Grenade"`). If weapon names change in the JSON configs, these silently break. Consider using a weapon property/tag (e.g., `"deathAnim": "dismember"` in the weapon JSON) instead.

- **I4.** **Gib physics bodies leak if game state changes abruptly** — If the game transitions to `RoundOver`/`GameOver` while death effects are active, `DeathAnimationSystem` never gets a cleanup call and Box2D bodies leak. Consider cleanup in destructor or on state transition.

- **I5.** **`m_wasAlive` not resized defensively** — `checkPlayerDeaths()` guards with `i < m_wasAlive.size()` but silently skips untracked players. An assert or resize at the point of player addition would be safer.

*Next: I6*

## Features

- **F1.** **Dragon character with fire breath** — A full dragon character. Its attack is a fire breath that catches opponents and flammable environment on fire. Requires: new `CharacterType::Dragon`, custom `drawDragon()`, a fire breath weapon/attack mechanic, a fire/burning status effect system (DoT + visual), and flammable terrain interactions.

- **F2.** **Dragonoid character with fireball and wings** — A humanoid dragon-like character with small wings and a small flame ball breath attack. Requires: new `CharacterType::Dragonoid`, custom `drawDragonoid()` (stick figure base with wing and tail details), a fireball projectile weapon, and wing visuals (cosmetic or possibly a glide/double-jump mechanic).

*Next: F3*

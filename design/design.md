# StickBrawl — System Design

## Overview
A 2D local multiplayer stick figure fighting game. Players select characters, spawn on an arena, and fight using melee/ranged/explosive weapons until one player remains.

## Tech Stack
- **Language**: C++17
- **Graphics**: SFML 3.0
- **Physics**: Box2D (v3 C API)
- **Data**: nlohmann_json for weapon/rules config
- **Build**: CMake 3.20+

## Architecture

### Core Loop
`Game` owns all subsystems. The main loop: `processEvents() → update(dt) → render()`.

Update order matters:
1. `handlePlayerInput(dt)` — reads input, triggers attacks/movement
2. `player->update(dt)` — per-player tick (cooldowns, poison, respawn timer)
3. `m_physics.step(dt)` — Box2D world step
4. `updateProjectiles(dt)` — move/collide/detonate projectiles
5. `updateWeaponPickups(dt)` — pickup collision checks
6. `checkFallDeath()` — off-screen kills
7. `checkPlayerDeaths()` — detect death transitions, spawn death anims, handle lives/respawn
8. `m_deathAnims.update(dt)` / `cleanup()` — tick and GC death effects
9. `updateWeaponSpawns(dt)` — periodic weapon drops
10. `checkRoundEnd()` — check win condition

**Invariant**: `checkPlayerDeaths()` must run after all damage sources (projectiles, melee, falls) and before `checkRoundEnd()`. The `m_wasAlive` vector tracks per-frame alive state to detect death transitions exactly once.

### Physics (Box2D v3)
- World uses category/mask bit filtering: `CAT_PLAYER`, `CAT_PLATFORM`, `CAT_PROJECTILE`, `CAT_PICKUP`
- Coordinate system: world origin at screen center, Y-up. Screen conversion: `SCREEN_CX + x * PPM`, `SCREEN_CY - y * PPM`
- Each `StickFigure` is a multi-body ragdoll: head, torso, arms, legs connected by joints

### Characters
Six types: Stick, Cat, Cobra, Unicorn, Crocodile, StickLady. Each has a custom `draw*()` method. Character type is cosmetic only (no gameplay differences currently).

### Weapons
Loaded from JSON files in `assets/weapons/`. Three types:
- **Melee** — range check + facing, instant damage
- **Projectile** — spawns physics body, travels, hits on proximity
- **Explosive** — projectile that detonates on contact/timer, area damage + terrain carving

### Arena / Terrain
Destructible terrain via `Arena::carveCircle()`. Platforms are Box2D static bodies.

### Death Animation System
`DeathAnimationSystem` manages post-death visual effects. Four types:
- **Collapse** — stick figure tilts and falls over (melee, guns, falls)
- **Dismember** — a body part (head/arm) detaches as a physics gib (blades)
- **Explode** — body fragments into many physics gibs (explosives)
- **Disintegrate** — particle cloud, no physics bodies (nuke)

Death type is selected by `Game::getDeathAnimType()` based on the last weapon that dealt damage to the dying player. Weapon tracking is stored on `StickFigure` via the `takeDamage(amount, kbX, kbY, weaponName, weaponType)` overload.

### Wrap-Around Mode
Optional mode where players/projectiles wrap screen edges instead of dying from falls.

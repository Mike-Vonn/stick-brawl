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
2. `player->update(dt)` — per-player tick (cooldowns, poison, burn, respawn timer)
3. `m_physics.step(dt)` — Box2D world step
4. `updateProjectiles(dt)` — move/collide/detonate projectiles, ignite flammable platforms
5. `updateWeaponPickups(dt)` — pickup collision checks
6. `m_arena.updateFire(dt)` — spread fire, destroy burnt platforms
7. Burning platform contact damage — apply burn DOT to players standing on fire
8. `checkFallDeath()` — off-screen kills
9. `checkPlayerDeaths()` — detect death transitions, spawn death anims, handle lives/respawn
10. `m_deathAnims.update(dt)` / `cleanup()` — tick and GC death effects
11. `updateWeaponSpawns(dt)` — periodic weapon drops
12. `checkRoundEnd()` — check win condition

**Invariant**: `checkPlayerDeaths()` must run after all damage sources (projectiles, melee, falls) and before `checkRoundEnd()`. The `m_wasAlive` vector tracks per-frame alive state to detect death transitions exactly once.

### Physics (Box2D v3)
- World uses category/mask bit filtering: `CAT_PLAYER`, `CAT_PLATFORM`, `CAT_PROJECTILE`, `CAT_PICKUP`
- Coordinate system: world origin at screen center, Y-up. Screen conversion: `SCREEN_CX + x * PPM`, `SCREEN_CY - y * PPM`
- Each `StickFigure` is a multi-body ragdoll: head, torso, arms, legs connected by joints

### Characters
Seven types: Stick, Cat, Cobra, Unicorn, Crocodile, StickLady, Dragon. Each has a custom `draw*()` method. Some characters have innate weapons (Cobra=Poison Spit, Unicorn=Horn Blast, Crocodile=Jaw Snap, StickLady=Purse Swing, Dragon=Fire Breath). These innate weapons are excluded from random weapon spawns.

**Dragon glide**: Dragon can glide by holding the jump button while airborne. Downward velocity is clamped to -2.0 m/s (slow fall). Wings spread wide with a gentle flap during glide. Uses the held `pi.jump` state (not `jumpPressed`) to detect hold.

### Weapons
Loaded from JSON files in `assets/weapons/`. Three types:
- **Melee** — range check + facing, instant damage
- **Projectile** — spawns physics body, travels, hits on proximity
- **Explosive** — projectile that detonates on contact/timer, area damage + terrain carving

### Status Effects (DOT)
Two independent DOT systems on `StickFigure`, can stack simultaneously:
- **Poison** — green particles, ticks every 0.5s, applied by Cobra's Poison Spit
- **Burn** — orange flame particles, ticks every 0.5s, applied by Dragon's Fire Breath or standing on burning platforms

Both are cleared on respawn. Each has its own timer, DPS, and tick timer fields.

### Arena / Terrain
Destructible terrain via `Arena::carveCircle()`. Platforms are Box2D static bodies. Platform types: Ground, Wood, Stone, Metal, Brick, Roof.

**Fire system**: Wood and Roof platforms are flammable. Fire projectiles hitting a flammable platform ignite it. Burning platforms:
- Deal burn DOT to players standing on them
- Spread fire to adjacent flammable platforms after 1.5s (proximity < 1.0m gap)
- Collapse (destroyed) after 5 seconds of burning
- Render with orange overlay + flickering flame particles

### Death Animation System
`DeathAnimationSystem` manages post-death visual effects. Five types:
- **Collapse** — body tilts and falls over (melee, guns, falls)
- **Dismember** — a body part (head/arm) detaches as a physics gib (blades)
- **Explode** — body fragments into many physics gibs (explosives)
- **Disintegrate** — particle cloud, no physics bodies (nuke)
- **Incinerate** — charcoalizes (darkens to black), crumbles with ash particles, then skeleton collapses (fire/burn deaths)

Death type is selected by `Game::getDeathAnimType()` based on the last weapon that dealt damage to the dying player. Weapon tracking is stored on `StickFigure` via the `takeDamage(amount, kbX, kbY, weaponName, weaponType)` overload. Burn DOT also sets the damage weapon to "Burn" on tick.

**Body-plan awareness**: Collapse, Dismember, and Incinerate animations render character-appropriate body shapes via `BodyPlan` (Humanoid, Quadruped, Serpentine) derived from `CharacterType`. Each `DeathEffect` stores the dying character's type.

### Wrap-Around Mode
Optional mode where players/projectiles wrap screen edges instead of dying from falls.

# StickBrawl — System Design

## Overview
A 2D local multiplayer stick figure fighting game. Players select characters, spawn on an arena, and fight using melee/ranged/explosive weapons until one player remains. Supports up to 5 local players with gamepad/keyboard input.

## Tech Stack
- **Language**: C++17
- **Graphics**: SFML 3.0
- **Physics**: Box2D (v3 C API)
- **Data**: nlohmann_json for weapon/rules config
- **Build**: CMake 3.20+, vcpkg for dependencies

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

### Character System
Characters are defined by `CharacterType` enum. Each has:
- Unique draw method for visual appearance
- Optional innate weapon (equipped on spawn, restored on respawn via `setInnateWeapon()`)
- Optional stat modifiers (health multiplier, speed, jump, damage multiplier)

#### Current Characters (13)
- **Stick** — default, no special abilities
- **Cat** — housecat, 0.8x HP, fast (10.0 speed), wall climb, Cat Scratch (fast/low dmg)
- **Lion** — 1.3x HP, high knockback bite, mane visual
- **Tiger** — 1.1x HP, slow but 1.25x melee damage, stripes
- **Jaguar** — 1.0x HP, wall climb, powerful bite, rosette spots
- **Panther** — 0.95x HP, wall climb, fast slash, dark/sleek visual
- **Cheetah** — 0.75x HP, super fast (11.5 speed, 14.0 jump), tear lines
- **Cobra** — poison spit ranged weapon
- **Unicorn** — horn blast attack, rainbow effects
- **Crocodile** — jaw snap melee
- **Stick Lady** — purse swing melee
- **Dragon** — fire breath, glide ability, quadruped
- **Mr Diaper-Pants** — milk squirt ranged weapon

#### Design Rules
- **Health**: All characters use `RulesEngine.maxHealth` as base, scaled by per-character `m_healthMultiplier`. Never hardcode absolute health values.
- **Damage multiplier**: `m_damageMultiplier` applies to melee attacks only (in `handleMeleeAttack()`). Does NOT affect projectile or explosive damage.
- **Innate weapons**: Not spawnable as pickups. Weapon JSON uses `"spawnable": false` (default). Restored on respawn.
- **Wall climbing**: Cat, Jaguar, and Panther can wall climb. Triggered by moving toward a wall while airborne (not aim-up). `wallSide()` returns -1/0/1 for wall direction.
- **Big cats** (`isBigCat()`): Lion, Tiger, Jaguar, Panther, Cheetah. Cat (housecat) is NOT a big cat.
- **Dragon glide**: Dragon can glide by holding jump while airborne. Downward velocity clamped to -2.0 m/s.

### Weapons
Loaded from JSON files in `assets/weapons/`. Three types:
- **Melee** — range check + facing, instant damage
- **Projectile** — spawns physics body, travels, hits on proximity
- **Explosive** — projectile that detonates on contact/timer, area damage + terrain carving

#### Weapon Spawning
- `WeaponFactory` builds a spawnable index at load time from weapons with `"spawnable": true`
- `getRandomSpawnableWeapon()` picks only from spawnable weapons — no rejection loop
- Innate weapons default to not spawnable

#### Weapon Inventory
Characters carry multiple weapons (slot 0 = innate, rest = pickups). Swap key cycles active weapon. On death, non-innate weapons scatter as pickups with preserved ammo.

#### Weapon Icons
Each weapon has a distinct visual icon rendered both as pickup and above characters holding them.

### Status Effects (DOT)
Two independent DOT systems on `StickFigure`, can stack simultaneously:
- **Poison** — green particles, ticks every 0.5s, applied by Cobra's Poison Spit
- **Burn** — orange flame particles, ticks every 0.5s, applied by Dragon's Fire Breath or standing on burning platforms

Both are cleared on respawn. Each has its own timer, DPS, and tick timer fields.

### Arena / Terrain
Destructible terrain via `Arena::carveCircle()`. Platforms are Box2D static bodies. Platform types: Ground, Wood, Stone, Metal, Brick, Roof.

**Safe spawns**: `Arena::getSafeSpawnPoint()` checks for ground below spawn point; falls back to nearest surviving platform if terrain was carved.

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
- **Incinerate** — charcoalizes, crumbles with ash, skeleton collapses (fire/burn deaths)

Death type selected by weapon's `deathAnim` tag. Body-plan awareness renders character-appropriate shapes (Humanoid, Quadruped, Serpentine).

### Respawn System
- All 6 physics bodies (head, torso, arms, legs) are hidden offscreen during respawn timer
- Uses `getSafeSpawnPoint()` to avoid spawning over carved terrain

### Wrap-Around Mode
Optional mode where players/projectiles wrap screen edges instead of dying from falls.

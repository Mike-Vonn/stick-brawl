# Stick Brawl — Design

## Overview
2D stick figure brawler using SFML for rendering and Box2D for physics. Supports up to 5 local players with gamepad/keyboard input.

## Architecture
- **Game** — main loop, state machine (CharSelect, Playing, RoundOver, GameOver)
- **StickFigure** — character with physics bodies (head, torso, arms, legs connected by joints)
- **Arena** — destructible terrain with multiple level layouts, safe spawn point detection
- **WeaponFactory** — loads weapons from JSON files in `assets/weapons/`, maintains spawnable index
- **RulesEngine** — configurable game rules (lives, health, damage multipliers, round time)
- **Physics** — Box2D world wrapper
- **Renderer** — SFML window wrapper
- **HUD** — health bars, ammo, round info

## Character System
Characters are defined by `CharacterType` enum. Each has:
- Unique draw method for visual appearance
- Optional innate weapon (equipped on spawn, restored on respawn via `m_innateWeapon`)
- Optional stat modifiers (health multiplier, speed, jump, damage multiplier)

### Current Characters (11)
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

### Design Rules
- **Health**: All characters use `RulesEngine.maxHealth` as base, scaled by per-character `m_healthMultiplier`. Never hardcode absolute health values.
- **Damage multiplier**: `m_damageMultiplier` applies to melee attacks only (in `handleMeleeAttack()`). Does NOT affect projectile or explosive damage.
- **Innate weapons**: Not spawnable as pickups. Weapon JSON uses `"spawnable": false` (default). Restored on respawn.
- **Wall climbing**: Cat, Jaguar, and Panther can wall climb. Triggered by moving toward a wall while airborne (not aim-up). `wallSide()` returns -1/0/1 for wall direction.
- **Big cats** (`isBigCat()`): Lion, Tiger, Jaguar, Panther, Cheetah. Cat (housecat) is NOT a big cat.

## Weapon System
Weapons are JSON-defined with type (Melee/Projectile/Explosive), damage, knockback, range, attack rate, and optional poison/explosion properties. Characters can pick up weapon drops that spawn periodically.

### Weapon Spawning
- `WeaponFactory` builds a spawnable index at load time from weapons with `"spawnable": true`
- `getRandomSpawnableWeapon()` picks only from spawnable weapons — no rejection loop
- Innate weapons default to not spawnable

## Respawn System
- All 6 physics bodies (head, torso, arms, legs) are hidden offscreen during respawn timer
- `Arena::getSafeSpawnPoint()` checks for ground below spawn point; falls back to nearest surviving platform if terrain was carved

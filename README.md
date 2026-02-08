# StickBrawl

A Stick Fight-inspired 2D physics brawler built with C++, SFML, and Box2D.

## Features
- Physics-based stick figure ragdoll combat
- **Data-driven weapons** — add new weapons via JSON, no recompile needed
- **Configurable game rules** — tweak health, gravity, round settings via JSON
- Local multiplayer (keyboard + gamepad support)
- Destructible platforms

## Project Structure
```
stickbrawl/
├── CMakeLists.txt          # Build system
├── assets/
│   ├── weapons/            # Weapon definitions (JSON)
│   └── rules/              # Game rule configs (JSON)
├── src/
│   ├── main.cpp            # Entry point
│   ├── Game.h/cpp          # Game loop & state management
│   ├── Physics.h/cpp       # Box2D world wrapper
│   ├── StickFigure.h/cpp   # Ragdoll character
│   ├── Weapon.h/cpp        # Weapon base + loader
│   ├── WeaponFactory.h/cpp # Creates weapons from JSON
│   ├── Arena.h/cpp         # Level/platform layout
│   ├── Input.h/cpp         # Input abstraction (KB + gamepad)
│   ├── Renderer.h/cpp      # SFML rendering
│   ├── RulesEngine.h/cpp   # Configurable game rules
│   ├── HUD.h/cpp           # Health bars, scores
│   └── ContactListener.h/cpp # Collision callbacks
└── README.md
```

## Dependencies
- **SFML 2.6+** — Windowing, rendering, input, audio
- **Box2D 2.4+** — 2D physics
- **nlohmann/json** — JSON parsing for weapon/rules configs

## Building (Windows with vcpkg)
```bash
vcpkg install sfml box2d nlohmann-json
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## Adding a Weapon
Create a JSON file in `assets/weapons/`:
```json
{
    "name": "Laser Sword",
    "type": "melee",
    "damage": 35,
    "knockback": 12.0,
    "range": 2.5,
    "attack_rate": 0.4,
    "sprite": "laser_sword.png"
}
```

## Modifying Rules
Edit `assets/rules/default.json`:
```json
{
    "max_health": 100,
    "gravity": -20.0,
    "round_time": 120,
    "lives": 3,
    "friendly_fire": true,
    "weapon_spawn_interval": 5.0
}
```

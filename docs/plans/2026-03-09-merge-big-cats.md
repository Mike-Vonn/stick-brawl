# Merge Big Cats Branch Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Merge the `big-cats` branch into `merge-big-cats`, then apply 7 reviewed fixes (I1-I7) to resolve design conflicts.

**Architecture:** Merge all 3 commits from `big-cats` first, then apply fixes sequentially. Each fix is a separate commit for easy review/revert. The big cats commit replaces the old `Cat` with 5 big cats (Lion, Tiger, Jaguar, Panther, Cheetah) and adds wall climbing, damage multipliers, safe spawns, and innate weapon respawn. Fixes restore Cat, enforce RulesEngine health, scope damage multiplier to melee, change wall climb input, and restore stripped comments.

**Tech Stack:** C++ (SFML 3, Box2D 3, nlohmann/json), CMake, vcpkg

---

### Task 1: Merge big-cats branch

**Step 1: Merge**

```bash
git merge big-cats --no-ff -m "merge big-cats branch for review"
```

Expected: Clean merge (no conflicts, merge-big-cats is a fresh branch off main).

**Step 2: Build to verify merge compiles**

```bash
./build.sh
```

Expected: Successful build.

---

### Task 2: Restore Cat character type (I1)

Restore the original `Cat` as a housecat character distinct from Jaguar. Cat gets wall climb, 0.8x health multiplier, fast speed, and a "Cat Scratch" innate weapon.

**Files:**
- Modify: `src/StickFigure.h`
- Modify: `src/StickFigure.cpp`
- Modify: `src/Game.cpp`
- Create: `assets/weapons/cat_scratch.json`

**Step 1: Add Cat back to CharacterType enum in `src/StickFigure.h`**

Insert `Cat` after `Stick` in the enum (before Lion):

```cpp
enum class CharacterType {
    Stick,
    Cat,
    Lion,
    Tiger,
    Jaguar,
    Panther,
    Cheetah,
    Cobra,
    Unicorn,
    Crocodile,
    StickLady
};

constexpr int CHARACTER_TYPE_COUNT = 11;
```

**Step 2: Add Cat to `characterTypeName()` in `src/StickFigure.h`**

Add case in the switch:

```cpp
case CharacterType::Cat:       return "Cat";
```

**Step 3: Add Cat to `characterTypeBlurb()` in `src/StickFigure.h`**

```cpp
case CharacterType::Cat:     return "80% HP | Wall Climb | Fast Scratch";
```

**Step 4: Add Cat to `canWallClimb()` in `src/StickFigure.h`**

```cpp
inline bool canWallClimb(CharacterType t) {
    return t == CharacterType::Cat || t == CharacterType::Jaguar || t == CharacterType::Panther;
}
```

Note: `isBigCat()` should NOT include Cat — Cat is a housecat.

**Step 5: Add Cat stats to `applyCharacterStats()` in `src/StickFigure.cpp`**

Add case in the switch before `Lion`:

```cpp
case CharacterType::Cat:
    m_healthMultiplier = 0.8f;
    m_moveSpeed = 10.0f;
    m_jumpForce = 13.0f;
    break;
```

Note: Do NOT set `m_maxHealth`/`m_health` here — Task 4 (I4) will handle health via multiplier.

**Step 6: Add `drawCat()` declaration in `src/StickFigure.h`**

In the private section, add:

```cpp
void drawCat(sf::RenderTarget& target) const;
```

**Step 7: Add `drawCat()` implementation in `src/StickFigure.cpp`**

Restore the original Cat drawing from main branch. Place it before `drawLion()`. Use the `drawBigCatBase` helper but with smaller dimensions, OR write a custom smaller cat body. The key visual distinction: smaller body, triangular pointed ears (not round like Lion/Jaguar), simple whiskers, no spots/stripes/mane.

```cpp
void StickFigure::drawCat(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);

    // Tail — curvy
    float tx = c.x - dir * 12.0f;
    sf::VertexArray tail(sf::PrimitiveType::LineStrip, 4);
    tail[0] = sf::Vertex{{tx, c.y}, dc};
    tail[1] = sf::Vertex{{tx - dir * 8.0f, c.y - 8.0f}, dc};
    tail[2] = sf::Vertex{{tx - dir * 12.0f, c.y - 16.0f}, dc};
    tail[3] = sf::Vertex{{tx - dir * 8.0f, c.y - 22.0f}, dc};
    target.draw(tail);

    // Small body (smaller than big cats)
    sf::RectangleShape body({22.0f, 13.0f});
    body.setOrigin({11.0f, 6.5f});
    body.setPosition(c);
    body.setFillColor(dc);
    body.setOutlineColor(sf::Color::Black);
    body.setOutlineThickness(1.0f);
    target.draw(body);

    // 4 legs
    for (float lx : {-0.32f, -0.12f, 0.12f, 0.32f}) {
        sf::RectangleShape leg({3.0f, 10.0f});
        leg.setOrigin({1.5f, 0.0f});
        leg.setPosition({c.x + lx * 22.0f, c.y + 6.5f});
        leg.setFillColor(dc);
        leg.setOutlineColor(sf::Color::Black);
        leg.setOutlineThickness(0.5f);
        target.draw(leg);
    }

    // Head (smaller than big cats)
    sf::CircleShape head(8.0f);
    head.setOrigin({8.0f, 8.0f});
    head.setPosition({c.x + dir * 16.0f, c.y - 4.0f});
    head.setFillColor(dc);
    head.setOutlineColor(sf::Color::Black);
    head.setOutlineThickness(1.0f);
    target.draw(head);

    sf::Vector2f hc = head.getPosition();
    // Pointy triangular ears (distinctive from big cats' round ears)
    for (float s : {-1.0f, 1.0f}) {
        sf::ConvexShape ear(3);
        ear.setPoint(0, {hc.x + s * 5.0f, hc.y - 6.0f});
        ear.setPoint(1, {hc.x + s * 2.0f, hc.y - 16.0f});
        ear.setPoint(2, {hc.x + s * 8.0f, hc.y - 10.0f});
        ear.setFillColor(dc);
        ear.setOutlineColor(sf::Color::Black);
        ear.setOutlineThickness(0.5f);
        target.draw(ear);
    }
    // Eyes
    for (float s : {-1.0f, 1.0f}) {
        sf::CircleShape eye(2.0f);
        eye.setOrigin({2.0f, 2.0f});
        eye.setPosition({hc.x + dir * 3.0f + s * 3.0f, hc.y - 1.0f});
        eye.setFillColor(sf::Color(100, 200, 100));
        target.draw(eye);
        sf::CircleShape pupil(1.0f);
        pupil.setOrigin({1.0f, 1.0f});
        pupil.setPosition({hc.x + dir * 3.5f + s * 3.0f, hc.y - 1.0f});
        pupil.setFillColor(sf::Color::Black);
        target.draw(pupil);
    }
    // Nose
    sf::CircleShape nose(1.5f);
    nose.setOrigin({1.5f, 1.5f});
    nose.setPosition({hc.x + dir * 7.0f, hc.y + 1.0f});
    nose.setFillColor(sf::Color(200, 100, 100));
    target.draw(nose);
    // Whiskers
    for (float wy : {-1.0f, 0.0f, 1.0f}) {
        sf::VertexArray w(sf::PrimitiveType::Lines, 2);
        w[0] = sf::Vertex{{hc.x + dir * 8.0f, hc.y + 1.0f + wy * 2.0f}, sf::Color::Black};
        w[1] = sf::Vertex{{hc.x + dir * 20.0f, hc.y + 1.0f + wy * 5.0f}, sf::Color::Black};
        target.draw(w);
    }
}
```

**Step 8: Add Cat case to `StickFigure::draw()` in `src/StickFigure.cpp`**

In the switch statement, add before Lion:

```cpp
case CharacterType::Cat:       drawCat(target); break;
```

**Step 9: Add Cat to `indexToType()` in `src/Game.cpp`**

Insert `case 1: return CharacterType::Cat;` and increment all subsequent cases:

```cpp
static CharacterType indexToType(int idx) {
    switch (idx % CHARACTER_TYPE_COUNT) {
        case 0: return CharacterType::Stick;
        case 1: return CharacterType::Cat;
        case 2: return CharacterType::Lion;
        case 3: return CharacterType::Tiger;
        case 4: return CharacterType::Jaguar;
        case 5: return CharacterType::Panther;
        case 6: return CharacterType::Cheetah;
        case 7: return CharacterType::Cobra;
        case 8: return CharacterType::Unicorn;
        case 9: return CharacterType::Crocodile;
        case 10: return CharacterType::StickLady;
        default: return CharacterType::Stick;
    }
}
```

**Step 10: Add Cat preview to `renderCharSelect()` in `src/Game.cpp`**

Add a `case CharacterType::Cat:` block in the character preview switch. Use the original Cat preview from main (small body, triangular ears, no mane/spots/stripes):

```cpp
case CharacterType::Cat: {
    // Small cat body
    sf::CircleShape catBody(14.0f);
    catBody.setScale({1.1f, 0.8f});
    catBody.setOrigin({14.0f, 14.0f});
    catBody.setPosition({cx, previewY});
    catBody.setFillColor(pc);
    catBody.setOutlineColor(sf::Color::Black); catBody.setOutlineThickness(1.0f);
    win.draw(catBody);
    // Head
    sf::CircleShape catHead(10.0f);
    catHead.setOrigin({10.0f, 10.0f});
    catHead.setPosition({cx + 14.0f, previewY - 10.0f});
    catHead.setFillColor(pc);
    catHead.setOutlineColor(sf::Color::Black); catHead.setOutlineThickness(1.0f);
    win.draw(catHead);
    // Pointy ears
    for (float es : {-1.0f, 1.0f}) {
        sf::ConvexShape ear(3);
        ear.setPoint(0, {cx + 14.0f + es * 6.0f, previewY - 18.0f});
        ear.setPoint(1, {cx + 14.0f + es * 3.0f, previewY - 30.0f});
        ear.setPoint(2, {cx + 14.0f + es * 10.0f, previewY - 22.0f});
        ear.setFillColor(pc); win.draw(ear);
    }
    break;
}
```

**Step 11: Add Cat innate weapon equip in `startGame()` in `src/Game.cpp`**

In the innate weapon if/else chain, add before Lion:

```cpp
if (ct == CharacterType::Cat) {
    auto* w = m_weaponFactory.getWeapon("Cat Scratch");
    if (w) p->equipWeapon(*w);
} else if (ct == CharacterType::Lion) {
```

**Step 12: Create `assets/weapons/cat_scratch.json`**

```json
{
    "name": "Cat Scratch",
    "type": "melee",
    "damage": 8,
    "knockback_force": 4.0,
    "range": 1.5,
    "attack_rate_seconds": 0.15,
    "env_damage_radius": 0.1,
    "description": "Rapid scratching claws. Low damage but very fast."
}
```

No `"spawnable"` key — defaults to `false`.

**Step 13: Build and verify**

```bash
./build.sh
```

**Step 14: Commit**

```bash
git add src/StickFigure.h src/StickFigure.cpp src/Game.cpp assets/weapons/cat_scratch.json
git commit -m "restore Cat as housecat character, distinct from Jaguar"
```

---

### Task 3: Restore stripped comments (I3)

Restore useful comments that were removed in the big-cats commit.

**Files:**
- Modify: `src/StickFigure.h`
- Modify: `src/StickFigure.cpp`
- Modify: `src/Game.h`
- Modify: `src/Game.cpp`

**Step 1: Restore comments in `src/StickFigure.h`**

Restore these comments:
- `// Number of available character types` above `constexpr int CHARACTER_TYPE_COUNT`
- `// Aiming` above `void aimUp();`
- `void teleportTo(float x, float y); // preserves velocity (for wrap-around)`
- `float m_aimAngle = 0.0f; // radians, 0=straight, positive=up, negative=down`
- `// Poison DOT` above `float m_poisonTimer`
- `// Animation` above `float m_animTime`

**Step 2: Restore comments in `src/Game.h`**

Restore these comments:
- `float x, y;              // world position` (ExplosionEffect)
- `float radius;            // blast radius in meters`
- `float timer = 0.0f;      // time since detonation`
- `float duration = 1.5f;   // how long the effect lasts`
- `bool isNuke = false;     // nuke gets special visuals`
- `// Per-player selection state during character select` above PlayerSelectState
- `int  charIndex = 0;  // index into CharacterType enum`
- `// Character select` above processCharSelectEvents
- `// Gameplay` above processEvents
- `// Character select state` above m_selectState
- `bool  m_wrapAround = false;  // fall-through wrap-around mode`
- `sf::Color(255, 200, 100),  // Gold (unicorn default)` comment

**Step 3: Revert em dash to double dash changes in `src/Game.cpp` and `src/StickFigure.cpp`**

Find all `--` that were changed from `—` and revert them. These are in comments only, purely cosmetic. Revert to the original em dashes.

**Step 4: Commit**

```bash
git add src/StickFigure.h src/StickFigure.cpp src/Game.h src/Game.cpp
git commit -m "restore stripped comments and revert cosmetic em dash changes"
```

---

### Task 4: Health multiplier system (I4)

Replace hardcoded health values with a multiplier that respects RulesEngine.

**Files:**
- Modify: `src/StickFigure.h`
- Modify: `src/StickFigure.cpp`
- Modify: `src/Game.cpp`

**Step 1: Add `m_healthMultiplier` to `src/StickFigure.h`**

In the private section, near `m_damageMultiplier`:

```cpp
float m_healthMultiplier = 1.0f;
```

Add public accessor:

```cpp
float getHealthMultiplier() const { return m_healthMultiplier; }
```

**Step 2: Update `applyCharacterStats()` in `src/StickFigure.cpp`**

Remove all `m_maxHealth = ...` and `m_health = ...` lines. Replace with `m_healthMultiplier`:

```cpp
void StickFigure::applyCharacterStats() {
    switch (m_charType) {
        case CharacterType::Cat:
            m_healthMultiplier = 0.8f;
            m_moveSpeed = 10.0f;
            m_jumpForce = 13.0f;
            break;
        case CharacterType::Lion:
            m_healthMultiplier = 1.3f;
            m_moveSpeed = 7.5f;
            m_jumpForce = 12.0f;
            break;
        case CharacterType::Tiger:
            m_healthMultiplier = 1.1f;
            m_moveSpeed = 6.5f;
            m_jumpForce = 11.0f;
            m_damageMultiplier = 1.25f;
            break;
        case CharacterType::Cheetah:
            m_healthMultiplier = 0.75f;
            m_moveSpeed = 11.5f;
            m_jumpForce = 14.0f;
            break;
        case CharacterType::Jaguar:
            m_healthMultiplier = 1.0f;
            m_moveSpeed = 8.5f;
            m_jumpForce = 13.0f;
            break;
        case CharacterType::Panther:
            m_healthMultiplier = 0.95f;
            m_moveSpeed = 9.5f;
            m_jumpForce = 13.0f;
            break;
        default:
            break;
    }
}
```

**Step 3: Update `startGame()` in `src/Game.cpp`**

Remove the `if (!isBigCat(ct))` guard. Apply health for ALL characters:

```cpp
p->setMaxHealth(rules.maxHealth * p->getHealthMultiplier());
```

This replaces:
```cpp
if (!isBigCat(ct)) {
    p->setMaxHealth(rules.maxHealth);
}
```

**Step 4: Build and verify**

```bash
./build.sh
```

**Step 5: Commit**

```bash
git add src/StickFigure.h src/StickFigure.cpp src/Game.cpp
git commit -m "use health multiplier system with RulesEngine base for all characters"
```

---

### Task 5: Scope damageMultiplier to melee only (I5)

Remove damage multiplier from projectile/explosive damage. Keep it only in melee.

**Files:**
- Modify: `src/Game.h`
- Modify: `src/Game.cpp`

**Step 1: Remove `ownerDamageMultiplier` from `Projectile` struct in `src/Game.h`**

Delete this line from the Projectile struct:

```cpp
float ownerDamageMultiplier = 1.0f;
```

**Step 2: Remove multiplier assignment in `spawnProjectile()` in `src/Game.cpp`**

Delete this line:

```cpp
proj.ownerDamageMultiplier = shooter.getDamageMultiplier();
```

**Step 3: Remove multiplier from projectile damage calculations in `src/Game.cpp`**

In `updateProjectiles()`, find two lines that use `proj.ownerDamageMultiplier` and revert to:

```cpp
float dmg = proj.weapon.damage * rules.damageMultiplier;
```

(Remove `* proj.ownerDamageMultiplier` from both occurrences.)

**Step 4: Verify melee still uses multiplier**

Confirm `handleMeleeAttack()` still has:

```cpp
float dmg = weapon.damage * rules.damageMultiplier * attacker.getDamageMultiplier();
```

This should already be correct — no changes needed here.

**Step 5: Build and verify**

```bash
./build.sh
```

**Step 6: Commit**

```bash
git add src/Game.h src/Game.cpp
git commit -m "scope damage multiplier to melee attacks only"
```

---

### Task 6: Change wall climb input (I6)

Wall climb triggers on move-toward-wall while airborne, not aim-up.

**Files:**
- Modify: `src/StickFigure.h`
- Modify: `src/StickFigure.cpp`
- Modify: `src/Game.cpp`

**Step 1: Update `isTouchingWall()` to return wall side in `src/StickFigure.h` and `src/StickFigure.cpp`**

Change the signature to return which side the wall is on (0 = no wall, -1 = left, 1 = right):

In `src/StickFigure.h`, change:
```cpp
bool isTouchingWall() const;
```
to:
```cpp
int wallSide() const;  // 0 = no wall, -1 = wall on left, 1 = wall on right
```

In `src/StickFigure.cpp`, update:
```cpp
int StickFigure::wallSide() const {
    b2Vec2 pos = b2Body_GetPosition(m_torso);
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.categoryBits = CAT_PLAYER;
    filter.maskBits = CAT_PLATFORM;
    for (float side : {-1.0f, 1.0f}) {
        b2Vec2 origin = {pos.x + side * (m_config.bodyWidth / 2.0f + 0.05f), pos.y};
        b2Vec2 translation = {side * 0.3f, 0.0f};
        b2RayResult result = b2World_CastRayClosest(m_physics->getWorldId(), origin, translation, filter);
        if (result.hit) return static_cast<int>(side);
    }
    return 0;
}
```

**Step 2: Update `wallClimbUp()` in `src/StickFigure.cpp`**

Change `isTouchingWall()` call to `wallSide()`:
```cpp
void StickFigure::wallClimbUp() {
    if (!canWallClimb(m_charType)) return;
    if (wallSide() == 0) return;
    b2Vec2 v = b2Body_GetLinearVelocity(m_torso);
    b2Body_SetLinearVelocity(m_torso, {v.x, m_moveSpeed * 0.7f});
}
```

**Step 3: Update wall climb indicator in `draw()` in `src/StickFigure.cpp`**

Change:
```cpp
if (canWallClimb(m_charType) && isTouchingWall() && !isOnGround()) {
```
to:
```cpp
if (canWallClimb(m_charType) && wallSide() != 0 && !isOnGround()) {
```

**Step 4: Change wall climb input in `handlePlayerInput()` in `src/Game.cpp`**

Replace the aim-up based wall climb with move-toward-wall:

```cpp
// Wall climbing for Cat/Jaguar/Panther: move toward wall while airborne
if (canWallClimb(player->getCharacterType()) && !player->isOnGround()) {
    int ws = player->wallSide();
    if ((ws == -1 && pi.moveLeft) || (ws == 1 && pi.moveRight)) {
        player->wallClimbUp();
    }
}

// Aiming (no longer conflicts with wall climb)
if (pi.aimUp) player->aimUp();
else if (pi.aimDown) player->aimDown();
else player->resetAim();
```

Remove the old `didWallClimb` variable and its usage entirely.

**Step 5: Build and verify**

```bash
./build.sh
```

**Step 6: Commit**

```bash
git add src/StickFigure.h src/StickFigure.cpp src/Game.cpp
git commit -m "change wall climb input to move-toward-wall while airborne"
```

---

### Task 7: Update design docs

**Files:**
- Modify: `design/design.md`
- Modify: `design/ToDo.md`
- Modify: `design/Done.md`

**Step 1: Move resolved items from ToDo.md to Done.md**

Move I1-I7 to Done.md with brief resolution notes.

**Step 2: Update design.md**

Update the Character System section to reflect:
- Cat restored as housecat with wall climb
- Health multiplier system documented
- Damage multiplier melee-only rule
- Wall climb input documented (move-toward-wall)
- `canWallClimb()` returns true for Cat, Jaguar, Panther

**Step 3: Commit**

```bash
git add design/
git commit -m "update design docs with merge review decisions"
```

---

## Verification

After all tasks, run a full build:

```bash
./build.sh
```

Then manually verify in-game:
- Cat appears in character select with blurb "80% HP | Wall Climb | Fast Scratch"
- Cat visual is small with pointy ears, distinct from Jaguar
- Cat can wall climb by moving into walls while airborne
- All characters' health scales with RulesEngine maxHealth
- Tiger's damage bonus only applies to melee
- Aim-up works near walls for all characters

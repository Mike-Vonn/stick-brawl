#include "Arena.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

// ============================================================
// PLATFORM TYPE PROPERTIES
// ============================================================

float Arena::healthForType(PlatformType type) {
    switch (type) {
        case PlatformType::Wood:   return 60.0f;
        case PlatformType::Brick:  return 120.0f;
        case PlatformType::Stone:  return 180.0f;
        case PlatformType::Metal:  return 300.0f;
        case PlatformType::Roof:   return 50.0f;
        case PlatformType::Ground: return 500.0f;
        default: return 100.0f;
    }
}

sf::Color Arena::fillColorForType(PlatformType type) {
    switch (type) {
        case PlatformType::Wood:   return sf::Color(120, 80, 40);
        case PlatformType::Brick:  return sf::Color(150, 60, 50);
        case PlatformType::Stone:  return sf::Color(110, 110, 115);
        case PlatformType::Metal:  return sf::Color(140, 150, 170);
        case PlatformType::Roof:   return sf::Color(140, 50, 40);
        case PlatformType::Ground: return sf::Color(70, 75, 70);
        default: return sf::Color(80, 80, 80);
    }
}

sf::Color Arena::outlineColorForType(PlatformType type) {
    switch (type) {
        case PlatformType::Wood:   return sf::Color(90, 60, 30);
        case PlatformType::Brick:  return sf::Color(120, 40, 35);
        case PlatformType::Stone:  return sf::Color(80, 80, 85);
        case PlatformType::Metal:  return sf::Color(170, 180, 200);
        case PlatformType::Roof:   return sf::Color(110, 35, 30);
        case PlatformType::Ground: return sf::Color(100, 105, 100);
        default: return sf::Color(150, 150, 150);
    }
}

// ============================================================
// LEVEL MANAGEMENT
// ============================================================

int Arena::getLevelCount() { return 4; }

std::string Arena::getLevelName(int index) {
    switch (index) {
        case 0: return "Classic";
        case 1: return "Village";
        case 2: return "Fortress";
        case 3: return "Skyscrapers";
        default: return "???";
    }
}

void Arena::addPlatform(Physics& physics, float cx, float cy, float hw, float hh, PlatformType type) {
    Platform p;
    p.cx = cx; p.cy = cy; p.halfWidth = hw; p.halfHeight = hh;
    p.bodyId = physics.createStaticBox(cx, cy, hw, hh, CAT_PLATFORM);
    p.alive = true;
    p.type = type;
    p.maxHealth = healthForType(type);
    p.health = p.maxHealth;
    m_platforms.push_back(p);
}

void Arena::createLevel(Physics& physics, int levelIndex) {
    m_physics = &physics;
    m_platforms.clear();
    m_spawnPoints.clear();
    m_currentLevel = levelIndex;

    switch (levelIndex) {
        case 0: buildClassic(physics); break;
        case 1: buildVillage(physics); break;
        case 2: buildFortress(physics); break;
        case 3: buildSkyscrapers(physics); break;
        default: buildClassic(physics); break;
    }

    std::cout << "[Arena] Built level: " << getLevelName(levelIndex)
              << " (" << m_platforms.size() << " platforms)\n";
}

// ============================================================
// LEVEL LAYOUTS
// ============================================================

void Arena::buildClassic(Physics& physics) {
    // Original layout
    addPlatform(physics, 0.0f, -5.0f, 15.0f, 0.5f, PlatformType::Ground);
    addPlatform(physics, -8.0f, -1.0f, 3.0f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 8.0f, -1.0f, 3.0f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 0.0f, 2.0f, 2.5f, 0.3f, PlatformType::Wood);
    addPlatform(physics, -4.0f, 4.5f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 4.0f, 4.5f, 1.5f, 0.2f, PlatformType::Wood);

    m_spawnPoints = {
        {-10.0f, -3.5f}, {10.0f, -3.5f},
        {-5.0f, -3.5f}, {5.0f, -3.5f}, {0.0f, -3.5f},
    };
}

void Arena::buildVillage(Physics& physics) {
    // Ground
    addPlatform(physics, 0.0f, -6.0f, 18.0f, 0.5f, PlatformType::Ground);

    // === Left house ===
    // Floor
    addPlatform(physics, -11.0f, -4.0f, 3.5f, 0.2f, PlatformType::Wood);
    // Left wall
    addPlatform(physics, -14.3f, -2.5f, 0.3f, 1.7f, PlatformType::Brick);
    // Right wall
    addPlatform(physics, -7.7f, -2.5f, 0.3f, 1.7f, PlatformType::Brick);
    // Roof
    addPlatform(physics, -11.0f, -0.6f, 4.0f, 0.2f, PlatformType::Roof);
    // Interior shelf
    addPlatform(physics, -12.0f, -2.8f, 1.2f, 0.15f, PlatformType::Wood);

    // === Center market stall ===
    addPlatform(physics, 0.0f, -3.5f, 2.0f, 0.15f, PlatformType::Wood);
    // Stall posts (thin walls)
    addPlatform(physics, -1.8f, -4.5f, 0.15f, 1.2f, PlatformType::Wood);
    addPlatform(physics, 1.8f, -4.5f, 0.15f, 1.2f, PlatformType::Wood);

    // === Right house (taller, stone) ===
    // Floor
    addPlatform(physics, 10.0f, -4.0f, 3.0f, 0.2f, PlatformType::Stone);
    // Walls
    addPlatform(physics, 7.2f, -2.0f, 0.3f, 2.2f, PlatformType::Stone);
    addPlatform(physics, 12.8f, -2.0f, 0.3f, 2.2f, PlatformType::Stone);
    // Second floor
    addPlatform(physics, 10.0f, -0.5f, 3.0f, 0.2f, PlatformType::Wood);
    // Upper walls
    addPlatform(physics, 7.2f, 1.2f, 0.3f, 1.5f, PlatformType::Brick);
    addPlatform(physics, 12.8f, 1.2f, 0.3f, 1.5f, PlatformType::Brick);
    // Roof
    addPlatform(physics, 10.0f, 2.9f, 3.5f, 0.2f, PlatformType::Roof);

    // === Small fence/wall in middle ===
    addPlatform(physics, -4.0f, -4.8f, 0.2f, 0.8f, PlatformType::Wood);
    addPlatform(physics, 4.0f, -4.8f, 0.2f, 0.8f, PlatformType::Wood);

    // Floating platform high up
    addPlatform(physics, -3.0f, 3.0f, 2.0f, 0.2f, PlatformType::Metal);

    m_spawnPoints = {
        {-11.0f, -3.0f}, {10.0f, -3.0f},
        {0.0f, -2.5f}, {-5.0f, -4.5f}, {5.0f, -4.5f},
    };
}

void Arena::buildFortress(Physics& physics) {
    // Ground with a moat gap in the center
    addPlatform(physics, -10.0f, -6.0f, 8.0f, 0.5f, PlatformType::Ground);
    addPlatform(physics, 10.0f, -6.0f, 8.0f, 0.5f, PlatformType::Ground);

    // === Left fortress tower ===
    // Base
    addPlatform(physics, -12.0f, -4.0f, 2.5f, 0.3f, PlatformType::Stone);
    // Tower walls
    addPlatform(physics, -14.3f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, -9.7f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    // Battlements floor
    addPlatform(physics, -12.0f, 1.5f, 3.0f, 0.2f, PlatformType::Stone);
    // Battlement walls (short)
    addPlatform(physics, -14.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
    addPlatform(physics, -9.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);

    // === Right fortress tower ===
    addPlatform(physics, 12.0f, -4.0f, 2.5f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 14.3f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, 9.7f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, 12.0f, 1.5f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 14.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
    addPlatform(physics, 9.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);

    // === Central bridge (wood, destructible!) ===
    addPlatform(physics, -3.0f, -3.0f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 3.0f, -3.0f, 2.5f, 0.2f, PlatformType::Wood);
    // Bridge supports
    addPlatform(physics, -5.2f, -4.5f, 0.2f, 1.3f, PlatformType::Wood);
    addPlatform(physics, 5.2f, -4.5f, 0.2f, 1.3f, PlatformType::Wood);

    // Central arena platform
    addPlatform(physics, 0.0f, 0.5f, 2.0f, 0.2f, PlatformType::Metal);

    // High floating platforms
    addPlatform(physics, -6.0f, 4.0f, 1.5f, 0.2f, PlatformType::Metal);
    addPlatform(physics, 6.0f, 4.0f, 1.5f, 0.2f, PlatformType::Metal);
    addPlatform(physics, 0.0f, 5.5f, 1.0f, 0.15f, PlatformType::Metal);

    m_spawnPoints = {
        {-12.0f, -3.0f}, {12.0f, -3.0f},
        {-3.0f, -2.0f}, {3.0f, -2.0f}, {0.0f, 1.5f},
    };
}

void Arena::buildSkyscrapers(Physics& physics) {
    // No main ground — just buildings rising from the void
    // Fall = death

    // === Left building (short, wide) ===
    addPlatform(physics, -13.0f, -2.0f, 3.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, -15.8f, -4.0f, 0.3f, 2.3f, PlatformType::Metal);
    addPlatform(physics, -10.2f, -4.0f, 0.3f, 2.3f, PlatformType::Metal);
    // Interior floors
    addPlatform(physics, -13.0f, -4.5f, 2.5f, 0.15f, PlatformType::Metal);

    // === Center-left building (tall) ===
    addPlatform(physics, -5.0f, 2.0f, 2.0f, 0.3f, PlatformType::Metal); // roof
    addPlatform(physics, -6.8f, -1.0f, 0.3f, 3.3f, PlatformType::Stone);
    addPlatform(physics, -3.2f, -1.0f, 0.3f, 3.3f, PlatformType::Stone);
    addPlatform(physics, -5.0f, -1.5f, 1.5f, 0.15f, PlatformType::Wood); // floor 1
    addPlatform(physics, -5.0f, 0.5f, 1.5f, 0.15f, PlatformType::Wood);  // floor 2

    // === Center-right building (tallest) ===
    addPlatform(physics, 4.0f, 4.0f, 2.5f, 0.3f, PlatformType::Metal); // roof
    addPlatform(physics, 1.7f, 0.5f, 0.3f, 3.8f, PlatformType::Stone);
    addPlatform(physics, 6.3f, 0.5f, 0.3f, 3.8f, PlatformType::Stone);
    addPlatform(physics, 4.0f, -1.0f, 2.0f, 0.15f, PlatformType::Wood); // floor 1
    addPlatform(physics, 4.0f, 1.0f, 2.0f, 0.15f, PlatformType::Wood);  // floor 2
    addPlatform(physics, 4.0f, 3.0f, 2.0f, 0.15f, PlatformType::Wood);  // floor 3

    // === Right building (medium) ===
    addPlatform(physics, 13.0f, 0.0f, 2.5f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 10.7f, -3.0f, 0.3f, 3.3f, PlatformType::Brick);
    addPlatform(physics, 15.3f, -3.0f, 0.3f, 3.3f, PlatformType::Brick);
    addPlatform(physics, 13.0f, -2.5f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 13.0f, -0.5f, 2.0f, 0.15f, PlatformType::Wood);

    // Connecting bridges between buildings (fragile)
    addPlatform(physics, -9.0f, -1.0f, 1.2f, 0.12f, PlatformType::Wood);
    addPlatform(physics, -0.5f, 1.5f, 1.5f, 0.12f, PlatformType::Wood);
    addPlatform(physics, 9.0f, 1.0f, 1.5f, 0.12f, PlatformType::Wood);

    m_spawnPoints = {
        {-13.0f, -1.0f}, {13.0f, 1.0f},
        {-5.0f, -0.5f}, {4.0f, -0.0f}, {4.0f, 4.8f},
    };
}

// ============================================================
// RANDOM PLATFORM TOP
// ============================================================

b2Vec2 Arena::getRandomPlatformTop() const {
    std::vector<size_t> aliveIdx;
    for (size_t i = 0; i < m_platforms.size(); i++) {
        if (m_platforms[i].alive && m_platforms[i].halfWidth > 0.5f) aliveIdx.push_back(i);
    }
    if (aliveIdx.empty()) return {0.0f, 0.0f};

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, aliveIdx.size() - 1);
    const auto& p = m_platforms[aliveIdx[dist(rng)]];

    std::uniform_real_distribution<float> xDist(-p.halfWidth * 0.8f, p.halfWidth * 0.8f);
    return {p.cx + xDist(rng), p.cy + p.halfHeight + 0.5f};
}

// ============================================================
// DAMAGE / DESTRUCTION
// ============================================================

void Arena::damagePlatformsInRadius(Physics& physics, float ex, float ey, float radius, float damage) {
    std::vector<Platform> newPlatforms;
    bool anyDestroyed = false;

    for (auto& plat : m_platforms) {
        if (!plat.alive) continue;

        float platLeft  = plat.cx - plat.halfWidth;
        float platRight = plat.cx + plat.halfWidth;
        float platBot   = plat.cy - plat.halfHeight;
        float platTop   = plat.cy + plat.halfHeight;

        float closestX = std::clamp(ex, platLeft, platRight);
        float closestY = std::clamp(ey, platBot, platTop);
        float dx = ex - closestX;
        float dy = ey - closestY;
        float distSq = dx * dx + dy * dy;

        if (distSq >= radius * radius) continue;

        // Scale damage by distance
        float dist = std::sqrt(distSq);
        float falloff = 1.0f - (dist / radius);
        float actualDmg = damage * std::max(0.2f, falloff);

        plat.health -= actualDmg;

        if (plat.health <= 0.0f) {
            // Platform destroyed — use the splitting logic
            b2DestroyBody(plat.bodyId);
            plat.alive = false;
            anyDestroyed = true;

            // Only create remnants for wide platforms
            if (plat.halfWidth > 1.0f) {
                float blastLeft  = ex - radius * 0.5f;
                float blastRight = ex + radius * 0.5f;
                float overlapLeft  = std::max(platLeft, blastLeft);
                float overlapRight = std::min(platRight, blastRight);

                float leftW = (overlapLeft - platLeft) / 2.0f;
                if (leftW > 0.4f) {
                    Platform r;
                    r.halfWidth = leftW; r.halfHeight = plat.halfHeight;
                    r.cx = platLeft + leftW; r.cy = plat.cy;
                    r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                    r.alive = true; r.type = plat.type;
                    r.maxHealth = plat.maxHealth * 0.6f;
                    r.health = r.maxHealth;
                    newPlatforms.push_back(r);
                }
                float rightW = (platRight - overlapRight) / 2.0f;
                if (rightW > 0.4f) {
                    Platform r;
                    r.halfWidth = rightW; r.halfHeight = plat.halfHeight;
                    r.cx = overlapRight + rightW; r.cy = plat.cy;
                    r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                    r.alive = true; r.type = plat.type;
                    r.maxHealth = plat.maxHealth * 0.6f;
                    r.health = r.maxHealth;
                    newPlatforms.push_back(r);
                }
            }
        }
    }

    if (anyDestroyed) {
        m_platforms.erase(
            std::remove_if(m_platforms.begin(), m_platforms.end(),
                            [](const Platform& p) { return !p.alive; }),
            m_platforms.end());
        for (auto& np : newPlatforms) m_platforms.push_back(np);
    }
}

int Arena::destroyPlatformsInRadius(Physics& physics, float ex, float ey, float radius) {
    // Nuke-style: just deal massive damage
    damagePlatformsInRadius(physics, ex, ey, radius, 9999.0f);
    return 1;
}

// ============================================================
// RENDERING
// ============================================================

void Arena::draw(sf::RenderTarget& target) const {
    auto toScreen = [](float x, float y) -> sf::Vector2f {
        return {SCREEN_CX + x * PPM, SCREEN_CY - y * PPM};
    };

    for (const auto& p : m_platforms) {
        if (!p.alive) continue;

        float w = p.halfWidth * 2.0f * PPM;
        float h = p.halfHeight * 2.0f * PPM;
        sf::RectangleShape rect({w, h});
        rect.setPosition(toScreen(p.cx - p.halfWidth, p.cy + p.halfHeight));

        sf::Color fill = fillColorForType(p.type);
        sf::Color outline = outlineColorForType(p.type);

        // Darken damaged platforms
        float hpRatio = p.health / p.maxHealth;
        if (hpRatio < 1.0f) {
            float darken = 0.4f + 0.6f * hpRatio;
            fill.r = static_cast<uint8_t>(fill.r * darken);
            fill.g = static_cast<uint8_t>(fill.g * darken);
            fill.b = static_cast<uint8_t>(fill.b * darken);
        }

        rect.setFillColor(fill);
        rect.setOutlineColor(outline);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);

        // Damage cracks for platforms below 50% HP
        if (hpRatio < 0.5f) {
            sf::Vector2f tl = toScreen(p.cx - p.halfWidth, p.cy + p.halfHeight);
            int cracks = static_cast<int>((1.0f - hpRatio) * 4) + 1;
            for (int c = 0; c < cracks; c++) {
                float frac = static_cast<float>(c + 1) / static_cast<float>(cracks + 1);
                float cx1 = tl.x + frac * w;
                sf::VertexArray crack(sf::PrimitiveType::LineStrip, 3);
                crack[0] = sf::Vertex{{cx1, tl.y}, sf::Color(30, 30, 30, 150)};
                crack[1] = sf::Vertex{{cx1 + 3.0f, tl.y + h * 0.5f}, sf::Color(30, 30, 30, 120)};
                crack[2] = sf::Vertex{{cx1 - 2.0f, tl.y + h}, sf::Color(30, 30, 30, 80)};
                target.draw(crack);
            }
        }

        // Texture details based on type
        sf::Vector2f tl = toScreen(p.cx - p.halfWidth, p.cy + p.halfHeight);
        if (p.type == PlatformType::Brick && w > 10.0f && h > 6.0f) {
            // Brick pattern: horizontal lines
            for (float by = 6.0f; by < h; by += 6.0f) {
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0] = sf::Vertex{{tl.x + 1.0f, tl.y + by}, sf::Color(100, 35, 30, 80)};
                line[1] = sf::Vertex{{tl.x + w - 1.0f, tl.y + by}, sf::Color(100, 35, 30, 80)};
                target.draw(line);
            }
        } else if (p.type == PlatformType::Wood && w > 8.0f) {
            // Wood grain: horizontal lines
            for (float wy = 4.0f; wy < h; wy += 5.0f) {
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0] = sf::Vertex{{tl.x + 2.0f, tl.y + wy}, sf::Color(80, 55, 25, 60)};
                line[1] = sf::Vertex{{tl.x + w - 2.0f, tl.y + wy}, sf::Color(80, 55, 25, 60)};
                target.draw(line);
            }
        } else if (p.type == PlatformType::Metal && w > 8.0f) {
            // Rivet dots
            for (float rx = 6.0f; rx < w; rx += 12.0f) {
                sf::CircleShape rivet(1.5f);
                rivet.setOrigin({1.5f, 1.5f});
                rivet.setPosition({tl.x + rx, tl.y + h * 0.5f});
                rivet.setFillColor(sf::Color(180, 190, 210, 100));
                target.draw(rivet);
            }
        }
    }
}

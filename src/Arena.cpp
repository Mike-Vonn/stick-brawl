#include "Arena.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

// ============================================================
// PLATFORM TYPE COLORS
// ============================================================

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
    addPlatform(physics, 0.0f, -6.0f, 18.0f, 0.5f, PlatformType::Ground);

    // Left house
    addPlatform(physics, -11.0f, -4.0f, 3.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -14.3f, -2.5f, 0.3f, 1.7f, PlatformType::Brick);
    addPlatform(physics, -7.7f, -2.5f, 0.3f, 1.7f, PlatformType::Brick);
    addPlatform(physics, -11.0f, -0.6f, 4.0f, 0.2f, PlatformType::Roof);
    addPlatform(physics, -12.0f, -2.8f, 1.2f, 0.15f, PlatformType::Wood);

    // Center market stall
    addPlatform(physics, 0.0f, -3.5f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, -1.8f, -4.5f, 0.15f, 1.2f, PlatformType::Wood);
    addPlatform(physics, 1.8f, -4.5f, 0.15f, 1.2f, PlatformType::Wood);

    // Right house (2-story)
    addPlatform(physics, 10.0f, -4.0f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 7.2f, -2.0f, 0.3f, 2.2f, PlatformType::Stone);
    addPlatform(physics, 12.8f, -2.0f, 0.3f, 2.2f, PlatformType::Stone);
    addPlatform(physics, 10.0f, -0.5f, 3.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 7.2f, 1.2f, 0.3f, 1.5f, PlatformType::Brick);
    addPlatform(physics, 12.8f, 1.2f, 0.3f, 1.5f, PlatformType::Brick);
    addPlatform(physics, 10.0f, 2.9f, 3.5f, 0.2f, PlatformType::Roof);

    // Fences
    addPlatform(physics, -4.0f, -4.8f, 0.2f, 0.8f, PlatformType::Wood);
    addPlatform(physics, 4.0f, -4.8f, 0.2f, 0.8f, PlatformType::Wood);

    addPlatform(physics, -3.0f, 3.0f, 2.0f, 0.2f, PlatformType::Metal);

    m_spawnPoints = {
        {-11.0f, -3.0f}, {10.0f, -3.0f},
        {0.0f, -2.5f}, {-5.0f, -4.5f}, {5.0f, -4.5f},
    };
}

void Arena::buildFortress(Physics& physics) {
    addPlatform(physics, -10.0f, -6.0f, 8.0f, 0.5f, PlatformType::Ground);
    addPlatform(physics, 10.0f, -6.0f, 8.0f, 0.5f, PlatformType::Ground);

    // Left tower
    addPlatform(physics, -12.0f, -4.0f, 2.5f, 0.3f, PlatformType::Stone);
    addPlatform(physics, -14.3f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, -9.7f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, -12.0f, 1.5f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -14.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
    addPlatform(physics, -9.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);

    // Right tower
    addPlatform(physics, 12.0f, -4.0f, 2.5f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 14.3f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, 9.7f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, 12.0f, 1.5f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 14.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
    addPlatform(physics, 9.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);

    // Bridge
    addPlatform(physics, -3.0f, -3.0f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 3.0f, -3.0f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -5.2f, -4.5f, 0.2f, 1.3f, PlatformType::Wood);
    addPlatform(physics, 5.2f, -4.5f, 0.2f, 1.3f, PlatformType::Wood);

    addPlatform(physics, 0.0f, 0.5f, 2.0f, 0.2f, PlatformType::Metal);
    addPlatform(physics, -6.0f, 4.0f, 1.5f, 0.2f, PlatformType::Metal);
    addPlatform(physics, 6.0f, 4.0f, 1.5f, 0.2f, PlatformType::Metal);
    addPlatform(physics, 0.0f, 5.5f, 1.0f, 0.15f, PlatformType::Metal);

    m_spawnPoints = {
        {-12.0f, -3.0f}, {12.0f, -3.0f},
        {-3.0f, -2.0f}, {3.0f, -2.0f}, {0.0f, 1.5f},
    };
}

void Arena::buildSkyscrapers(Physics& physics) {
    // Left building
    addPlatform(physics, -13.0f, -2.0f, 3.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, -15.8f, -4.0f, 0.3f, 2.3f, PlatformType::Metal);
    addPlatform(physics, -10.2f, -4.0f, 0.3f, 2.3f, PlatformType::Metal);
    addPlatform(physics, -13.0f, -4.5f, 2.5f, 0.15f, PlatformType::Metal);

    // Center-left building
    addPlatform(physics, -5.0f, 2.0f, 2.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, -6.8f, -1.0f, 0.3f, 3.3f, PlatformType::Stone);
    addPlatform(physics, -3.2f, -1.0f, 0.3f, 3.3f, PlatformType::Stone);
    addPlatform(physics, -5.0f, -1.5f, 1.5f, 0.15f, PlatformType::Wood);
    addPlatform(physics, -5.0f, 0.5f, 1.5f, 0.15f, PlatformType::Wood);

    // Center-right building (tallest)
    addPlatform(physics, 4.0f, 4.0f, 2.5f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 1.7f, 0.5f, 0.3f, 3.8f, PlatformType::Stone);
    addPlatform(physics, 6.3f, 0.5f, 0.3f, 3.8f, PlatformType::Stone);
    addPlatform(physics, 4.0f, -1.0f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 4.0f, 1.0f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 4.0f, 3.0f, 2.0f, 0.15f, PlatformType::Wood);

    // Right building
    addPlatform(physics, 13.0f, 0.0f, 2.5f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 10.7f, -3.0f, 0.3f, 3.3f, PlatformType::Brick);
    addPlatform(physics, 15.3f, -3.0f, 0.3f, 3.3f, PlatformType::Brick);
    addPlatform(physics, 13.0f, -2.5f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 13.0f, -0.5f, 2.0f, 0.15f, PlatformType::Wood);

    // Bridges
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
// WORMS-STYLE TERRAIN CARVING
// ============================================================
//
// Carves a circular hole out of all overlapping platforms.
// Each affected platform is destroyed and replaced by up to 4
// axis-aligned remnant rectangles (left, right, top, bottom)
// from the portions outside the carve circle's bounding box.
//
//   +--[===LEFT===]--[  CARVED  ]--[==RIGHT==]--+
//   |                [  circle  ]                |
//   +--[=============BOTTOM=============]--------+
//   +--[==============TOP===============]--------+
//
// Tiny remnants (< minimum size) are discarded. This gives a
// chunky, blocky deformation that looks like Worms terrain.

int Arena::carveCircle(Physics& physics, float ex, float ey, float radius) {
    if (radius < 0.05f) return 0;

    int affected = 0;
    std::vector<Platform> newPlatforms;

    // Carve bounding box
    float carveLeft   = ex - radius;
    float carveRight  = ex + radius;
    float carveBottom = ey - radius;
    float carveTop    = ey + radius;

    constexpr float MIN_HW = 0.15f; // minimum half-width for a remnant
    constexpr float MIN_HH = 0.08f; // minimum half-height for a remnant

    for (auto& plat : m_platforms) {
        if (!plat.alive) continue;

        float pLeft   = plat.cx - plat.halfWidth;
        float pRight  = plat.cx + plat.halfWidth;
        float pBottom = plat.cy - plat.halfHeight;
        float pTop    = plat.cy + plat.halfHeight;

        // Quick AABB check: does the carve bbox overlap the platform?
        if (carveRight < pLeft || carveLeft > pRight ||
            carveTop < pBottom || carveBottom > pTop) continue;

        // Finer check: closest point on rect to circle center
        float closestX = std::clamp(ex, pLeft, pRight);
        float closestY = std::clamp(ey, pBottom, pTop);
        float dx = ex - closestX;
        float dy = ey - closestY;
        if (dx * dx + dy * dy >= radius * radius) continue;

        // This platform IS affected — destroy it
        affected++;
        b2DestroyBody(plat.bodyId);
        plat.alive = false;

        PlatformType type = plat.type;

        // Clamp carve bbox to the platform bounds
        float cLeft   = std::max(carveLeft,   pLeft);
        float cRight  = std::min(carveRight,  pRight);
        float cBottom = std::max(carveBottom, pBottom);
        float cTop    = std::min(carveTop,    pTop);

        // LEFT remnant: from platform left edge to carve left edge, full height
        {
            float rLeft  = pLeft;
            float rRight = cLeft;
            float hw = (rRight - rLeft) / 2.0f;
            if (hw > MIN_HW) {
                Platform r;
                r.cx = rLeft + hw;
                r.cy = plat.cy;
                r.halfWidth = hw;
                r.halfHeight = plat.halfHeight;
                r.type = type;
                r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }

        // RIGHT remnant: from carve right edge to platform right edge, full height
        {
            float rLeft  = cRight;
            float rRight = pRight;
            float hw = (rRight - rLeft) / 2.0f;
            if (hw > MIN_HW) {
                Platform r;
                r.cx = rLeft + hw;
                r.cy = plat.cy;
                r.halfWidth = hw;
                r.halfHeight = plat.halfHeight;
                r.type = type;
                r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }

        // BOTTOM remnant: within the carve X range, from platform bottom to carve bottom
        {
            float rLeft   = cLeft;
            float rRight  = cRight;
            float rBottom = pBottom;
            float rTop    = cBottom;
            float hw = (rRight - rLeft) / 2.0f;
            float hh = (rTop - rBottom) / 2.0f;
            if (hw > MIN_HW && hh > MIN_HH) {
                Platform r;
                r.cx = rLeft + hw;
                r.cy = rBottom + hh;
                r.halfWidth = hw;
                r.halfHeight = hh;
                r.type = type;
                r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }

        // TOP remnant: within the carve X range, from carve top to platform top
        {
            float rLeft   = cLeft;
            float rRight  = cRight;
            float rBottom = cTop;
            float rTop    = pTop;
            float hw = (rRight - rLeft) / 2.0f;
            float hh = (rTop - rBottom) / 2.0f;
            if (hw > MIN_HW && hh > MIN_HH) {
                Platform r;
                r.cx = rLeft + hw;
                r.cy = rBottom + hh;
                r.halfWidth = hw;
                r.halfHeight = hh;
                r.type = type;
                r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }
    }

    // Clean up dead, add remnants
    if (affected > 0) {
        m_platforms.erase(
            std::remove_if(m_platforms.begin(), m_platforms.end(),
                            [](const Platform& p) { return !p.alive; }),
            m_platforms.end());
        for (auto& np : newPlatforms) m_platforms.push_back(np);
    }

    return affected;
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

        rect.setFillColor(fillColorForType(p.type));
        rect.setOutlineColor(outlineColorForType(p.type));
        rect.setOutlineThickness(1.0f);
        target.draw(rect);

        // Texture details
        sf::Vector2f tl = toScreen(p.cx - p.halfWidth, p.cy + p.halfHeight);
        if (p.type == PlatformType::Brick && w > 10.0f && h > 6.0f) {
            for (float by = 6.0f; by < h; by += 6.0f) {
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0] = sf::Vertex{{tl.x + 1.0f, tl.y + by}, sf::Color(100, 35, 30, 80)};
                line[1] = sf::Vertex{{tl.x + w - 1.0f, tl.y + by}, sf::Color(100, 35, 30, 80)};
                target.draw(line);
            }
        } else if (p.type == PlatformType::Wood && w > 8.0f) {
            for (float wy = 4.0f; wy < h; wy += 5.0f) {
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0] = sf::Vertex{{tl.x + 2.0f, tl.y + wy}, sf::Color(80, 55, 25, 60)};
                line[1] = sf::Vertex{{tl.x + w - 2.0f, tl.y + wy}, sf::Color(80, 55, 25, 60)};
                target.draw(line);
            }
        } else if (p.type == PlatformType::Metal && w > 8.0f) {
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

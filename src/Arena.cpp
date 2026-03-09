#include "Arena.h"
#include "StickFigure.h"
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
        case PlatformType::Ice:    return sf::Color(170, 210, 240);
        case PlatformType::Lava:   return sf::Color(200, 60, 20);
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
        case PlatformType::Ice:    return sf::Color(130, 180, 220);
        case PlatformType::Lava:   return sf::Color(255, 100, 30);
        default: return sf::Color(150, 150, 150);
    }
}

// ============================================================
// LEVEL MANAGEMENT
// ============================================================

int Arena::getLevelCount() { return 15; }

std::string Arena::getLevelName(int index) {
    switch (index) {
        case 0: return "Classic";
        case 1: return "Village";
        case 2: return "Fortress";
        case 3: return "Skyscrapers";
        case 4: return "Volcano";
        case 5: return "Ice Cavern";
        case 6: return "Sky Temple";
        case 7: return "Death Pit";
        case 8: return "Treehouse";
        case 9: return "Abyss";
        case 10: return "Canyon";
        case 11: return "Shipwreck";
        case 12: return "Underground";
        case 13: return "Towers";
        case 14: return "Random";
        default: return "???";
    }
}

// ── ADD HELPERS ──────────────────────────────────────────────

void Arena::addPlatform(Physics& physics, float cx, float cy, float hw, float hh, PlatformType type) {
    Platform p;
    p.cx = cx; p.cy = cy; p.halfWidth = hw; p.halfHeight = hh;
    p.bodyId = physics.createStaticBox(cx, cy, hw, hh, CAT_PLATFORM);
    p.alive = true;
    p.type = type;
    m_platforms.push_back(p);
}

void Arena::addSpike(float cx, float cy, float hw, bool pointsUp, float dmg) {
    Spike s;
    s.cx = cx; s.cy = cy; s.halfWidth = hw;
    s.pointsUp = pointsUp; s.damage = dmg;
    m_spikes.push_back(s);
}

void Arena::addMovingPlatform(Physics& physics, float sx, float sy, float ex, float ey,
                               float hw, float hh, float speed, PlatformType type) {
    MovingPlatform mp;
    mp.startX = sx; mp.startY = sy;
    mp.endX = ex;   mp.endY = ey;
    mp.cx = sx; mp.cy = sy;
    mp.halfWidth = hw; mp.halfHeight = hh;
    mp.speed = speed; mp.type = type;
    mp.bodyId = physics.createStaticBox(sx, sy, hw, hh, CAT_PLATFORM);
    m_movers.push_back(mp);
}

void Arena::addSwingingPlatform(Physics& physics, float pivX, float pivY, float ropeLen,
                                 float hw, float hh, float maxAng, PlatformType type) {
    SwingingPlatform sp;
    sp.pivotX = pivX; sp.pivotY = pivY;
    sp.ropeLength = ropeLen;
    sp.halfWidth = hw; sp.halfHeight = hh;
    sp.maxAngle = maxAng; sp.type = type;
    sp.angle = maxAng * 0.9f;
    sp.angularVel = 0.0f;
    float cx = pivX + std::sin(sp.angle) * ropeLen;
    float cy = pivY - std::cos(sp.angle) * ropeLen;
    sp.bodyId = physics.createStaticBox(cx, cy, hw, hh, CAT_PLATFORM);
    m_swingers.push_back(sp);
}

void Arena::addCrusher(Physics& physics, float cx, float topY, float botY,
                        float hw, float hh, float speed, float pause) {
    Crusher c;
    c.cx = cx; c.topY = topY; c.botY = botY;
    c.halfWidth = hw; c.halfHeight = hh;
    c.cy = topY; c.speed = speed;
    c.pauseDuration = pause;
    c.pauseTimer = pause * 0.5f;
    c.bodyId = physics.createStaticBox(cx, topY, hw, hh, CAT_PLATFORM);
    m_crushers.push_back(c);
}

// ── CREATE LEVEL ─────────────────────────────────────────────

void Arena::createLevel(Physics& physics, int levelIndex) {
    m_physics = &physics;

    // Destroy old physics bodies before rebuilding
    for (auto& p : m_platforms) {
        if (b2Body_IsValid(p.bodyId)) b2DestroyBody(p.bodyId);
    }
    for (auto& mp : m_movers) {
        if (b2Body_IsValid(mp.bodyId)) b2DestroyBody(mp.bodyId);
    }
    for (auto& sp : m_swingers) {
        if (b2Body_IsValid(sp.bodyId)) b2DestroyBody(sp.bodyId);
    }
    for (auto& c : m_crushers) {
        if (b2Body_IsValid(c.bodyId)) b2DestroyBody(c.bodyId);
    }

    m_platforms.clear();
    m_spawnPoints.clear();
    m_spikes.clear();
    m_movers.clear();
    m_swingers.clear();
    m_crushers.clear();
    m_currentLevel = levelIndex;

    int actualLevel = levelIndex;
    if (levelIndex == 14) {
        // Random: pick a random arena each round
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 13);
        actualLevel = dist(rng);
    }
    m_currentLevel = levelIndex;  // store original selection (14 = Random)

    switch (actualLevel) {
        case 0: buildClassic(physics); break;
        case 1: buildVillage(physics); break;
        case 2: buildFortress(physics); break;
        case 3: buildSkyscrapers(physics); break;
        case 4: buildVolcano(physics); break;
        case 5: buildIceCavern(physics); break;
        case 6: buildSkyTemple(physics); break;
        case 7: buildDeathPit(physics); break;
        case 8: buildTreehouse(physics); break;
        case 9: buildAbyss(physics); break;
        case 10: buildCanyon(physics); break;
        case 11: buildShipwreck(physics); break;
        case 12: buildUnderground(physics); break;
        case 13: buildTowers(physics); break;
        default: buildClassic(physics); break;
    }

    std::cout << "[Arena] Built level: " << getLevelName(actualLevel)
              << (levelIndex == 14 ? " (Random)" : "")
              << " (" << m_platforms.size() << " platforms, "
              << m_spikes.size() << " spikes, "
              << m_movers.size() << " movers, "
              << m_swingers.size() << " swingers, "
              << m_crushers.size() << " crushers)\n";

    // Add invisible boundary walls far outside playable area
    // These prevent bounce projectiles from escaping while being off-camera
    constexpr float BOUNDARY = 60.0f; // far enough to never see
    constexpr float WALL_THICK = 2.0f;
    constexpr float WALL_HEIGHT = 120.0f;
    // Left wall
    physics.createStaticBox(-BOUNDARY, 0.0f, WALL_THICK, WALL_HEIGHT, CAT_PLATFORM);
    // Right wall
    physics.createStaticBox(BOUNDARY, 0.0f, WALL_THICK, WALL_HEIGHT, CAT_PLATFORM);
    // Ceiling (very high)
    physics.createStaticBox(0.0f, BOUNDARY, BOUNDARY, WALL_THICK, CAT_PLATFORM);
    // Floor (very deep)
    physics.createStaticBox(0.0f, -BOUNDARY, BOUNDARY, WALL_THICK, CAT_PLATFORM);
}

// ============================================================
// ORIGINAL 4 LEVEL LAYOUTS
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
    addPlatform(physics, -11.0f, -4.0f, 3.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -14.3f, -2.5f, 0.3f, 1.7f, PlatformType::Brick);
    addPlatform(physics, -7.7f, -2.5f, 0.3f, 1.7f, PlatformType::Brick);
    addPlatform(physics, -11.0f, -0.6f, 4.0f, 0.2f, PlatformType::Roof);
    addPlatform(physics, -12.0f, -2.8f, 1.2f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 0.0f, -3.5f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, -1.8f, -4.5f, 0.15f, 1.2f, PlatformType::Wood);
    addPlatform(physics, 1.8f, -4.5f, 0.15f, 1.2f, PlatformType::Wood);
    addPlatform(physics, 10.0f, -4.0f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 7.2f, -2.0f, 0.3f, 2.2f, PlatformType::Stone);
    addPlatform(physics, 12.8f, -2.0f, 0.3f, 2.2f, PlatformType::Stone);
    addPlatform(physics, 10.0f, -0.5f, 3.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 7.2f, 1.2f, 0.3f, 1.5f, PlatformType::Brick);
    addPlatform(physics, 12.8f, 1.2f, 0.3f, 1.5f, PlatformType::Brick);
    addPlatform(physics, 10.0f, 2.9f, 3.5f, 0.2f, PlatformType::Roof);
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
    addPlatform(physics, -12.0f, -4.0f, 2.5f, 0.3f, PlatformType::Stone);
    addPlatform(physics, -14.3f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, -9.7f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, -12.0f, 1.5f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -14.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
    addPlatform(physics, -9.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
    addPlatform(physics, 12.0f, -4.0f, 2.5f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 14.3f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, 9.7f, -1.5f, 0.4f, 2.8f, PlatformType::Stone);
    addPlatform(physics, 12.0f, 1.5f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 14.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
    addPlatform(physics, 9.5f, 2.3f, 0.3f, 0.6f, PlatformType::Stone);
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
    addPlatform(physics, -13.0f, -2.0f, 3.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, -15.8f, -4.0f, 0.3f, 2.3f, PlatformType::Metal);
    addPlatform(physics, -10.2f, -4.0f, 0.3f, 2.3f, PlatformType::Metal);
    addPlatform(physics, -13.0f, -4.5f, 2.5f, 0.15f, PlatformType::Metal);
    addPlatform(physics, -5.0f, 2.0f, 2.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, -6.8f, -1.0f, 0.3f, 3.3f, PlatformType::Stone);
    addPlatform(physics, -3.2f, -1.0f, 0.3f, 3.3f, PlatformType::Stone);
    addPlatform(physics, -5.0f, -1.5f, 1.5f, 0.15f, PlatformType::Wood);
    addPlatform(physics, -5.0f, 0.5f, 1.5f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 4.0f, 4.0f, 2.5f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 1.7f, 0.5f, 0.3f, 3.8f, PlatformType::Stone);
    addPlatform(physics, 6.3f, 0.5f, 0.3f, 3.8f, PlatformType::Stone);
    addPlatform(physics, 4.0f, -1.0f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 4.0f, 1.0f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 4.0f, 3.0f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 13.0f, 0.0f, 2.5f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 10.7f, -3.0f, 0.3f, 3.3f, PlatformType::Brick);
    addPlatform(physics, 15.3f, -3.0f, 0.3f, 3.3f, PlatformType::Brick);
    addPlatform(physics, 13.0f, -2.5f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 13.0f, -0.5f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, -9.0f, -1.0f, 1.2f, 0.12f, PlatformType::Wood);
    addPlatform(physics, -0.5f, 1.5f, 1.5f, 0.12f, PlatformType::Wood);
    addPlatform(physics, 9.0f, 1.0f, 1.5f, 0.12f, PlatformType::Wood);
    m_spawnPoints = {
        {-13.0f, -1.0f}, {13.0f, 1.0f},
        {-5.0f, -0.5f}, {4.0f, -0.0f}, {4.0f, 4.8f},
    };
}

// ============================================================
// NEW BIGGER MAPS
// ============================================================

void Arena::buildVolcano(Physics& physics) {
    // Lava floor
    addPlatform(physics, 0.0f, -10.5f, 20.0f, 0.5f, PlatformType::Lava);
    // Rocky cliffs
    addPlatform(physics, -17.0f, -7.0f, 3.0f, 3.0f, PlatformType::Stone);
    addPlatform(physics, -14.0f, -6.0f, 1.5f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 17.0f, -7.0f, 3.0f, 3.0f, PlatformType::Stone);
    addPlatform(physics, 14.0f, -6.0f, 1.5f, 0.3f, PlatformType::Stone);
    // Pillars from lava
    addPlatform(physics, -8.0f, -7.0f, 1.5f, 3.0f, PlatformType::Stone);
    addPlatform(physics, 0.0f, -6.0f, 2.0f, 4.0f, PlatformType::Stone);
    addPlatform(physics, 8.0f, -7.0f, 1.5f, 3.0f, PlatformType::Stone);
    // Upper platforms
    addPlatform(physics, -11.0f, -1.0f, 2.5f, 0.25f, PlatformType::Stone);
    addPlatform(physics, -4.0f, 1.5f, 2.0f, 0.25f, PlatformType::Stone);
    addPlatform(physics, 4.0f, 1.5f, 2.0f, 0.25f, PlatformType::Stone);
    addPlatform(physics, 11.0f, -1.0f, 2.5f, 0.25f, PlatformType::Stone);
    addPlatform(physics, 0.0f, 5.0f, 3.0f, 0.25f, PlatformType::Metal);
    addPlatform(physics, -7.0f, 4.0f, 1.0f, 0.2f, PlatformType::Brick);
    addPlatform(physics, 7.0f, 4.0f, 1.0f, 0.2f, PlatformType::Brick);
    // Spikes
    addSpike(-8.0f, -3.8f, 1.3f, true, 25.0f);
    addSpike(8.0f, -3.8f, 1.3f, true, 25.0f);
    // Moving platforms over lava
    addMovingPlatform(physics, -12.0f, -3.5f, -9.5f, -3.5f, 1.2f, 0.15f, 2.5f);
    addMovingPlatform(physics, -5.5f, -2.5f, -2.5f, -2.5f, 1.0f, 0.15f, 2.0f);
    addMovingPlatform(physics, 2.5f, -2.5f, 5.5f, -2.5f, 1.0f, 0.15f, 2.0f);
    addMovingPlatform(physics, 9.5f, -3.5f, 12.0f, -3.5f, 1.2f, 0.15f, 2.5f);
    addMovingPlatform(physics, 0.0f, -1.5f, 0.0f, 3.5f, 1.5f, 0.15f, 1.8f);
    m_spawnPoints = {
        {-17.0f, -3.0f}, {17.0f, -3.0f},
        {-8.0f, -3.0f}, {8.0f, -3.0f}, {0.0f, -1.0f},
    };
}

void Arena::buildIceCavern(Physics& physics) {
    // Floor and walls
    addPlatform(physics, 0.0f, -9.0f, 20.0f, 0.5f, PlatformType::Ice);
    addPlatform(physics, 0.0f, 10.0f, 20.0f, 0.5f, PlatformType::Stone);
    addPlatform(physics, -20.0f, 0.0f, 0.5f, 10.0f, PlatformType::Stone);
    addPlatform(physics, 20.0f, 0.0f, 0.5f, 10.0f, PlatformType::Stone);
    // Ice ledges
    addPlatform(physics, -15.0f, -6.0f, 3.0f, 0.2f, PlatformType::Ice);
    addPlatform(physics, -10.0f, -3.5f, 2.5f, 0.2f, PlatformType::Ice);
    addPlatform(physics, -5.0f, -1.0f, 2.0f, 0.2f, PlatformType::Ice);
    addPlatform(physics, 5.0f, -1.0f, 2.0f, 0.2f, PlatformType::Ice);
    addPlatform(physics, 10.0f, -3.5f, 2.5f, 0.2f, PlatformType::Ice);
    addPlatform(physics, 15.0f, -6.0f, 3.0f, 0.2f, PlatformType::Ice);
    addPlatform(physics, 0.0f, 1.5f, 4.0f, 0.2f, PlatformType::Ice);
    addPlatform(physics, -12.0f, 3.0f, 2.0f, 0.2f, PlatformType::Ice);
    addPlatform(physics, 12.0f, 3.0f, 2.0f, 0.2f, PlatformType::Ice);
    addPlatform(physics, -6.0f, 5.5f, 1.5f, 0.2f, PlatformType::Ice);
    addPlatform(physics, 6.0f, 5.5f, 1.5f, 0.2f, PlatformType::Ice);
    addPlatform(physics, 0.0f, 7.5f, 2.0f, 0.2f, PlatformType::Metal);
    addPlatform(physics, -8.0f, -6.5f, 0.4f, 2.5f, PlatformType::Ice);
    addPlatform(physics, 8.0f, -6.5f, 0.4f, 2.5f, PlatformType::Ice);
    // Stalactites
    addSpike(-14.0f, 9.2f, 1.5f, false, 35.0f);
    addSpike(-6.0f, 9.2f, 1.0f, false, 35.0f);
    addSpike(2.0f, 9.2f, 1.2f, false, 35.0f);
    addSpike(10.0f, 9.2f, 1.5f, false, 35.0f);
    addSpike(-3.0f, -8.3f, 1.0f, true, 30.0f);
    addSpike(3.0f, -8.3f, 1.0f, true, 30.0f);
    // Swinging platforms
    addSwingingPlatform(physics, -4.0f, 9.5f, 5.0f, 1.5f, 0.15f, 1.1f);
    addSwingingPlatform(physics, 4.0f, 9.5f, 5.0f, 1.5f, 0.15f, 1.1f);
    addMovingPlatform(physics, -16.0f, -5.0f, 16.0f, -5.0f, 1.5f, 0.15f, 3.0f);
    addCrusher(physics, 0.0f, 8.0f, 2.5f, 1.5f, 0.5f, 12.0f, 2.5f);
    m_spawnPoints = {
        {-15.0f, -5.0f}, {15.0f, -5.0f},
        {-10.0f, -2.5f}, {10.0f, -2.5f}, {0.0f, 2.5f},
    };
}

void Arena::buildSkyTemple(Physics& physics) {
    // Central temple
    addPlatform(physics, 0.0f, -2.0f, 5.0f, 0.3f, PlatformType::Stone);
    addPlatform(physics, -4.8f, 0.5f, 0.3f, 2.5f, PlatformType::Stone);
    addPlatform(physics, 4.8f, 0.5f, 0.3f, 2.5f, PlatformType::Stone);
    addPlatform(physics, 0.0f, 3.2f, 5.5f, 0.25f, PlatformType::Roof);
    addPlatform(physics, -2.0f, 0.5f, 1.5f, 0.15f, PlatformType::Stone);
    addPlatform(physics, 2.0f, 0.5f, 1.5f, 0.15f, PlatformType::Stone);
    // Left floating islands
    addPlatform(physics, -10.0f, -4.0f, 2.5f, 0.25f, PlatformType::Stone);
    addPlatform(physics, -15.0f, -1.5f, 2.0f, 0.25f, PlatformType::Stone);
    addPlatform(physics, -12.0f, 2.0f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -18.0f, 1.0f, 1.5f, 0.2f, PlatformType::Stone);
    // Right floating islands
    addPlatform(physics, 10.0f, -4.0f, 2.5f, 0.25f, PlatformType::Stone);
    addPlatform(physics, 15.0f, -1.5f, 2.0f, 0.25f, PlatformType::Stone);
    addPlatform(physics, 12.0f, 2.0f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 18.0f, 1.0f, 1.5f, 0.2f, PlatformType::Stone);
    // High altars
    addPlatform(physics, -7.0f, 6.0f, 1.5f, 0.2f, PlatformType::Metal);
    addPlatform(physics, 7.0f, 6.0f, 1.5f, 0.2f, PlatformType::Metal);
    addPlatform(physics, 0.0f, 8.0f, 2.0f, 0.2f, PlatformType::Metal);
    // Swinging bridges
    addSwingingPlatform(physics, -7.0f, 8.0f, 6.0f, 2.0f, 0.15f, 1.2f);
    addSwingingPlatform(physics, 7.0f, 8.0f, 6.0f, 2.0f, 0.15f, 1.2f);
    addSwingingPlatform(physics, 0.0f, 11.0f, 7.0f, 1.5f, 0.15f, 1.0f, PlatformType::Metal);
    // Moving platforms
    addMovingPlatform(physics, -7.0f, -3.0f, -3.0f, -3.0f, 1.2f, 0.15f, 2.0f);
    addMovingPlatform(physics, 3.0f, -3.0f, 7.0f, -3.0f, 1.2f, 0.15f, 2.0f);
    addMovingPlatform(physics, -10.0f, -3.0f, -10.0f, 4.0f, 1.0f, 0.15f, 1.5f);
    addMovingPlatform(physics, 10.0f, -3.0f, 10.0f, 4.0f, 1.0f, 0.15f, 1.5f);
    addMovingPlatform(physics, -15.0f, -0.5f, -12.0f, 3.0f, 1.0f, 0.12f, 1.8f);
    addMovingPlatform(physics, 15.0f, -0.5f, 12.0f, 3.0f, 1.0f, 0.12f, 1.8f);
    m_spawnPoints = {
        {-10.0f, -3.0f}, {10.0f, -3.0f},
        {0.0f, -1.0f}, {-15.0f, -0.5f}, {15.0f, -0.5f},
    };
}

void Arena::buildDeathPit(Physics& physics) {
    // Narrow floor segments with spike pits
    addPlatform(physics, -16.0f, -8.0f, 3.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, -7.0f, -8.0f, 3.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 0.0f, -9.0f, 2.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 7.0f, -8.0f, 3.0f, 0.3f, PlatformType::Metal);
    addPlatform(physics, 16.0f, -8.0f, 3.0f, 0.3f, PlatformType::Metal);
    // Spike pits
    addSpike(-11.5f, -9.5f, 1.5f, true, 40.0f);
    addSpike(-3.5f, -9.5f, 1.5f, true, 40.0f);
    addSpike(3.5f, -9.5f, 1.5f, true, 40.0f);
    addSpike(11.5f, -9.5f, 1.5f, true, 40.0f);
    // Mid-level
    addPlatform(physics, -13.0f, -4.0f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -5.0f, -4.5f, 1.5f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 5.0f, -4.5f, 1.5f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 13.0f, -4.0f, 2.0f, 0.2f, PlatformType::Stone);
    // Central arena
    addPlatform(physics, 0.0f, -2.0f, 3.0f, 0.2f, PlatformType::Metal);
    addPlatform(physics, -2.8f, -0.5f, 0.2f, 1.5f, PlatformType::Metal);
    addPlatform(physics, 2.8f, -0.5f, 0.2f, 1.5f, PlatformType::Metal);
    // Upper
    addPlatform(physics, -10.0f, 0.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 10.0f, 0.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -6.0f, 3.0f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 6.0f, 3.0f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 0.0f, 5.0f, 2.5f, 0.2f, PlatformType::Metal);
    // Ceiling + hanging spikes
    addPlatform(physics, 0.0f, 9.5f, 20.0f, 0.5f, PlatformType::Stone);
    addSpike(-15.0f, 8.7f, 2.0f, false, 35.0f);
    addSpike(-8.0f, 8.7f, 1.5f, false, 35.0f);
    addSpike(0.0f, 8.7f, 2.0f, false, 35.0f);
    addSpike(8.0f, 8.7f, 1.5f, false, 35.0f);
    addSpike(15.0f, 8.7f, 2.0f, false, 35.0f);
    // Crushers
    addCrusher(physics, -8.0f, 7.0f, -3.5f, 1.2f, 0.6f, 10.0f, 2.0f);
    addCrusher(physics, 8.0f, 7.0f, -3.5f, 1.2f, 0.6f, 10.0f, 2.0f);
    // Swinging platforms
    addSwingingPlatform(physics, -13.0f, 8.0f, 5.0f, 1.0f, 0.12f, 1.3f, PlatformType::Metal);
    addSwingingPlatform(physics, 13.0f, 8.0f, 5.0f, 1.0f, 0.12f, 1.3f, PlatformType::Metal);
    // Moving platforms
    addMovingPlatform(physics, -16.0f, -5.5f, -10.0f, -5.5f, 1.0f, 0.12f, 3.0f);
    addMovingPlatform(physics, 10.0f, -5.5f, 16.0f, -5.5f, 1.0f, 0.12f, 3.0f);
    addMovingPlatform(physics, -5.0f, -6.5f, 5.0f, -6.5f, 1.2f, 0.12f, 2.5f);
    addMovingPlatform(physics, -16.0f, -7.0f, -16.0f, 2.0f, 1.0f, 0.12f, 2.0f);
    addMovingPlatform(physics, 16.0f, -7.0f, 16.0f, 2.0f, 1.0f, 0.12f, 2.0f);
    m_spawnPoints = {
        {-16.0f, -7.0f}, {16.0f, -7.0f},
        {-7.0f, -7.0f}, {7.0f, -7.0f}, {0.0f, -1.0f},
    };
}

// ── TREEHOUSE ─────────────────────────────────────────────
void Arena::buildTreehouse(Physics& physics) {
    // Giant tree trunk in center
    addPlatform(physics, 0.0f, -2.0f, 1.0f, 8.0f, PlatformType::Wood);
    // Root platforms (ground level)
    addPlatform(physics, -8.0f, -9.0f, 6.0f, 0.4f, PlatformType::Ground);
    addPlatform(physics, 8.0f, -9.0f, 6.0f, 0.4f, PlatformType::Ground);
    // Branch platforms going up - left side
    addPlatform(physics, -5.0f, -5.0f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -8.0f, -2.5f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -4.0f, 0.5f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -10.0f, 2.5f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -5.5f, 5.0f, 2.0f, 0.2f, PlatformType::Wood);
    // Branch platforms - right side
    addPlatform(physics, 5.0f, -4.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 9.0f, -1.5f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 4.5f, 1.5f, 1.8f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 8.0f, 4.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 3.0f, 6.5f, 2.5f, 0.2f, PlatformType::Wood);
    // Canopy (top platforms)
    addPlatform(physics, 0.0f, 8.0f, 4.0f, 0.3f, PlatformType::Roof);
    addPlatform(physics, -7.0f, 7.5f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 7.0f, 7.5f, 2.0f, 0.2f, PlatformType::Wood);
    // Swing vines
    addSwingingPlatform(physics, -12.0f, 8.0f, 5.0f, 1.2f, 0.1f, 0.8f, PlatformType::Wood);
    addSwingingPlatform(physics, 12.0f, 8.0f, 5.0f, 1.2f, 0.1f, 0.8f, PlatformType::Wood);
    addSwingingPlatform(physics, 0.0f, 12.0f, 4.0f, 1.0f, 0.1f, 1.0f, PlatformType::Wood);
    m_spawnPoints = {
        {-8.0f, -8.0f}, {8.0f, -8.0f},
        {-5.0f, -4.0f}, {5.0f, -3.0f}, {0.0f, 9.0f},
    };
}

// ── ABYSS (tall vertical map) ─────────────────────────────
void Arena::buildAbyss(Physics& physics) {
    // Narrow vertical shaft with platforms going way down
    // Left and right walls
    addPlatform(physics, -10.0f, 0.0f, 0.5f, 20.0f, PlatformType::Stone);
    addPlatform(physics, 10.0f, 0.0f, 0.5f, 20.0f, PlatformType::Stone);
    // Staggered platforms descending
    addPlatform(physics, -4.0f, 8.0f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 5.0f, 5.5f, 2.5f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -3.0f, 3.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 4.0f, 0.5f, 2.5f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -5.0f, -2.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 3.0f, -4.5f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -4.0f, -7.0f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 5.0f, -9.5f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 0.0f, -12.0f, 3.0f, 0.2f, PlatformType::Metal);
    addPlatform(physics, -5.0f, -14.5f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 4.0f, -17.0f, 2.5f, 0.2f, PlatformType::Stone);
    // Floor at the very bottom — lava zone visually
    addPlatform(physics, 0.0f, -20.0f, 10.0f, 0.5f, PlatformType::Metal);
    addSpike(0.0f, -19.2f, 8.0f, true, 50.0f);
    // Top opening
    addPlatform(physics, 0.0f, 11.0f, 3.0f, 0.3f, PlatformType::Stone);
    // Moving platforms in the shaft
    addMovingPlatform(physics, -7.0f, -5.0f, 7.0f, -5.0f, 1.5f, 0.12f, 2.0f, PlatformType::Metal);
    addMovingPlatform(physics, 7.0f, -10.0f, -7.0f, -10.0f, 1.5f, 0.12f, 2.5f, PlatformType::Metal);
    addMovingPlatform(physics, -7.0f, -15.0f, 7.0f, -15.0f, 1.2f, 0.12f, 3.0f, PlatformType::Metal);
    m_spawnPoints = {
        {-4.0f, 9.0f}, {5.0f, 6.5f},
        {-3.0f, 4.0f}, {4.0f, 1.5f}, {0.0f, 12.0f},
    };
}

// ── CANYON (wide scrolling map) ───────────────────────────
void Arena::buildCanyon(Physics& physics) {
    // Wide map that requires camera scrolling — 3x normal width
    // Ground with gaps
    addPlatform(physics, -30.0f, -8.0f, 8.0f, 0.5f, PlatformType::Ground);
    addPlatform(physics, -15.0f, -9.0f, 5.0f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 0.0f, -8.0f, 6.0f, 0.5f, PlatformType::Ground);
    addPlatform(physics, 15.0f, -9.0f, 5.0f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 30.0f, -8.0f, 8.0f, 0.5f, PlatformType::Ground);
    // Canyon walls
    addPlatform(physics, -20.0f, -5.0f, 0.5f, 3.0f, PlatformType::Stone);
    addPlatform(physics, -8.0f, -4.0f, 0.5f, 4.0f, PlatformType::Stone);
    addPlatform(physics, 8.0f, -4.0f, 0.5f, 4.0f, PlatformType::Stone);
    addPlatform(physics, 20.0f, -5.0f, 0.5f, 3.0f, PlatformType::Stone);
    // Bridges and upper areas
    addPlatform(physics, -24.0f, -4.0f, 3.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -14.0f, -4.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 0.0f, -3.0f, 4.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 14.0f, -4.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 24.0f, -4.0f, 3.0f, 0.2f, PlatformType::Wood);
    // High lookout points
    addPlatform(physics, -32.0f, -3.0f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -28.0f, -0.5f, 1.5f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -22.0f, 1.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 22.0f, 1.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 28.0f, -0.5f, 1.5f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 32.0f, -3.0f, 2.0f, 0.2f, PlatformType::Stone);
    // Central tower
    addPlatform(physics, 0.0f, 0.0f, 1.5f, 0.2f, PlatformType::Metal);
    addPlatform(physics, 0.0f, 3.0f, 2.0f, 0.2f, PlatformType::Metal);
    addPlatform(physics, -1.3f, 1.0f, 0.2f, 1.3f, PlatformType::Metal);
    addPlatform(physics, 1.3f, 1.0f, 0.2f, 1.3f, PlatformType::Metal);
    // Spike pits between ground segments
    addSpike(-22.5f, -10.0f, 2.0f, true, 35.0f);
    addSpike(-7.5f, -10.0f, 1.5f, true, 35.0f);
    addSpike(7.5f, -10.0f, 1.5f, true, 35.0f);
    addSpike(22.5f, -10.0f, 2.0f, true, 35.0f);
    // Moving platforms crossing gaps
    addMovingPlatform(physics, -20.0f, -6.5f, -8.0f, -6.5f, 1.5f, 0.12f, 3.0f, PlatformType::Metal);
    addMovingPlatform(physics, 8.0f, -6.5f, 20.0f, -6.5f, 1.5f, 0.12f, 3.0f, PlatformType::Metal);
    // Swinging bridge
    addSwingingPlatform(physics, -14.0f, 4.0f, 6.0f, 2.0f, 0.12f, 0.6f, PlatformType::Wood);
    addSwingingPlatform(physics, 14.0f, 4.0f, 6.0f, 2.0f, 0.12f, 0.6f, PlatformType::Wood);
    m_spawnPoints = {
        {-30.0f, -7.0f}, {30.0f, -7.0f},
        {-15.0f, -8.0f}, {15.0f, -8.0f}, {0.0f, -7.0f},
    };
}

// ── SHIPWRECK ─────────────────────────────────────────────
void Arena::buildShipwreck(Physics& physics) {
    // Tilted ship hull
    addPlatform(physics, -3.0f, -7.5f, 12.0f, 0.4f, PlatformType::Wood);
    // Ship deck structures
    addPlatform(physics, -12.0f, -6.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -12.0f, -4.5f, 0.2f, 1.5f, PlatformType::Wood);
    addPlatform(physics, 7.0f, -5.5f, 1.5f, 0.2f, PlatformType::Wood);
    // Masts
    addPlatform(physics, -5.0f, -2.0f, 0.15f, 5.0f, PlatformType::Wood);
    addPlatform(physics, 3.0f, -1.5f, 0.15f, 5.5f, PlatformType::Wood);
    // Crow's nests
    addPlatform(physics, -5.0f, 3.5f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 3.0f, 4.5f, 1.8f, 0.2f, PlatformType::Wood);
    // Rigging platforms between masts
    addPlatform(physics, -1.0f, 1.0f, 2.0f, 0.15f, PlatformType::Wood);
    addPlatform(physics, -1.0f, -1.5f, 1.5f, 0.15f, PlatformType::Wood);
    // Floating debris around ship
    addPlatform(physics, -18.0f, -8.5f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 14.0f, -8.0f, 2.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 18.0f, -7.0f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, -16.0f, -5.0f, 1.0f, 0.2f, PlatformType::Wood);
    // Water line — spikes below
    addSpike(-18.0f, -10.0f, 18.0f, true, 20.0f);
    // Swinging anchor
    addSwingingPlatform(physics, -10.0f, -3.0f, 4.0f, 0.8f, 0.15f, 1.2f, PlatformType::Metal);
    // Floating barrel platforms
    addMovingPlatform(physics, -18.0f, -8.5f, -14.0f, -7.0f, 1.0f, 0.15f, 1.5f, PlatformType::Wood);
    addMovingPlatform(physics, 12.0f, -8.5f, 18.0f, -7.0f, 1.2f, 0.15f, 2.0f, PlatformType::Wood);
    m_spawnPoints = {
        {-10.0f, -6.5f}, {5.0f, -6.5f},
        {-5.0f, 4.5f}, {3.0f, 5.5f}, {14.0f, -7.0f},
    };
}

// ── UNDERGROUND (caverns with stalactites) ────────────────
void Arena::buildUnderground(Physics& physics) {
    // Ceiling
    addPlatform(physics, 0.0f, 10.0f, 20.0f, 0.5f, PlatformType::Stone);
    // Stalactites (ceiling spikes)
    addSpike(-12.0f, 9.2f, 1.5f, false, 30.0f);
    addSpike(-4.0f, 9.2f, 1.0f, false, 30.0f);
    addSpike(6.0f, 9.2f, 1.5f, false, 30.0f);
    addSpike(14.0f, 9.2f, 1.0f, false, 30.0f);
    // Uneven cave floor
    addPlatform(physics, -14.0f, -7.0f, 4.0f, 0.4f, PlatformType::Stone);
    addPlatform(physics, -6.0f, -8.0f, 3.0f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 2.0f, -7.5f, 4.0f, 0.4f, PlatformType::Stone);
    addPlatform(physics, 12.0f, -6.5f, 4.0f, 0.5f, PlatformType::Stone);
    // Raised rock shelves
    addPlatform(physics, -12.0f, -3.5f, 2.5f, 0.3f, PlatformType::Stone);
    addPlatform(physics, -6.0f, -1.5f, 1.5f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 0.0f, -4.0f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 7.0f, -2.5f, 2.0f, 0.3f, PlatformType::Stone);
    addPlatform(physics, 14.0f, -2.0f, 2.5f, 0.2f, PlatformType::Stone);
    // Upper cave paths
    addPlatform(physics, -9.0f, 1.5f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 0.0f, 2.5f, 3.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 10.0f, 1.0f, 2.0f, 0.2f, PlatformType::Stone);
    // Central column
    addPlatform(physics, -3.0f, -2.5f, 0.3f, 2.5f, PlatformType::Stone);
    addPlatform(physics, 5.0f, -0.5f, 0.3f, 2.0f, PlatformType::Stone);
    // Crystal bridges (moving platforms)
    addMovingPlatform(physics, -15.0f, -5.5f, -7.0f, -5.5f, 1.2f, 0.1f, 2.0f, PlatformType::Metal);
    addMovingPlatform(physics, 3.0f, -5.5f, 12.0f, -5.5f, 1.2f, 0.1f, 2.5f, PlatformType::Metal);
    // Dripping stalactite (crusher)
    addCrusher(physics, -8.0f, 8.0f, -2.0f, 0.8f, 0.5f, 8.0f, 3.0f);
    addCrusher(physics, 8.0f, 8.0f, -1.0f, 0.8f, 0.5f, 8.0f, 4.0f);
    // Swinging rock
    addSwingingPlatform(physics, 0.0f, 9.0f, 5.0f, 1.5f, 0.15f, 0.7f, PlatformType::Stone);
    m_spawnPoints = {
        {-14.0f, -6.0f}, {12.0f, -5.5f},
        {-6.0f, -7.0f}, {2.0f, -6.5f}, {0.0f, 3.5f},
    };
}

// ── TOWERS (tall narrow towers) ───────────────────────────
void Arena::buildTowers(Physics& physics) {
    // Ground
    addPlatform(physics, 0.0f, -9.0f, 20.0f, 0.5f, PlatformType::Ground);
    // Left tower
    addPlatform(physics, -12.0f, -6.0f, 2.0f, 0.2f, PlatformType::Brick);
    addPlatform(physics, -13.5f, -3.0f, 0.2f, 3.2f, PlatformType::Brick);
    addPlatform(physics, -10.5f, -3.0f, 0.2f, 3.2f, PlatformType::Brick);
    addPlatform(physics, -12.0f, 0.0f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, -13.5f, 3.0f, 0.2f, 2.8f, PlatformType::Brick);
    addPlatform(physics, -10.5f, 3.0f, 0.2f, 2.8f, PlatformType::Brick);
    addPlatform(physics, -12.0f, 6.0f, 2.5f, 0.2f, PlatformType::Roof);
    // Right tower
    addPlatform(physics, 12.0f, -6.0f, 2.0f, 0.2f, PlatformType::Brick);
    addPlatform(physics, 10.5f, -3.0f, 0.2f, 3.2f, PlatformType::Brick);
    addPlatform(physics, 13.5f, -3.0f, 0.2f, 3.2f, PlatformType::Brick);
    addPlatform(physics, 12.0f, 0.0f, 2.0f, 0.2f, PlatformType::Stone);
    addPlatform(physics, 10.5f, 3.0f, 0.2f, 2.8f, PlatformType::Brick);
    addPlatform(physics, 13.5f, 3.0f, 0.2f, 2.8f, PlatformType::Brick);
    addPlatform(physics, 12.0f, 6.0f, 2.5f, 0.2f, PlatformType::Roof);
    // Center bridge between towers
    addPlatform(physics, 0.0f, 3.0f, 5.0f, 0.2f, PlatformType::Wood);
    // Under-bridge platforms
    addPlatform(physics, -5.0f, -3.0f, 2.0f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 0.0f, -5.0f, 1.5f, 0.2f, PlatformType::Wood);
    addPlatform(physics, 5.0f, -3.0f, 2.0f, 0.2f, PlatformType::Wood);
    // Mid-level platforms inside towers
    addPlatform(physics, -12.0f, -3.0f, 1.2f, 0.15f, PlatformType::Wood);
    addPlatform(physics, 12.0f, -3.0f, 1.2f, 0.15f, PlatformType::Wood);
    // Swinging bridge
    addSwingingPlatform(physics, 0.0f, 8.0f, 5.0f, 2.5f, 0.12f, 0.5f, PlatformType::Wood);
    // Elevator platforms
    addMovingPlatform(physics, -8.0f, -8.0f, -8.0f, 4.0f, 1.0f, 0.12f, 2.0f, PlatformType::Metal);
    addMovingPlatform(physics, 8.0f, -8.0f, 8.0f, 4.0f, 1.0f, 0.12f, 2.0f, PlatformType::Metal);
    m_spawnPoints = {
        {-12.0f, -5.0f}, {12.0f, -5.0f},
        {0.0f, -4.0f}, {-5.0f, -8.0f}, {5.0f, -8.0f},
    };
}

// ============================================================
// RANDOM PLATFORM TOP
// ============================================================

b2Vec2 Arena::getRandomPlatformTop() const {
    std::vector<size_t> aliveIdx;
    for (size_t i = 0; i < m_platforms.size(); i++) {
        if (m_platforms[i].alive && m_platforms[i].halfWidth > 0.5f &&
            m_platforms[i].type != PlatformType::Lava)
            aliveIdx.push_back(i);
    }
    if (aliveIdx.empty()) return {0.0f, 0.0f};
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, aliveIdx.size() - 1);
    const auto& p = m_platforms[aliveIdx[dist(rng)]];
    std::uniform_real_distribution<float> xDist(-p.halfWidth * 0.8f, p.halfWidth * 0.8f);
    return {p.cx + xDist(rng), p.cy + p.halfHeight + 0.5f};
}

// ============================================================
// HAZARD + DYNAMIC PLATFORM UPDATES
// ============================================================

void Arena::update(float dt, std::vector<std::unique_ptr<StickFigure>>& players,
                   const std::vector<ArenaSlowZone>& slowZones,
                   const std::vector<ArenaExplosion>& explosions) {
    // ── SPIKE DAMAGE ─────────────────────────────────────────
    for (auto& spike : m_spikes) {
        spike.cooldown -= dt;
        if (spike.cooldown > 0.0f) continue;

        float spikeH = 0.4f;  // visual spike height
        float sLeft  = spike.cx - spike.halfWidth;
        float sRight = spike.cx + spike.halfWidth;
        float sBot, sTop;
        if (spike.pointsUp) {
            sBot = spike.cy;
            sTop = spike.cy + spikeH;
        } else {
            sBot = spike.cy - spikeH;
            sTop = spike.cy;
        }

        for (auto& player : players) {
            if (!player->isAlive()) continue;
            b2Vec2 pos = player->getPosition();
            // Rough AABB overlap (player ~1m tall, 0.4m wide)
            if (pos.x > sLeft - 0.3f && pos.x < sRight + 0.3f &&
                pos.y - 0.6f < sTop && pos.y + 0.6f > sBot) {
                float kbY = spike.pointsUp ? spike.knockback : -spike.knockback;
                player->takeDamage(spike.damage, 0.0f, kbY);
                spike.cooldown = 0.5f;  // half-second between hits
                break;
            }
        }
    }

    // ── LAVA DAMAGE ──────────────────────────────────────────
    for (const auto& plat : m_platforms) {
        if (!plat.alive || plat.type != PlatformType::Lava) continue;
        float lLeft  = plat.cx - plat.halfWidth;
        float lRight = plat.cx + plat.halfWidth;
        float lTop   = plat.cy + plat.halfHeight;

        for (auto& player : players) {
            if (!player->isAlive()) continue;
            b2Vec2 pos = player->getPosition();
            if (pos.x > lLeft && pos.x < lRight && pos.y < lTop + 0.5f) {
                player->takeDamage(50.0f * dt, 0.0f, 3.0f);  // constant burn + upward knockback
            }
        }
    }

    // ── MOVING PLATFORMS ─────────────────────────────────────
    for (auto& mp : m_movers) {
        float pathLen = std::sqrt(
            (mp.endX - mp.startX) * (mp.endX - mp.startX) +
            (mp.endY - mp.startY) * (mp.endY - mp.startY));
        if (pathLen < 0.01f) continue;

        // Check if in a time slow zone
        float timeFactor = 1.0f;
        for (const auto& sz : slowZones) {
            float dx = mp.cx - sz.x, dy = mp.cy - sz.y;
            if (std::sqrt(dx*dx + dy*dy) < sz.radius)
                timeFactor = std::min(timeFactor, sz.factor);
        }

        float tStep = (mp.speed * dt * timeFactor) / pathLen;
        if (mp.forward) {
            mp.t += tStep;
            if (mp.t >= 1.0f) { mp.t = 1.0f; mp.forward = false; }
        } else {
            mp.t -= tStep;
            if (mp.t <= 0.0f) { mp.t = 0.0f; mp.forward = true; }
        }

        float newX = mp.startX + (mp.endX - mp.startX) * mp.t;
        float newY = mp.startY + (mp.endY - mp.startY) * mp.t;

        // Apply explosion shove to path endpoints
        for (const auto& ex : explosions) {
            float dx = mp.cx - ex.x, dy = mp.cy - ex.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < ex.radius && dist > 0.1f) {
                float f = (1.0f - dist / ex.radius) * ex.force * 0.08f;
                float shoveX = (dx / dist) * f;
                float shoveY = (dy / dist) * f;
                mp.startX += shoveX; mp.startY += shoveY;
                mp.endX += shoveX; mp.endY += shoveY;
            }
        }

        // Calculate velocity for carrying players
        float velX = (newX - mp.cx) / std::max(dt, 0.001f);
        float velY = (newY - mp.cy) / std::max(dt, 0.001f);

        mp.cx = newX;
        mp.cy = newY;

        // Move the physics body
        b2Body_SetTransform(mp.bodyId, {newX, newY}, b2MakeRot(0.0f));
        b2Body_SetLinearVelocity(mp.bodyId, {velX, velY});
    }

    // ── SWINGING PLATFORMS ───────────────────────────────────
    for (auto& sp : m_swingers) {
        // Check if in a time slow zone
        float platX = sp.pivotX + std::sin(sp.angle) * sp.ropeLength;
        float platY = sp.pivotY - std::cos(sp.angle) * sp.ropeLength;
        float timeFactor = 1.0f;
        for (const auto& sz : slowZones) {
            float dx = platX - sz.x, dy = platY - sz.y;
            if (std::sqrt(dx*dx + dy*dy) < sz.radius)
                timeFactor = std::min(timeFactor, sz.factor);
        }

        // Apply explosion angular impulse
        for (const auto& ex : explosions) {
            float dx = platX - ex.x, dy = platY - ex.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < ex.radius && dist > 0.1f) {
                float f = (1.0f - dist / ex.radius) * ex.force;
                // Push tangentially: perpendicular to rope
                float cosA = std::cos(sp.angle), sinA = std::sin(sp.angle);
                float tangent = (dx * cosA + dy * sinA) / sp.ropeLength;
                sp.angularVel += tangent * f * 0.02f;
            }
        }

        // Simple pendulum: angular accel = -(g/L) * sin(angle)
        float gravity = 10.0f;
        float angAccel = -(gravity / sp.ropeLength) * std::sin(sp.angle);
        sp.angularVel += angAccel * dt * timeFactor;
        sp.angularVel *= 0.9999f;  // very low damping for big persistent swings
        sp.angularVel *= timeFactor;  // time slow dampens velocity
        sp.angle += sp.angularVel * dt * timeFactor;

        float newX = sp.pivotX + std::sin(sp.angle) * sp.ropeLength;
        float newY = sp.pivotY - std::cos(sp.angle) * sp.ropeLength;

        b2Body_SetTransform(sp.bodyId, {newX, newY}, b2MakeRot(sp.angle * 0.3f));
        float vx = sp.angularVel * sp.ropeLength * std::cos(sp.angle);
        float vy = sp.angularVel * sp.ropeLength * std::sin(sp.angle);
        b2Body_SetLinearVelocity(sp.bodyId, {vx, vy});
    }

    // ── CRUSHERS ─────────────────────────────────────────────
    for (auto& c : m_crushers) {
        if (c.waiting) {
            c.pauseTimer -= dt;
            if (c.pauseTimer <= 0.0f) {
                c.waiting = false;
                c.descending = !c.descending;
            }
        } else {
            if (c.descending) {
                c.cy -= c.speed * dt;
                if (c.cy <= c.botY) {
                    c.cy = c.botY;
                    c.waiting = true;
                    c.pauseTimer = 0.3f; // brief pause at bottom

                    // Damage players caught underneath
                    for (auto& player : players) {
                        if (!player->isAlive()) continue;
                        b2Vec2 pos = player->getPosition();
                        if (std::abs(pos.x - c.cx) < c.halfWidth + 0.3f &&
                            pos.y < c.cy + c.halfHeight + 0.8f &&
                            pos.y > c.cy - c.halfHeight - 0.3f) {
                            player->takeDamage(c.damage, 0.0f, -5.0f);
                        }
                    }
                }
            } else {
                c.cy += c.speed * 0.4f * dt;  // retract slower
                if (c.cy >= c.topY) {
                    c.cy = c.topY;
                    c.waiting = true;
                    c.pauseTimer = c.pauseDuration;
                }
            }
        }
        b2Body_SetTransform(c.bodyId, {c.cx, c.cy}, b2MakeRot(0.0f));
    }
}

// ============================================================
// WORMS-STYLE TERRAIN CARVING
// ============================================================

int Arena::carveCircle(Physics& physics, float ex, float ey, float radius, float raggedness) {
    if (radius < 0.05f) return 0;

    int affected = 0;
    std::vector<Platform> newPlatforms;

    float carveLeft   = ex - radius;
    float carveRight  = ex + radius;
    float carveBottom = ey - radius;
    float carveTop    = ey + radius;

    constexpr float MIN_HW = 0.15f;
    constexpr float MIN_HH = 0.08f;

    // Simple hash for ragged edges — deterministic per position
    auto edgeHash = [](float x, float y) -> float {
        int ix = static_cast<int>(x * 73.0f + 17.0f);
        int iy = static_cast<int>(y * 91.0f + 31.0f);
        int h = (ix * 2654435761u ^ iy * 2246822519u) & 0xFFFF;
        return static_cast<float>(h) / 65535.0f; // 0-1
    };

    for (auto& plat : m_platforms) {
        if (!plat.alive) continue;

        float pLeft   = plat.cx - plat.halfWidth;
        float pRight  = plat.cx + plat.halfWidth;
        float pBottom = plat.cy - plat.halfHeight;
        float pTop    = plat.cy + plat.halfHeight;

        // Quick AABB reject
        if (carveRight < pLeft || carveLeft > pRight ||
            carveTop < pBottom || carveBottom > pTop) continue;

        // True circular distance check — closest point on platform to explosion center
        float closestX = std::clamp(ex, pLeft, pRight);
        float closestY = std::clamp(ey, pBottom, pTop);
        float dx = ex - closestX;
        float dy = ey - closestY;
        float distSq = dx * dx + dy * dy;
        if (distSq >= radius * radius) continue;

        // Raggedness: platforms near the edge have a chance to survive
        if (raggedness > 0.0f) {
            float dist = std::sqrt(distSq);
            float edgeFactor = dist / radius; // 0 at center, 1 at edge
            // Only apply raggedness to outer portion (beyond 50%)
            if (edgeFactor > 0.5f) {
                float ragChance = (edgeFactor - 0.5f) * 2.0f * raggedness; // 0→raggedness
                float h = edgeHash(plat.cx, plat.cy);
                if (h < ragChance) continue; // skip this platform — it survives!
            }
        }

        affected++;
        b2DestroyBody(plat.bodyId);
        plat.alive = false;

        PlatformType type = plat.type;

        // Use circular clamp for carve bounds
        float cLeft   = std::max(carveLeft,   pLeft);
        float cRight  = std::min(carveRight,  pRight);
        float cBottom = std::max(carveBottom, pBottom);
        float cTop    = std::min(carveTop,    pTop);

        // LEFT remnant
        {
            float hw = (cLeft - pLeft) / 2.0f;
            if (hw > MIN_HW) {
                Platform r;
                r.cx = pLeft + hw; r.cy = plat.cy;
                r.halfWidth = hw; r.halfHeight = plat.halfHeight;
                r.type = type; r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }
        // RIGHT remnant
        {
            float hw = (pRight - cRight) / 2.0f;
            if (hw > MIN_HW) {
                Platform r;
                r.cx = cRight + hw; r.cy = plat.cy;
                r.halfWidth = hw; r.halfHeight = plat.halfHeight;
                r.type = type; r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }
        // BOTTOM remnant
        {
            float hw = (cRight - cLeft) / 2.0f;
            float hh = (cBottom - pBottom) / 2.0f;
            if (hw > MIN_HW && hh > MIN_HH) {
                Platform r;
                r.cx = cLeft + hw; r.cy = pBottom + hh;
                r.halfWidth = hw; r.halfHeight = hh;
                r.type = type; r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }
        // TOP remnant
        {
            float hw = (cRight - cLeft) / 2.0f;
            float hh = (pTop - cTop) / 2.0f;
            if (hw > MIN_HW && hh > MIN_HH) {
                Platform r;
                r.cx = cLeft + hw; r.cy = cTop + hh;
                r.halfWidth = hw; r.halfHeight = hh;
                r.type = type; r.alive = true;
                r.bodyId = physics.createStaticBox(r.cx, r.cy, r.halfWidth, r.halfHeight, CAT_PLATFORM);
                newPlatforms.push_back(r);
            }
        }
    }

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

    // ── STATIC PLATFORMS ─────────────────────────────────────
    for (const auto& p : m_platforms) {
        if (!p.alive) continue;

        float w = p.halfWidth * 2.0f * PPM;
        float h = p.halfHeight * 2.0f * PPM;
        sf::RectangleShape rect({w, h});
        rect.setPosition(toScreen(p.cx - p.halfWidth, p.cy + p.halfHeight));

        sf::Color fill = fillColorForType(p.type);
        // Lava glow animation
        if (p.type == PlatformType::Lava) {
            float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(clock()) * 0.003f);
            fill.r = static_cast<uint8_t>(180 + pulse * 75);
            fill.g = static_cast<uint8_t>(40 + pulse * 40);
        }

        rect.setFillColor(fill);
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

    // ── MOVING PLATFORMS ─────────────────────────────────────
    for (const auto& mp : m_movers) {
        float w = mp.halfWidth * 2.0f * PPM;
        float h = mp.halfHeight * 2.0f * PPM;
        sf::RectangleShape rect({w, h});
        rect.setPosition(toScreen(mp.cx - mp.halfWidth, mp.cy + mp.halfHeight));
        rect.setFillColor(fillColorForType(mp.type));
        rect.setOutlineColor(sf::Color(220, 220, 100, 180));
        rect.setOutlineThickness(1.5f);
        target.draw(rect);

        // Arrow indicators showing movement direction
        sf::Vector2f center = toScreen(mp.cx, mp.cy);
        float dirX = mp.endX - mp.startX;
        float dirY = mp.endY - mp.startY;
        float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len > 0.1f) {
            sf::CircleShape arrow(3.0f, 3); // triangle
            arrow.setOrigin({3.0f, 3.0f});
            arrow.setPosition(center);
            float angle = std::atan2(-dirY, dirX) * 57.2958f;
            if (!mp.forward) angle += 180.0f;
            arrow.setRotation(sf::degrees(angle - 90.0f));
            arrow.setFillColor(sf::Color(220, 220, 100, 150));
            target.draw(arrow);
        }
    }

    // ── SWINGING PLATFORMS ───────────────────────────────────
    for (const auto& sp : m_swingers) {
        float cx = sp.pivotX + std::sin(sp.angle) * sp.ropeLength;
        float cy = sp.pivotY - std::cos(sp.angle) * sp.ropeLength;

        // Rope/chain
        sf::Vector2f pivScreen = toScreen(sp.pivotX, sp.pivotY);
        sf::Vector2f platScreen = toScreen(cx, cy);
        float dx = platScreen.x - pivScreen.x;
        float dy = platScreen.y - pivScreen.y;
        float ropePixels = std::sqrt(dx * dx + dy * dy);

        sf::RectangleShape rope({ropePixels, 2.0f});
        rope.setOrigin({0.0f, 1.0f});
        rope.setPosition(pivScreen);
        rope.setRotation(sf::degrees(std::atan2(dy, dx) * 57.2958f));
        rope.setFillColor(sf::Color(160, 140, 100));
        target.draw(rope);

        // Pivot circle
        sf::CircleShape pivot(4.0f);
        pivot.setOrigin({4.0f, 4.0f});
        pivot.setPosition(pivScreen);
        pivot.setFillColor(sf::Color(100, 100, 100));
        target.draw(pivot);

        // Platform
        float w = sp.halfWidth * 2.0f * PPM;
        float h = sp.halfHeight * 2.0f * PPM;
        sf::RectangleShape rect({w, h});
        rect.setOrigin({w * 0.5f, h * 0.5f});
        rect.setPosition(platScreen);
        rect.setRotation(sf::degrees(sp.angle * 0.3f * 57.2958f));
        rect.setFillColor(fillColorForType(sp.type));
        rect.setOutlineColor(sf::Color(200, 180, 100, 200));
        rect.setOutlineThickness(1.5f);
        target.draw(rect);
    }

    // ── CRUSHERS ─────────────────────────────────────────────
    for (const auto& c : m_crushers) {
        float w = c.halfWidth * 2.0f * PPM;
        float h = c.halfHeight * 2.0f * PPM;
        sf::RectangleShape rect({w, h});
        rect.setPosition(toScreen(c.cx - c.halfWidth, c.cy + c.halfHeight));
        rect.setFillColor(sf::Color(80, 80, 90));
        rect.setOutlineColor(sf::Color(200, 50, 50, 220));
        rect.setOutlineThickness(2.0f);
        target.draw(rect);

        // Teeth on bottom
        float teethY = c.cy - c.halfHeight;
        int numTeeth = static_cast<int>(c.halfWidth * 2.0f / 0.4f);
        for (int i = 0; i < numTeeth; i++) {
            float tx = (c.cx - c.halfWidth) + 0.2f + i * 0.4f;
            sf::ConvexShape tooth(3);
            sf::Vector2f t0 = toScreen(tx - 0.15f, teethY);
            sf::Vector2f t1 = toScreen(tx + 0.15f, teethY);
            sf::Vector2f t2 = toScreen(tx, teethY - 0.25f);
            tooth.setPoint(0, t0);
            tooth.setPoint(1, t1);
            tooth.setPoint(2, t2);
            tooth.setFillColor(sf::Color(200, 50, 50));
            target.draw(tooth);
        }

        // Piston rod from top
        sf::Vector2f topScreen = toScreen(c.cx, c.topY + 2.0f);
        sf::Vector2f curScreen = toScreen(c.cx, c.cy + c.halfHeight);
        float rodDx = curScreen.x - topScreen.x;
        float rodDy = curScreen.y - topScreen.y;
        float rodLen = std::sqrt(rodDx * rodDx + rodDy * rodDy);
        sf::RectangleShape rod({rodLen, 4.0f});
        rod.setOrigin({0.0f, 2.0f});
        rod.setPosition(topScreen);
        rod.setRotation(sf::degrees(std::atan2(rodDy, rodDx) * 57.2958f));
        rod.setFillColor(sf::Color(100, 100, 110));
        target.draw(rod);
    }

    // ── SPIKES ───────────────────────────────────────────────
    for (const auto& spike : m_spikes) {
        float spikeH = 0.4f;
        int numSpikes = static_cast<int>(spike.halfWidth * 2.0f / 0.35f);
        if (numSpikes < 1) numSpikes = 1;

        for (int i = 0; i < numSpikes; i++) {
            float sx = (spike.cx - spike.halfWidth) + 0.175f + i * 0.35f;
            sf::ConvexShape tri(3);

            if (spike.pointsUp) {
                sf::Vector2f bl = toScreen(sx - 0.15f, spike.cy);
                sf::Vector2f br = toScreen(sx + 0.15f, spike.cy);
                sf::Vector2f tp = toScreen(sx, spike.cy + spikeH);
                tri.setPoint(0, bl);
                tri.setPoint(1, br);
                tri.setPoint(2, tp);
            } else {
                sf::Vector2f tl = toScreen(sx - 0.15f, spike.cy);
                sf::Vector2f tr = toScreen(sx + 0.15f, spike.cy);
                sf::Vector2f bt = toScreen(sx, spike.cy - spikeH);
                tri.setPoint(0, tl);
                tri.setPoint(1, tr);
                tri.setPoint(2, bt);
            }

            tri.setFillColor(sf::Color(180, 180, 190));
            tri.setOutlineColor(sf::Color(220, 50, 50, 200));
            tri.setOutlineThickness(1.0f);
            target.draw(tri);
        }
    }
}

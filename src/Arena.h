#pragma once
#include "Physics.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class StickFigure;

enum class PlatformType {
    Ground,     // dark grey
    Wood,       // brown
    Stone,      // grey
    Metal,      // steel blue
    Brick,      // reddish
    Roof,       // dark red
    Ice,        // light blue
    Lava,       // orange-red (damages on contact)
};

struct Platform {
    b2BodyId bodyId;
    float halfWidth;
    float halfHeight;
    float cx, cy;
    bool alive = true;
    PlatformType type = PlatformType::Ground;
};

// ── HAZARD TYPES ──────────────────────────────────────────────

struct Spike {
    float cx, cy;           // center position
    float halfWidth;        // spike row width
    bool pointsUp;          // true = points up, false = points down (ceiling)
    float damage = 30.0f;   // damage on contact
    float knockback = 8.0f;
    float cooldown = 0.0f;  // prevents multi-hit per frame
};

struct MovingPlatform {
    b2BodyId bodyId;
    float halfWidth, halfHeight;
    float cx, cy;           // current center
    float startX, startY;   // path start
    float endX, endY;       // path end
    float speed;            // meters/sec along path
    float t = 0.0f;         // 0..1 interpolation
    bool  forward = true;
    PlatformType type = PlatformType::Metal;
};

struct SwingingPlatform {
    b2BodyId bodyId;
    float halfWidth, halfHeight;
    float pivotX, pivotY;   // anchor point (ceiling)
    float ropeLength;       // distance from pivot to platform center
    float angle = 0.0f;     // current angle (radians, 0 = straight down)
    float angularVel = 0.0f;
    float maxAngle;         // max swing amplitude
    PlatformType type = PlatformType::Wood;
};

struct Crusher {
    float cx;               // x position
    float topY;             // retracted Y
    float botY;             // extended Y (crush position)
    float halfWidth;
    float halfHeight;
    float cy;               // current Y
    float speed;
    float pauseTimer = 0.0f;
    float pauseDuration = 1.5f;
    bool  descending = false;
    bool  waiting = true;
    float damage = 40.0f;
    b2BodyId bodyId;
};

struct ArenaSlowZone {
    float x, y, radius, factor;
};
struct ArenaExplosion {
    float x, y, radius, force;
};

class Arena {
public:
    void createLevel(Physics& physics, int levelIndex);
    void update(float dt, std::vector<std::unique_ptr<StickFigure>>& players,
                const std::vector<ArenaSlowZone>& slowZones = {},
                const std::vector<ArenaExplosion>& explosions = {});
    void draw(sf::RenderTarget& target) const;
    const std::vector<b2Vec2>& getSpawnPoints() const { return m_spawnPoints; }
    const std::vector<Platform>& getPlatforms() const { return m_platforms; }

    b2Vec2 getRandomPlatformTop() const;

    int carveCircle(Physics& physics, float cx, float cy, float radius, float raggedness = 0.0f);

    static int getLevelCount();
    static std::string getLevelName(int index);

private:
    void addPlatform(Physics& physics, float cx, float cy, float hw, float hh,
                     PlatformType type = PlatformType::Ground);
    void addSpike(float cx, float cy, float hw, bool pointsUp, float dmg = 30.0f);
    void addMovingPlatform(Physics& physics, float sx, float sy, float ex, float ey,
                           float hw, float hh, float speed, PlatformType type = PlatformType::Metal);
    void addSwingingPlatform(Physics& physics, float pivX, float pivY, float ropeLen,
                             float hw, float hh, float maxAng, PlatformType type = PlatformType::Wood);
    void addCrusher(Physics& physics, float cx, float topY, float botY,
                    float hw, float hh, float speed, float pause);

    std::vector<Platform>          m_platforms;
    std::vector<Spike>             m_spikes;
    std::vector<MovingPlatform>    m_movers;
    std::vector<SwingingPlatform>  m_swingers;
    std::vector<Crusher>           m_crushers;
    std::vector<b2Vec2>            m_spawnPoints;
    Physics* m_physics = nullptr;
    int m_currentLevel = 0;

    // Original 4 maps
    void buildClassic(Physics& physics);
    void buildVillage(Physics& physics);
    void buildFortress(Physics& physics);
    void buildSkyscrapers(Physics& physics);
    // Bigger maps
    void buildVolcano(Physics& physics);
    void buildIceCavern(Physics& physics);
    void buildSkyTemple(Physics& physics);
    void buildDeathPit(Physics& physics);
    // New maps
    void buildTreehouse(Physics& physics);
    void buildAbyss(Physics& physics);
    void buildCanyon(Physics& physics);
    void buildShipwreck(Physics& physics);
    void buildUnderground(Physics& physics);
    void buildTowers(Physics& physics);

    static sf::Color fillColorForType(PlatformType type);
    static sf::Color outlineColorForType(PlatformType type);
};

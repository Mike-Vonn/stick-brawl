#pragma once
#include "Physics.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class PlatformType {
    Ground,     // dark grey, sturdy
    Wood,       // brown, destructible
    Stone,      // grey, tough
    Metal,      // steel blue, very tough
    Brick,      // reddish, moderate
    Roof,       // triangular looking (drawn as angled)
};

struct Platform {
    b2BodyId bodyId;
    float halfWidth;
    float halfHeight;
    float cx, cy;
    bool alive = true;
    PlatformType type = PlatformType::Ground;
    float health = 100.0f;    // structural HP
    float maxHealth = 100.0f;
};

// Level definition
struct LevelDef {
    std::string name;
    std::vector<Platform> platforms; // template, bodies created at load
    std::vector<b2Vec2> spawnPoints;
};

class Arena {
public:
    // Build a specific level
    void createLevel(Physics& physics, int levelIndex);
    void draw(sf::RenderTarget& target) const;
    const std::vector<b2Vec2>& getSpawnPoints() const { return m_spawnPoints; }
    const std::vector<Platform>& getPlatforms() const { return m_platforms; }

    b2Vec2 getRandomPlatformTop() const;

    // Destroy platforms in radius (nuke-style, removes chunks)
    int destroyPlatformsInRadius(Physics& physics, float cx, float cy, float radius);

    // Damage platforms in radius (all weapons, chips away HP)
    void damagePlatformsInRadius(Physics& physics, float cx, float cy, float radius, float damage);

    static int getLevelCount();
    static std::string getLevelName(int index);

private:
    void addPlatform(Physics& physics, float cx, float cy, float hw, float hh,
                     PlatformType type = PlatformType::Ground);

    std::vector<Platform> m_platforms;
    std::vector<b2Vec2>   m_spawnPoints;
    Physics* m_physics = nullptr;
    int m_currentLevel = 0;

    // Level builders
    void buildClassic(Physics& physics);
    void buildVillage(Physics& physics);
    void buildFortress(Physics& physics);
    void buildSkyscrapers(Physics& physics);

    // Platform health based on type
    static float healthForType(PlatformType type);

    // Colors for rendering
    static sf::Color fillColorForType(PlatformType type);
    static sf::Color outlineColorForType(PlatformType type);
};

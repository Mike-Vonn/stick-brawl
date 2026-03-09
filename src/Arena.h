#pragma once
#include "Physics.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class PlatformType {
    Ground,     // dark grey
    Wood,       // brown
    Stone,      // grey
    Metal,      // steel blue
    Brick,      // reddish
    Roof,       // dark red
};

struct Platform {
    b2BodyId bodyId;
    float halfWidth;
    float halfHeight;
    float cx, cy;
    bool alive = true;
    PlatformType type = PlatformType::Ground;
    bool onFire = false;
    float fireTimer = 0.0f;
    static constexpr float BURN_DURATION = 5.0f;
};

class Arena {
public:
    void createLevel(Physics& physics, int levelIndex);
    void draw(sf::RenderTarget& target) const;
    const std::vector<b2Vec2>& getSpawnPoints() const { return m_spawnPoints; }
    const std::vector<Platform>& getPlatforms() const { return m_platforms; }

    b2Vec2 getRandomPlatformTop() const;

    // Returns the original spawn if there's ground beneath it,
    // otherwise picks the top of the nearest surviving platform.
    b2Vec2 getSafeSpawnPoint(int spawnIndex, Physics& physics) const;

    // Worms-style terrain carving: removes a circular chunk from all platforms
    // Returns number of platforms affected
    int carveCircle(Physics& physics, float cx, float cy, float radius);

    void ignitePlatform(size_t index);
    void updateFire(Physics& physics, float dt);
    bool isFlammable(PlatformType type) const;
    int getBurningPlatformAt(float x, float y) const;

    static int getLevelCount();
    static std::string getLevelName(int index);

private:
    void addPlatform(Physics& physics, float cx, float cy, float hw, float hh,
                     PlatformType type = PlatformType::Ground);

    std::vector<Platform> m_platforms;
    std::vector<b2Vec2>   m_spawnPoints;
    Physics* m_physics = nullptr;
    int m_currentLevel = 0;

    void buildClassic(Physics& physics);
    void buildVillage(Physics& physics);
    void buildFortress(Physics& physics);
    void buildSkyscrapers(Physics& physics);

    static sf::Color fillColorForType(PlatformType type);
    static sf::Color outlineColorForType(PlatformType type);
};

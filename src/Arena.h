#pragma once
#include "Physics.h"
#include <SFML/Graphics.hpp>
#include <vector>

struct Platform {
    b2BodyId bodyId;
    float halfWidth;
    float halfHeight;
    float cx, cy;
};

class Arena {
public:
    void createDefaultArena(Physics& physics);
    void draw(sf::RenderTarget& target) const;
    const std::vector<b2Vec2>& getSpawnPoints() const { return m_spawnPoints; }
    const std::vector<Platform>& getPlatforms() const { return m_platforms; }

    // Get a random position on top of a platform
    b2Vec2 getRandomPlatformTop() const;

private:
    std::vector<Platform> m_platforms;
    std::vector<b2Vec2>   m_spawnPoints;
};

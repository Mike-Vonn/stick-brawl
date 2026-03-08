#pragma once
#include "Physics.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <random>

// What kind of death animation to play
enum class DeathAnimType {
    Collapse,       // Fists, gun, fall — body topples over
    Dismember,      // Katana/blade — head or arm detaches and flies off
    Explode,        // Explosive direct hit — body blows into many pieces
    Disintegrate    // Nuke — body vaporizes into particles
};

// A single flying body part (gib)
struct Gib {
    b2BodyId bodyId;
    float width, height;    // size in meters (for box gibs)
    float radius;           // size in meters (for circle gibs, like head)
    bool isCircle;
    sf::Color color;
    float lifetime;
    float maxLifetime;
    bool alive = true;
};

// Blood/particle splatter
struct BloodParticle {
    float x, y;
    float vx, vy;
    float lifetime;
    float maxLifetime;
    float size;
    sf::Color color;
    bool alive = true;
};

// A complete death animation effect
struct DeathEffect {
    DeathAnimType type;
    float x, y;                  // world position where death occurred
    sf::Color playerColor;
    float timer = 0.0f;
    float duration = 3.0f;       // how long the effect lasts
    bool alive = true;
    int playerIndex = -1;

    std::vector<Gib> gibs;
    std::vector<BloodParticle> particles;

    // For collapse animation: the body tilts and falls
    float collapseAngle = 0.0f;
    float collapseVelY = 0.0f;
    bool collapseLanded = false;

    // For dismember: which part was detached (0=head, 1=left arm, 2=right arm)
    int dismemberedPart = -1;
};

class DeathAnimationSystem {
public:
    // Spawn a death effect at the given position
    void spawnDeath(Physics& physics, DeathAnimType type,
                    float x, float y, sf::Color color, int playerIndex,
                    float knockbackX = 0.0f, float knockbackY = 0.0f);

    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    void cleanup(Physics& physics);

    bool hasActiveEffect(int playerIndex) const;

private:
    void spawnCollapse(Physics& physics, DeathEffect& fx, float kbX, float kbY);
    void spawnDismember(Physics& physics, DeathEffect& fx, float kbX, float kbY);
    void spawnExplode(Physics& physics, DeathEffect& fx, float kbX, float kbY);
    void spawnDisintegrate(DeathEffect& fx);

    void drawGib(sf::RenderTarget& target, const Gib& gib, float effectAlpha) const;
    void drawCollapse(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const;
    void drawParticles(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const;

    void spawnBloodBurst(DeathEffect& fx, float cx, float cy, int count,
                         float speed, sf::Color color);

    std::vector<DeathEffect> m_effects;
    std::mt19937 m_rng{std::random_device{}()};
};

#pragma once
#include "Physics.h"
#include "StickFigure.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <random>

// What kind of death animation to play
enum class DeathAnimType {
    Collapse,       // Fists, gun, fall — body topples over
    Dismember,      // Katana/blade — head or arm detaches and flies off
    Explode,        // Explosive direct hit — body blows into many pieces
    Disintegrate,   // Nuke — body vaporizes into particles
    Incinerate      // Fire/burn — charcoalizes, skeleton appears, ash/embers
};

// Body plan for character-aware death animations
enum class BodyPlan { Humanoid, Quadruped, Serpentine };

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
struct Particle {
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
    CharacterType charType = CharacterType::Stick;

    std::vector<Gib> gibs;
    std::vector<Particle> particles;

    // For collapse animation: the body tilts and falls
    float collapseAngle = 0.0f;
    float collapseVelY = 0.0f;
    bool collapseLanded = false;

    // For dismember: which part was detached (0=head, 1=left arm, 2=right arm)
    int dismemberedPart = -1;

    // For incinerate animation
    float incinerateScale = 1.0f;   // shrink factor during crumble phase
    float skeletonAngle = 0.0f;     // skeleton falling-over angle
    bool skeletonLanded = false;
};

class DeathAnimationSystem {
public:
    // Spawn a death effect at the given position
    void spawnDeath(Physics& physics, DeathAnimType type,
                    float x, float y, sf::Color color, int playerIndex,
                    CharacterType charType = CharacterType::Stick,
                    float knockbackX = 0.0f, float knockbackY = 0.0f);

    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    void cleanup(Physics& physics);
    void cleanupAll(Physics& physics);

    bool hasActiveEffect(int playerIndex) const;

private:
    void spawnCollapse(Physics& physics, DeathEffect& fx, float kbX, float kbY);
    void spawnDismember(Physics& physics, DeathEffect& fx, float kbX, float kbY);
    void spawnExplode(Physics& physics, DeathEffect& fx, float kbX, float kbY);
    void spawnDisintegrate(DeathEffect& fx);
    void spawnIncinerate(DeathEffect& fx);

    void drawGib(sf::RenderTarget& target, const Gib& gib, float effectAlpha) const;
    void drawCollapse(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const;
    void drawParticles(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const;
    void drawIncinerate(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const;

    // Body-plan collapse drawing helpers
    void drawCollapseHumanoid(sf::RenderTarget& target, const DeathEffect& fx,
                               sf::Color c, float angle, float scale) const;
    void drawCollapseQuadruped(sf::RenderTarget& target, const DeathEffect& fx,
                                sf::Color c, float angle, float scale) const;
    void drawCollapseSerpentine(sf::RenderTarget& target, const DeathEffect& fx,
                                 sf::Color c, float angle, float scale) const;

    // Skeleton drawing for incinerate phase 3
    void drawSkeletonHumanoid(sf::RenderTarget& target, const DeathEffect& fx,
                               sf::Color c, float angle) const;
    void drawSkeletonQuadruped(sf::RenderTarget& target, const DeathEffect& fx,
                                sf::Color c, float angle) const;
    void drawSkeletonSerpentine(sf::RenderTarget& target, const DeathEffect& fx,
                                 sf::Color c, float angle) const;

    void spawnBloodBurst(DeathEffect& fx, float cx, float cy, int count,
                         float speed, sf::Color color);

    std::vector<DeathEffect> m_effects;
    std::mt19937 m_rng{std::random_device{}()};
};

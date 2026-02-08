#pragma once
#include "Physics.h"
#include "Renderer.h"
#include "Arena.h"
#include "StickFigure.h"
#include "Input.h"
#include "WeaponFactory.h"
#include "RulesEngine.h"
#include "HUD.h"
#include <vector>
#include <memory>

enum class GameState { Menu, Playing, RoundOver, GameOver };

struct Projectile {
    b2BodyId bodyId;
    WeaponData weapon;
    int ownerIndex = -1;
    float lifetime = 0.0f;
    bool alive = true;
    bool isPoison = false;   // poison projectiles apply DOT instead of burst
    float poisonDps = 0.0f;
    float poisonDuration = 0.0f;
};

struct WeaponPickup {
    b2Vec2 position;
    WeaponData weapon;
    float bobTimer = 0.0f;
    bool alive = true;
};

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();

private:
    void processEvents();
    void update(float dt);
    void render();

    void handlePlayerInput(float dt);
    void handleMeleeAttack(StickFigure& attacker);
    void spawnProjectile(StickFigure& shooter);
    void updateProjectiles(float dt);
    void checkFallDeath();
    void updateWeaponSpawns(float dt);
    void updateWeaponPickups(float dt);
    void checkRoundEnd();

    GameState m_state = GameState::Playing;
    float     m_roundTimer = 0.0f;
    float     m_weaponSpawnTimer = 0.0f;

    Renderer      m_renderer;
    Physics       m_physics;
    Arena         m_arena;
    Input         m_input;
    WeaponFactory m_weaponFactory;
    RulesEngine   m_rulesEngine;
    HUD           m_hud;

    std::vector<std::unique_ptr<StickFigure>> m_players;
    std::vector<Projectile> m_projectiles;
    std::vector<WeaponPickup> m_pickups;

    const sf::Color m_playerColors[4] = {
        sf::Color(100, 180, 255),  // Blue
        sf::Color(255, 100, 100),  // Red
        sf::Color(100, 255, 100),  // Green
        sf::Color(180, 100, 220),  // Purple (cobra)
    };
};

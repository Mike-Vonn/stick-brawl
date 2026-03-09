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
#include <array>

enum class GameState { CharSelect, Playing, RoundOver, GameOver };

struct Projectile {
    b2BodyId bodyId;
    WeaponData weapon;
    int ownerIndex = -1;
    float lifetime = 0.0f;
    bool alive = true;
    bool isPoison = false;
    float poisonDps = 0.0f;
    float poisonDuration = 0.0f;
    float ownerDamageMultiplier = 1.0f;
};

struct WeaponPickup {
    b2Vec2 position;
    WeaponData weapon;
    float bobTimer = 0.0f;
    bool alive = true;
};

struct ExplosionEffect {
    float x, y;
    float radius;
    float timer = 0.0f;
    float duration = 1.5f;
    bool isNuke = false;
    bool alive = true;
};

struct PlayerSelectState {
    bool joined = false;
    bool ready = false;
    int  charIndex = 0;
    float previewTimer = 0.0f;
};

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();

private:
    void processCharSelectEvents();
    void updateCharSelect(float dt);
    void renderCharSelect();
    bool allPlayersReady() const;
    void startGame();

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

    GameState m_state = GameState::CharSelect;
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
    std::vector<ExplosionEffect> m_explosions;

    std::array<PlayerSelectState, MAX_PLAYERS> m_selectState;
    std::array<bool, MAX_PLAYERS> m_prevSelectLeft = {};
    std::array<bool, MAX_PLAYERS> m_prevSelectRight = {};
    float m_selectAnimTimer = 0.0f;
    int   m_selectedLevel = 0;
    bool  m_wrapAround = false;

    static constexpr sf::Color m_playerColors[MAX_PLAYERS] = {
        sf::Color(100, 180, 255),  // Blue
        sf::Color(255, 100, 100),  // Red
        sf::Color(100, 255, 100),  // Green
        sf::Color(180, 100, 220),  // Purple
        sf::Color(255, 200, 100),  // Gold
    };
};

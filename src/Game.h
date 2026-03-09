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
    // Advanced
    bool  returnsToSender   = false;
    bool  hitsMultipleTimes = false;
    int   bouncesLeft       = 0;
    bool  isStuck           = false;   // sticky/mine: landed on surface
    float stuckDetonateTimer = 0.0f;
    std::vector<int> hitPlayers;       // track multi-hit targets
    // Thrown weapon
    bool  isThrownWeapon    = false;
    WeaponData thrownWeaponData;       // original weapon to drop as pickup
    // Portal projectile
    bool  createsPortal     = false;
    float portalDuration    = 0.0f;
    // Grenade casing state
    bool  isMine            = false;   // mine casing: armed on surface
    bool  mineTriggered     = false;   // mine: player detected, beeping before detonation
    float mineDetonateTimer = 0.0f;    // mine: countdown after trigger (0.25s)
    bool  isFragChild       = false;   // frag sub-grenade
    bool  isImploding       = false;   // implode charge: currently pulling players in
    float implodeTimer      = 0.0f;    // implode charge: time spent pulling
    bool  stuckToPlayer     = false;   // sticky casing: stuck to a player
    int   stuckPlayerIndex  = -1;
    b2Vec2 stuckOffset      = {0, 0};
    bool  stuckToPlatform   = false;   // stuck to a moving platform
    b2BodyId stuckPlatformBody = b2_nullBodyId; // platform body we're attached to
    b2Vec2 stuckPlatformOffset = {0, 0}; // offset from platform body center
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
    GrenadeCharge charge = GrenadeCharge::Normal;
    bool alive = true;
};

struct BlackHoleEffect {
    float x, y, radius, pullForce, dps, timer, duration;
    bool alive = true;
};

struct TornadoEffect {
    float x, y, vx, vy, radius, liftForce, spinForce, dps, timer, duration;
    int shooterIndex = -1;  // player who fired it
    bool alive = true;
};

struct TimeSlowZone {
    float x, y, radius, slowFactor, timer, duration;
    bool alive = true;
    // Secondary explosion when zone expires (Time Grenade)
    float secondaryDamage = 0.0f;
    float secondaryKnockback = 0.0f;
    float freezeBuildup = 0.0f;
    float freezeDuration = 0.0f;
};

struct Portal {
    float x, y, timer, duration;
    int   pairIndex = -1;
    bool  alive = true;
};

struct MeteorShower {
    float centerX, centerY, spreadRadius;
    int   meteorsLeft;
    float meteorTimer, meteorDelay;
    float damage, explosionRadius, knockback;
    int   ownerIndex;
    bool  alive = true;
};

struct HealZone {
    float x, y, radius, healRate, timer, duration;
    bool alive = true;
};

struct LavaPool {
    float x, y, radius, dps, timer, duration;
    int ownerIndex = -1;
    float particleTimer = 0.0f;  // spawn particles periodically
    bool alive = true;
};

struct LavaParticle {
    b2BodyId bodyId;
    float timer = 0.0f;
    float lifetime = 2.0f;
    float dps = 10.0f;
    bool alive = true;
};

struct GrappleState {
    bool active = false;
    b2BodyId hookBody = b2_nullBodyId;  // physics body of the hook projectile
    b2Vec2 hookPoint = {0, 0};          // world position where hook attached
    int playerIndex = -1;
    int grappledPlayerIndex = -1;       // -1 = attached to surface, >=0 = attached to player
    float ropeLength = 0.0f;            // current rope length
    float maxLength = 25.0f;
    float minLength = 1.5f;
    WeaponData weapon;                  // grapple weapon data for speeds/forces
    bool hookFlying = false;            // hook projectile in flight
    b2BodyId hookProjectile = b2_nullBodyId; // flying hook body
    b2BodyId attachedPlatformBody = b2_nullBodyId; // platform the hook is attached to
    b2Vec2 attachOffset = {0, 0};       // offset from platform body center
};

// Per-player selection state during character select
struct PlayerSelectState {
    bool joined = false;
    bool ready = false;
    int  charIndex = 0;  // index into CharacterType enum
    float previewTimer = 0.0f;
};

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();

private:
    // Character select
    void processCharSelectEvents();
    void updateCharSelect(float dt);
    void renderCharSelect();
    bool allPlayersReady() const;
    void startGame();

    // Gameplay
    void processEvents();
    void update(float dt);
    void render();

    void handlePlayerInput(float dt);
    void handleMeleeAttack(StickFigure& attacker);
    void spawnProjectile(StickFigure& shooter);
    void throwWeapon(StickFigure& thrower);
    void updateProjectiles(float dt);
    void checkFallDeath();
    void checkCombatDeaths();
    void updateWeaponSpawns(float dt);
    void updateWeaponPickups(float dt);
    void checkRoundEnd();
    // Area effects
    void updateBlackHoles(float dt);
    void updateTornadoes(float dt);
    void updateTimeSlowZones(float dt);
    void updatePortals(float dt);
    void updateMeteorShowers(float dt);
    void updateHealZones(float dt);
    void updateLavaPools(float dt);
    // Helpers
    void applyProjectileHitEffects(StickFigure& target, const WeaponData& w, int ownerIdx, float dx, float dy);
    void spawnExplosion(float x, float y, float radius, bool isNuke = false, GrenadeCharge charge = GrenadeCharge::Normal);
    void pushNearbyProjectiles(float x, float y, float radius, float force, int excludeIndex = -1);
    void spawnFragChildren(float x, float y, int ownerIndex, const WeaponData& parentWeapon);
    void spawnLavaPool(float x, float y, float radius, float dps, float duration, int ownerIndex);
    WeaponData randomizeGrenade(const WeaponData& base);  // roll random charge+casing
    StickFigure* findNearestEnemy(float x, float y, int excludeIdx, float maxRange);
    void updateGrapples(float dt);
    void fireGrapple(StickFigure& shooter);
    void releaseGrapple(int playerIndex);
    void slingshotGrapple(int playerIndex);

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
    std::vector<BlackHoleEffect> m_blackHoles;
    std::vector<TornadoEffect>   m_tornadoes;
    std::vector<TimeSlowZone>    m_timeSlowZones;
    std::vector<Portal>          m_portals;
    std::vector<MeteorShower>    m_meteorShowers;
    std::vector<HealZone>        m_healZones;
    std::vector<LavaPool>        m_lavaPools;
    std::vector<LavaParticle>    m_lavaParticles;
    std::vector<GrappleState>    m_grapples;

    // Character select state
    std::array<PlayerSelectState, MAX_PLAYERS> m_selectState;
    float m_selectAnimTimer = 0.0f;
    int   m_selectedLevel = 14; // 14 = Random
    bool  m_wrapAround = false;  // fall-through wrap-around mode

    // Camera / scrolling
    float m_camX = 0.0f;  // camera center in world coords
    float m_camY = 0.0f;
    float m_camZoom = 1.0f; // >1 = zoomed out to fit more
    void updateCamera(float dt);

    static constexpr sf::Color m_playerColors[MAX_PLAYERS] = {
        sf::Color(100, 180, 255),  // Blue
        sf::Color(255, 100, 100),  // Red
        sf::Color(100, 255, 100),  // Green
        sf::Color(180, 100, 220),  // Purple
        sf::Color(255, 200, 100),  // Gold (unicorn default)
    };
};

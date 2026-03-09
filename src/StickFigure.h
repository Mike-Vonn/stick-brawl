#pragma once
#include "Physics.h"
#include "Weapon.h"
#include <SFML/Graphics.hpp>

// Global player size scale (1.0 = original, 0.6 = 60% size for bigger maps)
constexpr float PLAYER_SCALE = 0.6f;

struct StickFigureConfig {
    float bodyHeight = 1.8f;
    float bodyWidth  = 0.3f;
    float headRadius = 0.25f;
    float limbLength = 0.6f;
    float limbWidth  = 0.08f;
};

enum class CharacterType {
    Stick,
    Cat,
    Cobra,
    Unicorn,
    Crocodile,
    StickLady
};

// Number of available character types
constexpr int CHARACTER_TYPE_COUNT = 6;

inline const char* characterTypeName(CharacterType t) {
    switch (t) {
        case CharacterType::Stick:     return "Stick";
        case CharacterType::Cat:       return "Cat";
        case CharacterType::Cobra:     return "Cobra";
        case CharacterType::Unicorn:   return "Unicorn";
        case CharacterType::Crocodile: return "Crocodile";
        case CharacterType::StickLady: return "Stick Lady";
    }
    return "???";
}

// ── Per-body-part hitbox system ──────────────────────────────────
enum class BodyPart {
    None = -1,
    Head = 0,
    Torso,
    LeftUpperArm,
    RightUpperArm,
    LeftForeArm,
    RightForeArm,
    LeftUpperLeg,
    RightUpperLeg,
    LeftLowerLeg,
    RightLowerLeg,
    COUNT // 10 total
};

constexpr int BODY_PART_COUNT = static_cast<int>(BodyPart::COUNT);

// Damage multiplier per body part
inline float bodyPartDamageMultiplier(BodyPart part) {
    switch (part) {
        case BodyPart::Head:           return 2.0f;   // headshot!
        case BodyPart::Torso:          return 1.0f;   // standard
        case BodyPart::LeftUpperArm:
        case BodyPart::RightUpperArm:  return 0.7f;   // arm hit
        case BodyPart::LeftForeArm:
        case BodyPart::RightForeArm:   return 0.6f;   // forearm
        case BodyPart::LeftUpperLeg:
        case BodyPart::RightUpperLeg:  return 0.75f;  // thigh
        case BodyPart::LeftLowerLeg:
        case BodyPart::RightLowerLeg:  return 0.5f;   // shin
        default:                       return 1.0f;
    }
}

inline const char* bodyPartName(BodyPart part) {
    switch (part) {
        case BodyPart::Head:           return "HEAD";
        case BodyPart::Torso:          return "BODY";
        case BodyPart::LeftUpperArm:   return "L.ARM";
        case BodyPart::RightUpperArm:  return "R.ARM";
        case BodyPart::LeftForeArm:    return "L.HAND";
        case BodyPart::RightForeArm:   return "R.HAND";
        case BodyPart::LeftUpperLeg:   return "L.LEG";
        case BodyPart::RightUpperLeg:  return "R.LEG";
        case BodyPart::LeftLowerLeg:   return "L.FOOT";
        case BodyPart::RightLowerLeg:  return "R.FOOT";
        default:                       return "";
    }
}

struct HitResult {
    bool     hit      = false;
    BodyPart part     = BodyPart::None;
    float    distance = 999.0f;
    b2Vec2   partPos  = {0,0};
};

class StickFigure {
public:
    StickFigure(int playerIndex, Physics& physics, float spawnX, float spawnY,
                sf::Color color, CharacterType type = CharacterType::Stick);
    ~StickFigure() = default;

    void moveLeft();
    void moveRight();
    void jump();
    void stopMoving();
    void applyRecoil(float forceX, float forceY); // weapon kickback

    // Aiming
    void aimUp();
    void aimDown();
    void resetAim();
    float getAimAngle() const { return m_aimAngle; }

    bool canAttack() const;
    void attack();
    void equipWeapon(const WeaponData& weapon);
    void setDefaultWeapon(const WeaponData& weapon) { m_defaultWeapon = weapon; }
    void revertToDefaultWeapon();
    const WeaponData& getCurrentWeapon() const { return m_weapon; }
    const WeaponData& getDefaultWeapon() const { return m_defaultWeapon; }
    bool hasPickedUpWeapon() const { return m_weapon.name != m_defaultWeapon.name; }
    int getAmmo() const { return m_currentAmmo; }

    void takeDamage(float amount, float knockbackX, float knockbackY);
    // Per-body-part damage: applies multiplier and flashes the hit part
    void takeDamageAt(BodyPart part, float baseDamage, float knockbackX, float knockbackY);

    void applyPoison(float dps, float duration);
    void applySlow(float factor, float duration);
    void addFreezeStacks(int stacks);       // freeze ray/grenade: each stack = 9% slow
    void applyTimeSlow(float duration);     // time gun/grenade: full stop
    void applyStun(float duration);
    void applyShrink(float scale, float speedMult, float duration);
    void applyShield(float reduction, float duration, bool reflects);
    void applyHeal(float rate, float duration);
    void heal(float amount);
    void applyMindControl(int controller, float duration);
    void breakMindControl();
    bool isFrozen()        const { return m_timeSlowTimer > 0.0f; }
    int  getFreezeStacks() const { return m_freezeStacks; }
    bool isStunned()       const { return m_stunTimer > 0.0f; }
    bool isMindControlled()const { return m_mindControlled; }
    int  getMindController()const{ return m_mindController; }
    bool hasShield()       const { return m_shieldTimer > 0.0f; }
    bool isReflecting()    const { return m_reflecting; }
    float getDamageReduction() const { return m_damageReduction; }
    float getSlowFactor()  const { return m_slowFactor; }
    float getTeleportCD()  const { return m_teleportCD; }
    void  setTeleportCD(float v){ m_teleportCD = v; }
    float getShieldTimer() const { return m_shieldTimer; }
    void respawn(float x, float y);
    void teleportTo(float x, float y); // preserves velocity (for wrap-around)
    void startRespawnTimer(float delay, float x, float y);
    bool isWaitingToRespawn() const { return m_waitingToRespawn; }
    void update(float dt);

    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    bool  isAlive() const { return m_health > 0.0f; }
    bool  isPoisoned() const { return m_poisonTimer > 0.0f; }
    int   getPlayerIndex() const { return m_playerIndex; }
    int   getLives() const { return m_lives; }
    void  setLives(int lives) { m_lives = lives; }
    void  setMaxHealth(float hp) { m_maxHealth = hp; m_health = hp; }
    sf::Color getColor() const { return m_color; }
    CharacterType getCharacterType() const { return m_charType; }

    b2Vec2 getPosition() const;
    int getFacingDirection() const { return m_facingDir; }

    void draw(sf::RenderTarget& target) const;

    b2BodyId getTorsoBodyId() const { return m_torso; }
    bool isOnGround() const;
    int  isTouchingWall() const;  // -1=left wall, +1=right wall, 0=none

    // ── Per-body-part hitbox queries ────────────────────────────
    HitResult checkHit(float wx, float wy, float maxRadius = 0.8f) const;
    b2Vec2 getBodyPartPosition(BodyPart part) const;
    float getBodyPartRadius(BodyPart part) const;

private:
    void createBodies(Physics& physics, float spawnX, float spawnY);
    void drawStick(sf::RenderTarget& target) const;
    void drawCat(sf::RenderTarget& target) const;
    void drawCobra(sf::RenderTarget& target) const;
    void drawUnicorn(sf::RenderTarget& target) const;
    void drawCrocodile(sf::RenderTarget& target) const;
    void drawStickLady(sf::RenderTarget& target) const;
    void drawAttackEffect(sf::RenderTarget& target) const;
    void drawAimIndicator(sf::RenderTarget& target) const;
    void drawWeapon(sf::RenderTarget& target) const;
    void drawStatusEffects(sf::RenderTarget& target) const;
    void drawHitFlashes(sf::RenderTarget& target) const;

    int           m_playerIndex;
    sf::Color     m_color;
    CharacterType m_charType;
    Physics*      m_physics = nullptr;

    b2BodyId  m_head, m_torso;
    b2BodyId  m_leftUpperArm,  m_rightUpperArm;
    b2BodyId  m_leftForeArm,   m_rightForeArm;
    b2BodyId  m_leftUpperLeg,  m_rightUpperLeg;
    b2BodyId  m_leftLowerLeg,  m_rightLowerLeg;
    b2BodyId& m_leftArm  = m_leftUpperArm;
    b2BodyId& m_rightArm = m_rightUpperArm;
    b2BodyId& m_leftLeg  = m_leftUpperLeg;
    b2BodyId& m_rightLeg = m_rightUpperLeg;

    b2JointId m_neckJoint;
    b2JointId m_leftShoulderJoint, m_rightShoulderJoint;
    b2JointId m_leftElbowJoint,    m_rightElbowJoint;
    b2JointId m_leftHipJoint,      m_rightHipJoint;
    b2JointId m_leftKneeJoint,     m_rightKneeJoint;

    float m_health;
    float m_maxHealth = 100.0f;
    int   m_lives = 3;
    int   m_facingDir = 1;
    float m_aimAngle = 0.0f;
    float m_attackCooldown = 0.0f;
    float m_attackAnimTimer = 0.0f;
    float m_damageFlashTimer = 0.0f;
    float m_respawnTimer = 0.0f;
    bool  m_waitingToRespawn = false;
    float m_pendingRespawnX = 0.0f;
    float m_pendingRespawnY = 0.0f;

    float m_poisonTimer = 0.0f;
    float m_poisonDps = 0.0f;
    float m_poisonTickTimer = 0.0f;

    float m_slowTimer       = 0.0f;    // generic slow (from slow weapons)
    float m_slowFactor      = 1.0f;
    int   m_freezeStacks    = 0;       // 0-10, each = 9% speed reduction
    float m_freezeStackDecayTimer = 0.0f;  // time until next stack falls off
    float m_timeSlowTimer   = 0.0f;    // time slow: completely stops player
    float m_stunTimer       = 0.0f;
    float m_shrinkTimer     = 0.0f;
    float m_shrinkScale     = 1.0f;
    float m_shrinkSpeedMult = 1.0f;
    float m_shieldTimer     = 0.0f;
    float m_damageReduction = 0.0f;
    bool  m_reflecting      = false;
    float m_healTimer       = 0.0f;
    float m_healRate        = 0.0f;
    float m_teleportCD      = 0.0f;
    bool  m_mindControlled  = false;
    float m_mindControlTimer = 0.0f;
    int   m_mindController  = -1;

    WeaponData m_weapon;
    WeaponData m_defaultWeapon;   // character's innate weapon to revert to
    int        m_currentAmmo = -1;

    float m_moveSpeed = 8.0f;
    float m_jumpForce = 16.0f;

    StickFigureConfig m_config;

    float m_animTime = 0.0f;

    // Per-body-part hit flash timers (visual feedback)
    float m_partFlash[BODY_PART_COUNT] = {};
};

#pragma once
#include "Physics.h"
#include "Weapon.h"
#include <SFML/Graphics.hpp>
#include <vector>

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
    Lion,
    Tiger,
    Jaguar,
    Panther,
    Cheetah,
    Cobra,
    Unicorn,
    Crocodile,
    StickLady,
    Dragon,
    MrDiaperPants
};

// Number of available character types
constexpr int CHARACTER_TYPE_COUNT = 13;

inline const char* characterTypeName(CharacterType t) {
    switch (t) {
        case CharacterType::Stick:     return "Stick";
        case CharacterType::Cat:       return "Cat";
        case CharacterType::Lion:      return "Lion";
        case CharacterType::Tiger:     return "Tiger";
        case CharacterType::Jaguar:    return "Jaguar";
        case CharacterType::Panther:   return "Panther";
        case CharacterType::Cheetah:   return "Cheetah";
        case CharacterType::Cobra:     return "Cobra";
        case CharacterType::Unicorn:   return "Unicorn";
        case CharacterType::Crocodile: return "Crocodile";
        case CharacterType::StickLady: return "Stick Lady";
        case CharacterType::Dragon:    return "Dragon";
        case CharacterType::MrDiaperPants: return "Mr Diaper-Pants";
    }
    return "???";
}

inline const char* characterTypeBlurb(CharacterType t) {
    switch (t) {
        case CharacterType::Cat:     return "80% HP | Wall Climb | Fast Scratch";
        case CharacterType::Lion:    return "130 HP | High Knockback";
        case CharacterType::Tiger:   return "Slow but Strong | +25% DMG";
        case CharacterType::Jaguar:  return "Wall Climb | Strong Bite";
        case CharacterType::Panther: return "Wall Climb | Fast & Stealthy";
        case CharacterType::Cheetah: return "75 HP | Super Fast";
        default: return "";
    }
}

inline bool canWallClimb(CharacterType t) {
    return t == CharacterType::Cat || t == CharacterType::Jaguar || t == CharacterType::Panther;
}

inline bool isBigCat(CharacterType t) {
    return t == CharacterType::Lion || t == CharacterType::Tiger
        || t == CharacterType::Jaguar || t == CharacterType::Panther
        || t == CharacterType::Cheetah;
}

class StickFigure {
public:
    StickFigure(int playerIndex, Physics& physics, float spawnX, float spawnY,
                sf::Color color, CharacterType type = CharacterType::Stick);
    ~StickFigure() = default;

    void moveLeft();
    void moveRight();
    void jump();
    void stopMoving();

    void wallClimbUp();
    int wallSide() const;  // 0 = no wall, -1 = wall on left, 1 = wall on right

    // Aiming
    void aimUp();
    void aimDown();
    void resetAim();
    float getAimAngle() const { return m_aimAngle; }

    struct WeaponSlot { WeaponData weapon; int ammo = -1; };

    bool canAttack() const;
    void attack();
    void setInnateWeapon(const WeaponData& weapon);
    void equipWeapon(const WeaponData& weapon);
    void equipWeapon(const WeaponData& weapon, int currentAmmo);
    void switchWeapon();
    std::vector<WeaponSlot> dropAllNonInnate();
    const WeaponData& getCurrentWeapon() const { return m_inventory[m_activeWeapon].weapon; }
    int getAmmo() const { return m_inventory[m_activeWeapon].ammo; }
    int getInventorySize() const { return static_cast<int>(m_inventory.size()); }
    int getActiveWeaponIndex() const { return m_activeWeapon; }

    void takeDamage(float amount, float knockbackX, float knockbackY);
    void takeDamage(float amount, float knockbackX, float knockbackY,
                    const std::string& weaponName, WeaponType weaponType,
                    const std::string& deathAnim = "");
    void applyPoison(float dps, float duration);
    void applyBurn(float dps, float duration);
    void glide(float dt);
    bool isGliding() const { return m_isGliding; }
    void respawn(float x, float y);

    // Last weapon that dealt damage (for death animation selection)
    const std::string& getLastDamageWeapon() const { return m_lastDamageWeapon; }
    WeaponType getLastDamageWeaponType() const { return m_lastDamageWeaponType; }
    const std::string& getLastDamageDeathAnim() const { return m_lastDamageDeathAnim; }
    float getLastKnockbackX() const { return m_lastKnockbackX; }
    float getLastKnockbackY() const { return m_lastKnockbackY; }
    void teleportTo(float x, float y); // preserves velocity (for wrap-around)
    void startRespawnTimer(float delay, float x, float y);
    bool isWaitingToRespawn() const { return m_waitingToRespawn; }
    void update(float dt);

    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    bool  isAlive() const { return m_health > 0.0f; }
    bool  isPoisoned() const { return m_poisonTimer > 0.0f; }
    bool  isBurning() const { return m_burnTimer > 0.0f; }
    int   getPlayerIndex() const { return m_playerIndex; }
    int   getLives() const { return m_lives; }
    void  setLives(int lives) { m_lives = lives; }
    void  setMaxHealth(float hp) { m_maxHealth = hp; m_health = hp; }
    sf::Color getColor() const { return m_color; }
    CharacterType getCharacterType() const { return m_charType; }
    float getDamageMultiplier() const { return m_damageMultiplier; }
    float getHealthMultiplier() const { return m_healthMultiplier; }

    b2Vec2 getPosition() const;
    int getFacingDirection() const { return m_facingDir; }

    void draw(sf::RenderTarget& target) const;

    b2BodyId getTorsoBodyId() const { return m_torso; }
    bool isOnGround() const;

private:
    void createBodies(Physics& physics, float spawnX, float spawnY);
    void applyCharacterStats();

    void drawStick(sf::RenderTarget& target) const;
    void drawCat(sf::RenderTarget& target) const;
    void drawLion(sf::RenderTarget& target) const;
    void drawTiger(sf::RenderTarget& target) const;
    void drawJaguar(sf::RenderTarget& target) const;
    void drawPanther(sf::RenderTarget& target) const;
    void drawCheetah(sf::RenderTarget& target) const;
    void drawCobra(sf::RenderTarget& target) const;
    void drawUnicorn(sf::RenderTarget& target) const;
    void drawCrocodile(sf::RenderTarget& target) const;
    void drawStickLady(sf::RenderTarget& target) const;
    void drawDragon(sf::RenderTarget& target) const;
    void drawMrDiaperPants(sf::RenderTarget& target) const;
    void drawAttackEffect(sf::RenderTarget& target) const;
    void drawAimIndicator(sf::RenderTarget& target) const;

    int           m_playerIndex;
    sf::Color     m_color;
    CharacterType m_charType;
    Physics*      m_physics = nullptr;

    b2BodyId  m_head, m_torso, m_leftArm, m_rightArm, m_leftLeg, m_rightLeg;
    b2JointId m_neckJoint, m_leftShoulderJoint, m_rightShoulderJoint;
    b2JointId m_leftHipJoint, m_rightHipJoint;

    float m_health;
    float m_maxHealth = 100.0f;
    int   m_lives = 3;
    int   m_facingDir = 1;
    float m_aimAngle = 0.0f; // radians, 0=straight, positive=up, negative=down
    float m_attackCooldown = 0.0f;
    float m_attackAnimTimer = 0.0f;
    float m_damageFlashTimer = 0.0f;
    float m_respawnTimer = 0.0f;
    bool  m_waitingToRespawn = false;
    float m_pendingRespawnX = 0.0f;
    float m_pendingRespawnY = 0.0f;

    // Poison DOT
    float m_poisonTimer = 0.0f;
    float m_poisonDps = 0.0f;
    float m_poisonTickTimer = 0.0f;

    // Burn DOT (fire)
    float m_burnTimer = 0.0f;
    float m_burnDps = 0.0f;
    float m_burnTickTimer = 0.0f;

    std::vector<WeaponSlot> m_inventory;  // slot 0 = innate weapon
    int m_activeWeapon = 0;

    // Last weapon that dealt the killing blow (for death animation selection)
    std::string m_lastDamageWeapon;
    WeaponType  m_lastDamageWeaponType = WeaponType::Melee;
    std::string m_lastDamageDeathAnim;
    float       m_lastKnockbackX = 0.0f;
    float       m_lastKnockbackY = 0.0f;

    bool  m_isGliding = false;
    float m_moveSpeed = 8.0f;
    float m_jumpForce = 12.0f;
    float m_damageMultiplier = 1.0f;
    float m_healthMultiplier = 1.0f;

    StickFigureConfig m_config;
    // Animation
    float m_animTime = 0.0f;
};

#pragma once
#include "Physics.h"
#include "Weapon.h"
#include <SFML/Graphics.hpp>

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
    Cobra
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

    // Aiming
    void aimUp();
    void aimDown();
    void resetAim();
    float getAimAngle() const { return m_aimAngle; }

    bool canAttack() const;
    void attack();
    void equipWeapon(const WeaponData& weapon);
    const WeaponData& getCurrentWeapon() const { return m_weapon; }
    int getAmmo() const { return m_currentAmmo; }

    void takeDamage(float amount, float knockbackX, float knockbackY);
    void applyPoison(float dps, float duration);
    void respawn(float x, float y);
    void update(float dt);

    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    bool  isAlive() const { return m_health > 0.0f; }
    bool  isPoisoned() const { return m_poisonTimer > 0.0f; }
    int   getPlayerIndex() const { return m_playerIndex; }
    int   getLives() const { return m_lives; }
    void  setLives(int lives) { m_lives = lives; }
    sf::Color getColor() const { return m_color; }
    CharacterType getCharacterType() const { return m_charType; }

    b2Vec2 getPosition() const;
    int getFacingDirection() const { return m_facingDir; }

    void draw(sf::RenderTarget& target) const;

    b2BodyId getTorsoBodyId() const { return m_torso; }
    bool isOnGround() const;

private:
    void createBodies(Physics& physics, float spawnX, float spawnY);
    void drawStick(sf::RenderTarget& target) const;
    void drawCat(sf::RenderTarget& target) const;
    void drawCobra(sf::RenderTarget& target) const;
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

    // Poison DOT
    float m_poisonTimer = 0.0f;
    float m_poisonDps = 0.0f;
    float m_poisonTickTimer = 0.0f;

    WeaponData m_weapon;
    int        m_currentAmmo = -1;

    float m_moveSpeed = 8.0f;
    float m_jumpForce = 12.0f;

    StickFigureConfig m_config;
};

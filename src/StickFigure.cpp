#include "StickFigure.h"
#include <cmath>
#include <iostream>
#include <algorithm>

static sf::Vector2f toScreen(b2Vec2 pos) {
    return {SCREEN_CX + pos.x * PPM, SCREEN_CY - pos.y * PPM};
}

StickFigure::StickFigure(int playerIndex, Physics& physics, float spawnX, float spawnY,
                         sf::Color color, CharacterType type)
    : m_playerIndex(playerIndex), m_color(color), m_charType(type)
    , m_physics(&physics), m_health(100.0f)
{
    createBodies(physics, spawnX, spawnY);
}

void StickFigure::createBodies(Physics& physics, float spawnX, float spawnY) {
    b2WorldId world = physics.getWorldId();

    const float torsoHalf = 0.55f * PLAYER_SCALE;
    const float torsoWide = 0.20f * PLAYER_SCALE;
    const float headR     = 0.30f * PLAYER_SCALE;
    const float collisionHalf = 1.40f * PLAYER_SCALE;

    m_config.bodyHeight = torsoHalf * 2.0f;
    m_config.bodyWidth  = torsoWide * 2.0f;
    m_config.headRadius = headR;
    m_config.limbLength = 0.60f * PLAYER_SCALE;
    m_config.limbWidth  = 0.07f * PLAYER_SCALE;

    // ── TORSO ──
    {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = {spawnX, spawnY};
        bd.fixedRotation = true;
        m_torso = b2CreateBody(world, &bd);

        b2Polygon box = b2MakeBox(torsoWide, collisionHalf);
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 3.0f; sd.material.friction = 0.5f;
        sd.filter.categoryBits = CAT_PLAYER;
        sd.filter.maskBits     = CAT_PLATFORM | CAT_PROJECTILE | CAT_PICKUP | CAT_PLAYER;
        b2CreatePolygonShape(m_torso, &sd, &box);
    }

    // ── HEAD ──
    {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = {spawnX, spawnY + torsoHalf + headR + 0.03f};
        m_head = b2CreateBody(world, &bd);

        b2Circle circ = {{0,0}, headR};
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 1.0f;
        sd.filter.categoryBits = CAT_PLAYER;
        sd.filter.maskBits     = CAT_PLATFORM | CAT_PROJECTILE;
        b2CreateCircleShape(m_head, &sd, &circ);

        b2RevoluteJointDef jd = b2DefaultRevoluteJointDef();
        jd.bodyIdA = m_torso; jd.bodyIdB = m_head;
        jd.localAnchorA = {0, torsoHalf};
        jd.localAnchorB = {0, -headR};
        jd.enableLimit = true; jd.lowerAngle = -0.3f; jd.upperAngle = 0.3f;
        jd.enableMotor = true; jd.motorSpeed = 0; jd.maxMotorTorque = 50.f;
        m_neckJoint = b2CreateRevoluteJoint(world, &jd);
    }

    // ── LIMBS — kinematic sensor bodies ──
    auto makeLimb = [&](b2BodyId& body, float cx, float cy) {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_kinematicBody;
        bd.position = {cx, cy};
        body = b2CreateBody(world, &bd);

        b2Polygon box = b2MakeBox(0.06f * PLAYER_SCALE, 0.28f * PLAYER_SCALE);
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 0.1f;
        sd.isSensor = true;
        sd.filter.categoryBits = CAT_PLAYER;
        sd.filter.maskBits     = CAT_PROJECTILE;
        b2CreatePolygonShape(body, &sd, &box);
    };

    makeLimb(m_leftUpperArm,  spawnX - 0.3f, spawnY + 0.3f);
    makeLimb(m_rightUpperArm, spawnX + 0.3f, spawnY + 0.3f);
    makeLimb(m_leftForeArm,   spawnX - 0.3f, spawnY);
    makeLimb(m_rightForeArm,  spawnX + 0.3f, spawnY);
    makeLimb(m_leftUpperLeg,  spawnX - 0.1f, spawnY - 0.6f);
    makeLimb(m_rightUpperLeg, spawnX + 0.1f, spawnY - 0.6f);
    makeLimb(m_leftLowerLeg,  spawnX - 0.1f, spawnY - 1.1f);
    makeLimb(m_rightLowerLeg, spawnX + 0.1f, spawnY - 1.1f);

    m_leftShoulderJoint = m_rightShoulderJoint = b2_nullJointId;
    m_leftElbowJoint = m_rightElbowJoint = b2_nullJointId;
    m_leftHipJoint = m_rightHipJoint = b2_nullJointId;
    m_leftKneeJoint = m_rightKneeJoint = b2_nullJointId;
}


bool StickFigure::isOnGround() const {
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.categoryBits = CAT_PLATFORM;
    filter.maskBits = CAT_PLATFORM;

    // Only check torso and leg bodies for ground contact
    // (not arms/head — those shouldn't reset jump)
    b2BodyId bodies[] = {m_torso, m_leftLowerLeg, m_rightLowerLeg,
                         m_leftUpperLeg, m_rightUpperLeg};
    for (auto body : bodies) {
        if (!B2_IS_NON_NULL(body) || !b2Body_IsValid(body)) continue;
        b2Vec2 pos = b2Body_GetPosition(body);
        b2Vec2 origin = {pos.x, pos.y - 0.15f};
        b2Vec2 translation = {0.0f, -0.25f};
        b2RayResult result = b2World_CastRayClosest(m_physics->getWorldId(), origin, translation, filter);
        if (result.hit) return true;
    }
    return false;
}

// Returns -1 for wall on left, +1 for wall on right, 0 for no wall
int StickFigure::isTouchingWall() const {
    b2Vec2 pos = b2Body_GetPosition(m_torso);
    float collisionHalf = 1.40f * PLAYER_SCALE;
    float torsoW = 0.20f * PLAYER_SCALE;
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.categoryBits = CAT_PLATFORM;
    filter.maskBits = CAT_PLATFORM;

    // Cast rays from the sides at 5 heights spanning the full collision box
    for (float frac : {-0.9f, -0.5f, 0.0f, 0.5f, 0.9f}) {
        float yOff = frac * collisionHalf;
        // Check right side
        b2Vec2 originR = {pos.x + torsoW, pos.y + yOff};
        b2Vec2 transR = {0.5f, 0.0f};
        b2RayResult resultR = b2World_CastRayClosest(m_physics->getWorldId(), originR, transR, filter);
        if (resultR.hit) return 1;

        // Check left side
        b2Vec2 originL = {pos.x - torsoW, pos.y + yOff};
        b2Vec2 transL = {-0.5f, 0.0f};
        b2RayResult resultL = b2World_CastRayClosest(m_physics->getWorldId(), originL, transL, filter);
        if (resultL.hit) return -1;
    }
    return 0;
}

void StickFigure::moveLeft()  {
    if (m_stunTimer > 0.0f || m_timeSlowTimer > 0.0f) return;
    m_facingDir = -1;
    float spd = m_moveSpeed * m_slowFactor * m_shrinkSpeedMult;
    b2Vec2 v = b2Body_GetLinearVelocity(m_torso);
    b2Body_SetLinearVelocity(m_torso, {-spd, v.y});
}
void StickFigure::moveRight() {
    if (m_stunTimer > 0.0f || m_timeSlowTimer > 0.0f) return;
    m_facingDir = 1;
    float spd = m_moveSpeed * m_slowFactor * m_shrinkSpeedMult;
    b2Vec2 v = b2Body_GetLinearVelocity(m_torso);
    b2Body_SetLinearVelocity(m_torso, { spd, v.y});
}
void StickFigure::stopMoving(){
    b2Vec2 v = b2Body_GetLinearVelocity(m_torso);
    b2Body_SetLinearVelocity(m_torso, {v.x * 0.85f, v.y});
}

void StickFigure::applyRecoil(float forceX, float forceY) {
    b2Body_ApplyLinearImpulseToCenter(m_torso, {forceX, forceY}, true);
}

void StickFigure::jump() {
    if (m_timeSlowTimer > 0.0f) return;
    float jumpMult = (m_slowTimer > 0.0f) ? std::max(0.3f, m_slowFactor) : 1.0f;
    // Any body part touching ground allows jump
    if (isOnGround()) {
        b2Vec2 v = b2Body_GetLinearVelocity(m_torso);
        b2Body_SetLinearVelocity(m_torso, {v.x, 0.0f});
        b2Body_ApplyLinearImpulseToCenter(m_torso, {0.0f, m_jumpForce * jumpMult}, true);
    } else {
        int wall = isTouchingWall();
        if (wall != 0) {
            float wallKickX = static_cast<float>(-wall) * m_jumpForce * 0.6f * jumpMult;
            b2Body_SetLinearVelocity(m_torso, {0.0f, 0.0f});
            b2Body_ApplyLinearImpulseToCenter(m_torso, {wallKickX, m_jumpForce * 0.85f * jumpMult}, true);
            m_facingDir = -wall;
        }
    }
}
void StickFigure::aimUp()    { m_aimAngle = std::min(m_aimAngle + 0.05f,  1.2f); }
void StickFigure::aimDown()  { m_aimAngle = std::max(m_aimAngle - 0.05f, -1.2f); }
void StickFigure::resetAim() { m_aimAngle *= 0.9f; } // slowly return to center

bool StickFigure::canAttack() const {
    if (m_attackCooldown > 0.0f) return false;
    if (m_timeSlowTimer > 0.0f) return false;  // can't attack while time-stopped
    if (m_weapon.ammo >= 0 && m_currentAmmo <= 0) return false;
    return true;
}

void StickFigure::attack() {
    m_attackCooldown = m_weapon.attackRate;
    m_attackAnimTimer = 0.2f;
    if (m_currentAmmo > 0) {
        m_currentAmmo--;
    }
}

void StickFigure::revertToDefaultWeapon() {
    m_weapon = m_defaultWeapon;
    m_currentAmmo = m_defaultWeapon.ammo;  // -1 for infinite melee
}

void StickFigure::equipWeapon(const WeaponData& weapon) {
    m_weapon = weapon;
    m_currentAmmo = weapon.ammo;
}

void StickFigure::takeDamage(float amount, float knockbackX, float knockbackY) {
    amount *= (1.0f - m_damageReduction);
    m_health -= amount;
    if (m_health < 0.0f) m_health = 0.0f;
    m_damageFlashTimer = 0.15f;
    if (m_timeSlowTimer <= 0.0f) {
        // Ragdoll: apply knockback to torso AND let it rotate
        b2Body_ApplyLinearImpulseToCenter(m_torso, {knockbackX, knockbackY}, true);

    }
    if (m_mindControlled) breakMindControl();
}

void StickFigure::applyPoison(float dps, float duration) {
    m_poisonDps = dps;
    m_poisonTimer = duration;
    m_poisonTickTimer = 0.0f;
}

void StickFigure::applySlow(float factor, float duration) {
    // Keep the strongest slow
    if (factor < m_slowFactor) m_slowFactor = factor;
    if (duration > m_slowTimer) m_slowTimer = duration;
}

void StickFigure::addFreezeStacks(int stacks) {
    m_freezeStacks = std::min(10, m_freezeStacks + stacks);
    m_freezeStackDecayTimer = 1.0f;  // reset decay timer on new stacks
}

void StickFigure::applyTimeSlow(float duration) {
    if (duration > m_timeSlowTimer) {
        m_timeSlowTimer = duration;
        b2Body_SetLinearVelocity(m_torso, {0.0f, 0.0f});
    }
}

void StickFigure::applyStun(float duration) {
    if (duration > m_stunTimer) m_stunTimer = duration;
}

void StickFigure::applyShrink(float scale, float speedMult, float duration) {
    m_shrinkScale     = scale;
    m_shrinkSpeedMult = speedMult;
    m_shrinkTimer     = duration;
}

void StickFigure::applyShield(float reduction, float duration, bool reflects) {
    m_damageReduction = reduction;
    m_shieldTimer     = duration;
    m_reflecting      = reflects;
}

void StickFigure::applyHeal(float rate, float duration) {
    m_healRate  = rate;
    m_healTimer = duration;
}

void StickFigure::heal(float amount) {
    m_health = std::min(m_health + amount, m_maxHealth);
}

void StickFigure::applyMindControl(int controller, float duration) {
    m_mindControlled   = true;
    m_mindController   = controller;
    m_mindControlTimer = duration;
}

void StickFigure::breakMindControl() {
    m_mindControlled = false;
    m_mindController = -1;
    m_mindControlTimer = 0.0f;
}

void StickFigure::respawn(float x, float y) {
    m_health = m_maxHealth;
    m_poisonTimer = 0.0f;
    m_slowTimer = 0.0f;
    m_slowFactor = 1.0f;
    m_freezeStacks = 0;
    m_freezeStackDecayTimer = 0.0f;
    m_timeSlowTimer = 0.0f;
    m_stunTimer = 0.0f;
    m_aimAngle = 0.0f;

    const float torsoH = 0.55f * PLAYER_SCALE;
    const float headR  = 0.30f * PLAYER_SCALE;

    b2Rot  zero = b2MakeRot(0.0f);
    b2Vec2 zv   = {0, 0};

    auto reset = [&](b2BodyId body, float cx, float cy) {
        b2Body_SetTransform(body, {cx, cy}, zero);
        b2Body_SetLinearVelocity(body, zv);
        b2Body_SetAngularVelocity(body, 0.0f);
    };

    reset(m_torso, x, y);
    reset(m_head,  x, y + torsoH + headR + 0.03f);

    float S = PLAYER_SCALE;
    reset(m_leftUpperArm,  x - 0.25f*S, y + 0.35f*S);
    reset(m_rightUpperArm, x + 0.25f*S, y + 0.35f*S);
    reset(m_leftForeArm,   x - 0.25f*S, y);
    reset(m_rightForeArm,  x + 0.25f*S, y);
    reset(m_leftUpperLeg,  x - 0.12f*S, y - 0.55f*S);
    reset(m_rightUpperLeg, x + 0.12f*S, y - 0.55f*S);
    reset(m_leftLowerLeg,  x - 0.12f*S, y - 1.05f*S);
    reset(m_rightLowerLeg, x + 0.12f*S, y - 1.05f*S);

    revertToDefaultWeapon();
}

void StickFigure::teleportTo(float x, float y) {
    b2Vec2 curPos = b2Body_GetPosition(m_torso);
    float dx = x - curPos.x;
    float dy = y - curPos.y;

    auto shift = [&](b2BodyId body) {
        b2Vec2 p = b2Body_GetPosition(body);
        b2Body_SetTransform(body, {p.x + dx, p.y + dy}, b2Body_GetRotation(body));
    };

    shift(m_torso);
    shift(m_head);
    shift(m_leftUpperArm);  shift(m_rightUpperArm);
    shift(m_leftForeArm);   shift(m_rightForeArm);
    shift(m_leftUpperLeg);  shift(m_rightUpperLeg);
    shift(m_leftLowerLeg);  shift(m_rightLowerLeg);
}

void StickFigure::update(float dt) {
    if (m_attackCooldown > 0.0f) m_attackCooldown -= dt;
    if (m_attackAnimTimer > 0.0f) m_attackAnimTimer -= dt;
    if (m_damageFlashTimer > 0.0f) m_damageFlashTimer -= dt;
    if (m_teleportCD > 0.0f) m_teleportCD -= dt;
    m_animTime += dt;

    // Decay per-body-part hit flash timers
    for (int i = 0; i < BODY_PART_COUNT; i++) {
        if (m_partFlash[i] > 0.0f) {
            m_partFlash[i] -= dt;
            if (m_partFlash[i] < 0.0f) m_partFlash[i] = 0.0f;
        }
    }

    // Respawn delay
    if (m_waitingToRespawn) {
        m_respawnTimer -= dt;
        if (m_respawnTimer <= 0.0f) {
            m_waitingToRespawn = false;
            respawn(m_pendingRespawnX, m_pendingRespawnY);
        }
        return;
    }

    // Poison
    if (m_poisonTimer > 0.0f) {
        m_poisonTimer -= dt;
        m_poisonTickTimer += dt;
        if (m_poisonTickTimer >= 0.5f) {
            m_poisonTickTimer -= 0.5f;
            m_health -= m_poisonDps * 0.5f;
            if (m_health < 0.0f) m_health = 0.0f;
        }
    }

    // Slow — decay timer, restore speed when done; also dampen fall speed
    if (m_slowTimer > 0.0f) {
        m_slowTimer -= dt;
        if (m_slowTimer <= 0.0f) {
            m_slowFactor = 1.0f;
        } else {
            // Dampen vertical velocity (falling feels slower in time-slowed zones)
            b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
            float maxFall = -8.0f * m_slowFactor;  // cap fall speed based on slow
            if (vel.y < maxFall) {
                b2Body_SetLinearVelocity(m_torso, {vel.x, maxFall});
            }
        }
    }

    // Time slow — completely stops player
    if (m_timeSlowTimer > 0.0f) {
        m_timeSlowTimer -= dt;
        b2Body_SetLinearVelocity(m_torso, {0.0f, 0.0f});
    }

    // Freeze stacks — each stack = 9% speed reduction, decay one stack per ~1 sec
    if (m_freezeStacks > 0) {
        // Apply speed reduction: stacks * 9% (10 stacks = 90%)
        float freezeSlowFactor = 1.0f - (m_freezeStacks * 0.09f);
        m_slowFactor = std::min(m_slowFactor, freezeSlowFactor);
        m_slowTimer = std::max(m_slowTimer, 0.1f);  // keep slow active

        // Decay stacks over time
        m_freezeStackDecayTimer -= dt;
        if (m_freezeStackDecayTimer <= 0.0f) {
            m_freezeStacks--;
            m_freezeStackDecayTimer = 1.0f;  // 1 second per stack decay
        }
    }

    // Stun
    if (m_stunTimer > 0.0f) {
        m_stunTimer -= dt;
        if (m_stunTimer < 0.0f) m_stunTimer = 0.0f;
    }

    // Shrink — restore when done
    if (m_shrinkTimer > 0.0f) {
        m_shrinkTimer -= dt;
        if (m_shrinkTimer <= 0.0f) {
            m_shrinkScale = 1.0f;
            m_shrinkSpeedMult = 1.0f;
        }
    }

    // Shield
    if (m_shieldTimer > 0.0f) {
        m_shieldTimer -= dt;
        if (m_shieldTimer <= 0.0f) {
            m_damageReduction = 0.0f;
            m_reflecting = false;
        }
    }

    // Heal over time
    if (m_healTimer > 0.0f) {
        m_healTimer -= dt;
        heal(m_healRate * dt);
    }

    // Mind control countdown
    if (m_mindControlled) {
        m_mindControlTimer -= dt;
        if (m_mindControlTimer <= 0.0f) breakMindControl();
    }

    // ── PROCEDURAL LIMB ANIMATION WITH RAGDOLL WOBBLE ─────────────
    b2Vec2 torsoPos = b2Body_GetPosition(m_torso);
    float  tx = torsoPos.x;
    float  ty = torsoPos.y;
    float  t  = m_animTime;

    b2Vec2 vel   = b2Body_GetLinearVelocity(m_torso);
    float  hspd  = std::abs(vel.x);
    bool   onGnd = isOnGround();
    bool   moving = hspd > 0.3f;
    bool   jumping = !onGnd;
    float  dir = static_cast<float>(m_facingDir);

    float phase = t * (3.5f + hspd * 0.5f);

    // Ragdoll wobble — increases with speed, airborne, and damage
    float wobble = std::min(hspd * 0.015f, 0.15f);
    if (jumping) wobble += 0.12f;
    if (m_damageFlashTimer > 0.0f) wobble += 0.3f;
    float wobX = std::sin(t * 7.3f) * wobble;
    float wobY = std::cos(t * 5.7f) * wobble * 0.5f;

    const float torsoH  = 0.55f * PLAYER_SCALE;
    const float torsoW  = 0.20f * PLAYER_SCALE;
    const float uArmLen = 0.38f * PLAYER_SCALE;
    const float lArmLen = 0.34f * PLAYER_SCALE;
    const float uLegLen = 0.40f * PLAYER_SCALE;
    const float lLegLen = 0.38f * PLAYER_SCALE;

    auto placeLimb = [](b2BodyId body, float pivX, float pivY,
                        float angleRad, float halfLen) {
        float cx = pivX + std::sin(angleRad) * halfLen;
        float cy = pivY - std::cos(angleRad) * halfLen;
        b2Rot rot = b2MakeRot(angleRad);
        b2Body_SetTransform(body, {cx, cy}, rot);
        b2Body_SetLinearVelocity(body,  {0,0});
        b2Body_SetAngularVelocity(body, 0.0f);
    };

    float shoulderY = ty + torsoH * 0.55f;
    float shoulderLx = tx - torsoW;
    float shoulderRx = tx + torsoW;
    float hipY  = ty - torsoH;
    float hipLx = tx - torsoW * 0.5f;
    float hipRx = tx + torsoW * 0.5f;

    // ── LEG ANGLES ──
    float lHipAngle=0, rHipAngle=0, lKneeAngle=0, rKneeAngle=0;

    if (m_timeSlowTimer > 0.0f) {
        // Frozen
    } else if (moving && onGnd) {
        float swing = std::sin(phase);
        lHipAngle  =  swing * 0.30f + wobX;
        rHipAngle  = -swing * 0.30f - wobX;
        lKneeAngle = std::max(0.f, -swing * 0.5f) + std::abs(wobY);
        rKneeAngle = std::max(0.f,  swing * 0.5f) + std::abs(wobY);
    } else if (jumping) {
        float tuck = (vel.y > 0) ? 0.5f : 0.15f;
        lHipAngle = tuck + wobX; rHipAngle = tuck - wobX;
        lKneeAngle = rKneeAngle = (vel.y > 0) ? 0.8f : 0.2f;
        if (vel.y < -5.0f) {
            float flail = std::sin(t * 8.0f) * 0.3f;
            lHipAngle += flail; rHipAngle -= flail;
            lKneeAngle += std::abs(flail) * 0.5f;
            rKneeAngle += std::abs(flail) * 0.5f;
        }
    } else {
        float sway = std::sin(t * 1.3f) * 0.08f;
        lHipAngle = sway; rHipAngle = -sway;
    }

    placeLimb(m_leftUpperLeg,  hipLx, hipY, lHipAngle, uLegLen);
    placeLimb(m_rightUpperLeg, hipRx, hipY, rHipAngle, uLegLen);

    float lKneePivX = hipLx + std::sin(lHipAngle) * uLegLen * 2.f;
    float lKneePivY = hipY  - std::cos(lHipAngle) * uLegLen * 2.f;
    float rKneePivX = hipRx + std::sin(rHipAngle) * uLegLen * 2.f;
    float rKneePivY = hipY  - std::cos(rHipAngle) * uLegLen * 2.f;

    placeLimb(m_leftLowerLeg,  lKneePivX, lKneePivY,  lHipAngle + lKneeAngle, lLegLen);
    placeLimb(m_rightLowerLeg, rKneePivX, rKneePivY,  rHipAngle + rKneeAngle, lLegLen);

    // ── ARM ANGLES ──
    float lShoulderAngle=0, rShoulderAngle=0, lElbowAngle=0, rElbowAngle=0;

    if (m_timeSlowTimer > 0.0f) {
        // Frozen
    } else if (m_attackAnimTimer > 0.0f) {
        float prog = 1.0f - (m_attackAnimTimer / 0.2f);
        float punch = dir * std::sin(prog * 3.14159f) * 1.0f;
        if (dir > 0) {
            rShoulderAngle = punch; rElbowAngle = punch * 0.5f;
            lShoulderAngle = -0.3f; lElbowAngle = 0.1f;
        } else {
            lShoulderAngle = punch; lElbowAngle = punch * 0.5f;
            rShoulderAngle = 0.3f;  rElbowAngle = 0.1f;
        }
    } else if (moving && onGnd) {
        float swing = std::sin(phase + 3.14159f);
        lShoulderAngle =  swing * 0.55f + wobX * 1.5f;
        rShoulderAngle = -swing * 0.55f - wobX * 1.5f;
        lElbowAngle = std::max(0.f, -swing * 0.5f);
        rElbowAngle = std::max(0.f,  swing * 0.5f);
    } else if (jumping) {
        lShoulderAngle = -0.7f + wobX * 2.0f;
        rShoulderAngle =  0.7f - wobX * 2.0f;
        lElbowAngle = rElbowAngle = 0.2f;
        if (vel.y < -5.0f) {
            float flail = std::cos(t * 9.0f) * 0.4f;
            lShoulderAngle += flail; rShoulderAngle -= flail;
        }
    } else {
        float breathe = std::sin(t * 2.0f) * 0.07f;
        lShoulderAngle = breathe; rShoulderAngle = -breathe;
        lElbowAngle = rElbowAngle = 0.05f;
    }

    placeLimb(m_leftUpperArm,  shoulderLx, shoulderY, lShoulderAngle, uArmLen);
    placeLimb(m_rightUpperArm, shoulderRx, shoulderY, rShoulderAngle, uArmLen);

    float lElbPivX = shoulderLx + std::sin(lShoulderAngle) * uArmLen * 2.f;
    float lElbPivY = shoulderY  - std::cos(lShoulderAngle) * uArmLen * 2.f;
    float rElbPivX = shoulderRx + std::sin(rShoulderAngle) * uArmLen * 2.f;
    float rElbPivY = shoulderY  - std::cos(rShoulderAngle) * uArmLen * 2.f;

    placeLimb(m_leftForeArm,  lElbPivX, lElbPivY,  lShoulderAngle + lElbowAngle, lArmLen);
    placeLimb(m_rightForeArm, rElbPivX, rElbPivY,  rShoulderAngle + rElbowAngle, lArmLen);
}

void StickFigure::startRespawnTimer(float delay, float x, float y) {
    m_waitingToRespawn = true;
    m_respawnTimer = delay;
    m_pendingRespawnX = x;
    m_pendingRespawnY = y;
    // Move all bodies offscreen
    auto hide = [](b2BodyId body) {
        b2Vec2 p = b2Body_GetPosition(body);
        b2Body_SetLinearVelocity(body, {0,0});
        b2Body_SetTransform(body, {p.x, -100.0f}, b2MakeRot(0.0f));
    };
    hide(m_torso); hide(m_head);
    hide(m_leftUpperArm); hide(m_rightUpperArm);
    hide(m_leftForeArm);  hide(m_rightForeArm);
    hide(m_leftUpperLeg); hide(m_rightUpperLeg);
    hide(m_leftLowerLeg); hide(m_rightLowerLeg);
}

b2Vec2 StickFigure::getPosition() const { return b2Body_GetPosition(m_torso); }

// ── Per-body-part hitbox system ─────────────────────────────────

b2Vec2 StickFigure::getBodyPartPosition(BodyPart part) const {
    switch (part) {
        case BodyPart::Head:           return b2Body_GetPosition(m_head);
        case BodyPart::Torso:          return b2Body_GetPosition(m_torso);
        case BodyPart::LeftUpperArm:   return b2Body_GetPosition(m_leftUpperArm);
        case BodyPart::RightUpperArm:  return b2Body_GetPosition(m_rightUpperArm);
        case BodyPart::LeftForeArm:    return b2Body_GetPosition(m_leftForeArm);
        case BodyPart::RightForeArm:   return b2Body_GetPosition(m_rightForeArm);
        case BodyPart::LeftUpperLeg:   return b2Body_GetPosition(m_leftUpperLeg);
        case BodyPart::RightUpperLeg:  return b2Body_GetPosition(m_rightUpperLeg);
        case BodyPart::LeftLowerLeg:   return b2Body_GetPosition(m_leftLowerLeg);
        case BodyPart::RightLowerLeg:  return b2Body_GetPosition(m_rightLowerLeg);
        default:                       return b2Body_GetPosition(m_torso);
    }
}

float StickFigure::getBodyPartRadius(BodyPart part) const {
    // Generous hit radii — unscaled so they cover animal character visuals
    // which draw larger than the stick figure skeleton.
    switch (part) {
        case BodyPart::Head:           return 0.55f;
        case BodyPart::Torso:          return 0.85f;
        case BodyPart::LeftUpperArm:
        case BodyPart::RightUpperArm:  return 0.50f;
        case BodyPart::LeftForeArm:
        case BodyPart::RightForeArm:   return 0.45f;
        case BodyPart::LeftUpperLeg:
        case BodyPart::RightUpperLeg:  return 0.55f;
        case BodyPart::LeftLowerLeg:
        case BodyPart::RightLowerLeg:  return 0.50f;
        default:                       return 0.60f;
    }
}

HitResult StickFigure::checkHit(float wx, float wy, float maxRadius) const {
    HitResult best;
    // Check all body parts; pick the closest one within its hit radius
    for (int i = 0; i < BODY_PART_COUNT; i++) {
        BodyPart part = static_cast<BodyPart>(i);
        b2Vec2 partPos = getBodyPartPosition(part);
        float dx = wx - partPos.x;
        float dy = wy - partPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        float hitR = getBodyPartRadius(part);
        // Use min of the part's own radius and the caller's maxRadius
        if (dist < hitR && dist < maxRadius && dist < best.distance) {
            best.hit = true;
            best.part = part;
            best.distance = dist;
            best.partPos = partPos;
        }
    }
    return best;
}

void StickFigure::takeDamageAt(BodyPart part, float baseDamage, float kbX, float kbY) {
    float mult = bodyPartDamageMultiplier(part);
    float dmg = baseDamage * mult * (1.0f - m_damageReduction);
    m_health -= dmg;
    if (m_health < 0.0f) m_health = 0.0f;
    m_damageFlashTimer = 0.15f;
    if (static_cast<int>(part) >= 0 && static_cast<int>(part) < BODY_PART_COUNT)
        m_partFlash[static_cast<int>(part)] = 0.2f;
    if (m_timeSlowTimer <= 0.0f)
        b2Body_ApplyLinearImpulseToCenter(m_torso, {kbX * mult, kbY * mult}, true);
    if (m_mindControlled) breakMindControl();
}

void StickFigure::draw(sf::RenderTarget& target) const {
    if (!isAlive()) return;

    switch (m_charType) {
        case CharacterType::Cat:       drawCat(target); break;
        case CharacterType::Cobra:     drawCobra(target); break;
        case CharacterType::Unicorn:   drawUnicorn(target); break;
        case CharacterType::Crocodile: drawCrocodile(target); break;
        case CharacterType::StickLady: drawStickLady(target); break;
        default:                       drawStick(target); break;
    }

    if (m_attackAnimTimer > 0.0f) drawAttackEffect(target);

    // Draw aim indicator for ranged weapons
    if (m_weapon.type != WeaponType::Melee) drawAimIndicator(target);

    drawWeapon(target);
    drawStatusEffects(target);
    drawHitFlashes(target);

    // Poison effect - green particles
    if (m_poisonTimer > 0.0f) {
        sf::Vector2f pos = toScreen(getPosition());
        for (int i = 0; i < 3; i++) {
            float offset = static_cast<float>(i) * 8.0f - 8.0f;
            sf::CircleShape dot(2.0f);
            dot.setOrigin({2.0f, 2.0f});
            dot.setPosition({pos.x + offset, pos.y - 25.0f - static_cast<float>(i) * 4.0f});
            dot.setFillColor(sf::Color(0, 200, 0, 180));
            target.draw(dot);
        }
    }
}

void StickFigure::drawStick(sf::RenderTarget& target) const {
    sf::Color dc    = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    sf::Color jc    = sf::Color(
        static_cast<uint8_t>(std::min(255, (int)dc.r + 60)),
        static_cast<uint8_t>(std::min(255, (int)dc.g + 60)),
        static_cast<uint8_t>(std::min(255, (int)dc.b + 60)));

    auto drawSeg = [&](b2Vec2 a, b2Vec2 bv, sf::Color col, float thick) {
        sf::Vector2f sa = toScreen(a);
        sf::Vector2f sb = toScreen(bv);
        float dx = sb.x-sa.x, dy = sb.y-sa.y;
        float len = std::sqrt(dx*dx+dy*dy);
        if (len < 0.5f) return;
        sf::RectangleShape seg({len, thick});
        seg.setOrigin({0.f, thick*0.5f});
        seg.setPosition(sa);
        seg.setRotation(sf::degrees(std::atan2(dy,dx)*57.2958f));
        seg.setFillColor(col);
        target.draw(seg);
    };
    auto drawDot = [&](b2Vec2 pos, float r, sf::Color col) {
        sf::Vector2f sp = toScreen(pos);
        sf::CircleShape c(r); c.setOrigin({r,r}); c.setPosition(sp);
        c.setFillColor(col); target.draw(c);
    };

    // Anatomy — must match animation
    const float torsoH  = 0.55f * PLAYER_SCALE;
    const float torsoW  = 0.20f * PLAYER_SCALE;
    const float uArmLen = 0.38f * PLAYER_SCALE;
    const float lArmLen = 0.34f * PLAYER_SCALE;
    const float uLegLen = 0.40f * PLAYER_SCALE;
    const float lLegLen = 0.38f * PLAYER_SCALE;

    b2Vec2 tp = b2Body_GetPosition(m_torso);
    b2Vec2 hp = b2Body_GetPosition(m_head);
    b2Vec2 neck   = {tp.x,          tp.y + torsoH};
    b2Vec2 spine  = {tp.x,          tp.y - torsoH};
    b2Vec2 shoulL = {tp.x - torsoW, tp.y + torsoH*0.55f};
    b2Vec2 shoulR = {tp.x + torsoW, tp.y + torsoH*0.55f};
    b2Vec2 hipL   = {tp.x - torsoW*0.5f, tp.y - torsoH};
    b2Vec2 hipR   = {tp.x + torsoW*0.5f, tp.y - torsoH};

    // Tip of a limb = centre shifted by halfLen along local -Y
    // b2Rot: local (0,-hl) → world = (sin*hl, -cos*hl) relative to centre
    auto limbTip = [](b2BodyId body, float hl) -> b2Vec2 {
        b2Vec2 c = b2Body_GetPosition(body);
        b2Rot  r = b2Body_GetRotation(body);
        return {c.x + r.s*hl,  c.y - r.c*hl};
    };

    b2Vec2 elbowL = limbTip(m_leftUpperArm,  uArmLen);
    b2Vec2 elbowR = limbTip(m_rightUpperArm, uArmLen);
    b2Vec2 handL  = limbTip(m_leftForeArm,   lArmLen);
    b2Vec2 handR  = limbTip(m_rightForeArm,  lArmLen);
    b2Vec2 kneeL  = limbTip(m_leftUpperLeg,  uLegLen);
    b2Vec2 kneeR  = limbTip(m_rightUpperLeg, uLegLen);
    b2Vec2 footL  = limbTip(m_leftLowerLeg,  lLegLen);
    b2Vec2 footR  = limbTip(m_rightLowerLeg, lLegLen);


    sf::Color backCol(
        static_cast<uint8_t>(dc.r*0.45f),
        static_cast<uint8_t>(dc.g*0.45f),
        static_cast<uint8_t>(dc.b*0.45f));
    sf::Color legCol(
        static_cast<uint8_t>(dc.r*0.78f),
        static_cast<uint8_t>(dc.g*0.78f),
        static_cast<uint8_t>(dc.b*0.78f));

    // Back limbs
    if (m_facingDir > 0) {
        drawSeg(shoulL, elbowL, backCol, 3.5f);
        drawSeg(elbowL, handL,  backCol, 3.0f);
        drawSeg(hipL,   kneeL,  backCol, 3.5f);
        drawSeg(kneeL,  footL,  backCol, 3.0f);
    } else {
        drawSeg(shoulR, elbowR, backCol, 3.5f);
        drawSeg(elbowR, handR,  backCol, 3.0f);
        drawSeg(hipR,   kneeR,  backCol, 3.5f);
        drawSeg(kneeR,  footR,  backCol, 3.0f);
    }

    // Torso
    drawSeg(neck, spine,    dc, 5.5f);
    drawSeg(shoulL, shoulR, dc, 4.5f);

    // Head
    float hr = m_config.headRadius * PPM;
    sf::CircleShape hs(hr); hs.setOrigin({hr,hr});
    hs.setPosition(toScreen(hp));
    hs.setFillColor(sf::Color::Transparent);
    hs.setOutlineColor(dc); hs.setOutlineThickness(3.0f);
    target.draw(hs);
    // Eye
    b2Vec2 eyeW = {hp.x + static_cast<float>(m_facingDir)*m_config.headRadius*0.5f,
                   hp.y + m_config.headRadius*0.1f};
    sf::Vector2f eyeS = toScreen(eyeW);
    sf::CircleShape eye(3.0f); eye.setOrigin({3.f,3.f}); eye.setPosition(eyeS);
    eye.setFillColor(dc); target.draw(eye);

    // Front limbs
    if (m_facingDir > 0) {
        drawSeg(shoulR, elbowR, dc,     4.5f);
        drawSeg(elbowR, handR,  dc,     4.0f);
        drawSeg(hipR,   kneeR,  legCol, 4.5f);
        drawSeg(kneeR,  footR,  legCol, 4.0f);
        drawSeg(footR, {footR.x+0.18f, footR.y}, legCol, 3.5f);
    } else {
        drawSeg(shoulL, elbowL, dc,     4.5f);
        drawSeg(elbowL, handL,  dc,     4.0f);
        drawSeg(hipL,   kneeL,  legCol, 4.5f);
        drawSeg(kneeL,  footL,  legCol, 4.0f);
        drawSeg(footL, {footL.x-0.18f, footL.y}, legCol, 3.5f);
    }
}


void StickFigure::drawCat(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);
    const float S = PLAYER_SCALE;

    // Body
    sf::RectangleShape body({28.0f*S, 16.0f*S});
    body.setOrigin({14.0f*S, 8.0f*S}); body.setPosition(c);
    body.setFillColor(dc); body.setOutlineColor(sf::Color::Black); body.setOutlineThickness(1.0f);
    target.draw(body);

    // Head
    sf::CircleShape head(10.0f*S);
    head.setOrigin({10.0f*S, 10.0f*S});
    head.setPosition({c.x + dir * 19.0f*S, c.y - 4.0f*S});
    head.setFillColor(dc); head.setOutlineColor(sf::Color::Black); head.setOutlineThickness(1.0f);
    target.draw(head);

    sf::Vector2f hc = head.getPosition();
    // Ears
    for (float s : {-1.0f, 1.0f}) {
        sf::ConvexShape ear(3);
        ear.setPoint(0, {hc.x + s * 5.0f*S, hc.y - 8.0f*S});
        ear.setPoint(1, {hc.x + s * 2.0f*S, hc.y - 16.0f*S});
        ear.setPoint(2, {hc.x + s * 8.0f*S, hc.y - 13.0f*S});
        ear.setFillColor(dc); ear.setOutlineColor(sf::Color::Black); ear.setOutlineThickness(1.0f);
        target.draw(ear);
    }
    // Eyes
    for (float s : {-1.0f, 1.0f}) {
        sf::CircleShape eye(2.0f*S); eye.setOrigin({2.0f*S, 2.0f*S});
        eye.setPosition({hc.x + dir * 3.0f*S + s * 3.0f*S, hc.y - 2.0f*S});
        eye.setFillColor(sf::Color::Black); target.draw(eye);
    }
    // Nose + Whiskers
    sf::CircleShape nose(1.5f*S); nose.setOrigin({1.5f*S, 1.5f*S});
    nose.setPosition({hc.x + dir * 7.0f*S, hc.y + 1.0f*S});
    nose.setFillColor(sf::Color(200, 100, 100)); target.draw(nose);
    for (float wy : {-1.0f, 0.0f, 1.0f}) {
        sf::VertexArray w(sf::PrimitiveType::Lines, 2);
        w[0] = sf::Vertex{{hc.x + dir * 8.0f*S, hc.y + 1.0f*S + wy * 2.0f*S}, sf::Color::Black};
        w[1] = sf::Vertex{{hc.x + dir * 20.0f*S, hc.y + 1.0f*S + wy * 5.0f*S}, sf::Color::Black};
        target.draw(w);
    }
    // Tail
    sf::VertexArray tail(sf::PrimitiveType::LineStrip, 4);
    float tx = c.x - dir * 14.0f*S;
    tail[0] = sf::Vertex{{tx, c.y}, dc};
    tail[1] = sf::Vertex{{tx - dir * 8.0f*S, c.y - 8.0f*S}, dc};
    tail[2] = sf::Vertex{{tx - dir * 12.0f*S, c.y - 16.0f*S}, dc};
    tail[3] = sf::Vertex{{tx - dir * 8.0f*S, c.y - 22.0f*S}, dc};
    target.draw(tail);
    // Legs
    float maxLegY = c.y + 1.40f * PLAYER_SCALE * PPM;
    for (float lx : {-0.3f, -0.1f, 0.1f, 0.3f}) {
        sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
        leg[0] = sf::Vertex{{c.x + lx * 28.0f*S, c.y + 8.0f*S}, dc};
        leg[1] = sf::Vertex{{c.x + lx * 28.0f*S, std::min(c.y + 18.0f*S, maxLegY)}, dc};
        target.draw(leg);
    }
}

void StickFigure::drawCobra(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);
    float t = m_animTime;
    const float S = PLAYER_SCALE;

    // Velocity-based wiggle speed: faster movement = faster wiggle
    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    float wiggleSpeed = 4.0f + speed * 1.5f;
    float wiggleAmp = (3.0f + speed * 0.8f) * S;

    // --- Coiled body: a spiral of line segments that wiggle ---
    // Draw 2.5 loops of a coil sitting at the base
    constexpr int coilSegs = 28;
    constexpr float coilLoops = 2.5f;
    float coilRadiusX = 14.0f * S;
    float coilRadiusY = 6.0f * S;
    sf::Vector2f coilCenter = {c.x - dir * 2.0f * S, c.y + 6.0f * S};

    // Build coil points with per-segment wiggle
    sf::VertexArray coilLine(sf::PrimitiveType::LineStrip, coilSegs + 1);
    for (int i = 0; i <= coilSegs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(coilSegs);
        float angle = frac * coilLoops * 2.0f * 3.14159f;
        // Shrink radius toward the center to look like a real coil
        float rScale = 1.0f - frac * 0.3f;
        float wiggle = std::sin(t * wiggleSpeed + frac * 8.0f) * wiggleAmp * (1.0f - frac * 0.5f);
        float px = coilCenter.x + std::cos(angle) * coilRadiusX * rScale + wiggle * 0.3f;
        float py = coilCenter.y + std::sin(angle) * coilRadiusY * rScale + wiggle * 0.15f;
        // Stack coils vertically with slight offset
        py -= frac * 8.0f * S;

        // Color: darken toward tail end
        uint8_t fade = static_cast<uint8_t>(255 - static_cast<int>(frac * 80.0f));
        sf::Color segColor = {
            static_cast<uint8_t>(dc.r * fade / 255),
            static_cast<uint8_t>(dc.g * fade / 255),
            static_cast<uint8_t>(dc.b * fade / 255),
            dc.a
        };
        coilLine[i] = sf::Vertex{{px, py}, segColor};
    }
    target.draw(coilLine);

    // Draw thicker coil by offsetting and drawing again
    sf::VertexArray coilLine2(sf::PrimitiveType::LineStrip, coilSegs + 1);
    for (int i = 0; i <= coilSegs; i++) {
        coilLine2[i] = coilLine[i];
        coilLine2[i].position.y += 1.5f;
    }
    target.draw(coilLine2);
    sf::VertexArray coilLine3(sf::PrimitiveType::LineStrip, coilSegs + 1);
    for (int i = 0; i <= coilSegs; i++) {
        coilLine3[i] = coilLine[i];
        coilLine3[i].position.y -= 1.5f;
    }
    target.draw(coilLine3);

    // --- Neck: segmented line rising up from coil with S-curve wiggle ---
    constexpr int neckSegs = 12;
    float neckHeight = 38.0f * S;
    sf::Vector2f neckBase = {c.x, c.y + 2.0f * S};

    sf::VertexArray neckLine(sf::PrimitiveType::LineStrip, neckSegs + 1);
    sf::Vector2f neckTop;
    for (int i = 0; i <= neckSegs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(neckSegs);
        // S-curve wiggle that travels up the neck
        float sway = std::sin(t * wiggleSpeed * 0.8f + frac * 3.5f) * wiggleAmp * 0.6f * (1.0f - frac * 0.3f);
        float px = neckBase.x + dir * frac * 6.0f * S + sway;
        float py = neckBase.y - frac * neckHeight;
        neckLine[i] = sf::Vertex{{px, py}, dc};
        if (i == neckSegs) neckTop = {px, py};
    }
    target.draw(neckLine);

    // Thicken the neck with parallel lines, tapering toward the head
    for (float offset : {-2.0f, 2.0f, -1.0f, 1.0f}) {
        sf::VertexArray neckThick(sf::PrimitiveType::LineStrip, neckSegs + 1);
        for (int i = 0; i <= neckSegs; i++) {
            float frac = static_cast<float>(i) / static_cast<float>(neckSegs);
            float thickness = (1.0f - frac * 0.4f); // taper toward head
            neckThick[i] = neckLine[i];
            neckThick[i].position.x += offset * thickness;
        }
        target.draw(neckThick);
    }

    // --- Hood: flared shape behind the head, wiggles slightly ---
    float hoodSway = std::sin(t * wiggleSpeed * 0.6f) * 2.0f * S;
    float hx = neckTop.x + dir * 2.0f * S + hoodSway * 0.3f;
    float hy = neckTop.y;

    sf::ConvexShape hood(7);
    hood.setPoint(0, {hx - 14.0f*S,             hy + 6.0f*S});
    hood.setPoint(1, {hx - 12.0f*S + hoodSway,  hy - 4.0f*S});
    hood.setPoint(2, {hx - 6.0f*S,              hy - 10.0f*S});
    hood.setPoint(3, {hx,                     hy - 13.0f*S});
    hood.setPoint(4, {hx + 6.0f*S,              hy - 10.0f});
    hood.setPoint(5, {hx + 12.0f*S - hoodSway,  hy - 4.0f});
    hood.setPoint(6, {hx + 14.0f*S,             hy + 6.0f});
    hood.setFillColor(dc);
    hood.setOutlineColor(sf::Color::Black);
    hood.setOutlineThickness(1.0f);
    target.draw(hood);

    // Hood pattern (lighter belly stripe)
    sf::ConvexShape hoodBelly(5);
    sf::Color bellyColor = {
        static_cast<uint8_t>(std::min(255, dc.r + 60)),
        static_cast<uint8_t>(std::min(255, dc.g + 60)),
        static_cast<uint8_t>(std::min(255, dc.b + 20)),
        dc.a
    };
    hoodBelly.setPoint(0, {hx - 6.0f, hy + 4.0f});
    hoodBelly.setPoint(1, {hx - 4.0f, hy - 3.0f});
    hoodBelly.setPoint(2, {hx,        hy - 6.0f});
    hoodBelly.setPoint(3, {hx + 4.0f, hy - 3.0f});
    hoodBelly.setPoint(4, {hx + 6.0f, hy + 4.0f});
    hoodBelly.setFillColor(bellyColor);
    target.draw(hoodBelly);

    // --- Head ---
    sf::CircleShape head(6.0f*S);
    head.setOrigin({6.0f*S, 6.0f*S});
    head.setPosition({hx + dir * 2.0f*S, hy - 11.0f*S});
    head.setFillColor(dc);
    head.setOutlineColor(sf::Color::Black);
    head.setOutlineThickness(1.0f);
    target.draw(head);

    // Eyes (menacing, red with slit pupils)
    for (float s : {-1.0f, 1.0f}) {
        sf::CircleShape eye(2.0f); eye.setOrigin({2.0f, 2.0f});
        eye.setPosition({hx + dir * 2.0f + s * 3.5f, hy - 12.0f});
        eye.setFillColor(sf::Color::Yellow);
        target.draw(eye);
        // Slit pupil
        sf::RectangleShape pupil({1.0f, 3.0f});
        pupil.setOrigin({0.5f, 1.5f});
        pupil.setPosition(eye.getPosition());
        pupil.setFillColor(sf::Color::Black);
        target.draw(pupil);
    }

    // --- Tongue: forked, with flicker animation ---
    float tongueFlicker = std::sin(t * 12.0f);
    bool tongueOut = tongueFlicker > -0.3f; // tongue flicks in and out
    if (tongueOut) {
        float tongueLen = 8.0f + tongueFlicker * 4.0f;
        sf::Vector2f tongueStart = {hx + dir * 8.0f, hy - 10.0f};
        float forkAngle = 0.25f + tongueFlicker * 0.1f;

        sf::VertexArray tongue(sf::PrimitiveType::LineStrip, 3);
        tongue[0] = sf::Vertex{tongueStart, sf::Color::Red};
        tongue[1] = sf::Vertex{{tongueStart.x + dir * tongueLen, tongueStart.y}, sf::Color::Red};
        tongue[2] = sf::Vertex{{tongueStart.x + dir * (tongueLen + 4.0f),
                                tongueStart.y - forkAngle * 8.0f}, sf::Color::Red};
        target.draw(tongue);

        sf::VertexArray tongue2(sf::PrimitiveType::Lines, 2);
        tongue2[0] = sf::Vertex{{tongueStart.x + dir * tongueLen, tongueStart.y}, sf::Color::Red};
        tongue2[1] = sf::Vertex{{tongueStart.x + dir * (tongueLen + 4.0f),
                                 tongueStart.y + forkAngle * 8.0f}, sf::Color::Red};
        target.draw(tongue2);
    }

    // --- Scale pattern along the neck (diamond shapes) ---
    for (int i = 2; i <= neckSegs - 2; i += 2) {
        sf::Vector2f pos = neckLine[i].position;
        float frac = static_cast<float>(i) / static_cast<float>(neckSegs);
        float scaleSize = 2.5f * (1.0f - frac * 0.3f);
        sf::CircleShape scale(scaleSize, 4);
        scale.setOrigin({scaleSize, scaleSize});
        scale.setPosition(pos);
        sf::Color scaleColor = {
            static_cast<uint8_t>(std::min(255, static_cast<int>(dc.r) + 40)),
            static_cast<uint8_t>(std::min(255, static_cast<int>(dc.g) + 20)),
            dc.b, dc.a
        };
        scale.setFillColor(scaleColor);
        target.draw(scale);
    }
}

void StickFigure::drawUnicorn(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);
    float t = m_animTime;
    const float S = PLAYER_SCALE;

    // Mane shimmer colors
    auto rainbow = [&](float phase) -> sf::Color {
        float r = std::sin(t * 2.0f + phase) * 0.5f + 0.5f;
        float g = std::sin(t * 2.0f + phase + 2.094f) * 0.5f + 0.5f;
        float b = std::sin(t * 2.0f + phase + 4.189f) * 0.5f + 0.5f;
        return {static_cast<uint8_t>(r * 255), static_cast<uint8_t>(g * 255),
                static_cast<uint8_t>(b * 255), 255};
    };

    // --- Body (barrel) ---
    sf::RectangleShape body({36.0f*S, 20.0f*S});
    body.setOrigin({18.0f*S, 10.0f*S});
    body.setPosition(c);
    body.setFillColor(dc);
    body.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    body.setOutlineThickness(1.0f);
    target.draw(body);

    // --- Legs (4 legs with slight animation) ---
    float gallop = std::sin(t * 8.0f);
    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x);
    float legAnim = speed > 1.0f ? gallop * 6.0f * S : 0.0f;
    float legOffsets[4] = {-0.35f, -0.12f, 0.12f, 0.35f};
    float legPhases[4] = {0.0f, 3.14159f, 0.0f, 3.14159f}; // diagonal pairs
    float maxLegY = c.y + 1.40f * PLAYER_SCALE * PPM;
    for (int i = 0; i < 4; i++) {
        float lx = c.x + legOffsets[i] * 36.0f * S;
        float anim = speed > 1.0f ? std::sin(t * 8.0f + legPhases[i]) * 6.0f : 0.0f;
        float footY = std::min(c.y + 24.0f * S, maxLegY);
        sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
        leg[0] = sf::Vertex{{lx, c.y + 10.0f * S}, dc};
        leg[1] = sf::Vertex{{lx + anim * 0.3f, footY}, dc};
        target.draw(leg);
        // Hooves
        sf::CircleShape hoof(2.5f);
        hoof.setOrigin({2.5f, 2.5f});
        hoof.setPosition({lx + anim * 0.3f, std::min(c.y + 25.0f * S, maxLegY)});
        hoof.setFillColor(sf::Color(60, 60, 60));
        target.draw(hoof);
    }

    // --- Neck (angled up from front of body) ---
    float neckBaseX = c.x + dir * 18.0f * S;
    float neckBaseY = c.y - 6.0f * S;
    float neckTopX = neckBaseX + dir * 12.0f * S;
    float neckTopY = neckBaseY - 22.0f * S;
    // Thick neck with convex shape
    sf::ConvexShape neck(4);
    neck.setPoint(0, {neckBaseX - dir * 5.0f * S, neckBaseY});
    neck.setPoint(1, {neckBaseX + dir * 3.0f * S, neckBaseY});
    neck.setPoint(2, {neckTopX + dir * 2.0f * S, neckTopY + 4.0f * S});
    neck.setPoint(3, {neckTopX - dir * 4.0f * S, neckTopY + 4.0f * S});
    neck.setFillColor(dc);
    target.draw(neck);

    // --- Head (elongated oval) ---
    float headX = neckTopX + dir * 6.0f * S;
    float headY = neckTopY;
    sf::CircleShape headShape(8.0f * S);
    headShape.setScale({1.4f, 1.0f});
    headShape.setOrigin({8.0f * S, 8.0f * S});
    headShape.setPosition({headX, headY});
    headShape.setFillColor(dc);
    headShape.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    headShape.setOutlineThickness(1.0f);
    target.draw(headShape);

    // Snout extension
    sf::ConvexShape snout(4);
    snout.setPoint(0, {headX + dir * 6.0f, headY - 3.0f});
    snout.setPoint(1, {headX + dir * 16.0f, headY - 1.0f});
    snout.setPoint(2, {headX + dir * 16.0f, headY + 3.0f});
    snout.setPoint(3, {headX + dir * 6.0f, headY + 5.0f});
    snout.setFillColor(dc);
    snout.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    snout.setOutlineThickness(1.0f);
    target.draw(snout);

    // Eye
    sf::CircleShape eye(2.5f);
    eye.setOrigin({2.5f, 2.5f});
    eye.setPosition({headX + dir * 4.0f, headY - 2.0f});
    eye.setFillColor(sf::Color(50, 0, 100));
    target.draw(eye);
    // Eye highlight
    sf::CircleShape eyeHighlight(1.0f);
    eyeHighlight.setOrigin({1.0f, 1.0f});
    eyeHighlight.setPosition({headX + dir * 4.5f, headY - 3.0f});
    eyeHighlight.setFillColor(sf::Color::White);
    target.draw(eyeHighlight);

    // Nostril
    sf::CircleShape nostril(1.0f);
    nostril.setOrigin({1.0f, 1.0f});
    nostril.setPosition({headX + dir * 14.0f, headY + 1.0f});
    nostril.setFillColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    target.draw(nostril);

    // --- Ear ---
    sf::ConvexShape ear(3);
    ear.setPoint(0, {headX - dir * 2.0f, headY - 6.0f});
    ear.setPoint(1, {headX,              headY - 16.0f});
    ear.setPoint(2, {headX + dir * 4.0f, headY - 7.0f});
    ear.setFillColor(dc);
    ear.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    ear.setOutlineThickness(1.0f);
    target.draw(ear);

    // --- HORN (magical, spiraling, with rainbow glow) ---
    float hornBaseX = headX + dir * 2.0f;
    float hornBaseY = headY - 12.0f;
    float hornLen = 22.0f;
    float hornAngle = -1.2f; // angled forward-up

    // Spiral lines for the horn
    constexpr int hornSegs = 16;
    sf::VertexArray hornLine(sf::PrimitiveType::LineStrip, hornSegs + 1);
    for (int i = 0; i <= hornSegs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(hornSegs);
        float spiralOffset = std::sin(frac * 12.0f + t * 3.0f) * (3.0f - frac * 2.5f);
        float hx = hornBaseX + dir * std::cos(hornAngle) * hornLen * frac;
        float hy = hornBaseY + std::sin(hornAngle) * hornLen * frac + spiralOffset;
        sf::Color hc = rainbow(frac * 6.28f);
        // Brighten toward tip
        hc.r = static_cast<uint8_t>(std::min(255, hc.r + static_cast<int>(frac * 100)));
        hc.g = static_cast<uint8_t>(std::min(255, hc.g + static_cast<int>(frac * 100)));
        hc.b = static_cast<uint8_t>(std::min(255, hc.b + static_cast<int>(frac * 100)));
        hornLine[i] = sf::Vertex{{hx, hy}, hc};
    }
    target.draw(hornLine);
    // Thicken horn with parallel offsets
    for (float off : {-1.5f, 1.5f, -0.7f, 0.7f}) {
        sf::VertexArray thick(sf::PrimitiveType::LineStrip, hornSegs + 1);
        for (int i = 0; i <= hornSegs; i++) {
            float frac = static_cast<float>(i) / static_cast<float>(hornSegs);
            float taper = 1.0f - frac * 0.8f;
            thick[i] = hornLine[i];
            thick[i].position.x += off * taper * 0.3f;
            thick[i].position.y += off * taper;
        }
        target.draw(thick);
    }

    // Horn tip sparkle
    {
        float sparkle = std::sin(t * 10.0f) * 0.5f + 0.5f;
        sf::Vector2f tip = hornLine[hornSegs].position;
        sf::CircleShape spark(2.0f + sparkle * 2.0f);
        spark.setOrigin({spark.getRadius(), spark.getRadius()});
        spark.setPosition(tip);
        spark.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(150 + sparkle * 105)));
        target.draw(spark);
        // Smaller colored spark
        sf::CircleShape spark2(1.0f + sparkle * 1.5f);
        spark2.setOrigin({spark2.getRadius(), spark2.getRadius()});
        spark2.setPosition(tip);
        spark2.setFillColor(rainbow(t * 3.0f));
        target.draw(spark2);
    }

    // --- Mane (rainbow flowing hair along neck) ---
    constexpr int maneStrands = 7;
    for (int s = 0; s < maneStrands; s++) {
        float sf2 = static_cast<float>(s) / static_cast<float>(maneStrands);
        float startX = neckBaseX + (neckTopX - neckBaseX) * sf2;
        float startY = neckBaseY + (neckTopY - neckBaseY) * sf2 - 2.0f;

        sf::VertexArray strand(sf::PrimitiveType::LineStrip, 5);
        for (int j = 0; j < 5; j++) {
            float jf = static_cast<float>(j) / 4.0f;
            float wave = std::sin(t * 3.0f + sf2 * 4.0f + jf * 3.0f) * (4.0f + jf * 6.0f);
            float px = startX - dir * jf * 12.0f + wave * 0.3f;
            float py = startY - jf * 4.0f + wave;
            strand[j] = sf::Vertex{{px, py}, rainbow(sf2 * 6.28f + jf * 2.0f)};
        }
        target.draw(strand);
    }

    // --- Tail (flowing rainbow) ---
    float tailBaseX = c.x - dir * 18.0f;
    float tailBaseY = c.y - 2.0f;
    constexpr int tailStrands = 5;
    for (int s = 0; s < tailStrands; s++) {
        float sf2 = static_cast<float>(s) / static_cast<float>(tailStrands);
        sf::VertexArray strand(sf::PrimitiveType::LineStrip, 6);
        for (int j = 0; j < 6; j++) {
            float jf = static_cast<float>(j) / 5.0f;
            float wave = std::sin(t * 2.5f + sf2 * 3.0f + jf * 4.0f) * (5.0f + jf * 8.0f);
            float px = tailBaseX - dir * jf * 25.0f;
            float py = tailBaseY + sf2 * 4.0f - 2.0f + wave;
            strand[j] = sf::Vertex{{px, py}, rainbow(sf2 * 6.28f + jf * 1.5f + 3.0f)};
        }
        target.draw(strand);
    }

    // --- Magical particles around the unicorn (subtle sparkles) ---
    for (int i = 0; i < 4; i++) {
        float phase = static_cast<float>(i) * 1.57f;
        float px = c.x + std::cos(t * 1.5f + phase) * 25.0f;
        float py = c.y - 10.0f + std::sin(t * 2.0f + phase) * 15.0f;
        float sz = 1.0f + std::sin(t * 5.0f + phase) * 0.8f;
        sf::CircleShape sparkle(sz, 4); // diamond shape
        sparkle.setOrigin({sz, sz});
        sparkle.setPosition({px, py});
        sparkle.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(80 + std::sin(t * 4.0f + phase) * 60)));
        target.draw(sparkle);
    }
}

void StickFigure::drawCrocodile(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);
    float t = m_animTime;
    const float S = PLAYER_SCALE;
    sf::Color belly(
        static_cast<uint8_t>(std::min(255, dc.r + 40)),
        static_cast<uint8_t>(std::min(255, dc.g + 50)),
        static_cast<uint8_t>(std::min(255, dc.b + 20)));

    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x);

    // --- Tail (thick, segmented, swishing) ---
    float tailBaseX = c.x - dir * 20.0f * S;
    float tailBaseY = c.y + 2.0f * S;
    constexpr int tailSegs = 10;
    sf::VertexArray tail(sf::PrimitiveType::LineStrip, tailSegs + 1);
    float swish = speed > 1.0f ? std::sin(t * 6.0f) * 8.0f * S : std::sin(t * 1.5f) * 3.0f * S;
    for (int i = 0; i <= tailSegs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(tailSegs);
        float wave = std::sin(t * 3.0f + frac * 4.0f) * swish * frac;
        float tx = tailBaseX - dir * frac * 30.0f * S;
        float ty = tailBaseY + frac * 6.0f * S + wave;
        tail[i] = sf::Vertex{{tx, ty}, dc};
    }
    target.draw(tail);
    // Tail thickness passes
    for (float off : {-2.5f, 2.5f, -1.2f, 1.2f}) {
        sf::VertexArray thick(sf::PrimitiveType::LineStrip, tailSegs + 1);
        for (int i = 0; i <= tailSegs; i++) {
            float frac = static_cast<float>(i) / static_cast<float>(tailSegs);
            float taper = 1.0f - frac * 0.7f;
            thick[i] = tail[i];
            thick[i].position.y += off * taper;
        }
        target.draw(thick);
    }

    // --- Body (long, low rectangle) ---
    sf::ConvexShape body(6);
    body.setPoint(0, {c.x - dir * 20.0f*S, c.y - 8.0f*S});
    body.setPoint(1, {c.x + dir * 12.0f*S, c.y - 10.0f*S});
    body.setPoint(2, {c.x + dir * 20.0f*S, c.y - 6.0f*S});
    body.setPoint(3, {c.x + dir * 20.0f*S, c.y + 8.0f*S});
    body.setPoint(4, {c.x - dir * 10.0f*S, c.y + 10.0f*S});
    body.setPoint(5, {c.x - dir * 20.0f*S, c.y + 6.0f*S});
    body.setFillColor(dc);
    body.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    body.setOutlineThickness(1.0f);
    target.draw(body);

    // Belly stripe
    sf::ConvexShape bellyShape(4);
    bellyShape.setPoint(0, {c.x - dir * 14.0f*S, c.y + 2.0f*S});
    bellyShape.setPoint(1, {c.x + dir * 14.0f*S, c.y + 1.0f*S});
    bellyShape.setPoint(2, {c.x + dir * 12.0f*S, c.y + 8.0f*S});
    bellyShape.setPoint(3, {c.x - dir * 10.0f*S, c.y + 9.0f*S});
    bellyShape.setFillColor(belly);
    target.draw(bellyShape);

    // Scutes (back ridges)
    for (int i = 0; i < 6; i++) {
        float sx = c.x - dir * 14.0f*S + dir * static_cast<float>(i) * 6.0f*S;
        sf::ConvexShape scute(3);
        scute.setPoint(0, {sx - 2.0f*S, c.y - 8.0f*S});
        scute.setPoint(1, {sx, c.y - 13.0f*S});
        scute.setPoint(2, {sx + 2.0f*S, c.y - 8.0f*S});
        scute.setFillColor(sf::Color(dc.r * 3 / 4, dc.g * 3 / 4, dc.b * 3 / 4));
        target.draw(scute);
    }

    // --- Legs (4 stubby legs) ---
    float legAnim = speed > 1.0f ? std::sin(t * 8.0f) * 4.0f * S : 0.0f;
    float legPositions[4] = {-0.30f, -0.10f, 0.15f, 0.35f};
    float legPhases[4] = {0.0f, 3.14159f, 0.0f, 3.14159f};
    float maxLegY = c.y + 1.40f * PLAYER_SCALE * PPM;
    for (int i = 0; i < 4; i++) {
        float lx = c.x + dir * legPositions[i] * 45.0f * S;
        float anim = speed > 1.0f ? std::sin(t * 8.0f + legPhases[i]) * 4.0f : 0.0f;
        float legLen = std::min(14.0f * S, maxLegY - (c.y + 8.0f * S));
        legLen = std::max(legLen, 2.0f);
        sf::RectangleShape leg({4.0f, legLen});
        leg.setOrigin({2.0f, 0.0f});
        leg.setPosition({lx, c.y + 8.0f * S});
        leg.setRotation(sf::degrees(anim));
        leg.setFillColor(dc);
        target.draw(leg);
        // Claws
        float clawY = std::min(c.y + 22.0f * S, maxLegY);
        for (float cx2 : {-1.5f, 0.0f, 1.5f}) {
            sf::CircleShape claw(1.0f);
            claw.setOrigin({1.0f, 1.0f});
            claw.setPosition({lx + cx2, clawY});
            claw.setFillColor(sf::Color(60, 60, 50));
            target.draw(claw);
        }
    }

    // --- Head / Snout (the distinctive long jaw) ---
    float headX = c.x + dir * 20.0f * S;
    float headY = c.y - 4.0f * S;

    // Jaw opening animation when attacking
    float jawOpen = 0.0f;
    if (m_attackAnimTimer > 0.0f) {
        float prog = m_attackAnimTimer / 0.2f;
        jawOpen = std::sin(prog * 3.14159f) * 20.0f * S; // opens then snaps shut
    }

    // Upper jaw
    sf::ConvexShape upperJaw(5);
    upperJaw.setPoint(0, {headX, headY - 6.0f*S});
    upperJaw.setPoint(1, {headX + dir * 8.0f*S, headY - 7.0f*S - jawOpen * 0.3f});
    upperJaw.setPoint(2, {headX + dir * 28.0f*S, headY - 3.0f*S - jawOpen * 0.5f});
    upperJaw.setPoint(3, {headX + dir * 30.0f*S, headY - jawOpen * 0.2f});
    upperJaw.setPoint(4, {headX, headY + 2.0f*S});
    upperJaw.setFillColor(dc);
    upperJaw.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    upperJaw.setOutlineThickness(1.0f);
    target.draw(upperJaw);

    // Lower jaw
    sf::ConvexShape lowerJaw(4);
    lowerJaw.setPoint(0, {headX, headY + 2.0f*S});
    lowerJaw.setPoint(1, {headX + dir * 26.0f*S, headY + 2.0f*S + jawOpen * 0.5f});
    lowerJaw.setPoint(2, {headX + dir * 24.0f*S, headY + 7.0f*S + jawOpen * 0.4f});
    lowerJaw.setPoint(3, {headX - dir * 2.0f*S, headY + 8.0f*S});
    lowerJaw.setFillColor(belly);
    lowerJaw.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    lowerJaw.setOutlineThickness(1.0f);
    target.draw(lowerJaw);

    // Teeth (upper)
    for (int i = 0; i < 5; i++) {
        float tx = headX + dir * (6.0f + static_cast<float>(i) * 5.0f) * S;
        float ty = headY + 1.0f*S - jawOpen * 0.15f;
        sf::ConvexShape tooth(3);
        tooth.setPoint(0, {tx - 1.0f, ty});
        tooth.setPoint(1, {tx, ty + 4.0f + jawOpen * 0.1f});
        tooth.setPoint(2, {tx + 1.0f, ty});
        tooth.setFillColor(sf::Color(240, 235, 210));
        target.draw(tooth);
    }
    // Teeth (lower)
    for (int i = 0; i < 4; i++) {
        float tx = headX + dir * (8.0f + static_cast<float>(i) * 5.0f) * S;
        float ty = headY + 3.0f*S + jawOpen * 0.4f;
        sf::ConvexShape tooth(3);
        tooth.setPoint(0, {tx - 1.0f, ty});
        tooth.setPoint(1, {tx, ty - 3.5f - jawOpen * 0.1f});
        tooth.setPoint(2, {tx + 1.0f, ty});
        tooth.setFillColor(sf::Color(230, 225, 200));
        target.draw(tooth);
    }

    // Nostril bumps at tip of snout
    for (float ns : {-1.5f, 1.5f}) {
        sf::CircleShape nostril(1.5f);
        nostril.setOrigin({1.5f, 1.5f});
        nostril.setPosition({headX + dir * 28.0f*S, headY - 4.0f*S + ns});
        nostril.setFillColor(sf::Color(dc.r * 3 / 4, dc.g * 3 / 4, dc.b / 2));
        target.draw(nostril);
    }

    // Eyes (menacing, slit pupils on bumps)
    for (float es : {-1.0f, 1.0f}) {
        // Eye bump
        sf::CircleShape eyeBump(3.5f);
        eyeBump.setOrigin({3.5f, 3.5f});
        eyeBump.setPosition({headX + dir * 4.0f*S + es * 3.0f*S * (dir > 0 ? 1.0f : -1.0f), headY - 9.0f*S});
        eyeBump.setFillColor(dc);
        target.draw(eyeBump);
        // Eye
        sf::CircleShape eye(2.5f);
        eye.setOrigin({2.5f, 2.5f});
        eye.setPosition({headX + dir * 4.0f*S + es * 3.0f*S * (dir > 0 ? 1.0f : -1.0f), headY - 10.0f*S});
        eye.setFillColor(sf::Color(200, 180, 50));
        target.draw(eye);
        // Slit pupil
        sf::RectangleShape pupil({1.0f, 4.0f});
        pupil.setOrigin({0.5f, 2.0f});
        pupil.setPosition({headX + dir * 4.0f + es * 3.0f * (dir > 0 ? 1.0f : -1.0f), headY - 10.0f});
        pupil.setFillColor(sf::Color::Black);
        target.draw(pupil);
    }
}

void StickFigure::drawStickLady(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    sf::Color jc = sf::Color(
        static_cast<uint8_t>(std::min(255, (int)dc.r + 60)),
        static_cast<uint8_t>(std::min(255, (int)dc.g + 60)),
        static_cast<uint8_t>(std::min(255, (int)dc.b + 60)));
    float dir = static_cast<float>(m_facingDir);
    float t = m_animTime;

    auto drawSeg = [&](b2Vec2 a, b2Vec2 bv, sf::Color col, float thick) {
        sf::Vector2f sa = toScreen(a);
        sf::Vector2f sb = toScreen(bv);
        float dx = sb.x-sa.x, dy = sb.y-sa.y;
        float len = std::sqrt(dx*dx+dy*dy);
        if (len < 0.5f) return;
        sf::RectangleShape seg({len, thick});
        seg.setOrigin({0.f, thick*0.5f});
        seg.setPosition(sa);
        seg.setRotation(sf::degrees(std::atan2(dy,dx)*57.2958f));
        seg.setFillColor(col);
        target.draw(seg);
    };
    auto drawDot = [&](b2Vec2 pos, float r, sf::Color col) {
        sf::Vector2f sp = toScreen(pos);
        sf::CircleShape c(r); c.setOrigin({r,r}); c.setPosition(sp);
        c.setFillColor(col); target.draw(c);
    };

    const float torsoH  = 0.55f * PLAYER_SCALE;
    const float torsoW  = 0.20f * PLAYER_SCALE;
    const float uArmLen = 0.38f * PLAYER_SCALE;
    const float lArmLen = 0.34f * PLAYER_SCALE;
    const float uLegLen = 0.40f * PLAYER_SCALE;
    const float lLegLen = 0.38f * PLAYER_SCALE;

    b2Vec2 tp = b2Body_GetPosition(m_torso);
    b2Vec2 hp = b2Body_GetPosition(m_head);
    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x);

    b2Vec2 neck   = {tp.x, tp.y + torsoH};
    b2Vec2 spine  = {tp.x, tp.y - torsoH};
    b2Vec2 shoulL = {tp.x - torsoW, tp.y + torsoH*0.55f};
    b2Vec2 shoulR = {tp.x + torsoW, tp.y + torsoH*0.55f};
    b2Vec2 hipL   = {tp.x - torsoW*0.5f, tp.y - torsoH};
    b2Vec2 hipR   = {tp.x + torsoW*0.5f, tp.y - torsoH};

    auto limbTip = [](b2BodyId body, float hl) -> b2Vec2 {
        b2Vec2 c = b2Body_GetPosition(body);
        b2Rot  r = b2Body_GetRotation(body);
        return {c.x + r.s*hl, c.y - r.c*hl};
    };

    b2Vec2 elbowL = limbTip(m_leftUpperArm,  uArmLen);
    b2Vec2 elbowR = limbTip(m_rightUpperArm, uArmLen);
    b2Vec2 handL  = limbTip(m_leftForeArm,   lArmLen);
    b2Vec2 handR  = limbTip(m_rightForeArm,  lArmLen);
    b2Vec2 kneeL  = limbTip(m_leftUpperLeg,  uLegLen);
    b2Vec2 kneeR  = limbTip(m_rightUpperLeg, uLegLen);
    b2Vec2 footL  = limbTip(m_leftLowerLeg,  lLegLen);
    b2Vec2 footR  = limbTip(m_rightLowerLeg, lLegLen);


    sf::Color backCol(
        static_cast<uint8_t>(dc.r*0.45f),
        static_cast<uint8_t>(dc.g*0.45f),
        static_cast<uint8_t>(dc.b*0.45f));
    sf::Color legCol(
        static_cast<uint8_t>(dc.r*0.78f),
        static_cast<uint8_t>(dc.g*0.78f),
        static_cast<uint8_t>(dc.b*0.78f));

    // Flowing hair
    sf::Vector2f headSc = toScreen(hp);
    float headR = m_config.headRadius * PPM;
    constexpr int hairStrands = 8;
    float hairLen = 22.0f;
    for (int i = 0; i < hairStrands; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(hairStrands - 1);
        float attachAngle = (0.6f + frac * 1.8f);
        float ax = headSc.x - dir * std::cos(attachAngle) * headR * 0.8f;
        float ay = headSc.y - std::sin(attachAngle) * headR * 0.4f;
        sf::VertexArray strand(sf::PrimitiveType::LineStrip, 6);
        for (int s = 0; s < 6; s++) {
            float sf2 = static_cast<float>(s) / 5.0f;
            float wave = std::sin(t * 3.0f + frac * 2.0f + sf2 * 4.0f) * (3.0f + speed * 0.5f);
            float windPush = -dir * sf2 * (4.0f + speed * 1.5f);
            float sx = ax + windPush + wave * 0.3f;
            float sy = ay + sf2 * hairLen + std::sin(t * 2.0f + frac) * 2.0f * sf2;
            uint8_t alpha = static_cast<uint8_t>(255 * (1.0f - sf2 * 0.3f));
            strand[s] = sf::Vertex{{sx, sy}, sf::Color(dc.r, dc.g, dc.b, alpha)};
        }
        target.draw(strand);
    }

    // Back limbs
    if (m_facingDir > 0) {
        drawSeg(shoulL, elbowL, backCol, 3.5f);
        drawSeg(elbowL, handL,  backCol, 3.0f);
        drawSeg(hipL,   kneeL,  backCol, 3.5f);
        drawSeg(kneeL,  footL,  backCol, 3.0f);
    } else {
        drawSeg(shoulR, elbowR, backCol, 3.5f);
        drawSeg(elbowR, handR,  backCol, 3.0f);
        drawSeg(hipR,   kneeR,  backCol, 3.5f);
        drawSeg(kneeR,  footR,  backCol, 3.0f);
    }

    // Torso
    drawSeg(neck, spine, dc, 5.5f);
    drawSeg(shoulL, shoulR, dc, 4.5f);

    // ── Dress (A-line from waist to knees) ──
    {
        sf::Vector2f waistSc = toScreen({tp.x, tp.y - torsoH * 0.3f});
        sf::Vector2f hipLSc  = toScreen(hipL);
        sf::Vector2f hipRSc  = toScreen(hipR);
        sf::Vector2f kneeLSc = toScreen(kneeL);
        sf::Vector2f kneeRSc = toScreen(kneeR);
        // Dress color: slightly lighter/different from body
        sf::Color dressCol(
            static_cast<uint8_t>(std::min(255, dc.r + 40)),
            static_cast<uint8_t>(std::min(255, dc.g + 20)),
            static_cast<uint8_t>(std::min(255, dc.b + 60)),
            220);
        float flare = 8.0f * PLAYER_SCALE * PPM / 30.0f;
        float sway = std::sin(t * 2.5f + speed * 0.3f) * 2.0f;
        // Trapezoid: waist narrow, hem wide at knee level
        sf::ConvexShape dress(4);
        dress.setPoint(0, {waistSc.x - 6.0f * PLAYER_SCALE, waistSc.y});
        dress.setPoint(1, {waistSc.x + 6.0f * PLAYER_SCALE, waistSc.y});
        float hemY = (kneeLSc.y + kneeRSc.y) * 0.5f;
        dress.setPoint(2, {waistSc.x + flare + sway, hemY});
        dress.setPoint(3, {waistSc.x - flare - sway, hemY});
        dress.setFillColor(dressCol);
        dress.setOutlineColor(sf::Color(dressCol.r/2, dressCol.g/2, dressCol.b/2, 180));
        dress.setOutlineThickness(1.0f);
        target.draw(dress);
    }

    // Head with eyelashes
    sf::CircleShape hs(headR);
    hs.setOrigin({headR, headR});
    hs.setPosition(headSc);
    hs.setFillColor(sf::Color::Transparent);
    hs.setOutlineColor(dc);
    hs.setOutlineThickness(3.0f);
    target.draw(hs);

    float eyeX = headSc.x + dir * headR * 0.4f;
    float eyeY = headSc.y - headR * 0.15f;
    sf::CircleShape eye(2.5f);
    eye.setOrigin({2.5f, 2.5f});
    eye.setPosition({eyeX, eyeY});
    eye.setFillColor(dc);
    target.draw(eye);

    for (int i = 0; i < 3; i++) {
        float angle = -0.5f + static_cast<float>(i) * 0.5f;
        float lashLen = 4.0f + static_cast<float>(i == 1) * 2.0f;
        sf::VertexArray lash(sf::PrimitiveType::Lines, 2);
        lash[0] = sf::Vertex{{eyeX, eyeY - 2.0f}, dc};
        lash[1] = sf::Vertex{{eyeX + std::sin(angle) * lashLen * dir,
                               eyeY - 2.0f - std::cos(angle) * lashLen}, dc};
        target.draw(lash);
    }

    // Front limbs
    if (m_facingDir > 0) {
        drawSeg(shoulR, elbowR, dc, 4.5f);
        drawSeg(elbowR, handR, dc, 4.0f);
        drawSeg(hipR, kneeR, legCol, 4.5f);
        drawSeg(kneeR, footR, legCol, 4.0f);
        drawSeg(footR, {footR.x+0.18f, footR.y}, legCol, 3.5f);
    } else {
        drawSeg(shoulL, elbowL, dc, 4.5f);
        drawSeg(elbowL, handL, dc, 4.0f);
        drawSeg(hipL, kneeL, legCol, 4.5f);
        drawSeg(kneeL, footL, legCol, 4.0f);
        drawSeg(footL, {footL.x-0.18f, footL.y}, legCol, 3.5f);
    }

    // Purse (if applicable)
    if (m_weapon.name == "Purse Swing") {
        sf::Vector2f armSc = toScreen((m_facingDir > 0) ? handR : handL);
        float purseSwing = std::sin(t * 1.5f) * 8.0f;
        float purseX = armSc.x + dir * 8.0f;
        float purseY = armSc.y + 12.0f + purseSwing;
        
        sf::VertexArray strap(sf::PrimitiveType::Lines, 2);
        strap[0] = sf::Vertex{armSc, dc};
        strap[1] = sf::Vertex{{purseX, purseY}, dc};
        target.draw(strap);
        
        sf::RectangleShape purseBody({10.0f, 8.0f});
        purseBody.setOrigin({5.0f, 0.0f});
        purseBody.setPosition({purseX, purseY});
        purseBody.setRotation(sf::degrees(purseSwing + std::sin(t * 2.0f) * 5.0f));
        sf::Color purseColor(
            static_cast<uint8_t>(std::min(255, 255 - dc.r / 2)),
            static_cast<uint8_t>(std::min(255, dc.g / 3)),
            static_cast<uint8_t>(std::min(255, dc.b / 2 + 80)));
        purseBody.setFillColor(purseColor);
        purseBody.setOutlineColor(sf::Color(purseColor.r / 2, purseColor.g / 2, purseColor.b / 2));
        purseBody.setOutlineThickness(1.0f);
        target.draw(purseBody);
        
        sf::CircleShape clasp(1.5f);
        clasp.setOrigin({1.5f, 1.5f});
        clasp.setPosition({purseX, purseY + 2.0f});
        clasp.setFillColor(sf::Color(200, 180, 100));
        target.draw(clasp);
    }

    // Joint dots removed for cleaner look
}

void StickFigure::drawAttackEffect(sf::RenderTarget& target) const {
    sf::Vector2f sp = toScreen(getPosition());
    float dir = static_cast<float>(m_facingDir);
    float prog = 1.0f - (m_attackAnimTimer / 0.2f);

    if (m_weapon.type == WeaponType::Melee && m_charType == CharacterType::StickLady) {
        // Purse swing attack — wide arc with purse trail
        float swingAngle = -120.0f + 240.0f * prog; // big swing arc
        float swingRad = swingAngle * 3.14159f / 180.0f;
        float swingR = m_weapon.range * PPM * 0.5f;
        float purseX = sp.x + dir * std::cos(swingRad) * swingR;
        float purseY = sp.y - 5.0f + std::sin(swingRad) * swingR;

        // Trail particles
        int trailCount = static_cast<int>(prog * 8);
        for (int i = 0; i < trailCount; i++) {
            float trailProg = prog - static_cast<float>(i) * 0.04f;
            if (trailProg < 0.0f) continue;
            float ta = -120.0f + 240.0f * trailProg;
            float tr = ta * 3.14159f / 180.0f;
            float tx = sp.x + dir * std::cos(tr) * swingR;
            float ty = sp.y - 5.0f + std::sin(tr) * swingR;
            float sz = 2.0f * (1.0f - static_cast<float>(i) * 0.1f);
            sf::CircleShape trail(sz);
            trail.setOrigin({sz, sz});
            trail.setPosition({tx, ty});
            uint8_t alpha = static_cast<uint8_t>(180 * (1.0f - static_cast<float>(i) * 0.12f));
            trail.setFillColor(sf::Color(255, 180, 220, alpha));
            target.draw(trail);
        }

        // Purse at current position
        sf::RectangleShape purseHit({12.0f, 10.0f});
        purseHit.setOrigin({6.0f, 5.0f});
        purseHit.setPosition({purseX, purseY});
        purseHit.setRotation(sf::degrees(swingAngle));
        purseHit.setFillColor(sf::Color(200, 80, 150, static_cast<uint8_t>(220 * (1.0f - prog))));
        purseHit.setOutlineColor(sf::Color(150, 50, 100, static_cast<uint8_t>(200 * (1.0f - prog))));
        purseHit.setOutlineThickness(1.0f);
        target.draw(purseHit);

        // Impact star at end of swing
        if (prog > 0.6f && prog < 0.9f) {
            float starSz = 8.0f * (1.0f - (prog - 0.6f) / 0.3f);
            for (int s = 0; s < 4; s++) {
                float a = static_cast<float>(s) * 0.785f + m_animTime * 3.0f;
                sf::VertexArray ray(sf::PrimitiveType::Lines, 2);
                ray[0] = sf::Vertex{{purseX, purseY}, sf::Color(255, 255, 100, 200)};
                ray[1] = sf::Vertex{{purseX + std::cos(a) * starSz,
                                      purseY + std::sin(a) * starSz}, sf::Color(255, 255, 100, 60)};
                target.draw(ray);
            }
        }
    } else if (m_weapon.type == WeaponType::Melee && m_charType == CharacterType::Crocodile) {
        // Jaw snap effect — closing jaws with impact lines
        float snapProg = prog; // 0 = start, 1 = fully snapped
        float jawAngle = (1.0f - std::abs(snapProg * 2.0f - 1.0f)) * 25.0f; // opens then snaps

        // Upper jaw line
        float jawLen = m_weapon.range * PPM * 0.5f;
        sf::ConvexShape upperJaw(3);
        upperJaw.setPoint(0, {sp.x + dir * 10.0f, sp.y - 8.0f});
        upperJaw.setPoint(1, {sp.x + dir * (10.0f + jawLen), sp.y - 8.0f - jawAngle * 0.5f});
        upperJaw.setPoint(2, {sp.x + dir * (10.0f + jawLen * 0.8f), sp.y - 2.0f});
        upperJaw.setFillColor(sf::Color(200, 200, 180, static_cast<uint8_t>(200 * (1.0f - prog))));
        target.draw(upperJaw);

        // Lower jaw line
        sf::ConvexShape lowerJaw(3);
        lowerJaw.setPoint(0, {sp.x + dir * 10.0f, sp.y + 4.0f});
        lowerJaw.setPoint(1, {sp.x + dir * (10.0f + jawLen), sp.y + 4.0f + jawAngle * 0.5f});
        lowerJaw.setPoint(2, {sp.x + dir * (10.0f + jawLen * 0.8f), sp.y});
        lowerJaw.setFillColor(sf::Color(200, 200, 180, static_cast<uint8_t>(200 * (1.0f - prog))));
        target.draw(lowerJaw);

        // Impact crunch lines at snap moment
        if (snapProg > 0.4f && snapProg < 0.7f) {
            float impactX = sp.x + dir * (15.0f + jawLen * 0.6f);
            float impactY = sp.y - 2.0f;
            for (int i = 0; i < 6; i++) {
                float angle = static_cast<float>(i) / 6.0f * 6.28f;
                float len2 = 6.0f + std::sin(m_animTime * 10.0f + angle) * 3.0f;
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0] = sf::Vertex{{impactX, impactY}, sf::Color(255, 255, 200, 200)};
                line[1] = sf::Vertex{{impactX + std::cos(angle) * len2,
                                       impactY + std::sin(angle) * len2},
                                      sf::Color(255, 255, 200, 80)};
                target.draw(line);
            }
        }
    } else if (m_weapon.type == WeaponType::Melee && m_charType == CharacterType::Unicorn) {
        // Magical horn blast — expanding rainbow ring
        float arcR = m_weapon.range * PPM * 0.7f * prog;
        constexpr int particles = 12;
        for (int i = 0; i < particles; i++) {
            float angle = static_cast<float>(i) / static_cast<float>(particles) * 6.28318f;
            float px = sp.x + dir * 15.0f + std::cos(angle) * arcR;
            float py = sp.y - 15.0f + std::sin(angle) * arcR;
            float sz = 3.0f * (1.0f - prog);
            float phase = static_cast<float>(i) / static_cast<float>(particles) * 6.28f;
            float r = std::sin(m_animTime * 3.0f + phase) * 0.5f + 0.5f;
            float g = std::sin(m_animTime * 3.0f + phase + 2.094f) * 0.5f + 0.5f;
            float b = std::sin(m_animTime * 3.0f + phase + 4.189f) * 0.5f + 0.5f;
            sf::CircleShape dot(sz, 4);
            dot.setOrigin({sz, sz}); dot.setPosition({px, py});
            dot.setFillColor(sf::Color(
                static_cast<uint8_t>(r * 255),
                static_cast<uint8_t>(g * 255),
                static_cast<uint8_t>(b * 255),
                static_cast<uint8_t>(220 * (1.0f - prog))));
            target.draw(dot);
        }
        // Central flash
        float flashSz = 8.0f * (1.0f - prog);
        sf::CircleShape flash(flashSz);
        flash.setOrigin({flashSz, flashSz});
        flash.setPosition({sp.x + dir * 15.0f, sp.y - 15.0f});
        flash.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(180 * (1.0f - prog))));
        target.draw(flash);
    } else if (m_weapon.type == WeaponType::Melee) {
        float arcR = m_weapon.range * PPM * 0.6f;
        int segs = 8;
        for (int i = 0; i <= segs; i++) {
            float t = static_cast<float>(i) / static_cast<float>(segs);
            if (t > prog) break;
            float angle = (-45.0f + 90.0f * t) * 3.14159f / 180.0f;
            float ax = sp.x + dir * std::cos(angle) * arcR;
            float ay = sp.y - std::sin(angle) * arcR;
            float ds = 3.0f * (1.0f - t * 0.5f);
            sf::CircleShape dot(ds); dot.setOrigin({ds, ds}); dot.setPosition({ax, ay});
            dot.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(255 * (1.0f - prog))));
            target.draw(dot);
        }
    } else {
        // Muzzle flash
        float fs = 6.0f * (1.0f - prog);
        sf::CircleShape flash(fs); flash.setOrigin({fs, fs});
        float aimY = -std::sin(m_aimAngle) * 20.0f;
        flash.setPosition({sp.x + dir * 20.0f, sp.y - 5.0f + aimY});
        flash.setFillColor(sf::Color(255, 255, 0, static_cast<uint8_t>(200 * (1.0f - prog))));
        target.draw(flash);
    }
}

void StickFigure::drawAimIndicator(sf::RenderTarget& target) const {
    sf::Vector2f sp = toScreen(getPosition());
    float dir = static_cast<float>(m_facingDir);

    // Draw a dotted line showing aim direction
    float len = 40.0f;
    float cosA = std::cos(m_aimAngle);
    float sinA = std::sin(m_aimAngle);

    for (int i = 1; i <= 4; i++) {
        float t = static_cast<float>(i) / 4.0f;
        float dx = dir * cosA * len * t;
        float dy = -sinA * len * t; // negative because screen Y is flipped
        sf::CircleShape dot(1.5f); dot.setOrigin({1.5f, 1.5f});
        dot.setPosition({sp.x + dx, sp.y - 5.0f + dy});
        dot.setFillColor(sf::Color(255, 255, 255, 120));
        target.draw(dot);
    }
}

// ============================================================
// WEAPON VISUALS — held weapon drawn on the character's hand
// ============================================================
void StickFigure::drawWeapon(sf::RenderTarget& target) const {
    if (m_weapon.name == "Fists" || m_weapon.name.empty()) return;

    sf::Vector2f sp = toScreen(getPosition());
    float dir  = static_cast<float>(m_facingDir);
    float t    = m_animTime;
    float aimY = -std::sin(m_aimAngle) * 10.0f;

    // Hand position: end of forearm on the facing side
    b2BodyId foreArmBody = (m_facingDir > 0) ? m_rightForeArm : m_leftForeArm;
    b2Vec2 foreArmPos    = b2Body_GetPosition(foreArmBody);
    b2Rot  foreArmRot    = b2Body_GetRotation(foreArmBody);
    float  faLen         = 0.34f;  // matches animation lArmLen
    // Tip of forearm = centre + rot*(0,-halfLen) = (sin*hl, -cos*hl)
    b2Vec2 handWorld = {foreArmPos.x + foreArmRot.s * faLen,
                        foreArmPos.y - foreArmRot.c * faLen};
    sf::Vector2f handSc = toScreen(handWorld);
    handSc.y += aimY;

    // Attack bob — punch forward slightly
    if (m_attackAnimTimer > 0.0f) {
        float swing = std::sin((1.0f - m_attackAnimTimer / 0.2f) * 3.14159f);
        handSc.x += dir * swing * 8.0f;
        handSc.y -= swing * 4.0f;
    }

    const std::string& n = m_weapon.name;
    sf::Color wc = m_color;

    // ── MELEE ────────────────────────────────────────────────
    if (n == "Katana") {
        // Long thin blade
        sf::VertexArray blade(sf::PrimitiveType::Lines, 2);
        blade[0] = sf::Vertex{handSc, sf::Color(200,200,220)};
        blade[1] = sf::Vertex{{handSc.x + dir*22.f, handSc.y - 4.f}, sf::Color(230,230,255,180)};
        target.draw(blade);
        // Guard
        sf::RectangleShape guard({2.f, 8.f});
        guard.setOrigin({1.f, 4.f});
        guard.setPosition(handSc);
        guard.setFillColor(sf::Color(180,150,80));
        target.draw(guard);
    } else if (n == "Gravity Hammer") {
        // Thick handle + big hammer head
        sf::RectangleShape handle({14.f, 3.f});
        handle.setOrigin({0.f, 1.5f});
        handle.setPosition(handSc);
        handle.setRotation(sf::degrees(dir > 0 ? 0 : 180));
        handle.setFillColor(sf::Color(100,80,60));
        target.draw(handle);
        sf::RectangleShape head({8.f, 14.f});
        head.setOrigin({dir > 0 ? 0.f : 8.f, 7.f});
        head.setPosition({handSc.x + dir*14.f, handSc.y});
        head.setFillColor(sf::Color(80,80,90));
        head.setOutlineColor(sf::Color(140,140,160));
        head.setOutlineThickness(1.f);
        target.draw(head);
    } else if (n == "Teleport Dagger") {
        // Short glowing blade
        sf::VertexArray blade(sf::PrimitiveType::Lines, 2);
        float glow = std::sin(t * 6.f) * 0.3f + 0.7f;
        blade[0] = sf::Vertex{handSc, sf::Color(180,100,255)};
        blade[1] = sf::Vertex{{handSc.x + dir*14.f, handSc.y - 2.f},
                              sf::Color(220,180,255, static_cast<uint8_t>(200*glow))};
        target.draw(blade);
        // Sparkle at tip
        float sz = 2.5f * glow;
        sf::CircleShape spark(sz); spark.setOrigin({sz,sz});
        spark.setPosition({handSc.x + dir*14.f, handSc.y - 2.f});
        spark.setFillColor(sf::Color(220,180,255, static_cast<uint8_t>(180*glow)));
        target.draw(spark);
    } else if (n == "Chain Whip") {
        // Chain links
        int links = 5;
        float prevX = handSc.x, prevY = handSc.y;
        for (int i = 1; i <= links; i++) {
            float f = static_cast<float>(i) / links;
            float wave = std::sin(t * 4.f + f * 3.14159f) * 4.f;
            float nx = handSc.x + dir * f * 20.f;
            float ny = handSc.y + wave;
            sf::VertexArray seg(sf::PrimitiveType::Lines, 2);
            seg[0] = sf::Vertex{{prevX,prevY}, sf::Color(140,140,160)};
            seg[1] = sf::Vertex{{nx,ny}, sf::Color(180,180,200)};
            target.draw(seg);
            sf::CircleShape link(2.f); link.setOrigin({2.f,2.f});
            link.setPosition({nx, ny});
            link.setFillColor(sf::Color(160,160,180));
            target.draw(link);
            prevX = nx; prevY = ny;
        }
    } else if (n == "Rocket Fist") {
        // Armored glove with rocket exhaust
        sf::RectangleShape glove({12.f, 9.f});
        glove.setOrigin({0.f, 4.5f});
        glove.setPosition(handSc);
        glove.setFillColor(sf::Color(180,60,60));
        glove.setOutlineColor(sf::Color(220,100,80));
        glove.setOutlineThickness(1.f);
        target.draw(glove);
        // Exhaust flame
        float flame = std::sin(t * 12.f) * 2.f + 4.f;
        sf::CircleShape exhaust(flame); exhaust.setOrigin({flame, flame});
        exhaust.setPosition({handSc.x - dir*flame, handSc.y});
        exhaust.setFillColor(sf::Color(255,120,0, 160));
        target.draw(exhaust);
    } else if (n == "Energy Shield") {
        // Hexagonal shield glow
        float alpha = m_shieldTimer > 0.f
            ? static_cast<uint8_t>(180 + std::sin(t*8.f)*40.f)
            : 80u;
        sf::CircleShape shield(16.f, 6);
        shield.setOrigin({16.f, 16.f});
        shield.setPosition({handSc.x + dir*8.f, handSc.y});
        shield.setFillColor(sf::Color(80,120,255, static_cast<uint8_t>(alpha*0.3f)));
        shield.setOutlineColor(sf::Color(120,180,255, static_cast<uint8_t>(alpha)));
        shield.setOutlineThickness(2.f);
        target.draw(shield);

    // ── PROJECTILE ───────────────────────────────────────────
    } else if (n == "Pistol") {
        sf::RectangleShape body({12.f, 6.f});
        body.setOrigin({0.f, 3.f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(60,60,70));
        target.draw(body);
        sf::RectangleShape barrel({8.f, 3.f});
        barrel.setOrigin({0.f, 1.5f});
        barrel.setPosition({handSc.x + dir*6.f, handSc.y - 1.f});
        barrel.setFillColor(sf::Color(40,40,50));
        target.draw(barrel);
    } else if (n == "Shotgun") {
        sf::RectangleShape stock({8.f, 5.f});
        stock.setOrigin({0.f, 2.5f});
        stock.setPosition(handSc);
        stock.setFillColor(sf::Color(100,70,40));
        target.draw(stock);
        sf::RectangleShape barrel({18.f, 4.f});
        barrel.setOrigin({0.f, 2.f});
        barrel.setPosition({handSc.x + dir*6.f, handSc.y});
        barrel.setFillColor(sf::Color(50,50,55));
        target.draw(barrel);
    } else if (n == "Sticky Gun") {
        sf::RectangleShape body({11.f, 7.f});
        body.setOrigin({0.f, 3.5f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(60,140,60));
        target.draw(body);
        // Goo nozzle
        sf::CircleShape nozzle(4.f); nozzle.setOrigin({4.f,4.f});
        nozzle.setPosition({handSc.x + dir*12.f, handSc.y});
        nozzle.setFillColor(sf::Color(100,220,80));
        target.draw(nozzle);
    } else if (n == "Freeze Ray") {
        sf::RectangleShape body({14.f, 5.f});
        body.setOrigin({0.f, 2.5f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(80,180,220));
        target.draw(body);
        float glow = std::sin(t * 5.f) * 0.3f + 0.7f;
        sf::CircleShape tip(3.f); tip.setOrigin({3.f,3.f});
        tip.setPosition({handSc.x + dir*14.f, handSc.y});
        tip.setFillColor(sf::Color(180,240,255, static_cast<uint8_t>(200*glow)));
        target.draw(tip);
    } else if (n == "Lightning Staff") {
        // Tall staff with crackling tip
        sf::RectangleShape staff({3.f, 22.f});
        staff.setOrigin({1.5f, 19.f});
        staff.setPosition(handSc);
        staff.setFillColor(sf::Color(120,80,180));
        target.draw(staff);
        float bolt = std::sin(t * 10.f) * 0.5f + 0.5f;
        sf::CircleShape orb(5.f); orb.setOrigin({5.f,5.f});
        orb.setPosition({handSc.x, handSc.y - 19.f});
        orb.setFillColor(sf::Color(180,120,255, static_cast<uint8_t>(200*bolt)));
        orb.setOutlineColor(sf::Color(220,180,255, static_cast<uint8_t>(255*bolt)));
        orb.setOutlineThickness(2.f);
        target.draw(orb);
    } else if (n == "Shrink Ray") {
        sf::RectangleShape body({13.f, 6.f});
        body.setOrigin({0.f, 3.f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(180,80,180));
        target.draw(body);
        // Funnel shape at the front
        sf::ConvexShape funnel(3);
        funnel.setPoint(0, {handSc.x + dir*13.f, handSc.y - 4.f});
        funnel.setPoint(1, {handSc.x + dir*13.f, handSc.y + 4.f});
        funnel.setPoint(2, {handSc.x + dir*18.f, handSc.y});
        funnel.setFillColor(sf::Color(220,120,220));
        target.draw(funnel);
    } else if (n == "Time Gun") {
        sf::RectangleShape body({12.f, 6.f});
        body.setOrigin({0.f, 3.f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(100,100,60));
        target.draw(body);
        // Clock face on side
        float pulse = std::sin(t * 2.f) * 0.3f + 0.7f;
        sf::CircleShape clock(4.f); clock.setOrigin({4.f,4.f});
        clock.setPosition({handSc.x + dir*5.f, handSc.y});
        clock.setFillColor(sf::Color::Transparent);
        clock.setOutlineColor(sf::Color(255,220,80, static_cast<uint8_t>(200*pulse)));
        clock.setOutlineThickness(1.5f);
        target.draw(clock);
        // Clock hands
        float handAngle = t * 2.f;
        sf::VertexArray hand(sf::PrimitiveType::Lines, 2);
        hand[0] = sf::Vertex{{handSc.x + dir*5.f, handSc.y}, sf::Color(255,220,80,200)};
        hand[1] = sf::Vertex{{handSc.x + dir*5.f + std::cos(handAngle)*3.f,
                               handSc.y - std::sin(handAngle)*3.f},
                              sf::Color(255,220,80,200)};
        target.draw(hand);
    } else if (n == "Healing Pulse Gun") {
        sf::RectangleShape body({12.f, 6.f});
        body.setOrigin({0.f, 3.f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(60,160,80));
        target.draw(body);
        float pulse = std::sin(t * 3.f) * 0.4f + 0.6f;
        sf::CircleShape tip(3.5f); tip.setOrigin({3.5f,3.5f});
        tip.setPosition({handSc.x + dir*12.f, handSc.y});
        tip.setFillColor(sf::Color(100,255,120, static_cast<uint8_t>(220*pulse)));
        target.draw(tip);
    } else if (n == "Portal Gun") {
        sf::RectangleShape body({13.f, 7.f});
        body.setOrigin({0.f, 3.5f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(30,30,50));
        target.draw(body);
        float phase = std::fmod(t * 2.f, 6.28318f);
        sf::Color portalCol = (m_currentAmmo % 2 == 0)
            ? sf::Color(80, 160, 255, 220)
            : sf::Color(255, 140, 40, 220);
        sf::CircleShape tip(4.f); tip.setOrigin({4.f,4.f});
        tip.setPosition({handSc.x + dir*13.f, handSc.y});
        tip.setFillColor(portalCol);
        tip.setOutlineColor(sf::Color::White); tip.setOutlineThickness(1.f);
        target.draw(tip);
    } else if (n == "Swap Staff") {
        // Two-color twisted staff
        sf::RectangleShape staff({3.f, 20.f});
        staff.setOrigin({1.5f, 18.f});
        staff.setPosition(handSc);
        float twist = std::sin(t * 3.f) * 0.5f + 0.5f;
        staff.setFillColor(sf::Color(
            static_cast<uint8_t>(100 + 100*twist),
            80,
            static_cast<uint8_t>(200 - 100*twist)));
        target.draw(staff);
        sf::CircleShape orb(4.f); orb.setOrigin({4.f,4.f});
        orb.setPosition({handSc.x, handSc.y - 18.f});
        orb.setFillColor(sf::Color(200,100,255,200));
        target.draw(orb);
    } else if (n == "Mind Control Staff") {
        sf::RectangleShape staff({3.f, 20.f});
        staff.setOrigin({1.5f, 18.f});
        staff.setPosition(handSc);
        staff.setFillColor(sf::Color(80,40,120));
        target.draw(staff);
        // Swirling eye
        float spin = t * 3.f;
        for (int i = 0; i < 3; i++) {
            float a = spin + i * 2.094f;
            float sx = handSc.x + std::cos(a) * 4.f;
            float sy = handSc.y - 18.f + std::sin(a) * 4.f;
            sf::CircleShape dot(1.5f); dot.setOrigin({1.5f,1.5f});
            dot.setPosition({sx, sy});
            dot.setFillColor(sf::Color(220,120,255,200));
            target.draw(dot);
        }
    } else if (n == "Boomerang") {
        float spin = (m_attackAnimTimer > 0.f) ? t * 15.f : t * 2.f;
        sf::ConvexShape boom(3);
        float sz = 10.f;
        boom.setPoint(0, {0.f, -sz});
        boom.setPoint(1, {-sz*0.6f, sz*0.5f});
        boom.setPoint(2, { sz*0.6f, sz*0.5f});
        boom.setOrigin({0.f, 0.f});
        boom.setPosition(handSc);
        boom.setRotation(sf::degrees(spin * 57.3f));
        boom.setFillColor(sf::Color(180,120,60));
        boom.setOutlineColor(sf::Color(220,160,80));
        boom.setOutlineThickness(1.f);
        target.draw(boom);
    } else if (n == "Bouncy Ball Launcher") {
        sf::RectangleShape body({14.f, 7.f});
        body.setOrigin({0.f, 3.5f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(200,80,80));
        target.draw(body);
        // Ammo display — small balls
        for (int i = 0; i < 3; i++) {
            sf::CircleShape ball(2.f); ball.setOrigin({2.f,2.f});
            ball.setPosition({handSc.x + dir*3.f + dir*static_cast<float>(i)*4.f,
                               handSc.y - 6.f});
            ball.setFillColor(sf::Color(255,120,120));
            target.draw(ball);
        }
    } else if (n == "Black Hole Launcher") {
        sf::RectangleShape body({13.f, 6.f});
        body.setOrigin({0.f, 3.f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(10,10,20));
        body.setOutlineColor(sf::Color(80,0,120));
        body.setOutlineThickness(1.5f);
        target.draw(body);
        float spin = t * 4.f;
        sf::CircleShape tip(4.f); tip.setOrigin({4.f,4.f});
        tip.setPosition({handSc.x + dir*13.f, handSc.y});
        tip.setFillColor(sf::Color(20,0,40,200));
        tip.setOutlineColor(sf::Color(160,0,220, static_cast<uint8_t>(180 + std::sin(spin)*50)));
        tip.setOutlineThickness(2.f);
        target.draw(tip);
    } else if (n == "Tornado Launcher") {
        sf::RectangleShape body({12.f, 6.f});
        body.setOrigin({0.f, 3.f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(100,160,200));
        target.draw(body);
        // Spinning wind lines at nozzle
        for (int i = 0; i < 4; i++) {
            float a = t * 6.f + i * 1.5708f;
            float wx = handSc.x + dir*16.f + std::cos(a)*4.f;
            float wy = handSc.y + std::sin(a)*4.f;
            sf::VertexArray wl(sf::PrimitiveType::Lines, 2);
            wl[0] = sf::Vertex{{handSc.x + dir*12.f, handSc.y}, sf::Color(200,230,255,120)};
            wl[1] = sf::Vertex{{wx, wy}, sf::Color(200,230,255,60)};
            target.draw(wl);
        }
    } else if (n == "Meteor Shower Staff") {
        sf::RectangleShape staff({3.f, 22.f});
        staff.setOrigin({1.5f, 20.f});
        staff.setPosition(handSc);
        staff.setFillColor(sf::Color(140,80,40));
        target.draw(staff);
        // Flame at tip
        float flicker = std::sin(t * 8.f) * 2.f + 5.f;
        sf::CircleShape flame(flicker); flame.setOrigin({flicker, flicker});
        flame.setPosition({handSc.x, handSc.y - 20.f});
        flame.setFillColor(sf::Color(255, static_cast<uint8_t>(80 + flicker*10), 0, 180));
        target.draw(flame);

    // ── EXPLOSIVES ───────────────────────────────────────────
    } else if (n == "Grenade Launcher") {
        sf::RectangleShape body({14.f, 7.f});
        body.setOrigin({0.f, 3.5f});
        body.setPosition(handSc);
        body.setFillColor(sf::Color(60,90,60));
        target.draw(body);
        sf::CircleShape grenade(4.f); grenade.setOrigin({4.f,4.f});
        grenade.setPosition({handSc.x + dir*12.f, handSc.y});
        grenade.setFillColor(sf::Color(80,100,60));
        grenade.setOutlineColor(sf::Color(60,80,50));
        grenade.setOutlineThickness(1.f);
        target.draw(grenade);
    } else if (n == "Sticky Bomb") {
        // Thrown bomb with goo dripping
        sf::CircleShape bomb(5.f); bomb.setOrigin({5.f,5.f});
        bomb.setPosition(handSc);
        bomb.setFillColor(sf::Color(60,140,60));
        bomb.setOutlineColor(sf::Color(100,220,80));
        bomb.setOutlineThickness(1.5f);
        target.draw(bomb);
        // Goo drip
        for (int i = 0; i < 3; i++) {
            float drip = std::fmod(t * 0.5f + i * 0.33f, 1.0f);
            sf::CircleShape drop(1.5f); drop.setOrigin({1.5f,1.5f});
            drop.setPosition({handSc.x + dir*static_cast<float>(i)*2.f,
                               handSc.y + 5.f + drip*8.f});
            drop.setFillColor(sf::Color(100,220,80, static_cast<uint8_t>(255*(1.f-drip))));
            target.draw(drop);
        }
    } else {
        // Generic fallback: small colored block
        sf::RectangleShape generic({10.f, 5.f});
        generic.setOrigin({0.f, 2.5f});
        generic.setPosition(handSc);
        sf::Color gc = (m_weapon.type == WeaponType::Melee)
            ? sf::Color(200,80,80) : sf::Color(80,180,220);
        generic.setFillColor(gc);
        target.draw(generic);
    }
}

// ============================================================
// STATUS EFFECT OVERLAYS
// ============================================================
void StickFigure::drawStatusEffects(sf::RenderTarget& target) const {
    sf::Vector2f sp = toScreen(getPosition());
    float t = m_animTime;

    // ── FROZEN: ice crystal overlay ───────────────────────────
    if (m_timeSlowTimer > 0.0f) {
        float alpha = std::min(1.f, m_timeSlowTimer);
        // Ice tint behind character
        sf::CircleShape iceAura(22.f); iceAura.setOrigin({22.f,22.f});
        iceAura.setPosition(sp);
        iceAura.setFillColor(sf::Color(180,220,255, static_cast<uint8_t>(80*alpha)));
        iceAura.setOutlineColor(sf::Color(200,240,255, static_cast<uint8_t>(180*alpha)));
        iceAura.setOutlineThickness(2.f);
        target.draw(iceAura);
        // Ice crystals
        for (int i = 0; i < 6; i++) {
            float a = static_cast<float>(i) / 6.f * 6.28318f;
            float r = 18.f;
            float cx = sp.x + std::cos(a)*r;
            float cy = sp.y + std::sin(a)*r;
            sf::VertexArray crystal(sf::PrimitiveType::Lines, 2);
            crystal[0] = sf::Vertex{{cx, cy},
                sf::Color(200,240,255, static_cast<uint8_t>(200*alpha))};
            crystal[1] = sf::Vertex{{cx + std::cos(a)*5.f, cy + std::sin(a)*5.f},
                sf::Color(220,255,255, static_cast<uint8_t>(100*alpha))};
            target.draw(crystal);
        }
        // Freeze text
        sf::CircleShape snowflake(3.f); snowflake.setOrigin({3.f,3.f});
        snowflake.setPosition({sp.x, sp.y - 30.f});
        snowflake.setFillColor(sf::Color(180,220,255, static_cast<uint8_t>(200*alpha)));
        target.draw(snowflake);
    }

    // ── SLOWED: blue slow particles ───────────────────────────
    if (m_slowTimer > 0.0f && m_timeSlowTimer <= 0.0f) {
        for (int i = 0; i < 3; i++) {
            float phase = t * 1.5f + i * 2.094f;
            float ox = std::cos(phase) * 14.f;
            float oy = std::sin(phase * 0.7f) * 8.f;
            sf::CircleShape dot(2.f); dot.setOrigin({2.f,2.f});
            dot.setPosition({sp.x + ox, sp.y + oy});
            dot.setFillColor(sf::Color(100,150,255,180));
            target.draw(dot);
        }
    }

    // ── STUNNED: spinning stars ────────────────────────────────
    if (m_stunTimer > 0.0f) {
        for (int i = 0; i < 4; i++) {
            float a = t * 4.f + i * 1.5708f;
            float sx = sp.x + std::cos(a) * 16.f;
            float sy = sp.y - 25.f + std::sin(a) * 6.f;
            sf::CircleShape star(3.f, 5); star.setOrigin({3.f,3.f});
            star.setPosition({sx, sy});
            star.setFillColor(sf::Color(255,255,100,220));
            star.setRotation(sf::degrees(a * 57.3f));
            target.draw(star);
        }
    }

    // ── SHRUNK: small size indicator ──────────────────────────
    if (m_shrinkTimer > 0.0f) {
        float pulse = std::sin(t * 4.f) * 0.3f + 0.7f;
        sf::CircleShape ring(10.f); ring.setOrigin({10.f,10.f});
        ring.setPosition(sp);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(220,100,220, static_cast<uint8_t>(160*pulse)));
        ring.setOutlineThickness(2.f);
        target.draw(ring);
    }

    // ── SHIELD ACTIVE: hex glow around body ───────────────────
    if (m_shieldTimer > 0.0f) {
        float pulse = std::sin(t * 6.f) * 0.3f + 0.7f;
        sf::CircleShape shield(26.f, 6); shield.setOrigin({26.f,26.f});
        shield.setPosition(sp);
        shield.setFillColor(sf::Color(80,120,255, static_cast<uint8_t>(40*pulse)));
        shield.setOutlineColor(sf::Color(120,180,255, static_cast<uint8_t>(200*pulse)));
        shield.setOutlineThickness(2.f);
        shield.setRotation(sf::degrees(t * 20.f));
        target.draw(shield);
    }

    // ── MIND CONTROLLED: spiral eyes ──────────────────────────
    if (m_mindControlled) {
        float spin = t * 5.f;
        for (int i = 0; i < 3; i++) {
            float a = spin + i * 2.094f;
            float r = 3.f + i * 2.f;
            float ox = std::cos(a) * r;
            float oy = std::sin(a) * r;
            sf::CircleShape dot(1.5f); dot.setOrigin({1.5f,1.5f});
            dot.setPosition({sp.x + ox, sp.y - 28.f + oy});
            dot.setFillColor(sf::Color(200,100,255,220));
            target.draw(dot);
        }
    }

    // ── HEALING: rising green crosses ─────────────────────────
    if (m_healTimer > 0.0f) {
        for (int i = 0; i < 2; i++) {
            float phase = std::fmod(t * 0.8f + i * 0.5f, 1.0f);
            float ox = (i == 0 ? -8.f : 8.f);
            float oy = -phase * 25.f;
            float alpha = (phase < 0.7f) ? 1.f : (1.f - phase) / 0.3f;
            // Small + cross
            sf::VertexArray cross(sf::PrimitiveType::Lines, 4);
            cross[0] = sf::Vertex{{sp.x + ox - 3.f, sp.y + oy},
                sf::Color(80,255,80, static_cast<uint8_t>(220*alpha))};
            cross[1] = sf::Vertex{{sp.x + ox + 3.f, sp.y + oy},
                sf::Color(80,255,80, static_cast<uint8_t>(220*alpha))};
            cross[2] = sf::Vertex{{sp.x + ox, sp.y + oy - 3.f},
                sf::Color(80,255,80, static_cast<uint8_t>(220*alpha))};
            cross[3] = sf::Vertex{{sp.x + ox, sp.y + oy + 3.f},
                sf::Color(80,255,80, static_cast<uint8_t>(220*alpha))};
            target.draw(cross);
        }
    }

    // ── FREEZE STACKS: frost dots around player ────────────────────
    if (m_freezeStacks > 0 && m_timeSlowTimer <= 0.0f) {
        for (int i = 0; i < m_freezeStacks; i++) {
            float a = static_cast<float>(i) / 10.f * 6.28318f;
            float cx = sp.x + std::cos(a) * 20.f;
            float cy = sp.y + std::sin(a) * 20.f;
            sf::CircleShape frost(2.0f); frost.setOrigin({2.0f,2.0f});
            frost.setPosition({cx, cy});
            uint8_t alpha = static_cast<uint8_t>(100 + m_freezeStacks * 15);
            frost.setFillColor(sf::Color(140,200,255, alpha));
            frost.setOutlineColor(sf::Color(200,230,255, alpha));
            frost.setOutlineThickness(0.5f);
            target.draw(frost);
        }
    }
}

void StickFigure::drawHitFlashes(sf::RenderTarget& target) const {
    for (int i = 0; i < BODY_PART_COUNT; i++) {
        if (m_partFlash[i] <= 0.0f) continue;
        BodyPart part = static_cast<BodyPart>(i);
        b2Vec2 wp = getBodyPartPosition(part);
        sf::Vector2f sp = toScreen(wp);
        float alpha = m_partFlash[i] / 0.25f; // 0..1

        // Flash circle on the hit body part
        float r = getBodyPartRadius(part) * PPM;
        sf::CircleShape flash(r);
        flash.setOrigin({r, r});
        flash.setPosition(sp);
        flash.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(alpha * 120)));
        flash.setOutlineColor(sf::Color(255, 50, 50, static_cast<uint8_t>(alpha * 200)));
        flash.setOutlineThickness(2.0f);
        target.draw(flash);

        // "HEADSHOT!" or part name text for headshots
        if (part == BodyPart::Head && alpha > 0.5f) {
            // Draw a small red circle burst for headshot emphasis
            float burstR = r * (2.0f - alpha);
            sf::CircleShape burst(burstR);
            burst.setOrigin({burstR, burstR});
            burst.setPosition(sp);
            burst.setFillColor(sf::Color::Transparent);
            burst.setOutlineColor(sf::Color(255, 0, 0, static_cast<uint8_t>(alpha * 180)));
            burst.setOutlineThickness(2.5f);
            target.draw(burst);
        }
    }
}

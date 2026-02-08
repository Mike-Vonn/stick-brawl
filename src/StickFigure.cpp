#include "StickFigure.h"
#include <cmath>
#include <iostream>

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
    b2WorldId worldId = physics.getWorldId();

    // --- Torso ---
    {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = {spawnX, spawnY};
        bd.fixedRotation = true;
        m_torso = b2CreateBody(worldId, &bd);

        b2Polygon box = b2MakeBox(m_config.bodyWidth / 2.0f, m_config.bodyHeight / 4.0f);
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 2.0f;
        sd.material.friction = 0.4f;
        sd.filter.categoryBits = CAT_PLAYER;
        sd.filter.maskBits = CAT_PLATFORM | CAT_PROJECTILE | CAT_PICKUP;
        b2CreatePolygonShape(m_torso, &sd, &box);
    }

    // --- Head ---
    {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = {spawnX, spawnY + m_config.bodyHeight / 4.0f + m_config.headRadius + 0.05f};
        m_head = b2CreateBody(worldId, &bd);

        b2Circle circle = {{0.0f, 0.0f}, m_config.headRadius};
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 1.0f;
        sd.filter.categoryBits = CAT_PLAYER;
        sd.filter.maskBits = CAT_PLATFORM | CAT_PROJECTILE;
        b2CreateCircleShape(m_head, &sd, &circle);

        b2RevoluteJointDef jd = b2DefaultRevoluteJointDef();
        jd.bodyIdA = m_torso; jd.bodyIdB = m_head;
        jd.localAnchorA = {0.0f, m_config.bodyHeight / 4.0f};
        jd.localAnchorB = {0.0f, -m_config.headRadius};
        jd.enableLimit = true; jd.lowerAngle = -0.2f; jd.upperAngle = 0.2f;
        m_neckJoint = b2CreateRevoluteJoint(worldId, &jd);
    }

    // --- Arms ---
    auto createArm = [&](float side, b2BodyId& armBody, b2JointId& joint) {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = {spawnX + side * m_config.limbLength * 0.5f, spawnY + m_config.bodyHeight / 6.0f};
        armBody = b2CreateBody(worldId, &bd);
        b2Polygon box = b2MakeBox(m_config.limbLength / 2.0f, m_config.limbWidth / 2.0f);
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 0.5f; sd.filter.categoryBits = CAT_PLAYER; sd.filter.maskBits = CAT_PLATFORM;
        b2CreatePolygonShape(armBody, &sd, &box);
        b2RevoluteJointDef jd = b2DefaultRevoluteJointDef();
        jd.bodyIdA = m_torso; jd.bodyIdB = armBody;
        jd.localAnchorA = {side * m_config.bodyWidth / 2.0f, m_config.bodyHeight / 6.0f};
        jd.localAnchorB = {-side * m_config.limbLength / 2.0f, 0.0f};
        jd.enableLimit = true; jd.lowerAngle = -1.5f; jd.upperAngle = 1.5f;
        joint = b2CreateRevoluteJoint(worldId, &jd);
    };
    createArm(-1.0f, m_leftArm, m_leftShoulderJoint);
    createArm(1.0f, m_rightArm, m_rightShoulderJoint);

    // --- Legs ---
    auto createLeg = [&](float side, b2BodyId& legBody, b2JointId& joint) {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = {spawnX + side * 0.1f, spawnY - m_config.bodyHeight / 4.0f - m_config.limbLength * 0.4f};
        legBody = b2CreateBody(worldId, &bd);
        b2Polygon box = b2MakeBox(m_config.limbWidth / 2.0f, m_config.limbLength / 2.0f);
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 0.8f; sd.filter.categoryBits = CAT_PLAYER; sd.filter.maskBits = CAT_PLATFORM;
        b2CreatePolygonShape(legBody, &sd, &box);
        b2RevoluteJointDef jd = b2DefaultRevoluteJointDef();
        jd.bodyIdA = m_torso; jd.bodyIdB = legBody;
        jd.localAnchorA = {side * 0.1f, -m_config.bodyHeight / 4.0f};
        jd.localAnchorB = {0.0f, m_config.limbLength / 2.0f};
        jd.enableLimit = true; jd.lowerAngle = -0.8f; jd.upperAngle = 0.8f;
        joint = b2CreateRevoluteJoint(worldId, &jd);
    };
    createLeg(-1.0f, m_leftLeg, m_leftHipJoint);
    createLeg(1.0f, m_rightLeg, m_rightHipJoint);
}

bool StickFigure::isOnGround() const {
    b2Vec2 pos = b2Body_GetPosition(m_torso);
    b2Vec2 origin = {pos.x, pos.y - m_config.bodyHeight / 4.0f - 0.05f};
    b2Vec2 translation = {0.0f, -0.5f};
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.categoryBits = CAT_PLATFORM;
    filter.maskBits = CAT_PLATFORM;
    b2RayResult result = b2World_CastRayClosest(m_physics->getWorldId(), origin, translation, filter);
    return result.hit;
}

void StickFigure::moveLeft()  { m_facingDir = -1; b2Vec2 v = b2Body_GetLinearVelocity(m_torso); b2Body_SetLinearVelocity(m_torso, {-m_moveSpeed, v.y}); }
void StickFigure::moveRight() { m_facingDir =  1; b2Vec2 v = b2Body_GetLinearVelocity(m_torso); b2Body_SetLinearVelocity(m_torso, { m_moveSpeed, v.y}); }
void StickFigure::stopMoving(){ b2Vec2 v = b2Body_GetLinearVelocity(m_torso); b2Body_SetLinearVelocity(m_torso, {v.x * 0.8f, v.y}); }

void StickFigure::jump() {
    if (isOnGround()) {
        b2Vec2 v = b2Body_GetLinearVelocity(m_torso);
        b2Body_SetLinearVelocity(m_torso, {v.x, 0.0f});
        b2Body_ApplyLinearImpulseToCenter(m_torso, {0.0f, m_jumpForce * 2.0f}, true);
    }
}

void StickFigure::aimUp()    { m_aimAngle = std::min(m_aimAngle + 0.05f,  1.2f); }
void StickFigure::aimDown()  { m_aimAngle = std::max(m_aimAngle - 0.05f, -1.2f); }
void StickFigure::resetAim() { m_aimAngle *= 0.9f; } // slowly return to center

bool StickFigure::canAttack() const {
    if (m_attackCooldown > 0.0f) return false;
    if (m_weapon.ammo >= 0 && m_currentAmmo <= 0) return false;
    return true;
}

void StickFigure::attack() {
    m_attackCooldown = m_weapon.attackRate;
    m_attackAnimTimer = 0.2f;
    if (m_currentAmmo > 0) m_currentAmmo--;
}

void StickFigure::equipWeapon(const WeaponData& weapon) {
    m_weapon = weapon;
    m_currentAmmo = weapon.ammo;
}

void StickFigure::takeDamage(float amount, float knockbackX, float knockbackY) {
    m_health -= amount;
    if (m_health < 0.0f) m_health = 0.0f;
    m_damageFlashTimer = 0.15f;
    b2Body_ApplyLinearImpulseToCenter(m_torso, {knockbackX, knockbackY}, true);
}

void StickFigure::applyPoison(float dps, float duration) {
    m_poisonDps = dps;
    m_poisonTimer = duration;
    m_poisonTickTimer = 0.0f;
}

void StickFigure::respawn(float x, float y) {
    m_health = m_maxHealth;
    m_poisonTimer = 0.0f;
    m_aimAngle = 0.0f;

    b2Rot zeroRot = b2MakeRot(0.0f);
    b2Vec2 zero = {0.0f, 0.0f};

    // Torso
    b2Body_SetTransform(m_torso, {x, y}, zeroRot);
    b2Body_SetLinearVelocity(m_torso, zero);
    b2Body_SetAngularVelocity(m_torso, 0.0f);

    // Head
    b2Body_SetTransform(m_head, {x, y + m_config.bodyHeight / 4.0f + m_config.headRadius + 0.05f}, zeroRot);
    b2Body_SetLinearVelocity(m_head, zero);
    b2Body_SetAngularVelocity(m_head, 0.0f);

    // Left arm
    b2Body_SetTransform(m_leftArm, {x - m_config.limbLength * 0.5f, y + m_config.bodyHeight / 6.0f}, zeroRot);
    b2Body_SetLinearVelocity(m_leftArm, zero);
    b2Body_SetAngularVelocity(m_leftArm, 0.0f);

    // Right arm
    b2Body_SetTransform(m_rightArm, {x + m_config.limbLength * 0.5f, y + m_config.bodyHeight / 6.0f}, zeroRot);
    b2Body_SetLinearVelocity(m_rightArm, zero);
    b2Body_SetAngularVelocity(m_rightArm, 0.0f);

    // Left leg
    b2Body_SetTransform(m_leftLeg, {x - 0.1f, y - m_config.bodyHeight / 4.0f - m_config.limbLength * 0.4f}, zeroRot);
    b2Body_SetLinearVelocity(m_leftLeg, zero);
    b2Body_SetAngularVelocity(m_leftLeg, 0.0f);

    // Right leg
    b2Body_SetTransform(m_rightLeg, {x + 0.1f, y - m_config.bodyHeight / 4.0f - m_config.limbLength * 0.4f}, zeroRot);
    b2Body_SetLinearVelocity(m_rightLeg, zero);
    b2Body_SetAngularVelocity(m_rightLeg, 0.0f);

    m_weapon = WeaponData{};
    m_currentAmmo = -1;
}

void StickFigure::update(float dt) {
    if (m_attackCooldown > 0.0f) m_attackCooldown -= dt;
    if (m_attackAnimTimer > 0.0f) m_attackAnimTimer -= dt;
    if (m_damageFlashTimer > 0.0f) m_damageFlashTimer -= dt;
    m_animTime += dt;

    // Respawn delay
    if (m_waitingToRespawn) {
        m_respawnTimer -= dt;
        if (m_respawnTimer <= 0.0f) {
            m_waitingToRespawn = false;
            respawn(m_pendingRespawnX, m_pendingRespawnY);
        }
        return; // skip other updates while waiting
    }

    // Poison tick
    if (m_poisonTimer > 0.0f) {
        m_poisonTimer -= dt;
        m_poisonTickTimer += dt;
        if (m_poisonTickTimer >= 0.5f) { // tick every 0.5s
            m_poisonTickTimer -= 0.5f;
            m_health -= m_poisonDps * 0.5f;
            if (m_health < 0.0f) m_health = 0.0f;
        }
    }
}

void StickFigure::startRespawnTimer(float delay, float x, float y) {
    m_waitingToRespawn = true;
    m_respawnTimer = delay;
    m_pendingRespawnX = x;
    m_pendingRespawnY = y;
    // Move body off-screen while waiting
    b2Body_SetLinearVelocity(m_torso, {0.0f, 0.0f});
    b2Body_SetTransform(m_torso, {x, -100.0f}, b2MakeRot(0.0f));
}

b2Vec2 StickFigure::getPosition() const { return b2Body_GetPosition(m_torso); }

void StickFigure::draw(sf::RenderTarget& target) const {
    if (!isAlive()) return;

    switch (m_charType) {
        case CharacterType::Cat:   drawCat(target); break;
        case CharacterType::Cobra: drawCobra(target); break;
        default:                   drawStick(target); break;
    }

    if (m_attackAnimTimer > 0.0f) drawAttackEffect(target);

    // Draw aim indicator for ranged weapons
    if (m_weapon.type != WeaponType::Melee) drawAimIndicator(target);

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
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;

    sf::CircleShape headShape(m_config.headRadius * PPM);
    headShape.setOrigin({m_config.headRadius * PPM, m_config.headRadius * PPM});
    headShape.setPosition(toScreen(b2Body_GetPosition(m_head)));
    headShape.setFillColor(sf::Color::Transparent);
    headShape.setOutlineColor(dc);
    headShape.setOutlineThickness(2.0f);
    target.draw(headShape);

    auto drawLimb = [&](b2Vec2 s, b2Vec2 e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{toScreen(s), dc};
        line[1] = sf::Vertex{toScreen(e), dc};
        target.draw(line);
    };

    b2Vec2 tp = b2Body_GetPosition(m_torso);
    b2Vec2 tTop = {tp.x, tp.y + m_config.bodyHeight / 4.0f};
    b2Vec2 tBot = {tp.x, tp.y - m_config.bodyHeight / 4.0f};
    drawLimb(tTop, tBot);
    b2Vec2 shoulder = {tp.x, tp.y + m_config.bodyHeight / 6.0f};
    drawLimb(shoulder, b2Body_GetPosition(m_leftArm));
    drawLimb(shoulder, b2Body_GetPosition(m_rightArm));
    drawLimb(tBot, b2Body_GetPosition(m_leftLeg));
    drawLimb(tBot, b2Body_GetPosition(m_rightLeg));
}

void StickFigure::drawCat(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);

    // Body
    sf::RectangleShape body({28.0f, 16.0f});
    body.setOrigin({14.0f, 8.0f}); body.setPosition(c);
    body.setFillColor(dc); body.setOutlineColor(sf::Color::Black); body.setOutlineThickness(1.0f);
    target.draw(body);

    // Head
    sf::CircleShape head(10.0f);
    head.setOrigin({10.0f, 10.0f});
    head.setPosition({c.x + dir * 19.0f, c.y - 4.0f});
    head.setFillColor(dc); head.setOutlineColor(sf::Color::Black); head.setOutlineThickness(1.0f);
    target.draw(head);

    sf::Vector2f hc = head.getPosition();
    // Ears
    for (float s : {-1.0f, 1.0f}) {
        sf::ConvexShape ear(3);
        ear.setPoint(0, {hc.x + s * 5.0f, hc.y - 8.0f});
        ear.setPoint(1, {hc.x + s * 2.0f, hc.y - 16.0f});
        ear.setPoint(2, {hc.x + s * 8.0f, hc.y - 13.0f});
        ear.setFillColor(dc); ear.setOutlineColor(sf::Color::Black); ear.setOutlineThickness(1.0f);
        target.draw(ear);
    }
    // Eyes
    for (float s : {-1.0f, 1.0f}) {
        sf::CircleShape eye(2.0f); eye.setOrigin({2.0f, 2.0f});
        eye.setPosition({hc.x + dir * 3.0f + s * 3.0f, hc.y - 2.0f});
        eye.setFillColor(sf::Color::Black); target.draw(eye);
    }
    // Nose + Whiskers
    sf::CircleShape nose(1.5f); nose.setOrigin({1.5f, 1.5f});
    nose.setPosition({hc.x + dir * 7.0f, hc.y + 1.0f});
    nose.setFillColor(sf::Color(200, 100, 100)); target.draw(nose);
    for (float wy : {-1.0f, 0.0f, 1.0f}) {
        sf::VertexArray w(sf::PrimitiveType::Lines, 2);
        w[0] = sf::Vertex{{hc.x + dir * 8.0f, hc.y + 1.0f + wy * 2.0f}, sf::Color::Black};
        w[1] = sf::Vertex{{hc.x + dir * 20.0f, hc.y + 1.0f + wy * 5.0f}, sf::Color::Black};
        target.draw(w);
    }
    // Tail
    sf::VertexArray tail(sf::PrimitiveType::LineStrip, 4);
    float tx = c.x - dir * 14.0f;
    tail[0] = sf::Vertex{{tx, c.y}, dc};
    tail[1] = sf::Vertex{{tx - dir * 8.0f, c.y - 8.0f}, dc};
    tail[2] = sf::Vertex{{tx - dir * 12.0f, c.y - 16.0f}, dc};
    tail[3] = sf::Vertex{{tx - dir * 8.0f, c.y - 22.0f}, dc};
    target.draw(tail);
    // Legs
    for (float lx : {-0.3f, -0.1f, 0.1f, 0.3f}) {
        sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
        leg[0] = sf::Vertex{{c.x + lx * 28.0f, c.y + 8.0f}, dc};
        leg[1] = sf::Vertex{{c.x + lx * 28.0f, c.y + 18.0f}, dc};
        target.draw(leg);
    }
}

void StickFigure::drawCobra(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);
    float t = m_animTime;

    // Velocity-based wiggle speed: faster movement = faster wiggle
    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    float wiggleSpeed = 4.0f + speed * 1.5f;
    float wiggleAmp = 3.0f + speed * 0.8f;

    // --- Coiled body: a spiral of line segments that wiggle ---
    // Draw 2.5 loops of a coil sitting at the base
    constexpr int coilSegs = 28;
    constexpr float coilLoops = 2.5f;
    constexpr float coilRadiusX = 14.0f;
    constexpr float coilRadiusY = 6.0f;
    sf::Vector2f coilCenter = {c.x - dir * 2.0f, c.y + 10.0f};

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
        py -= frac * 8.0f;

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
    constexpr float neckHeight = 38.0f;
    sf::Vector2f neckBase = {c.x, c.y + 2.0f};

    sf::VertexArray neckLine(sf::PrimitiveType::LineStrip, neckSegs + 1);
    sf::Vector2f neckTop;
    for (int i = 0; i <= neckSegs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(neckSegs);
        // S-curve wiggle that travels up the neck
        float sway = std::sin(t * wiggleSpeed * 0.8f + frac * 3.5f) * wiggleAmp * 0.6f * (1.0f - frac * 0.3f);
        float px = neckBase.x + dir * frac * 6.0f + sway;
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
    float hoodSway = std::sin(t * wiggleSpeed * 0.6f) * 2.0f;
    float hx = neckTop.x + dir * 2.0f + hoodSway * 0.3f;
    float hy = neckTop.y;

    sf::ConvexShape hood(7);
    hood.setPoint(0, {hx - 14.0f,             hy + 6.0f});
    hood.setPoint(1, {hx - 12.0f + hoodSway,  hy - 4.0f});
    hood.setPoint(2, {hx - 6.0f,              hy - 10.0f});
    hood.setPoint(3, {hx,                     hy - 13.0f});
    hood.setPoint(4, {hx + 6.0f,              hy - 10.0f});
    hood.setPoint(5, {hx + 12.0f - hoodSway,  hy - 4.0f});
    hood.setPoint(6, {hx + 14.0f,             hy + 6.0f});
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
    sf::CircleShape head(6.0f);
    head.setOrigin({6.0f, 6.0f});
    head.setPosition({hx + dir * 2.0f, hy - 11.0f});
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

void StickFigure::drawAttackEffect(sf::RenderTarget& target) const {
    sf::Vector2f sp = toScreen(getPosition());
    float dir = static_cast<float>(m_facingDir);
    float prog = 1.0f - (m_attackAnimTimer / 0.2f);

    if (m_weapon.type == WeaponType::Melee) {
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

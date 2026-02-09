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
        sd.filter.maskBits = CAT_PLATFORM | CAT_PROJECTILE | CAT_PICKUP | CAT_PLAYER;
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
        sd.density = 0.8f; sd.filter.categoryBits = CAT_PLAYER; sd.filter.maskBits = CAT_PLATFORM | CAT_PLAYER;
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

void StickFigure::teleportTo(float x, float y) {
    // Compute offset from current position to target
    b2Vec2 curPos = b2Body_GetPosition(m_torso);
    float dx = x - curPos.x;
    float dy = y - curPos.y;

    // Teleport each body by the same offset, preserving velocity
    auto shift = [&](b2BodyId body) {
        b2Vec2 p = b2Body_GetPosition(body);
        b2Rot r = b2Body_GetRotation(body);
        b2Body_SetTransform(body, {p.x + dx, p.y + dy}, r);
        // velocity is preserved automatically
    };

    shift(m_torso);
    shift(m_head);
    shift(m_leftArm);
    shift(m_rightArm);
    shift(m_leftLeg);
    shift(m_rightLeg);
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

void StickFigure::drawUnicorn(sf::RenderTarget& target) const {
    sf::Color dc = (m_damageFlashTimer > 0.0f) ? sf::Color::White : m_color;
    b2Vec2 tp = b2Body_GetPosition(m_torso);
    sf::Vector2f c = toScreen(tp);
    float dir = static_cast<float>(m_facingDir);
    float t = m_animTime;

    // Mane shimmer colors
    auto rainbow = [&](float phase) -> sf::Color {
        float r = std::sin(t * 2.0f + phase) * 0.5f + 0.5f;
        float g = std::sin(t * 2.0f + phase + 2.094f) * 0.5f + 0.5f;
        float b = std::sin(t * 2.0f + phase + 4.189f) * 0.5f + 0.5f;
        return {static_cast<uint8_t>(r * 255), static_cast<uint8_t>(g * 255),
                static_cast<uint8_t>(b * 255), 255};
    };

    // --- Body (barrel) ---
    sf::RectangleShape body({36.0f, 20.0f});
    body.setOrigin({18.0f, 10.0f});
    body.setPosition(c);
    body.setFillColor(dc);
    body.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    body.setOutlineThickness(1.0f);
    target.draw(body);

    // --- Legs (4 legs with slight animation) ---
    float gallop = std::sin(t * 8.0f);
    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x);
    float legAnim = speed > 1.0f ? gallop * 6.0f : 0.0f;
    float legOffsets[4] = {-0.35f, -0.12f, 0.12f, 0.35f};
    float legPhases[4] = {0.0f, 3.14159f, 0.0f, 3.14159f}; // diagonal pairs
    for (int i = 0; i < 4; i++) {
        float lx = c.x + legOffsets[i] * 36.0f;
        float anim = speed > 1.0f ? std::sin(t * 8.0f + legPhases[i]) * 6.0f : 0.0f;
        sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
        leg[0] = sf::Vertex{{lx, c.y + 10.0f}, dc};
        leg[1] = sf::Vertex{{lx + anim * 0.3f, c.y + 24.0f}, dc};
        target.draw(leg);
        // Hooves
        sf::CircleShape hoof(2.5f);
        hoof.setOrigin({2.5f, 2.5f});
        hoof.setPosition({lx + anim * 0.3f, c.y + 25.0f});
        hoof.setFillColor(sf::Color(60, 60, 60));
        target.draw(hoof);
    }

    // --- Neck (angled up from front of body) ---
    float neckBaseX = c.x + dir * 18.0f;
    float neckBaseY = c.y - 6.0f;
    float neckTopX = neckBaseX + dir * 12.0f;
    float neckTopY = neckBaseY - 22.0f;
    // Thick neck with convex shape
    sf::ConvexShape neck(4);
    neck.setPoint(0, {neckBaseX - dir * 5.0f, neckBaseY});
    neck.setPoint(1, {neckBaseX + dir * 3.0f, neckBaseY});
    neck.setPoint(2, {neckTopX + dir * 2.0f, neckTopY + 4.0f});
    neck.setPoint(3, {neckTopX - dir * 4.0f, neckTopY + 4.0f});
    neck.setFillColor(dc);
    target.draw(neck);

    // --- Head (elongated oval) ---
    float headX = neckTopX + dir * 6.0f;
    float headY = neckTopY;
    sf::CircleShape headShape(8.0f);
    headShape.setScale({1.4f, 1.0f});
    headShape.setOrigin({8.0f, 8.0f});
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

    // Darker belly color
    sf::Color belly(
        static_cast<uint8_t>(std::min(255, dc.r + 40)),
        static_cast<uint8_t>(std::min(255, dc.g + 50)),
        static_cast<uint8_t>(std::min(255, dc.b + 20)));

    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x);

    // --- Tail (thick, segmented, swishing) ---
    float tailBaseX = c.x - dir * 20.0f;
    float tailBaseY = c.y + 2.0f;
    constexpr int tailSegs = 10;
    sf::VertexArray tail(sf::PrimitiveType::LineStrip, tailSegs + 1);
    float swish = speed > 1.0f ? std::sin(t * 6.0f) * 8.0f : std::sin(t * 1.5f) * 3.0f;
    for (int i = 0; i <= tailSegs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(tailSegs);
        float wave = std::sin(t * 3.0f + frac * 4.0f) * swish * frac;
        float tx = tailBaseX - dir * frac * 30.0f;
        float ty = tailBaseY + frac * 6.0f + wave;
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
    body.setPoint(0, {c.x - dir * 20.0f, c.y - 8.0f});
    body.setPoint(1, {c.x + dir * 12.0f, c.y - 10.0f});
    body.setPoint(2, {c.x + dir * 20.0f, c.y - 6.0f});
    body.setPoint(3, {c.x + dir * 20.0f, c.y + 8.0f});
    body.setPoint(4, {c.x - dir * 10.0f, c.y + 10.0f});
    body.setPoint(5, {c.x - dir * 20.0f, c.y + 6.0f});
    body.setFillColor(dc);
    body.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    body.setOutlineThickness(1.0f);
    target.draw(body);

    // Belly stripe
    sf::ConvexShape bellyShape(4);
    bellyShape.setPoint(0, {c.x - dir * 14.0f, c.y + 2.0f});
    bellyShape.setPoint(1, {c.x + dir * 14.0f, c.y + 1.0f});
    bellyShape.setPoint(2, {c.x + dir * 12.0f, c.y + 8.0f});
    bellyShape.setPoint(3, {c.x - dir * 10.0f, c.y + 9.0f});
    bellyShape.setFillColor(belly);
    target.draw(bellyShape);

    // Scutes (back ridges)
    for (int i = 0; i < 6; i++) {
        float sx = c.x - dir * 14.0f + dir * static_cast<float>(i) * 6.0f;
        sf::ConvexShape scute(3);
        scute.setPoint(0, {sx - 2.0f, c.y - 8.0f});
        scute.setPoint(1, {sx, c.y - 13.0f});
        scute.setPoint(2, {sx + 2.0f, c.y - 8.0f});
        scute.setFillColor(sf::Color(dc.r * 3 / 4, dc.g * 3 / 4, dc.b * 3 / 4));
        target.draw(scute);
    }

    // --- Legs (4 stubby legs) ---
    float legAnim = speed > 1.0f ? std::sin(t * 8.0f) * 4.0f : 0.0f;
    float legPositions[4] = {-0.30f, -0.10f, 0.15f, 0.35f};
    float legPhases[4] = {0.0f, 3.14159f, 0.0f, 3.14159f};
    for (int i = 0; i < 4; i++) {
        float lx = c.x + dir * legPositions[i] * 45.0f;
        float anim = speed > 1.0f ? std::sin(t * 8.0f + legPhases[i]) * 4.0f : 0.0f;
        sf::RectangleShape leg({4.0f, 14.0f});
        leg.setOrigin({2.0f, 0.0f});
        leg.setPosition({lx, c.y + 8.0f});
        leg.setRotation(sf::degrees(anim));
        leg.setFillColor(dc);
        target.draw(leg);
        // Claws
        for (float cx2 : {-1.5f, 0.0f, 1.5f}) {
            sf::CircleShape claw(1.0f);
            claw.setOrigin({1.0f, 1.0f});
            claw.setPosition({lx + cx2, c.y + 22.0f});
            claw.setFillColor(sf::Color(60, 60, 50));
            target.draw(claw);
        }
    }

    // --- Head / Snout (the distinctive long jaw) ---
    float headX = c.x + dir * 20.0f;
    float headY = c.y - 4.0f;

    // Jaw opening animation when attacking
    float jawOpen = 0.0f;
    if (m_attackAnimTimer > 0.0f) {
        float prog = m_attackAnimTimer / 0.2f;
        jawOpen = std::sin(prog * 3.14159f) * 20.0f; // opens then snaps shut
    }

    // Upper jaw
    sf::ConvexShape upperJaw(5);
    upperJaw.setPoint(0, {headX, headY - 6.0f});
    upperJaw.setPoint(1, {headX + dir * 8.0f, headY - 7.0f - jawOpen * 0.3f});
    upperJaw.setPoint(2, {headX + dir * 28.0f, headY - 3.0f - jawOpen * 0.5f});
    upperJaw.setPoint(3, {headX + dir * 30.0f, headY - jawOpen * 0.2f});
    upperJaw.setPoint(4, {headX, headY + 2.0f});
    upperJaw.setFillColor(dc);
    upperJaw.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    upperJaw.setOutlineThickness(1.0f);
    target.draw(upperJaw);

    // Lower jaw
    sf::ConvexShape lowerJaw(4);
    lowerJaw.setPoint(0, {headX, headY + 2.0f});
    lowerJaw.setPoint(1, {headX + dir * 26.0f, headY + 2.0f + jawOpen * 0.5f});
    lowerJaw.setPoint(2, {headX + dir * 24.0f, headY + 7.0f + jawOpen * 0.4f});
    lowerJaw.setPoint(3, {headX - dir * 2.0f, headY + 8.0f});
    lowerJaw.setFillColor(belly);
    lowerJaw.setOutlineColor(sf::Color(dc.r / 2, dc.g / 2, dc.b / 2));
    lowerJaw.setOutlineThickness(1.0f);
    target.draw(lowerJaw);

    // Teeth (upper)
    for (int i = 0; i < 5; i++) {
        float tx = headX + dir * (6.0f + static_cast<float>(i) * 5.0f);
        float ty = headY + 1.0f - jawOpen * 0.15f;
        sf::ConvexShape tooth(3);
        tooth.setPoint(0, {tx - 1.0f, ty});
        tooth.setPoint(1, {tx, ty + 4.0f + jawOpen * 0.1f});
        tooth.setPoint(2, {tx + 1.0f, ty});
        tooth.setFillColor(sf::Color(240, 235, 210));
        target.draw(tooth);
    }
    // Teeth (lower)
    for (int i = 0; i < 4; i++) {
        float tx = headX + dir * (8.0f + static_cast<float>(i) * 5.0f);
        float ty = headY + 3.0f + jawOpen * 0.4f;
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
        nostril.setPosition({headX + dir * 28.0f, headY - 4.0f + ns});
        nostril.setFillColor(sf::Color(dc.r * 3 / 4, dc.g * 3 / 4, dc.b / 2));
        target.draw(nostril);
    }

    // Eyes (menacing, slit pupils on bumps)
    for (float es : {-1.0f, 1.0f}) {
        // Eye bump
        sf::CircleShape eyeBump(3.5f);
        eyeBump.setOrigin({3.5f, 3.5f});
        eyeBump.setPosition({headX + dir * 4.0f + es * 3.0f * (dir > 0 ? 1.0f : -1.0f), headY - 9.0f});
        eyeBump.setFillColor(dc);
        target.draw(eyeBump);
        // Eye
        sf::CircleShape eye(2.5f);
        eye.setOrigin({2.5f, 2.5f});
        eye.setPosition({headX + dir * 4.0f + es * 3.0f * (dir > 0 ? 1.0f : -1.0f), headY - 10.0f});
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
    float dir = static_cast<float>(m_facingDir);
    float t = m_animTime;

    b2Vec2 tp = b2Body_GetPosition(m_torso);
    b2Vec2 hp = b2Body_GetPosition(m_head);
    sf::Vector2f headSc = toScreen(hp);
    sf::Vector2f torsoSc = toScreen(tp);

    b2Vec2 vel = b2Body_GetLinearVelocity(m_torso);
    float speed = std::sqrt(vel.x * vel.x);

    // --- Flowing hair ---
    // Multiple strands from back of head, flowing opposite to facing direction
    constexpr int hairStrands = 8;
    float hairLen = 22.0f;
    for (int i = 0; i < hairStrands; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(hairStrands - 1);
        // Attach points around back half of head
        float attachAngle = (0.6f + frac * 1.8f); // radians, back of head arc
        float ax = headSc.x - dir * std::cos(attachAngle) * m_config.headRadius * PPM * 0.8f;
        float ay = headSc.y - std::sin(attachAngle) * m_config.headRadius * PPM * 0.4f;

        sf::VertexArray strand(sf::PrimitiveType::LineStrip, 6);
        for (int s = 0; s < 6; s++) {
            float sf2 = static_cast<float>(s) / 5.0f;
            float wave = std::sin(t * 3.0f + frac * 2.0f + sf2 * 4.0f) * (3.0f + speed * 0.5f);
            float windPush = -dir * sf2 * (4.0f + speed * 1.5f); // hair blows back
            float sx = ax + windPush + wave * 0.3f;
            float sy = ay + sf2 * hairLen + std::sin(t * 2.0f + frac) * 2.0f * sf2;
            uint8_t alpha = static_cast<uint8_t>(255 * (1.0f - sf2 * 0.3f));
            strand[s] = sf::Vertex{{sx, sy}, sf::Color(dc.r, dc.g, dc.b, alpha)};
        }
        target.draw(strand);
    }

    // --- Head (circle, same as stick but filled) ---
    float headR = m_config.headRadius * PPM;
    sf::CircleShape headShape(headR);
    headShape.setOrigin({headR, headR});
    headShape.setPosition(headSc);
    headShape.setFillColor(sf::Color::Transparent);
    headShape.setOutlineColor(dc);
    headShape.setOutlineThickness(2.0f);
    target.draw(headShape);

    // --- Eyelashes ---
    float eyeX = headSc.x + dir * headR * 0.4f;
    float eyeY = headSc.y - headR * 0.15f;
    // Eye dot
    sf::CircleShape eye(2.0f);
    eye.setOrigin({2.0f, 2.0f});
    eye.setPosition({eyeX, eyeY});
    eye.setFillColor(dc);
    target.draw(eye);
    // Three eyelash lines
    for (int i = 0; i < 3; i++) {
        float angle = -0.5f + static_cast<float>(i) * 0.5f; // fan out upward
        float lashLen = 4.0f + static_cast<float>(i == 1) * 2.0f; // middle is longest
        sf::VertexArray lash(sf::PrimitiveType::Lines, 2);
        lash[0] = sf::Vertex{{eyeX, eyeY - 2.0f}, dc};
        lash[1] = sf::Vertex{{eyeX + std::sin(angle) * lashLen * dir,
                               eyeY - 2.0f - std::cos(angle) * lashLen}, dc};
        target.draw(lash);
    }

    // --- Body line (torso) ---
    b2Vec2 tTop = {tp.x, tp.y + m_config.bodyHeight / 4.0f};
    b2Vec2 tBot = {tp.x, tp.y - m_config.bodyHeight / 4.0f};
    auto drawLine = [&](b2Vec2 s, b2Vec2 e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{toScreen(s), dc};
        line[1] = sf::Vertex{toScreen(e), dc};
        target.draw(line);
    };
    drawLine(tTop, tBot);

    // --- Arms ---
    b2Vec2 shoulder = {tp.x, tp.y + m_config.bodyHeight / 6.0f};
    drawLine(shoulder, b2Body_GetPosition(m_leftArm));
    drawLine(shoulder, b2Body_GetPosition(m_rightArm));

    // --- Triangle skirt ---
    sf::Vector2f waist = toScreen(tBot);
    float skirtWidth = 18.0f;
    float skirtLen = 16.0f;
    // Slight sway animation
    float sway = speed > 1.0f ? std::sin(t * 6.0f) * 3.0f : std::sin(t * 1.5f) * 1.0f;

    sf::ConvexShape skirt(4);
    skirt.setPoint(0, {waist.x - 3.0f, waist.y - 2.0f});
    skirt.setPoint(1, {waist.x + 3.0f, waist.y - 2.0f});
    skirt.setPoint(2, {waist.x + skirtWidth + sway, waist.y + skirtLen});
    skirt.setPoint(3, {waist.x - skirtWidth + sway, waist.y + skirtLen});
    // Slightly lighter fill
    sf::Color skirtColor(
        static_cast<uint8_t>(std::min(255, dc.r + 30)),
        static_cast<uint8_t>(std::min(255, dc.g + 10)),
        static_cast<uint8_t>(std::min(255, dc.b + 40)));
    skirt.setFillColor(skirtColor);
    skirt.setOutlineColor(dc);
    skirt.setOutlineThickness(1.0f);
    target.draw(skirt);

    // Skirt hem detail line
    sf::VertexArray hem(sf::PrimitiveType::Lines, 2);
    hem[0] = sf::Vertex{{waist.x - skirtWidth * 0.8f + sway, waist.y + skirtLen - 2.0f}, dc};
    hem[1] = sf::Vertex{{waist.x + skirtWidth * 0.8f + sway, waist.y + skirtLen - 2.0f}, dc};
    target.draw(hem);

    // --- Legs (below skirt) ---
    sf::Vector2f leftLegSc = toScreen(b2Body_GetPosition(m_leftLeg));
    sf::Vector2f rightLegSc = toScreen(b2Body_GetPosition(m_rightLeg));
    // Legs start at bottom of skirt
    float legStartY = waist.y + skirtLen;
    {
        sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
        leg[0] = sf::Vertex{{leftLegSc.x, legStartY}, dc};
        leg[1] = sf::Vertex{leftLegSc, dc};
        target.draw(leg);
    }
    {
        sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
        leg[0] = sf::Vertex{{rightLegSc.x, legStartY}, dc};
        leg[1] = sf::Vertex{rightLegSc, dc};
        target.draw(leg);
    }

    // --- Purse (hangs from arm, swings during attack) ---
    float purseSwing = 0.0f;
    if (m_attackAnimTimer > 0.0f) {
        float prog = m_attackAnimTimer / 0.2f;
        purseSwing = std::sin(prog * 3.14159f * 2.0f) * 30.0f; // wild swing
    }
    // Purse hangs from the forward arm
    b2Vec2 armPos = (dir > 0) ? b2Body_GetPosition(m_rightArm) : b2Body_GetPosition(m_leftArm);
    sf::Vector2f armSc = toScreen(armPos);
    float purseX = armSc.x + dir * 6.0f + std::sin(t * 2.0f + purseSwing * 0.05f) * 2.0f;
    float purseY = armSc.y + 4.0f;

    // Strap
    sf::VertexArray strap(sf::PrimitiveType::Lines, 2);
    strap[0] = sf::Vertex{armSc, dc};
    strap[1] = sf::Vertex{{purseX, purseY}, dc};
    target.draw(strap);

    // Purse body (small rectangle with flap)
    float purseAngle = purseSwing + std::sin(t * 2.0f) * 5.0f;
    sf::RectangleShape purseBody({10.0f, 8.0f});
    purseBody.setOrigin({5.0f, 0.0f});
    purseBody.setPosition({purseX, purseY});
    purseBody.setRotation(sf::degrees(purseAngle));
    // Purse is a contrasting color
    sf::Color purseColor(
        static_cast<uint8_t>(std::min(255, 255 - dc.r / 2)),
        static_cast<uint8_t>(std::min(255, dc.g / 3)),
        static_cast<uint8_t>(std::min(255, dc.b / 2 + 80)));
    purseBody.setFillColor(purseColor);
    purseBody.setOutlineColor(sf::Color(purseColor.r / 2, purseColor.g / 2, purseColor.b / 2));
    purseBody.setOutlineThickness(1.0f);
    target.draw(purseBody);

    // Clasp
    sf::CircleShape clasp(1.5f);
    clasp.setOrigin({1.5f, 1.5f});
    clasp.setPosition({purseX, purseY + 2.0f});
    clasp.setFillColor(sf::Color(200, 180, 100));
    target.draw(clasp);
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

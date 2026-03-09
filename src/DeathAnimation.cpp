#include "DeathAnimation.h"
#include <cmath>
#include <algorithm>

static constexpr float TWO_PI = 6.283185f;

static sf::Vector2f toScreen(b2Vec2 pos) {
    return {SCREEN_CX + pos.x * PPM, SCREEN_CY - pos.y * PPM};
}

static BodyPlan getBodyPlan(CharacterType ct) {
    switch (ct) {
        case CharacterType::Stick:
        case CharacterType::StickLady:
        case CharacterType::MrDiaperPants:
            return BodyPlan::Humanoid;
        case CharacterType::Cat:
        case CharacterType::Unicorn:
        case CharacterType::Crocodile:
        case CharacterType::Dragon:
            return BodyPlan::Quadruped;
        case CharacterType::Cobra:
            return BodyPlan::Serpentine;
    }
    return BodyPlan::Humanoid;
}

// ─── Blood burst helper ──────────────────────────────────────────────
void DeathAnimationSystem::spawnBloodBurst(DeathEffect& fx, float cx, float cy,
                                           int count, float speed, sf::Color color) {
    // Convert pixel-space speed to world-space (meters/sec)
    float worldSpeed = speed / PPM;
    std::uniform_real_distribution<float> angleDist(0.0f, TWO_PI);
    std::uniform_real_distribution<float> speedDist(worldSpeed * 0.3f, worldSpeed);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.5f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 1.5f);

    for (int i = 0; i < count; i++) {
        float a = angleDist(m_rng);
        float s = speedDist(m_rng);
        Particle p;
        p.x = cx; p.y = cy;
        p.vx = std::cos(a) * s;
        p.vy = std::sin(a) * s;
        p.lifetime = lifeDist(m_rng);
        p.maxLifetime = p.lifetime;
        p.size = sizeDist(m_rng);
        p.color = color;
        p.alive = true;
        fx.particles.push_back(p);
    }
}

// ─── Spawn dispatching ───────────────────────────────────────────────
void DeathAnimationSystem::spawnDeath(Physics& physics, DeathAnimType type,
                                      float x, float y, sf::Color color,
                                      int playerIndex, CharacterType charType,
                                      float knockbackX, float knockbackY) {
    DeathEffect fx;
    fx.type = type;
    fx.x = x; fx.y = y;
    fx.playerColor = color;
    fx.playerIndex = playerIndex;
    fx.charType = charType;

    switch (type) {
        case DeathAnimType::Collapse:
            fx.duration = 2.5f;
            spawnCollapse(physics, fx, knockbackX, knockbackY);
            break;
        case DeathAnimType::Dismember:
            fx.duration = 3.0f;
            spawnDismember(physics, fx, knockbackX, knockbackY);
            break;
        case DeathAnimType::Explode:
            fx.duration = 3.0f;
            spawnExplode(physics, fx, knockbackX, knockbackY);
            break;
        case DeathAnimType::Disintegrate:
            fx.duration = 2.0f;
            spawnDisintegrate(fx);
            break;
        case DeathAnimType::Incinerate:
            fx.duration = 3.0f;
            spawnIncinerate(fx);
            break;
    }

    m_effects.push_back(std::move(fx));
}

// ─── COLLAPSE: body topples sideways ─────────────────────────────────
void DeathAnimationSystem::spawnCollapse(Physics& physics, DeathEffect& fx,
                                         float kbX, float kbY) {
    fx.collapseAngle = 0.0f;
    fx.collapseVelY = 0.0f;
    fx.collapseLanded = false;

    sf::Color blood(180, 0, 0, 220);
    spawnBloodBurst(fx, fx.x, fx.y, 8, 60.0f, blood);
}

// ─── DISMEMBER: head or arm flies off ────────────────────────────────
void DeathAnimationSystem::spawnDismember(Physics& physics, DeathEffect& fx,
                                          float kbX, float kbY) {
    std::uniform_real_distribution<float> upDist(5.0f, 10.0f);
    std::uniform_real_distribution<float> sideDist(-3.0f, 3.0f);
    std::uniform_int_distribution<int> partChoice(0, 2);

    float dir = (kbX >= 0.0f) ? 1.0f : -1.0f;
    int choice = partChoice(m_rng);
    fx.dismemberedPart = choice;

    auto spawnGibBody = [&](float offX, float offY, float hw, float hh,
                            bool circle, float launchX, float launchY) {
        Gib g;
        g.isCircle = circle;
        g.color = fx.playerColor;
        g.lifetime = 3.0f;
        g.maxLifetime = 3.0f;
        g.alive = true;

        if (circle) {
            g.radius = hw;
            g.width = g.height = 0.0f;
            g.bodyId = physics.createDynamicCircle(
                fx.x + offX, fx.y + offY, hw, 0.5f,
                CAT_PROJECTILE, CAT_PLATFORM);
        } else {
            g.width = hw; g.height = hh;
            g.radius = 0.0f;
            g.bodyId = physics.createDynamicBox(
                fx.x + offX, fx.y + offY, hw, hh, 0.5f,
                CAT_PROJECTILE, CAT_PLATFORM);
        }
        b2Body_ApplyLinearImpulseToCenter(g.bodyId, {launchX, launchY}, true);
        b2Body_SetAngularVelocity(g.bodyId, sideDist(m_rng) * 3.0f);
        fx.gibs.push_back(g);
    };

    if (choice == 0) {
        spawnGibBody(0.0f, 0.7f, 0.25f, 0.0f, true,
                     dir * 4.0f + sideDist(m_rng), upDist(m_rng));
    } else {
        float side = (choice == 1) ? -1.0f : 1.0f;
        spawnGibBody(side * 0.4f, 0.3f, 0.3f, 0.04f, false,
                     side * 5.0f + sideDist(m_rng), upDist(m_rng));
    }

    sf::Color blood(200, 0, 0, 240);
    float bloodY = (choice == 0) ? fx.y + 0.5f : fx.y + 0.3f;
    spawnBloodBurst(fx, fx.x, bloodY, 20, 90.0f, blood);

    sf::Color darkBlood(120, 0, 0, 200);
    spawnBloodBurst(fx, fx.x, bloodY, 6, 30.0f, darkBlood);

    fx.collapseAngle = 0.0f;
    fx.collapseVelY = 0.0f;
    fx.collapseLanded = false;
}

// ─── EXPLODE: body blows into many pieces ────────────────────────────
void DeathAnimationSystem::spawnExplode(Physics& physics, DeathEffect& fx,
                                        float kbX, float kbY) {
    std::uniform_real_distribution<float> angleDist(0.0f, TWO_PI);
    std::uniform_real_distribution<float> speedDist(5.0f, 15.0f);
    std::uniform_real_distribution<float> sizeDist(0.06f, 0.18f);
    std::uniform_real_distribution<float> spinDist(-10.0f, 10.0f);

    int gibCount = 8 + (m_rng() % 5);
    for (int i = 0; i < gibCount; i++) {
        float angle = angleDist(m_rng);
        float speed = speedDist(m_rng);
        float sz = sizeDist(m_rng);

        Gib g;
        g.isCircle = (i < 2);
        g.color = fx.playerColor;
        g.lifetime = 2.8f;
        g.maxLifetime = 2.8f;
        g.alive = true;

        float offX = std::cos(angle) * 0.2f;
        float offY = std::sin(angle) * 0.2f;

        if (g.isCircle) {
            g.radius = sz;
            g.width = g.height = 0.0f;
            g.bodyId = physics.createDynamicCircle(
                fx.x + offX, fx.y + offY, sz, 0.3f,
                CAT_PROJECTILE, CAT_PLATFORM);
        } else {
            g.width = sz; g.height = sz * 0.4f;
            g.radius = 0.0f;
            g.bodyId = physics.createDynamicBox(
                fx.x + offX, fx.y + offY, sz, sz * 0.4f, 0.3f,
                CAT_PROJECTILE, CAT_PLATFORM);
        }

        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed + 5.0f;
        b2Body_ApplyLinearImpulseToCenter(g.bodyId, {vx, vy}, true);
        b2Body_SetAngularVelocity(g.bodyId, spinDist(m_rng));

        fx.gibs.push_back(g);
    }

    sf::Color blood(200, 0, 0, 220);
    spawnBloodBurst(fx, fx.x, fx.y, 40, 150.0f, blood);

    sf::Color bone(220, 210, 180, 200);
    spawnBloodBurst(fx, fx.x, fx.y, 10, 100.0f, bone);
}

// ─── DISINTEGRATE: particle cloud (nuke) ─────────────────────────────
void DeathAnimationSystem::spawnDisintegrate(DeathEffect& fx) {
    std::uniform_real_distribution<float> angleDist(0.0f, TWO_PI);
    std::uniform_real_distribution<float> speedDist(30.0f / PPM, 200.0f / PPM);
    std::uniform_real_distribution<float> sizeDist(1.0f, 4.0f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 2.0f);

    for (int i = 0; i < 60; i++) {
        float a = angleDist(m_rng);
        float s = speedDist(m_rng);
        Particle p;
        p.x = fx.x; p.y = fx.y;
        p.vx = std::cos(a) * s;
        p.vy = std::sin(a) * s;
        p.lifetime = lifeDist(m_rng);
        p.maxLifetime = p.lifetime;
        p.size = sizeDist(m_rng);

        if (i < 20)
            p.color = fx.playerColor;
        else if (i < 40)
            p.color = sf::Color(255, static_cast<uint8_t>(100 + (m_rng() % 100)), 0, 220);
        else
            p.color = sf::Color(80, 80, 80, 180);

        p.alive = true;
        fx.particles.push_back(p);
    }
}

// ─── INCINERATE: charcoalize then skeleton ───────────────────────────
void DeathAnimationSystem::spawnIncinerate(DeathEffect& fx) {
    fx.incinerateScale = 1.0f;
    fx.skeletonAngle = 0.0f;
    fx.skeletonLanded = false;
    fx.collapseAngle = 0.0f;
}

// ─── Update ──────────────────────────────────────────────────────────
void DeathAnimationSystem::update(float dt) {
    for (auto& fx : m_effects) {
        if (!fx.alive) continue;
        fx.timer += dt;
        if (fx.timer >= fx.duration) {
            fx.alive = false;
            continue;
        }

        // Update gibs lifetime
        for (auto& g : fx.gibs) {
            if (!g.alive) continue;
            g.lifetime -= dt;
            if (g.lifetime <= 0.0f) g.alive = false;
        }

        // Update particles
        for (auto& p : fx.particles) {
            if (!p.alive) continue;
            p.lifetime -= dt;
            if (p.lifetime <= 0.0f) { p.alive = false; continue; }

            p.vy -= 10.0f * dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
        }

        // Collapse animation (used for Collapse and Dismember body remainder)
        if (fx.type == DeathAnimType::Collapse || fx.type == DeathAnimType::Dismember) {
            if (!fx.collapseLanded) {
                float targetAngle = 1.57f;
                float rotSpeed = 3.0f;
                fx.collapseAngle += rotSpeed * dt;
                if (fx.collapseAngle >= targetAngle) {
                    fx.collapseAngle = targetAngle;
                    fx.collapseLanded = true;
                }
            }
        }

        // Incinerate animation phases
        if (fx.type == DeathAnimType::Incinerate) {
            float t = fx.timer;

            if (t < 0.8f) {
                // Phase 1: Charcoalizing — color change at draw time
            }
            else if (t < 1.5f) {
                // Phase 2: Crumble — figure shrinks, ash particles spawn
                float crumbleT = (t - 0.8f) / 0.7f;
                fx.incinerateScale = 1.0f - crumbleT * 0.8f;

                // Emit ash/ember particles upward
                if (m_rng() % 3 == 0) {
                    std::uniform_real_distribution<float> offX(-0.3f, 0.3f);
                    std::uniform_real_distribution<float> upSpeed(2.0f, 5.0f);
                    std::uniform_real_distribution<float> sideSpeed(-0.5f, 0.5f);
                    std::uniform_real_distribution<float> sizeDist(1.0f, 2.5f);

                    Particle ash;
                    ash.x = fx.x + offX(m_rng);
                    ash.y = fx.y + 0.5f;
                    ash.vx = sideSpeed(m_rng);
                    ash.vy = upSpeed(m_rng);
                    ash.lifetime = 1.2f;
                    ash.maxLifetime = 1.2f;
                    ash.size = sizeDist(m_rng);
                    if (m_rng() % 3 == 0)
                        ash.color = sf::Color(255, static_cast<uint8_t>(80 + m_rng() % 80), 0, 200);
                    else
                        ash.color = sf::Color(60, 60, 60, 180);
                    ash.alive = true;
                    fx.particles.push_back(ash);
                }
            }
            else {
                // Phase 3: Skeleton collapses and fades
                fx.incinerateScale = 0.0f;

                if (!fx.skeletonLanded) {
                    float rotSpeed = 2.5f;
                    fx.skeletonAngle += rotSpeed * dt;
                    if (fx.skeletonAngle >= 1.57f) {
                        fx.skeletonAngle = 1.57f;
                        fx.skeletonLanded = true;
                    }
                }

                // Sparse rising embers
                if (m_rng() % 5 == 0) {
                    std::uniform_real_distribution<float> offX(-0.2f, 0.2f);
                    std::uniform_real_distribution<float> upSpeed(1.5f, 3.5f);
                    Particle ember;
                    ember.x = fx.x + offX(m_rng);
                    ember.y = fx.y;
                    ember.vx = offX(m_rng) * 0.5f;
                    ember.vy = upSpeed(m_rng);
                    ember.lifetime = 1.0f;
                    ember.maxLifetime = 1.0f;
                    ember.size = 1.5f;
                    ember.color = sf::Color(255, static_cast<uint8_t>(100 + m_rng() % 100), 0, 220);
                    ember.alive = true;
                    fx.particles.push_back(ember);
                }
            }
        }
    }
}

// ─── Draw ────────────────────────────────────────────────────────────
void DeathAnimationSystem::draw(sf::RenderTarget& target) const {
    for (const auto& fx : m_effects) {
        if (!fx.alive) continue;

        // Fade out in final 0.5 seconds
        float alpha = 1.0f;
        float remaining = fx.duration - fx.timer;
        if (remaining < 0.5f) alpha = remaining / 0.5f;

        // Draw collapse body (for Collapse and Dismember)
        if (fx.type == DeathAnimType::Collapse || fx.type == DeathAnimType::Dismember) {
            drawCollapse(target, fx, alpha);
        }

        // Draw incinerate effect
        if (fx.type == DeathAnimType::Incinerate) {
            drawIncinerate(target, fx, alpha);
        }

        // Draw physics gibs
        for (const auto& g : fx.gibs) {
            if (!g.alive) continue;
            drawGib(target, g, alpha);
        }

        // Draw particles
        drawParticles(target, fx, alpha);
    }
}

void DeathAnimationSystem::drawGib(sf::RenderTarget& target, const Gib& gib, float effectAlpha) const {
    if (!b2Body_IsValid(gib.bodyId)) return;
    b2Vec2 pos = b2Body_GetPosition(gib.bodyId);
    b2Rot rot = b2Body_GetRotation(gib.bodyId);
    float angle = std::atan2(rot.s, rot.c);

    sf::Vector2f sp = toScreen(pos);

    float lifeRatio = gib.lifetime / gib.maxLifetime;
    uint8_t a = static_cast<uint8_t>(255.0f * std::min(1.0f, lifeRatio * 2.0f) * effectAlpha);
    sf::Color c = gib.color;
    c.a = a;

    if (gib.isCircle) {
        float r = gib.radius * PPM;
        sf::CircleShape shape(r);
        shape.setOrigin({r, r});
        shape.setPosition(sp);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(c);
        shape.setOutlineThickness(2.0f);
        shape.setRotation(sf::radians(angle));
        target.draw(shape);

        if (gib.radius > 0.15f) {
            float es = r * 0.35f;
            sf::VertexArray x1(sf::PrimitiveType::Lines, 2);
            sf::VertexArray x2(sf::PrimitiveType::Lines, 2);
            sf::Color ec(c.r, c.g, c.b, a);
            x1[0] = sf::Vertex{{sp.x - es, sp.y - es}, ec};
            x1[1] = sf::Vertex{{sp.x + es, sp.y + es}, ec};
            x2[0] = sf::Vertex{{sp.x + es, sp.y - es}, ec};
            x2[1] = sf::Vertex{{sp.x - es, sp.y + es}, ec};
            target.draw(x1);
            target.draw(x2);
        }
    } else {
        float w = gib.width * PPM * 2.0f;
        float h = gib.height * PPM * 2.0f;
        sf::RectangleShape shape({w, h});
        shape.setOrigin({w / 2.0f, h / 2.0f});
        shape.setPosition(sp);
        shape.setFillColor(c);
        shape.setOutlineColor(sf::Color(c.r / 2, c.g / 2, c.b / 2, a));
        shape.setOutlineThickness(1.0f);
        shape.setRotation(sf::radians(angle));
        target.draw(shape);
    }
}

// ─── COLLAPSE: dispatch by body plan ─────────────────────────────────
void DeathAnimationSystem::drawCollapse(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const {
    uint8_t a = static_cast<uint8_t>(255.0f * effectAlpha);
    sf::Color c(fx.playerColor.r, fx.playerColor.g, fx.playerColor.b, a);

    BodyPlan plan = getBodyPlan(fx.charType);
    switch (plan) {
        case BodyPlan::Humanoid:
            drawCollapseHumanoid(target, fx, c, fx.collapseAngle, 1.0f);
            break;
        case BodyPlan::Quadruped:
            drawCollapseQuadruped(target, fx, c, fx.collapseAngle, 1.0f);
            break;
        case BodyPlan::Serpentine:
            drawCollapseSerpentine(target, fx, c, fx.collapseAngle, 1.0f);
            break;
    }
}

// ─── Humanoid collapse (Stick, StickLady) ────────────────────────────
void DeathAnimationSystem::drawCollapseHumanoid(sf::RenderTarget& target, const DeathEffect& fx,
                                                 sf::Color c, float angle, float scale) const {
    sf::Vector2f center = toScreen({fx.x, fx.y});

    float bodyH = 1.8f * PPM * 0.5f * scale;
    float headR = 0.25f * PPM * scale;
    float limbLen = 0.6f * PPM * scale;

    sf::Vector2f pivot = {center.x, center.y + bodyH};

    auto rotPoint = [&](float localX, float localY) -> sf::Vector2f {
        float rx = localX * std::cos(angle) - localY * std::sin(angle);
        float ry = localX * std::sin(angle) + localY * std::cos(angle);
        return {pivot.x + rx, pivot.y + ry};
    };

    auto drawLine = [&](sf::Vector2f s, sf::Vector2f e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{s, c};
        line[1] = sf::Vertex{e, c};
        target.draw(line);
    };

    // Torso line
    sf::Vector2f tBot = rotPoint(0.0f, 0.0f);
    sf::Vector2f tTop = rotPoint(0.0f, -bodyH * 2.0f);
    drawLine(tBot, tTop);

    // Head
    bool headDetached = (fx.type == DeathAnimType::Dismember && fx.dismemberedPart == 0);
    if (!headDetached) {
        sf::Vector2f headPos = rotPoint(0.0f, -bodyH * 2.0f - headR);
        sf::CircleShape head(headR);
        head.setOrigin({headR, headR});
        head.setPosition(headPos);
        head.setFillColor(sf::Color::Transparent);
        head.setOutlineColor(c);
        head.setOutlineThickness(2.0f);
        target.draw(head);

        // X eyes
        float es = headR * 0.35f;
        sf::VertexArray x1(sf::PrimitiveType::Lines, 2);
        sf::VertexArray x2(sf::PrimitiveType::Lines, 2);
        x1[0] = sf::Vertex{{headPos.x - es, headPos.y - es}, c};
        x1[1] = sf::Vertex{{headPos.x + es, headPos.y + es}, c};
        x2[0] = sf::Vertex{{headPos.x + es, headPos.y - es}, c};
        x2[1] = sf::Vertex{{headPos.x - es, headPos.y + es}, c};
        target.draw(x1);
        target.draw(x2);
    } else {
        sf::Vector2f neckPos = rotPoint(0.0f, -bodyH * 2.0f);
        sf::CircleShape stump(3.0f * scale);
        stump.setOrigin({3.0f * scale, 3.0f * scale});
        stump.setPosition(neckPos);
        stump.setFillColor(sf::Color(180, 0, 0, c.a));
        target.draw(stump);
    }

    // Arms
    sf::Vector2f shoulder = rotPoint(0.0f, -bodyH * 1.5f);
    sf::Vector2f lArm = rotPoint(-limbLen, -bodyH * 1.2f);
    sf::Vector2f rArm = rotPoint(limbLen, -bodyH * 1.2f);
    bool lArmDetached = (fx.type == DeathAnimType::Dismember && fx.dismemberedPart == 1);
    bool rArmDetached = (fx.type == DeathAnimType::Dismember && fx.dismemberedPart == 2);
    if (!lArmDetached) drawLine(shoulder, lArm);
    if (!rArmDetached) drawLine(shoulder, rArm);
    if (lArmDetached || rArmDetached) {
        sf::CircleShape stump(2.5f * scale);
        stump.setOrigin({2.5f * scale, 2.5f * scale});
        stump.setPosition(shoulder);
        stump.setFillColor(sf::Color(180, 0, 0, c.a));
        target.draw(stump);
    }

    // Legs
    sf::Vector2f lLeg = rotPoint(-limbLen * 0.5f, limbLen * 0.6f);
    sf::Vector2f rLeg = rotPoint(limbLen * 0.5f, limbLen * 0.6f);
    drawLine(tBot, lLeg);
    drawLine(tBot, rLeg);
}

// ─── Quadruped collapse (Cat, Unicorn, Crocodile, Dragon) ────────────
void DeathAnimationSystem::drawCollapseQuadruped(sf::RenderTarget& target, const DeathEffect& fx,
                                                  sf::Color c, float angle, float scale) const {
    sf::Vector2f center = toScreen({fx.x, fx.y});

    float bodyLen = 1.6f * PPM * scale;
    float bodyHalf = bodyLen / 2.0f;
    float legLen = 0.5f * PPM * scale;
    float bodyElev = legLen;                 // body height above ground
    float headR = 0.2f * PPM * scale;
    float tailLen = 0.6f * PPM * scale;

    // Pivot at center-bottom (ground level)
    sf::Vector2f pivot = {center.x, center.y + legLen};

    auto rotPoint = [&](float localX, float localY) -> sf::Vector2f {
        float rx = localX * std::cos(angle) - localY * std::sin(angle);
        float ry = localX * std::sin(angle) + localY * std::cos(angle);
        return {pivot.x + rx, pivot.y + ry};
    };

    auto drawLine = [&](sf::Vector2f s, sf::Vector2f e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{s, c};
        line[1] = sf::Vertex{e, c};
        target.draw(line);
    };

    // Horizontal body line
    sf::Vector2f bFront = rotPoint(bodyHalf, -bodyElev);
    sf::Vector2f bBack = rotPoint(-bodyHalf, -bodyElev);
    drawLine(bFront, bBack);

    // 4 legs from body down to ground
    float legX[4] = {bodyHalf * 0.8f, bodyHalf * 0.3f, -bodyHalf * 0.3f, -bodyHalf * 0.8f};
    for (int i = 0; i < 4; i++) {
        sf::Vector2f top = rotPoint(legX[i], -bodyElev);
        sf::Vector2f bot = rotPoint(legX[i], 0.0f);
        drawLine(top, bot);
    }

    // Head circle at front
    bool headDetached = (fx.type == DeathAnimType::Dismember && fx.dismemberedPart == 0);
    if (!headDetached) {
        sf::Vector2f headPos = rotPoint(bodyHalf + headR, -bodyElev);
        sf::CircleShape head(headR);
        head.setOrigin({headR, headR});
        head.setPosition(headPos);
        head.setFillColor(sf::Color::Transparent);
        head.setOutlineColor(c);
        head.setOutlineThickness(2.0f);
        target.draw(head);

        float es = headR * 0.35f;
        sf::VertexArray x1(sf::PrimitiveType::Lines, 2);
        sf::VertexArray x2(sf::PrimitiveType::Lines, 2);
        x1[0] = sf::Vertex{{headPos.x - es, headPos.y - es}, c};
        x1[1] = sf::Vertex{{headPos.x + es, headPos.y + es}, c};
        x2[0] = sf::Vertex{{headPos.x + es, headPos.y - es}, c};
        x2[1] = sf::Vertex{{headPos.x - es, headPos.y + es}, c};
        target.draw(x1);
        target.draw(x2);
    } else {
        sf::Vector2f neckPos = rotPoint(bodyHalf, -bodyElev);
        sf::CircleShape stump(3.0f * scale);
        stump.setOrigin({3.0f * scale, 3.0f * scale});
        stump.setPosition(neckPos);
        stump.setFillColor(sf::Color(180, 0, 0, c.a));
        target.draw(stump);
    }

    // Tail
    sf::Vector2f tailEnd = rotPoint(-bodyHalf - tailLen, -bodyElev - tailLen * 0.4f);
    drawLine(bBack, tailEnd);
}

// ─── Serpentine collapse (Cobra) ─────────────────────────────────────
void DeathAnimationSystem::drawCollapseSerpentine(sf::RenderTarget& target, const DeathEffect& fx,
                                                   sf::Color c, float angle, float scale) const {
    sf::Vector2f center = toScreen({fx.x, fx.y});

    float coilR = 0.4f * PPM * scale;
    float neckH = 1.2f * PPM * scale;
    float headR = 0.15f * PPM * scale;

    sf::Vector2f pivot = {center.x, center.y + coilR};

    auto rotPoint = [&](float localX, float localY) -> sf::Vector2f {
        float rx = localX * std::cos(angle) - localY * std::sin(angle);
        float ry = localX * std::sin(angle) + localY * std::cos(angle);
        return {pivot.x + rx, pivot.y + ry};
    };

    // Coil base (does not rotate — stays on ground)
    sf::CircleShape coil(coilR);
    coil.setScale({1.3f, 0.6f});
    coil.setOrigin({coilR, coilR});
    coil.setPosition(pivot);
    coil.setFillColor(sf::Color::Transparent);
    coil.setOutlineColor(c);
    coil.setOutlineThickness(2.0f);
    target.draw(coil);

    // Neck line rising from coil (rotates/falls)
    constexpr int segs = 6;
    sf::VertexArray neck(sf::PrimitiveType::LineStrip, segs + 1);
    for (int i = 0; i <= segs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(segs);
        float localX = std::sin(frac * 1.5f) * coilR * 0.3f;
        float localY = -frac * neckH;
        sf::Vector2f pt = rotPoint(localX, localY);
        neck[i] = sf::Vertex{pt, c};
    }
    target.draw(neck);

    // Head at top of neck
    bool headDetached = (fx.type == DeathAnimType::Dismember && fx.dismemberedPart == 0);
    if (!headDetached) {
        sf::Vector2f headPos = rotPoint(0.0f, -neckH);
        sf::CircleShape head(headR);
        head.setOrigin({headR, headR});
        head.setPosition(headPos);
        head.setFillColor(sf::Color::Transparent);
        head.setOutlineColor(c);
        head.setOutlineThickness(2.0f);
        target.draw(head);

        float es = headR * 0.35f;
        sf::VertexArray x1(sf::PrimitiveType::Lines, 2);
        sf::VertexArray x2(sf::PrimitiveType::Lines, 2);
        x1[0] = sf::Vertex{{headPos.x - es, headPos.y - es}, c};
        x1[1] = sf::Vertex{{headPos.x + es, headPos.y + es}, c};
        x2[0] = sf::Vertex{{headPos.x + es, headPos.y - es}, c};
        x2[1] = sf::Vertex{{headPos.x - es, headPos.y + es}, c};
        target.draw(x1);
        target.draw(x2);
    }
}

// ─── INCINERATE: draw ────────────────────────────────────────────────
void DeathAnimationSystem::drawIncinerate(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const {
    float t = fx.timer;
    BodyPlan plan = getBodyPlan(fx.charType);

    if (t < 0.8f) {
        // Phase 1: Darkening character silhouette
        float darkenT = t / 0.8f;
        sf::Color charcoal(40, 35, 30);
        sf::Color c;
        c.r = static_cast<uint8_t>(fx.playerColor.r + (static_cast<int>(charcoal.r) - fx.playerColor.r) * darkenT);
        c.g = static_cast<uint8_t>(fx.playerColor.g + (static_cast<int>(charcoal.g) - fx.playerColor.g) * darkenT);
        c.b = static_cast<uint8_t>(fx.playerColor.b + (static_cast<int>(charcoal.b) - fx.playerColor.b) * darkenT);
        c.a = static_cast<uint8_t>(255 * effectAlpha);

        switch (plan) {
            case BodyPlan::Humanoid:   drawCollapseHumanoid(target, fx, c, 0.0f, 1.0f); break;
            case BodyPlan::Quadruped:  drawCollapseQuadruped(target, fx, c, 0.0f, 1.0f); break;
            case BodyPlan::Serpentine: drawCollapseSerpentine(target, fx, c, 0.0f, 1.0f); break;
        }
    }
    else if (t < 1.5f) {
        // Phase 2: Charcoal figure crumbling/shrinking
        float scale = fx.incinerateScale;
        sf::Color charcoal(40, 35, 30, static_cast<uint8_t>(255 * effectAlpha));

        if (scale > 0.05f) {
            switch (plan) {
                case BodyPlan::Humanoid:   drawCollapseHumanoid(target, fx, charcoal, 0.0f, scale); break;
                case BodyPlan::Quadruped:  drawCollapseQuadruped(target, fx, charcoal, 0.0f, scale); break;
                case BodyPlan::Serpentine: drawCollapseSerpentine(target, fx, charcoal, 0.0f, scale); break;
            }
        }
    }
    else {
        // Phase 3: Skeleton appears and collapses
        float phaseT = (t - 1.5f) / 1.5f;
        uint8_t skeletonAlpha = static_cast<uint8_t>(255 * (1.0f - phaseT * 0.7f) * effectAlpha);
        sf::Color boneColor(220, 210, 180, skeletonAlpha);

        switch (plan) {
            case BodyPlan::Humanoid:   drawSkeletonHumanoid(target, fx, boneColor, fx.skeletonAngle); break;
            case BodyPlan::Quadruped:  drawSkeletonQuadruped(target, fx, boneColor, fx.skeletonAngle); break;
            case BodyPlan::Serpentine: drawSkeletonSerpentine(target, fx, boneColor, fx.skeletonAngle); break;
        }
    }
}

// ─── Skeleton: Humanoid ──────────────────────────────────────────────
void DeathAnimationSystem::drawSkeletonHumanoid(sf::RenderTarget& target, const DeathEffect& fx,
                                                 sf::Color c, float angle) const {
    sf::Vector2f center = toScreen({fx.x, fx.y});

    float bodyH = 1.8f * PPM * 0.5f;
    float headR = 0.22f * PPM;
    float limbLen = 0.55f * PPM;

    sf::Vector2f pivot = {center.x, center.y + bodyH};

    auto rotPoint = [&](float localX, float localY) -> sf::Vector2f {
        float rx = localX * std::cos(angle) - localY * std::sin(angle);
        float ry = localX * std::sin(angle) + localY * std::cos(angle);
        return {pivot.x + rx, pivot.y + ry};
    };

    auto drawLine = [&](sf::Vector2f s, sf::Vector2f e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{s, c};
        line[1] = sf::Vertex{e, c};
        target.draw(line);
    };

    // Spine
    sf::Vector2f sBot = rotPoint(0.0f, 0.0f);
    sf::Vector2f sTop = rotPoint(0.0f, -bodyH * 2.0f);
    drawLine(sBot, sTop);

    // Ribs (3 horizontal lines across torso)
    for (int i = 0; i < 3; i++) {
        float ribY = -bodyH * 1.0f - static_cast<float>(i) * bodyH * 0.3f;
        float ribW = limbLen * 0.4f * (1.0f - static_cast<float>(i) * 0.15f);
        sf::Vector2f rL = rotPoint(-ribW, ribY);
        sf::Vector2f rR = rotPoint(ribW, ribY);
        drawLine(rL, rR);
    }

    // Skull (circle)
    sf::Vector2f skullPos = rotPoint(0.0f, -bodyH * 2.0f - headR);
    sf::CircleShape skull(headR);
    skull.setOrigin({headR, headR});
    skull.setPosition(skullPos);
    skull.setFillColor(sf::Color::Transparent);
    skull.setOutlineColor(c);
    skull.setOutlineThickness(1.5f);
    target.draw(skull);

    // Eye sockets (two dark circles)
    float eyeR = headR * 0.2f;
    float eyeOff = headR * 0.3f;
    for (float side : {-1.0f, 1.0f}) {
        sf::Vector2f eyePos = rotPoint(side * eyeOff, -bodyH * 2.0f - headR);
        sf::CircleShape eye(eyeR);
        eye.setOrigin({eyeR, eyeR});
        eye.setPosition(eyePos);
        eye.setFillColor(c);
        target.draw(eye);
    }

    // Arm bones (short stubs)
    sf::Vector2f shoulder = rotPoint(0.0f, -bodyH * 1.5f);
    sf::Vector2f lArm = rotPoint(-limbLen * 0.7f, -bodyH * 1.2f);
    sf::Vector2f rArm = rotPoint(limbLen * 0.7f, -bodyH * 1.2f);
    drawLine(shoulder, lArm);
    drawLine(shoulder, rArm);

    // Leg bones
    sf::Vector2f lLeg = rotPoint(-limbLen * 0.4f, limbLen * 0.5f);
    sf::Vector2f rLeg = rotPoint(limbLen * 0.4f, limbLen * 0.5f);
    drawLine(sBot, lLeg);
    drawLine(sBot, rLeg);
}

// ─── Skeleton: Quadruped ─────────────────────────────────────────────
void DeathAnimationSystem::drawSkeletonQuadruped(sf::RenderTarget& target, const DeathEffect& fx,
                                                  sf::Color c, float angle) const {
    sf::Vector2f center = toScreen({fx.x, fx.y});

    float bodyLen = 1.4f * PPM;
    float bodyHalf = bodyLen / 2.0f;
    float legLen = 0.45f * PPM;
    float headR = 0.18f * PPM;

    sf::Vector2f pivot = {center.x, center.y + legLen};

    auto rotPoint = [&](float localX, float localY) -> sf::Vector2f {
        float rx = localX * std::cos(angle) - localY * std::sin(angle);
        float ry = localX * std::sin(angle) + localY * std::cos(angle);
        return {pivot.x + rx, pivot.y + ry};
    };

    auto drawLine = [&](sf::Vector2f s, sf::Vector2f e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{s, c};
        line[1] = sf::Vertex{e, c};
        target.draw(line);
    };

    // Spine line
    sf::Vector2f sFront = rotPoint(bodyHalf, -legLen);
    sf::Vector2f sBack = rotPoint(-bodyHalf, -legLen);
    drawLine(sFront, sBack);

    // Ribs hanging down from spine
    for (int i = 0; i < 4; i++) {
        float ribX = bodyHalf * 0.6f - static_cast<float>(i) * bodyHalf * 0.35f;
        float ribLen = legLen * 0.35f;
        sf::Vector2f top = rotPoint(ribX, -legLen);
        sf::Vector2f bot = rotPoint(ribX, -legLen + ribLen);
        drawLine(top, bot);
    }

    // 4 leg bones
    float legX[4] = {bodyHalf * 0.7f, bodyHalf * 0.25f, -bodyHalf * 0.25f, -bodyHalf * 0.7f};
    for (int i = 0; i < 4; i++) {
        sf::Vector2f top = rotPoint(legX[i], -legLen);
        sf::Vector2f bot = rotPoint(legX[i], 0.0f);
        drawLine(top, bot);
    }

    // Skull at front
    sf::Vector2f skullPos = rotPoint(bodyHalf + headR, -legLen);
    sf::CircleShape skull(headR);
    skull.setOrigin({headR, headR});
    skull.setPosition(skullPos);
    skull.setFillColor(sf::Color::Transparent);
    skull.setOutlineColor(c);
    skull.setOutlineThickness(1.5f);
    target.draw(skull);

    // Eye sockets
    float eyeR = headR * 0.2f;
    sf::Vector2f eyePos = rotPoint(bodyHalf + headR * 1.2f, -legLen - headR * 0.2f);
    sf::CircleShape eye(eyeR);
    eye.setOrigin({eyeR, eyeR});
    eye.setPosition(eyePos);
    eye.setFillColor(c);
    target.draw(eye);
}

// ─── Skeleton: Serpentine ────────────────────────────────────────────
void DeathAnimationSystem::drawSkeletonSerpentine(sf::RenderTarget& target, const DeathEffect& fx,
                                                   sf::Color c, float angle) const {
    sf::Vector2f center = toScreen({fx.x, fx.y});

    float spineH = 1.4f * PPM;
    float headR = 0.13f * PPM;

    sf::Vector2f pivot = {center.x, center.y + 0.3f * PPM};

    auto rotPoint = [&](float localX, float localY) -> sf::Vector2f {
        float rx = localX * std::cos(angle) - localY * std::sin(angle);
        float ry = localX * std::sin(angle) + localY * std::cos(angle);
        return {pivot.x + rx, pivot.y + ry};
    };

    auto drawLine = [&](sf::Vector2f s, sf::Vector2f e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{s, c};
        line[1] = sf::Vertex{e, c};
        target.draw(line);
    };

    // Vertebral column (slightly wavy)
    constexpr int segs = 8;
    sf::VertexArray spine(sf::PrimitiveType::LineStrip, segs + 1);
    for (int i = 0; i <= segs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(segs);
        float localX = std::sin(frac * 2.0f) * 0.15f * PPM;
        float localY = -frac * spineH;
        sf::Vector2f pt = rotPoint(localX, localY);
        spine[i] = sf::Vertex{pt, c};
    }
    target.draw(spine);

    // Small perpendicular rib marks along spine
    for (int i = 1; i < segs; i++) {
        float frac = static_cast<float>(i) / static_cast<float>(segs);
        float localX = std::sin(frac * 2.0f) * 0.15f * PPM;
        float localY = -frac * spineH;
        float ribW = 0.08f * PPM;
        sf::Vector2f rL = rotPoint(localX - ribW, localY);
        sf::Vector2f rR = rotPoint(localX + ribW, localY);
        drawLine(rL, rR);
    }

    // Skull at top
    sf::Vector2f skullPos = rotPoint(0.0f, -spineH - headR);
    sf::CircleShape skull(headR);
    skull.setOrigin({headR, headR});
    skull.setPosition(skullPos);
    skull.setFillColor(sf::Color::Transparent);
    skull.setOutlineColor(c);
    skull.setOutlineThickness(1.5f);
    target.draw(skull);
}

// ─── Draw particles ──────────────────────────────────────────────────
void DeathAnimationSystem::drawParticles(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const {
    for (const auto& p : fx.particles) {
        if (!p.alive) continue;

        float lifeRatio = p.lifetime / p.maxLifetime;
        uint8_t a = static_cast<uint8_t>(p.color.a * lifeRatio * effectAlpha);

        sf::Vector2f screenPos = toScreen({p.x, p.y});
        sf::CircleShape dot(p.size);
        dot.setOrigin({p.size, p.size});
        dot.setPosition(screenPos);
        dot.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b, a));
        target.draw(dot);
    }
}

// ─── Cleanup: destroy physics bodies of dead gibs ────────────────────
void DeathAnimationSystem::cleanup(Physics& physics) {
    for (auto& fx : m_effects) {
        if (!fx.alive) {
            for (auto& g : fx.gibs) {
                if (b2Body_IsValid(g.bodyId)) {
                    b2DestroyBody(g.bodyId);
                }
            }
            fx.gibs.clear();
        } else {
            for (auto& g : fx.gibs) {
                if (!g.alive && b2Body_IsValid(g.bodyId)) {
                    b2DestroyBody(g.bodyId);
                    g.bodyId = {};
                }
            }
        }
    }

    m_effects.erase(
        std::remove_if(m_effects.begin(), m_effects.end(),
                        [](const DeathEffect& fx) { return !fx.alive; }),
        m_effects.end());
}

void DeathAnimationSystem::cleanupAll(Physics& physics) {
    for (auto& fx : m_effects) {
        for (auto& g : fx.gibs) {
            if (b2Body_IsValid(g.bodyId)) {
                b2DestroyBody(g.bodyId);
            }
        }
        fx.gibs.clear();
    }
    m_effects.clear();
}

bool DeathAnimationSystem::hasActiveEffect(int playerIndex) const {
    for (const auto& fx : m_effects) {
        if (fx.alive && fx.playerIndex == playerIndex) return true;
    }
    return false;
}

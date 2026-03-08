#include "DeathAnimation.h"
#include <cmath>
#include <algorithm>

static sf::Vector2f toScreen(b2Vec2 pos) {
    return {SCREEN_CX + pos.x * PPM, SCREEN_CY - pos.y * PPM};
}

// ─── Blood burst helper ──────────────────────────────────────────────
void DeathAnimationSystem::spawnBloodBurst(DeathEffect& fx, float cx, float cy,
                                           int count, float speed, sf::Color color) {
    // Convert pixel-space speed to world-space (meters/sec)
    float worldSpeed = speed / PPM;
    std::uniform_real_distribution<float> angleDist(0.0f, 6.283f);
    std::uniform_real_distribution<float> speedDist(worldSpeed * 0.3f, worldSpeed);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.5f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 1.5f);

    for (int i = 0; i < count; i++) {
        float a = angleDist(m_rng);
        float s = speedDist(m_rng);
        BloodParticle p;
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
                                      int playerIndex,
                                      float knockbackX, float knockbackY) {
    DeathEffect fx;
    fx.type = type;
    fx.x = x; fx.y = y;
    fx.playerColor = color;
    fx.playerIndex = playerIndex;

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
    }

    m_effects.push_back(std::move(fx));
}

// ─── COLLAPSE: body topples sideways ─────────────────────────────────
void DeathAnimationSystem::spawnCollapse(Physics& physics, DeathEffect& fx,
                                         float kbX, float kbY) {
    // Collapse is purely visual — no physics gibs, just an animated stick figure falling
    fx.collapseAngle = 0.0f;
    fx.collapseVelY = 0.0f;
    fx.collapseLanded = false;

    // Small blood burst at position
    sf::Color blood(180, 0, 0, 220);
    spawnBloodBurst(fx, fx.x, fx.y, 8, 60.0f, blood);
}

// ─── DISMEMBER: head or arm flies off ────────────────────────────────
void DeathAnimationSystem::spawnDismember(Physics& physics, DeathEffect& fx,
                                          float kbX, float kbY) {
    std::uniform_real_distribution<float> upDist(5.0f, 10.0f);
    std::uniform_real_distribution<float> sideDist(-3.0f, 3.0f);
    std::uniform_int_distribution<int> partChoice(0, 2);  // 0=head, 1=left arm, 2=right arm

    float dir = (kbX >= 0.0f) ? 1.0f : -1.0f;
    int choice = partChoice(m_rng);
    fx.dismemberedPart = choice;

    // Spawn the flying detached part as a physics gib
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
        // Head flies off
        spawnGibBody(0.0f, 0.7f, 0.25f, 0.0f, true,
                     dir * 4.0f + sideDist(m_rng), upDist(m_rng));
    } else {
        // Arm flies off (left or right)
        float side = (choice == 1) ? -1.0f : 1.0f;
        spawnGibBody(side * 0.4f, 0.3f, 0.3f, 0.04f, false,
                     side * 5.0f + sideDist(m_rng), upDist(m_rng));
    }

    // Blood spray at the separation point
    sf::Color blood(200, 0, 0, 240);
    float bloodY = (choice == 0) ? fx.y + 0.5f : fx.y + 0.3f;
    spawnBloodBurst(fx, fx.x, bloodY, 20, 90.0f, blood);

    // Also add a darker stump indicator
    sf::Color darkBlood(120, 0, 0, 200);
    spawnBloodBurst(fx, fx.x, bloodY, 6, 30.0f, darkBlood);

    // The "rest of body" collapses
    fx.collapseAngle = 0.0f;
    fx.collapseVelY = 0.0f;
    fx.collapseLanded = false;
}

// ─── EXPLODE: body blows into many pieces ────────────────────────────
void DeathAnimationSystem::spawnExplode(Physics& physics, DeathEffect& fx,
                                        float kbX, float kbY) {
    std::uniform_real_distribution<float> angleDist(0.0f, 6.283f);
    std::uniform_real_distribution<float> speedDist(5.0f, 15.0f);
    std::uniform_real_distribution<float> sizeDist(0.06f, 0.18f);
    std::uniform_real_distribution<float> spinDist(-10.0f, 10.0f);

    // Spawn 8-12 gibs flying in all directions
    int gibCount = 8 + (m_rng() % 5);
    for (int i = 0; i < gibCount; i++) {
        float angle = angleDist(m_rng);
        float speed = speedDist(m_rng);
        float sz = sizeDist(m_rng);

        Gib g;
        g.isCircle = (i < 2); // first couple are circles (head, etc)
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
        float vy = std::sin(angle) * speed + 5.0f; // bias upward
        b2Body_ApplyLinearImpulseToCenter(g.bodyId, {vx, vy}, true);
        b2Body_SetAngularVelocity(g.bodyId, spinDist(m_rng));

        fx.gibs.push_back(g);
    }

    // Massive blood explosion
    sf::Color blood(200, 0, 0, 220);
    spawnBloodBurst(fx, fx.x, fx.y, 40, 150.0f, blood);

    // Add some bone-colored particles
    sf::Color bone(220, 210, 180, 200);
    spawnBloodBurst(fx, fx.x, fx.y, 10, 100.0f, bone);
}

// ─── DISINTEGRATE: particle cloud (nuke) ─────────────────────────────
void DeathAnimationSystem::spawnDisintegrate(DeathEffect& fx) {
    // No physics gibs — just a massive particle cloud
    std::uniform_real_distribution<float> angleDist(0.0f, 6.283f);
    std::uniform_real_distribution<float> speedDist(30.0f / PPM, 200.0f / PPM);
    std::uniform_real_distribution<float> sizeDist(1.0f, 4.0f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 2.0f);

    // Character-colored ash particles
    for (int i = 0; i < 60; i++) {
        float a = angleDist(m_rng);
        float s = speedDist(m_rng);
        BloodParticle p;
        p.x = fx.x; p.y = fx.y;
        p.vx = std::cos(a) * s;
        p.vy = std::sin(a) * s;
        p.lifetime = lifeDist(m_rng);
        p.maxLifetime = p.lifetime;
        p.size = sizeDist(m_rng);

        // Mix between player color and ash/ember colors
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

            // Gravity in world space (Y-up, so subtract to pull down)
            p.vy -= 10.0f * dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
        }

        // Collapse animation (used for Collapse and Dismember body remainder)
        if (fx.type == DeathAnimType::Collapse || fx.type == DeathAnimType::Dismember) {
            if (!fx.collapseLanded) {
                // Rotate toward ground
                float targetAngle = 1.57f; // 90 degrees (fallen over)
                float rotSpeed = 3.0f;
                fx.collapseAngle += rotSpeed * dt;
                if (fx.collapseAngle >= targetAngle) {
                    fx.collapseAngle = targetAngle;
                    fx.collapseLanded = true;
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

    // Fade based on lifetime, multiplied by overall effect fade-out
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

        // "X" eyes on head gibs
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

void DeathAnimationSystem::drawCollapse(sf::RenderTarget& target, const DeathEffect& fx, float effectAlpha) const {
    // Draw a simplified stick figure that's rotating to fallen position
    uint8_t a = static_cast<uint8_t>(255.0f * effectAlpha);
    sf::Color c(fx.playerColor.r, fx.playerColor.g, fx.playerColor.b, a);

    sf::Vector2f center = toScreen({fx.x, fx.y});
    float angle = fx.collapseAngle;

    // The body pivots from the feet
    float bodyH = 1.8f * PPM * 0.5f; // half body height in pixels
    float headR = 0.25f * PPM;
    float limbLen = 0.6f * PPM;

    // Pivot point at feet level
    sf::Vector2f pivot = {center.x, center.y + bodyH};

    // Compute body positions rotated around pivot
    auto rotPoint = [&](float localX, float localY) -> sf::Vector2f {
        float rx = localX * std::cos(angle) - localY * std::sin(angle);
        float ry = localX * std::sin(angle) + localY * std::cos(angle);
        return {pivot.x + rx, pivot.y + ry};
    };

    // Torso: from feet (0,0) to top (0, -bodyH*2)
    sf::Vector2f tBot = rotPoint(0.0f, 0.0f);
    sf::Vector2f tTop = rotPoint(0.0f, -bodyH * 2.0f);
    sf::Vector2f tMid = rotPoint(0.0f, -bodyH);

    auto drawLine = [&](sf::Vector2f s, sf::Vector2f e) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{s, c};
        line[1] = sf::Vertex{e, c};
        target.draw(line);
    };

    // Torso line
    drawLine(tBot, tTop);

    // Head — draw unless it was the dismembered part
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
        sf::Color ec = c;
        sf::VertexArray x1(sf::PrimitiveType::Lines, 2);
        sf::VertexArray x2(sf::PrimitiveType::Lines, 2);
        x1[0] = sf::Vertex{{headPos.x - es, headPos.y - es}, ec};
        x1[1] = sf::Vertex{{headPos.x + es, headPos.y + es}, ec};
        x2[0] = sf::Vertex{{headPos.x + es, headPos.y - es}, ec};
        x2[1] = sf::Vertex{{headPos.x - es, headPos.y + es}, ec};
        target.draw(x1);
        target.draw(x2);
    } else {
        // Head was detached — draw blood stump at neck
        sf::Vector2f neckPos = rotPoint(0.0f, -bodyH * 2.0f);
        sf::CircleShape stump(3.0f);
        stump.setOrigin({3.0f, 3.0f});
        stump.setPosition(neckPos);
        stump.setFillColor(sf::Color(180, 0, 0, a));
        target.draw(stump);
    }

    // Arms — shoulder is at tTop area, skip the detached arm
    sf::Vector2f shoulder = rotPoint(0.0f, -bodyH * 1.5f);
    sf::Vector2f lArm = rotPoint(-limbLen, -bodyH * 1.2f);
    sf::Vector2f rArm = rotPoint(limbLen, -bodyH * 1.2f);
    bool lArmDetached = (fx.type == DeathAnimType::Dismember && fx.dismemberedPart == 1);
    bool rArmDetached = (fx.type == DeathAnimType::Dismember && fx.dismemberedPart == 2);
    if (!lArmDetached) drawLine(shoulder, lArm);
    if (!rArmDetached) drawLine(shoulder, rArm);
    // Draw stump at shoulder for detached arm
    if (lArmDetached || rArmDetached) {
        sf::CircleShape stump(2.5f);
        stump.setOrigin({2.5f, 2.5f});
        stump.setPosition(shoulder);
        stump.setFillColor(sf::Color(180, 0, 0, a));
        target.draw(stump);
    }

    // Legs
    sf::Vector2f lLeg = rotPoint(-limbLen * 0.5f, limbLen * 0.6f);
    sf::Vector2f rLeg = rotPoint(limbLen * 0.5f, limbLen * 0.6f);
    drawLine(tBot, lLeg);
    drawLine(tBot, rLeg);
}

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
            // Cleanup individual dead gibs
            for (auto& g : fx.gibs) {
                if (!g.alive && b2Body_IsValid(g.bodyId)) {
                    b2DestroyBody(g.bodyId);
                    g.bodyId = {}; // invalidate
                }
            }
        }
    }

    // Remove dead effects
    m_effects.erase(
        std::remove_if(m_effects.begin(), m_effects.end(),
                        [](const DeathEffect& fx) { return !fx.alive; }),
        m_effects.end());
}

bool DeathAnimationSystem::hasActiveEffect(int playerIndex) const {
    for (const auto& fx : m_effects) {
        if (fx.alive && fx.playerIndex == playerIndex) return true;
    }
    return false;
}

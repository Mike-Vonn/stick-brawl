#include "Game.h"
#include <iostream>
#include <cmath>
#include <algorithm>

Game::Game() = default;
Game::~Game() = default;

bool Game::init() {
    m_rulesEngine.loadFromFile("assets/rules/default.json");
    const auto& rules = m_rulesEngine.getRules();
    m_weaponFactory.loadWeaponsFromDirectory("assets/weapons");

    if (!m_renderer.init(1280, 720, "StickBrawl")) return false;

    m_physics.setGravity(rules.gravityX, rules.gravityY);
    m_arena.createDefaultArena(m_physics);

    const auto& spawns = m_arena.getSpawnPoints();
    CharacterType types[] = { CharacterType::Stick, CharacterType::Stick,
                              CharacterType::Cat,   CharacterType::Cobra };
    int numPlayers = 4;
    for (int i = 0; i < numPlayers && i < static_cast<int>(spawns.size()); i++) {
        auto p = std::make_unique<StickFigure>(
            i, m_physics, spawns[i].x, spawns[i].y, m_playerColors[i], types[i]);
        p->setLives(rules.livesPerPlayer);

        // Give cobra its innate poison spit
        if (types[i] == CharacterType::Cobra) {
            auto* poison = m_weaponFactory.getWeapon("Poison Spit");
            if (poison) p->equipWeapon(*poison);
        }

        m_players.push_back(std::move(p));
    }

    m_hud.init();
    m_roundTimer = rules.roundTimeSeconds;
    m_weaponSpawnTimer = rules.weaponSpawnInterval;
    m_state = GameState::Playing;

    std::cout << "Game initialized with " << numPlayers << " players!\n";
    return true;
}

void Game::run() {
    sf::Clock clock;
    const float fixedDt = 1.0f / 60.0f;
    float accumulator = 0.0f;

    while (m_renderer.isOpen()) {
        float frameTime = clock.restart().asSeconds();
        if (frameTime > 0.25f) frameTime = 0.25f;
        accumulator += frameTime;
        processEvents();
        while (accumulator >= fixedDt) {
            update(fixedDt);
            m_input.update();
            accumulator -= fixedDt;
        }
        render();
    }
}

void Game::processEvents() {
    while (const auto event = m_renderer.getWindow().pollEvent()) {
        if (event->is<sf::Event::Closed>()) m_renderer.getWindow().close();
        if (const auto* k = event->getIf<sf::Event::KeyPressed>()) {
            if (k->code == sf::Keyboard::Key::Escape) m_renderer.getWindow().close();
            if (k->code == sf::Keyboard::Key::R && m_state == GameState::RoundOver) {
                const auto& spawns = m_arena.getSpawnPoints();
                for (size_t i = 0; i < m_players.size(); i++)
                    m_players[i]->respawn(spawns[i].x, spawns[i].y);
                m_pickups.clear();
                m_roundTimer = m_rulesEngine.getRules().roundTimeSeconds;
                m_state = GameState::Playing;
            }
        }
    }
}

void Game::update(float dt) {
    if (m_state != GameState::Playing) return;
    m_roundTimer -= dt;
    if (m_roundTimer <= 0.0f) { m_roundTimer = 0.0f; m_state = GameState::RoundOver; return; }

    handlePlayerInput(dt);
    for (auto& p : m_players) p->update(dt);
    m_physics.step(dt);
    updateProjectiles(dt);
    updateWeaponPickups(dt);
    checkFallDeath();
    updateWeaponSpawns(dt);
    checkRoundEnd();
}

void Game::handlePlayerInput(float dt) {
    for (size_t i = 0; i < m_players.size(); i++) {
        auto& player = m_players[i];
        if (!player->isAlive()) continue;

        PlayerInput pi = m_input.getPlayerInput(static_cast<int>(i));

        if (pi.moveLeft) player->moveLeft();
        else if (pi.moveRight) player->moveRight();
        else player->stopMoving();

        if (pi.jumpPressed) player->jump();

        // Aiming
        if (pi.aimUp) player->aimUp();
        else if (pi.aimDown) player->aimDown();
        else player->resetAim();

        if (pi.attackPressed && player->canAttack()) {
            const auto& weapon = player->getCurrentWeapon();
            player->attack();
            if (weapon.type == WeaponType::Melee)
                handleMeleeAttack(*player);
            else
                spawnProjectile(*player);
        }
    }
}

void Game::handleMeleeAttack(StickFigure& attacker) {
    const auto& weapon = attacker.getCurrentWeapon();
    const auto& rules = m_rulesEngine.getRules();
    b2Vec2 ap = attacker.getPosition();
    float dir = static_cast<float>(attacker.getFacingDirection());

    for (auto& target : m_players) {
        if (target->getPlayerIndex() == attacker.getPlayerIndex()) continue;
        if (!target->isAlive()) continue;

        b2Vec2 tp = target->getPosition();
        float dx = tp.x - ap.x, dy = tp.y - ap.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        bool inRange = dist < weapon.range;
        bool facing = dx * dir >= -0.3f;
        bool close = dist < weapon.range * 0.5f;

        if (inRange && (facing || close)) {
            float dmg = weapon.damage * rules.damageMultiplier;
            float kbX = weapon.knockbackForce * dir * rules.knockbackMultiplier;
            float kbY = weapon.knockbackForce * 0.5f * rules.knockbackMultiplier;
            target->takeDamage(dmg, kbX, kbY);
        }
    }
}

void Game::spawnProjectile(StickFigure& shooter) {
    const auto& weapon = shooter.getCurrentWeapon();
    if (weapon.type == WeaponType::Melee) return;

    b2Vec2 pos = shooter.getPosition();
    float dir = static_cast<float>(shooter.getFacingDirection());
    float aim = shooter.getAimAngle();

    float vx = weapon.projectileSpeed * dir * std::cos(aim);
    float vy = weapon.projectileSpeed * std::sin(aim);

    b2BodyId bullet = m_physics.createDynamicCircle(
        pos.x + dir * 0.5f, pos.y + 0.3f, 0.15f, 0.1f,
        CAT_PROJECTILE, CAT_PLATFORM | CAT_PLAYER);

    b2Body_SetBullet(bullet, true);
    b2Body_SetLinearVelocity(bullet, {vx, vy});

    if (!weapon.affectedByGravity)
        b2Body_SetGravityScale(bullet, 0.0f);

    Projectile proj;
    proj.bodyId = bullet;
    proj.weapon = weapon;
    proj.ownerIndex = shooter.getPlayerIndex();
    proj.lifetime = weapon.projectileLifetime;
    proj.alive = true;

    // Check if this is a poison weapon
    if (weapon.name == "Poison Spit") {
        proj.isPoison = true;
        proj.poisonDps = 8.0f;
        proj.poisonDuration = 4.0f;
    }

    m_projectiles.push_back(proj);
}

void Game::updateProjectiles(float dt) {
    const auto& rules = m_rulesEngine.getRules();

    for (auto& proj : m_projectiles) {
        if (!proj.alive) continue;
        proj.lifetime -= dt;
        if (proj.lifetime <= 0.0f) {
            proj.alive = false;
            b2DestroyBody(proj.bodyId);
            continue;
        }

        b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
        for (auto& player : m_players) {
            if (player->getPlayerIndex() == proj.ownerIndex) continue;
            if (!player->isAlive()) continue;

            b2Vec2 plp = player->getPosition();
            float dx = pp.x - plp.x, dy = pp.y - plp.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            float hitR = (proj.weapon.type == WeaponType::Explosive) ? proj.weapon.explosionRadius : 0.6f;

            if (dist < hitR) {
                if (proj.isPoison) {
                    // Poison: small initial damage + DOT
                    player->takeDamage(5.0f, 0.0f, 0.0f);
                    player->applyPoison(proj.poisonDps, proj.poisonDuration);
                } else {
                    float dmg = proj.weapon.damage * rules.damageMultiplier;
                    float kbDir = (dx > 0) ? -1.0f : 1.0f;
                    float kbX = proj.weapon.knockbackForce * kbDir * rules.knockbackMultiplier;
                    float kbY = proj.weapon.knockbackForce * 0.5f * rules.knockbackMultiplier;
                    player->takeDamage(dmg, kbX, kbY);
                }
                proj.alive = false;
                b2DestroyBody(proj.bodyId);
                break;
            }
        }
    }

    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
                        [](const Projectile& p) { return !p.alive; }),
        m_projectiles.end());
}

void Game::updateWeaponSpawns(float dt) {
    const auto& rules = m_rulesEngine.getRules();
    m_weaponSpawnTimer -= dt;
    if (m_weaponSpawnTimer <= 0.0f && static_cast<int>(m_pickups.size()) < rules.weaponSpawnMax) {
        WeaponPickup pickup;
        pickup.position = m_arena.getRandomPlatformTop();
        // Don't spawn fists or poison spit as pickups
        do {
            pickup.weapon = m_weaponFactory.getRandomWeapon();
        } while (pickup.weapon.name == "Fists" || pickup.weapon.name == "Poison Spit");
        pickup.alive = true;
        pickup.bobTimer = 0.0f;
        m_pickups.push_back(pickup);
        m_weaponSpawnTimer = rules.weaponSpawnInterval;
    }
}

void Game::updateWeaponPickups(float dt) {
    for (auto& pickup : m_pickups) {
        if (!pickup.alive) continue;
        pickup.bobTimer += dt;

        // Check if any player is close enough to pick up
        for (auto& player : m_players) {
            if (!player->isAlive()) continue;
            b2Vec2 pp = player->getPosition();
            float dx = pp.x - pickup.position.x;
            float dy = pp.y - pickup.position.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 1.5f) {
                player->equipWeapon(pickup.weapon);
                pickup.alive = false;
                std::cout << "Player " << player->getPlayerIndex()
                          << " picked up " << pickup.weapon.name << "!\n";
                break;
            }
        }
    }

    m_pickups.erase(
        std::remove_if(m_pickups.begin(), m_pickups.end(),
                        [](const WeaponPickup& p) { return !p.alive; }),
        m_pickups.end());
}

void Game::checkFallDeath() {
    const auto& rules = m_rulesEngine.getRules();
    const auto& spawns = m_arena.getSpawnPoints();
    for (auto& player : m_players) {
        if (!player->isAlive()) continue;
        if (player->getPosition().y < rules.fallDeathY) {
            player->takeDamage(9999.0f, 0.0f, 0.0f);
            int lives = player->getLives() - 1;
            player->setLives(lives);
            if (lives > 0) {
                size_t idx = static_cast<size_t>(player->getPlayerIndex());
                player->respawn(spawns[idx].x, spawns[idx].y);
            }
        }
    }
}

void Game::checkRoundEnd() {
    int alive = 0; int last = -1;
    for (const auto& p : m_players) {
        if (p->isAlive() && p->getLives() > 0) { alive++; last = p->getPlayerIndex(); }
    }
    if (alive <= 1) {
        m_state = GameState::RoundOver;
        if (last >= 0) std::cout << "Player " << last << " wins!\n";
        else std::cout << "Draw!\n";
    }
}

void Game::render() {
    m_renderer.clear(sf::Color(25, 25, 30));
    m_arena.draw(m_renderer.getWindow());

    // Draw weapon pickups
    for (const auto& pickup : m_pickups) {
        if (!pickup.alive) continue;
        float bob = std::sin(pickup.bobTimer * 3.0f) * 3.0f;
        sf::Vector2f sp = {640.0f + pickup.position.x * PPM,
                           360.0f - pickup.position.y * PPM + bob};

        // Glowing box
        sf::RectangleShape box({16.0f, 16.0f});
        box.setOrigin({8.0f, 8.0f});
        box.setPosition(sp);
        box.setFillColor(sf::Color(255, 200, 50, 200));
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(1.0f);
        box.setRotation(sf::degrees(pickup.bobTimer * 60.0f));
        m_renderer.getWindow().draw(box);

        // Weapon type indicator
        sf::CircleShape indicator(3.0f);
        indicator.setOrigin({3.0f, 3.0f});
        indicator.setPosition({sp.x, sp.y - 12.0f});
        if (pickup.weapon.type == WeaponType::Melee)
            indicator.setFillColor(sf::Color::Red);
        else if (pickup.weapon.type == WeaponType::Explosive)
            indicator.setFillColor(sf::Color(255, 100, 0));
        else
            indicator.setFillColor(sf::Color::Cyan);
        m_renderer.getWindow().draw(indicator);
    }

    // Draw players
    for (const auto& p : m_players) p->draw(m_renderer.getWindow());

    // Draw projectiles
    for (const auto& proj : m_projectiles) {
        if (!proj.alive) continue;
        b2Vec2 pos = b2Body_GetPosition(proj.bodyId);
        sf::CircleShape c(4.0f); c.setOrigin({4.0f, 4.0f});
        c.setPosition({640.0f + pos.x * PPM, 360.0f - pos.y * PPM});
        if (proj.isPoison)
            c.setFillColor(sf::Color(0, 220, 0));
        else
            c.setFillColor(sf::Color::Yellow);
        m_renderer.getWindow().draw(c);
    }

    m_hud.draw(m_renderer.getWindow(), m_players, m_roundTimer);

    if (m_state == GameState::RoundOver) {
        sf::RectangleShape overlay({1280.0f, 720.0f});
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        m_renderer.getWindow().draw(overlay);
    }

    m_renderer.display();
}

#include "Game.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <sstream>
#include <optional>

Game::Game() = default;
Game::~Game() = default;

bool Game::init() {
    m_rulesEngine.loadFromFile("assets/rules/default.json");
    m_weaponFactory.loadWeaponsFromDirectory("assets/weapons");

    if (!m_renderer.init(1280, 720, "StickBrawl")) return false;
    m_hud.init();

    // Start in character select
    m_state = GameState::CharSelect;
    for (auto& ps : m_selectState) {
        ps.joined = false;
        ps.ready = false;
        ps.charIndex = 0;
    }
    // Player 0 auto-joins
    m_selectState[0].joined = true;

    return true;
}

// ============================================================
// CHARACTER SELECT
// ============================================================

static CharacterType indexToType(int idx) {
    switch (idx % CHARACTER_TYPE_COUNT) {
        case 0: return CharacterType::Stick;
        case 1: return CharacterType::Cat;
        case 2: return CharacterType::Cobra;
        case 3: return CharacterType::Unicorn;
        default: return CharacterType::Stick;
    }
}

bool Game::allPlayersReady() const {
    int joined = 0;
    int ready = 0;
    for (const auto& ps : m_selectState) {
        if (ps.joined) { joined++; if (ps.ready) ready++; }
    }
    return joined >= 2 && ready == joined;
}

void Game::processCharSelectEvents() {
    while (const auto event = m_renderer.getWindow().pollEvent()) {
        if (event->is<sf::Event::Closed>()) m_renderer.getWindow().close();
        if (const auto* k = event->getIf<sf::Event::KeyPressed>()) {
            if (k->code == sf::Keyboard::Key::Escape)
                m_renderer.getWindow().close();

            // Enter/Space starts game if all ready
            if ((k->code == sf::Keyboard::Key::Enter || k->code == sf::Keyboard::Key::Space)
                && allPlayersReady()) {
                startGame();
                return;
            }

            // Per-player controls during select:
            // Each player uses their attack key to join/ready, left/right to pick character
            for (int i = 0; i < MAX_PLAYERS; i++) {
                PlayerInput pi = m_input.getPlayerInput(i);
                // We check raw keys here via the event
                // Attack key = join then ready
                // Jump key while ready = unready

                // Get the key bindings by testing input
                // We'll handle it in updateCharSelect with held keys instead
            }
        }
    }
}

void Game::updateCharSelect(float dt) {
    m_selectAnimTimer += dt;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        auto& ps = m_selectState[i];
        ps.previewTimer += dt;

        PlayerInput pi = m_input.getPlayerInput(i);

        if (!ps.joined) {
            // Attack key to join
            if (pi.attackPressed) {
                ps.joined = true;
                ps.ready = false;
                ps.charIndex = i % CHARACTER_TYPE_COUNT;
            }
            continue;
        }

        if (!ps.ready) {
            // Left/right to cycle character
            static bool prevLeft[MAX_PLAYERS] = {};
            static bool prevRight[MAX_PLAYERS] = {};

            if (pi.moveLeft && !prevLeft[i]) {
                ps.charIndex--;
                if (ps.charIndex < 0) ps.charIndex = CHARACTER_TYPE_COUNT - 1;
            }
            if (pi.moveRight && !prevRight[i]) {
                ps.charIndex++;
                if (ps.charIndex >= CHARACTER_TYPE_COUNT) ps.charIndex = 0;
            }
            prevLeft[i] = pi.moveLeft;
            prevRight[i] = pi.moveRight;

            // Attack to confirm (ready up)
            if (pi.attackPressed) {
                ps.ready = true;
            }

            // Jump key to leave
            if (pi.jumpPressed && i > 0) {
                ps.joined = false;
            }
        } else {
            // Jump key to unready
            if (pi.jumpPressed) {
                ps.ready = false;
            }
        }
    }
}

void Game::renderCharSelect() {
    m_renderer.clear(sf::Color(20, 15, 30));
    auto& win = m_renderer.getWindow();

    // We need a font — reuse HUD's approach
    // Try to load font inline (simple approach)
    static std::optional<sf::Font> font;
    static bool fontLoaded = false;
    if (!fontLoaded) {
        const char* fontPaths[] = {
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/consola.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "assets/fonts/default.ttf",
        };
        for (const char* path : fontPaths) {
            try { font.emplace(path); fontLoaded = true; break; }
            catch (...) {}
        }
    }

    float slotWidth = SCREEN_WIDTH / static_cast<float>(MAX_PLAYERS);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        const auto& ps = m_selectState[i];
        float x = slotWidth * static_cast<float>(i);
        float cx = x + slotWidth / 2.0f;

        // Slot background
        sf::RectangleShape bg({slotWidth - 8.0f, SCREEN_HEIGHT - 120.0f});
        bg.setPosition({x + 4.0f, 80.0f});
        if (ps.ready)
            bg.setFillColor(sf::Color(30, 60, 30));
        else if (ps.joined)
            bg.setFillColor(sf::Color(40, 35, 50));
        else
            bg.setFillColor(sf::Color(25, 25, 30));
        bg.setOutlineColor(m_playerColors[i]);
        bg.setOutlineThickness(ps.joined ? 2.0f : 1.0f);
        win.draw(bg);

        if (!fontLoaded) continue;

        if (!ps.joined) {
            sf::Text joinText(*font, "Press ATK\nto join", 18);
            joinText.setFillColor(sf::Color(120, 120, 120));
            sf::FloatRect jb = joinText.getLocalBounds();
            joinText.setPosition({cx - jb.size.x / 2.0f, SCREEN_HEIGHT / 2.0f - 20.0f});
            win.draw(joinText);

            sf::Text pNum(*font, "P" + std::to_string(i + 1), 22);
            pNum.setFillColor(m_playerColors[i]);
            sf::FloatRect pb = pNum.getLocalBounds();
            pNum.setPosition({cx - pb.size.x / 2.0f, 90.0f});
            win.draw(pNum);
            continue;
        }

        // Player number
        sf::Text pNum(*font, "P" + std::to_string(i + 1), 22);
        pNum.setFillColor(m_playerColors[i]);
        sf::FloatRect pb = pNum.getLocalBounds();
        pNum.setPosition({cx - pb.size.x / 2.0f, 90.0f});
        win.draw(pNum);

        // Character name
        CharacterType ct = indexToType(ps.charIndex);
        std::string charName = characterTypeName(ct);
        sf::Text nameText(*font, charName, 20);
        nameText.setFillColor(sf::Color::White);
        sf::FloatRect nb = nameText.getLocalBounds();
        nameText.setPosition({cx - nb.size.x / 2.0f, 130.0f});
        win.draw(nameText);

        // Arrows (< and >) if not ready
        if (!ps.ready) {
            sf::Text leftArrow(*font, "<", 28);
            leftArrow.setFillColor(sf::Color(200, 200, 200));
            leftArrow.setPosition({x + 15.0f, 126.0f});
            win.draw(leftArrow);

            sf::Text rightArrow(*font, ">", 28);
            rightArrow.setFillColor(sf::Color(200, 200, 200));
            rightArrow.setPosition({x + slotWidth - 30.0f, 126.0f});
            win.draw(rightArrow);
        }

        // Character preview — draw a simple iconic representation
        float previewY = SCREEN_HEIGHT / 2.0f + 20.0f;
        sf::Color pc = m_playerColors[i];

        switch (ct) {
            case CharacterType::Stick: {
                // Stick figure
                sf::CircleShape head(12.0f);
                head.setOrigin({12.0f, 12.0f});
                head.setPosition({cx, previewY - 50.0f});
                head.setFillColor(sf::Color::Transparent);
                head.setOutlineColor(pc); head.setOutlineThickness(2.0f);
                win.draw(head);
                sf::VertexArray body(sf::PrimitiveType::Lines, 2);
                body[0] = sf::Vertex{{cx, previewY - 38.0f}, pc};
                body[1] = sf::Vertex{{cx, previewY}, pc};
                win.draw(body);
                for (float s : {-1.0f, 1.0f}) {
                    sf::VertexArray arm(sf::PrimitiveType::Lines, 2);
                    arm[0] = sf::Vertex{{cx, previewY - 28.0f}, pc};
                    arm[1] = sf::Vertex{{cx + s * 18.0f, previewY - 15.0f}, pc};
                    win.draw(arm);
                    sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
                    leg[0] = sf::Vertex{{cx, previewY}, pc};
                    leg[1] = sf::Vertex{{cx + s * 14.0f, previewY + 22.0f}, pc};
                    win.draw(leg);
                }
                break;
            }
            case CharacterType::Cat: {
                // Cat body
                sf::CircleShape catBody(18.0f);
                catBody.setScale({1.2f, 0.8f});
                catBody.setOrigin({18.0f, 18.0f});
                catBody.setPosition({cx, previewY});
                catBody.setFillColor(pc);
                catBody.setOutlineColor(sf::Color::Black); catBody.setOutlineThickness(1.0f);
                win.draw(catBody);
                // Head
                sf::CircleShape catHead(12.0f);
                catHead.setOrigin({12.0f, 12.0f});
                catHead.setPosition({cx + 16.0f, previewY - 14.0f});
                catHead.setFillColor(pc);
                catHead.setOutlineColor(sf::Color::Black); catHead.setOutlineThickness(1.0f);
                win.draw(catHead);
                // Ears
                for (float es : {-1.0f, 1.0f}) {
                    sf::ConvexShape ear(3);
                    ear.setPoint(0, {cx + 16.0f + es * 6.0f, previewY - 24.0f});
                    ear.setPoint(1, {cx + 16.0f + es * 3.0f, previewY - 36.0f});
                    ear.setPoint(2, {cx + 16.0f + es * 10.0f, previewY - 28.0f});
                    ear.setFillColor(pc); win.draw(ear);
                }
                break;
            }
            case CharacterType::Cobra: {
                // Coil
                sf::CircleShape coil(16.0f, 16);
                coil.setScale({1.2f, 0.6f});
                coil.setOrigin({16.0f, 16.0f});
                coil.setPosition({cx, previewY + 15.0f});
                coil.setFillColor(pc);
                coil.setOutlineColor(sf::Color::Black); coil.setOutlineThickness(1.0f);
                win.draw(coil);
                // Neck
                sf::RectangleShape snakeNeck({5.0f, 35.0f});
                snakeNeck.setOrigin({2.5f, 35.0f});
                snakeNeck.setPosition({cx + 3.0f, previewY + 5.0f});
                snakeNeck.setFillColor(pc); win.draw(snakeNeck);
                // Hood
                sf::ConvexShape hood(5);
                float shy = previewY - 28.0f;
                hood.setPoint(0, {cx - 14.0f, shy + 5.0f});
                hood.setPoint(1, {cx - 8.0f, shy - 6.0f});
                hood.setPoint(2, {cx, shy - 10.0f});
                hood.setPoint(3, {cx + 8.0f, shy - 6.0f});
                hood.setPoint(4, {cx + 14.0f, shy + 5.0f});
                hood.setFillColor(pc); hood.setOutlineColor(sf::Color::Black); hood.setOutlineThickness(1.0f);
                win.draw(hood);
                // Eyes
                for (float es : {-1.0f, 1.0f}) {
                    sf::CircleShape eye(2.0f); eye.setOrigin({2.0f, 2.0f});
                    eye.setPosition({cx + es * 4.0f, shy - 6.0f});
                    eye.setFillColor(sf::Color::Red); win.draw(eye);
                }
                break;
            }
            case CharacterType::Unicorn: {
                // Body
                sf::RectangleShape ubody({40.0f, 22.0f});
                ubody.setOrigin({20.0f, 11.0f});
                ubody.setPosition({cx, previewY + 5.0f});
                ubody.setFillColor(pc);
                ubody.setOutlineColor(sf::Color(pc.r/2, pc.g/2, pc.b/2)); ubody.setOutlineThickness(1.0f);
                win.draw(ubody);
                // Head
                sf::CircleShape uhead(9.0f);
                uhead.setScale({1.3f, 1.0f});
                uhead.setOrigin({9.0f, 9.0f});
                uhead.setPosition({cx + 22.0f, previewY - 15.0f});
                uhead.setFillColor(pc);
                uhead.setOutlineColor(sf::Color(pc.r/2, pc.g/2, pc.b/2)); uhead.setOutlineThickness(1.0f);
                win.draw(uhead);
                // Horn (rainbow)
                float hornT = m_selectAnimTimer;
                constexpr int hs = 8;
                sf::VertexArray horn(sf::PrimitiveType::LineStrip, hs + 1);
                for (int hi = 0; hi <= hs; hi++) {
                    float hf = static_cast<float>(hi) / static_cast<float>(hs);
                    float r = std::sin(hornT * 2.0f + hf * 6.28f) * 0.5f + 0.5f;
                    float g = std::sin(hornT * 2.0f + hf * 6.28f + 2.094f) * 0.5f + 0.5f;
                    float b = std::sin(hornT * 2.0f + hf * 6.28f + 4.189f) * 0.5f + 0.5f;
                    float spiral = std::sin(hf * 10.0f + hornT * 3.0f) * (2.0f - hf * 1.5f);
                    horn[hi] = sf::Vertex{
                        {cx + 24.0f + hf * 4.0f + spiral * 0.3f,
                         previewY - 22.0f - hf * 20.0f},
                        sf::Color(static_cast<uint8_t>(r*255), static_cast<uint8_t>(g*255),
                                  static_cast<uint8_t>(b*255))};
                }
                win.draw(horn);
                // Mane strands
                for (int ms = 0; ms < 4; ms++) {
                    float mf = static_cast<float>(ms) / 4.0f;
                    float r2 = std::sin(hornT * 2.0f + mf * 6.28f) * 0.5f + 0.5f;
                    float g2 = std::sin(hornT * 2.0f + mf * 6.28f + 2.094f) * 0.5f + 0.5f;
                    float b2 = std::sin(hornT * 2.0f + mf * 6.28f + 4.189f) * 0.5f + 0.5f;
                    sf::VertexArray mane(sf::PrimitiveType::LineStrip, 3);
                    float mx = cx + 14.0f - mf * 10.0f;
                    float my = previewY - 12.0f - mf * 6.0f;
                    float wave = std::sin(hornT * 3.0f + mf * 5.0f) * 5.0f;
                    mane[0] = sf::Vertex{{mx, my}, sf::Color(static_cast<uint8_t>(r2*255), static_cast<uint8_t>(g2*255), static_cast<uint8_t>(b2*255))};
                    mane[1] = sf::Vertex{{mx - 10.0f, my - 5.0f + wave}, mane[0].color};
                    mane[2] = sf::Vertex{{mx - 18.0f, my - 2.0f + wave * 1.3f}, mane[0].color};
                    win.draw(mane);
                }
                // Legs
                for (float lx : {-0.3f, -0.1f, 0.1f, 0.3f}) {
                    sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
                    leg[0] = sf::Vertex{{cx + lx * 40.0f, previewY + 16.0f}, pc};
                    leg[1] = sf::Vertex{{cx + lx * 40.0f, previewY + 30.0f}, pc};
                    win.draw(leg);
                }
                break;
            }
        }

        // Ready indicator
        if (ps.ready) {
            sf::Text readyText(*font, "READY!", 20);
            readyText.setFillColor(sf::Color(100, 255, 100));
            sf::FloatRect rb = readyText.getLocalBounds();
            readyText.setPosition({cx - rb.size.x / 2.0f, SCREEN_HEIGHT - 100.0f});
            win.draw(readyText);
        } else if (ps.joined) {
            sf::Text hint(*font, "ATK=Ready", 14);
            hint.setFillColor(sf::Color(150, 150, 150));
            sf::FloatRect hb = hint.getLocalBounds();
            hint.setPosition({cx - hb.size.x / 2.0f, SCREEN_HEIGHT - 95.0f});
            win.draw(hint);
        }
    }

    // Title
    if (fontLoaded && font) {
        sf::Text title(*font, "STICKBRAWL", 36);
        title.setFillColor(sf::Color::White);
        sf::FloatRect tb = title.getLocalBounds();
        title.setPosition({(SCREEN_WIDTH - tb.size.x) / 2.0f, 20.0f});
        win.draw(title);

        if (allPlayersReady()) {
            float pulse = std::sin(m_selectAnimTimer * 4.0f) * 0.3f + 0.7f;
            sf::Text startText(*font, "ENTER / SPACE to START", 22);
            startText.setFillColor(sf::Color(
                static_cast<uint8_t>(255 * pulse),
                static_cast<uint8_t>(255 * pulse),
                static_cast<uint8_t>(100 * pulse)));
            sf::FloatRect sb = startText.getLocalBounds();
            startText.setPosition({(SCREEN_WIDTH - sb.size.x) / 2.0f, SCREEN_HEIGHT - 45.0f});
            win.draw(startText);
        } else {
            sf::Text hint(*font, "At least 2 players needed. Use your controls to join!", 16);
            hint.setFillColor(sf::Color(120, 120, 120));
            sf::FloatRect hb = hint.getLocalBounds();
            hint.setPosition({(SCREEN_WIDTH - hb.size.x) / 2.0f, SCREEN_HEIGHT - 42.0f});
            win.draw(hint);
        }
    }

    m_renderer.display();
}

void Game::startGame() {
    const auto& rules = m_rulesEngine.getRules();
    m_physics.setGravity(rules.gravityX, rules.gravityY);
    m_arena.createDefaultArena(m_physics);

    const auto& spawns = m_arena.getSpawnPoints();
    m_players.clear();
    m_projectiles.clear();
    m_pickups.clear();

    int playerIdx = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!m_selectState[i].joined) continue;
        if (playerIdx >= static_cast<int>(spawns.size())) break;

        CharacterType ct = indexToType(m_selectState[i].charIndex);
        auto p = std::make_unique<StickFigure>(
            playerIdx, m_physics, spawns[playerIdx].x, spawns[playerIdx].y,
            m_playerColors[i], ct);
        p->setLives(rules.livesPerPlayer);
        p->setMaxHealth(rules.maxHealth);

        // Give innate weapons
        if (ct == CharacterType::Cobra) {
            auto* poison = m_weaponFactory.getWeapon("Poison Spit");
            if (poison) p->equipWeapon(*poison);
        } else if (ct == CharacterType::Unicorn) {
            auto* horn = m_weaponFactory.getWeapon("Horn Blast");
            if (horn) p->equipWeapon(*horn);
        }

        m_players.push_back(std::move(p));
        playerIdx++;
    }

    m_roundTimer = rules.roundTimeSeconds;
    m_weaponSpawnTimer = rules.weaponSpawnInterval;
    m_state = GameState::Playing;

    std::cout << "Game started with " << m_players.size() << " players!\n";
}

// ============================================================
// MAIN LOOP
// ============================================================

void Game::run() {
    sf::Clock clock;
    const float fixedDt = 1.0f / 60.0f;
    float accumulator = 0.0f;

    while (m_renderer.isOpen()) {
        float frameTime = clock.restart().asSeconds();
        if (frameTime > 0.25f) frameTime = 0.25f;
        accumulator += frameTime;

        if (m_state == GameState::CharSelect) {
            processCharSelectEvents();
            while (accumulator >= fixedDt) {
                updateCharSelect(fixedDt);
                m_input.update();
                accumulator -= fixedDt;
            }
            renderCharSelect();
        } else {
            processEvents();
            while (accumulator >= fixedDt) {
                update(fixedDt);
                m_input.update();
                accumulator -= fixedDt;
            }
            render();
        }
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
            // Return to character select
            if (k->code == sf::Keyboard::Key::Backspace && m_state == GameState::RoundOver) {
                m_state = GameState::CharSelect;
                for (auto& ps : m_selectState) { ps.ready = false; }
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

    // Apply weapon spread
    float spreadRad = weapon.spreadDegrees * 3.14159f / 180.0f;
    if (spreadRad > 0.0f) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> spreadDist(-spreadRad, spreadRad);
        aim += spreadDist(rng);
    }

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

    if (weapon.poisonDps > 0.0f && weapon.poisonDuration > 0.0f) {
        proj.isPoison = true;
        proj.poisonDps = weapon.poisonDps;
        proj.poisonDuration = weapon.poisonDuration;
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
            continue;
        }

        b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
        bool isExplosive = (proj.weapon.type == WeaponType::Explosive);
        float hitR = isExplosive ? proj.weapon.explosionRadius : 0.6f;

        for (auto& player : m_players) {
            if (player->getPlayerIndex() == proj.ownerIndex) continue;
            if (!player->isAlive()) continue;

            b2Vec2 plp = player->getPosition();
            float dx = pp.x - plp.x, dy = pp.y - plp.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < hitR) {
                if (proj.isPoison) {
                    player->takeDamage(5.0f, 0.0f, 0.0f);
                    player->applyPoison(proj.poisonDps, proj.poisonDuration);
                } else {
                    float dmg = proj.weapon.damage * rules.damageMultiplier;
                    if (isExplosive && proj.weapon.explosionRadius > 0.0f) {
                        float falloff = 1.0f - (dist / proj.weapon.explosionRadius);
                        dmg *= std::max(0.3f, falloff);
                    }
                    float kbDir = (plp.x > pp.x) ? 1.0f : -1.0f;
                    float kbX = proj.weapon.knockbackForce * kbDir * rules.knockbackMultiplier;
                    float kbY = proj.weapon.knockbackForce * 0.5f * rules.knockbackMultiplier;
                    player->takeDamage(dmg, kbX, kbY);
                }

                if (!isExplosive) {
                    proj.alive = false;
                    break;
                }
            }
        }

        if (isExplosive) {
            for (const auto& player : m_players) {
                if (player->getPlayerIndex() == proj.ownerIndex) continue;
                if (!player->isAlive()) continue;
                b2Vec2 plp = player->getPosition();
                float dx = pp.x - plp.x, dy = pp.y - plp.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < hitR) { proj.alive = false; break; }
            }
        }
    }

    for (auto& proj : m_projectiles) {
        if (!proj.alive) b2DestroyBody(proj.bodyId);
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
        // Don't spawn innate character weapons as pickups
        do {
            pickup.weapon = m_weaponFactory.getRandomWeapon();
        } while (pickup.weapon.name == "Fists" || pickup.weapon.name == "Poison Spit"
                 || pickup.weapon.name == "Horn Blast");
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
        if (!player->isAlive() || player->isWaitingToRespawn()) continue;
        if (player->getPosition().y < rules.fallDeathY) {
            player->takeDamage(9999.0f, 0.0f, 0.0f);
            int lives = player->getLives() - 1;
            player->setLives(lives);
            if (lives > 0) {
                size_t idx = static_cast<size_t>(player->getPlayerIndex());
                player->startRespawnTimer(rules.respawnDelay, spawns[idx].x, spawns[idx].y);
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
        sf::Vector2f sp = {SCREEN_CX + pickup.position.x * PPM,
                           SCREEN_CY - pickup.position.y * PPM + bob};

        sf::RectangleShape box({16.0f, 16.0f});
        box.setOrigin({8.0f, 8.0f});
        box.setPosition(sp);
        box.setFillColor(sf::Color(255, 200, 50, 200));
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(1.0f);
        box.setRotation(sf::degrees(pickup.bobTimer * 60.0f));
        m_renderer.getWindow().draw(box);

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

    for (const auto& p : m_players) p->draw(m_renderer.getWindow());

    for (const auto& proj : m_projectiles) {
        if (!proj.alive) continue;
        b2Vec2 pos = b2Body_GetPosition(proj.bodyId);
        sf::CircleShape c(4.0f); c.setOrigin({4.0f, 4.0f});
        c.setPosition({SCREEN_CX + pos.x * PPM, SCREEN_CY - pos.y * PPM});
        if (proj.isPoison)
            c.setFillColor(sf::Color(0, 220, 0));
        else
            c.setFillColor(sf::Color::Yellow);
        m_renderer.getWindow().draw(c);
    }

    m_hud.draw(m_renderer.getWindow(), m_players, m_roundTimer);

    if (m_state == GameState::RoundOver) {
        sf::RectangleShape overlay({SCREEN_WIDTH, SCREEN_HEIGHT});
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        m_renderer.getWindow().draw(overlay);
    }

    m_renderer.display();
}

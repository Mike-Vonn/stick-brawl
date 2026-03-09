#include "Game.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <sstream>
#include <optional>

static constexpr float PI = 3.14159265f;

// ─── Weapon pickup icon drawing ──────────────────────────────────────
static void drawWeaponIcon(sf::RenderTarget& target, const std::string& name,
                           sf::Vector2f pos, float timer, float scale = 1.0f) {
    // Scale transform around pos so icons can be drawn smaller above characters
    sf::RenderStates states;
    if (scale != 1.0f) {
        sf::Transform xf;
        xf.translate(pos);
        xf.scale({scale, scale});
        xf.translate(-pos);
        states.transform = xf;
    }
    auto draw = [&](const auto& drawable) { target.draw(drawable, states); };

    auto drawLine = [&](sf::Vector2f a, sf::Vector2f b, sf::Color c) {
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0] = sf::Vertex{a, c};
        line[1] = sf::Vertex{b, c};
        draw(line);
    };

    float rot = timer * 20.0f; // gentle slow rotation in degrees
    float gleam = std::sin(timer * 4.0f) * 0.3f + 0.7f; // pulsing brightness

    if (name == "Katana") {
        // Blade: long thin angled rectangle
        sf::Color blade(220, 230, 255, static_cast<uint8_t>(255 * gleam));
        sf::Color handle(140, 90, 40);
        sf::Color guard(200, 180, 100);

        sf::RectangleShape bladeShape({22.0f, 3.0f});
        bladeShape.setOrigin({11.0f, 1.5f});
        bladeShape.setPosition({pos.x, pos.y - 2.0f});
        bladeShape.setRotation(sf::degrees(-30.0f));
        bladeShape.setFillColor(blade);
        draw(bladeShape);

        // Guard (small cross piece)
        sf::RectangleShape guardShape({2.0f, 7.0f});
        guardShape.setOrigin({1.0f, 3.5f});
        guardShape.setPosition({pos.x - 6.0f, pos.y + 1.5f});
        guardShape.setRotation(sf::degrees(-30.0f));
        guardShape.setFillColor(guard);
        draw(guardShape);

        // Handle
        sf::RectangleShape handleShape({8.0f, 3.0f});
        handleShape.setOrigin({8.0f, 1.5f});
        handleShape.setPosition({pos.x - 7.0f, pos.y + 2.0f});
        handleShape.setRotation(sf::degrees(-30.0f));
        handleShape.setFillColor(handle);
        draw(handleShape);
    }
    else if (name == "Pistol") {
        // Gun silhouette: barrel + grip
        sf::Color metal(120, 120, 130);
        sf::Color outline(200, 200, 210);

        // Barrel
        sf::RectangleShape barrel({14.0f, 4.0f});
        barrel.setOrigin({7.0f, 2.0f});
        barrel.setPosition({pos.x + 2.0f, pos.y - 3.0f});
        barrel.setFillColor(metal);
        barrel.setOutlineColor(outline);
        barrel.setOutlineThickness(0.5f);
        draw(barrel);

        // Grip
        sf::RectangleShape grip({4.0f, 8.0f});
        grip.setOrigin({2.0f, 0.0f});
        grip.setPosition({pos.x - 2.0f, pos.y - 1.0f});
        grip.setFillColor(metal);
        grip.setOutlineColor(outline);
        grip.setOutlineThickness(0.5f);
        draw(grip);

        // Trigger guard (small arc)
        sf::CircleShape trigGuard(3.0f);
        trigGuard.setOrigin({3.0f, 3.0f});
        trigGuard.setPosition({pos.x + 1.0f, pos.y + 2.0f});
        trigGuard.setFillColor(sf::Color::Transparent);
        trigGuard.setOutlineColor(outline);
        trigGuard.setOutlineThickness(0.5f);
        draw(trigGuard);
    }
    else if (name == "Shotgun") {
        // Longer barrel + stock
        sf::Color metal(100, 100, 110);
        sf::Color wood(140, 95, 50);
        sf::Color outline(180, 180, 190);

        // Long barrel
        sf::RectangleShape barrel({22.0f, 4.0f});
        barrel.setOrigin({11.0f, 2.0f});
        barrel.setPosition({pos.x + 3.0f, pos.y - 2.0f});
        barrel.setFillColor(metal);
        barrel.setOutlineColor(outline);
        barrel.setOutlineThickness(0.5f);
        draw(barrel);

        // Wide muzzle end
        sf::RectangleShape muzzle({3.0f, 6.0f});
        muzzle.setOrigin({1.5f, 3.0f});
        muzzle.setPosition({pos.x + 14.0f, pos.y - 2.0f});
        muzzle.setFillColor(metal);
        draw(muzzle);

        // Stock
        sf::RectangleShape stock({10.0f, 5.0f});
        stock.setOrigin({10.0f, 2.0f});
        stock.setPosition({pos.x - 8.0f, pos.y - 1.0f});
        stock.setFillColor(wood);
        stock.setOutlineColor(outline);
        stock.setOutlineThickness(0.5f);
        draw(stock);

        // Grip
        sf::RectangleShape grip({3.0f, 6.0f});
        grip.setOrigin({1.5f, 0.0f});
        grip.setPosition({pos.x - 2.0f, pos.y});
        grip.setFillColor(wood);
        draw(grip);
    }
    else if (name == "Grenade Launcher") {
        // Tube + grenade at tip
        sf::Color tube(80, 100, 60);     // military green
        sf::Color grenade(100, 110, 70);
        sf::Color outline(140, 160, 100);

        // Tube body
        sf::RectangleShape body({20.0f, 5.0f});
        body.setOrigin({10.0f, 2.5f});
        body.setPosition(pos);
        body.setFillColor(tube);
        body.setOutlineColor(outline);
        body.setOutlineThickness(0.5f);
        draw(body);

        // Grenade (circle at muzzle)
        sf::CircleShape gren(4.0f);
        gren.setOrigin({4.0f, 4.0f});
        gren.setPosition({pos.x + 13.0f, pos.y});
        gren.setFillColor(grenade);
        gren.setOutlineColor(outline);
        gren.setOutlineThickness(0.5f);
        draw(gren);

        // Grip
        sf::RectangleShape grip({3.0f, 6.0f});
        grip.setOrigin({1.5f, 0.0f});
        grip.setPosition({pos.x - 3.0f, pos.y + 2.5f});
        grip.setFillColor(tube);
        draw(grip);
    }
    else if (name == "Nuclear Hand Grenade") {
        // Glowing circle with radiation symbol
        uint8_t glowA = static_cast<uint8_t>(80 * gleam);
        sf::Color glow(180, 255, 50, glowA);
        sf::Color shell(80, 80, 80);
        sf::Color radColor(255, 220, 0);

        // Glow aura
        sf::CircleShape aura(12.0f);
        aura.setOrigin({12.0f, 12.0f});
        aura.setPosition(pos);
        aura.setFillColor(glow);
        draw(aura);

        // Grenade body
        sf::CircleShape body(7.0f);
        body.setOrigin({7.0f, 7.0f});
        body.setPosition(pos);
        body.setFillColor(shell);
        body.setOutlineColor(sf::Color(160, 160, 160));
        body.setOutlineThickness(1.0f);
        draw(body);

        // Radiation trefoil: 3 lines from center
        for (int i = 0; i < 3; i++) {
            float angle = static_cast<float>(i) * 2.094f + timer * 0.5f; // 120° apart, slow spin
            float ex = pos.x + std::cos(angle) * 5.0f;
            float ey = pos.y + std::sin(angle) * 5.0f;
            drawLine(pos, {ex, ey}, radColor);

            // Small wedge dot at end
            sf::CircleShape dot(1.5f);
            dot.setOrigin({1.5f, 1.5f});
            dot.setPosition({ex, ey});
            dot.setFillColor(radColor);
            draw(dot);
        }

        // Center dot
        sf::CircleShape center(2.0f);
        center.setOrigin({2.0f, 2.0f});
        center.setPosition(pos);
        center.setFillColor(radColor);
        draw(center);
    }
    else if (name == "Milk Squirt") {
        // Baby bottle
        sf::Color bottle(240, 240, 255, 220);
        sf::Color nippleC(255, 180, 140);
        sf::Color milk(255, 255, 255, 180);

        // Bottle body
        sf::RectangleShape body({6.0f, 14.0f});
        body.setOrigin({3.0f, 7.0f});
        body.setPosition({pos.x, pos.y + 2.0f});
        body.setFillColor(bottle);
        body.setOutlineColor(sf::Color(180, 180, 200));
        body.setOutlineThickness(0.5f);
        draw(body);

        // Milk inside (slightly smaller, white)
        sf::RectangleShape milkFill({4.0f, 8.0f});
        milkFill.setOrigin({2.0f, 4.0f});
        milkFill.setPosition({pos.x, pos.y + 4.0f});
        milkFill.setFillColor(milk);
        draw(milkFill);

        // Nipple on top
        sf::CircleShape nip(3.5f);
        nip.setOrigin({3.5f, 3.5f});
        nip.setPosition({pos.x, pos.y - 6.0f});
        nip.setFillColor(nippleC);
        draw(nip);
    }
    else {
        // Fallback: generic gold box (for any future weapons)
        sf::RectangleShape box({16.0f, 16.0f});
        box.setOrigin({8.0f, 8.0f});
        box.setPosition(pos);
        box.setFillColor(sf::Color(255, 200, 50, 200));
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(1.0f);
        box.setRotation(sf::degrees(timer * 60.0f));
        draw(box);
    }
}

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
        case 4: return CharacterType::Crocodile;
        case 5: return CharacterType::StickLady;
        case 6: return CharacterType::Dragon;
        case 7: return CharacterType::MrDiaperPants;
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

            // Tab cycles level
            if (k->code == sf::Keyboard::Key::Tab) {
                m_selectedLevel = (m_selectedLevel + 1) % Arena::getLevelCount();
            }

            // Grave/tilde toggles wrap-around
            if (k->code == sf::Keyboard::Key::Grave) {
                m_wrapAround = !m_wrapAround;
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
            case CharacterType::Crocodile: {
                // Body (long, low)
                sf::ConvexShape crocBody(6);
                crocBody.setPoint(0, {cx - 22.0f, previewY - 5.0f});
                crocBody.setPoint(1, {cx + 10.0f, previewY - 7.0f});
                crocBody.setPoint(2, {cx + 18.0f, previewY - 3.0f});
                crocBody.setPoint(3, {cx + 18.0f, previewY + 6.0f});
                crocBody.setPoint(4, {cx - 8.0f, previewY + 8.0f});
                crocBody.setPoint(5, {cx - 22.0f, previewY + 4.0f});
                crocBody.setFillColor(pc);
                crocBody.setOutlineColor(sf::Color::Black);
                crocBody.setOutlineThickness(1.0f);
                win.draw(crocBody);
                // Snout/jaw
                sf::ConvexShape cSnout(4);
                cSnout.setPoint(0, {cx + 18.0f, previewY - 5.0f});
                cSnout.setPoint(1, {cx + 40.0f, previewY - 2.0f});
                cSnout.setPoint(2, {cx + 38.0f, previewY + 2.0f});
                cSnout.setPoint(3, {cx + 18.0f, previewY + 4.0f});
                cSnout.setFillColor(pc);
                cSnout.setOutlineColor(sf::Color::Black);
                cSnout.setOutlineThickness(1.0f);
                win.draw(cSnout);
                // Teeth
                for (int ti = 0; ti < 4; ti++) {
                    float ttx = cx + 22.0f + static_cast<float>(ti) * 4.5f;
                    sf::ConvexShape tooth(3);
                    tooth.setPoint(0, {ttx - 1.0f, previewY + 1.0f});
                    tooth.setPoint(1, {ttx, previewY + 4.5f});
                    tooth.setPoint(2, {ttx + 1.0f, previewY + 1.0f});
                    tooth.setFillColor(sf::Color(240, 235, 210));
                    win.draw(tooth);
                }
                // Eye
                sf::CircleShape crocEye(2.0f);
                crocEye.setOrigin({2.0f, 2.0f});
                crocEye.setPosition({cx + 14.0f, previewY - 7.0f});
                crocEye.setFillColor(sf::Color(200, 180, 50));
                win.draw(crocEye);
                // Scutes
                for (int si = 0; si < 4; si++) {
                    float ssx = cx - 14.0f + static_cast<float>(si) * 7.0f;
                    sf::ConvexShape scute(3);
                    scute.setPoint(0, {ssx - 2.0f, previewY - 5.0f});
                    scute.setPoint(1, {ssx, previewY - 10.0f});
                    scute.setPoint(2, {ssx + 2.0f, previewY - 5.0f});
                    scute.setFillColor(sf::Color(pc.r * 3 / 4, pc.g * 3 / 4, pc.b * 3 / 4));
                    win.draw(scute);
                }
                // Tail
                sf::VertexArray crocTail(sf::PrimitiveType::LineStrip, 5);
                float crocT = m_selectAnimTimer;
                for (int tti = 0; tti < 5; tti++) {
                    float ttf = static_cast<float>(tti) / 4.0f;
                    float wave = std::sin(crocT * 2.0f + ttf * 3.0f) * 4.0f * ttf;
                    crocTail[tti] = sf::Vertex{{cx - 22.0f - ttf * 18.0f, previewY + wave}, pc};
                }
                win.draw(crocTail);
                // Stubby legs
                for (float clx : {-0.2f, 0.0f, 0.2f, 0.4f}) {
                    sf::VertexArray cleg(sf::PrimitiveType::Lines, 2);
                    cleg[0] = sf::Vertex{{cx + clx * 35.0f, previewY + 6.0f}, pc};
                    cleg[1] = sf::Vertex{{cx + clx * 35.0f, previewY + 16.0f}, pc};
                    win.draw(cleg);
                }
                break;
            }
            case CharacterType::StickLady: {
                // Head
                sf::CircleShape slHead(12.0f);
                slHead.setOrigin({12.0f, 12.0f});
                slHead.setPosition({cx, previewY - 50.0f});
                slHead.setFillColor(sf::Color::Transparent);
                slHead.setOutlineColor(pc); slHead.setOutlineThickness(2.0f);
                win.draw(slHead);
                // Hair strands
                for (int hi = 0; hi < 5; hi++) {
                    float hfrac = static_cast<float>(hi) / 4.0f;
                    float hx = cx - 6.0f + hfrac * 3.0f;
                    float wave = std::sin(m_selectAnimTimer * 2.0f + hfrac * 2.0f) * 3.0f;
                    sf::VertexArray hair(sf::PrimitiveType::LineStrip, 3);
                    hair[0] = sf::Vertex{{hx, previewY - 52.0f + hfrac * 4.0f}, pc};
                    hair[1] = sf::Vertex{{hx - 8.0f + wave, previewY - 40.0f}, pc};
                    hair[2] = sf::Vertex{{hx - 10.0f + wave, previewY - 28.0f}, sf::Color(pc.r, pc.g, pc.b, 150)};
                    win.draw(hair);
                }
                // Eyelashes
                sf::CircleShape slEye(1.5f);
                slEye.setOrigin({1.5f, 1.5f});
                slEye.setPosition({cx + 4.0f, previewY - 51.0f});
                slEye.setFillColor(pc);
                win.draw(slEye);
                for (int li = 0; li < 3; li++) {
                    float la = -0.4f + static_cast<float>(li) * 0.4f;
                    sf::VertexArray lash(sf::PrimitiveType::Lines, 2);
                    lash[0] = sf::Vertex{{cx + 4.0f, previewY - 53.0f}, pc};
                    lash[1] = sf::Vertex{{cx + 4.0f + std::sin(la) * 4.0f,
                                           previewY - 53.0f - std::cos(la) * 4.0f}, pc};
                    win.draw(lash);
                }
                // Body
                sf::VertexArray slBody(sf::PrimitiveType::Lines, 2);
                slBody[0] = sf::Vertex{{cx, previewY - 38.0f}, pc};
                slBody[1] = sf::Vertex{{cx, previewY - 5.0f}, pc};
                win.draw(slBody);
                // Arms
                for (float s : {-1.0f, 1.0f}) {
                    sf::VertexArray arm(sf::PrimitiveType::Lines, 2);
                    arm[0] = sf::Vertex{{cx, previewY - 28.0f}, pc};
                    arm[1] = sf::Vertex{{cx + s * 18.0f, previewY - 15.0f}, pc};
                    win.draw(arm);
                }
                // Triangle skirt
                sf::ConvexShape slSkirt(3);
                slSkirt.setPoint(0, {cx - 4.0f, previewY - 6.0f});
                slSkirt.setPoint(1, {cx + 4.0f, previewY - 6.0f});
                float skirtSway = std::sin(m_selectAnimTimer * 1.5f) * 2.0f;
                slSkirt.setPoint(2, {cx + skirtSway, previewY + 14.0f});
                sf::Color skirtC(
                    static_cast<uint8_t>(std::min(255, pc.r + 30)),
                    static_cast<uint8_t>(std::min(255, pc.g + 10)),
                    static_cast<uint8_t>(std::min(255, pc.b + 40)));
                slSkirt.setFillColor(skirtC);
                slSkirt.setOutlineColor(pc); slSkirt.setOutlineThickness(1.0f);
                win.draw(slSkirt);
                // Legs below skirt
                for (float s : {-1.0f, 1.0f}) {
                    sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
                    leg[0] = sf::Vertex{{cx + s * 4.0f, previewY + 14.0f}, pc};
                    leg[1] = sf::Vertex{{cx + s * 10.0f, previewY + 28.0f}, pc};
                    win.draw(leg);
                }
                // Purse
                sf::RectangleShape slPurse({8.0f, 6.0f});
                slPurse.setOrigin({4.0f, 0.0f});
                float purseSwing2 = std::sin(m_selectAnimTimer * 3.0f) * 8.0f;
                slPurse.setPosition({cx + 18.0f, previewY - 13.0f});
                slPurse.setRotation(sf::degrees(purseSwing2));
                slPurse.setFillColor(sf::Color(200, 80, 150));
                slPurse.setOutlineColor(sf::Color(150, 50, 100));
                slPurse.setOutlineThickness(1.0f);
                win.draw(slPurse);
                break;
            }
            case CharacterType::Dragon: {
                // Body (elongated)
                sf::ConvexShape dBody(6);
                dBody.setPoint(0, {cx - 20.0f, previewY - 4.0f});
                dBody.setPoint(1, {cx + 10.0f, previewY - 7.0f});
                dBody.setPoint(2, {cx + 18.0f, previewY - 3.0f});
                dBody.setPoint(3, {cx + 18.0f, previewY + 6.0f});
                dBody.setPoint(4, {cx - 8.0f, previewY + 8.0f});
                dBody.setPoint(5, {cx - 20.0f, previewY + 4.0f});
                dBody.setFillColor(pc);
                dBody.setOutlineColor(sf::Color::Black);
                dBody.setOutlineThickness(1.0f);
                win.draw(dBody);

                // Head (angular)
                sf::ConvexShape dHead(5);
                dHead.setPoint(0, {cx + 18.0f, previewY - 6.0f});
                dHead.setPoint(1, {cx + 38.0f, previewY - 4.0f});
                dHead.setPoint(2, {cx + 40.0f, previewY});
                dHead.setPoint(3, {cx + 36.0f, previewY + 3.0f});
                dHead.setPoint(4, {cx + 18.0f, previewY + 2.0f});
                dHead.setFillColor(pc);
                dHead.setOutlineColor(sf::Color::Black);
                dHead.setOutlineThickness(1.0f);
                win.draw(dHead);

                // Horns
                for (float hs : {-1.0f, 1.0f}) {
                    sf::ConvexShape horn(3);
                    horn.setPoint(0, {cx + 22.0f + hs * 3.0f, previewY - 6.0f});
                    horn.setPoint(1, {cx + 18.0f + hs * 2.0f, previewY - 16.0f});
                    horn.setPoint(2, {cx + 26.0f + hs * 3.0f, previewY - 7.0f});
                    horn.setFillColor(sf::Color(180, 160, 100));
                    win.draw(horn);
                }

                // Wings (bat-like)
                sf::ConvexShape wing(4);
                wing.setPoint(0, {cx - 8.0f, previewY - 4.0f});
                wing.setPoint(1, {cx - 5.0f, previewY - 30.0f});
                wing.setPoint(2, {cx + 10.0f, previewY - 25.0f});
                wing.setPoint(3, {cx + 5.0f, previewY - 2.0f});
                wing.setFillColor(sf::Color(pc.r * 3 / 4, pc.g * 3 / 4, pc.b * 3 / 4, 180));
                win.draw(wing);

                // Eye
                sf::CircleShape dEye(2.0f);
                dEye.setOrigin({2.0f, 2.0f});
                dEye.setPosition({cx + 26.0f, previewY - 5.0f});
                dEye.setFillColor(sf::Color(255, 180, 0));
                win.draw(dEye);

                // Spines along back
                for (int si = 0; si < 5; si++) {
                    float sx = cx - 14.0f + static_cast<float>(si) * 6.0f;
                    sf::ConvexShape spine(3);
                    spine.setPoint(0, {sx - 1.5f, previewY - 5.0f});
                    spine.setPoint(1, {sx, previewY - 12.0f});
                    spine.setPoint(2, {sx + 1.5f, previewY - 5.0f});
                    spine.setFillColor(sf::Color(pc.r * 3 / 4, pc.g * 3 / 4, pc.b * 3 / 4));
                    win.draw(spine);
                }

                // Tail (animated)
                sf::VertexArray dTail(sf::PrimitiveType::LineStrip, 5);
                float dT = m_selectAnimTimer;
                for (int ti = 0; ti < 5; ti++) {
                    float tf = static_cast<float>(ti) / 4.0f;
                    float wave = std::sin(dT * 2.0f + tf * 3.0f) * 4.0f * tf;
                    dTail[ti] = sf::Vertex{{cx - 20.0f - tf * 18.0f, previewY + wave}, pc};
                }
                win.draw(dTail);

                // Legs
                for (float lx : {-0.2f, 0.0f, 0.2f, 0.4f}) {
                    sf::VertexArray dLeg(sf::PrimitiveType::Lines, 2);
                    dLeg[0] = sf::Vertex{{cx + lx * 35.0f, previewY + 6.0f}, pc};
                    dLeg[1] = sf::Vertex{{cx + lx * 35.0f, previewY + 16.0f}, pc};
                    win.draw(dLeg);
                }

                // Fire breath hint
                for (int fi = 0; fi < 3; fi++) {
                    float ff = static_cast<float>(fi) / 3.0f;
                    float fx = cx + 40.0f + ff * 12.0f;
                    float fy = previewY - 2.0f + std::sin(m_selectAnimTimer * 6.0f + ff * 4.0f) * 3.0f;
                    float fsz = 2.5f - ff * 0.8f;
                    sf::CircleShape fireDot(fsz);
                    fireDot.setOrigin({fsz, fsz});
                    fireDot.setPosition({fx, fy});
                    uint8_t fg = static_cast<uint8_t>(150 - ff * 100);
                    fireDot.setFillColor(sf::Color(255, fg, 0, static_cast<uint8_t>(200 - ff * 80)));
                    win.draw(fireDot);
                }
                break;
            }
            case CharacterType::MrDiaperPants: {
                // Big round belly
                sf::CircleShape belly(22.0f);
                belly.setScale({1.0f, 0.85f});
                belly.setOrigin({22.0f, 22.0f});
                belly.setPosition({cx, previewY + 2.0f});
                belly.setFillColor(sf::Color(pc.r, pc.g, pc.b, 120));
                belly.setOutlineColor(pc);
                belly.setOutlineThickness(2.0f);
                win.draw(belly);

                // Diaper
                sf::ConvexShape diaper(4);
                diaper.setPoint(0, {cx - 16.0f, previewY + 12.0f});
                diaper.setPoint(1, {cx + 16.0f, previewY + 12.0f});
                diaper.setPoint(2, {cx + 12.0f, previewY + 26.0f});
                diaper.setPoint(3, {cx - 12.0f, previewY + 26.0f});
                diaper.setFillColor(sf::Color(255, 255, 255, 220));
                diaper.setOutlineColor(sf::Color(200, 200, 200));
                diaper.setOutlineThickness(1.0f);
                win.draw(diaper);

                // Diaper pin
                sf::CircleShape pin(2.5f);
                pin.setOrigin({2.5f, 2.5f});
                pin.setPosition({cx + 5.0f, previewY + 16.0f});
                pin.setFillColor(sf::Color(80, 150, 255));
                win.draw(pin);

                // Big head
                sf::CircleShape head(15.0f);
                head.setOrigin({15.0f, 15.0f});
                head.setPosition({cx, previewY - 28.0f});
                head.setFillColor(sf::Color(pc.r, pc.g, pc.b, 80));
                head.setOutlineColor(pc);
                head.setOutlineThickness(2.0f);
                win.draw(head);

                // Eyes
                for (float es : {-1.0f, 1.0f}) {
                    sf::CircleShape eye(2.0f);
                    eye.setOrigin({2.0f, 2.0f});
                    eye.setPosition({cx + es * 6.0f, previewY - 30.0f});
                    eye.setFillColor(pc);
                    win.draw(eye);
                }

                // Dopey smile (arc)
                sf::CircleShape smile(6.0f, 12);
                smile.setOrigin({6.0f, 6.0f});
                smile.setPosition({cx, previewY - 24.0f});
                smile.setFillColor(sf::Color::Transparent);
                smile.setOutlineColor(pc);
                smile.setOutlineThickness(1.0f);
                win.draw(smile);

                // Stubby arms
                for (float s : {-1.0f, 1.0f}) {
                    sf::VertexArray arm(sf::PrimitiveType::Lines, 2);
                    arm[0] = sf::Vertex{{cx + s * 14.0f, previewY - 6.0f}, pc};
                    arm[1] = sf::Vertex{{cx + s * 26.0f, previewY + 6.0f}, pc};
                    win.draw(arm);
                }

                // Baby bottle in right hand
                sf::RectangleShape bottle({5.0f, 12.0f});
                bottle.setOrigin({2.5f, 12.0f});
                bottle.setPosition({cx + 29.0f, previewY + 6.0f});
                bottle.setFillColor(sf::Color(240, 240, 255, 200));
                bottle.setOutlineColor(sf::Color(180, 180, 200));
                bottle.setOutlineThickness(0.5f);
                win.draw(bottle);
                sf::CircleShape nipple(3.0f);
                nipple.setOrigin({3.0f, 3.0f});
                nipple.setPosition({cx + 29.0f, previewY - 8.0f});
                nipple.setFillColor(sf::Color(255, 180, 140));
                win.draw(nipple);

                // Stubby legs
                for (float s : {-1.0f, 1.0f}) {
                    sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
                    leg[0] = sf::Vertex{{cx + s * 8.0f, previewY + 24.0f}, pc};
                    leg[1] = sf::Vertex{{cx + s * 10.0f, previewY + 38.0f}, pc};
                    win.draw(leg);
                }

                // Milk squirt hint
                for (int mi = 0; mi < 3; mi++) {
                    float mf = static_cast<float>(mi) / 3.0f;
                    float mx = cx + 32.0f + mf * 10.0f;
                    float my = previewY - 6.0f + std::sin(m_selectAnimTimer * 5.0f + mf * 3.0f) * 4.0f;
                    sf::CircleShape milk(2.0f - mf * 0.5f);
                    milk.setOrigin({2.0f - mf * 0.5f, 2.0f - mf * 0.5f});
                    milk.setPosition({mx, my});
                    milk.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(200 - mf * 80)));
                    win.draw(milk);
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
        title.setPosition({(SCREEN_WIDTH - tb.size.x) / 2.0f, 12.0f});
        win.draw(title);

        // Level selector
        std::string levelStr = "Level: < " + Arena::getLevelName(m_selectedLevel) + " >  [TAB]";
        sf::Text levelText(*font, levelStr, 18);
        levelText.setFillColor(sf::Color(200, 180, 100));
        sf::FloatRect lb = levelText.getLocalBounds();
        levelText.setPosition({(SCREEN_WIDTH - lb.size.x) / 2.0f, 56.0f});
        win.draw(levelText);

        // Wrap-around toggle
        std::string wrapStr = "Wrap-Around: " + std::string(m_wrapAround ? "ON" : "OFF") + "  [`]";
        sf::Text wrapText(*font, wrapStr, 16);
        wrapText.setFillColor(m_wrapAround ? sf::Color(100, 255, 180) : sf::Color(150, 150, 150));
        sf::FloatRect wb = wrapText.getLocalBounds();
        wrapText.setPosition({(SCREEN_WIDTH - wb.size.x) / 2.0f, 78.0f});
        win.draw(wrapText);

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
    m_arena.createLevel(m_physics, m_selectedLevel);

    const auto& spawns = m_arena.getSpawnPoints();
    m_players.clear();
    m_projectiles.clear();
    m_pickups.clear();
    m_explosions.clear();

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

        // Give innate weapons (replaces slot 0)
        if (ct == CharacterType::Cobra) {
            auto* poison = m_weaponFactory.getWeapon("Poison Spit");
            if (poison) p->setInnateWeapon(*poison);
        } else if (ct == CharacterType::Unicorn) {
            auto* horn = m_weaponFactory.getWeapon("Horn Blast");
            if (horn) p->setInnateWeapon(*horn);
        } else if (ct == CharacterType::Crocodile) {
            auto* jaw = m_weaponFactory.getWeapon("Jaw Snap");
            if (jaw) p->setInnateWeapon(*jaw);
        } else if (ct == CharacterType::StickLady) {
            auto* purse = m_weaponFactory.getWeapon("Purse Swing");
            if (purse) p->setInnateWeapon(*purse);
        } else if (ct == CharacterType::Dragon) {
            auto* fire = m_weaponFactory.getWeapon("Fire Breath");
            if (fire) p->setInnateWeapon(*fire);
        } else if (ct == CharacterType::MrDiaperPants) {
            auto* milk = m_weaponFactory.getWeapon("Milk Squirt");
            if (milk) p->setInnateWeapon(*milk);
        }

        m_players.push_back(std::move(p));
        playerIdx++;
    }

    m_roundTimer = rules.roundTimeSeconds;
    m_weaponSpawnTimer = rules.weaponSpawnInterval;
    m_state = GameState::Playing;

    // Initialize death tracking
    m_wasAlive.clear();
    m_wasAlive.resize(m_players.size(), true);

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
                m_deathAnims.cleanupAll(m_physics);
                const auto& spawns = m_arena.getSpawnPoints();
                for (size_t i = 0; i < m_players.size(); i++)
                    m_players[i]->respawn(spawns[i].x, spawns[i].y);
                m_pickups.clear();
                m_roundTimer = m_rulesEngine.getRules().roundTimeSeconds;
                m_state = GameState::Playing;
            }
            // Return to character select
            if (k->code == sf::Keyboard::Key::Backspace && m_state == GameState::RoundOver) {
                m_deathAnims.cleanupAll(m_physics);
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
    m_arena.updateFire(m_physics, dt);

    // Burning platform contact damage
    for (auto& player : m_players) {
        if (!player->isAlive()) continue;
        b2Vec2 pos = player->getPosition();
        if (m_arena.getBurningPlatformAt(pos.x, pos.y) >= 0 && !player->isBurning()) {
            player->applyBurn(4.0f, 2.0f);
        }
    }

    checkFallDeath();
    checkPlayerDeaths();
    m_deathAnims.update(dt);
    m_deathAnims.cleanup(m_physics);
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

        // Dragon glide: hold jump while airborne
        if (pi.jump && !pi.jumpPressed && player->getCharacterType() == CharacterType::Dragon) {
            player->glide(dt);
        }

        // Aiming
        if (pi.aimUp) player->aimUp();
        else if (pi.aimDown) player->aimDown();
        else player->resetAim();

        if (pi.swapPressed) player->switchWeapon();

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
            target->takeDamage(dmg, kbX, kbY, weapon.name, weapon.type, weapon.deathAnim);
        }
    }

    // Worms-style terrain carving from melee — small chip in front of attacker
    float envR = weapon.envDamageRadius;
    if (envR <= 0.0f) envR = weapon.damage * 0.015f; // small carve radius
    float hitX = ap.x + dir * weapon.range * 0.6f;
    float hitY = ap.y;
    m_arena.carveCircle(m_physics, hitX, hitY, envR);
}

void Game::spawnProjectile(StickFigure& shooter) {
    const auto& weapon = shooter.getCurrentWeapon();
    if (weapon.type == WeaponType::Melee) return;

    b2Vec2 pos = shooter.getPosition();
    float dir = static_cast<float>(shooter.getFacingDirection());
    float baseAim = shooter.getAimAngle();
    float spreadRad = weapon.spreadDegrees * PI / 180.0f;

    static std::mt19937 rng(std::random_device{}());

    int pellets = std::max(1, weapon.pelletCount);
    for (int p = 0; p < pellets; p++) {
        float aim = baseAim;

        if (pellets > 1) {
            // Even spread across the cone, with a little randomness
            float evenSpread = -spreadRad + 2.0f * spreadRad * (static_cast<float>(p) / static_cast<float>(pellets - 1));
            std::uniform_real_distribution<float> jitter(-spreadRad * 0.15f, spreadRad * 0.15f);
            aim += evenSpread + jitter(rng);
        } else if (spreadRad > 0.0f) {
            std::uniform_real_distribution<float> spreadDist(-spreadRad, spreadRad);
            aim += spreadDist(rng);
        }

        // Slight speed variance for multi-pellet
        float speed = weapon.projectileSpeed;
        if (pellets > 1) {
            std::uniform_real_distribution<float> speedVar(0.85f, 1.15f);
            speed *= speedVar(rng);
        }

        float vx = speed * dir * std::cos(aim);
        float vy = speed * std::sin(aim);

        // Smaller pellets for shotgun
        float radius = (pellets > 1) ? 0.08f : 0.15f;
        float mass = (pellets > 1) ? 0.05f : 0.1f;

        b2BodyId bullet = m_physics.createDynamicCircle(
            pos.x + dir * 0.5f, pos.y + 0.3f, radius, mass,
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
        if (weapon.burnDps > 0.0f && weapon.burnDuration > 0.0f) {
            proj.isBurn = true;
            proj.burnDps = weapon.burnDps;
            proj.burnDuration = weapon.burnDuration;
        }

        m_projectiles.push_back(proj);
    }
}

void Game::updateProjectiles(float dt) {
    const auto& rules = m_rulesEngine.getRules();

    for (auto& proj : m_projectiles) {
        if (!proj.alive) continue;
        proj.lifetime -= dt;

        b2Vec2 pp = b2Body_GetPosition(proj.bodyId);

        // Wrap projectiles around if enabled
        if (m_wrapAround) {
            float worldHalfW = SCREEN_WIDTH / PPM / 2.0f + 2.0f;
            float worldTop   = SCREEN_HEIGHT / PPM / 2.0f + 2.0f;
            float worldBot   = -20.0f;
            bool wrapped = false;
            if (pp.x < -worldHalfW)  { pp.x = worldHalfW - 1.0f; wrapped = true; }
            if (pp.x > worldHalfW)   { pp.x = -worldHalfW + 1.0f; wrapped = true; }
            if (pp.y < worldBot)     { pp.y = worldTop; wrapped = true; }
            if (wrapped) {
                b2Body_SetTransform(proj.bodyId, pp, b2Body_GetRotation(proj.bodyId));
            }
        }
        bool isExplosive = (proj.weapon.type == WeaponType::Explosive);
        float hitR = isExplosive ? proj.weapon.explosionRadius : 0.6f;

        // Check if explosive projectile has stopped moving (hit a platform)
        bool contactDetonation = false;
        if (isExplosive && proj.lifetime < proj.weapon.projectileLifetime - 0.1f) {
            b2Vec2 vel = b2Body_GetLinearVelocity(proj.bodyId);
            float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
            if (speed < 1.0f) contactDetonation = true;
        }

        // Lifetime expiry for explosives = detonate in place
        bool expired = proj.lifetime <= 0.0f;
        bool shouldDetonate = contactDetonation || (expired && isExplosive);

        if (expired && !isExplosive) {
            // Small carve where bullet lands
            float envR = proj.weapon.envDamageRadius;
            if (envR <= 0.0f) envR = proj.weapon.damage * 0.015f;
            m_arena.carveCircle(m_physics, pp.x, pp.y, envR);

            // Fire projectiles ignite flammable platforms
            if (proj.isBurn) {
                const auto& platforms = m_arena.getPlatforms();
                for (size_t pi2 = 0; pi2 < platforms.size(); pi2++) {
                    const auto& plat = platforms[pi2];
                    if (!plat.alive) continue;
                    if (std::abs(pp.x - plat.cx) < plat.halfWidth + 0.5f &&
                        std::abs(pp.y - plat.cy) < plat.halfHeight + 0.5f) {
                        m_arena.ignitePlatform(pi2);
                    }
                }
            }

            proj.alive = false;
            continue;
        }

        // Check player hits
        bool hitAnyPlayer = false;
        for (auto& player : m_players) {
            if (player->getPlayerIndex() == proj.ownerIndex) continue;
            if (!player->isAlive()) continue;

            b2Vec2 plp = player->getPosition();
            float dx = pp.x - plp.x, dy = pp.y - plp.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // For non-explosive: check close hit. For explosive: check blast radius on detonation
            float checkR = isExplosive ? (shouldDetonate ? hitR : 0.6f) : 0.6f;

            if (dist < checkR) {
                if (proj.isBurn) {
                    float dmg = proj.weapon.damage * rules.damageMultiplier;
                    float kbDir = (plp.x > pp.x) ? 1.0f : -1.0f;
                    float kbX = proj.weapon.knockbackForce * kbDir * rules.knockbackMultiplier;
                    float kbY = proj.weapon.knockbackForce * 0.5f * rules.knockbackMultiplier;
                    player->takeDamage(dmg, kbX, kbY, proj.weapon.name, proj.weapon.type, proj.weapon.deathAnim);
                    player->applyBurn(proj.burnDps, proj.burnDuration);
                } else if (proj.isPoison) {
                    player->takeDamage(5.0f, 0.0f, 0.0f, "Poison Spit", WeaponType::Projectile);
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
                    player->takeDamage(dmg, kbX, kbY, proj.weapon.name, proj.weapon.type, proj.weapon.deathAnim);
                }

                if (!isExplosive) {
                    // Carve terrain at impact point
                    float envR = proj.weapon.envDamageRadius;
                    if (envR <= 0.0f) envR = proj.weapon.damage * 0.02f;
                    m_arena.carveCircle(m_physics, pp.x, pp.y, envR);
                    proj.alive = false;
                    break;
                }
                hitAnyPlayer = true;
                shouldDetonate = true;
            }
        }

        // Detonate explosive (contact, timer, or direct hit)
        if (isExplosive && shouldDetonate && proj.alive) {
            // Damage all players in blast radius (if we haven't already from the loop above)
            if (!hitAnyPlayer) {
                for (auto& player : m_players) {
                    if (player->getPlayerIndex() == proj.ownerIndex) continue;
                    if (!player->isAlive()) continue;
                    b2Vec2 plp = player->getPosition();
                    float dx = pp.x - plp.x, dy = pp.y - plp.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < hitR) {
                        float dmg = proj.weapon.damage * rules.damageMultiplier;
                        float falloff = 1.0f - (dist / proj.weapon.explosionRadius);
                        dmg *= std::max(0.3f, falloff);
                        float kbDir = (plp.x > pp.x) ? 1.0f : -1.0f;
                        float kbX = proj.weapon.knockbackForce * kbDir * rules.knockbackMultiplier;
                        float kbY = proj.weapon.knockbackForce * 0.5f * rules.knockbackMultiplier;
                        player->takeDamage(dmg, kbX, kbY, proj.weapon.name, proj.weapon.type, proj.weapon.deathAnim);
                    }
                }
            }

            // Carve terrain — nuke uses full explosion radius, regular explosives a bit less
            if (proj.weapon.destroysPlatforms) {
                m_arena.carveCircle(m_physics, pp.x, pp.y, proj.weapon.explosionRadius);
            } else {
                m_arena.carveCircle(m_physics, pp.x, pp.y, proj.weapon.explosionRadius * 0.6f);
            }

            // Spawn visual explosion effect
            ExplosionEffect fx;
            fx.x = pp.x;
            fx.y = pp.y;
            fx.radius = proj.weapon.explosionRadius;
            fx.timer = 0.0f;
            fx.isNuke = proj.weapon.destroysPlatforms;
            fx.duration = fx.isNuke ? 2.5f : 0.8f;
            fx.alive = true;
            m_explosions.push_back(fx);

            proj.alive = false;
        }
    }

    // Deferred body destruction
    for (auto& proj : m_projectiles) {
        if (!proj.alive) b2DestroyBody(proj.bodyId);
    }
    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
                        [](const Projectile& p) { return !p.alive; }),
        m_projectiles.end());

    // Update explosion effects
    for (auto& fx : m_explosions) {
        fx.timer += dt;
        if (fx.timer >= fx.duration) fx.alive = false;
    }
    m_explosions.erase(
        std::remove_if(m_explosions.begin(), m_explosions.end(),
                        [](const ExplosionEffect& e) { return !e.alive; }),
        m_explosions.end());
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
                 || pickup.weapon.name == "Horn Blast" || pickup.weapon.name == "Jaw Snap"
                 || pickup.weapon.name == "Purse Swing" || pickup.weapon.name == "Fire Breath"
                 || pickup.weapon.name == "Milk Squirt");
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
                int ammo = (pickup.currentAmmo >= 0) ? pickup.currentAmmo : pickup.weapon.ammo;
                player->equipWeapon(pickup.weapon, ammo);
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

    // World bounds in meters (screen edges + margin)
    float worldHalfW = SCREEN_WIDTH / PPM / 2.0f + 2.0f;  // ~23m
    float worldTop   = SCREEN_HEIGHT / PPM / 2.0f + 2.0f;  // ~14m
    float worldBot   = rules.fallDeathY;                     // -20m

    for (auto& player : m_players) {
        if (!player->isAlive() || player->isWaitingToRespawn()) continue;
        b2Vec2 pos = player->getPosition();

        if (m_wrapAround) {
            // Vertical wrap: fell below bottom → appear at top
            if (pos.y < worldBot) {
                player->teleportTo(pos.x, worldTop);
            }
            // Horizontal wrap: off left/right → appear on opposite side
            if (pos.x < -worldHalfW) {
                player->teleportTo(worldHalfW - 1.0f, pos.y);
            } else if (pos.x > worldHalfW) {
                player->teleportTo(-worldHalfW + 1.0f, pos.y);
            }
        } else {
            // Normal mode: fall = death
            if (pos.y < worldBot) {
                player->takeDamage(9999.0f, 0.0f, 0.0f, "Fall", WeaponType::Melee);
                // Lives/respawn handled by checkPlayerDeaths()
            }
        }
    }
}

DeathAnimType Game::getDeathAnimType(const StickFigure& player) const {
    const std::string& da = player.getLastDamageDeathAnim();
    WeaponType wtype = player.getLastDamageWeaponType();

    // Use weapon's death_anim tag if set
    if (da == "dismember")    return DeathAnimType::Dismember;
    if (da == "disintegrate") return DeathAnimType::Disintegrate;
    if (da == "incinerate")   return DeathAnimType::Incinerate;
    if (da == "explode")      return DeathAnimType::Explode;
    if (da == "collapse")     return DeathAnimType::Collapse;

    // Fallback: explosive weapons explode
    if (wtype == WeaponType::Explosive)
        return DeathAnimType::Explode;

    // Default
    return DeathAnimType::Collapse;
}

void Game::checkPlayerDeaths() {
    const auto& rules = m_rulesEngine.getRules();
    const auto& spawns = m_arena.getSpawnPoints();

    // Ensure m_wasAlive is always sized to match players
    if (m_wasAlive.size() < m_players.size())
        m_wasAlive.resize(m_players.size(), true);

    for (size_t i = 0; i < m_players.size(); i++) {
        auto& player = m_players[i];
        bool aliveNow = player->isAlive();

        // Detect death transition: was alive last frame, dead now
        if (m_wasAlive[i] && !aliveNow && !player->isWaitingToRespawn()) {

            b2Vec2 pos = player->getPosition();

            // Don't spawn death animation for fall deaths (off screen)
            if (player->getLastDamageWeapon() != "Fall") {
                DeathAnimType animType = getDeathAnimType(*player);
                m_deathAnims.spawnDeath(m_physics, animType,
                    pos.x, pos.y, player->getColor(), player->getPlayerIndex(),
                    player->getCharacterType(),
                    player->getLastKnockbackX(), player->getLastKnockbackY());
            }

            // Drop all non-innate weapons as pickups
            auto dropped = player->dropAllNonInnate();
            for (size_t d = 0; d < dropped.size(); d++) {
                WeaponPickup wp;
                float scatter = (static_cast<float>(d) - static_cast<float>(dropped.size()) / 2.0f) * 0.8f;
                wp.position = {pos.x + scatter, pos.y + 0.5f};
                wp.weapon = dropped[d].weapon;
                wp.currentAmmo = dropped[d].ammo;
                wp.bobTimer = 0.0f;
                wp.alive = true;
                m_pickups.push_back(wp);
            }

            // Handle lives/respawn
            int lives = player->getLives() - 1;
            player->setLives(lives);
            if (lives > 0) {
                size_t idx = static_cast<size_t>(player->getPlayerIndex());
                player->startRespawnTimer(rules.respawnDelay, spawns[idx].x, spawns[idx].y);
            }
        }

        m_wasAlive[i] = aliveNow;
    }
}

void Game::checkRoundEnd() {
    int alive = 0; int last = -1;
    for (const auto& p : m_players) {
        if (p->isAlive() && p->getLives() > 0) { alive++; last = p->getPlayerIndex(); }
    }
    if (alive <= 1) {
        m_state = GameState::RoundOver;
        m_deathAnims.cleanupAll(m_physics);
        if (last >= 0) std::cout << "Player " << last << " wins!\n";
        else std::cout << "Draw!\n";
    }
}

void Game::render() {
    m_renderer.clear(sf::Color(25, 25, 30));
    m_arena.draw(m_renderer.getWindow());

    // Draw weapon pickups with unique icons
    for (const auto& pickup : m_pickups) {
        if (!pickup.alive) continue;
        float bob = std::sin(pickup.bobTimer * 3.0f) * 3.0f;
        sf::Vector2f sp = {SCREEN_CX + pickup.position.x * PPM,
                           SCREEN_CY - pickup.position.y * PPM + bob};

        drawWeaponIcon(m_renderer.getWindow(), pickup.weapon.name, sp, pickup.bobTimer);
    }

    for (const auto& p : m_players) p->draw(m_renderer.getWindow());

    // Draw weapon icon above each player holding a non-Fists weapon
    for (const auto& p : m_players) {
        if (!p->isAlive()) continue;
        const auto& wep = p->getCurrentWeapon();
        if (wep.name == "Fists") continue;
        b2Vec2 wpos = p->getPosition();
        sf::Vector2f sp = {SCREEN_CX + wpos.x * PPM,
                           SCREEN_CY - wpos.y * PPM - 45.0f};
        drawWeaponIcon(m_renderer.getWindow(), wep.name, sp, m_roundTimer, 0.6f);
    }

    // Draw death animations (gibs, blood, collapse effects)
    m_deathAnims.draw(m_renderer.getWindow());

    for (const auto& proj : m_projectiles) {
        if (!proj.alive) continue;
        b2Vec2 pos = b2Body_GetPosition(proj.bodyId);
        sf::Vector2f sp = {SCREEN_CX + pos.x * PPM, SCREEN_CY - pos.y * PPM};

        if (proj.weapon.destroysPlatforms) {
            // Nuke grenade: pulsing radioactive green with hazard symbol
            float pulse = std::sin(proj.lifetime * 8.0f) * 0.3f + 0.7f;
            sf::CircleShape c(6.0f);
            c.setOrigin({6.0f, 6.0f});
            c.setPosition(sp);
            c.setFillColor(sf::Color(static_cast<uint8_t>(50 * pulse),
                                      static_cast<uint8_t>(255 * pulse), 0, 230));
            c.setOutlineColor(sf::Color(255, 255, 0, 180));
            c.setOutlineThickness(1.5f);
            m_renderer.getWindow().draw(c);
            // Radiation ring
            sf::CircleShape ring(9.0f);
            ring.setOrigin({9.0f, 9.0f});
            ring.setPosition(sp);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(255, 255, 0, static_cast<uint8_t>(80 * pulse)));
            ring.setOutlineThickness(1.0f);
            m_renderer.getWindow().draw(ring);
        } else if (proj.isBurn) {
            float flicker = std::sin(proj.lifetime * 20.0f) * 0.3f + 0.7f;
            float sz = 3.0f * flicker;
            sf::CircleShape c(sz); c.setOrigin({sz, sz});
            c.setPosition(sp);
            c.setFillColor(sf::Color(255, static_cast<uint8_t>(120 * flicker), 0, 220));
            m_renderer.getWindow().draw(c);
            sf::CircleShape glow(sz + 2.0f);
            glow.setOrigin({sz + 2.0f, sz + 2.0f});
            glow.setPosition(sp);
            glow.setFillColor(sf::Color(255, 200, 0, 60));
            m_renderer.getWindow().draw(glow);
        } else if (proj.isPoison) {
            sf::CircleShape c(4.0f); c.setOrigin({4.0f, 4.0f});
            c.setPosition(sp);
            c.setFillColor(sf::Color(0, 220, 0));
            m_renderer.getWindow().draw(c);
        } else if (proj.weapon.name == "Milk Squirt") {
            // White milk droplets
            sf::CircleShape c(3.0f); c.setOrigin({3.0f, 3.0f});
            c.setPosition(sp);
            c.setFillColor(sf::Color(255, 255, 255, 220));
            m_renderer.getWindow().draw(c);
        } else {
            // Regular bullets / shotgun pellets
            float sz = (proj.weapon.pelletCount > 1) ? 2.0f : 4.0f;
            sf::CircleShape c(sz); c.setOrigin({sz, sz});
            c.setPosition(sp);
            if (proj.weapon.pelletCount > 1)
                c.setFillColor(sf::Color(255, 180, 80)); // orange pellets
            else
                c.setFillColor(sf::Color::Yellow);
            m_renderer.getWindow().draw(c);
        }
    }

    // Draw explosion effects
    for (const auto& fx : m_explosions) {
        if (!fx.alive) continue;
        float progress = fx.timer / fx.duration;
        sf::Vector2f ep = {SCREEN_CX + fx.x * PPM, SCREEN_CY - fx.y * PPM};
        float blastPx = fx.radius * PPM;

        if (fx.isNuke) {
            // === NUCLEAR EXPLOSION ===

            // Screen flash (white overlay fading out)
            if (progress < 0.3f) {
                float flashAlpha = (1.0f - progress / 0.3f) * 0.6f;
                sf::RectangleShape flash({SCREEN_WIDTH, SCREEN_HEIGHT});
                flash.setFillColor(sf::Color(255, 255, 255,
                    static_cast<uint8_t>(flashAlpha * 255)));
                m_renderer.getWindow().draw(flash);
            }

            // Expanding fireball (orange->red->dark)
            float fireR = blastPx * std::min(1.0f, progress * 3.0f);
            if (progress < 0.6f) {
                float fireAlpha = 1.0f - (progress / 0.6f);
                sf::CircleShape fireball(fireR);
                fireball.setOrigin({fireR, fireR});
                fireball.setPosition(ep);
                uint8_t r = static_cast<uint8_t>(255 - progress * 200);
                uint8_t g = static_cast<uint8_t>(std::max(0.0f, 150 - progress * 400));
                fireball.setFillColor(sf::Color(r, g, 0, static_cast<uint8_t>(fireAlpha * 180)));
                m_renderer.getWindow().draw(fireball);
            }

            // Mushroom cloud stem
            if (progress > 0.1f && progress < 0.9f) {
                float stemProgress = (progress - 0.1f) / 0.8f;
                float stemH = blastPx * 2.5f * stemProgress;
                float stemW = blastPx * 0.3f * (1.0f - stemProgress * 0.3f);
                uint8_t stemAlpha = static_cast<uint8_t>((1.0f - stemProgress) * 150);
                sf::RectangleShape stem({stemW, stemH});
                stem.setOrigin({stemW / 2.0f, stemH});
                stem.setPosition(ep);
                stem.setFillColor(sf::Color(120, 80, 40, stemAlpha));
                m_renderer.getWindow().draw(stem);

                // Mushroom cap
                float capR = blastPx * 0.8f * (0.5f + stemProgress * 0.5f);
                float capY = ep.y - stemH;
                sf::CircleShape cap(capR);
                cap.setScale({1.5f, 0.8f});
                cap.setOrigin({capR, capR});
                cap.setPosition({ep.x, capY});
                cap.setFillColor(sf::Color(100, 60, 30, stemAlpha));
                m_renderer.getWindow().draw(cap);

                // Cap highlight
                sf::CircleShape capTop(capR * 0.6f);
                capTop.setScale({1.5f, 0.7f});
                capTop.setOrigin({capR * 0.6f, capR * 0.6f});
                capTop.setPosition({ep.x, capY - capR * 0.15f});
                capTop.setFillColor(sf::Color(180, 100, 30, static_cast<uint8_t>(stemAlpha * 0.6f)));
                m_renderer.getWindow().draw(capTop);
            }

            // Shockwave ring
            if (progress < 0.5f) {
                float ringR = blastPx * 1.5f * (progress / 0.5f);
                float ringAlpha = 1.0f - (progress / 0.5f);
                sf::CircleShape ring(ringR);
                ring.setOrigin({ringR, ringR});
                ring.setPosition(ep);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineColor(sf::Color(255, 200, 100, static_cast<uint8_t>(ringAlpha * 200)));
                ring.setOutlineThickness(3.0f);
                m_renderer.getWindow().draw(ring);
            }

            // Debris particles
            float debrisPhase = progress * 20.0f;
            int debrisCount = static_cast<int>(12 * (1.0f - progress));
            for (int d = 0; d < debrisCount; d++) {
                float angle = static_cast<float>(d) * 0.52f + debrisPhase;
                float dist = blastPx * progress * (0.5f + std::sin(angle * 3.0f) * 0.5f);
                float dx = std::cos(angle) * dist;
                float dy = std::sin(angle) * dist - progress * blastPx * 0.5f; // arc upward
                float sz = 2.0f * (1.0f - progress);
                sf::CircleShape debris(sz);
                debris.setOrigin({sz, sz});
                debris.setPosition({ep.x + dx, ep.y + dy});
                debris.setFillColor(sf::Color(200, 100 + d * 10, 0,
                    static_cast<uint8_t>((1.0f - progress) * 200)));
                m_renderer.getWindow().draw(debris);
            }

        } else {
            // === REGULAR EXPLOSION ===
            float r = blastPx * progress;
            float alpha = 1.0f - progress;
            sf::CircleShape blast(r);
            blast.setOrigin({r, r});
            blast.setPosition(ep);
            blast.setFillColor(sf::Color(255, 150, 0, static_cast<uint8_t>(alpha * 150)));
            m_renderer.getWindow().draw(blast);

            sf::CircleShape core(r * 0.5f);
            core.setOrigin({r * 0.5f, r * 0.5f});
            core.setPosition(ep);
            core.setFillColor(sf::Color(255, 255, 200, static_cast<uint8_t>(alpha * 200)));
            m_renderer.getWindow().draw(core);
        }
    }

    m_hud.draw(m_renderer.getWindow(), m_players, m_roundTimer);

    if (m_state == GameState::RoundOver) {
        sf::RectangleShape overlay({SCREEN_WIDTH, SCREEN_HEIGHT});
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        m_renderer.getWindow().draw(overlay);
    }

    m_renderer.display();
}

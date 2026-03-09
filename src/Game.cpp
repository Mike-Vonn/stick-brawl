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
        case 4: return CharacterType::Crocodile;
        case 5: return CharacterType::StickLady;
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
            // F11 toggles fullscreen
            if (k->code == sf::Keyboard::Key::F11) {
                m_renderer.toggleFullscreen();
                return;
            }
            // Escape quits from character select
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
    m_camX = 0.0f; m_camY = 0.0f; m_camZoom = 1.0f;

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

        // Give innate weapons (set as default so they revert to it)
        if (ct == CharacterType::Cobra) {
            auto* poison = m_weaponFactory.getWeapon("Poison Spit");
            if (poison) { p->setDefaultWeapon(*poison); p->equipWeapon(*poison); }
        } else if (ct == CharacterType::Unicorn) {
            auto* horn = m_weaponFactory.getWeapon("Horn Blast");
            if (horn) { p->setDefaultWeapon(*horn); p->equipWeapon(*horn); }
        } else if (ct == CharacterType::Crocodile) {
            auto* jaw = m_weaponFactory.getWeapon("Jaw Snap");
            if (jaw) { p->setDefaultWeapon(*jaw); p->equipWeapon(*jaw); }
        } else if (ct == CharacterType::StickLady) {
            auto* purse = m_weaponFactory.getWeapon("Purse Swing");
            if (purse) { p->setDefaultWeapon(*purse); p->equipWeapon(*purse); }
        }
        // Default weapon is already WeaponData{} (Fists) if none of the above

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
            // F11 toggles fullscreen
            if (k->code == sf::Keyboard::Key::F11) {
                m_renderer.toggleFullscreen();
                return; // window recreated, bail out of event loop
            }
            // Escape returns to character select
            if (k->code == sf::Keyboard::Key::Escape) {
                // Clean up arena bodies
                m_arena.createLevel(m_physics, m_selectedLevel);
                for (auto& proj : m_projectiles) {
                    if (b2Body_IsValid(proj.bodyId)) b2DestroyBody(proj.bodyId);
                }
                m_projectiles.clear();
                m_pickups.clear();
                m_explosions.clear();
                m_blackHoles.clear();
                m_tornadoes.clear();
                m_timeSlowZones.clear();
                m_portals.clear();
                m_meteorShowers.clear();
                m_healZones.clear();
        m_lavaPools.clear();
        for (auto& lp : m_lavaParticles) b2DestroyBody(lp.bodyId);
        m_lavaParticles.clear();
        for (auto& g : m_grapples) { if (B2_IS_NON_NULL(g.hookProjectile) && b2Body_IsValid(g.hookProjectile)) b2DestroyBody(g.hookProjectile); if (B2_IS_NON_NULL(g.hookBody) && b2Body_IsValid(g.hookBody)) b2DestroyBody(g.hookBody); }
        m_grapples.clear();
                m_players.clear();
                m_state = GameState::CharSelect;
                for (auto& ps : m_selectState) { ps.ready = false; }
                return;
            }
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
    if (m_roundTimer <= 0.0f) {
        // Time's up — restart the round instead of stopping
        m_roundTimer = 0.0f;
        std::cout << "Time's up! Restarting round...\n";

        m_arena.createLevel(m_physics, m_selectedLevel);
        const auto& spawns = m_arena.getSpawnPoints();
        const auto& rules = m_rulesEngine.getRules();

        for (auto& proj : m_projectiles) {
            if (b2Body_IsValid(proj.bodyId)) b2DestroyBody(proj.bodyId);
        }
        m_projectiles.clear();
        m_pickups.clear();
        m_explosions.clear();
        m_blackHoles.clear();
        m_tornadoes.clear();
        m_timeSlowZones.clear();
        m_portals.clear();
        m_meteorShowers.clear();
        m_healZones.clear();
        m_lavaPools.clear();
        for (auto& lp : m_lavaParticles) b2DestroyBody(lp.bodyId);
        m_lavaParticles.clear();
        for (auto& g : m_grapples) { if (B2_IS_NON_NULL(g.hookProjectile) && b2Body_IsValid(g.hookProjectile)) b2DestroyBody(g.hookProjectile); if (B2_IS_NON_NULL(g.hookBody) && b2Body_IsValid(g.hookBody)) b2DestroyBody(g.hookBody); }
        m_grapples.clear();

        for (size_t i = 0; i < m_players.size(); i++) {
            size_t spawnIdx = i % spawns.size();
            m_players[i]->respawn(spawns[spawnIdx].x, spawns[spawnIdx].y);
            m_players[i]->setLives(rules.livesPerPlayer);
            m_players[i]->setMaxHealth(rules.maxHealth);
        }

        m_roundTimer = rules.roundTimeSeconds;
        m_weaponSpawnTimer = rules.weaponSpawnInterval;
        return;
    }

    handlePlayerInput(dt);
    for (auto& p : m_players) p->update(dt);
    // Build slow zone and explosion lists for arena platform physics
    std::vector<ArenaSlowZone> arenaSlowZones;
    for (const auto& z : m_timeSlowZones) {
        if (z.alive) arenaSlowZones.push_back({z.x, z.y, z.radius, z.slowFactor});
    }
    std::vector<ArenaExplosion> arenaExplosions;
    for (const auto& fx : m_explosions) {
        if (fx.alive && fx.timer < 0.1f)  // only apply explosion force on first few frames
            arenaExplosions.push_back({fx.x, fx.y, fx.radius, fx.isNuke ? 80.0f : 30.0f});
    }
    m_arena.update(dt, m_players, arenaSlowZones, arenaExplosions);
    m_physics.step(dt);
    updateProjectiles(dt);
    updateWeaponPickups(dt);
    checkFallDeath();
    checkCombatDeaths();
    updateWeaponSpawns(dt);
    updateBlackHoles(dt);
    updateTornadoes(dt);
    updateTimeSlowZones(dt);
    updatePortals(dt);
    updateMeteorShowers(dt);
    updateHealZones(dt);
    updateLavaPools(dt);
    updateGrapples(dt);
    updateCamera(dt);
    checkRoundEnd();
}

void Game::handlePlayerInput(float dt) {
    for (size_t i = 0; i < m_players.size(); i++) {
        auto& player = m_players[i];
        if (!player->isAlive()) continue;

        // If mind-controlled, the controller's input drives this player
        PlayerInput pi;
        if (player->isMindControlled()) {
            pi = m_input.getPlayerInput(player->getMindController());
        } else {
            pi = m_input.getPlayerInput(static_cast<int>(i));
        }

        // Check if this player is actively grappling (attached, not flying)
        GrappleState* activeGrapple = nullptr;
        for (auto& g : m_grapples) {
            if (g.playerIndex == static_cast<int>(i) && g.active && !g.hookFlying) {
                activeGrapple = &g;
                break;
            }
        }

        if (activeGrapple) {
            // Grapple controls: Jump=retract, Down=extend, L/R=swing pump
            float retractSpeed = activeGrapple->weapon.grappleRetractSpeed;
            float extendSpeed = activeGrapple->weapon.grappleExtendSpeed;
            if (pi.jump)
                activeGrapple->ropeLength = std::max(activeGrapple->minLength,
                    activeGrapple->ropeLength - retractSpeed * dt);
            if (pi.moveDown)
                activeGrapple->ropeLength = std::min(activeGrapple->maxLength,
                    activeGrapple->ropeLength + extendSpeed * dt);

            // Pendulum swing: force applied tangent to rope arc
            float swingForce = activeGrapple->weapon.grappleSwingForce;
            b2Vec2 hookPt = activeGrapple->hookPoint;
            if (activeGrapple->grappledPlayerIndex >= 0) {
                for (auto& other : m_players) {
                    if (other->getPlayerIndex() == activeGrapple->grappledPlayerIndex && other->isAlive())
                        hookPt = other->getPosition();
                }
            }
            b2Vec2 ppos2 = b2Body_GetPosition(player->getTorsoBodyId());
            float rdx = ppos2.x - hookPt.x, rdy = ppos2.y - hookPt.y;
            float rlen = std::sqrt(rdx*rdx + rdy*rdy);
            if (rlen > 0.1f) {
                // Tangent vector: perpendicular to rope
                // rdy/rlen points "right" when hanging below, -rdy/rlen points "left"
                float tx = rdy / rlen, ty = -rdx / rlen;
                if (pi.moveLeft) {
                    b2Body_ApplyForceToCenter(player->getTorsoBodyId(),
                        {-tx * swingForce, -ty * swingForce}, true);
                }
                if (pi.moveRight) {
                    b2Body_ApplyForceToCenter(player->getTorsoBodyId(),
                        {tx * swingForce, ty * swingForce}, true);
                }
            }
            // Speed cap at rope end
            b2Vec2 pvel = b2Body_GetLinearVelocity(player->getTorsoBodyId());
            float speed = std::sqrt(pvel.x*pvel.x + pvel.y*pvel.y);
            float maxSpeed = 25.0f;
            if (speed > maxSpeed) {
                float scale = maxSpeed / speed;
                b2Body_SetLinearVelocity(player->getTorsoBodyId(), {pvel.x*scale, pvel.y*scale});
            }

            // DO NOT call stopMoving — it damps horizontal velocity and kills swings
            // DO NOT call normal movement either — grapple overrides everything

            // Zero out friction while grappling so surfaces don't slow swing
            b2ShapeId shapes[8];
            int count = b2Body_GetShapes(player->getTorsoBodyId(), shapes, 8);
            for (int s = 0; s < count; s++) {
                b2Shape_SetFriction(shapes[s], 0.0f);
            }
        } else {
            // Normal movement
            if (pi.moveLeft) player->moveLeft();
            else if (pi.moveRight) player->moveRight();
            else player->stopMoving();

            if (pi.jumpPressed) player->jump();
        }

        // Aiming (always available)
        if (pi.aimUp) player->aimUp();
        else if (pi.aimDown) player->aimDown();
        else player->resetAim();

        // Attack: use held for continuous-fire weapons (lava thrower), pressed for others
        bool shouldFire = false;
        const auto& weapon = player->getCurrentWeapon();
        if (weapon.isLavaThrower) {
            shouldFire = pi.attack && player->canAttack(); // continuous fire
        } else {
            shouldFire = pi.attackPressed && player->canAttack();
        }

        if (shouldFire) {
            player->attack();
            if (weapon.type == WeaponType::Melee)
                handleMeleeAttack(*player);
            else
                spawnProjectile(*player);
            // Apply weapon recoil (kickback)
            {
                const auto& w = player->getCurrentWeapon();
                if (w.type != WeaponType::Melee) {
                    float dir = static_cast<float>(player->getFacingDirection());
                    float aim = player->getAimAngle();
                    // Recoil proportional to weapon damage/projectile speed
                    float recoilStr = std::min(w.damage * 0.08f + w.projectileSpeed * 0.02f, 5.0f);
                    float rx = -dir * std::cos(aim) * recoilStr;
                    float ry = -std::sin(aim) * recoilStr;
                    player->applyRecoil(rx, ry);
                }
            }
            if (player->getAmmo() == 0 && player->getCurrentWeapon().ammo >= 0) {
                player->revertToDefaultWeapon();
            }
        }

        // Throw weapon: creates a projectile that does big damage, then revert to default
        if (pi.throwPressed && player->hasPickedUpWeapon()) {
            throwWeapon(*player);
        }
    }
}

void Game::handleMeleeAttack(StickFigure& attacker) {
    const auto& weapon = attacker.getCurrentWeapon();
    const auto& rules  = m_rulesEngine.getRules();
    b2Vec2 ap  = attacker.getPosition();
    float  dir = static_cast<float>(attacker.getFacingDirection());

    // --- Energy Shield: toggle defensive buff ---
    if (weapon.blocksProjectiles || weapon.damageReduction > 0.0f) {
        attacker.applyShield(weapon.damageReduction, weapon.attackRate * 2.0f + 0.1f,
                             weapon.reflectsProjectiles);
        return; // shield doesn't hit enemies directly
    }

    // --- Gravity Hammer: shockwave ---
    if (weapon.shockwaveRadius > 0.0f) {
        spawnExplosion(ap.x, ap.y, weapon.shockwaveRadius);
        for (auto& target : m_players) {
            if (target->getPlayerIndex() == attacker.getPlayerIndex()) continue;
            if (!target->isAlive()) continue;
            b2Vec2 tp = target->getPosition();
            float dx = tp.x - ap.x, dy = tp.y - ap.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < weapon.shockwaveRadius) {
                float f = 1.0f - dist / weapon.shockwaveRadius;
                float kbX = (dist > 0.01f ? dx/dist : dir) * weapon.shockwaveForce * f * rules.knockbackMultiplier;
                float kbY = weapon.shockwaveForce * 0.4f * f * rules.knockbackMultiplier;
                target->takeDamage(weapon.shockwaveDamage * f * rules.damageMultiplier, kbX, kbY);
            }
        }
        // Also basic hit in range
    }

    // --- Rocket Fist: dash forward ---
    if (weapon.dashDistance > 0.0f) {
        b2Vec2 dashImpulse = {dir * 22.0f, 4.0f};
        b2Body_ApplyLinearImpulseToCenter(attacker.getTorsoBodyId(), dashImpulse, true);
    }

    // --- Teleport Dagger: dash teleport on attack ---
    if (weapon.teleportDistance > 0.0f && attacker.getTeleportCD() <= 0.0f) {
        float tx = ap.x + dir * weapon.teleportDistance;
        attacker.teleportTo(tx, ap.y);
        float cd = weapon.teleportCooldown > 0.0f ? weapon.teleportCooldown : weapon.attackRate * 3.0f;
        attacker.setTeleportCD(cd);
    }

    // Standard melee hit (per-body-part)
    for (auto& target : m_players) {
        if (target->getPlayerIndex() == attacker.getPlayerIndex()) continue;
        if (!target->isAlive()) continue;

        // Check if melee swing hits any body part
        // The melee hit point is in front of the attacker
        float hitX = ap.x + dir * weapon.range * 0.5f;
        float hitY = ap.y;
        HitResult hr = target->checkHit(hitX, hitY, weapon.range);

        // Also check from attacker's position for close-range
        if (!hr.hit) {
            hr = target->checkHit(ap.x, ap.y, weapon.range);
        }

        if (!hr.hit) continue;

        // Verify facing direction (unless very close)
        b2Vec2 tp = target->getPosition();
        float dx = tp.x - ap.x;
        float dist = std::sqrt(dx*dx + (tp.y-ap.y)*(tp.y-ap.y));
        bool facing  = dx * dir >= -0.3f;
        bool close   = dist < weapon.range * 0.5f;
        if (!(facing || close)) continue;

        float dmg  = weapon.damage * rules.damageMultiplier;
        float kbX  = weapon.knockbackForce * dir * rules.knockbackMultiplier;
        float kbY  = weapon.knockbackForce * 0.5f * rules.knockbackMultiplier;

        // Chain Whip: pull enemy toward attacker instead
        if (weapon.canPullEnemies && weapon.pullForce > 0.0f) {
            float pullDir = (dist > 0.01f) ? -(dx/dist) : -dir;
            b2Body_ApplyLinearImpulseToCenter(target->getTorsoBodyId(),
                {pullDir * weapon.pullForce, weapon.pullForce * 0.3f}, true);
        }

        // Swap Staff (melee version): swap positions on hit
        if (weapon.swapsPositions) {
            b2Vec2 myPos = ap;
            attacker.teleportTo(tp.x, tp.y);
            target->teleportTo(myPos.x, myPos.y);
        }

        // Apply damage with body-part multiplier
        target->takeDamageAt(hr.part, dmg, kbX, kbY);

        // Rocket fist dash damage
        if (weapon.dashDamage > 0.0f)
            target->takeDamageAt(hr.part, weapon.dashDamage * rules.damageMultiplier, kbX * 1.5f, kbY * 1.5f);
    }

    // Terrain carve
    float envR = weapon.envDamageRadius;
    if (envR <= 0.0f) envR = weapon.damage * 0.015f;
    m_arena.carveCircle(m_physics, ap.x + dir * weapon.range * 0.6f, ap.y, envR);
}

// Helper: spawn visual explosion
void Game::spawnExplosion(float x, float y, float radius, bool isNuke, GrenadeCharge charge) {
    ExplosionEffect fx;
    fx.x = x; fx.y = y; fx.radius = radius; fx.timer = 0.0f;
    fx.isNuke = isNuke; fx.charge = charge;
    fx.duration = isNuke ? 2.5f : (charge == GrenadeCharge::Lava ? 1.2f : 
                  charge == GrenadeCharge::Freeze ? 1.0f :
                  charge == GrenadeCharge::Time ? 1.5f :
                  charge == GrenadeCharge::Frag ? 0.6f :
                  charge == GrenadeCharge::Implode ? 1.0f : 0.8f);
    fx.alive = true;
    m_explosions.push_back(fx);
}

// Helper: push nearby projectiles (mines, stickies, etc.) from explosion
void Game::pushNearbyProjectiles(float x, float y, float radius, float force, int excludeIndex) {
    for (auto& proj : m_projectiles) {
        if (!proj.alive) continue;
        // Skip the grenade that caused this explosion (by body position match won't work, use index)
        b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
        float dx = pp.x - x, dy = pp.y - y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist >= radius || dist < 0.01f) continue;

        // Only push physical grenades (stuck stickies and mines, not stuck-to-player)
        if (proj.stuckToPlayer) continue;

        float f = 1.0f - dist / radius;
        float dirX = dx / dist;
        float dirY = dy / dist;
        // Apply impulse
        b2Vec2 impulse = {dirX * force * f, dirY * force * f + force * 0.3f * f};
        b2Body_ApplyLinearImpulseToCenter(proj.bodyId, impulse, true);

        // Cap velocity to prevent tiny objects going insanely fast
        b2Vec2 vel = b2Body_GetLinearVelocity(proj.bodyId);
        float spd = std::sqrt(vel.x*vel.x + vel.y*vel.y);
        constexpr float MAX_PROJ_SPEED = 40.0f;
        if (spd > MAX_PROJ_SPEED) {
            float scale = MAX_PROJ_SPEED / spd;
            b2Body_SetLinearVelocity(proj.bodyId, {vel.x * scale, vel.y * scale});
        }

        // If it was a stuck sticky, unstick it from surface (it's now airborne)
        if (proj.isStuck && !proj.stuckToPlayer) {
            proj.isStuck = false;
            b2Body_SetLinearDamping(proj.bodyId, 0.1f); // restore normal damping in air
            b2Body_SetGravityScale(proj.bodyId, 1.0f);
        }
        // Mine stays armed when blasted — just restore physics so it flies
        if (proj.isMine) {
            b2Body_SetLinearDamping(proj.bodyId, 0.1f);
            b2Body_SetGravityScale(proj.bodyId, 1.0f);
        }
    }
    // Also push lava particles
    for (auto& lp : m_lavaParticles) {
        if (!lp.alive) continue;
        b2Vec2 pp = b2Body_GetPosition(lp.bodyId);
        float dx = pp.x - x, dy = pp.y - y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist >= radius || dist < 0.01f) continue;
        float f = 1.0f - dist / radius;
        float dirX = dx / dist, dirY = dy / dist;
        b2Body_ApplyLinearImpulseToCenter(lp.bodyId, {dirX * force * f * 0.3f, dirY * force * f * 0.3f + force * 0.2f * f}, true);
    }
}
StickFigure* Game::findNearestEnemy(float x, float y, int excludeIdx, float maxRange) {
    StickFigure* best = nullptr;
    float bestDist = maxRange;
    for (auto& p : m_players) {
        if (p->getPlayerIndex() == excludeIdx || !p->isAlive()) continue;
        b2Vec2 pos = p->getPosition();
        float d = std::sqrt((pos.x-x)*(pos.x-x) + (pos.y-y)*(pos.y-y));
        if (d < bestDist) { bestDist = d; best = p.get(); }
    }
    return best;
}

// Helper: apply all on-hit weapon effects to a target
void Game::applyProjectileHitEffects(StickFigure& target, const WeaponData& w,
                                     int ownerIdx, float dx, float dy) {
    const auto& rules = m_rulesEngine.getRules();
    float dist = std::sqrt(dx*dx + dy*dy);
    float kbDir = (dx >= 0) ? 1.0f : -1.0f;
    float kbX = w.knockbackForce * kbDir * rules.knockbackMultiplier;
    float kbY = w.knockbackForce * 0.4f  * rules.knockbackMultiplier;

    if (w.damage != 0.0f)
        target.takeDamage(w.damage * rules.damageMultiplier, kbX, kbY);

    if (w.poisonDps > 0.0f)
        target.applyPoison(w.poisonDps, w.poisonDuration);

    if (w.slowEffect > 0.0f)
        target.applySlow(1.0f - w.slowEffect, w.slowDuration);

    if (w.freezeBuildup > 0.0f)
        target.addFreezeStacks(1);

    if (w.stunDuration > 0.0f)
        target.applyStun(w.stunDuration);

    if (w.shrinkScale < 1.0f)
        target.applyShrink(w.shrinkScale, w.speedIncrease, w.shrinkDuration);

    // Mind control (swap staff fires as projectile; mind control staff)
    if (w.mindControlDuration > 0.0f)
        target.applyMindControl(ownerIdx, w.mindControlDuration);

    // Swap positions
    if (w.swapsPositions) {
        // Find owner and swap
        for (auto& p : m_players) {
            if (p->getPlayerIndex() == ownerIdx) {
                b2Vec2 myPos = p->getPosition();
                b2Vec2 theirPos = target.getPosition();
                p->teleportTo(theirPos.x, theirPos.y);
                target.teleportTo(myPos.x, myPos.y);
                break;
            }
        }
    }

    // Healing pulse: negative damage = heal
    if (w.damage < 0.0f)
        target.heal(-w.damage * rules.damageMultiplier);

    // Healing zone on impact
    if (w.healingRadius > 0.0f && w.healOverTime > 0.0f) {
        b2Vec2 pos = target.getPosition();
        HealZone hz;
        hz.x = pos.x; hz.y = pos.y;
        hz.radius   = w.healingRadius;
        hz.healRate = w.healOverTime;
        hz.timer    = 0.0f;
        hz.duration = w.healDuration > 0 ? w.healDuration : 3.0f;
        hz.alive    = true;
        m_healZones.push_back(hz);
    }

    // Chain lightning
    if (w.chainTargets > 0 && w.chainRange > 0.0f) {
        b2Vec2 curPos = target.getPosition();
        float chainDmg = w.damage * w.chainDamageFalloff;
        StickFigure* prev = &target;
        for (int c = 0; c < w.chainTargets; c++) {
            StickFigure* next = findNearestEnemy(curPos.x, curPos.y,
                                                 prev->getPlayerIndex(), w.chainRange);
            if (!next) break;
            next->takeDamage(chainDmg * rules.damageMultiplier,
                             kbX * 0.5f, kbY * 0.5f);
            if (w.stunDuration > 0.0f) next->applyStun(w.stunDuration * 0.5f);
            curPos   = next->getPosition();
            chainDmg *= w.chainDamageFalloff;
            prev     = next;
        }
    }
}

void Game::spawnProjectile(StickFigure& shooter) {
    const auto& weapon = shooter.getCurrentWeapon();
    if (weapon.type == WeaponType::Melee) return;

    b2Vec2 pos  = shooter.getPosition();
    float  dir  = static_cast<float>(shooter.getFacingDirection());
    float  aim  = shooter.getAimAngle();
    float  cosA = std::cos(aim);
    float  sinA = std::sin(aim);

    // --- Black Hole ---
    if (weapon.blackHoleDuration > 0.0f) {
        // Cap spawn distance so the black hole appears on-screen
        // (weapon.range can be very large, e.g. 40m which is way off-screen)
        float bhRange = std::min(weapon.range, 12.0f);
        float tx = pos.x + dir * bhRange * cosA;
        float ty = pos.y + bhRange * sinA;
        BlackHoleEffect bh;
        bh.x = tx; bh.y = ty;
        bh.radius    = weapon.blackHoleRadius > 0 ? weapon.blackHoleRadius : 8.0f;
        bh.pullForce = weapon.blackHolePullForce;
        bh.dps       = weapon.damagePerSecond;
        bh.timer     = 0.0f;
        bh.duration  = weapon.blackHoleDuration;
        bh.alive     = true;
        m_blackHoles.push_back(bh);
        return;
    }

    // --- Tornado ---
    if (weapon.tornadoLiftForce > 0.0f) {
        TornadoEffect t;
        float tornadoSpeed = 6.0f;
        t.x = pos.x + dir * 0.5f; t.y = pos.y + 0.3f;
        t.vx = dir * tornadoSpeed * cosA; t.vy = tornadoSpeed * sinA;
        t.radius    = weapon.tornadoRadius > 0 ? weapon.tornadoRadius : 4.0f;
        t.liftForce = weapon.tornadoLiftForce;
        t.spinForce = weapon.tornadoSpinForce;
        t.dps       = weapon.damagePerSecond;
        t.timer     = 0.0f;
        t.duration  = weapon.projectileLifetime > 0 ? weapon.projectileLifetime : 6.0f;
        t.alive     = true;
        t.shooterIndex = shooter.getPlayerIndex();
        m_tornadoes.push_back(t);
        return;
    }

    // --- Time Slow Zone ---
    // Time Gun fires as a standard projectile; zone created on impact in updateProjectiles

    // --- Meteor Shower ---
    if (weapon.meteorCount > 0) {
        // Cap targeting distance to keep the meteor shower on-screen
        // (weapon.range can be very large for meteor staff, e.g. 50m which is way off-screen)
        float meteorRange = std::min(weapon.range, 15.0f);
        float tx = pos.x + dir * meteorRange * cosA;
        float ty = pos.y + meteorRange * sinA;
        MeteorShower ms;
        ms.centerX       = tx; ms.centerY = ty;
        ms.spreadRadius  = weapon.meteorSpreadRadius > 0 ? weapon.meteorSpreadRadius : 8.0f;
        ms.meteorsLeft   = weapon.meteorCount;
        ms.meteorTimer   = 0.0f;
        ms.meteorDelay   = weapon.meteorDelay > 0 ? weapon.meteorDelay : 0.3f;
        ms.damage        = weapon.damage;
        ms.explosionRadius = weapon.explosionRadius > 0 ? weapon.explosionRadius : 2.0f;
        ms.knockback     = weapon.knockbackForce;
        ms.ownerIndex    = shooter.getPlayerIndex();
        ms.alive         = true;
        m_meteorShowers.push_back(ms);
        return;
    }

    // --- Portal (fires as a projectile that becomes a portal on contact) ---
    if (weapon.portalDuration > 0.0f) {
        float speed = weapon.projectileSpeed > 0 ? weapon.projectileSpeed : 25.0f;
        float vx = speed * dir * cosA;
        float vy = speed * sinA;

        b2BodyId bullet = m_physics.createDynamicCircle(
            pos.x + dir * 0.5f, pos.y + 0.3f, 0.15f, 0.05f,
            CAT_PROJECTILE, CAT_PLATFORM | CAT_PLAYER);
        b2Body_SetBullet(bullet, true);
        b2Body_SetLinearVelocity(bullet, {vx, vy});
        b2Body_SetGravityScale(bullet, 0.0f);

        Projectile proj;
        proj.bodyId = bullet;
        proj.weapon = weapon;
        proj.ownerIndex = shooter.getPlayerIndex();
        proj.lifetime = weapon.projectileLifetime > 0 ? weapon.projectileLifetime : 1.5f;
        proj.alive = true;
        proj.createsPortal = true;
        proj.portalDuration = weapon.portalDuration;
        m_projectiles.push_back(proj);
        return;
    }

    // --- Grappling Hook: fires hook projectile ---
    if (weapon.isGrapple) {
        // If already grappling, attack behavior depends on what's grappled
        for (auto& g : m_grapples) {
            if (g.playerIndex == shooter.getPlayerIndex() && g.active && !g.hookFlying) {
                if (g.grappledPlayerIndex >= 0) {
                    // Grappled a player — attack pulls them toward you
                    b2Vec2 ppos = shooter.getPosition();
                    for (auto& other : m_players) {
                        if (other->getPlayerIndex() == g.grappledPlayerIndex && other->isAlive()) {
                            b2Vec2 opos = other->getPosition();
                            float dx2 = ppos.x - opos.x, dy2 = ppos.y - opos.y;
                            float dist2 = std::sqrt(dx2*dx2 + dy2*dy2);
                            if (dist2 > 0.1f) {
                                float pullF = g.weapon.grappleSlingshotForce * 0.7f;
                                b2Body_ApplyLinearImpulseToCenter(other->getTorsoBodyId(),
                                    {dx2/dist2 * pullF, dy2/dist2 * pullF}, true);
                            }
                        }
                    }
                    releaseGrapple(shooter.getPlayerIndex());
                } else {
                    // Grappled a surface — release
                    releaseGrapple(shooter.getPlayerIndex());
                }
                return;
            }
        }
        fireGrapple(shooter);
        return;
    }

    // --- Lava Thrower: shoots lava particles directly ---
    if (weapon.isLavaThrower) {
        static std::mt19937 rng(std::random_device{}());
        float spreadRad = weapon.spreadDegrees * 3.14159f / 180.0f;
        std::uniform_real_distribution<float> spreadDist(-spreadRad / 2.0f, spreadRad / 2.0f);
        float angle = aim + spreadDist(rng);
        float speed = weapon.projectileSpeed > 0 ? weapon.projectileSpeed : 18.0f;
        std::uniform_real_distribution<float> speedJitter(0.8f, 1.2f);
        speed *= speedJitter(rng);
        float vx = speed * dir * std::cos(angle);
        float vy = speed * std::sin(angle);

        b2BodyId body = m_physics.createDynamicCircle(
            pos.x + dir * 0.6f, pos.y + 0.3f, 0.12f, 0.02f,
            CAT_PROJECTILE, CAT_PLATFORM | CAT_PLAYER);
        b2Body_SetLinearVelocity(body, {vx, vy});
        b2Body_SetGravityScale(body, 0.6f);

        LavaParticle lp;
        lp.bodyId = body;
        lp.timer = 0.0f;
        lp.lifetime = weapon.projectileLifetime > 0 ? weapon.projectileLifetime * 10.0f : 8.0f;
        lp.dps = weapon.damage > 0 ? weapon.damage * 3.0f : 20.0f;
        lp.alive = true;
        m_lavaParticles.push_back(lp);
        return;
    }

    // --- Standard projectile (including boomerang, bouncy ball, sticky gun, freeze, shrink, etc.) ---
    static std::mt19937 rng(std::random_device{}());
    float spreadRad = weapon.spreadDegrees * 3.14159f / 180.0f;
    int pellets = std::max(1, weapon.pelletCount);

    for (int p = 0; p < pellets; p++) {
        float a = aim;
        if (pellets > 1) {
            float even = -spreadRad + 2.0f * spreadRad * (static_cast<float>(p) / static_cast<float>(pellets - 1));
            std::uniform_real_distribution<float> jitter(-spreadRad*0.15f, spreadRad*0.15f);
            a += even + jitter(rng);
        } else if (spreadRad > 0.0f) {
            std::uniform_real_distribution<float> sd(-spreadRad, spreadRad);
            a += sd(rng);
        }

        float speed = weapon.projectileSpeed;
        if (pellets > 1) {
            std::uniform_real_distribution<float> sv(0.85f, 1.15f);
            speed *= sv(rng);
        }

        float vx = speed * dir * std::cos(a);
        float vy = speed * std::sin(a);
        float radius = (pellets > 1) ? 0.08f : 0.15f;
        float mass   = (pellets > 1) ? 0.05f : 0.1f;

        // All projectiles collide with platforms and players
        uint64_t projMask = CAT_PLAYER | CAT_PLATFORM;

        // Bounce casing gets high restitution for elastic bouncing
        // Bouncy weapons get their configured retention
        // Everything else gets 0 restitution (no bouncing off walls)
        float restitution = 0.0f;
        if (weapon.grenadeCasing == GrenadeCasing::Bounce) restitution = 0.95f;
        else if (weapon.bounces > 0) restitution = weapon.bounceEnergyRetention;

        b2BodyId bullet = m_physics.createDynamicCircle(
            pos.x + dir * 0.5f, pos.y + 0.3f, radius, mass,
            CAT_PROJECTILE, projMask, restitution);

        b2Body_SetBullet(bullet, true);
        b2Body_SetLinearVelocity(bullet, {vx, vy});
        if (!weapon.affectedByGravity)
            b2Body_SetGravityScale(bullet, 0.0f);

        Projectile proj;
        proj.bodyId       = bullet;
        proj.weapon       = weapon;
        proj.ownerIndex   = shooter.getPlayerIndex();
        proj.lifetime     = weapon.projectileLifetime;
        proj.alive        = true;
        proj.isPoison     = (weapon.poisonDps > 0.0f);
        proj.poisonDps    = weapon.poisonDps;
        proj.poisonDuration = weapon.poisonDuration;
        proj.returnsToSender  = weapon.returnsToSender;
        proj.hitsMultipleTimes = weapon.hitsMultipleTimes;
        proj.bouncesLeft  = weapon.bounces;

        m_projectiles.push_back(proj);
    }
}

void Game::throwWeapon(StickFigure& thrower) {
    // If grappling, throw = slingshot
    if (thrower.getCurrentWeapon().isGrapple) {
        for (auto& g : m_grapples) {
            if (g.playerIndex == thrower.getPlayerIndex() && g.active) {
                slingshotGrapple(thrower.getPlayerIndex());
                return;
            }
        }
        // Not grappling — fall through to normal throw
    }

    b2Vec2 pos = thrower.getPosition();
    float dir = static_cast<float>(thrower.getFacingDirection());
    float aim = thrower.getAimAngle();
    const auto& rules = m_rulesEngine.getRules();

    // Store the original weapon before reverting
    WeaponData originalWeapon = thrower.getCurrentWeapon();

    // Create a thrown weapon projectile
    float speed = 20.0f;
    float vx = speed * dir * std::cos(aim);
    float vy = speed * std::sin(aim);

    b2BodyId bullet = m_physics.createDynamicCircle(
        pos.x + dir * 0.5f, pos.y + 0.3f, 0.25f, 0.4f,
        CAT_PROJECTILE, CAT_PLATFORM | CAT_PLAYER);

    b2Body_SetBullet(bullet, true);
    b2Body_SetLinearVelocity(bullet, {vx, vy});
    b2Body_SetGravityScale(bullet, 0.8f);  // arc like a thrown object

    // Thrown weapon does 1/3 to 1/2 of max health as damage
    WeaponData throwData;
    throwData.name = "Thrown " + originalWeapon.name;
    throwData.type = WeaponType::Projectile;
    throwData.damage = rules.maxHealth * 0.4f;  // 40% of max HP
    throwData.knockbackForce = 12.0f;
    throwData.projectileSpeed = speed;
    throwData.projectileLifetime = 4.0f;
    throwData.affectedByGravity = true;

    Projectile proj;
    proj.bodyId = bullet;
    proj.weapon = throwData;
    proj.ownerIndex = thrower.getPlayerIndex();
    proj.lifetime = 4.0f;
    proj.alive = true;
    proj.isThrownWeapon = true;
    // Reset ammo to full for the pickup
    originalWeapon.ammo = std::max(originalWeapon.ammo, 1);
    proj.thrownWeaponData = originalWeapon;
    m_projectiles.push_back(proj);

    // Revert thrower to default weapon
    thrower.revertToDefaultWeapon();
    std::cout << "Player " << thrower.getPlayerIndex() << " threw their weapon!\n";
}

void Game::updateProjectiles(float dt) {
    const auto& rules = m_rulesEngine.getRules();

    // Deferred spawns to avoid modifying containers during iteration
    struct DeferredFrag { float x, y; int ownerIndex; WeaponData weapon; };
    std::vector<DeferredFrag> deferredFrags;
    struct DeferredLava { float x, y, r, dps, dur; int owner; };
    std::vector<DeferredLava> deferredLavas;

    for (auto& proj : m_projectiles) {
        if (!proj.alive) continue;

        // === IMPLODE CHARGE: pulling phase ===
        if (proj.isImploding) {
            proj.implodeTimer += dt;
            b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
            float pullR = proj.weapon.implodePullRadius > 0 ? proj.weapon.implodePullRadius : 12.0f;
            float pullDur = proj.weapon.implodePullDuration > 0 ? proj.weapon.implodePullDuration : 1.0f;
            float progress = proj.implodeTimer / pullDur; // 0→1

            // Pull players toward center with increasing force
            for (auto& player : m_players) {
                if (!player->isAlive()) continue;
                b2Vec2 pl = player->getPosition();
                float dx = pp.x - pl.x, dy = pp.y - pl.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist < pullR && dist > 0.3f) {
                    float pullStrength = (1.0f - dist / pullR) * (1.0f + progress * 5.0f) * 300.0f * dt;
                    float dirX = dx / dist, dirY = dy / dist;
                    b2Body_ApplyLinearImpulseToCenter(player->getTorsoBodyId(),
                        {dirX * pullStrength, dirY * pullStrength}, true);
                }
            }

            if (proj.implodeTimer >= pullDur) {
                // Time's up — explode!
                proj.isImploding = false;
                m_arena.carveCircle(m_physics, pp.x, pp.y, proj.weapon.explosionRadius * 0.5f, 0.4f);
                spawnExplosion(pp.x, pp.y, proj.weapon.explosionRadius, false, proj.weapon.grenadeCharge);
                pushNearbyProjectiles(pp.x, pp.y, proj.weapon.explosionRadius, proj.weapon.knockbackForce * 0.5f);
                for (auto& player : m_players) {
                    if (!player->isAlive()) continue;
                    b2Vec2 pl = player->getPosition();
                    float ddx = pl.x - pp.x, ddy = pl.y - pp.y;
                    float dist = std::sqrt(ddx*ddx + ddy*ddy);
                    if (dist < proj.weapon.explosionRadius) {
                        float f = std::max(0.3f, 1.0f - dist / proj.weapon.explosionRadius);
                        float dmg = proj.weapon.damage * f * rules.damageMultiplier;
                        float kbDir = ddx >= 0 ? 1.f : -1.f;
                        player->takeDamage(dmg,
                            kbDir * proj.weapon.knockbackForce * f * rules.knockbackMultiplier,
                            proj.weapon.knockbackForce * 0.3f * f * rules.knockbackMultiplier);
                    }
                }
                proj.alive = false;
            }
            continue;
        }

        // === GRENADE CASING: stuck/armed state handling ===
        if (proj.isStuck || proj.isMine) {
            // If stuck to a player, track their position (frozen to player)
            if (proj.stuckToPlayer && proj.stuckPlayerIndex >= 0) {
                for (auto& player : m_players) {
                    if (player->getPlayerIndex() == proj.stuckPlayerIndex && player->isAlive()) {
                        b2Vec2 ppos = player->getPosition();
                        b2Body_SetTransform(proj.bodyId, {ppos.x + proj.stuckOffset.x, ppos.y + proj.stuckOffset.y}, b2Body_GetRotation(proj.bodyId));
                        break;
                    }
                }
            }
            // If stuck to a platform, track its position
            else if (proj.stuckToPlatform && B2_IS_NON_NULL(proj.stuckPlatformBody)) {
                if (b2Body_IsValid(proj.stuckPlatformBody)) {
                    b2Vec2 platPos = b2Body_GetPosition(proj.stuckPlatformBody);
                    b2Body_SetTransform(proj.bodyId,
                        {platPos.x + proj.stuckPlatformOffset.x, platPos.y + proj.stuckPlatformOffset.y},
                        b2Body_GetRotation(proj.bodyId));
                } else {
                    // Platform was destroyed — detach
                    proj.stuckToPlatform = false;
                    proj.stuckPlatformBody = b2_nullBodyId;
                }
            }

            // Verify surface still exists (walls/ceilings can be destroyed)
            if ((proj.isStuck && !proj.stuckToPlayer) || proj.isMine) {
                b2Vec2 pp2 = b2Body_GetPosition(proj.bodyId);
                b2QueryFilter qf2 = b2DefaultQueryFilter();
                qf2.maskBits = CAT_PLATFORM;
                bool surfaceExists = false;
                for (auto [dx3, dy3] : std::initializer_list<std::pair<float,float>>{{0.f,-0.35f},{0.f,0.35f},{-0.35f,0.f},{0.35f,0.f}}) {
                    b2RayResult rr2 = b2World_CastRayClosest(m_physics.getWorldId(), pp2, {dx3, dy3}, qf2);
                    if (rr2.hit) { surfaceExists = true; break; }
                }
                if (!surfaceExists) {
                    // Surface gone — fall normally but mine stays armed
                    if (proj.isStuck) proj.isStuck = false;
                    // Mine stays isMine=true so it still detonates on proximity
                    b2Body_SetLinearDamping(proj.bodyId, 0.1f);
                    b2Body_SetGravityScale(proj.bodyId, 1.0f);
                    if (!proj.isMine) continue; // stickies re-enter normal flow
                    // mines continue to proximity check below
                }
            }

            // Sticky on surface: check if a player walks into it -> stick to them
            if (proj.isStuck && !proj.stuckToPlayer && !proj.isMine) {
                b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
                for (auto& player : m_players) {
                    if (player->getPlayerIndex() == proj.ownerIndex) continue;
                    if (!player->isAlive()) continue;
                    b2Vec2 pl = player->getPosition();
                    float dx = pl.x - pp.x, dy = pl.y - pp.y;
                    if (std::sqrt(dx*dx + dy*dy) < 1.0f) {
                        proj.stuckToPlayer = true;
                        proj.stuckPlayerIndex = player->getPlayerIndex();
                        proj.stuckOffset = {pp.x - pl.x, pp.y - pl.y};
                        b2Body_SetLinearVelocity(proj.bodyId, {0,0});
                        b2Body_SetGravityScale(proj.bodyId, 0.0f);
                        b2Body_SetLinearDamping(proj.bodyId, 0.0f);
                        break;
                    }
                }
            }

            bool shouldDetonate = false;

            if (proj.isMine) {
                b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
                float proxDist = proj.weapon.mineProximity > 0 ? proj.weapon.mineProximity : 2.0f;

                if (proj.mineTriggered) {
                    // Already triggered — count down beep timer
                    proj.mineDetonateTimer -= dt;
                    if (proj.mineDetonateTimer <= 0.0f) shouldDetonate = true;
                } else {
                    // Check proximity to ANY player (including owner)
                    for (auto& player : m_players) {
                        if (!player->isAlive()) continue;
                        b2Vec2 pl = player->getPosition();
                        float dx = pl.x - pp.x, dy = pl.y - pp.y;
                        float dist = std::sqrt(dx*dx + dy*dy);
                        if (dist < proxDist) {
                            proj.mineTriggered = true;
                            proj.mineDetonateTimer = 0.25f; // beep-beep then boom
                            break;
                        }
                    }
                }
                // Mines also expire after their lifetime
                proj.lifetime -= dt;
                if (proj.lifetime <= 0.0f) shouldDetonate = true;
            } else {
                // Sticky: countdown timer
                proj.stuckDetonateTimer -= dt;
                if (proj.stuckDetonateTimer <= 0.0f) shouldDetonate = true;
            }

            if (shouldDetonate) {
                // Implode charge: enter pulling phase instead of exploding immediately
                if (proj.weapon.grenadeCharge == GrenadeCharge::Implode && !proj.isImploding) {
                    proj.isImploding = true;
                    proj.implodeTimer = 0.0f;
                    proj.isStuck = false;
                    proj.isMine = false;
                    b2Body_SetLinearVelocity(proj.bodyId, {0,0});
                    b2Body_SetGravityScale(proj.bodyId, 0.0f);
                    continue;
                }
                b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
                bool isNukeCharge = (proj.weapon.grenadeCharge == GrenadeCharge::Nuclear);

                // Terrain damage varies by charge type
                float terrainR = proj.weapon.explosionRadius;
                float raggedness = 0.3f; // default moderate raggedness
                auto charge = proj.weapon.grenadeCharge;
                if (isNukeCharge || proj.weapon.destroysPlatforms) {
                    raggedness = 0.5f; // nuclear: very ragged edges with chunks
                } else if (charge == GrenadeCharge::Freeze || charge == GrenadeCharge::Time) {
                    terrainR *= 0.15f; // barely any terrain damage
                    raggedness = 0.0f;
                } else if (charge == GrenadeCharge::Lava) {
                    terrainR *= 0.2f; // very little terrain damage
                    raggedness = 0.0f;
                } else if (charge == GrenadeCharge::Implode) {
                    terrainR *= 0.5f;
                    raggedness = 0.4f;
                } else if (charge == GrenadeCharge::Frag) {
                    terrainR *= 0.5f;
                    raggedness = 0.35f;
                } else if (charge == GrenadeCharge::Normal) {
                    terrainR *= 0.6f;
                    raggedness = 0.3f;
                } else {
                    terrainR *= 0.5f; // non-grenade explosives
                }
                m_arena.carveCircle(m_physics, pp.x, pp.y, terrainR, raggedness);

                spawnExplosion(pp.x, pp.y, proj.weapon.explosionRadius, isNukeCharge, proj.weapon.grenadeCharge);

                // Push nearby mines/stickies
                pushNearbyProjectiles(pp.x, pp.y, proj.weapon.explosionRadius, proj.weapon.knockbackForce * 0.5f);

                for (auto& player : m_players) {
                    if (!player->isAlive()) continue;
                    b2Vec2 pl = player->getPosition();
                    float dx = pl.x - pp.x, dy = pl.y - pp.y;
                    float dist = std::sqrt(dx*dx + dy*dy);
                    if (dist < proj.weapon.explosionRadius) {
                        float f = 1.0f - dist / proj.weapon.explosionRadius;
                        float dmg = proj.weapon.damage * std::max(0.3f, f) * rules.damageMultiplier;
                        float kbDir = dx >= 0 ? 1.f : -1.f;
                        player->takeDamage(dmg, kbDir * proj.weapon.knockbackForce * f * rules.knockbackMultiplier,
                                           proj.weapon.knockbackForce * 0.3f * f * rules.knockbackMultiplier);
                        if (proj.weapon.grenadeCharge == GrenadeCharge::Freeze) {
                            // Freeze only applies within half the visual radius
                            float freezeR = proj.weapon.explosionRadius * 0.5f;
                            if (dist < freezeR) {
                                float ff = 1.0f - dist / freezeR;
                                int stacks = static_cast<int>(std::ceil(10.0f * ff));
                                player->addFreezeStacks(stacks);
                            }
                        }
                        if (proj.weapon.grenadeCharge == GrenadeCharge::Time)
                            player->applyTimeSlow(3.5f * f);
                    }
                }
                // Charge-specific zone effects
                if (proj.weapon.grenadeCharge == GrenadeCharge::Time) {
                    TimeSlowZone z;
                    z.x = pp.x; z.y = pp.y;
                    z.radius = 4.5f; z.slowFactor = 0.05f;
                    z.timer = 0.0f; z.duration = 3.5f; z.alive = true;
                    z.secondaryDamage = proj.weapon.damage * 1.5f;
                    z.secondaryKnockback = proj.weapon.knockbackForce * 1.2f;
                    z.freezeBuildup = 0; z.freezeDuration = 0;
                    m_timeSlowZones.push_back(z);
                }
                if (proj.weapon.grenadeCharge == GrenadeCharge::Lava) {
                    float r = proj.weapon.lavaRadius > 0 ? proj.weapon.lavaRadius : proj.weapon.explosionRadius * 0.8f;
                    deferredLavas.push_back({pp.x, pp.y, r, proj.weapon.lavaDps, proj.weapon.lavaDuration, proj.ownerIndex});
                }
                if (proj.weapon.grenadeCharge == GrenadeCharge::Frag && !proj.isFragChild) {
                    deferredFrags.push_back({pp.x, pp.y, proj.ownerIndex, proj.weapon});
                }
                proj.alive = false;
            }
            continue;
        }

        proj.lifetime -= dt;
        b2Vec2 pp = b2Body_GetPosition(proj.bodyId);

        // Boomerang: reverse direction halfway through lifetime
        if (proj.returnsToSender && proj.lifetime < proj.weapon.projectileLifetime * 0.5f) {
            // Find owner and home in on them
            for (auto& player : m_players) {
                if (player->getPlayerIndex() != proj.ownerIndex) continue;
                b2Vec2 op = player->getPosition();
                float dx = op.x - pp.x, dy = op.y - pp.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist > 0.5f) {
                    float spd = proj.weapon.projectileSpeed;
                    b2Body_SetLinearVelocity(proj.bodyId, {dx/dist*spd, dy/dist*spd});
                } else {
                    proj.alive = false; // caught it
                }
                break;
            }
        }

        // Wrap around screen
        if (m_wrapAround) {
            float worldHalfW = SCREEN_WIDTH / PPM / 2.0f + 2.0f;
            if (pp.x < -worldHalfW) { pp.x = worldHalfW - 1.0f; b2Body_SetTransform(proj.bodyId, pp, b2Body_GetRotation(proj.bodyId)); }
            if (pp.x >  worldHalfW) { pp.x = -worldHalfW + 1.0f; b2Body_SetTransform(proj.bodyId, pp, b2Body_GetRotation(proj.bodyId)); }
        }

        bool isExplosive = (proj.weapon.type == WeaponType::Explosive);

        // === GRENADE CASING: in-flight behavior ===
        if (isExplosive && !proj.isStuck && !proj.isMine && !proj.isFragChild &&
            proj.lifetime < proj.weapon.projectileLifetime - 0.05f) {
            GrenadeCasing casing = proj.weapon.grenadeCasing;
            if (proj.weapon.sticksToSurfaces && casing == GrenadeCasing::Normal)
                casing = GrenadeCasing::Sticky;

            // Check surface contact (skip for Ballistic and Bounce — they never stick)
            bool hitSurface = false;
            b2BodyId hitPlatformBody = b2_nullBodyId;
            if (casing != GrenadeCasing::Ballistic && casing != GrenadeCasing::Bounce) {
                b2QueryFilter qf = b2DefaultQueryFilter();
                qf.maskBits = CAT_PLATFORM;
                for (auto [dx2, dy2] : std::initializer_list<std::pair<float,float>>{{0.f,-0.25f},{0.f,0.25f},{-0.25f,0.f},{0.25f,0.f}}) {
                    b2RayResult rr = b2World_CastRayClosest(m_physics.getWorldId(), pp, {dx2, dy2}, qf);
                    if (rr.hit) {
                        hitSurface = true;
                        hitPlatformBody = b2Shape_GetBody(rr.shapeId);
                        break;
                    }
                }
            }

            // Sticky casing: stick to surfaces AND players
            if (casing == GrenadeCasing::Sticky) {
                // Check player contact (in flight -> stick to player)
                bool stuck = false;
                for (auto& player : m_players) {
                    if (player->getPlayerIndex() == proj.ownerIndex) continue;
                    if (!player->isAlive()) continue;
                    b2Vec2 pl = player->getPosition();
                    float dx = pl.x - pp.x, dy = pl.y - pp.y;
                    if (std::sqrt(dx*dx + dy*dy) < 1.0f) {
                        proj.isStuck = true;
                        proj.stuckToPlayer = true;
                        proj.stuckPlayerIndex = player->getPlayerIndex();
                        proj.stuckOffset = {pp.x - pl.x, pp.y - pl.y};
                        proj.stuckDetonateTimer = proj.weapon.detonationDelay > 0 ? proj.weapon.detonationDelay : 2.0f;
                        b2Body_SetLinearVelocity(proj.bodyId, {0,0});
                        b2Body_SetGravityScale(proj.bodyId, 0.0f);
                        stuck = true; break;
                    }
                }
                if (stuck) continue;
                if (hitSurface) {
                    proj.isStuck = true;
                    proj.stuckDetonateTimer = proj.weapon.detonationDelay > 0 ? proj.weapon.detonationDelay : 2.0f;
                    b2Body_SetLinearVelocity(proj.bodyId, {0,0});
                    b2Body_SetGravityScale(proj.bodyId, 0.0f);
                    b2Body_SetLinearDamping(proj.bodyId, 8.0f);
                    // Track platform for movement
                    if (B2_IS_NON_NULL(hitPlatformBody)) {
                        proj.stuckToPlatform = true;
                        proj.stuckPlatformBody = hitPlatformBody;
                        b2Vec2 platPos = b2Body_GetPosition(hitPlatformBody);
                        proj.stuckPlatformOffset = {pp.x - platPos.x, pp.y - platPos.y};
                    }
                    continue;
                }
            }

            // Mine casing: land on surface and arm
            if (casing == GrenadeCasing::Mine && hitSurface) {
                proj.isMine = true;
                proj.lifetime = 30.0f;
                b2Body_SetLinearVelocity(proj.bodyId, {0,0});
                b2Body_SetGravityScale(proj.bodyId, 0.0f);
                b2Body_SetLinearDamping(proj.bodyId, 5.0f);
                // Track platform for movement
                if (B2_IS_NON_NULL(hitPlatformBody)) {
                    proj.stuckToPlatform = true;
                    proj.stuckPlatformBody = hitPlatformBody;
                    b2Vec2 platPos = b2Body_GetPosition(hitPlatformBody);
                    proj.stuckPlatformOffset = {pp.x - platPos.x, pp.y - platPos.y};
                }
                continue;
            }
        }

        // Portal projectile: die on surface contact (after a grace period to get away from shooter)
        if (proj.createsPortal && !proj.isStuck) {
            float age = proj.weapon.projectileLifetime - proj.lifetime;
            if (age > 0.15f) {  // grace period: don't check for first 0.15s
                b2QueryFilter qf = b2DefaultQueryFilter();
                qf.maskBits = CAT_PLATFORM;
                bool hitSurface2 = false;
                for (auto [dx2, dy2] : std::initializer_list<std::pair<float,float>>{{0.f,-0.2f},{0.f,0.2f},{-0.2f,0.f},{0.2f,0.f}}) {
                    b2RayResult rr = b2World_CastRayClosest(m_physics.getWorldId(), pp, {dx2, dy2}, qf);
                    if (rr.hit) { hitSurface2 = true; break; }
                }
                if (hitSurface2) {
                    proj.alive = false;
                    continue;
                }
            }
        }

        // Bouncing projectiles
        if (proj.bouncesLeft > 0) {
            b2Vec2 vel = b2Body_GetLinearVelocity(proj.bodyId);
            bool bounced = false;

            // Platform bounce (ground/ceiling)
            b2QueryFilter qf = b2DefaultQueryFilter();
            qf.maskBits = CAT_PLATFORM;
            b2RayResult rr = b2World_CastRayClosest(m_physics.getWorldId(), pp, {0,-0.3f}, qf);
            if (rr.hit && vel.y < 0) {
                vel.y = -vel.y * proj.weapon.bounceEnergyRetention;
                bounced = true;
            }
            // Ceiling bounce
            b2RayResult ru = b2World_CastRayClosest(m_physics.getWorldId(), pp, {0, 0.3f}, qf);
            if (ru.hit && vel.y > 0) {
                vel.y = -vel.y * proj.weapon.bounceEnergyRetention;
                bounced = true;
            }
            // Wall bounce (left/right platforms)
            b2RayResult rl = b2World_CastRayClosest(m_physics.getWorldId(), pp, {-0.3f, 0}, qf);
            if (rl.hit && vel.x < 0) {
                vel.x = -vel.x * proj.weapon.bounceEnergyRetention;
                bounced = true;
            }
            b2RayResult rright = b2World_CastRayClosest(m_physics.getWorldId(), pp, {0.3f, 0}, qf);
            if (rright.hit && vel.x > 0) {
                vel.x = -vel.x * proj.weapon.bounceEnergyRetention;
                bounced = true;
            }

            // Arena boundary bounce (invisible walls at ±60)
            constexpr float bounceW = 58.0f;
            constexpr float bounceTop = 34.0f;
            constexpr float bounceBot = -28.0f;
            if (pp.x < -bounceW && vel.x < 0) { vel.x = -vel.x; bounced = true; }
            if (pp.x >  bounceW && vel.x > 0) { vel.x = -vel.x; bounced = true; }
            if (pp.y >  bounceTop && vel.y > 0) { vel.y = -vel.y; bounced = true; }
            if (pp.y <  bounceBot && vel.y < 0) { vel.y = -vel.y; bounced = true; }

            if (bounced) {
                b2Body_SetLinearVelocity(proj.bodyId, vel);
                proj.bouncesLeft--;
            }
        }

        bool expired = proj.lifetime <= 0.0f;

        // Non-explosive, non-bouncing bullets: die on surface contact
        if (!isExplosive && proj.bouncesLeft <= 0 && !proj.isStuck && !proj.isThrownWeapon &&
            !proj.createsPortal) {
            b2Vec2 vel = b2Body_GetLinearVelocity(proj.bodyId);
            float spd = std::sqrt(vel.x*vel.x + vel.y*vel.y);
            // Speed dropped significantly = hit something
            if (spd < proj.weapon.projectileSpeed * 0.3f && proj.lifetime < proj.weapon.projectileLifetime - 0.05f) {
                float envR = proj.weapon.envDamageRadius;
                if (envR <= 0.0f) envR = proj.weapon.damage * 0.015f;
                m_arena.carveCircle(m_physics, pp.x, pp.y, envR);
                // Apply any on-surface effects (time slow zone, etc.)
                if (proj.weapon.timeSlowDuration > 0.0f && proj.weapon.timeSlowRadius > 0.0f) {
                    TimeSlowZone z;
                    z.x = pp.x; z.y = pp.y;
                    z.radius = proj.weapon.timeSlowRadius;
                    z.slowFactor = proj.weapon.timeSlowFactor > 0 ? proj.weapon.timeSlowFactor : 0.1f;
                    z.timer = 0.0f; z.duration = proj.weapon.timeSlowDuration; z.alive = true;
                    z.secondaryDamage = 0; z.secondaryKnockback = 0;
                    z.freezeBuildup = 0; z.freezeDuration = 0;
                    m_timeSlowZones.push_back(z);
                }
                if (proj.weapon.portalDuration > 0.0f) {
                    proj.createsPortal = true; // let portal logic handle it
                }
                if (proj.weapon.healingRadius > 0.0f && proj.weapon.healOverTime > 0.0f) {
                    HealZone hz;
                    hz.x = pp.x; hz.y = pp.y;
                    hz.radius   = proj.weapon.healingRadius;
                    hz.healRate = proj.weapon.healOverTime;
                    hz.timer    = 0.0f;
                    hz.duration = proj.weapon.healDuration > 0 ? proj.weapon.healDuration : 3.0f;
                    hz.alive    = true;
                    m_healZones.push_back(hz);
                }
                proj.alive = false;
                continue;
            }
            // Also check raycast for surface proximity (for fast bullets that might not slow down in one frame)
            b2QueryFilter qfBullet = b2DefaultQueryFilter();
            qfBullet.maskBits = CAT_PLATFORM;
            bool hitWall = false;
            for (auto [dx2, dy2] : std::initializer_list<std::pair<float,float>>{{0.f,-0.2f},{0.f,0.2f},{-0.2f,0.f},{0.2f,0.f}}) {
                b2RayResult rr = b2World_CastRayClosest(m_physics.getWorldId(), pp, {dx2, dy2}, qfBullet);
                if (rr.hit) { hitWall = true; break; }
            }
            if (hitWall && proj.lifetime < proj.weapon.projectileLifetime - 0.05f) {
                float envR = proj.weapon.envDamageRadius;
                if (envR <= 0.0f) envR = proj.weapon.damage * 0.015f;
                m_arena.carveCircle(m_physics, pp.x, pp.y, envR);
                if (proj.weapon.timeSlowDuration > 0.0f && proj.weapon.timeSlowRadius > 0.0f) {
                    TimeSlowZone z;
                    z.x = pp.x; z.y = pp.y;
                    z.radius = proj.weapon.timeSlowRadius;
                    z.slowFactor = proj.weapon.timeSlowFactor > 0 ? proj.weapon.timeSlowFactor : 0.1f;
                    z.timer = 0.0f; z.duration = proj.weapon.timeSlowDuration; z.alive = true;
                    z.secondaryDamage = 0; z.secondaryKnockback = 0;
                    z.freezeBuildup = 0; z.freezeDuration = 0;
                    m_timeSlowZones.push_back(z);
                }
                if (proj.weapon.healingRadius > 0.0f && proj.weapon.healOverTime > 0.0f) {
                    HealZone hz;
                    hz.x = pp.x; hz.y = pp.y;
                    hz.radius   = proj.weapon.healingRadius;
                    hz.healRate = proj.weapon.healOverTime;
                    hz.timer    = 0.0f;
                    hz.duration = proj.weapon.healDuration > 0 ? proj.weapon.healDuration : 3.0f;
                    hz.alive    = true;
                    m_healZones.push_back(hz);
                }
                proj.alive = false;
                continue;
            }
        }

        bool contactDetonation = false;
        if (isExplosive && proj.lifetime < proj.weapon.projectileLifetime - 0.1f) {
            b2Vec2 vel = b2Body_GetLinearVelocity(proj.bodyId);
            float spd = std::sqrt(vel.x*vel.x + vel.y*vel.y);

            GrenadeCasing casing = proj.weapon.grenadeCasing;
            if (proj.weapon.sticksToSurfaces && casing == GrenadeCasing::Normal)
                casing = GrenadeCasing::Sticky;

            if (casing == GrenadeCasing::Ballistic || proj.isFragChild) {
                // Ballistic / frag children: detonate on any contact
                if (spd < 1.5f) contactDetonation = true;
                b2QueryFilter qf = b2DefaultQueryFilter();
                qf.maskBits = CAT_PLATFORM;
                for (auto [dx2, dy2] : std::initializer_list<std::pair<float,float>>{{0.f,-0.25f},{0.f,0.25f},{-0.25f,0.f},{0.25f,0.f}}) {
                    b2RayResult rr = b2World_CastRayClosest(m_physics.getWorldId(), pp, {dx2, dy2}, qf);
                    if (rr.hit) { contactDetonation = true; break; }
                }
            } else {
                // Normal, Sticky: detonate on speed drop (ground contact)
                // Mine, Bounce: do NOT detonate on speed drop
                if (casing != GrenadeCasing::Mine && casing != GrenadeCasing::Bounce && spd < 1.0f)
                    contactDetonation = true;
            }
        }

        // Bounce casing: timer-based detonation
        if (isExplosive && proj.weapon.grenadeCasing == GrenadeCasing::Bounce &&
            !proj.isStuck && !proj.isMine) {
            float age = proj.weapon.projectileLifetime - proj.lifetime;
            float delay = proj.weapon.detonationDelay > 0 ? proj.weapon.detonationDelay : 5.0f;
            if (age >= delay) contactDetonation = true;
        }

        bool shouldDetonate = contactDetonation || (expired && isExplosive);

        if (expired && !isExplosive) {
            float envR = proj.weapon.envDamageRadius;
            if (envR <= 0.0f) envR = proj.weapon.damage * 0.015f;
            m_arena.carveCircle(m_physics, pp.x, pp.y, envR);
            proj.alive = false;
            continue;
        }

        // Check player hits (per-body-part hitbox)
        bool hitAnyPlayer = false;
        for (auto& player : m_players) {
            // Skip owner unless: returning boomerang, or bouncing projectile that has bounced
            bool isOwner = (player->getPlayerIndex() == proj.ownerIndex);
            bool bouncedAlready = (proj.weapon.bounces > 0 && proj.bouncesLeft < proj.weapon.bounces);
            if (isOwner && !proj.returnsToSender && !bouncedAlready) continue;
            if (!player->isAlive()) continue;
            // Multi-hit: skip already-hit players unless boomerang returning
            if (proj.hitsMultipleTimes) {
                bool alreadyHit = false;
                for (int idx : proj.hitPlayers) if (idx == player->getPlayerIndex()) { alreadyHit = true; break; }
                if (alreadyHit) continue;
            }

            // For explosives about to detonate, use explosion radius; otherwise use body-part hitboxes
            float checkR = isExplosive ? (shouldDetonate ? proj.weapon.explosionRadius : 1.2f) : 1.2f;
            HitResult hr = player->checkHit(pp.x, pp.y, checkR);
            if (!hr.hit) continue;

            float dx = pp.x - hr.partPos.x, dy = pp.y - hr.partPos.y;

            // Shield: reflect projectile back
            if (player->isReflecting() && !isExplosive) {
                b2Vec2 vel = b2Body_GetLinearVelocity(proj.bodyId);
                b2Body_SetLinearVelocity(proj.bodyId, {-vel.x, -vel.y});
                proj.ownerIndex = player->getPlayerIndex();
                hitAnyPlayer = true;
                break;
            }
            // Shield: block projectile
            if (player->hasShield() && !isExplosive) {
                proj.alive = false;
                break;
            }

            // Non-Ballistic grenades pass through players without dealing direct damage
            // (they only hurt via explosion radius on detonation)
            if (isExplosive) {
                bool isBallistic = (proj.weapon.grenadeCasing == GrenadeCasing::Ballistic || proj.isFragChild);
                if (!isBallistic && !shouldDetonate) {
                    // Don't hit this player, grenade flies through
                    continue;
                }
            }

            // Apply damage with body-part multiplier (skip if weapon does 0 or negative damage)
            {
                float baseDmg = proj.weapon.damage * rules.damageMultiplier;
                float kbDir = (dx >= 0) ? 1.0f : -1.0f;
                float kbX = proj.weapon.knockbackForce * kbDir * rules.knockbackMultiplier;
                float kbY = proj.weapon.knockbackForce * 0.4f  * rules.knockbackMultiplier;
                if (proj.weapon.damage > 0.0f)
                    player->takeDamageAt(hr.part, baseDmg, kbX, kbY);
                else if (proj.weapon.damage < 0.0f)
                    player->takeDamage(baseDmg, kbX, kbY); // healing — no body-part mult

                // Apply status effects (these don't scale with body part)
                if (proj.weapon.poisonDps > 0.0f)
                    player->applyPoison(proj.weapon.poisonDps, proj.weapon.poisonDuration);
                if (proj.weapon.slowEffect > 0.0f)
                    player->applySlow(1.0f - proj.weapon.slowEffect, proj.weapon.slowDuration);
                if (proj.weapon.freezeBuildup > 0.0f)
                    player->addFreezeStacks(1);
                if (proj.weapon.timeSlowDuration > 0.0f && proj.weapon.timeSlowRadius <= 0.0f)
                    player->applyTimeSlow(proj.weapon.timeSlowDuration);  // Time Gun: direct hit stops player
                if (proj.weapon.stunDuration > 0.0f)
                    player->applyStun(proj.weapon.stunDuration);
                if (proj.weapon.shrinkScale < 1.0f)
                    player->applyShrink(proj.weapon.shrinkScale, proj.weapon.speedIncrease, proj.weapon.shrinkDuration);
                if (proj.weapon.mindControlDuration > 0.0f)
                    player->applyMindControl(proj.ownerIndex, proj.weapon.mindControlDuration);
                if (proj.weapon.swapsPositions) {
                    for (auto& p : m_players) {
                        if (p->getPlayerIndex() == proj.ownerIndex) {
                            b2Vec2 myPos = p->getPosition();
                            b2Vec2 theirPos = player->getPosition();
                            p->teleportTo(theirPos.x, theirPos.y);
                            player->teleportTo(myPos.x, myPos.y);
                            break;
                        }
                    }
                }
                if (proj.weapon.damage < 0.0f)
                    player->heal(-proj.weapon.damage * rules.damageMultiplier);
                if (proj.weapon.healingRadius > 0.0f && proj.weapon.healOverTime > 0.0f) {
                    b2Vec2 pos = player->getPosition();
                    HealZone hz;
                    hz.x = pos.x; hz.y = pos.y;
                    hz.radius   = proj.weapon.healingRadius;
                    hz.healRate = proj.weapon.healOverTime;
                    hz.timer    = 0.0f;
                    hz.duration = proj.weapon.healDuration > 0 ? proj.weapon.healDuration : 3.0f;
                    hz.alive    = true;
                    m_healZones.push_back(hz);
                }
                if (proj.weapon.chainTargets > 0 && proj.weapon.chainRange > 0.0f) {
                    b2Vec2 curPos = player->getPosition();
                    float chainDmg = proj.weapon.damage * proj.weapon.chainDamageFalloff;
                    StickFigure* prev = player.get();
                    for (int c = 0; c < proj.weapon.chainTargets; c++) {
                        StickFigure* next = findNearestEnemy(curPos.x, curPos.y,
                                                             prev->getPlayerIndex(), proj.weapon.chainRange);
                        if (!next) break;
                        next->takeDamage(chainDmg * rules.damageMultiplier,
                                         kbX * 0.5f, kbY * 0.5f);
                        if (proj.weapon.stunDuration > 0.0f) next->applyStun(proj.weapon.stunDuration * 0.5f);
                        curPos   = next->getPosition();
                        chainDmg *= proj.weapon.chainDamageFalloff;
                        prev     = next;
                    }
                }
            }
            proj.hitPlayers.push_back(player->getPlayerIndex());

            if (!isExplosive && !proj.hitsMultipleTimes && !proj.returnsToSender) {
                float envR = proj.weapon.envDamageRadius;
                if (envR <= 0.0f) envR = proj.weapon.damage * 0.02f;
                m_arena.carveCircle(m_physics, pp.x, pp.y, envR);
                proj.alive = false;
                break;
            }
            hitAnyPlayer = true;
            // Only Ballistic casing detonates on hitting a player
            if (!proj.hitsMultipleTimes) {
                bool isBallistic = (proj.weapon.grenadeCasing == GrenadeCasing::Ballistic || proj.isFragChild);
                if (!isExplosive || isBallistic)
                    shouldDetonate = true;
            }
        }

        // Detonate explosive
        if (isExplosive && shouldDetonate && proj.alive) {
            // Implode charge: enter pulling phase instead of exploding immediately
            if (proj.weapon.grenadeCharge == GrenadeCharge::Implode && !proj.isImploding) {
                proj.isImploding = true;
                proj.implodeTimer = 0.0f;
                b2Body_SetLinearVelocity(proj.bodyId, {0,0});
                b2Body_SetGravityScale(proj.bodyId, 0.0f);
                continue;
            }
            bool isNukeCharge = (proj.weapon.grenadeCharge == GrenadeCharge::Nuclear || proj.weapon.destroysPlatforms);

            // Apply explosion damage + charge effects to players in radius
            // Skip players already hit by direct contact (they already took damage)
            for (auto& player : m_players) {
                if (!player->isAlive()) continue;

                // Check if this player was already hit by direct contact
                bool wasDirectHit = false;
                for (int idx : proj.hitPlayers) {
                    if (idx == player->getPlayerIndex()) { wasDirectHit = true; break; }
                }

                b2Vec2 plp = player->getPosition();
                float dx = pp.x - plp.x, dy = pp.y - plp.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist < proj.weapon.explosionRadius) {
                    float f = std::max(0.3f, 1.0f - dist / proj.weapon.explosionRadius);

                    // Apply blast damage only to players NOT already hit directly
                    if (!wasDirectHit) {
                        float dmg = proj.weapon.damage * f * rules.damageMultiplier;
                        float kbDir = dx >= 0 ? 1.f : -1.f;
                        player->takeDamage(dmg,
                            kbDir * proj.weapon.knockbackForce * f * rules.knockbackMultiplier,
                            proj.weapon.knockbackForce * 0.3f * f * rules.knockbackMultiplier);
                    }

                    // Apply charge effects only to players NOT already hit directly
                    if (!wasDirectHit) {
                        if (proj.weapon.grenadeCharge == GrenadeCharge::Freeze) {
                            float freezeR = proj.weapon.explosionRadius * 0.5f;
                            if (dist < freezeR) {
                                float ff = 1.0f - dist / freezeR;
                                int stacks = static_cast<int>(std::ceil(10.0f * ff));
                                player->addFreezeStacks(stacks);
                            }
                        }
                        if (proj.weapon.grenadeCharge == GrenadeCharge::Time) {
                            player->applyTimeSlow(3.5f * f);
                        }
                    }
                }
            }
            // Time charge: create time slow zone
            if (proj.weapon.grenadeCharge == GrenadeCharge::Time) {
                TimeSlowZone z;
                z.x = pp.x; z.y = pp.y;
                z.radius = 4.5f; z.slowFactor = 0.05f;
                z.timer = 0.0f; z.duration = 3.5f; z.alive = true;
                z.secondaryDamage = proj.weapon.damage * 1.5f;
                z.secondaryKnockback = proj.weapon.knockbackForce * 1.2f;
                z.freezeBuildup = 0; z.freezeDuration = 0;
                m_timeSlowZones.push_back(z);
            }
            // Lava charge: spawn lava pool
            if (proj.weapon.grenadeCharge == GrenadeCharge::Lava) {
                float r = proj.weapon.lavaRadius > 0 ? proj.weapon.lavaRadius : proj.weapon.explosionRadius * 0.8f;
                deferredLavas.push_back({pp.x, pp.y, r, proj.weapon.lavaDps, proj.weapon.lavaDuration, proj.ownerIndex});
            }
            // Frag charge: spawn child grenades
            if (proj.weapon.grenadeCharge == GrenadeCharge::Frag && !proj.isFragChild) {
                deferredFrags.push_back({pp.x, pp.y, proj.ownerIndex, proj.weapon});
            }

            // Terrain damage varies by charge type
            {
                float terrainR2 = proj.weapon.explosionRadius;
                float raggedness2 = 0.3f;
                auto charge2 = proj.weapon.grenadeCharge;
                if (isNukeCharge) {
                    raggedness2 = 0.5f;
                } else if (charge2 == GrenadeCharge::Freeze || charge2 == GrenadeCharge::Time) {
                    terrainR2 *= 0.15f; raggedness2 = 0.0f;
                } else if (charge2 == GrenadeCharge::Lava) {
                    terrainR2 *= 0.2f; raggedness2 = 0.0f;
                } else if (charge2 == GrenadeCharge::Implode) {
                    terrainR2 *= 0.5f; raggedness2 = 0.4f;
                } else if (charge2 == GrenadeCharge::Frag) {
                    terrainR2 *= 0.5f; raggedness2 = 0.35f;
                } else if (charge2 == GrenadeCharge::Normal) {
                    terrainR2 *= 0.6f; raggedness2 = 0.3f;
                } else {
                    terrainR2 *= 0.5f;
                }
                m_arena.carveCircle(m_physics, pp.x, pp.y, terrainR2, raggedness2);
            }

            spawnExplosion(pp.x, pp.y, proj.weapon.explosionRadius, isNukeCharge, proj.weapon.grenadeCharge);

            // Push nearby mines/stickies
            pushNearbyProjectiles(pp.x, pp.y, proj.weapon.explosionRadius, proj.weapon.knockbackForce * 0.5f);

            proj.alive = false;
        }
    }

    // Execute deferred spawns (safe — not iterating m_projectiles anymore)
    for (auto& df : deferredFrags)
        spawnFragChildren(df.x, df.y, df.ownerIndex, df.weapon);
    for (auto& dl : deferredLavas)
        spawnLavaPool(dl.x, dl.y, dl.r, dl.dps, dl.dur, dl.owner);

    for (auto& proj : m_projectiles) {
        if (!proj.alive) {
            b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
            // Spawn weapon pickup where thrown weapon landed
            if (proj.isThrownWeapon) {
                WeaponPickup pickup;
                pickup.position = pp;
                pickup.weapon = proj.thrownWeaponData;
                pickup.bobTimer = 0.0f;
                pickup.alive = true;
                m_pickups.push_back(pickup);
            }
            // Create time slow zone where time projectile landed (Time Gun)
            if (proj.weapon.timeSlowRadius > 0.0f && proj.weapon.type != WeaponType::Explosive) {
                TimeSlowZone z;
                z.x = pp.x; z.y = pp.y;
                z.radius     = proj.weapon.timeSlowRadius;
                z.slowFactor = proj.weapon.timeSlowFactor > 0 ? proj.weapon.timeSlowFactor : 0.05f;
                z.timer      = 0.0f;
                z.duration   = proj.weapon.timeSlowDuration > 0 ? proj.weapon.timeSlowDuration : 3.0f;
                z.alive      = true;
                m_timeSlowZones.push_back(z);
            }
            // Create portal where portal projectile landed
            if (proj.createsPortal) {
                Portal newP;
                newP.x = pp.x; newP.y = pp.y;
                newP.timer = 0.0f;
                newP.duration = proj.portalDuration;
                newP.alive = true;

                // Find an unlinked portal to pair with
                int unlinkedIdx = -1;
                for (int pi2 = 0; pi2 < static_cast<int>(m_portals.size()); pi2++)
                    if (m_portals[pi2].alive && m_portals[pi2].pairIndex < 0) { unlinkedIdx = pi2; break; }

                if (unlinkedIdx >= 0) {
                    int myIdx = static_cast<int>(m_portals.size());
                    newP.pairIndex = unlinkedIdx;
                    m_portals.push_back(newP);
                    m_portals[unlinkedIdx].pairIndex = myIdx;
                } else {
                    newP.pairIndex = -1;
                    m_portals.push_back(newP);
                }
            }
            b2DestroyBody(proj.bodyId);
        }
    }
    // Sticky projectiles slow nearby projectiles and grenades
    for (const auto& stuck : m_projectiles) {
        if (!stuck.alive || !stuck.isStuck) continue;
        if (!B2_IS_NON_NULL(stuck.bodyId) || !b2Body_IsValid(stuck.bodyId)) continue;
        if (stuck.weapon.slowEffect <= 0) continue;
        b2Vec2 sp = b2Body_GetPosition(stuck.bodyId);
        float slowRad = 3.0f;
        for (auto& proj : m_projectiles) {
            if (!proj.alive || proj.isStuck || proj.bodyId.index1 == stuck.bodyId.index1) continue;
            if (!B2_IS_NON_NULL(proj.bodyId) || !b2Body_IsValid(proj.bodyId)) continue;
            b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
            float dx = sp.x - pp.x, dy = sp.y - pp.y;
            if (std::sqrt(dx*dx+dy*dy) < slowRad) {
                b2Vec2 v = b2Body_GetLinearVelocity(proj.bodyId);
                b2Body_SetLinearVelocity(proj.bodyId, {v.x * 0.92f, v.y * 0.92f});
            }
        }
    }

    m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(),
        [](const Projectile& p){ return !p.alive; }), m_projectiles.end());

    for (auto& fx : m_explosions) { fx.timer += dt; if (fx.timer >= fx.duration) fx.alive = false; }
    m_explosions.erase(std::remove_if(m_explosions.begin(), m_explosions.end(),
        [](const ExplosionEffect& e){ return !e.alive; }), m_explosions.end());
}

void Game::updateWeaponSpawns(float dt) {
    const auto& rules = m_rulesEngine.getRules();
    m_weaponSpawnTimer -= dt;
    if (m_weaponSpawnTimer <= 0.0f && static_cast<int>(m_pickups.size()) < rules.weaponSpawnMax) {
        static const std::vector<std::string> kInnate = {
            "Fists","Poison Spit","Horn Blast","Jaw Snap","Purse Swing"};
        const auto& all = m_weaponFactory.getAllWeapons();
        std::vector<const WeaponData*> eligible;
        for (const auto& w : all) {
            bool innate = false;
            for (const auto& n : kInnate) if (w.name == n) { innate = true; break; }
            if (innate) continue;
            // Add weapon multiple times based on spawn_weight
            int weight = std::max(1, w.spawnWeight);
            for (int i = 0; i < weight; i++)
                eligible.push_back(&w);
        }
        if (!eligible.empty()) {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<size_t> dist(0, eligible.size()-1);
            WeaponPickup pickup;
            pickup.position = m_arena.getRandomPlatformTop();
            pickup.weapon   = *eligible[dist(rng)];
            // If this is a grenade launcher, randomize the charge+casing
            if (pickup.weapon.isGrenadeLauncher) {
                pickup.weapon = randomizeGrenade(pickup.weapon);
            }
            pickup.alive    = true;
            pickup.bobTimer = 0.0f;
            m_pickups.push_back(pickup);
            std::cout << "[Spawn] " << pickup.weapon.name << "\n";
        }
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

    // World bounds in meters (screen edges + margin)
    float worldHalfW = SCREEN_WIDTH / PPM / 2.0f + 10.0f;  // ~31m wide
    float worldTop   = SCREEN_HEIGHT / PPM / 2.0f + 20.0f;  // ~32m up
    float worldBot   = rules.fallDeathY - 15.0f;             // -35m down

    for (auto& player : m_players) {
        if (!player->isAlive() || player->isWaitingToRespawn()) continue;
        b2Vec2 pos = player->getPosition();

        // Horizontal wrap (always active when wrap-around is on)
        if (m_wrapAround) {
            if (pos.x < -worldHalfW) {
                player->teleportTo(worldHalfW - 1.0f, pos.y);
            } else if (pos.x > worldHalfW) {
                player->teleportTo(-worldHalfW + 1.0f, pos.y);
            }
        }

        // Fall death (always kills, never wraps vertically)
        if (pos.y < worldBot) {
            player->takeDamage(9999.0f, 0.0f, 0.0f);
            // Don't decrement lives here — checkCombatDeaths handles it
        }
    }
}


void Game::checkCombatDeaths() {
    const GameRules& rules = m_rulesEngine.getRules();
    const auto& spawns = m_arena.getSpawnPoints();
    
    for (size_t i = 0; i < m_players.size(); i++) {
        auto& player = m_players[i];
        
        // Skip if already waiting to respawn
        if (player->isWaitingToRespawn()) continue;
        
        // Check if player died from damage
        if (!player->isAlive() && player->getLives() > 0) {
            // Decrement lives
            int lives = player->getLives() - 1;
            player->setLives(lives);
            
            // Start respawn timer if they have lives remaining
            if (lives > 0) {
                player->startRespawnTimer(rules.respawnDelay, spawns[i].x, spawns[i].y);
                std::cout << "Player " << i << " died! Lives remaining: " << lives << "\n";
            } else {
                std::cout << "Player " << i << " eliminated!\n";
            }
        }
    }
}

void Game::checkRoundEnd() {
    // Count players still in the game (alive OR waiting to respawn with lives left)
    int inGame = 0; int lastIdx = -1;
    for (const auto& p : m_players) {
        if (p->getLives() > 0) {
            inGame++;
            lastIdx = p->getPlayerIndex();
        }
    }

    // Round ends when only 0 or 1 players have lives remaining
    if (inGame <= 1 && m_players.size() > 1) {
        if (lastIdx >= 0) std::cout << "Player " << lastIdx << " wins the round!\n";
        else std::cout << "Draw!\n";

        // Auto-restart: rebuild the level and reset all players
        m_arena.createLevel(m_physics, m_selectedLevel);
        const auto& spawns = m_arena.getSpawnPoints();
        const auto& rules = m_rulesEngine.getRules();

        // Clear projectiles, pickups, effects — destroy physics bodies first
        for (auto& proj : m_projectiles) {
            if (b2Body_IsValid(proj.bodyId)) b2DestroyBody(proj.bodyId);
        }
        m_projectiles.clear();
        m_pickups.clear();
        m_explosions.clear();
        m_blackHoles.clear();
        m_tornadoes.clear();
        m_timeSlowZones.clear();
        m_portals.clear();
        m_meteorShowers.clear();
        m_healZones.clear();
        m_lavaPools.clear();
        for (auto& lp : m_lavaParticles) b2DestroyBody(lp.bodyId);
        m_lavaParticles.clear();
        for (auto& g : m_grapples) { if (B2_IS_NON_NULL(g.hookProjectile) && b2Body_IsValid(g.hookProjectile)) b2DestroyBody(g.hookProjectile); if (B2_IS_NON_NULL(g.hookBody) && b2Body_IsValid(g.hookBody)) b2DestroyBody(g.hookBody); }
        m_grapples.clear();

        // Respawn all players with full lives and health
        for (size_t i = 0; i < m_players.size(); i++) {
            size_t spawnIdx = i % spawns.size();
            m_players[i]->respawn(spawns[spawnIdx].x, spawns[spawnIdx].y);
            m_players[i]->setLives(rules.livesPerPlayer);
            m_players[i]->setMaxHealth(rules.maxHealth);
        }

        m_roundTimer = rules.roundTimeSeconds;
        m_weaponSpawnTimer = rules.weaponSpawnInterval;
        // Stay in Playing state — no pause
    }
}

void Game::render() {
    m_renderer.clear(sf::Color(25, 25, 30));

    // Set camera view — offsets all rendering to follow players
    {
        sf::View view = m_renderer.getWindow().getDefaultView();
        // Camera offset: shift view center by camera position in pixel space
        float cx = SCREEN_CX + m_camX * PPM;
        float cy = SCREEN_CY - m_camY * PPM;
        view.setCenter({cx, cy});
        // Zoom: increase view size to show more of the map
        view.setSize({SCREEN_WIDTH * m_camZoom, SCREEN_HEIGHT * m_camZoom});
        m_renderer.getWindow().setView(view);
    }

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
        sf::Vector2f sp = {SCREEN_CX + pos.x * PPM, SCREEN_CY - pos.y * PPM};
        b2Vec2 vel = b2Body_GetLinearVelocity(proj.bodyId);
        float spd = std::sqrt(vel.x*vel.x + vel.y*vel.y);
        float t = proj.weapon.projectileLifetime - proj.lifetime; // age
        const std::string& wn = proj.weapon.name;

        // Imploding grenade: vortex effect
        if (proj.isImploding) {
            float pullR = proj.weapon.implodePullRadius > 0 ? proj.weapon.implodePullRadius : 12.0f;
            float pullDur = proj.weapon.implodePullDuration > 0 ? proj.weapon.implodePullDuration : 1.0f;
            float progress = proj.implodeTimer / pullDur;
            float screenR = pullR * PPM;

            // Shrinking vortex ring
            float ringR = screenR * (1.0f - progress * 0.7f);
            sf::CircleShape ring(ringR); ring.setOrigin({ringR, ringR}); ring.setPosition(sp);
            ring.setFillColor(sf::Color(30, 0, 80, static_cast<uint8_t>(40 + 60 * progress)));
            ring.setOutlineColor(sf::Color(120, 40, 200, static_cast<uint8_t>(100 + 155 * progress)));
            ring.setOutlineThickness(2.f + progress * 2.f);
            m_renderer.getWindow().draw(ring);

            // Spinning inward particles
            for (int i = 0; i < 8; i++) {
                float angle = proj.implodeTimer * 6.0f + i * 0.785f;
                float dist = ringR * (1.0f - std::fmod(proj.implodeTimer * 2.0f + i * 0.125f, 1.0f));
                float px = sp.x + std::cos(angle) * dist;
                float py = sp.y + std::sin(angle) * dist;
                float sz = 2.0f + progress;
                sf::CircleShape dot(sz); dot.setOrigin({sz, sz});
                dot.setPosition({px, py});
                dot.setFillColor(sf::Color(180, 80, 255, static_cast<uint8_t>(200 * (dist / ringR))));
                m_renderer.getWindow().draw(dot);
            }

            // Core glow (grows as it charges)
            float coreR = 4.f + progress * 6.f;
            sf::CircleShape core(coreR); core.setOrigin({coreR, coreR}); core.setPosition(sp);
            core.setFillColor(sf::Color(80, 0, 160, static_cast<uint8_t>(150 + 105 * progress)));
            m_renderer.getWindow().draw(core);
            continue;
        }

        if (proj.isStuck || proj.isMine) {
            // Mine: red pulsing with danger triangles, fast beep when triggered
            if (proj.isMine) {
                float pulse;
                if (proj.mineTriggered) {
                    // Fast double-beep: two quick flashes in 0.25s
                    float beepPhase = (0.25f - proj.mineDetonateTimer) / 0.25f;
                    // Two beeps: one at 0-40%, one at 50-90%
                    bool beepOn = (beepPhase < 0.4f && std::fmod(beepPhase, 0.2f) < 0.1f) ||
                                  (beepPhase >= 0.5f && beepPhase < 0.9f && std::fmod(beepPhase - 0.5f, 0.2f) < 0.1f);
                    pulse = beepOn ? 1.0f : 0.2f;
                } else {
                    pulse = std::sin(t * 4.f) * 0.3f + 0.7f;
                }
                sf::CircleShape mine(6.f); mine.setOrigin({6.f,6.f});
                mine.setPosition(sp);
                mine.setFillColor(sf::Color(180,40,40, static_cast<uint8_t>(200*pulse)));
                mine.setOutlineColor(sf::Color(255,60,60, static_cast<uint8_t>(255*pulse)));
                mine.setOutlineThickness(2.f);
                m_renderer.getWindow().draw(mine);
                // Danger triangles (spin faster when triggered)
                float spinSpeed = proj.mineTriggered ? 600.f : 120.f;
                for (int i = 0; i < 3; i++) {
                    float a = t * spinSpeed + i * 120.f;
                    float rad = a * 3.14159f / 180.f;
                    sf::CircleShape tri(2.f, 3);
                    tri.setOrigin({2.f, 2.f});
                    tri.setPosition({sp.x + std::cos(rad)*9.f, sp.y + std::sin(rad)*9.f});
                    tri.setFillColor(sf::Color(255, 80, 80, static_cast<uint8_t>(180*pulse)));
                    m_renderer.getWindow().draw(tri);
                }
                continue;
            }
            // Sticky stuck — color by charge type
            float pulse = std::sin(t * 8.f) * 0.3f + 0.7f;
            sf::Color cc(60,180,60), oc(100,255,80);
            if (proj.weapon.grenadeCharge == GrenadeCharge::Freeze) { cc={100,180,255}; oc={200,240,255}; }
            else if (proj.weapon.grenadeCharge == GrenadeCharge::Time) { cc={60,40,200}; oc={120,180,255}; }
            else if (proj.weapon.grenadeCharge == GrenadeCharge::Lava) { cc={255,100,20}; oc={255,200,50}; }
            else if (proj.weapon.grenadeCharge == GrenadeCharge::Nuclear) { cc={50,255,0}; oc={255,255,0}; }
            else if (proj.weapon.grenadeCharge == GrenadeCharge::Frag) { cc={180,180,180}; oc={255,255,255}; }
            else if (proj.weapon.grenadeCharge == GrenadeCharge::Implode) { cc={50,0,120}; oc={120,40,200}; }

            sf::CircleShape blob(7.f); blob.setOrigin({7.f,7.f});
            blob.setPosition(sp);
            blob.setFillColor(sf::Color(cc.r,cc.g,cc.b, static_cast<uint8_t>(200*pulse)));
            blob.setOutlineColor(sf::Color(oc.r,oc.g,oc.b, static_cast<uint8_t>(255*pulse)));
            blob.setOutlineThickness(2.f);
            m_renderer.getWindow().draw(blob);
            for (int i = 0; i < 6; i++) {
                float a = static_cast<float>(i)/6.f * 6.28318f;
                sf::VertexArray leg(sf::PrimitiveType::Lines, 2);
                leg[0] = sf::Vertex{sp, sf::Color(oc.r,oc.g,oc.b,180)};
                leg[1] = sf::Vertex{{sp.x + std::cos(a)*10.f, sp.y + std::sin(a)*6.f},
                                    sf::Color(oc.r,oc.g,oc.b,60)};
                m_renderer.getWindow().draw(leg);
            }
            continue;
        }

        // Thrown weapon — spinning weapon rectangle with name
        if (proj.isThrownWeapon) {
            float spinAngle = t * 720.0f;  // fast spin
            sf::RectangleShape wepRect({16.f, 8.f});
            wepRect.setOrigin({8.f, 4.f});
            wepRect.setPosition(sp);
            wepRect.setRotation(sf::degrees(spinAngle));
            wepRect.setFillColor(sf::Color(200, 180, 100));
            wepRect.setOutlineColor(sf::Color(255, 220, 120));
            wepRect.setOutlineThickness(1.5f);
            m_renderer.getWindow().draw(wepRect);
            // Handle/grip
            sf::RectangleShape grip({6.f, 4.f});
            grip.setOrigin({3.f, 2.f});
            grip.setPosition({sp.x + std::cos(spinAngle * 3.14159f/180.f) * 5.f,
                              sp.y + std::sin(spinAngle * 3.14159f/180.f) * 5.f});
            grip.setRotation(sf::degrees(spinAngle));
            grip.setFillColor(sf::Color(120, 80, 40));
            m_renderer.getWindow().draw(grip);
            // Trail
            if (spd > 2.f) {
                for (int i = 1; i <= 4; i++) {
                    float trail = static_cast<float>(i);
                    float tx2 = sp.x - (vel.x / PPM) * trail * 0.04f * PPM;
                    float ty2 = sp.y + (vel.y / PPM) * trail * 0.04f * PPM;
                    uint8_t alpha = static_cast<uint8_t>(120 - i * 25);
                    sf::CircleShape dot(2.f); dot.setOrigin({2.f,2.f});
                    dot.setPosition({tx2, ty2});
                    dot.setFillColor(sf::Color(255, 220, 120, alpha));
                    m_renderer.getWindow().draw(dot);
                }
            }
            continue;
        }

        if (proj.weapon.grenadeCharge == GrenadeCharge::Frag && proj.isFragChild) {
            // Frag child — small spinning triangle shard
            sf::CircleShape shard(3.f, 3);
            shard.setOrigin({3.f, 3.f}); shard.setPosition(sp);
            shard.setRotation(sf::degrees(t * 720.f));
            shard.setFillColor(sf::Color(200,200,200,230));
            shard.setOutlineColor(sf::Color(255,150,50,180));
            shard.setOutlineThickness(1.f);
            m_renderer.getWindow().draw(shard);
        } else if (proj.weapon.isGrenadeLauncher || (proj.weapon.type == WeaponType::Explosive &&
                   proj.weapon.grenadeCharge != GrenadeCharge::Normal)) {
            // === GRENADE IN-AIR RENDERING (casing shape + charge color) ===
            sf::Color chgCol(200, 180, 80); // normal = olive/gold
            sf::Color chgGlow(255, 220, 100);
            auto charge = proj.weapon.grenadeCharge;
            if (charge == GrenadeCharge::Freeze)   { chgCol = {100, 180, 255}; chgGlow = {180, 220, 255}; }
            else if (charge == GrenadeCharge::Time) { chgCol = {140, 80, 220}; chgGlow = {180, 130, 255}; }
            else if (charge == GrenadeCharge::Lava) { chgCol = {255, 100, 20}; chgGlow = {255, 180, 50}; }
            else if (charge == GrenadeCharge::Nuclear) { chgCol = {80, 255, 40}; chgGlow = {200, 255, 80}; }
            else if (charge == GrenadeCharge::Frag) { chgCol = {180, 180, 180}; chgGlow = {230, 230, 230}; }
            else if (charge == GrenadeCharge::Implode) { chgCol = {50, 0, 120}; chgGlow = {120, 40, 200}; }

            auto casing = proj.weapon.grenadeCasing;
            float spin = t * 360.f;

            if (casing == GrenadeCasing::Ballistic) {
                // BALLISTIC: sleek pointed dart shape with speed lines
                float angle = std::atan2(-vel.y, vel.x) * 180.f / 3.14159f;
                sf::ConvexShape dart(4);
                dart.setPoint(0, {12.f, 0.f});
                dart.setPoint(1, {0.f, 3.5f});
                dart.setPoint(2, {-4.f, 0.f});
                dart.setPoint(3, {0.f, -3.5f});
                dart.setOrigin({4.f, 0.f});
                dart.setPosition(sp);
                dart.setRotation(sf::degrees(angle));
                dart.setFillColor(chgCol);
                dart.setOutlineColor(chgGlow); dart.setOutlineThickness(1.f);
                m_renderer.getWindow().draw(dart);
                if (spd > 2.f) {
                    float rad = angle * 3.14159f / 180.f;
                    for (int i = 1; i <= 3; i++) {
                        float offset = static_cast<float>(i) * 5.f;
                        float lx = sp.x - std::cos(rad) * (8.f + offset);
                        float ly = sp.y + std::sin(rad) * (8.f + offset);
                        float spread = static_cast<float>(i) * 2.f;
                        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                        line[0] = sf::Vertex{{lx, ly - spread}, sf::Color(chgGlow.r, chgGlow.g, chgGlow.b, static_cast<uint8_t>(140 - i*40))};
                        line[1] = sf::Vertex{{lx - 6.f, ly - spread}, sf::Color(chgGlow.r, chgGlow.g, chgGlow.b, 0)};
                        m_renderer.getWindow().draw(line);
                        sf::VertexArray line2(sf::PrimitiveType::Lines, 2);
                        line2[0] = sf::Vertex{{lx, ly + spread}, sf::Color(chgGlow.r, chgGlow.g, chgGlow.b, static_cast<uint8_t>(140 - i*40))};
                        line2[1] = sf::Vertex{{lx - 6.f, ly + spread}, sf::Color(chgGlow.r, chgGlow.g, chgGlow.b, 0)};
                        m_renderer.getWindow().draw(line2);
                    }
                }
            } else if (casing == GrenadeCasing::Sticky) {
                // STICKY: wobbly goo blob with dripping tendrils
                float wobble = std::sin(t * 12.f) * 1.5f;
                sf::CircleShape blob(6.f + wobble);
                blob.setOrigin({6.f + wobble, 6.f + wobble}); blob.setPosition(sp);
                blob.setFillColor(sf::Color(
                    static_cast<uint8_t>(chgCol.r/2 + 30),
                    static_cast<uint8_t>(chgCol.g/2 + 80),
                    static_cast<uint8_t>(chgCol.b/2), 220));
                blob.setOutlineColor(sf::Color(
                    static_cast<uint8_t>(chgCol.r/2),
                    static_cast<uint8_t>(chgCol.g/2 + 120),
                    static_cast<uint8_t>(chgCol.b/2), 180));
                blob.setOutlineThickness(2.f);
                m_renderer.getWindow().draw(blob);
                for (int i = 0; i < 3; i++) {
                    float da = static_cast<float>(i) * 2.09f + t * 3.f;
                    float dripLen = 4.f + std::sin(t * 8.f + i * 1.5f) * 3.f;
                    float dx2 = std::cos(da) * 3.f;
                    sf::VertexArray drip(sf::PrimitiveType::Lines, 2);
                    drip[0] = sf::Vertex{{sp.x + dx2, sp.y + 4.f}, sf::Color(
                        static_cast<uint8_t>(chgCol.r/2),
                        static_cast<uint8_t>(chgCol.g/2 + 100),
                        static_cast<uint8_t>(chgCol.b/2), 180)};
                    drip[1] = sf::Vertex{{sp.x + dx2, sp.y + 4.f + dripLen}, sf::Color(
                        static_cast<uint8_t>(chgCol.r/2),
                        static_cast<uint8_t>(chgCol.g/2 + 100),
                        static_cast<uint8_t>(chgCol.b/2), 40)};
                    m_renderer.getWindow().draw(drip);
                }
            } else if (casing == GrenadeCasing::Mine) {
                // MINE: boxy shape with blinking red light
                float blink = std::fmod(t * 3.f, 1.f) < 0.5f ? 1.f : 0.3f;
                sf::RectangleShape box({12.f, 10.f});
                box.setOrigin({6.f, 5.f}); box.setPosition(sp);
                box.setRotation(sf::degrees(spin * 0.5f));
                box.setFillColor(sf::Color(
                    static_cast<uint8_t>(chgCol.r * 3/4),
                    static_cast<uint8_t>(chgCol.g * 3/4),
                    static_cast<uint8_t>(chgCol.b * 3/4), 230));
                box.setOutlineColor(sf::Color(80, 80, 80, 200));
                box.setOutlineThickness(1.5f);
                m_renderer.getWindow().draw(box);
                sf::CircleShape led(2.f);
                led.setOrigin({2.f, 2.f}); led.setPosition(sp);
                led.setFillColor(sf::Color(255, 30, 30, static_cast<uint8_t>(255 * blink)));
                m_renderer.getWindow().draw(led);
            } else if (casing == GrenadeCasing::Bounce) {
                // BOUNCE: rubber ball shape with squash/stretch and bounce ring
                float squash = 1.0f + std::sin(t * 15.f) * 0.15f;
                sf::CircleShape ball(6.f);
                ball.setOrigin({6.f, 6.f}); ball.setPosition(sp);
                ball.setScale({1.0f / squash, squash});
                ball.setRotation(sf::degrees(spin * 2.f));
                ball.setFillColor(chgCol);
                ball.setOutlineColor(sf::Color(255, 255, 255, 160));
                ball.setOutlineThickness(2.f);
                m_renderer.getWindow().draw(ball);
                // Shine highlight
                sf::CircleShape shine(2.f);
                shine.setOrigin({2.f, 2.f});
                shine.setPosition({sp.x - 2.f, sp.y - 3.f});
                shine.setFillColor(sf::Color(255, 255, 255, 120));
                m_renderer.getWindow().draw(shine);
                // Bounce arc trail (dotted)
                if (spd > 2.f) {
                    for (int i = 1; i <= 5; i++) {
                        float trail = static_cast<float>(i);
                        float tx2 = sp.x - (vel.x / PPM) * trail * 0.035f * PPM;
                        float ty2 = sp.y + (vel.y / PPM) * trail * 0.035f * PPM;
                        float sz2 = 2.5f - trail * 0.4f;
                        if (sz2 < 0.5f) continue;
                        sf::CircleShape dot(sz2);
                        dot.setOrigin({sz2, sz2}); dot.setPosition({tx2, ty2});
                        dot.setFillColor(sf::Color(chgGlow.r, chgGlow.g, chgGlow.b, static_cast<uint8_t>(120 - trail*22)));
                        m_renderer.getWindow().draw(dot);
                    }
                }
            } else {
                // NORMAL: classic round grenade with lever
                sf::CircleShape body(6.f);
                body.setOrigin({6.f, 6.f}); body.setPosition(sp);
                body.setFillColor(chgCol);
                body.setOutlineColor(sf::Color(
                    static_cast<uint8_t>(chgCol.r*3/4),
                    static_cast<uint8_t>(chgCol.g*3/4),
                    static_cast<uint8_t>(chgCol.b*3/4), 255));
                body.setOutlineThickness(1.5f);
                m_renderer.getWindow().draw(body);
                float leverAng = spin * 3.14159f / 180.f;
                sf::VertexArray lever(sf::PrimitiveType::Lines, 2);
                lever[0] = sf::Vertex{{sp.x, sp.y - 5.f}, sf::Color(120, 120, 120, 200)};
                lever[1] = sf::Vertex{{sp.x + std::cos(leverAng) * 6.f, sp.y - 5.f + std::sin(leverAng) * 4.f}, sf::Color(160, 160, 160, 120)};
                m_renderer.getWindow().draw(lever);
                sf::CircleShape nub(2.f);
                nub.setOrigin({2.f, 2.f});
                nub.setPosition({sp.x, sp.y - 6.f});
                nub.setFillColor(sf::Color(100, 100, 100, 200));
                m_renderer.getWindow().draw(nub);
            }

            // Charge glow trail
            if (spd > 1.5f) {
                for (int i = 1; i <= 3; i++) {
                    float trail = static_cast<float>(i);
                    float tx = sp.x - (vel.x / PPM) * trail * 0.04f * PPM;
                    float ty = sp.y + (vel.y / PPM) * trail * 0.04f * PPM;
                    float sz = 3.f - trail * 0.7f;
                    if (sz < 0.5f) continue;
                    sf::CircleShape glow(sz); glow.setOrigin({sz,sz}); glow.setPosition({tx,ty});
                    glow.setFillColor(sf::Color(chgGlow.r, chgGlow.g, chgGlow.b, static_cast<uint8_t>(100 - trail*30)));
                    m_renderer.getWindow().draw(glow);
                }
            }
        } else if (wn == "Sticky Bomb" || proj.weapon.grenadeCasing == GrenadeCasing::Sticky) {
            // Gooey blob with wobble
            float wobble = std::sin(t*10.f)*1.5f + 5.f;
            sf::CircleShape blob(wobble); blob.setOrigin({wobble,wobble}); blob.setPosition(sp);
            blob.setFillColor(sf::Color(80,220,60,230));
            m_renderer.getWindow().draw(blob);
            // Goo trail
            if (spd > 1.f) {
                for (int i = 1; i <= 3; i++) {
                    float trail = static_cast<float>(i);
                    float tx = sp.x - (vel.x / PPM) * trail * 0.05f * PPM;
                    float ty = sp.y + (vel.y / PPM) * trail * 0.05f * PPM;
                    float ts = (wobble - trail * 1.f);
                    if (ts < 1.f) continue;
                    sf::CircleShape td(ts); td.setOrigin({ts,ts}); td.setPosition({tx,ty});
                    td.setFillColor(sf::Color(80,220,60, static_cast<uint8_t>(120 - trail*35)));
                    m_renderer.getWindow().draw(td);
                }
            }
        } else if (proj.isPoison) {
            // Poison spit
            float wobble = std::sin(t*8.f)*1.f + 4.f;
            sf::CircleShape c(wobble); c.setOrigin({wobble,wobble}); c.setPosition(sp);
            c.setFillColor(sf::Color(0,200,0,220));
            m_renderer.getWindow().draw(c);
        } else if (wn == "Freeze Grenade") {
            // Icy blue glowing orb
            float pulse = std::sin(t * 5.f) * 0.3f + 0.7f;
            sf::CircleShape c(6.f); c.setOrigin({6.f,6.f}); c.setPosition(sp);
            c.setFillColor(sf::Color(100, 180, 255, static_cast<uint8_t>(220*pulse)));
            c.setOutlineColor(sf::Color(200, 240, 255, static_cast<uint8_t>(255*pulse)));
            c.setOutlineThickness(2.f);
            m_renderer.getWindow().draw(c);
            // Ice crystal details
            for (int i = 0; i < 4; i++) {
                float a2 = t * 90.f + i * 90.f;
                float rad = a2 * 3.14159f / 180.f;
                sf::VertexArray spike(sf::PrimitiveType::Lines, 2);
                spike[0] = sf::Vertex{sp, sf::Color(200,240,255,200)};
                spike[1] = sf::Vertex{{sp.x + std::cos(rad)*5.f, sp.y + std::sin(rad)*5.f},
                                       sf::Color(200,240,255,80)};
                m_renderer.getWindow().draw(spike);
            }
        } else if (wn == "Freeze Ray") {
            // Ice shard
            sf::RectangleShape shard({8.f, 3.f});
            shard.setOrigin({4.f, 1.5f});
            shard.setPosition(sp);
            float angle = (spd > 0.1f) ? std::atan2(-vel.y, vel.x) * 57.3f : 0.f;
            shard.setRotation(sf::degrees(angle));
            shard.setFillColor(sf::Color(160,220,255,240));
            shard.setOutlineColor(sf::Color(200,240,255,180)); shard.setOutlineThickness(1.f);
            m_renderer.getWindow().draw(shard);
            // Ice sparkle trail
            sf::CircleShape sparkle(2.f); sparkle.setOrigin({2.f,2.f});
            sparkle.setPosition({sp.x - vel.x*0.03f*PPM, sp.y + vel.y*0.03f*PPM});
            sparkle.setFillColor(sf::Color(180,230,255, static_cast<uint8_t>(100 + std::sin(t*10.f)*60)));
            m_renderer.getWindow().draw(sparkle);
        } else if (wn == "Lightning Staff") {
            // Zigzag bolt
            int segs = 5;
            float bolt_t = t;
            for (int i = 0; i < segs; i++) {
                float f0 = static_cast<float>(i)/segs;
                float f1 = static_cast<float>(i+1)/segs;
                float ox0 = std::sin(bolt_t*20.f + i*1.3f) * 4.f;
                float ox1 = std::sin(bolt_t*20.f + (i+1)*1.3f) * 4.f;
                sf::VertexArray seg(sf::PrimitiveType::Lines, 2);
                seg[0] = sf::Vertex{{sp.x + vel.x*f0*0.05f*PPM + ox0,
                                     sp.y - vel.y*f0*0.05f*PPM},
                                    sf::Color(200,180,255,220)};
                seg[1] = sf::Vertex{{sp.x + vel.x*f1*0.05f*PPM + ox1,
                                     sp.y - vel.y*f1*0.05f*PPM},
                                    sf::Color(220,200,255,180)};
                m_renderer.getWindow().draw(seg);
            }
            sf::CircleShape head(4.f); head.setOrigin({4.f,4.f}); head.setPosition(sp);
            head.setFillColor(sf::Color(200,180,255,240));
            m_renderer.getWindow().draw(head);
        } else if (wn == "Shrink Ray") {
            // Purple spiral beam
            for (int i = 0; i < 4; i++) {
                float phase = t*8.f + i*1.5708f;
                float ox = std::cos(phase)*3.f;
                float oy = std::sin(phase)*3.f;
                sf::CircleShape dot(2.f); dot.setOrigin({2.f,2.f});
                dot.setPosition({sp.x+ox, sp.y+oy});
                dot.setFillColor(sf::Color(220,100,220,200));
                m_renderer.getWindow().draw(dot);
            }
        } else if (wn == "Time Gun") {
            // Clock orb with rings
            sf::CircleShape orb(5.f); orb.setOrigin({5.f,5.f}); orb.setPosition(sp);
            orb.setFillColor(sf::Color(200,180,60,220));
            orb.setOutlineColor(sf::Color(255,240,100,180)); orb.setOutlineThickness(1.5f);
            m_renderer.getWindow().draw(orb);
            sf::CircleShape ring(8.f); ring.setOrigin({8.f,8.f}); ring.setPosition(sp);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(255,240,100,80)); ring.setOutlineThickness(1.f);
            m_renderer.getWindow().draw(ring);
        } else if (wn == "Healing Pulse Gun") {
            // Green glowing orb with cross
            float pulse = std::sin(t*5.f)*0.3f + 0.7f;
            sf::CircleShape orb(5.f); orb.setOrigin({5.f,5.f}); orb.setPosition(sp);
            orb.setFillColor(sf::Color(60,220,100, static_cast<uint8_t>(200*pulse)));
            m_renderer.getWindow().draw(orb);
            sf::VertexArray cross(sf::PrimitiveType::Lines, 4);
            cross[0] = sf::Vertex{{sp.x-5.f,sp.y}, sf::Color(100,255,120,200)};
            cross[1] = sf::Vertex{{sp.x+5.f,sp.y}, sf::Color(100,255,120,200)};
            cross[2] = sf::Vertex{{sp.x,sp.y-5.f}, sf::Color(100,255,120,200)};
            cross[3] = sf::Vertex{{sp.x,sp.y+5.f}, sf::Color(100,255,120,200)};
            m_renderer.getWindow().draw(cross);
        } else if (wn == "Portal Gun") {
            // Blue/orange swirling orb
            float phase = (m_roundTimer > 0) ? std::fmod(m_roundTimer*2.f, 6.28318f) : 0.f;
            sf::Color portalC = (proj.ownerIndex % 2 == 0)
                ? sf::Color(80,160,255,240) : sf::Color(255,140,40,240);
            sf::CircleShape orb(5.f); orb.setOrigin({5.f,5.f}); orb.setPosition(sp);
            orb.setFillColor(portalC);
            orb.setOutlineColor(sf::Color::White); orb.setOutlineThickness(1.f);
            m_renderer.getWindow().draw(orb);
        } else if (wn == "Mind Control Staff") {
            // Pulsing purple swirl
            for (int i = 0; i < 3; i++) {
                float a = t*6.f + i*2.094f;
                float r = 4.f;
                sf::CircleShape dot(2.f); dot.setOrigin({2.f,2.f});
                dot.setPosition({sp.x + std::cos(a)*r, sp.y + std::sin(a)*r});
                dot.setFillColor(sf::Color(200,100,255,220));
                m_renderer.getWindow().draw(dot);
            }
        } else if (wn == "Swap Staff") {
            // Split color orb
            for (int i = 0; i < 2; i++) {
                float off = (i == 0) ? -3.f : 3.f;
                sf::CircleShape half(4.f); half.setOrigin({4.f,4.f});
                half.setPosition({sp.x+off, sp.y});
                half.setFillColor(i == 0 ? sf::Color(80,160,255,200) : sf::Color(255,100,80,200));
                m_renderer.getWindow().draw(half);
            }
        } else if (wn == "Boomerang") {
            // Spinning boomerang shape
            float spin = t * 10.f;
            sf::ConvexShape boom(3);
            boom.setPoint(0, {0.f, -8.f});
            boom.setPoint(1, {-5.f, 6.f});
            boom.setPoint(2, { 5.f, 6.f});
            boom.setOrigin({0.f,0.f}); boom.setPosition(sp);
            boom.setRotation(sf::degrees(spin * 57.3f));
            boom.setFillColor(sf::Color(200,140,60,240));
            boom.setOutlineColor(sf::Color(240,180,80,180)); boom.setOutlineThickness(1.f);
            m_renderer.getWindow().draw(boom);
        } else if (wn == "Bouncy Ball Launcher") {
            // Bright bouncy ball with trail
            sf::CircleShape ball(5.f); ball.setOrigin({5.f,5.f}); ball.setPosition(sp);
            ball.setFillColor(sf::Color(255,80,80,240));
            ball.setOutlineColor(sf::Color(255,160,160,180)); ball.setOutlineThickness(1.5f);
            m_renderer.getWindow().draw(ball);
            // Trail dots
            for (int i = 1; i <= 4; i++) {
                float f = static_cast<float>(i);
                float tx = sp.x - vel.x*f*0.03f*PPM;
                float ty = sp.y + vel.y*f*0.03f*PPM;
                float ts = 5.f - f;
                if (ts < 1.f) continue;
                sf::CircleShape trail(ts); trail.setOrigin({ts,ts}); trail.setPosition({tx,ty});
                trail.setFillColor(sf::Color(255,80,80, static_cast<uint8_t>(80 - i*15)));
                m_renderer.getWindow().draw(trail);
            }
        } else if (wn == "Freeze Grenade") {
            // Icy blue orb with frost particles
            float pulse = std::sin(t * 5.f) * 0.3f + 0.7f;
            sf::CircleShape c(6.f); c.setOrigin({6.f,6.f}); c.setPosition(sp);
            c.setFillColor(sf::Color(100, 180, 255, 230));
            c.setOutlineColor(sf::Color(200, 230, 255, static_cast<uint8_t>(200*pulse)));
            c.setOutlineThickness(2.f);
            m_renderer.getWindow().draw(c);
            // Frost sparkles
            for (int i = 0; i < 4; i++) {
                float a2 = t * 3.f + i * 1.57f;
                float fx = sp.x + std::cos(a2) * 9.f;
                float fy = sp.y + std::sin(a2) * 9.f;
                sf::CircleShape spark(1.5f); spark.setOrigin({1.5f,1.5f});
                spark.setPosition({fx, fy});
                spark.setFillColor(sf::Color(200,230,255, static_cast<uint8_t>(180*pulse)));
                m_renderer.getWindow().draw(spark);
            }
        } else if (wn == "Time Grenade") {
            // Glowing blue-purple clock-like orb
            float pulse = std::sin(t * 6.f) * 0.3f + 0.7f;
            sf::CircleShape c(6.f); c.setOrigin({6.f,6.f}); c.setPosition(sp);
            c.setFillColor(sf::Color(60, 40, static_cast<uint8_t>(200*pulse), 230));
            c.setOutlineColor(sf::Color(120, 180, 255, static_cast<uint8_t>(200*pulse)));
            c.setOutlineThickness(2.f);
            m_renderer.getWindow().draw(c);
            // Clock hands
            float handAngle = t * 360.f;
            sf::VertexArray hand(sf::PrimitiveType::Lines, 2);
            hand[0] = sf::Vertex{sp, sf::Color(200,220,255)};
            hand[1] = sf::Vertex{{sp.x + std::cos(handAngle*3.14159f/180.f)*4.f,
                                   sp.y + std::sin(handAngle*3.14159f/180.f)*4.f}, sf::Color(200,220,255)};
            m_renderer.getWindow().draw(hand);
        } else if (wn == "Bounce Laser") {
            // Bright laser bolt with trail
            float angle2 = std::atan2(-vel.y, vel.x) * 180.f / 3.14159f;
            sf::RectangleShape beam({12.f, 3.f});
            beam.setOrigin({6.f, 1.5f});
            beam.setPosition(sp);
            beam.setRotation(sf::degrees(angle2));
            beam.setFillColor(sf::Color(0, 255, 200));
            m_renderer.getWindow().draw(beam);
            sf::CircleShape glow(5.f); glow.setOrigin({5.f,5.f}); glow.setPosition(sp);
            glow.setFillColor(sf::Color(0, 255, 200, 60));
            m_renderer.getWindow().draw(glow);
            // Trail
            for (int i = 1; i <= 5; i++) {
                float trail = static_cast<float>(i);
                float tx3 = sp.x - (vel.x/PPM)*trail*0.03f*PPM;
                float ty3 = sp.y + (vel.y/PPM)*trail*0.03f*PPM;
                uint8_t alpha = static_cast<uint8_t>(150 - i*28);
                sf::CircleShape dot(1.5f); dot.setOrigin({1.5f,1.5f});
                dot.setPosition({tx3, ty3});
                dot.setFillColor(sf::Color(0, 255, 200, alpha));
                m_renderer.getWindow().draw(dot);
            }
        } else {
            // Regular bullets / generic
            float sz = (proj.weapon.pelletCount > 1) ? 2.f : 4.f;
            sf::CircleShape c(sz); c.setOrigin({sz,sz}); c.setPosition(sp);
            c.setFillColor(proj.weapon.pelletCount > 1
                ? sf::Color(255,180,80) : sf::Color::Yellow);
            m_renderer.getWindow().draw(c);
        }
    }

    // Draw area effects
    float renderTime = m_roundTimer; // use as a time source
    for (const auto& bh : m_blackHoles) {
        if (!bh.alive) continue;
        sf::Vector2f bhp = {SCREEN_CX + bh.x * PPM, SCREEN_CY - bh.y * PPM};
        float spin = (m_rulesEngine.getRules().roundTimeSeconds - renderTime) * 3.f;
        // Event horizon
        float r = bh.radius * PPM * 0.18f;
        sf::CircleShape core(r); core.setOrigin({r,r}); core.setPosition(bhp);
        core.setFillColor(sf::Color(5,0,15));
        core.setOutlineColor(sf::Color(120,0,200,180)); core.setOutlineThickness(2.f);
        m_renderer.getWindow().draw(core);
        // Accretion rings
        for (int ring = 0; ring < 3; ring++) {
            float rr = r + ring * r * 0.8f;
            sf::CircleShape acc(rr); acc.setOrigin({rr,rr}); acc.setPosition(bhp);
            acc.setFillColor(sf::Color::Transparent);
            uint8_t alpha = static_cast<uint8_t>(120 - ring * 35);
            acc.setOutlineColor(sf::Color(160 - ring*40, 0, 255 - ring*60, alpha));
            acc.setOutlineThickness(2.f - ring * 0.5f);
            acc.setRotation(sf::degrees(spin * (ring+1) * 30.f));
            m_renderer.getWindow().draw(acc);
        }
        // Swirling particles
        for (int i = 0; i < 8; i++) {
            float a = spin + i * 0.785398f;
            float dist = r * 2.5f;
            sf::CircleShape p(2.f); p.setOrigin({2.f,2.f});
            p.setPosition({bhp.x + std::cos(a)*dist, bhp.y + std::sin(a)*dist});
            p.setFillColor(sf::Color(160,0,220,180));
            m_renderer.getWindow().draw(p);
        }
    }

    for (const auto& tornado : m_tornadoes) {
        if (!tornado.alive) continue;
        sf::Vector2f tp = {SCREEN_CX + tornado.x * PPM, SCREEN_CY - tornado.y * PPM};
        float spin = (m_rulesEngine.getRules().roundTimeSeconds - renderTime) * 5.f;
        float progress = tornado.timer / tornado.duration;
        float maxR = tornado.radius * PPM;
        // Draw funnel layers (wide at top, narrow at base)
        for (int layer = 0; layer < 6; layer++) {
            float f = static_cast<float>(layer) / 5.f;
            float layerR = maxR * (0.2f + f * 0.8f);
            float layerY = tp.y - f * 40.f;
            sf::CircleShape ring(layerR); ring.setOrigin({layerR, layerR});
            ring.setPosition({tp.x, layerY});
            ring.setFillColor(sf::Color::Transparent);
            uint8_t alpha = static_cast<uint8_t>(100 - layer*12);
            ring.setOutlineColor(sf::Color(180,220,255, alpha));
            ring.setOutlineThickness(1.5f);
            ring.setRotation(sf::degrees(spin * (6 - layer) * 20.f));
            m_renderer.getWindow().draw(ring);
        }
        // Flying debris
        for (int i = 0; i < 6; i++) {
            float a = spin * 2.f + i * 1.047f;
            float debrisR = maxR * 0.7f;
            sf::RectangleShape debris({4.f, 2.f});
            debris.setOrigin({2.f,1.f});
            debris.setPosition({tp.x + std::cos(a)*debrisR, tp.y - 20.f + std::sin(a)*10.f});
            debris.setRotation(sf::degrees(a*57.3f));
            debris.setFillColor(sf::Color(160,180,200,160));
            m_renderer.getWindow().draw(debris);
        }
    }

    for (const auto& zone : m_timeSlowZones) {
        if (!zone.alive) continue;
        sf::Vector2f zp = {SCREEN_CX + zone.x * PPM, SCREEN_CY - zone.y * PPM};
        float fade = 1.f - (zone.timer / zone.duration);
        float r = zone.radius * PPM;
        // Blue tinted area
        sf::CircleShape area(r); area.setOrigin({r,r}); area.setPosition(zp);
        area.setFillColor(sf::Color(80,120,200, static_cast<uint8_t>(30 * fade)));
        area.setOutlineColor(sf::Color(140,180,255, static_cast<uint8_t>(120 * fade)));
        area.setOutlineThickness(2.f);
        m_renderer.getWindow().draw(area);
        // Clock particles
        float spin = (m_rulesEngine.getRules().roundTimeSeconds - renderTime) * 0.5f;
        for (int i = 0; i < 6; i++) {
            float a = spin + i * 1.047f;
            sf::CircleShape dot(2.5f); dot.setOrigin({2.5f,2.5f});
            dot.setPosition({zp.x + std::cos(a)*r*0.7f, zp.y + std::sin(a)*r*0.7f});
            dot.setFillColor(sf::Color(180,210,255, static_cast<uint8_t>(140 * fade)));
            m_renderer.getWindow().draw(dot);
        }
    }

    for (const auto& portal : m_portals) {
        if (!portal.alive) continue;
        sf::Vector2f pp = {SCREEN_CX + portal.x * PPM, SCREEN_CY - portal.y * PPM};
        bool isBlue = (portal.pairIndex % 2 == 0);
        sf::Color portalCol = isBlue ? sf::Color(80,160,255) : sf::Color(255,140,40);
        float spin = (m_rulesEngine.getRules().roundTimeSeconds - renderTime) * 2.f;
        // Oval portal shape
        sf::CircleShape oval(18.f, 30); oval.setOrigin({18.f,18.f}); oval.setPosition(pp);
        oval.setFillColor(sf::Color(portalCol.r, portalCol.g, portalCol.b, 60));
        oval.setOutlineColor(sf::Color(portalCol.r, portalCol.g, portalCol.b, 220));
        oval.setOutlineThickness(3.f);
        oval.setScale({0.5f, 1.f}); // squish to oval
        m_renderer.getWindow().draw(oval);
        // Swirling inner
        for (int i = 0; i < 5; i++) {
            float a = spin * 3.f + i * 1.2566f;
            float r2 = 6.f;
            sf::CircleShape dot(2.f); dot.setOrigin({2.f,2.f});
            dot.setPosition({pp.x + std::cos(a)*r2*0.5f, pp.y + std::sin(a)*r2});
            dot.setFillColor(sf::Color(portalCol.r, portalCol.g, portalCol.b, 200));
            m_renderer.getWindow().draw(dot);
        }
    }

    for (const auto& hz : m_healZones) {
        if (!hz.alive) continue;
        sf::Vector2f hp = {SCREEN_CX + hz.x * PPM, SCREEN_CY - hz.y * PPM};
        float fade = 1.f - (hz.timer / hz.duration);
        float r = hz.radius * PPM;
        float pulse = std::sin((m_rulesEngine.getRules().roundTimeSeconds - renderTime) * 3.f) * 0.2f + 0.8f;
        sf::CircleShape area(r); area.setOrigin({r,r}); area.setPosition(hp);
        area.setFillColor(sf::Color(60,180,80, static_cast<uint8_t>(25*pulse*fade)));
        area.setOutlineColor(sf::Color(100,255,120, static_cast<uint8_t>(120*pulse*fade)));
        area.setOutlineThickness(2.f);
        m_renderer.getWindow().draw(area);
        // Rising cross particles
        float spin = (m_rulesEngine.getRules().roundTimeSeconds - renderTime);
        for (int i = 0; i < 3; i++) {
            float phase = std::fmod(spin * 0.7f + i * 0.33f, 1.f);
            float px = hp.x + (i - 1.f) * r * 0.4f;
            float py = hp.y + r * 0.3f - phase * r * 0.8f;
            float alpha = (phase < 0.7f) ? 1.f : (1.f - phase) / 0.3f;
            sf::VertexArray cross(sf::PrimitiveType::Lines, 4);
            cross[0] = sf::Vertex{{px-4.f,py}, sf::Color(80,255,100, static_cast<uint8_t>(180*alpha*fade))};
            cross[1] = sf::Vertex{{px+4.f,py}, sf::Color(80,255,100, static_cast<uint8_t>(180*alpha*fade))};
            cross[2] = sf::Vertex{{px,py-4.f}, sf::Color(80,255,100, static_cast<uint8_t>(180*alpha*fade))};
            cross[3] = sf::Vertex{{px,py+4.f}, sf::Color(80,255,100, static_cast<uint8_t>(180*alpha*fade))};
            m_renderer.getWindow().draw(cross);
        }
    }

    // Draw lava particles
    for (const auto& lp : m_lavaParticles) {
        if (!lp.alive) continue;
        b2Vec2 pos = b2Body_GetPosition(lp.bodyId);
        b2Vec2 lvel = b2Body_GetLinearVelocity(lp.bodyId);
        sf::Vector2f sp = {SCREEN_CX + pos.x * PPM, SCREEN_CY - pos.y * PPM};
        float fade = 1.0f - (lp.timer / lp.lifetime);
        float flicker = std::sin(lp.timer * 15.f) * 0.15f + 0.85f;

        // Outer glow halo
        float glowR = (7.f + std::sin(lp.timer * 8.f) * 2.f) * fade;
        sf::CircleShape glow(glowR * 2.0f);
        glow.setOrigin({glowR * 2.0f, glowR * 2.0f}); glow.setPosition(sp);
        glow.setFillColor(sf::Color(255, 60, 0, static_cast<uint8_t>(40 * fade)));
        m_renderer.getWindow().draw(glow);

        // Main lava blob (slightly stretched in direction of motion)
        float blobR = glowR;
        sf::CircleShape blob(blobR);
        blob.setOrigin({blobR, blobR}); blob.setPosition(sp);
        float lspd = std::sqrt(lvel.x*lvel.x + lvel.y*lvel.y);
        if (lspd > 1.f) {
            float stretch = std::min(1.6f, 1.0f + lspd * 0.05f);
            float moveAng = std::atan2(-lvel.y, lvel.x) * 180.f / 3.14159f;
            blob.setScale({stretch, 1.0f / stretch});
            blob.setRotation(sf::degrees(moveAng));
        }
        // Color: bright yellow core -> orange -> dark red as it cools
        uint8_t rr2 = 255;
        uint8_t gg2 = static_cast<uint8_t>(std::max(0.f, 200.f * fade * flicker - 40.f));
        uint8_t bb2 = static_cast<uint8_t>(std::max(0.f, 60.f * fade * flicker));
        blob.setFillColor(sf::Color(rr2, gg2, bb2, static_cast<uint8_t>(220 * fade)));
        blob.setOutlineColor(sf::Color(255, static_cast<uint8_t>(120 * flicker), 0, static_cast<uint8_t>(180 * fade)));
        blob.setOutlineThickness(1.5f);
        m_renderer.getWindow().draw(blob);

        // Hot core
        float coreR = blobR * 0.4f;
        sf::CircleShape core(coreR);
        core.setOrigin({coreR, coreR}); core.setPosition(sp);
        core.setFillColor(sf::Color(255, 255, static_cast<uint8_t>(150 * flicker), static_cast<uint8_t>(180 * fade)));
        m_renderer.getWindow().draw(core);

        // Dripping trail behind
        if (lspd > 0.5f) {
            for (int i = 1; i <= 4; i++) {
                float tf = static_cast<float>(i);
                float tx2 = sp.x - (lvel.x / PPM) * tf * 0.03f * PPM;
                float ty2 = sp.y + (lvel.y / PPM) * tf * 0.03f * PPM + tf * 1.5f; // drips down
                float dripSz = (blobR * 0.7f - tf * 0.8f) * fade;
                if (dripSz < 0.5f) continue;
                sf::CircleShape drip(dripSz);
                drip.setOrigin({dripSz, dripSz}); drip.setPosition({tx2, ty2});
                drip.setFillColor(sf::Color(255, static_cast<uint8_t>(80 - tf*15), 0, static_cast<uint8_t>((150 - tf*35) * fade)));
                m_renderer.getWindow().draw(drip);
            }
        }
    }

    // Draw grapple hooks and ropes
    for (const auto& g : m_grapples) {
        if (!g.active) continue;

        // Find player position
        b2Vec2 ppos = {0,0};
        for (auto& p : m_players) {
            if (p->getPlayerIndex() == g.playerIndex && p->isAlive()) {
                ppos = p->getPosition();
                break;
            }
        }
        sf::Vector2f playerScreen = {SCREEN_CX + ppos.x * PPM, SCREEN_CY - ppos.y * PPM};

        b2Vec2 hookPos;
        if (g.hookFlying && B2_IS_NON_NULL(g.hookProjectile) && b2Body_IsValid(g.hookProjectile)) {
            hookPos = b2Body_GetPosition(g.hookProjectile);
        } else {
            hookPos = g.hookPoint;
            // Track grappled player position
            if (g.grappledPlayerIndex >= 0) {
                for (auto& p : m_players) {
                    if (p->getPlayerIndex() == g.grappledPlayerIndex && p->isAlive()) {
                        hookPos = p->getPosition();
                        break;
                    }
                }
            }
        }
        sf::Vector2f hookScreen = {SCREEN_CX + hookPos.x * PPM, SCREEN_CY - hookPos.y * PPM};

        // Draw rope
        sf::VertexArray rope(sf::PrimitiveType::Lines, 2);
        rope[0] = sf::Vertex{playerScreen, sf::Color(180, 160, 120, 220)};
        rope[1] = sf::Vertex{hookScreen, sf::Color(140, 120, 80, 180)};
        m_renderer.getWindow().draw(rope);

        // Draw hook
        float hookR = g.hookFlying ? 3.f : 5.f;
        sf::CircleShape hook(hookR); hook.setOrigin({hookR, hookR});
        hook.setPosition(hookScreen);
        if (g.grappledPlayerIndex >= 0) {
            hook.setFillColor(sf::Color(255, 100, 100, 230)); // red for player grapple
        } else {
            hook.setFillColor(sf::Color(180, 180, 180, 230)); // grey for surface
        }
        hook.setOutlineColor(sf::Color(100, 100, 100, 200));
        hook.setOutlineThickness(1.f);
        m_renderer.getWindow().draw(hook);
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
            // === CHARGE-SPECIFIC EXPLOSION ANIMATIONS ===
            float r = blastPx * progress;
            float alpha = 1.0f - progress;
            auto charge = fx.charge;

            if (charge == GrenadeCharge::Freeze) {
                // FREEZE: expanding ice crystal ring with snowflake sparkles
                float iceR = blastPx * std::min(1.0f, progress * 2.5f);
                float iceAlpha = (1.0f - progress);
                // Outer frost ring
                sf::CircleShape frost(iceR);
                frost.setOrigin({iceR, iceR}); frost.setPosition(ep);
                frost.setFillColor(sf::Color(150, 220, 255, static_cast<uint8_t>(iceAlpha * 80)));
                frost.setOutlineColor(sf::Color(200, 240, 255, static_cast<uint8_t>(iceAlpha * 200)));
                frost.setOutlineThickness(2.5f);
                m_renderer.getWindow().draw(frost);
                // Inner flash
                if (progress < 0.3f) {
                    float flashR = blastPx * 0.6f * (1.0f - progress / 0.3f);
                    sf::CircleShape flash(flashR);
                    flash.setOrigin({flashR, flashR}); flash.setPosition(ep);
                    flash.setFillColor(sf::Color(220, 240, 255, static_cast<uint8_t>((1.0f - progress/0.3f) * 180)));
                    m_renderer.getWindow().draw(flash);
                }
                // Ice crystal sparkles
                int sparkCount = static_cast<int>(8 * iceAlpha);
                for (int s = 0; s < sparkCount; s++) {
                    float ang = static_cast<float>(s) * 0.785f + progress * 3.f;
                    float dist = iceR * 0.7f * (0.4f + std::sin(ang * 5.f) * 0.3f);
                    float sx2 = ep.x + std::cos(ang) * dist;
                    float sy2 = ep.y + std::sin(ang) * dist;
                    // 6-pointed star shape (two overlapping triangles)
                    float starSz = 3.f * iceAlpha;
                    sf::CircleShape star(starSz, 6);
                    star.setOrigin({starSz, starSz}); star.setPosition({sx2, sy2});
                    star.setRotation(sf::degrees(ang * 60.f));
                    star.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(200 * iceAlpha)));
                    m_renderer.getWindow().draw(star);
                }

            } else if (charge == GrenadeCharge::Time) {
                // TIME: purple vortex with clock-like spinning lines
                float vortexR = blastPx * (0.3f + progress * 0.7f);
                // Swirling purple rings
                for (int ring = 0; ring < 3; ring++) {
                    float rr = vortexR * (0.4f + ring * 0.3f);
                    float ringAlpha = alpha * (1.0f - ring * 0.25f);
                    sf::CircleShape vring(rr);
                    vring.setOrigin({rr, rr}); vring.setPosition(ep);
                    vring.setFillColor(sf::Color::Transparent);
                    vring.setOutlineColor(sf::Color(160, 80, 255, static_cast<uint8_t>(ringAlpha * 180)));
                    vring.setOutlineThickness(1.5f);
                    m_renderer.getWindow().draw(vring);
                }
                // Spinning clock hands
                float handAng = progress * 720.f * 3.14159f / 180.f;
                for (int h = 0; h < 4; h++) {
                    float a2 = handAng + h * 1.5708f;
                    float len = vortexR * 0.6f;
                    sf::VertexArray hand(sf::PrimitiveType::Lines, 2);
                    hand[0] = sf::Vertex{ep, sf::Color(200, 150, 255, static_cast<uint8_t>(alpha * 220))};
                    hand[1] = sf::Vertex{{ep.x + std::cos(a2)*len, ep.y + std::sin(a2)*len},
                                          sf::Color(120, 40, 200, static_cast<uint8_t>(alpha * 60))};
                    m_renderer.getWindow().draw(hand);
                }
                // Center glow
                float coreR = vortexR * 0.2f;
                sf::CircleShape core(coreR);
                core.setOrigin({coreR, coreR}); core.setPosition(ep);
                core.setFillColor(sf::Color(200, 160, 255, static_cast<uint8_t>(alpha * 150)));
                m_renderer.getWindow().draw(core);

            } else if (charge == GrenadeCharge::Lava) {
                // LAVA: eruption with rising fire chunks and magma splatter
                float eruptR = blastPx * std::min(1.0f, progress * 2.0f);
                // Magma pool at base
                float poolR = eruptR * 0.7f;
                sf::CircleShape magma(poolR);
                magma.setScale({1.3f, 0.5f});
                magma.setOrigin({poolR, poolR}); magma.setPosition(ep);
                magma.setFillColor(sf::Color(255, 80, 0, static_cast<uint8_t>(alpha * 160)));
                magma.setOutlineColor(sf::Color(255, 200, 0, static_cast<uint8_t>(alpha * 200)));
                magma.setOutlineThickness(2.f);
                m_renderer.getWindow().draw(magma);
                // Rising fire chunks
                int chunkCount = static_cast<int>(10 * (1.0f - progress * 0.5f));
                for (int c = 0; c < chunkCount; c++) {
                    float ang2 = static_cast<float>(c) * 0.628f + progress * 2.f;
                    float rise = progress * blastPx * 1.5f * (0.5f + std::sin(ang2 * 3.f) * 0.5f);
                    float spread = std::cos(ang2) * eruptR * 0.6f;
                    float chunkAlpha = alpha * (1.0f - rise / (blastPx * 1.5f));
                    if (chunkAlpha <= 0.f) continue;
                    float sz2 = 3.f * chunkAlpha;
                    sf::CircleShape chunk(sz2);
                    chunk.setOrigin({sz2, sz2});
                    chunk.setPosition({ep.x + spread, ep.y - rise});
                    uint8_t g2 = static_cast<uint8_t>(std::max(0.f, 150.f - rise * 2.f));
                    chunk.setFillColor(sf::Color(255, g2, 0, static_cast<uint8_t>(chunkAlpha * 230)));
                    m_renderer.getWindow().draw(chunk);
                }

            } else if (charge == GrenadeCharge::Frag) {
                // FRAG: sharp burst with metal shrapnel flying outward
                if (progress < 0.2f) {
                    // Initial flash
                    float flashR = blastPx * 0.5f * (1.0f - progress / 0.2f);
                    sf::CircleShape flash(flashR);
                    flash.setOrigin({flashR, flashR}); flash.setPosition(ep);
                    flash.setFillColor(sf::Color(255, 240, 200, static_cast<uint8_t>((1.0f - progress/0.2f) * 200)));
                    m_renderer.getWindow().draw(flash);
                }
                // Shrapnel pieces flying outward
                int shrapCount = static_cast<int>(12 * (1.0f - progress));
                for (int s = 0; s < shrapCount; s++) {
                    float ang3 = static_cast<float>(s) * 0.524f + 0.3f;
                    float dist2 = blastPx * progress * (0.8f + std::sin(ang3 * 7.f) * 0.2f);
                    float sx3 = ep.x + std::cos(ang3) * dist2;
                    float sy3 = ep.y + std::sin(ang3) * dist2;
                    // Rectangular shrapnel piece
                    sf::RectangleShape shrap({4.f * alpha, 2.f * alpha});
                    shrap.setOrigin({2.f * alpha, 1.f * alpha});
                    shrap.setPosition({sx3, sy3});
                    shrap.setRotation(sf::degrees(ang3 * 60.f + progress * 500.f));
                    shrap.setFillColor(sf::Color(180, 180, 180, static_cast<uint8_t>(alpha * 220)));
                    m_renderer.getWindow().draw(shrap);
                }
                // Smoke ring
                float smokeR = blastPx * progress * 0.8f;
                sf::CircleShape smoke(smokeR);
                smoke.setOrigin({smokeR, smokeR}); smoke.setPosition(ep);
                smoke.setFillColor(sf::Color(80, 80, 80, static_cast<uint8_t>(alpha * 60)));
                m_renderer.getWindow().draw(smoke);

            } else if (charge == GrenadeCharge::Implode) {
                // IMPLODE: rapid inward collapse then violent outward burst
                if (progress < 0.3f) {
                    float collapseR = blastPx * (1.0f - progress / 0.3f);
                    sf::CircleShape collapse(collapseR);
                    collapse.setOrigin({collapseR, collapseR}); collapse.setPosition(ep);
                    collapse.setFillColor(sf::Color(60, 0, 140, 120));
                    collapse.setOutlineColor(sf::Color(180, 80, 255, 200));
                    collapse.setOutlineThickness(3.f);
                    m_renderer.getWindow().draw(collapse);
                } else {
                    float burstProgress = (progress - 0.3f) / 0.7f;
                    float burstR = blastPx * burstProgress;
                    float burstAlpha = 1.0f - burstProgress;
                    sf::CircleShape burst(burstR);
                    burst.setOrigin({burstR, burstR}); burst.setPosition(ep);
                    burst.setFillColor(sf::Color(100, 20, 200, static_cast<uint8_t>(burstAlpha * 150)));
                    m_renderer.getWindow().draw(burst);
                    float coreR2 = burstR * 0.4f;
                    sf::CircleShape core2(coreR2);
                    core2.setOrigin({coreR2, coreR2}); core2.setPosition(ep);
                    core2.setFillColor(sf::Color(200, 150, 255, static_cast<uint8_t>(burstAlpha * 220)));
                    m_renderer.getWindow().draw(core2);
                }

            } else {
                // NORMAL: standard orange fireball
                sf::CircleShape blast(r);
                blast.setOrigin({r, r}); blast.setPosition(ep);
                blast.setFillColor(sf::Color(255, 150, 0, static_cast<uint8_t>(alpha * 150)));
                m_renderer.getWindow().draw(blast);
                sf::CircleShape core(r * 0.5f);
                core.setOrigin({r * 0.5f, r * 0.5f}); core.setPosition(ep);
                core.setFillColor(sf::Color(255, 255, 200, static_cast<uint8_t>(alpha * 200)));
                m_renderer.getWindow().draw(core);
            }
        }
    }

    // Reset view for HUD (screen-space, not world-space)
    m_renderer.getWindow().setView(m_renderer.getWindow().getDefaultView());

    m_hud.draw(m_renderer.getWindow(), m_players, m_roundTimer);

    if (m_state == GameState::RoundOver) {
        sf::RectangleShape overlay({SCREEN_WIDTH, SCREEN_HEIGHT});
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        m_renderer.getWindow().draw(overlay);
    }

    m_renderer.display();
}

// ============================================================
// AREA EFFECT UPDATES
// ============================================================

void Game::updateBlackHoles(float dt) {
    for (auto& bh : m_blackHoles) {
        if (!bh.alive) continue;
        bh.timer += dt;
        if (bh.timer >= bh.duration) { bh.alive = false; continue; }

        for (auto& player : m_players) {
            if (!player->isAlive()) continue;
            b2Vec2 pos = player->getPosition();
            float dx = bh.x - pos.x, dy = bh.y - pos.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < bh.radius && dist > 0.3f) {
                // Strong continuous pull using force (not impulse)
                float factor = 1.0f - dist / bh.radius;
                float strength = bh.pullForce * factor * factor * 15.0f;
                b2Body_ApplyForceToCenter(player->getTorsoBodyId(),
                    {dx/dist * strength, dy/dist * strength}, true);

                // Damage increases as players get closer to center
                if (dist < bh.radius * 0.5f) {
                    float dmgMult = 1.0f + (1.0f - dist / (bh.radius * 0.5f)) * 2.0f;
                    player->takeDamage(bh.dps * dmgMult * dt, 0.0f, 0.0f);
                }
            }
        }
        // Black hole pulls projectiles
        for (auto& proj : m_projectiles) {
            if (!proj.alive || !B2_IS_NON_NULL(proj.bodyId) || !b2Body_IsValid(proj.bodyId)) continue;
            b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
            float pdx = bh.x - pp.x, pdy = bh.y - pp.y;
            float pdist = std::sqrt(pdx*pdx + pdy*pdy);
            if (pdist < bh.radius && pdist > 0.3f) {
                float pf = (1.0f - pdist/bh.radius) * bh.pullForce * 8.0f;
                b2Body_ApplyForceToCenter(proj.bodyId, {pdx/pdist*pf, pdy/pdist*pf}, true);
            }
        }
        for (auto& lp : m_lavaParticles) {
            if (!lp.alive || !B2_IS_NON_NULL(lp.bodyId) || !b2Body_IsValid(lp.bodyId)) continue;
            b2Vec2 pp = b2Body_GetPosition(lp.bodyId);
            float pdx = bh.x - pp.x, pdy = bh.y - pp.y;
            float pdist = std::sqrt(pdx*pdx + pdy*pdy);
            if (pdist < bh.radius && pdist > 0.3f) {
                float pf = (1.0f - pdist/bh.radius) * bh.pullForce * 8.0f;
                b2Body_ApplyForceToCenter(lp.bodyId, {pdx/pdist*pf, pdy/pdist*pf}, true);
            }
        }
    }
    m_blackHoles.erase(std::remove_if(m_blackHoles.begin(), m_blackHoles.end(),
        [](const BlackHoleEffect& b){ return !b.alive; }), m_blackHoles.end());
}

void Game::updateTornadoes(float dt) {
    for (auto& t : m_tornadoes) {
        if (!t.alive) continue;
        t.timer += dt;
        if (t.timer >= t.duration) { t.alive = false; continue; }

        t.x += t.vx * dt;
        t.y += t.vy * dt;

        for (auto& player : m_players) {
            if (!player->isAlive()) continue;
            // Don't hurt the shooter for the first 1.5 seconds
            if (player->getPlayerIndex() == t.shooterIndex && t.timer < 1.5f) continue;

            b2Vec2 pos = player->getPosition();
            float dx = t.x - pos.x, dy = t.y - pos.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < t.radius) {
                float factor = 1.0f - dist / t.radius;
                // Random direction but never down
                float randAngle = static_cast<float>(std::rand()) / RAND_MAX * 3.14159f;
                float fx = std::cos(randAngle) * t.spinForce * factor * 10.0f;
                float fy = std::abs(std::sin(randAngle)) * t.liftForce * factor * 10.0f;
                b2Body_ApplyForceToCenter(player->getTorsoBodyId(), {fx, fy}, true);
                player->takeDamage(t.dps * dt, 0.0f, 0.0f);
            }
        }
        // Tornado flings projectiles and grenades
        for (auto& proj : m_projectiles) {
            if (!proj.alive || !B2_IS_NON_NULL(proj.bodyId) || !b2Body_IsValid(proj.bodyId)) continue;
            b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
            float pdx = t.x - pp.x, pdy = t.y - pp.y;
            float pdist = std::sqrt(pdx*pdx + pdy*pdy);
            if (pdist < t.radius) {
                float pf = (1.0f - pdist/t.radius) * t.spinForce * 6.0f;
                float ra = static_cast<float>(std::rand()) / RAND_MAX * 3.14159f;
                b2Body_ApplyForceToCenter(proj.bodyId, {std::cos(ra)*pf, std::abs(std::sin(ra))*pf}, true);
            }
        }
        for (auto& lp : m_lavaParticles) {
            if (!lp.alive || !B2_IS_NON_NULL(lp.bodyId) || !b2Body_IsValid(lp.bodyId)) continue;
            b2Vec2 pp = b2Body_GetPosition(lp.bodyId);
            float pdx = t.x - pp.x, pdy = t.y - pp.y;
            float pdist = std::sqrt(pdx*pdx + pdy*pdy);
            if (pdist < t.radius) {
                float pf = (1.0f - pdist/t.radius) * t.spinForce * 6.0f;
                float ra = static_cast<float>(std::rand()) / RAND_MAX * 3.14159f;
                b2Body_ApplyForceToCenter(lp.bodyId, {std::cos(ra)*pf, std::abs(std::sin(ra))*pf}, true);
            }
        }
    }
    m_tornadoes.erase(std::remove_if(m_tornadoes.begin(), m_tornadoes.end(),
        [](const TornadoEffect& t){ return !t.alive; }), m_tornadoes.end());
}

void Game::updateTimeSlowZones(float dt) {
    for (auto& z : m_timeSlowZones) {
        if (!z.alive) continue;
        z.timer += dt;
        if (z.timer >= z.duration) {
            // Secondary explosion when zone expires (Time Grenade)
            if (z.secondaryDamage > 0.0f) {
                const auto& rules = m_rulesEngine.getRules();
                for (auto& player : m_players) {
                    if (!player->isAlive()) continue;
                    b2Vec2 pos = player->getPosition();
                    float dx = z.x - pos.x, dy = z.y - pos.y;
                    float dist = std::sqrt(dx*dx + dy*dy);
                    if (dist < z.radius) {
                        float f = std::max(0.3f, 1.0f - dist / z.radius);
                        float kbDir = dx >= 0 ? 1.f : -1.f;
                        player->takeDamage(z.secondaryDamage * f * rules.damageMultiplier,
                            kbDir * z.secondaryKnockback * f * rules.knockbackMultiplier,
                            z.secondaryKnockback * 0.4f * f * rules.knockbackMultiplier);
                        // Secondary explosion always applies time slow
                        player->applyTimeSlow(2.0f * f);
                    }
                }
                spawnExplosion(z.x, z.y, z.radius, false);
            }
            z.alive = false;
            continue;
        }

        for (auto& player : m_players) {
            if (!player->isAlive()) continue;
            b2Vec2 pos = player->getPosition();
            float dx = z.x - pos.x, dy = z.y - pos.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < z.radius) {
                player->applyTimeSlow(dt + 0.1f);
            }
        }
        // Time zone freezes projectiles and grenades inside
        for (auto& proj : m_projectiles) {
            if (!proj.alive) continue;
            if (!B2_IS_NON_NULL(proj.bodyId) || !b2Body_IsValid(proj.bodyId)) continue;
            b2Vec2 pp = b2Body_GetPosition(proj.bodyId);
            float pdx = z.x - pp.x, pdy = z.y - pp.y;
            float pdist = std::sqrt(pdx*pdx + pdy*pdy);
            if (pdist < z.radius) {
                b2Body_SetLinearVelocity(proj.bodyId, {0, 0});
                b2Body_SetGravityScale(proj.bodyId, 0.0f);
                proj.lifetime += dt; // freeze countdown
            }
        }
        // Freeze lava particles
        for (auto& lp : m_lavaParticles) {
            if (!lp.alive) continue;
            if (!B2_IS_NON_NULL(lp.bodyId) || !b2Body_IsValid(lp.bodyId)) continue;
            b2Vec2 pp = b2Body_GetPosition(lp.bodyId);
            float pdx = z.x - pp.x, pdy = z.y - pp.y;
            if (std::sqrt(pdx*pdx + pdy*pdy) < z.radius) {
                b2Body_SetLinearVelocity(lp.bodyId, {0, 0});
                b2Body_SetGravityScale(lp.bodyId, 0.0f);
                lp.timer -= dt; // freeze timer
            }
        }
    }
    m_timeSlowZones.erase(std::remove_if(m_timeSlowZones.begin(), m_timeSlowZones.end(),
        [](const TimeSlowZone& z){ return !z.alive; }), m_timeSlowZones.end());
}

void Game::updatePortals(float dt) {
    for (size_t i = 0; i < m_portals.size(); i++) {
        auto& p = m_portals[i];
        if (!p.alive) continue;
        p.timer += dt;
        if (p.timer >= p.duration || p.pairIndex < 0) { p.alive = false; continue; }

        // Check if partner is still valid
        if (p.pairIndex < 0 || p.pairIndex >= static_cast<int>(m_portals.size())) {
            p.alive = false; continue;
        }
        auto& partner = m_portals[p.pairIndex];
        if (!partner.alive) { p.alive = false; continue; }

        for (auto& player : m_players) {
            if (!player->isAlive()) continue;
            // Use teleport cooldown to prevent instant re-teleport
            if (player->getTeleportCD() > 0.0f) continue;
            b2Vec2 pos = player->getPosition();
            float dx = p.x - pos.x, dy = p.y - pos.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < 1.5f) {
                // Teleport to partner portal
                player->teleportTo(partner.x + 1.0f, partner.y);
                player->setTeleportCD(0.5f); // prevent instant re-teleport
                break; // one player per frame
            }
        }
    }
    // Don't erase portals — pairIndex values are vector indices and would
    // become invalid if elements are removed. Just leave dead portals in place.
    // Clean up only when ALL portals are dead.
    bool anyAlive = false;
    for (const auto& p : m_portals) if (p.alive) { anyAlive = true; break; }
    if (!anyAlive) m_portals.clear();
}

void Game::updateMeteorShowers(float dt) {
    static std::mt19937 rng(std::random_device{}());
    for (auto& ms : m_meteorShowers) {
        if (!ms.alive) continue;
        ms.meteorTimer += dt;
        if (ms.meteorTimer >= ms.meteorDelay && ms.meteorsLeft > 0) {
            ms.meteorTimer = 0.0f;
            ms.meteorsLeft--;

            std::uniform_real_distribution<float> ang(0, 6.283f);
            std::uniform_real_distribution<float> rad(0, ms.spreadRadius);
            float a = ang(rng), r = rad(rng);
            float mx = ms.centerX + std::cos(a) * r;
            float my = ms.centerY + std::sin(a) * r;

            spawnExplosion(mx, my, ms.explosionRadius);
            m_arena.carveCircle(m_physics, mx, my, ms.explosionRadius * 0.5f);

            for (auto& player : m_players) {
                if (!player->isAlive()) continue;
                b2Vec2 pos = player->getPosition();
                float dx = mx - pos.x, dy = my - pos.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist < ms.explosionRadius) {
                    float f = std::max(0.3f, 1.0f - dist / ms.explosionRadius);
                    float kbDir = dx >= 0 ? 1.f : -1.f;
                    player->takeDamage(ms.damage * f,
                        kbDir * ms.knockback * f, ms.knockback * 0.5f * f);
                }
            }
        }
        if (ms.meteorsLeft <= 0) ms.alive = false;
    }
    m_meteorShowers.erase(std::remove_if(m_meteorShowers.begin(), m_meteorShowers.end(),
        [](const MeteorShower& ms){ return !ms.alive; }), m_meteorShowers.end());
}

void Game::updateHealZones(float dt) {
    for (auto& hz : m_healZones) {
        if (!hz.alive) continue;
        hz.timer += dt;
        if (hz.timer >= hz.duration) { hz.alive = false; continue; }

        for (auto& player : m_players) {
            if (!player->isAlive()) continue;
            b2Vec2 pos = player->getPosition();
            float dx = hz.x - pos.x, dy = hz.y - pos.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < hz.radius)
                player->heal(hz.healRate * dt);
        }
    }
    m_healZones.erase(std::remove_if(m_healZones.begin(), m_healZones.end(),
        [](const HealZone& hz){ return !hz.alive; }), m_healZones.end());
}

void Game::updateLavaPools(float dt) {
    const auto& rules = m_rulesEngine.getRules();

    // Update lava particles (physical objects that hurt on contact)
    for (auto& lp : m_lavaParticles) {
        if (!lp.alive) continue;
        lp.timer += dt;
        if (lp.timer >= lp.lifetime) { lp.alive = false; continue; }

        // Cap lava particle speed
        b2Vec2 lvel = b2Body_GetLinearVelocity(lp.bodyId);
        float lspd = std::sqrt(lvel.x*lvel.x + lvel.y*lvel.y);
        if (lspd > 30.0f) {
            float sc = 30.0f / lspd;
            b2Body_SetLinearVelocity(lp.bodyId, {lvel.x * sc, lvel.y * sc});
        }

        b2Vec2 pp = b2Body_GetPosition(lp.bodyId);
        for (auto& player : m_players) {
            if (!player->isAlive()) continue;
            b2Vec2 pl = player->getPosition();
            float dx = pl.x - pp.x, dy = pl.y - pp.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < 0.8f) {
                player->takeDamage(lp.dps * dt * rules.damageMultiplier, 0.0f, 1.0f * dt);
            }
        }
    }
    // Clean up dead particles
    for (auto& lp : m_lavaParticles) {
        if (!lp.alive) {
            b2DestroyBody(lp.bodyId);
        }
    }
    m_lavaParticles.erase(std::remove_if(m_lavaParticles.begin(), m_lavaParticles.end(),
        [](const LavaParticle& lp){ return !lp.alive; }), m_lavaParticles.end());
}

void Game::spawnLavaPool(float x, float y, float radius, float dps, float duration, int ownerIndex) {
    // Lava charge: spray out physical lava particles
    static std::mt19937 rng(std::random_device{}());
    // Particle count scales with radius
    int count = static_cast<int>(radius * 5.0f);
    if (count < 8) count = 8;
    if (count > 30) count = 30;

    for (int i = 0; i < count; i++) {
        float angle = (static_cast<float>(i) / count) * 6.28318f;
        std::uniform_real_distribution<float> jitter(-0.4f, 0.4f);
        angle += jitter(rng);
        std::uniform_real_distribution<float> speedDist(3.0f, radius * 2.5f);
        float spd = speedDist(rng);
        float vx = std::cos(angle) * spd;
        float vy = std::sin(angle) * spd + 3.0f; // bias upward

        b2BodyId body = m_physics.createDynamicCircle(
            x, y, 0.15f, 0.03f, CAT_PROJECTILE, CAT_PLATFORM | CAT_PLAYER);
        b2Body_SetLinearVelocity(body, {vx, vy});
        b2Body_SetGravityScale(body, 0.7f);

        LavaParticle lp;
        lp.bodyId = body;
        lp.timer = 0.0f;
        lp.lifetime = duration > 0 ? duration * 0.8f : 30.0f;
        lp.dps = dps > 0 ? dps : 12.0f;
        lp.alive = true;
        m_lavaParticles.push_back(lp);
    }
}

void Game::spawnFragChildren(float x, float y, int ownerIndex, const WeaponData& parentWeapon) {
    static std::mt19937 rng(std::random_device{}());
    int count = parentWeapon.fragChildCount > 0 ? parentWeapon.fragChildCount : 5;

    GrenadeCharge childCharge = parentWeapon.grenadeCharge;
    if (childCharge == GrenadeCharge::Frag) childCharge = GrenadeCharge::Normal;
    GrenadeCasing childCasing = parentWeapon.grenadeCasing;

    // High base velocity to clear parent explosion zone quickly
    float baseSpeed = 16.0f + parentWeapon.explosionRadius * 1.5f;

    for (int i = 0; i < count; i++) {
        // Tighter upward arc: ~45° to ~135° (mostly up and out, not horizontal)
        float arcStart = 0.785f;  // 45 degrees
        float arcEnd   = 2.356f;  // 135 degrees
        float angle = arcStart + (arcEnd - arcStart) * (static_cast<float>(i) / std::max(1, count - 1));
        std::uniform_real_distribution<float> jitter(-0.12f, 0.12f);
        angle += jitter(rng);

        std::uniform_real_distribution<float> speedJitter(0.9f, 1.1f);
        float speed = baseSpeed * speedJitter(rng);
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;

        WeaponData childWeapon = parentWeapon;
        childWeapon.grenadeCharge = childCharge;
        childWeapon.grenadeCasing = childCasing;
        childWeapon.fragChildCount = 0;
        childWeapon.isFragChild = true;
        childWeapon.affectedByGravity = true;
        childWeapon.damage = parentWeapon.fragChildDamage > 0 ? parentWeapon.fragChildDamage : parentWeapon.damage * 0.6f;
        childWeapon.explosionRadius = parentWeapon.fragChildRadius > 0 ? parentWeapon.fragChildRadius : parentWeapon.explosionRadius * 0.7f;
        childWeapon.knockbackForce = parentWeapon.knockbackForce * 0.3f;
        childWeapon.projectileSpeed = speed;

        // Staggered detonation timers — each child explodes at a different time
        std::uniform_real_distribution<float> staggerJitter(-0.15f, 0.15f);
        float staggerDelay = 0.8f + static_cast<float>(i) * 0.35f + staggerJitter(rng);
        childWeapon.projectileLifetime = staggerDelay + 8.0f; // long lifetime, timer-based det

        if (childCasing == GrenadeCasing::Sticky) {
            childWeapon.sticksToSurfaces = true;
            childWeapon.detonationDelay = staggerDelay;
        } else if (childCasing == GrenadeCasing::Mine) {
            childWeapon.mineProximity = 1.8f;
        } else if (childCasing == GrenadeCasing::Bounce) {
            childWeapon.bounces = 20;
            childWeapon.bounceEnergyRetention = 0.95f;
            childWeapon.detonationDelay = staggerDelay;
        } else if (childCasing == GrenadeCasing::Ballistic) {
            // ballistic children detonate on contact, no timer needed
        } else {
            // Normal casing: detonate on ground contact (speed drop) — no special timer
        }

        std::string chargeName = grenadeChargeName(childCharge);
        std::string casingName = grenadeCasingName(childCasing);
        if (childCharge == GrenadeCharge::Normal && childCasing == GrenadeCasing::Normal)
            childWeapon.name = "Frag Grenade";
        else if (childCharge == GrenadeCharge::Normal)
            childWeapon.name = casingName + " Frag Grenade";
        else if (childCasing == GrenadeCasing::Normal)
            childWeapon.name = chargeName + " Grenade";
        else
            childWeapon.name = chargeName + " " + casingName + " Grenade";

        float restitution = (childCasing == GrenadeCasing::Bounce) ? 0.95f : 0.3f;
        b2BodyId bullet = m_physics.createDynamicCircle(
            x, y + 0.5f, 0.35f, 0.1f, CAT_PROJECTILE, CAT_PLATFORM | CAT_PLAYER, restitution);
        b2Body_SetBullet(bullet, true);
        b2Body_SetLinearVelocity(bullet, {vx, vy});
        b2Body_SetGravityScale(bullet, 1.0f);

        Projectile proj;
        proj.bodyId = bullet;
        proj.weapon = childWeapon;
        proj.ownerIndex = ownerIndex;
        proj.lifetime = childWeapon.projectileLifetime;
        proj.alive = true;
        proj.isFragChild = false;
        proj.bouncesLeft = childWeapon.bounces;
        m_projectiles.push_back(proj);
    }
}

// ============================================================
// GRAPPLE
// ============================================================

void Game::updateCamera(float dt) {
    // Find bounding box of all alive players
    float minX = 999.f, maxX = -999.f, minY = 999.f, maxY = -999.f;
    int alive = 0;
    for (auto& p : m_players) {
        if (!p->isAlive()) continue;
        b2Vec2 pos = p->getPosition();
        minX = std::min(minX, pos.x);
        maxX = std::max(maxX, pos.x);
        minY = std::min(minY, pos.y);
        maxY = std::max(maxY, pos.y);
        alive++;
    }
    if (alive == 0) return;

    // Target camera center = centroid of alive players
    float targetX = (minX + maxX) * 0.5f;
    float targetY = (minY + maxY) * 0.5f;

    // Target zoom: fit all players with padding
    float spanX = (maxX - minX) + 12.0f; // padding in world units
    float spanY = (maxY - minY) + 8.0f;
    float zoomX = spanX / (SCREEN_WIDTH / PPM);
    float zoomY = spanY / (SCREEN_HEIGHT / PPM);
    float targetZoom = std::max(1.0f, std::max(zoomX, zoomY));
    targetZoom = std::min(targetZoom, 3.0f); // max zoom out

    // Smooth camera follow
    float smoothing = 5.0f * dt;
    m_camX += (targetX - m_camX) * smoothing;
    m_camY += (targetY - m_camY) * smoothing;
    m_camZoom += (targetZoom - m_camZoom) * smoothing;
}

WeaponData Game::randomizeGrenade(const WeaponData& base) {
    static std::mt19937 rng(std::random_device{}());

    // Pick random charge (weighted — nuclear is rarer)
    static const std::vector<std::pair<GrenadeCharge, int>> chargeWeights = {
        {GrenadeCharge::Normal, 3}, {GrenadeCharge::Freeze, 3}, {GrenadeCharge::Time, 2},
        {GrenadeCharge::Lava, 3}, {GrenadeCharge::Frag, 3}, {GrenadeCharge::Nuclear, 1},
        {GrenadeCharge::Implode, 2}
    };
    int totalCW = 0;
    for (auto& [c,w] : chargeWeights) totalCW += w;
    std::uniform_int_distribution<int> chargeDist(0, totalCW - 1);
    int roll = chargeDist(rng);
    GrenadeCharge charge = GrenadeCharge::Normal;
    int acc = 0;
    for (auto& [c,w] : chargeWeights) {
        acc += w;
        if (roll < acc) { charge = c; break; }
    }

    // Pick random casing
    static const std::vector<GrenadeCasing> casings = {
        GrenadeCasing::Normal, GrenadeCasing::Sticky, GrenadeCasing::Mine,
        GrenadeCasing::Ballistic, GrenadeCasing::Bounce
    };
    std::uniform_int_distribution<int> casingDist(0, static_cast<int>(casings.size()) - 1);
    GrenadeCasing casing = casings[casingDist(rng)];

    WeaponData g = base;
    g.grenadeCharge = charge;
    g.grenadeCasing = casing;

    // Build descriptive name
    std::string chargeName = grenadeChargeName(charge);
    std::string casingName = grenadeCasingName(casing);
    if (charge == GrenadeCharge::Normal && casing == GrenadeCasing::Normal)
        g.name = "Grenade";
    else if (charge == GrenadeCharge::Normal)
        g.name = casingName + " Grenade";
    else if (casing == GrenadeCasing::Normal)
        g.name = chargeName + " Grenade";
    else
        g.name = chargeName + " " + casingName + " Grenade";

    // Adjust stats by charge type
    switch (charge) {
        case GrenadeCharge::Freeze:
            g.damage = 12.f; g.knockbackForce = 1.5f; g.explosionRadius = 5.f;
            g.freezeBuildup = 10.f;
            break;
        case GrenadeCharge::Time:
            g.damage = 15.f; g.knockbackForce = 2.f; g.explosionRadius = 4.f;
            g.timeSlowFactor = 0.05f; g.timeSlowDuration = 3.5f; g.timeSlowRadius = 4.5f;
            break;
        case GrenadeCharge::Lava:
            g.damage = 40.f; g.knockbackForce = 6.f; g.explosionRadius = 4.f;
            g.lavaDps = 15.f; g.lavaDuration = 60.f; g.lavaRadius = 5.f;
            break;
        case GrenadeCharge::Nuclear:
            g.damage = 80.f; g.knockbackForce = 18.f; g.explosionRadius = 8.f;
            g.destroysPlatforms = true; g.ammo = 1;
            break;
        case GrenadeCharge::Frag:
            g.damage = 40.f; g.knockbackForce = 8.f; g.explosionRadius = 4.f;
            g.fragChildCount = 5; g.fragChildDamage = 18.f; g.fragChildRadius = 2.f;
            break;
        case GrenadeCharge::Implode:
            g.damage = 55.f; g.knockbackForce = 12.f; g.explosionRadius = 5.f;
            g.implodePullRadius = 14.f; g.implodePullDuration = 0.5f;
            break;
        default: // Normal
            g.damage = 50.f; g.knockbackForce = 10.f; g.explosionRadius = 4.f;
            break;
    }

    // Adjust stats by casing type
    switch (casing) {
        case GrenadeCasing::Sticky:
            g.sticksToSurfaces = true;
            g.detonationDelay = 2.0f;
            break;
        case GrenadeCasing::Mine:
            g.mineProximity = 1.8f;
            g.affectedByGravity = true;
            break;
        case GrenadeCasing::Ballistic:
            // Explodes on contact — no special fields needed
            break;
        case GrenadeCasing::Bounce:
            // 95% elastic bouncing — uses bounce system
            g.bounces = 20;
            g.bounceEnergyRetention = 0.95f;
            g.affectedByGravity = true;
            g.detonationDelay = 5.0f; // detonates after timer
            break;
        default: // Normal
            break;
    }

    std::cout << "[Grenade] Rolled: " << g.name << "\n";
    return g;
}

void Game::fireGrapple(StickFigure& shooter) {
    const auto& weapon = shooter.getCurrentWeapon();
    b2Vec2 pos = shooter.getPosition();
    float dir = static_cast<float>(shooter.getFacingDirection());
    float aim = shooter.getAimAngle();

    float speed = weapon.projectileSpeed > 0 ? weapon.projectileSpeed : 45.0f;
    float vx = speed * dir * std::cos(aim);
    float vy = speed * std::sin(aim);

    b2BodyId hook = m_physics.createDynamicCircle(
        pos.x + dir * 0.5f, pos.y + 0.3f, 0.1f, 0.01f,
        CAT_PROJECTILE, CAT_PLATFORM | CAT_PLAYER);
    b2Body_SetBullet(hook, true);
    b2Body_SetLinearVelocity(hook, {vx, vy});
    b2Body_SetGravityScale(hook, 0.0f); // hook flies straight

    GrappleState gs;
    gs.active = true;
    gs.playerIndex = shooter.getPlayerIndex();
    gs.grappledPlayerIndex = -1;
    gs.hookFlying = true;
    gs.hookProjectile = hook;
    gs.weapon = weapon;
    gs.maxLength = weapon.grappleMaxLength;
    gs.minLength = weapon.grappleMinLength;
    m_grapples.push_back(gs);
}

void Game::releaseGrapple(int playerIndex) {
    for (auto& g : m_grapples) {
        if (g.playerIndex == playerIndex && g.active) {
            g.active = false;
            if (B2_IS_NON_NULL(g.hookProjectile) && b2Body_IsValid(g.hookProjectile))
                b2DestroyBody(g.hookProjectile);
            if (B2_IS_NON_NULL(g.hookBody) && b2Body_IsValid(g.hookBody))
                b2DestroyBody(g.hookBody);
            g.hookProjectile = b2_nullBodyId;
            g.hookBody = b2_nullBodyId;
            // Restore friction on player
            for (auto& p : m_players) {
                if (p->getPlayerIndex() == playerIndex) {
                    b2ShapeId shapes[8];
                    int cnt = b2Body_GetShapes(p->getTorsoBodyId(), shapes, 8);
                    for (int s = 0; s < cnt; s++)
                        b2Shape_SetFriction(shapes[s], 0.5f);
                    // Brief upward nudge to prevent ground friction killing slingshot
                    b2Vec2 vel = b2Body_GetLinearVelocity(p->getTorsoBodyId());
                    if (std::abs(vel.x) > 5.0f || std::abs(vel.y) > 3.0f) {
                        // Give a tiny upward boost so they clear ground contact
                        b2Body_ApplyLinearImpulseToCenter(p->getTorsoBodyId(), {0, 2.0f}, true);
                    }
                    break;
                }
            }
        }
    }
    m_grapples.erase(std::remove_if(m_grapples.begin(), m_grapples.end(),
        [](const GrappleState& g){ return !g.active; }), m_grapples.end());
}

void Game::slingshotGrapple(int playerIndex) {
    for (auto& g : m_grapples) {
        if (g.playerIndex != playerIndex || !g.active || g.hookFlying) continue;

        // Find player
        for (auto& player : m_players) {
            if (player->getPlayerIndex() != playerIndex) continue;
            b2Vec2 ppos = player->getPosition();
            b2Vec2 target = g.hookPoint;

            if (g.grappledPlayerIndex >= 0) {
                // Grappled a player — pull yourself toward them
                for (auto& other : m_players) {
                    if (other->getPlayerIndex() == g.grappledPlayerIndex && other->isAlive())
                        target = other->getPosition();
                }
            }

            float dx = target.x - ppos.x, dy = target.y - ppos.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 0.1f) {
                float force = g.weapon.grappleSlingshotForce;
                // Clear ground contact so surface friction doesn't kill the launch
                b2Body_ApplyLinearImpulseToCenter(player->getTorsoBodyId(),
                    {dx / dist * force, dy / dist * force + 3.0f}, true);
            }
            break;
        }
        releaseGrapple(playerIndex);
        return;
    }
}

void Game::updateGrapples(float dt) {
    for (auto& g : m_grapples) {
        if (!g.active) continue;

        // Find the player
        StickFigure* player = nullptr;
        for (auto& p : m_players) {
            if (p->getPlayerIndex() == g.playerIndex && p->isAlive()) {
                player = p.get();
                break;
            }
        }
        if (!player) { g.active = false; continue; }

        // Hook in flight — check for surface/player contact
        if (g.hookFlying) {
            if (!b2Body_IsValid(g.hookProjectile)) { g.active = false; continue; }
            b2Vec2 hp = b2Body_GetPosition(g.hookProjectile);

            // Check lifetime (max range)
            b2Vec2 ppos = player->getPosition();
            float dx = hp.x - ppos.x, dy = hp.y - ppos.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > g.maxLength) {
                releaseGrapple(g.playerIndex);
                continue;
            }

            // Check player hit
            bool hitPlayer = false;
            for (auto& other : m_players) {
                if (other->getPlayerIndex() == g.playerIndex) continue;
                if (!other->isAlive()) continue;
                b2Vec2 op = other->getPosition();
                float ddx = hp.x - op.x, ddy = hp.y - op.y;
                if (std::sqrt(ddx * ddx + ddy * ddy) < 1.2f) {
                    g.grappledPlayerIndex = other->getPlayerIndex();
                    g.hookPoint = op;
                    g.hookFlying = false;
                    g.ropeLength = dist;
                    b2Body_SetLinearVelocity(g.hookProjectile, {0, 0});
                    b2Body_SetGravityScale(g.hookProjectile, 0.0f);
                    hitPlayer = true;
                    // Small damage on hook hit
                    other->takeDamage(g.weapon.damage, 0, 0);
                    break;
                }
            }
            if (hitPlayer) continue;

            // Check surface hit
            b2QueryFilter qf = b2DefaultQueryFilter();
            qf.maskBits = CAT_PLATFORM;
            bool hitSurface = false;
            b2BodyId hitBody = b2_nullBodyId;
            for (auto [ddx, ddy] : std::initializer_list<std::pair<float,float>>{{0.f,-0.5f},{0.f,0.5f},{-0.5f,0.f},{0.5f,0.f}}) {
                b2RayResult rr = b2World_CastRayClosest(m_physics.getWorldId(), hp, {ddx, ddy}, qf);
                if (rr.hit) {
                     hitSurface = true;
                     hitBody = b2Shape_GetBody(rr.shapeId);
                     break;
                 }
            }

            if (hitSurface) {
                g.hookPoint = hp;
                g.hookFlying = false;
                g.ropeLength = dist;
                b2Body_SetLinearVelocity(g.hookProjectile, {0, 0});
                b2Body_SetGravityScale(g.hookProjectile, 0.0f);
                g.attachedPlatformBody = hitBody;
                if (B2_IS_NON_NULL(hitBody) && b2Body_IsValid(hitBody)) {
                    b2Vec2 bp = b2Body_GetPosition(hitBody);
                    g.attachOffset = {hp.x - bp.x, hp.y - bp.y};
                }
            }
            continue;
        }

        // === Active grapple — rope constraint physics ===
        b2Vec2 ppos = player->getPosition();
        b2Vec2 target = g.hookPoint;

        // Update hook position for moving platforms
        if (g.grappledPlayerIndex < 0 && B2_IS_NON_NULL(g.attachedPlatformBody) && b2Body_IsValid(g.attachedPlatformBody)) {
            b2Vec2 bp = b2Body_GetPosition(g.attachedPlatformBody);
            g.hookPoint = {bp.x + g.attachOffset.x, bp.y + g.attachOffset.y};
            target = g.hookPoint;
            // Move hook visual body too
            if (B2_IS_NON_NULL(g.hookProjectile) && b2Body_IsValid(g.hookProjectile))
                b2Body_SetTransform(g.hookProjectile, target, b2Body_GetRotation(g.hookProjectile));
        }

        // If grappled to a player, track their position
        if (g.grappledPlayerIndex >= 0) {
            bool found = false;
            for (auto& other : m_players) {
                if (other->getPlayerIndex() == g.grappledPlayerIndex && other->isAlive()) {
                    target = other->getPosition();
                    g.hookPoint = target;
                    found = true;
                    break;
                }
            }
            if (!found) { releaseGrapple(g.playerIndex); continue; }
        }

        // Move hook visual to track point
        if (B2_IS_NON_NULL(g.hookProjectile) && b2Body_IsValid(g.hookProjectile))
            b2Body_SetTransform(g.hookProjectile, target, b2Body_GetRotation(g.hookProjectile));

        float dx = target.x - ppos.x, dy = target.y - ppos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        float dirX = dist > 0.01f ? dx / dist : 0.f;
        float dirY = dist > 0.01f ? dy / dist : 0.f;

        // Rope constraint: hard rope — preserve tangential velocity for swinging
        if (dist > g.ropeLength) {
            float excess = dist - g.ropeLength;

            // Very strong pull impulse
            float pullForce = excess * 150.0f;
            if (g.grappledPlayerIndex < 0) {
                b2Body_ApplyLinearImpulseToCenter(player->getTorsoBodyId(),
                    {dirX * pullForce * dt, dirY * pullForce * dt}, true);
            } else {
                b2Body_ApplyLinearImpulseToCenter(player->getTorsoBodyId(),
                    {dirX * pullForce * 0.5f * dt, dirY * pullForce * 0.5f * dt}, true);
                for (auto& other : m_players) {
                    if (other->getPlayerIndex() == g.grappledPlayerIndex && other->isAlive()) {
                        b2Body_ApplyLinearImpulseToCenter(other->getTorsoBodyId(),
                            {-dirX * pullForce * 0.5f * dt, -dirY * pullForce * 0.5f * dt}, true);
                    }
                }
            }

            // Hard position clamp — snap to rope length, keeps swing intact
            if (excess > 0.15f) {
                b2Vec2 clampPos = {target.x - dirX * g.ropeLength, target.y - dirY * g.ropeLength};
                b2Body_SetTransform(player->getTorsoBodyId(), clampPos, b2Body_GetRotation(player->getTorsoBodyId()));
            }

            // Strip ONLY outward radial velocity — tangential (swing) stays fully intact
            b2Vec2 vel = b2Body_GetLinearVelocity(player->getTorsoBodyId());
            float radialVel = vel.x * (-dirX) + vel.y * (-dirY);
            if (radialVel > 0) {
                vel.x += dirX * radialVel;
                vel.y += dirY * radialVel;
                b2Body_SetLinearVelocity(player->getTorsoBodyId(), vel);
            }
        }

        // Check if surface was destroyed
        if (g.grappledPlayerIndex < 0) {
            b2QueryFilter qf2 = b2DefaultQueryFilter();
            qf2.maskBits = CAT_PLATFORM;
            bool surfaceOk = false;
            for (auto [ddx2, ddy2] : std::initializer_list<std::pair<float,float>>{{0.f,-0.8f},{0.f,0.8f},{-0.8f,0.f},{0.8f,0.f}}) {
                b2RayResult rr2 = b2World_CastRayClosest(m_physics.getWorldId(), target, {ddx2, ddy2}, qf2);
                if (rr2.hit) { surfaceOk = true; break; }
            }
            if (!surfaceOk) { releaseGrapple(g.playerIndex); continue; }
        }
    }

    // Clean up
    m_grapples.erase(std::remove_if(m_grapples.begin(), m_grapples.end(),
        [](const GrappleState& gs){ return !gs.active; }), m_grapples.end());
}

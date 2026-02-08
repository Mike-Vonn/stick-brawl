#include "Arena.h"
#include <random>

void Arena::createDefaultArena(Physics& physics) {
    auto addPlatform = [&](float cx, float cy, float hw, float hh) {
        Platform p;
        p.cx = cx; p.cy = cy; p.halfWidth = hw; p.halfHeight = hh;
        p.bodyId = physics.createStaticBox(cx, cy, hw, hh, CAT_PLATFORM);
        m_platforms.push_back(p);
    };

    addPlatform(0.0f, -5.0f, 15.0f, 0.5f);   // Main ground
    addPlatform(-8.0f, -1.0f, 3.0f, 0.3f);    // Left elevated
    addPlatform(8.0f, -1.0f, 3.0f, 0.3f);     // Right elevated
    addPlatform(0.0f, 2.0f, 2.5f, 0.3f);      // Center floating
    addPlatform(-4.0f, 4.5f, 1.5f, 0.2f);     // Top left
    addPlatform(4.0f, 4.5f, 1.5f, 0.2f);      // Top right

    m_spawnPoints = {
        {-10.0f, -3.5f}, { 10.0f, -3.5f},
        { -5.0f, -3.5f}, {  5.0f, -3.5f},
    };
}

b2Vec2 Arena::getRandomPlatformTop() const {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, m_platforms.size() - 1);
    const auto& p = m_platforms[dist(rng)];

    std::uniform_real_distribution<float> xDist(-p.halfWidth * 0.8f, p.halfWidth * 0.8f);
    return {p.cx + xDist(rng), p.cy + p.halfHeight + 0.5f};
}

void Arena::draw(sf::RenderTarget& target) const {
    auto toScreen = [](float x, float y) -> sf::Vector2f {
        return {640.0f + x * PPM, 360.0f - y * PPM};
    };

    for (const auto& p : m_platforms) {
        sf::RectangleShape rect({p.halfWidth * 2.0f * PPM, p.halfHeight * 2.0f * PPM});
        rect.setPosition(toScreen(p.cx - p.halfWidth, p.cy + p.halfHeight));
        rect.setFillColor(sf::Color(80, 80, 80));
        rect.setOutlineColor(sf::Color(150, 150, 150));
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }
}

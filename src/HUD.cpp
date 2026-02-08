#include "HUD.h"
#include "StickFigure.h"
#include <sstream>
#include <iomanip>

bool HUD::init() {
    // SFML 3: Font constructor takes a path and throws on failure
    // Try multiple font locations
    const char* fontPaths[] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "assets/fonts/default.ttf",
    };

    for (const char* path : fontPaths) {
        try {
            m_font.emplace(path);
            m_fontLoaded = true;
            break;
        } catch (...) {
            // Try next font
        }
    }

    return true;
}

void HUD::draw(sf::RenderTarget& target, const std::vector<std::unique_ptr<StickFigure>>& players,
               float roundTime) {
    float barWidth = 200.0f;
    float barHeight = 20.0f;
    float margin = 20.0f;
    float spacing = 35.0f;

    for (size_t i = 0; i < players.size(); i++) {
        const auto& p = players[i];
        float x = margin;
        float y = margin + static_cast<float>(i) * (barHeight + spacing);

        if (i == 1) {
            x = static_cast<float>(target.getSize().x) - margin - barWidth;
        }

        float healthPct = p->getHealth() / p->getMaxHealth();
        drawHealthBar(target, x, y, barWidth, barHeight, healthPct, p->getColor());

        if (m_fontLoaded && m_font) {
            std::stringstream ss;
            ss << "P" << (i + 1) << " | " << p->getCurrentWeapon().name
               << " | Lives: " << p->getLives();
            if (p->getAmmo() >= 0) {
                ss << " | Ammo: " << p->getAmmo();
            }

            // SFML 3: Text constructor takes (font, string, charSize)
            sf::Text label(*m_font, ss.str(), 14);
            label.setFillColor(p->getColor());
            label.setPosition({x, y + barHeight + 2.0f});
            target.draw(label);
        }
    }

    // Round timer
    if (m_fontLoaded && m_font) {
        int minutes = static_cast<int>(roundTime) / 60;
        int seconds = static_cast<int>(roundTime) % 60;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << minutes << ":"
           << std::setfill('0') << std::setw(2) << seconds;

        sf::Text timer(*m_font, ss.str(), 24);
        timer.setFillColor(sf::Color::White);
        sf::FloatRect bounds = timer.getLocalBounds();
        timer.setPosition({(static_cast<float>(target.getSize().x) - bounds.size.x) / 2.0f, 15.0f});
        target.draw(timer);
    }
}

void HUD::drawHealthBar(sf::RenderTarget& target, float x, float y, float width, float height,
                         float healthPercent, sf::Color color) {
    // Background
    sf::RectangleShape bg({width, height});
    bg.setPosition({x, y});
    bg.setFillColor(sf::Color(40, 40, 40));
    bg.setOutlineColor(sf::Color(100, 100, 100));
    bg.setOutlineThickness(1.0f);
    target.draw(bg);

    // Health fill
    sf::RectangleShape fill({width * healthPercent, height});
    fill.setPosition({x, y});

    sf::Color healthColor = color;
    if (healthPercent < 0.3f) {
        healthColor = sf::Color(200, 50, 50);
    } else if (healthPercent < 0.6f) {
        healthColor = sf::Color(200, 150, 50);
    }
    fill.setFillColor(healthColor);
    target.draw(fill);
}

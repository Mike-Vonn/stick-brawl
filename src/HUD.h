#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <optional>

class StickFigure;

class HUD {
public:
    bool init();
    void draw(sf::RenderTarget& target, const std::vector<std::unique_ptr<StickFigure>>& players,
              float roundTime);

private:
    std::optional<sf::Font> m_font;
    bool m_fontLoaded = false;

    void drawHealthBar(sf::RenderTarget& target, float x, float y, float width, float height,
                       float healthPercent, sf::Color color);
};

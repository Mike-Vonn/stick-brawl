#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

class Renderer {
public:
    bool init(unsigned int width, unsigned int height, const std::string& title);
    void clear(sf::Color color = sf::Color(30, 30, 35));
    void display();
    bool isOpen() const;

    sf::RenderWindow& getWindow() { return *m_window; }

private:
    // SFML 3 removed default constructors, so we use optional
    std::optional<sf::RenderWindow> m_window;
};

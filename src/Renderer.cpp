#include "Renderer.h"

bool Renderer::init(unsigned int width, unsigned int height, const std::string& title) {
    // SFML 3: constructor takes VideoMode (using Vector2u), title, style
    m_window.emplace(sf::VideoMode({width, height}), title, sf::Style::Close | sf::Style::Titlebar);
    m_window->setFramerateLimit(60);
    return m_window->isOpen();
}

void Renderer::clear(sf::Color color) {
    if (m_window) m_window->clear(color);
}

void Renderer::display() {
    if (m_window) m_window->display();
}

bool Renderer::isOpen() const {
    return m_window && m_window->isOpen();
}

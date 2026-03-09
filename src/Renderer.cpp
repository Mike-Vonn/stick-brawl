#include "Renderer.h"

bool Renderer::init(unsigned int width, unsigned int height, const std::string& title) {
    m_width = width;
    m_height = height;
    m_title = title;
    m_window.emplace(sf::VideoMode({width, height}), title, sf::Style::Close | sf::Style::Titlebar);
    m_window->setFramerateLimit(60);
    return m_window->isOpen();
}

void Renderer::toggleFullscreen() {
    m_fullscreen = !m_fullscreen;
    m_window.reset();
    if (m_fullscreen) {
        m_window.emplace(sf::VideoMode::getDesktopMode(), m_title, sf::Style::None);
        // Use a view that matches our game resolution so rendering stays consistent
        sf::View view(sf::FloatRect({0.f, 0.f}, {static_cast<float>(m_width), static_cast<float>(m_height)}));
        m_window->setView(view);
    } else {
        m_window.emplace(sf::VideoMode({m_width, m_height}), m_title, sf::Style::Close | sf::Style::Titlebar);
    }
    m_window->setFramerateLimit(60);
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

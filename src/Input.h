#pragma once
#include <SFML/Window.hpp>
#include <array>

constexpr int MAX_PLAYERS = 5;

struct PlayerInput {
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool attack = false;
    bool aimUp = false;
    bool aimDown = false;
    bool jumpPressed = false;
    bool attackPressed = false;
};

class Input {
public:
    Input();
    void update();
    PlayerInput getPlayerInput(int playerIndex) const;

private:
    struct KeyBinding {
        sf::Keyboard::Key left;
        sf::Keyboard::Key right;
        sf::Keyboard::Key jump;
        sf::Keyboard::Key attack;
        sf::Keyboard::Key aimUp;
        sf::Keyboard::Key aimDown;
    };

    std::array<KeyBinding, MAX_PLAYERS> m_keyBindings;
    std::array<bool, MAX_PLAYERS> m_prevJump = {};
    std::array<bool, MAX_PLAYERS> m_prevAttack = {};
};

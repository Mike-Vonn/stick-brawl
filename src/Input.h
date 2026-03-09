#pragma once
#include <SFML/Window.hpp>
#include <array>

constexpr int MAX_PLAYERS = 5;

struct PlayerInput {
    bool moveLeft = false;
    bool moveRight = false;
    bool moveDown = false;
    bool jump = false;
    bool attack = false;
    bool aimUp = false;
    bool aimDown = false;
    bool jumpPressed = false;
    bool attackPressed = false;
    bool throwPressed = false;
};

class Input {
public:
    Input();
    void update();
    PlayerInput getPlayerInput(int playerIndex) const;
    bool isGamepadConnected(int playerIndex) const;

private:
    struct KeyBinding {
        sf::Keyboard::Key left;
        sf::Keyboard::Key right;
        sf::Keyboard::Key jump;
        sf::Keyboard::Key attack;
        sf::Keyboard::Key aimUp;
        sf::Keyboard::Key aimDown;
        sf::Keyboard::Key throwWeapon;
        sf::Keyboard::Key down;
    };

    static constexpr unsigned int GP_A     = 0;
    static constexpr unsigned int GP_B     = 1;
    static constexpr unsigned int GP_X     = 2;
    static constexpr unsigned int GP_Y     = 3;
    static constexpr unsigned int GP_LB    = 4;
    static constexpr unsigned int GP_RB    = 5;

    static constexpr float STICK_DEADZONE = 25.0f;

    std::array<KeyBinding, MAX_PLAYERS> m_keyBindings;
    std::array<bool, MAX_PLAYERS> m_prevJump = {};
    std::array<bool, MAX_PLAYERS> m_prevAttack = {};
    std::array<bool, MAX_PLAYERS> m_prevThrow = {};
    std::array<bool, MAX_PLAYERS> m_prevGpJump = {};
    std::array<bool, MAX_PLAYERS> m_prevGpAttack = {};
    std::array<bool, MAX_PLAYERS> m_prevGpThrow = {};
};

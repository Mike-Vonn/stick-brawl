#include "Input.h"
#include <cmath>

Input::Input() {
    // Player 0: WASD + F attack, E/Q aim, G throw, S down
    m_keyBindings[0] = { sf::Keyboard::Key::A, sf::Keyboard::Key::D,
                         sf::Keyboard::Key::W, sf::Keyboard::Key::F,
                         sf::Keyboard::Key::E, sf::Keyboard::Key::Q,
                         sf::Keyboard::Key::G, sf::Keyboard::Key::S };

    // Player 1: Arrows + RCtrl attack, RShift/Numpad0 aim, Enter throw, Down
    m_keyBindings[1] = { sf::Keyboard::Key::Left, sf::Keyboard::Key::Right,
                         sf::Keyboard::Key::Up, sf::Keyboard::Key::RControl,
                         sf::Keyboard::Key::RShift, sf::Keyboard::Key::Numpad0,
                         sf::Keyboard::Key::Enter, sf::Keyboard::Key::Down };

    // Player 2: IJKL + H attack, U/O aim, Y throw, K down
    m_keyBindings[2] = { sf::Keyboard::Key::J, sf::Keyboard::Key::L,
                         sf::Keyboard::Key::I, sf::Keyboard::Key::H,
                         sf::Keyboard::Key::U, sf::Keyboard::Key::O,
                         sf::Keyboard::Key::Y, sf::Keyboard::Key::K };

    // Player 3: Numpad 4/6/8 + Numpad5 attack, Numpad7/9 aim, Numpad1 throw, Numpad2 down
    m_keyBindings[3] = { sf::Keyboard::Key::Numpad4, sf::Keyboard::Key::Numpad6,
                         sf::Keyboard::Key::Numpad8, sf::Keyboard::Key::Numpad5,
                         sf::Keyboard::Key::Numpad7, sf::Keyboard::Key::Numpad9,
                         sf::Keyboard::Key::Numpad1, sf::Keyboard::Key::Numpad2 };

    // Player 4: ZXCV cluster, M throw, D down
    m_keyBindings[4] = { sf::Keyboard::Key::Z, sf::Keyboard::Key::C,
                         sf::Keyboard::Key::X, sf::Keyboard::Key::V,
                         sf::Keyboard::Key::B, sf::Keyboard::Key::N,
                         sf::Keyboard::Key::M, sf::Keyboard::Key::D };
}

bool Input::isGamepadConnected(int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    return sf::Joystick::isConnected(static_cast<unsigned int>(playerIndex));
}

void Input::update() {
    sf::Joystick::update();
    for (int i = 0; i < MAX_PLAYERS; i++) {
        m_prevJump[i] = sf::Keyboard::isKeyPressed(m_keyBindings[i].jump);
        m_prevAttack[i] = sf::Keyboard::isKeyPressed(m_keyBindings[i].attack);
        m_prevThrow[i] = sf::Keyboard::isKeyPressed(m_keyBindings[i].throwWeapon);
        unsigned int gpIdx = static_cast<unsigned int>(i);
        if (sf::Joystick::isConnected(gpIdx)) {
            m_prevGpJump[i] = sf::Joystick::isButtonPressed(gpIdx, GP_A);
            m_prevGpAttack[i] = sf::Joystick::isButtonPressed(gpIdx, GP_X) ||
                                sf::Joystick::isButtonPressed(gpIdx, GP_RB);
            m_prevGpThrow[i] = sf::Joystick::isButtonPressed(gpIdx, GP_B);
        } else {
            m_prevGpJump[i] = false;
            m_prevGpAttack[i] = false;
            m_prevGpThrow[i] = false;
        }
    }
}

PlayerInput Input::getPlayerInput(int playerIndex) const {
    PlayerInput pi;
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return pi;

    const auto& kb = m_keyBindings[playerIndex];
    bool kbJump   = sf::Keyboard::isKeyPressed(kb.jump);
    bool kbAttack = sf::Keyboard::isKeyPressed(kb.attack);
    bool kbThrow  = sf::Keyboard::isKeyPressed(kb.throwWeapon);

    pi.moveLeft  = sf::Keyboard::isKeyPressed(kb.left);
    pi.moveRight = sf::Keyboard::isKeyPressed(kb.right);
    pi.moveDown  = sf::Keyboard::isKeyPressed(kb.down);
    pi.jump      = kbJump;
    pi.attack    = kbAttack;
    pi.aimUp     = sf::Keyboard::isKeyPressed(kb.aimUp);
    pi.aimDown   = sf::Keyboard::isKeyPressed(kb.aimDown);

    pi.jumpPressed   = kbJump && !m_prevJump[playerIndex];
    pi.attackPressed = kbAttack && !m_prevAttack[playerIndex];
    pi.throwPressed  = kbThrow && !m_prevThrow[playerIndex];

    // Gamepad
    unsigned int gpIdx = static_cast<unsigned int>(playerIndex);
    if (!sf::Joystick::isConnected(gpIdx)) return pi;

    float lsX = sf::Joystick::getAxisPosition(gpIdx, sf::Joystick::Axis::X);
    if (lsX < -STICK_DEADZONE) pi.moveLeft = true;
    if (lsX >  STICK_DEADZONE) pi.moveRight = true;

    float dpX = sf::Joystick::getAxisPosition(gpIdx, sf::Joystick::Axis::PovX);
    float dpY = sf::Joystick::getAxisPosition(gpIdx, sf::Joystick::Axis::PovY);
    if (dpX < -50.0f) pi.moveLeft = true;
    if (dpX >  50.0f) pi.moveRight = true;
    if (dpY >  50.0f) pi.aimUp = true;
    if (dpY < -50.0f) { pi.aimDown = true; pi.moveDown = true; }

    float lsY = sf::Joystick::getAxisPosition(gpIdx, sf::Joystick::Axis::Y);
    if (lsY < -STICK_DEADZONE) pi.aimUp = true;
    if (lsY >  STICK_DEADZONE) { pi.aimDown = true; pi.moveDown = true; }

    if (sf::Joystick::hasAxis(gpIdx, sf::Joystick::Axis::V)) {
        float rsY = sf::Joystick::getAxisPosition(gpIdx, sf::Joystick::Axis::V);
        if (rsY < -STICK_DEADZONE) pi.aimUp = true;
        if (rsY >  STICK_DEADZONE) pi.aimDown = true;
    }

    if (sf::Joystick::hasAxis(gpIdx, sf::Joystick::Axis::U)) {
        float rsX = sf::Joystick::getAxisPosition(gpIdx, sf::Joystick::Axis::U);
        if (rsX < -STICK_DEADZONE) pi.moveLeft = true;
        if (rsX >  STICK_DEADZONE) pi.moveRight = true;
    }

    bool gpJump = sf::Joystick::isButtonPressed(gpIdx, GP_A);
    if (gpJump) pi.jump = true;
    if (gpJump && !m_prevGpJump[playerIndex]) pi.jumpPressed = true;

    bool gpAttack = sf::Joystick::isButtonPressed(gpIdx, GP_X) ||
                    sf::Joystick::isButtonPressed(gpIdx, GP_RB);
    if (gpAttack) pi.attack = true;
    if (gpAttack && !m_prevGpAttack[playerIndex]) pi.attackPressed = true;

    if (sf::Joystick::isButtonPressed(gpIdx, GP_LB)) pi.aimDown = true;

    bool gpThrow = sf::Joystick::isButtonPressed(gpIdx, GP_B);
    if (gpThrow && !m_prevGpThrow[playerIndex]) pi.throwPressed = true;

    if (sf::Joystick::isButtonPressed(gpIdx, GP_Y)) pi.aimUp = true;

    if (sf::Joystick::hasAxis(gpIdx, sf::Joystick::Axis::Z)) {
        float triggers = sf::Joystick::getAxisPosition(gpIdx, sf::Joystick::Axis::Z);
        if (triggers > 50.0f) {
            pi.attack = true;
            if (!m_prevGpAttack[playerIndex]) pi.attackPressed = true;
        }
    }

    return pi;
}

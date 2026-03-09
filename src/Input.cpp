#include "Input.h"

Input::Input() {
    // Player 0: WASD + F attack, E/Q aim, R swap
    m_keyBindings[0] = { sf::Keyboard::Key::A, sf::Keyboard::Key::D,
                         sf::Keyboard::Key::W, sf::Keyboard::Key::F,
                         sf::Keyboard::Key::E, sf::Keyboard::Key::Q,
                         sf::Keyboard::Key::R };

    // Player 1: Arrows + RCtrl attack, RShift/Numpad0 aim, Down swap
    m_keyBindings[1] = { sf::Keyboard::Key::Left, sf::Keyboard::Key::Right,
                         sf::Keyboard::Key::Up, sf::Keyboard::Key::RControl,
                         sf::Keyboard::Key::RShift, sf::Keyboard::Key::Numpad0,
                         sf::Keyboard::Key::Down };

    // Player 2: IJKL + H attack, U/O aim, K swap
    m_keyBindings[2] = { sf::Keyboard::Key::J, sf::Keyboard::Key::L,
                         sf::Keyboard::Key::I, sf::Keyboard::Key::H,
                         sf::Keyboard::Key::U, sf::Keyboard::Key::O,
                         sf::Keyboard::Key::K };

    // Player 3: Numpad 4/6/8 + Numpad5 attack, Numpad7/9 aim, Numpad2 swap
    m_keyBindings[3] = { sf::Keyboard::Key::Numpad4, sf::Keyboard::Key::Numpad6,
                         sf::Keyboard::Key::Numpad8, sf::Keyboard::Key::Numpad5,
                         sf::Keyboard::Key::Numpad7, sf::Keyboard::Key::Numpad9,
                         sf::Keyboard::Key::Numpad2 };

    // Player 4: ZXCV cluster — Z/C move, X jump, V attack, B/N aim, M swap
    m_keyBindings[4] = { sf::Keyboard::Key::Z, sf::Keyboard::Key::C,
                         sf::Keyboard::Key::X, sf::Keyboard::Key::V,
                         sf::Keyboard::Key::B, sf::Keyboard::Key::N,
                         sf::Keyboard::Key::M };
}

void Input::update() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerInput pi = getPlayerInput(i);
        m_prevJump[i] = pi.jump;
        m_prevAttack[i] = pi.attack;
        m_prevSwap[i] = sf::Keyboard::isKeyPressed(m_keyBindings[i].swap);
    }
}

PlayerInput Input::getPlayerInput(int playerIndex) const {
    PlayerInput pi;

    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return pi;

    const auto& kb = m_keyBindings[playerIndex];
    pi.moveLeft  = sf::Keyboard::isKeyPressed(kb.left);
    pi.moveRight = sf::Keyboard::isKeyPressed(kb.right);
    pi.jump      = sf::Keyboard::isKeyPressed(kb.jump);
    pi.attack    = sf::Keyboard::isKeyPressed(kb.attack);
    pi.aimUp     = sf::Keyboard::isKeyPressed(kb.aimUp);
    pi.aimDown   = sf::Keyboard::isKeyPressed(kb.aimDown);

    pi.jumpPressed   = pi.jump && !m_prevJump[playerIndex];
    pi.attackPressed  = pi.attack && !m_prevAttack[playerIndex];

    bool swapHeld = sf::Keyboard::isKeyPressed(kb.swap);
    pi.swapPressed = swapHeld && !m_prevSwap[playerIndex];

    return pi;
}

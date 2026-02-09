#include "Game.h"
#include <iostream>

int main() {
    std::cout << "=== StickBrawl ===\n";
    std::cout << "Controls:\n";
    std::cout << "  Player 1: WASD move, F attack, E/Q aim up/down\n";
    std::cout << "  Player 2: Arrows move, RCtrl attack, RShift/Num0 aim\n";
    std::cout << "  Player 3: IJKL move, H attack, U/O aim\n";
    std::cout << "  Player 4: Numpad 4/6/8 move, Num5 attack, Num7/9 aim\n";
    std::cout << "  Player 5: Z/C move, X jump, V attack, B/N aim\n";
    std::cout << "\n";
    std::cout << "  Character Select: LEFT/RIGHT to pick character, ATK to ready\n";
    std::cout << "  TAB to cycle levels | ENTER/SPACE to start\n";
    std::cout << "  Walk over glowing boxes to pick up weapons!\n";
    std::cout << "  ESC to quit, R to restart round, Backspace to return to select\n\n";

    Game game;
    if (!game.init()) {
        std::cerr << "Failed to initialize game\n";
        return 1;
    }

    game.run();
    return 0;
}

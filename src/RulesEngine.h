#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct GameRules {
    float maxHealth = 100.0f;
    float gravityX = 0.0f;
    float gravityY = -20.0f;
    float roundTimeSeconds = 120.0f;
    int   livesPerPlayer = 3;
    int   maxPlayers = 4;
    bool  friendlyFire = true;
    float weaponSpawnInterval = 5.0f;
    int   weaponSpawnMax = 3;
    float fallDeathY = -20.0f;
    float respawnDelay = 2.0f;
    float knockbackMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
};

class RulesEngine {
public:
    bool loadFromFile(const std::string& path);
    const GameRules& getRules() const { return m_rules; }

private:
    GameRules m_rules;
};

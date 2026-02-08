#include "RulesEngine.h"
#include <fstream>
#include <iostream>

bool RulesEngine::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[RulesEngine] Failed to open: " << path << "\n";
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("max_health"))                  m_rules.maxHealth = j["max_health"];
        if (j.contains("gravity_x"))                    m_rules.gravityX = j["gravity_x"];
        if (j.contains("gravity_y"))                    m_rules.gravityY = j["gravity_y"];
        if (j.contains("round_time_seconds"))           m_rules.roundTimeSeconds = j["round_time_seconds"];
        if (j.contains("lives_per_player"))             m_rules.livesPerPlayer = j["lives_per_player"];
        if (j.contains("max_players"))                  m_rules.maxPlayers = j["max_players"];
        if (j.contains("friendly_fire"))                m_rules.friendlyFire = j["friendly_fire"];
        if (j.contains("weapon_spawn_interval_seconds"))m_rules.weaponSpawnInterval = j["weapon_spawn_interval_seconds"];
        if (j.contains("weapon_spawn_max"))             m_rules.weaponSpawnMax = j["weapon_spawn_max"];
        if (j.contains("fall_death_y"))                 m_rules.fallDeathY = j["fall_death_y"];
        if (j.contains("respawn_delay_seconds"))        m_rules.respawnDelay = j["respawn_delay_seconds"];
        if (j.contains("knockback_multiplier"))         m_rules.knockbackMultiplier = j["knockback_multiplier"];
        if (j.contains("damage_multiplier"))            m_rules.damageMultiplier = j["damage_multiplier"];

        std::cout << "[RulesEngine] Loaded rules from: " << path << "\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[RulesEngine] Parse error: " << e.what() << "\n";
        return false;
    }
}

#include "Weapon.h"
#include <fstream>
#include <iostream>

WeaponType parseWeaponType(const std::string& s) {
    if (s == "projectile") return WeaponType::Projectile;
    if (s == "explosive")  return WeaponType::Explosive;
    return WeaponType::Melee;
}

WeaponData loadWeaponFromFile(const std::string& path) {
    WeaponData w;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Weapon] Failed to open: " << path << "\n";
        return w;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("name"))                        w.name = j["name"];
        if (j.contains("type"))                         w.type = parseWeaponType(j["type"]);
        if (j.contains("damage"))                       w.damage = j["damage"];
        if (j.contains("knockback_force"))              w.knockbackForce = j["knockback_force"];
        if (j.contains("range"))                        w.range = j["range"];
        if (j.contains("attack_rate_seconds"))          w.attackRate = j["attack_rate_seconds"];
        if (j.contains("projectile_speed"))             w.projectileSpeed = j["projectile_speed"];
        if (j.contains("projectile_lifetime_seconds"))  w.projectileLifetime = j["projectile_lifetime_seconds"];
        if (j.contains("ammo"))                         w.ammo = j["ammo"];
        if (j.contains("spread_degrees"))               w.spreadDegrees = j["spread_degrees"];
        if (j.contains("pellet_count"))                  w.pelletCount = j["pellet_count"];
        if (j.contains("affected_by_gravity"))          w.affectedByGravity = j["affected_by_gravity"];
        if (j.contains("explosion_radius"))             w.explosionRadius = j["explosion_radius"];
        if (j.contains("destroys_platforms"))            w.destroysPlatforms = j["destroys_platforms"];
        if (j.contains("env_damage_radius"))             w.envDamageRadius = j["env_damage_radius"];
        if (j.contains("poison_dps"))                   w.poisonDps = j["poison_dps"];
        if (j.contains("poison_duration"))              w.poisonDuration = j["poison_duration"];
        if (j.contains("sprite"))                       w.sprite = j["sprite"];
        if (j.contains("projectile_sprite"))            w.projectileSprite = j["projectile_sprite"];
        if (j.contains("sound_hit"))                    w.soundHit = j["sound_hit"];
        if (j.contains("sound_fire"))                   w.soundFire = j["sound_fire"];
        if (j.contains("sound_explode"))                w.soundExplode = j["sound_explode"];
        if (j.contains("description"))                  w.description = j["description"];

        std::cout << "[Weapon] Loaded: " << w.name << " (" << path << ")\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[Weapon] Parse error in " << path << ": " << e.what() << "\n";
    }
    return w;
}

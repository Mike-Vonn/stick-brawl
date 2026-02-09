#pragma once
#include <string>
#include <nlohmann/json.hpp>

enum class WeaponType {
    Melee,
    Projectile,
    Explosive
};

struct WeaponData {
    std::string name = "Fists";
    WeaponType  type = WeaponType::Melee;
    float       damage = 10.0f;
    float       knockbackForce = 5.0f;
    float       range = 1.2f;
    float       attackRate = 0.3f;

    // Projectile / explosive
    float       projectileSpeed = 0.0f;
    float       projectileLifetime = 0.0f;
    int         ammo = -1;  // -1 = infinite
    float       spreadDegrees = 0.0f;
    int         pelletCount = 1;     // >1 for shotgun-style multi-projectile
    bool        affectedByGravity = false;

    // Explosive
    float       explosionRadius = 0.0f;
    bool        destroysPlatforms = false;

    // Environmental damage (all weapons can chip terrain)
    float       envDamageRadius = 0.0f;  // 0 = auto-calculate from damage

    // Poison (damage-over-time)
    float       poisonDps = 0.0f;
    float       poisonDuration = 0.0f;

    // Assets
    std::string sprite;
    std::string projectileSprite;
    std::string soundHit;
    std::string soundFire;
    std::string soundExplode;
    std::string description;
};

WeaponType parseWeaponType(const std::string& s);
WeaponData loadWeaponFromFile(const std::string& path);

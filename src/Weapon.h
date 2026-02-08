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
    bool        affectedByGravity = false;

    // Explosive
    float       explosionRadius = 0.0f;

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

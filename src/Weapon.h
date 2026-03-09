#pragma once
#include <string>
#include <nlohmann/json.hpp>

enum class WeaponType { Melee, Projectile, Explosive };

// Grenade charge type — what kind of explosion it produces
enum class GrenadeCharge { Normal, Freeze, Time, Lava, Nuclear, Frag, Implode };

// Grenade casing type — how the grenade behaves before detonating
enum class GrenadeCasing { Normal, Sticky, Mine, Ballistic, Bounce };

GrenadeCharge parseGrenadeCharge(const std::string& s);
GrenadeCasing parseGrenadeCasing(const std::string& s);
std::string grenadeChargeName(GrenadeCharge c);
std::string grenadeCasingName(GrenadeCasing c);

struct WeaponData {
    std::string name        = "Fists";
    WeaponType  type        = WeaponType::Melee;
    float  damage           = 10.0f;
    float  knockbackForce   = 5.0f;
    float  range            = 1.2f;
    float  attackRate       = 0.3f;

    // Projectile
    float  projectileSpeed    = 0.0f;
    float  projectileLifetime = 0.0f;
    int    ammo               = -1;
    float  spreadDegrees      = 0.0f;
    int    pelletCount        = 1;
    bool   affectedByGravity  = false;

    // Explosive
    float  explosionRadius   = 0.0f;
    bool   destroysPlatforms = false;
    float  envDamageRadius   = 0.0f;

    // Poison
    float  poisonDps      = 0.0f;
    float  poisonDuration = 0.0f;

    // Teleport dagger
    float  teleportDistance  = 0.0f;
    float  teleportCooldown  = 0.0f;

    // Slow / freeze
    float  slowEffect        = 0.0f;   // 0-1: fraction of speed removed (0.9 = 90% slower)
    float  slowDuration      = 0.0f;
    float  freezeBuildup     = 0.0f;   // freeze stacks to add per hit (1=freeze gun, 10=freeze grenade)
    float  freezeDuration    = 0.0f;   // unused (stacks decay naturally)

    // Time slow (area) — full stop effect
    float  timeSlowFactor    = 0.0f;   // 0<x<1 = time multiplier when inside zone
    float  timeSlowDuration  = 0.0f;   // duration of time-stop on players
    float  timeSlowRadius    = 0.0f;

    // Chain lightning
    int    chainTargets       = 0;
    float  chainRange         = 0.0f;
    float  chainDamageFalloff = 1.0f;
    float  stunDuration       = 0.0f;

    // Black hole
    float  blackHoleDuration  = 0.0f;
    float  blackHolePullForce = 0.0f;
    float  blackHoleRadius    = 0.0f;
    float  damagePerSecond    = 0.0f;

    // Healing
    float  healingRadius = 0.0f;
    float  healOverTime  = 0.0f;
    float  healDuration  = 0.0f;

    // Boomerang
    bool   returnsToSender  = false;
    bool   hitsMultipleTimes = false;

    // Bouncing
    int    bounces               = 0;
    float  bounceEnergyRetention = 1.0f;

    // Sticky explosive
    bool   sticksToSurfaces = false;
    float  detonationDelay  = 0.0f;

    // Grenade system (charge + casing)
    GrenadeCharge grenadeCharge = GrenadeCharge::Normal;
    GrenadeCasing grenadeCasing = GrenadeCasing::Normal;
    float  lavaDps             = 0.0f;   // lava charge: DPS of lava pool
    float  lavaDuration        = 0.0f;   // lava charge: how long lava pool lasts
    float  lavaRadius          = 0.0f;   // lava charge: radius of lava pool
    int    fragChildCount      = 4;      // frag charge: number of sub-grenades
    float  fragChildDamage     = 0.0f;   // frag charge: damage of each sub-grenade
    float  fragChildRadius     = 0.0f;   // frag charge: explosion radius of sub-grenades
    float  mineProximity       = 0.0f;   // mine casing: trigger distance
    float  implodePullRadius   = 0.0f;   // implode charge: pull-in radius
    float  implodePullDuration = 0.0f;   // implode charge: how long to pull before exploding
    bool   isFragChild         = false;  // internal: is this a sub-grenade from frag?
    bool   isGrenadeLauncher   = false;  // internal: this weapon uses the random grenade system
    bool   isLavaThrower       = false;  // this weapon shoots lava particles
    bool   isGrapple           = false;  // grappling hook weapon
    float  grappleRetractSpeed = 12.0f;
    float  grappleExtendSpeed  = 8.0f;
    float  grappleSwingForce   = 25.0f;
    float  grappleMaxLength    = 25.0f;
    float  grappleMinLength    = 1.5f;
    float  grappleSlingshotForce = 35.0f;
    int    spawnWeight         = 1;      // how many times to add to the eligible pool

    // Melee special
    float  shockwaveRadius  = 0.0f;
    float  shockwaveDamage  = 0.0f;
    float  shockwaveForce   = 0.0f;
    float  dashDistance     = 0.0f;
    float  dashDamage       = 0.0f;
    float  pullForce        = 0.0f;
    bool   canPullEnemies   = false;

    // Shield
    bool   blocksProjectiles    = false;
    bool   reflectsProjectiles  = false;
    float  damageReduction      = 0.0f;

    // Portal
    float  portalDuration = 0.0f;

    // Shrink
    float  shrinkScale    = 1.0f;
    float  shrinkDuration = 0.0f;
    float  speedIncrease  = 1.0f;

    // Meteor shower
    int    meteorCount        = 0;
    float  meteorSpreadRadius = 0.0f;
    float  meteorDelay        = 0.0f;

    // Tornado
    float  tornadoLiftForce = 0.0f;
    float  tornadoSpinForce = 0.0f;
    float  tornadoRadius    = 0.0f;

    // Swap / mind control
    bool   swapsPositions       = false;
    float  mindControlDuration  = 0.0f;
    bool   controlBreaksOnDamage = false;

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

#include "Weapon.h"
#include <fstream>
#include <iostream>

WeaponType parseWeaponType(const std::string& s) {
    if (s == "projectile") return WeaponType::Projectile;
    if (s == "explosive")  return WeaponType::Explosive;
    return WeaponType::Melee;
}

GrenadeCharge parseGrenadeCharge(const std::string& s) {
    if (s == "freeze")  return GrenadeCharge::Freeze;
    if (s == "time")    return GrenadeCharge::Time;
    if (s == "lava")    return GrenadeCharge::Lava;
    if (s == "nuclear") return GrenadeCharge::Nuclear;
    if (s == "frag")    return GrenadeCharge::Frag;
    if (s == "implode") return GrenadeCharge::Implode;
    return GrenadeCharge::Normal;
}

GrenadeCasing parseGrenadeCasing(const std::string& s) {
    if (s == "sticky")    return GrenadeCasing::Sticky;
    if (s == "mine")      return GrenadeCasing::Mine;
    if (s == "ballistic") return GrenadeCasing::Ballistic;
    if (s == "bounce")    return GrenadeCasing::Bounce;
    return GrenadeCasing::Normal;
}

std::string grenadeChargeName(GrenadeCharge c) {
    switch (c) {
        case GrenadeCharge::Freeze:  return "Freeze";
        case GrenadeCharge::Time:    return "Time";
        case GrenadeCharge::Lava:    return "Lava";
        case GrenadeCharge::Nuclear: return "Nuclear";
        case GrenadeCharge::Frag:    return "Frag";
        case GrenadeCharge::Implode: return "Implode";
        default:                     return "Normal";
    }
}

std::string grenadeCasingName(GrenadeCasing c) {
    switch (c) {
        case GrenadeCasing::Sticky:    return "Sticky";
        case GrenadeCasing::Mine:      return "Mine";
        case GrenadeCasing::Ballistic: return "Ballistic";
        case GrenadeCasing::Bounce:    return "Bouncy";
        default:                       return "Normal";
    }
}

WeaponData loadWeaponFromFile(const std::string& path) {
    WeaponData w;
    std::ifstream file(path);
    if (!file.is_open()) { std::cerr << "[Weapon] Failed to open: " << path << "\n"; return w; }
    try {
        nlohmann::json j; file >> j;
        auto g = [&](auto key, auto& val) { if (j.contains(key)) val = j[key]; };

        g("name", w.name);
        if (j.contains("type")) w.type = parseWeaponType(j["type"]);
        g("damage", w.damage);
        g("knockback_force", w.knockbackForce);
        g("range", w.range);
        g("attack_rate_seconds", w.attackRate);
        g("projectile_speed", w.projectileSpeed);
        g("projectile_lifetime_seconds", w.projectileLifetime);
        g("ammo", w.ammo);
        g("spread_degrees", w.spreadDegrees);
        g("pellet_count", w.pelletCount);
        g("affected_by_gravity", w.affectedByGravity);
        g("explosion_radius", w.explosionRadius);
        g("destroys_platforms", w.destroysPlatforms);
        g("env_damage_radius", w.envDamageRadius);
        g("poison_dps", w.poisonDps);
        g("poison_duration", w.poisonDuration);
        g("teleport_distance", w.teleportDistance);
        g("teleport_cooldown", w.teleportCooldown);
        g("slow_effect", w.slowEffect);
        g("slow_duration", w.slowDuration);
        g("freeze_buildup", w.freezeBuildup);
        g("freeze_duration", w.freezeDuration);
        g("time_slow_factor", w.timeSlowFactor);
        g("time_slow_duration", w.timeSlowDuration);
        g("time_slow_radius", w.timeSlowRadius);
        g("chain_targets", w.chainTargets);
        g("chain_range", w.chainRange);
        g("chain_damage_falloff", w.chainDamageFalloff);
        g("stun_duration", w.stunDuration);
        g("black_hole_duration", w.blackHoleDuration);
        g("black_hole_pull_force", w.blackHolePullForce);
        g("black_hole_radius", w.blackHoleRadius);
        g("damage_per_second", w.damagePerSecond);
        g("healing_radius", w.healingRadius);
        g("heal_over_time", w.healOverTime);
        g("heal_duration", w.healDuration);
        g("returns_to_sender", w.returnsToSender);
        g("hits_multiple_times", w.hitsMultipleTimes);
        g("bounces", w.bounces);
        g("bounce_energy_retention", w.bounceEnergyRetention);
        g("sticks_to_surfaces", w.sticksToSurfaces);
        g("detonation_delay", w.detonationDelay);
        if (j.contains("grenade_charge")) w.grenadeCharge = parseGrenadeCharge(j["grenade_charge"]);
        if (j.contains("grenade_casing")) w.grenadeCasing = parseGrenadeCasing(j["grenade_casing"]);
        g("lava_dps", w.lavaDps);
        g("lava_duration", w.lavaDuration);
        g("lava_radius", w.lavaRadius);
        g("frag_child_count", w.fragChildCount);
        g("frag_child_damage", w.fragChildDamage);
        g("frag_child_radius", w.fragChildRadius);
        g("mine_proximity", w.mineProximity);
        g("is_grenade_launcher", w.isGrenadeLauncher);
        g("is_lava_thrower", w.isLavaThrower);
        g("is_grapple", w.isGrapple);
        g("grapple_retract_speed", w.grappleRetractSpeed);
        g("grapple_extend_speed", w.grappleExtendSpeed);
        g("grapple_swing_force", w.grappleSwingForce);
        g("grapple_max_length", w.grappleMaxLength);
        g("grapple_min_length", w.grappleMinLength);
        g("grapple_slingshot_force", w.grappleSlingshotForce);
        g("spawn_weight", w.spawnWeight);
        g("shockwave_radius", w.shockwaveRadius);
        g("shockwave_damage", w.shockwaveDamage);
        g("shockwave_force", w.shockwaveForce);
        g("dash_distance", w.dashDistance);
        g("dash_damage", w.dashDamage);
        g("pull_force", w.pullForce);
        g("can_pull_enemies", w.canPullEnemies);
        g("blocks_projectiles", w.blocksProjectiles);
        g("reflects_projectiles", w.reflectsProjectiles);
        g("damage_reduction", w.damageReduction);
        g("portal_duration", w.portalDuration);
        g("shrink_scale", w.shrinkScale);
        g("shrink_duration", w.shrinkDuration);
        g("speed_increase", w.speedIncrease);
        g("meteor_count", w.meteorCount);
        g("meteor_spread_radius", w.meteorSpreadRadius);
        g("meteor_delay", w.meteorDelay);
        g("tornado_lift_force", w.tornadoLiftForce);
        g("tornado_spin_force", w.tornadoSpinForce);
        g("tornado_radius", w.tornadoRadius);
        g("swaps_positions", w.swapsPositions);
        g("mind_control_duration", w.mindControlDuration);
        g("control_breaks_on_damage", w.controlBreaksOnDamage);
        g("sprite", w.sprite);
        g("projectile_sprite", w.projectileSprite);
        g("sound_hit", w.soundHit);
        g("sound_fire", w.soundFire);
        g("sound_explode", w.soundExplode);
        g("description", w.description);

        std::cout << "[Weapon] Loaded: " << w.name << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[Weapon] Parse error in " << path << ": " << e.what() << "\n";
    }
    return w;
}

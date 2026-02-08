#pragma once
#include "Weapon.h"
#include <vector>
#include <string>
#include <unordered_map>

class WeaponFactory {
public:
    bool loadWeaponsFromDirectory(const std::string& dir);
    const WeaponData& getRandomWeapon() const;
    const WeaponData* getWeapon(const std::string& name) const;
    const WeaponData& getDefaultWeapon() const;
    const std::vector<WeaponData>& getAllWeapons() const { return m_weapons; }

private:
    std::vector<WeaponData> m_weapons;
    std::unordered_map<std::string, size_t> m_nameIndex;
    WeaponData m_defaultFists;
};

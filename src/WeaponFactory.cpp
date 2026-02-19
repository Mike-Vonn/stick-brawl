#include "WeaponFactory.h"
#include <filesystem>
#include <iostream>
#include <random>

namespace fs = std::filesystem;

bool WeaponFactory::loadWeaponsFromDirectory(const std::string& dir) {
    m_weapons.clear();
    m_nameIndex.clear();
    m_spawnableIndices.clear();

    if (!fs::exists(dir)) {
        std::cerr << "[WeaponFactory] Directory not found: " << dir << "\n";
        return false;
    }

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".json") {
            WeaponData w = loadWeaponFromFile(entry.path().string());
            size_t idx = m_weapons.size();
            m_nameIndex[w.name] = idx;
            if (w.spawnable) m_spawnableIndices.push_back(idx);
            m_weapons.push_back(std::move(w));
        }
    }

    std::cout << "[WeaponFactory] Loaded " << m_weapons.size() << " weapons ("
              << m_spawnableIndices.size() << " spawnable) from " << dir << "\n";
    return !m_weapons.empty();
}

const WeaponData* WeaponFactory::getRandomSpawnableWeapon() const {
    if (m_spawnableIndices.empty()) return nullptr;
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, m_spawnableIndices.size() - 1);
    return &m_weapons[m_spawnableIndices[dist(rng)]];
}

const WeaponData* WeaponFactory::getWeapon(const std::string& name) const {
    auto it = m_nameIndex.find(name);
    if (it != m_nameIndex.end()) return &m_weapons[it->second];
    return nullptr;
}

const WeaponData& WeaponFactory::getDefaultWeapon() const {
    auto* fists = getWeapon("Fists");
    return fists ? *fists : m_defaultFists;
}

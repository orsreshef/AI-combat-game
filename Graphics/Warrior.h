#pragma once
#include "Unit.h"

/**
 * Warrior class - Combat unit with shooting and grenade abilities
 * Uses A* for movement and attacks enemy units
 */
class Warrior : public Unit
{
private:
    int ammo;
    int grenades;
    bool needsAmmo;
    bool needsMedic;
    bool inDefenseMode;
    std::vector<std::pair<int, int>> currentPath;
    int pathIndex;
    int targetRow, targetCol;
    bool hasTarget;

public:
    /**
     * Constructor
     */
    Warrior(int startRow, int startCol, int teamId);

    /**
     * Update warrior behavior
     */
    void update(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    ) override;

    /**
     * Check ammo status and request supply if needed
     */
    bool checkAmmoStatus();

    /**
     * Check health and request medic if needed
     */
    bool checkHealthStatus();

    /**
     * Shoot at enemy if in range
     */
    bool shootAtEnemy(std::vector<Unit*>& allUnits, int map[][MAP_SIZE]);

    /**
     * Throw grenade at enemy
     */
    bool throwGrenade(std::vector<Unit*>& allUnits, int map[][MAP_SIZE]);

    /**
     * Find and attack nearest enemy
     */
    void attackMode(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    );

    /**
     * Find cover and defend
     */
    void defenseMode(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    );

    /**
     * Refill ammo
     */
    void refillAmmo(int amount, int grenadeAmount);

    // Getters
    bool getNeedsAmmo() const { return needsAmmo; }
    bool getNeedsMedic() const { return needsMedic; }
    int getAmmo() const { return ammo; }
    int getGrenades() const { return grenades; }
};

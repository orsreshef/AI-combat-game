#pragma once
#include "Unit.h"

// Forward declaration
class Warrior;

/**
 * Supply class - Delivers ammunition to warriors
 * Moves 4x faster than warriors directly to unit needing ammo
 * Has 1 charge - must return to depot after each resupply
 */
class Supply : public Unit
{
private:
    bool hasOrder;
    int targetUnitRow, targetUnitCol;
    std::vector<std::pair<int, int>> currentPath;
    int pathIndex;
    int ammoDepotRow, ammoDepotCol;
    int supplyCharges;
    bool needsRecharge;

public:
    /**
     * Constructor
     */
    Supply(int startRow, int startCol, int teamId);

    /**
     * Update supply behavior
     */
    void update(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    ) override;

    /**
     * Receive order to supply specific unit
     */
    void receiveOrder(int targetRow, int targetCol);

    /**
     * Find ammo depot location
     */
    void findAmmoDepot(int map[][MAP_SIZE]);

    /**
     * Check if supply has active order
     */
    bool hasActiveOrder() const { return hasOrder; }

    /**
     * Check if supply has charges available
     */
    bool hasCharges() const { return supplyCharges > 0 && !needsRecharge; }
};


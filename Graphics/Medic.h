#pragma once
#include "Unit.h"

/**
 * Medic class - Heals wounded warriors
 * Moves 4x faster than warriors directly to wounded unit
 * Has 1 charge - must return to depot after each heal
 */
class Medic : public Unit
{
private:
    bool hasOrder;
    int targetUnitRow, targetUnitCol;
    std::vector<std::pair<int, int>> currentPath;
    int pathIndex;
    int medicalDepotRow, medicalDepotCol;
    int healingCharges;
    bool needsRecharge;

public:
    /**
     * Constructor
     */
    Medic(int startRow, int startCol, int teamId);

    /**
     * Update medic behavior
     */
    void update(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    ) override;

    /**
     * Receive order to heal specific unit
     */
    void receiveOrder(int targetRow, int targetCol);

    /**
     * Find medical depot location
     */
    void findMedicalDepot(int map[][MAP_SIZE]);

    /**
     * Check if medic has active order
     */
    bool hasActiveOrder() const { return hasOrder; }

    /**
     * Check if medic has charges available
     */
    bool hasCharges() const { return healingCharges > 0 && !needsRecharge; }
};


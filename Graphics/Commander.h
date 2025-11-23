#pragma once
#include "Unit.h"

/**
 * Commander class - Plans team strategy and gives orders
 * Uses combined visibility map from all team members
 */
class Commander : public Unit
{
private:
    bool teamVisibilityMap[MAP_SIZE][MAP_SIZE]; // Combined visibility of all team units
    bool inDefenseMode;
    int lastSeenEnemyRow, lastSeenEnemyCol;
    bool enemySeen;

public:
    /**
     * Constructor
     */
    Commander(int startRow, int startCol, int teamId);

    /**
     * Update commander behavior
     */
    void update(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    ) override;

    /**
     * Build combined visibility map from all team units
     */
    void buildTeamVisibilityMap(std::vector<Unit*>& allUnits);

    /**
     * Check if enemy is visible to team
     */
    bool isEnemyVisible(std::vector<Unit*>& allUnits);

    /**
     * Give orders to team members
     */
    void giveOrders(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    );

    /**
     * Find safe position for commander
     */
    void findSafePosition(int map[][MAP_SIZE], int safetyMap[][MAP_SIZE], std::vector<Unit*>& allUnits);
};

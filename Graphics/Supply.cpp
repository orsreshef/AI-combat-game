#include "Supply.h"
#include "Warrior.h"
#include <iostream>

Supply::Supply(int startRow, int startCol, int teamId)
    : Unit(startRow, startCol, teamId, SUPPLY, 'P')
{
    hasOrder = false;
    targetUnitRow = targetUnitCol = -1;
    pathIndex = 0;
    ammoDepotRow = ammoDepotCol = -1;
    supplyCharges = MAX_SUPPLY_CHARGES;
    needsRecharge = false;
}

void Supply::update(
    int map[][MAP_SIZE],
    int safetyMap[][MAP_SIZE],
    std::vector<Unit*>& allUnits)
{
    if (!alive)
        return;

    updateVisibilityMap(map);

    if (escapingTree && map[row][col] != TREE)
    {
        escapingTree = false;
        framesInTree = 0;
        currentPath.clear();
        pathIndex = 0;
    }

    if (detectRepeatingPattern(map, allUnits))
    {
        currentPath.clear();
        pathIndex = 0;
    }

    if (checkTreeHidingLimit(map, allUnits))
    {
        escapingTree = true;
        currentPath.clear();
        pathIndex = 0;
    }

    if (escapingTree)
    {
        int openRow, openCol;

        if (currentPath.empty() || pathIndex >= currentPath.size())
        {
            if (findNearestOpenSpace(map, openRow, openCol))
            {
                currentPath = findPathAStar(map, safetyMap, openRow, openCol, allUnits);
                pathIndex = 0;
            }
        }

        if (!currentPath.empty() && pathIndex < currentPath.size())
        {
            auto nextPos = currentPath[pathIndex];
            if (nextPos.first == row && nextPos.second == col)
            {
                pathIndex++;
                if (pathIndex < currentPath.size())
                    nextPos = currentPath[pathIndex];
            }

            if (pathIndex < currentPath.size())
            {
                nextPos = currentPath[pathIndex];

                if (isPositionAvailable(map, nextPos.first, nextPos.second, allUnits))
                {
                    row = nextPos.first;
                    col = nextPos.second;
                    pathIndex++;
                }
                else
                {
                    int deltaRow[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
                    int deltaCol[] = { 1, 0, 0, -1, 1, -1, -1, 1 };

                    bool moved = false;
                    for (int dir = 0; dir < 8; dir++)
                    {
                        int newRow = row + deltaRow[dir];
                        int newCol = col + deltaCol[dir];

                        if (isPositionAvailable(map, newRow, newCol, allUnits) && map[newRow][newCol] != TREE)
                        {
                            row = newRow;
                            col = newCol;
                            currentPath.clear();
                            pathIndex = 0;
                            moved = true;
                            break;
                        }
                    }

                    if (!moved)
                    {
                        currentPath.clear();
                        pathIndex = 0;
                    }
                }
            }
        }
        else
        {
            int deltaRow[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
            int deltaCol[] = { 1, 0, 0, -1, 1, -1, -1, 1 };

            for (int dir = 0; dir < 8; dir++)
            {
                int newRow = row + deltaRow[dir];
                int newCol = col + deltaCol[dir];

                if (isPositionAvailable(map, newRow, newCol, allUnits) && map[newRow][newCol] != TREE)
                {
                    row = newRow;
                    col = newCol;
                    break;
                }
            }
        }

        return;
    }

    if (health < CRITICAL_HEALTH)
    {
        needsRecharge = false;
        hasOrder = false;

        int coverRow, coverCol;
        if (findNearestCover(map, coverRow, coverCol))
        {
            if (currentPath.empty() || pathIndex >= currentPath.size())
            {
                currentPath = findPathAStar(map, safetyMap, coverRow, coverCol, allUnits);
                pathIndex = 0;
            }

            if (!currentPath.empty() && pathIndex < currentPath.size())
            {
                auto nextPos = currentPath[pathIndex];
                if (nextPos.first == row && nextPos.second == col)
                {
                    pathIndex++;
                    if (pathIndex < currentPath.size())
                        nextPos = currentPath[pathIndex];
                }

                if (pathIndex < currentPath.size())
                {
                    nextPos = currentPath[pathIndex];
                    if (isPositionAvailable(map, nextPos.first, nextPos.second, allUnits))
                    {
                        row = nextPos.first;
                        col = nextPos.second;
                        pathIndex++;
                    }
                    else
                    {
                        currentPath.clear();
                        pathIndex = 0;
                    }
                }
            }
        }
        else
        {
            int bestRow = row, bestCol = col;
            int minDanger = (safetyMap != nullptr) ? safetyMap[row][col] : 999;

            int deltaRow[] = { 0, -1, 1, 0 };
            int deltaCol[] = { 1, 0, 0, -1 };

            for (int dir = 0; dir < 4; dir++)
            {
                int newRow = row + deltaRow[dir];
                int newCol = col + deltaCol[dir];

                if (isPositionAvailable(map, newRow, newCol, allUnits))
                {
                    int danger = (safetyMap != nullptr) ? safetyMap[newRow][newCol] : 0;
                    if (danger < minDanger)
                    {
                        minDanger = danger;
                        bestRow = newRow;
                        bestCol = newCol;
                    }
                }
            }

            if (bestRow != row || bestCol != col)
            {
                row = bestRow;
                col = bestCol;
            }
        }
        return;
    }

    if (supplyCharges > 0 && !needsRecharge)
    {
        for (auto unit : allUnits)
        {
            if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
            {
                int dist = abs(row - unit->getRow()) + abs(col - unit->getCol());
                Warrior* warrior = dynamic_cast<Warrior*>(unit);
                if (dist <= 1 && warrior && warrior->getAmmo() < MAX_AMMO)
                {
                    std::cout << "Team " << team << " Supply found adjacent warrior needing ammo! Resupplying immediately!" << std::endl;
                    warrior->refillAmmo(SUPPLY_AMMO_AMOUNT, SUPPLY_GRENADE_AMOUNT);
                    supplyCharges--;
                    hasOrder = false;
                    currentPath.clear();
                    pathIndex = 0;
                    needsRecharge = true;
                    std::cout << "Team " << team << " Supply resupplied warrior! Ammo: " << warrior->getAmmo()
                        << ". Returning to depot." << std::endl;
                    return;
                }
            }
        }
    }

    if (needsRecharge)
    {
        if (ammoDepotRow == -1)
        {
            findAmmoDepot(map);
        }

        if (ammoDepotRow != -1)
        {
            int distToDepot = abs(row - ammoDepotRow) + abs(col - ammoDepotCol);
            if (distToDepot <= 1)
            {
                if (supplyCharges < MAX_SUPPLY_CHARGES)
                {
                    supplyCharges = MAX_SUPPLY_CHARGES;
                    std::cout << ">>> Team " << team << " Supply RECHARGED at depot! Charges: "
                        << supplyCharges << " <<<" << std::endl;
                }

                needsRecharge = false;
                currentPath.clear();
                pathIndex = 0;

                int currentDanger = (safetyMap != nullptr) ? safetyMap[row][col] : 0;
                if (currentDanger > DANGER_THRESHOLD)
                {
                    std::cout << "Team " << team << " Supply at depot detected danger! Escaping..." << std::endl;

                    int bestRow = row, bestCol = col;
                    int minDanger = currentDanger;

                    int deltaRow[] = { 0, -1, 1, 0 };
                    int deltaCol[] = { 1, 0, 0, -1 };

                    for (int dir = 0; dir < 4; dir++)
                    {
                        int newRow = row + deltaRow[dir];
                        int newCol = col + deltaCol[dir];

                        if (isPositionAvailable(map, newRow, newCol, allUnits))
                        {
                            int danger = (safetyMap != nullptr) ? safetyMap[newRow][newCol] : 0;
                            if (danger < minDanger)
                            {
                                minDanger = danger;
                                bestRow = newRow;
                                bestCol = newCol;
                            }
                        }
                    }

                    if (bestRow != row || bestCol != col)
                    {
                        row = bestRow;
                        col = bestCol;
                    }
                    return;
                }

                for (auto unit : allUnits)
                {
                    if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
                    {
                        Warrior* warrior = dynamic_cast<Warrior*>(unit);
                        if (warrior && warrior->getAmmo() < LOW_AMMO)
                        {
                            std::cout << "Team " << team << " Supply at depot sees warrior at ("
                                << unit->getRow() << "," << unit->getCol()
                                << ") needing ammo:" << warrior->getAmmo() << "! Leaving depot!" << std::endl;
                            hasOrder = true;
                            targetUnitRow = unit->getRow();
                            targetUnitCol = unit->getCol();
                            return;
                        }
                    }
                }

                return;
            }

            if (currentPath.empty() || pathIndex >= currentPath.size())
            {
                static int moveLogCounter = 0;
                if (moveLogCounter++ % 60 == 0)
                {
                    std::cout << "Team " << team << " Supply at (" << row << "," << col
                        << ") calculating A* path to depot at (" << ammoDepotRow << "," << ammoDepotCol << ")" << std::endl;
                }
                currentPath = findPathAStar(map, safetyMap, ammoDepotRow, ammoDepotCol, allUnits);
                pathIndex = 0;
            }

            if (!currentPath.empty() && pathIndex < currentPath.size())
            {
                auto nextPos = currentPath[pathIndex];
                if (nextPos.first == row && nextPos.second == col)
                {
                    pathIndex++;
                    if (pathIndex < currentPath.size())
                        nextPos = currentPath[pathIndex];
                }

                if (pathIndex < currentPath.size())
                {
                    nextPos = currentPath[pathIndex];
                    if (isPositionAvailable(map, nextPos.first, nextPos.second, allUnits))
                    {
                        row = nextPos.first;
                        col = nextPos.second;
                        pathIndex++;
                    }
                    else
                    {
                    }
                }
            }
        }
        return;
    }

    if (!hasOrder)
    {
        return;
    }

    Warrior* adjacentWarrior = nullptr;
    for (auto unit : allUnits)
    {
        if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
        {
            int distToUnit = abs(row - unit->getRow()) + abs(col - unit->getCol());
            Warrior* warrior = dynamic_cast<Warrior*>(unit);
            if (distToUnit <= 1 && warrior && warrior->getAmmo() < MAX_AMMO)
            {
                adjacentWarrior = warrior;
                break;
            }
        }
    }

    if (adjacentWarrior != nullptr)
    {
        std::cout << "Team " << team << " Supply found adjacent warrior to resupply!" << std::endl;
        adjacentWarrior->refillAmmo(SUPPLY_AMMO_AMOUNT, SUPPLY_GRENADE_AMOUNT);
        supplyCharges--;
        std::cout << "Team " << team << " Supply resupplied warrior at ("
            << adjacentWarrior->getRow() << "," << adjacentWarrior->getCol() << ")! Charges left: "
            << supplyCharges << std::endl;

        hasOrder = false;
        currentPath.clear();
        pathIndex = 0;
        targetUnitRow = targetUnitCol = -1;
        needsRecharge = true;

        std::cout << "Team " << team << " Supply returning to depot to wait for next order!" << std::endl;
        return;
    }

    Warrior* targetWarrior = nullptr;
    for (auto unit : allUnits)
    {
        if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
        {
            Warrior* warrior = dynamic_cast<Warrior*>(unit);
            if (warrior && warrior->getAmmo() < LOW_AMMO)
            {
                targetWarrior = warrior;
                targetUnitRow = unit->getRow();
                targetUnitCol = unit->getCol();
                break;
            }
        }
    }

    if (targetWarrior == nullptr)
    {
        hasOrder = false;
        currentPath.clear();
        pathIndex = 0;
        targetUnitRow = targetUnitCol = -1;
        needsRecharge = true;
        std::cout << "Team " << team << " Supply: No warriors needing ammo, returning to depot" << std::endl;
        return;
    }

    if (currentPath.empty() || pathIndex >= currentPath.size())
    {
        currentPath = findPathAStar(map, safetyMap, targetUnitRow, targetUnitCol, allUnits);
        pathIndex = 0;
    }

    if (!currentPath.empty() && pathIndex < currentPath.size())
    {
        auto nextPos = currentPath[pathIndex];
        if (nextPos.first == row && nextPos.second == col)
        {
            pathIndex++;
            if (pathIndex < currentPath.size())
                nextPos = currentPath[pathIndex];
        }

        if (pathIndex < currentPath.size())
        {
            nextPos = currentPath[pathIndex];
            if (isPositionAvailable(map, nextPos.first, nextPos.second, allUnits))
            {
                row = nextPos.first;
                col = nextPos.second;
                pathIndex++;
            }
            else
            {
                currentPath.clear();
                pathIndex = 0;
            }
        }
    }
}

void Supply::receiveOrder(int targetRow, int targetCol)
{
    if (supplyCharges > 0)
    {
        hasOrder = true;
        targetUnitRow = targetRow;
        targetUnitCol = targetCol;
        currentPath.clear();
        pathIndex = 0;
        needsRecharge = false;

        std::cout << "Team " << team << " Supply received order to resupply unit at ("
            << targetRow << "," << targetCol << "). Charges: " << supplyCharges
            << " - Leaving depot!" << std::endl;
    }
    else
    {
        std::cout << "Team " << team << " Supply cannot accept order - out of charges!" << std::endl;
    }
}

void Supply::findAmmoDepot(int map[][MAP_SIZE])
{
    int teamStartRow = (team == TEAM_BLUE) ? 0 : MAP_SIZE - 1;
    int teamStartCol = (team == TEAM_BLUE) ? 0 : MAP_SIZE - 1;

    int minDistFromStart = 9999;

    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            if (map[i][j] == AMMO_DEPOT)
            {
                int distFromStart = abs(i - teamStartRow) + abs(j - teamStartCol);

                if (distFromStart < minDistFromStart)
                {
                    minDistFromStart = distFromStart;
                    ammoDepotRow = i;
                    ammoDepotCol = j;
                }
            }
        }
    }

    if (ammoDepotRow != -1)
    {
        std::cout << "Team " << team << " Supply found team depot at ("
            << ammoDepotRow << "," << ammoDepotCol << ")" << std::endl;
    }
}
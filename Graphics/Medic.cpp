#include "Medic.h"
#include "Warrior.h"
#include <iostream>

Medic::Medic(int startRow, int startCol, int teamId)
    : Unit(startRow, startCol, teamId, MEDIC, 'M')
{
    hasOrder = false;
    targetUnitRow = targetUnitCol = -1;
    pathIndex = 0;
    medicalDepotRow = medicalDepotCol = -1;
    healingCharges = MAX_MEDIC_CHARGES;
    needsRecharge = false;
}

void Medic::update(
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

    if (healingCharges > 0 && !needsRecharge)
    {
        for (auto unit : allUnits)
        {
            if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
            {
                int dist = abs(row - unit->getRow()) + abs(col - unit->getCol());
                if (dist <= 1 && unit->getHealth() < MAX_HEALTH)
                {
                    std::cout << "Team " << team << " Medic found adjacent wounded warrior! Healing immediately!" << std::endl;
                    unit->heal(MEDIC_HEAL_AMOUNT);
                    healingCharges--;
                    hasOrder = false;
                    currentPath.clear();
                    pathIndex = 0;
                    needsRecharge = true;
                    std::cout << "Team " << team << " Medic healed warrior to " << unit->getHealth()
                        << " HP! Returning to depot." << std::endl;
                    return;
                }
            }
        }
    }

    static int debugCounter = 0;
    if (debugCounter++ % 120 == 0)
    {
        std::cout << "=== Team " << team << " Medic at (" << row << "," << col
            << ") | Order:" << hasOrder << " Recharge:" << needsRecharge
            << " Charges:" << healingCharges << " HP:" << health << " ===" << std::endl;

        for (auto unit : allUnits)
        {
            if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
            {
                int dist = abs(row - unit->getRow()) + abs(col - unit->getCol());
                if (dist <= 2)
                {
                    std::cout << "  -> Warrior at (" << unit->getRow() << "," << unit->getCol()
                        << ") dist:" << dist << " HP:" << unit->getHealth() << std::endl;
                }
            }
        }
    }

    if (needsRecharge)
    {
        if (medicalDepotRow == -1)
        {
            findMedicalDepot(map);
        }

        if (medicalDepotRow != -1)
        {
            int distToDepot = abs(row - medicalDepotRow) + abs(col - medicalDepotCol);

            if (distToDepot <= 1)
            {
                if (healingCharges < MAX_MEDIC_CHARGES)
                {
                    healingCharges = MAX_MEDIC_CHARGES;
                    std::cout << ">>> Team " << team << " Medic RECHARGED at depot! Charges: "
                        << healingCharges << " <<<" << std::endl;
                }

                needsRecharge = false;
                currentPath.clear();
                pathIndex = 0;

                int currentDanger = (safetyMap != nullptr) ? safetyMap[row][col] : 0;
                if (currentDanger > DANGER_THRESHOLD)
                {
                    std::cout << "Team " << team << " Medic at depot detected danger! Escaping..." << std::endl;

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

                int aliveWarriors = 0;
                for (auto u : allUnits)
                {
                    if (u->getTeam() == team && u->isAlive() && u->getUnitType() == WARRIOR)
                        aliveWarriors++;
                }

                if (aliveWarriors > 0)
                {
                    for (auto unit : allUnits)
                    {
                        if (unit->getTeam() == team && unit->isAlive() &&
                            unit->getUnitType() == WARRIOR && unit->getHealth() < CRITICAL_HEALTH)
                        {
                            std::cout << "Team " << team << " Medic at depot sees wounded warrior at ("
                                << unit->getRow() << "," << unit->getCol()
                                << ") HP:" << unit->getHealth() << "! Leaving depot!" << std::endl;
                            hasOrder = true;
                            targetUnitRow = unit->getRow();
                            targetUnitCol = unit->getCol();
                            return;
                        }
                    }
                }
                else
                {
                    for (auto unit : allUnits)
                    {
                        if (unit->getTeam() == team && unit->isAlive() &&
                            unit->getHealth() < CRITICAL_HEALTH && unit->getUnitType() != MEDIC)
                        {
                            std::cout << "Team " << team << " Medic (no warriors left) helping " << unit->getSymbol()
                                << " at (" << unit->getRow() << "," << unit->getCol() << ")" << std::endl;
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
                    std::cout << "Team " << team << " Medic at (" << row << "," << col
                        << ") calculating A* path to depot at (" << medicalDepotRow << "," << medicalDepotCol << ")" << std::endl;
                }
                currentPath = findPathAStar(map, safetyMap, medicalDepotRow, medicalDepotCol, allUnits);
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

    Unit* adjacentWarrior = nullptr;
    for (auto unit : allUnits)
    {
        if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
        {
            int distToUnit = abs(row - unit->getRow()) + abs(col - unit->getCol());
            if (distToUnit <= 1 && unit->getHealth() < MAX_HEALTH)
            {
                adjacentWarrior = unit;
                break;
            }
        }
    }

    if (adjacentWarrior != nullptr)
    {
        std::cout << "Team " << team << " Medic found adjacent warrior to heal!" << std::endl;
        adjacentWarrior->heal(MEDIC_HEAL_AMOUNT);
        healingCharges--;
        std::cout << "Team " << team << " Medic healed warrior at ("
            << adjacentWarrior->getRow() << "," << adjacentWarrior->getCol()
            << ") for " << MEDIC_HEAL_AMOUNT << " HP! Charges left: "
            << healingCharges << std::endl;

        hasOrder = false;
        currentPath.clear();
        pathIndex = 0;
        targetUnitRow = targetUnitCol = -1;
        needsRecharge = true;

        std::cout << "Team " << team << " Medic returning to depot to wait for next order!" << std::endl;
        return;
    }

    Unit* targetWarrior = nullptr;
    for (auto unit : allUnits)
    {
        if (unit->getTeam() == team && unit->isAlive() &&
            unit->getUnitType() == WARRIOR && unit->getHealth() < CRITICAL_HEALTH)
        {
            targetWarrior = unit;
            targetUnitRow = unit->getRow();
            targetUnitCol = unit->getCol();
            break;
        }
    }

    if (targetWarrior == nullptr)
    {
        hasOrder = false;
        currentPath.clear();
        pathIndex = 0;
        targetUnitRow = targetUnitCol = -1;
        needsRecharge = true;
        std::cout << "Team " << team << " Medic: No wounded warriors found, returning to depot" << std::endl;
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

void Medic::receiveOrder(int targetRow, int targetCol)
{
    if (healingCharges > 0)
    {
        hasOrder = true;
        targetUnitRow = targetRow;
        targetUnitCol = targetCol;
        currentPath.clear();
        pathIndex = 0;
        needsRecharge = false;

        std::cout << "Team " << team << " Medic received order to heal unit at ("
            << targetRow << "," << targetCol << "). Charges: " << healingCharges
            << " - Leaving depot!" << std::endl;
    }
    else
    {
        std::cout << "Team " << team << " Medic cannot accept order - out of charges!" << std::endl;
    }
}

void Medic::findMedicalDepot(int map[][MAP_SIZE])
{
    int teamStartRow = (team == TEAM_BLUE) ? 0 : MAP_SIZE - 1;
    int teamStartCol = (team == TEAM_BLUE) ? 0 : MAP_SIZE - 1;

    int minDistFromStart = 9999;

    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            if (map[i][j] == MEDICAL_DEPOT)
            {
                int distFromStart = abs(i - teamStartRow) + abs(j - teamStartCol);

                if (distFromStart < minDistFromStart)
                {
                    minDistFromStart = distFromStart;
                    medicalDepotRow = i;
                    medicalDepotCol = j;
                }
            }
        }
    }

    if (medicalDepotRow != -1)
    {
        std::cout << "Team " << team << " Medic found team depot at ("
            << medicalDepotRow << "," << medicalDepotCol << ")" << std::endl;
    }
}
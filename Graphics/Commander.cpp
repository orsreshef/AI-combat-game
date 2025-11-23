#include "Commander.h"
#include "Warrior.h"
#include "Medic.h"
#include "Supply.h"
#include <iostream>

Commander::Commander(int startRow, int startCol, int teamId)
    : Unit(startRow, startCol, teamId, COMMANDER, 'C')
{
    inDefenseMode = false;
    enemySeen = false;
    lastSeenEnemyRow = -1;
    lastSeenEnemyCol = -1;

    for (int i = 0; i < MAP_SIZE; i++)
        for (int j = 0; j < MAP_SIZE; j++)
            teamVisibilityMap[i][j] = false;
}

void Commander::update(
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
    }

    detectRepeatingPattern(map, allUnits);

    checkTreeHidingLimit(map, allUnits);

    if (escapingTree)
    {
        int openRow, openCol;
        if (findNearestOpenSpace(map, openRow, openCol))
        {
            int deltaRow = openRow - row;
            int deltaCol = openCol - col;

            if (abs(deltaRow) > abs(deltaCol))
            {
                int newRow = row + (deltaRow > 0 ? 1 : -1);
                if (isPositionAvailable(map, newRow, col, allUnits) && map[newRow][col] != TREE)
                {
                    row = newRow;
                    return;
                }
            }
            else
            {
                int newCol = col + (deltaCol > 0 ? 1 : -1);
                if (isPositionAvailable(map, row, newCol, allUnits) && map[row][newCol] != TREE)
                {
                    col = newCol;
                    return;
                }
            }
        }

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
        return;
    }

    if (health < CRITICAL_HEALTH)
    {
        int coverRow, coverCol;
        if (findNearestCover(map, coverRow, coverCol))
        {
            int deltaRow = coverRow - row;
            int deltaCol = coverCol - col;

            if (abs(deltaRow) > abs(deltaCol))
            {
                int newRow = row + (deltaRow > 0 ? 1 : -1);
                if (isPositionAvailable(map, newRow, col, allUnits))
                {
                    row = newRow;
                    return;
                }
            }
            else
            {
                int newCol = col + (deltaCol > 0 ? 1 : -1);
                if (isPositionAvailable(map, row, newCol, allUnits))
                {
                    col = newCol;
                    return;
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

    buildTeamVisibilityMap(allUnits);

    enemySeen = isEnemyVisible(allUnits);

    giveOrders(map, safetyMap, allUnits);

    findSafePosition(map, safetyMap, allUnits);
}

void Commander::buildTeamVisibilityMap(std::vector<Unit*>& allUnits)
{
    for (int i = 0; i < MAP_SIZE; i++)
        for (int j = 0; j < MAP_SIZE; j++)
            teamVisibilityMap[i][j] = false;

    for (auto unit : allUnits)
    {
        if (unit->getTeam() == team && unit->isAlive())
        {
            for (int i = 0; i < MAP_SIZE; i++)
            {
                for (int j = 0; j < MAP_SIZE; j++)
                {
                    if (unit->getVisibility(i, j))
                        teamVisibilityMap[i][j] = true;
                }
            }
        }
    }
}

bool Commander::isEnemyVisible(std::vector<Unit*>& allUnits)
{
    bool seen = false;

    for (auto unit : allUnits)
    {
        if (unit->getTeam() != team && unit->isAlive())
        {
            int r = unit->getRow();
            int c = unit->getCol();

            if (teamVisibilityMap[r][c])
            {
                lastSeenEnemyRow = r;
                lastSeenEnemyCol = c;
                seen = true;
            }
        }
    }

    return seen;
}

void Commander::giveOrders(
    int map[][MAP_SIZE],
    int safetyMap[][MAP_SIZE],
    std::vector<Unit*>& allUnits)
{
    int aliveWarriors = 0;
    int totalTeamMembers = 0;

    for (auto unit : allUnits)
    {
        if (unit->getTeam() == team && unit->isAlive())
        {
            totalTeamMembers++;
            if (unit->getUnitType() == WARRIOR)
                aliveWarriors++;
        }
    }

    if (totalTeamMembers <= 2 || !enemySeen)
    {
        inDefenseMode = true;
    }
    else if (aliveWarriors >= 2 && enemySeen)
    {
        inDefenseMode = false;
    }

    for (auto unit : allUnits)
    {
        if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == WARRIOR)
        {
            Warrior* warrior = dynamic_cast<Warrior*>(unit);
            if (warrior)
            {
                if (warrior->getNeedsMedic())
                {
                    std::cout << "Team " << team << " Commander: Warrior needs medic! Searching for available medic..." << std::endl;

                    bool medicFound = false;
                    for (auto u : allUnits)
                    {
                        if (u->getTeam() == team && u->isAlive() && u->getUnitType() == MEDIC)
                        {
                            Medic* medic = dynamic_cast<Medic*>(u);
                            if (medic && !medic->hasActiveOrder())
                            {
                                medic->receiveOrder(warrior->getRow(), warrior->getCol());
                                medicFound = true;
                                break;
                            }
                        }
                    }

                    if (!medicFound)
                    {
                        std::cout << "Team " << team << " Commander: No available medic found (medic busy or out of charges)" << std::endl;
                    }
                }

                if (warrior->getNeedsAmmo())
                {
                    for (auto u : allUnits)
                    {
                        if (u->getTeam() == team && u->isAlive() && u->getUnitType() == SUPPLY)
                        {
                            Supply* supply = dynamic_cast<Supply*>(u);
                            if (supply && !supply->hasActiveOrder())
                            {
                                supply->receiveOrder(warrior->getRow(), warrior->getCol());
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Commander::findSafePosition(int map[][MAP_SIZE], int safetyMap[][MAP_SIZE], std::vector<Unit*>& allUnits)
{
    int currentDanger = safetyMap[row][col];

    if (currentDanger > DANGER_THRESHOLD)
    {
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
                if (safetyMap[newRow][newCol] < minDanger)
                {
                    minDanger = safetyMap[newRow][newCol];
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
        else
        {
            tryUnstuck(map, allUnits);
        }
    }
    else
    {
        int cell = map[row][col];
        if (cell == AMMO_DEPOT || cell == MEDICAL_DEPOT)
        {
            int deltaRow[] = { 0, -1, 1, 0 };
            int deltaCol[] = { 1, 0, 0, -1 };

            for (int dir = 0; dir < 4; dir++)
            {
                int newRow = row + deltaRow[dir];
                int newCol = col + deltaCol[dir];

                if (isPositionAvailable(map, newRow, newCol, allUnits))
                {
                    int cell2 = map[newRow][newCol];
                    if (cell2 != AMMO_DEPOT && cell2 != MEDICAL_DEPOT)
                    {
                        row = newRow;
                        col = newCol;
                        return;
                    }
                }
            }
        }
    }
}
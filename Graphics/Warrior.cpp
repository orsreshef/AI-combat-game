#include "Warrior.h"
#include "Projectiles.h"
#include "Medic.h"
#include "Supply.h"
#include <iostream>
#include <cmath>

Warrior::Warrior(int startRow, int startCol, int teamId)
    : Unit(startRow, startCol, teamId, WARRIOR, 'W')
{
    ammo = MAX_AMMO;
    grenades = MAX_GRENADES;
    needsAmmo = false;
    needsMedic = false;
    inDefenseMode = false;
    pathIndex = 0;
    hasTarget = false;
    targetRow = targetCol = -1;

}
void Warrior::update(
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
        std::cout << "Team " << team << " Warrior successfully escaped tree!" << std::endl;
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
        // If stuck in tree for more than 5 seconds - force direct escape!
        if (framesInTree >= 300)  // 5 seconds at 60 FPS
        {
            std::cout << "Team " << team << " Warrior FORCING escape after 5 seconds!" << std::endl;

            int deltaRow[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
            int deltaCol[] = { 1, 0, 0, -1, 1, -1, -1, 1 };

            for (int dir = 0; dir < 8; dir++)
            {
                int newRow = row + deltaRow[dir];
                int newCol = col + deltaCol[dir];

                if (isPositionAvailable(map, newRow, newCol, allUnits))
                {
                    row = newRow;
                    col = newCol;

                    // Check if we left the tree
                    if (map[row][col] != TREE)
                    {
                        escapingTree = false;
                        framesInTree = 0;
                        currentPath.clear();
                        pathIndex = 0;
                        std::cout << "Team " << team << " Warrior FORCED escape successful!" << std::endl;
                    }
                    return;
                }
            }

            std::cout << "Team " << team << " Warrior can't escape - completely surrounded!" << std::endl;
        }

        std::cout << "Team " << team << " Warrior at (" << row << "," << col
            << ") in escapingTree mode, map[row][col]=" << map[row][col] << std::endl;

        if (currentPath.empty() || pathIndex >= currentPath.size())
        {
            std::cout << "Team " << team << " Warrior building new escape path..." << std::endl;
            int openRow, openCol;
            if (findNearestOpenSpace(map, openRow, openCol))
            {
                std::cout << "Team " << team << " Warrior at (" << row << "," << col
                    << ") found path to open space at (" << openRow << "," << openCol << ")" << std::endl;
                currentPath = findPathAStar(map, safetyMap, openRow, openCol, allUnits);
                pathIndex = 0;
                std::cout << "Path has " << currentPath.size() << " steps" << std::endl;
            }
            else
            {
                std::cout << "Team " << team << " Warrior can't find open space, trying direct escape" << std::endl;

                int deltaRow[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
                int deltaCol[] = { 1, 0, 0, -1, 1, -1, -1, 1 };

                for (int dir = 0; dir < 8; dir++)
                {
                    int newRow = row + deltaRow[dir];
                    int newCol = col + deltaCol[dir];

                    std::cout << "  Trying direction " << dir << ": (" << newRow << "," << newCol
                        << ") available=" << isPositionAvailable(map, newRow, newCol, allUnits)
                        << " isTree=" << (map[newRow][newCol] == TREE) << std::endl;

                    if (isPositionAvailable(map, newRow, newCol, allUnits) && map[newRow][newCol] != TREE)
                    {
                        row = newRow;
                        col = newCol;
                        stuckInTreeCounter = 0;
                        std::cout << "Team " << team << " Warrior escaped tree directly to ("
                            << row << "," << col << ")" << std::endl;
                        return;
                    }
                }

                std::cout << "Team " << team << " Warrior STUCK - no escape found!" << std::endl;
                return;
            }
        }

        if (!currentPath.empty() && pathIndex < currentPath.size())
        {
            std::cout << "Team " << team << " Warrior following path, index=" << pathIndex
                << "/" << currentPath.size() << std::endl;

            auto nextPos = currentPath[pathIndex];
            if (nextPos.first == row && nextPos.second == col)
            {
                pathIndex++;
                std::cout << "  Already at path position, advancing to " << pathIndex << std::endl;
                if (pathIndex < currentPath.size())
                    nextPos = currentPath[pathIndex];
            }

            if (pathIndex >= currentPath.size())
            {
                if (map[row][col] != TREE)
                {
                    escapingTree = false;
                    framesInTree = 0;
                    currentPath.clear();
                    pathIndex = 0;
                    stuckInTreeCounter = 0;
                    std::cout << "Team " << team << " Warrior completed escape path and left tree!" << std::endl;
                    return;
                }
                else
                {
                    std::cout << "Team " << team << " Warrior reached end of path but still in tree. Recalculating..." << std::endl;
                    currentPath.clear();
                    pathIndex = 0;
                    return;
                }
            }

            if (pathIndex < currentPath.size())
            {
                nextPos = currentPath[pathIndex];
                std::cout << "  Next position: (" << nextPos.first << "," << nextPos.second
                    << ") available=" << isPositionAvailable(map, nextPos.first, nextPos.second, allUnits) << std::endl;

                if (isPositionAvailable(map, nextPos.first, nextPos.second, allUnits))
                {
                    row = nextPos.first;
                    col = nextPos.second;
                    pathIndex++;
                    stuckInTreeCounter = 0;  // Reset counter because we moved!
                    std::cout << "Team " << team << " Warrior moved along path to ("
                        << row << "," << col << ")" << std::endl;
                }
                else
                {
                    std::cout << "  Path blocked! Trying alternative..." << std::endl;

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
                            moved = true;
                            currentPath.clear();
                            pathIndex = 0;
                            stuckInTreeCounter = 0;
                            std::cout << "Team " << team << " Warrior found alternative escape to ("
                                << row << "," << col << ")" << std::endl;
                            break;
                        }
                    }

                    if (!moved)
                    {
                        std::cout << "Team " << team << " Warrior COMPLETELY STUCK!" << std::endl;
                        currentPath.clear();
                        pathIndex = 0;
                    }
                }
            }
        }
        return;
    }
    needsAmmo = checkAmmoStatus();
    needsMedic = checkHealthStatus();

    if (health >= CRITICAL_HEALTH && inDefenseMode)
    {
        inDefenseMode = false;
        needsMedic = false;
        currentPath.clear();
        pathIndex = 0;
        std::cout << "Team " << team << " Warrior healed - resuming attack mode!" << std::endl;
    }

    if (health < CRITICAL_HEALTH)
    {
        inDefenseMode = true;
    }

    if (ammo > 0)
    {
        shootAtEnemy(allUnits, map);
    }

    if (grenades > 0)
    {
        throwGrenade(allUnits, map);
    }

    if (inDefenseMode || health < CRITICAL_HEALTH)
    {
        defenseMode(map, safetyMap, allUnits);
    }
    else
    {
        attackMode(map, safetyMap, allUnits);
    }
}

bool Warrior::checkAmmoStatus()
{
    return (ammo < LOW_AMMO);
}

bool Warrior::checkHealthStatus()
{
    bool needsHelp = (health < CRITICAL_HEALTH);
    if (needsHelp && !needsMedic) // Just started needing help
    {
        std::cout << "!!! Team " << team << " Warrior at (" << row << "," << col
            << ") needs medic! Health: " << health << " !!!" << std::endl;
    }
    else if (needsHelp && needsMedic) // Still needs help
    {
        std::cout << "Team " << team << " Warrior at (" << row << "," << col
            << ") STILL needs medic! Health: " << health << std::endl;
    }
    return needsHelp;
}

bool Warrior::shootAtEnemy(std::vector<Unit*>& allUnits, int map[][MAP_SIZE])
{
    if (ammo <= 0)
        return false;

    for (auto enemy : allUnits)
    {
        if (enemy->getTeam() != team && enemy->isAlive())
        {
            int enemyRow = enemy->getRow();
            int enemyCol = enemy->getCol();

            int distance = abs(row - enemyRow) + abs(col - enemyCol);
            if (distance <= SHOOTING_RANGE)
            {
                if (canSeePosition(map, enemyRow, enemyCol))
                {
                    ammo--;
                    int baseDamage = 5 + (rand() % 6); // Base Damage range is: 5 - 10
                    int damage = (int)(baseDamage * 0.85); // Damage range is: 4.25 - 8.5
                    if (damage < 1) damage = 1; // Minimum 1 damage

                    // Create visual bullet
                    AddProjectile(row, col, enemyRow, enemyCol, team, false);

                    enemy->takeDamage(damage);

                    std::cout << "Team " << team << " Warrior at (" << row << "," << col
                        << ") shoots enemy at (" << enemyRow << "," << enemyCol
                        << ") for " << damage << " damage!" << std::endl;

                    return true;
                }
            }
        }
    }

    return false;
}

bool Warrior::throwGrenade(std::vector<Unit*>& allUnits, int map[][MAP_SIZE])
{
    if (grenades <= 0)
        return false;

    for (auto enemy : allUnits)
    {
        if (enemy->getTeam() != team && enemy->isAlive())
        {
            int enemyRow = enemy->getRow();
            int enemyCol = enemy->getCol();

            int distance = abs(row - enemyRow) + abs(col - enemyCol);
            if (distance <= GRENADE_RANGE && distance > 2)
            {
                if (canSeePosition(map, enemyRow, enemyCol))
                {
                    grenades--;
                    int baseDamage = 10 + (rand() % 11); // Base Damage range is: 10 - 20
                    int damage = (int)(baseDamage * 0.85); // Damage range is: 8.5 - 17
                    if (damage < 1) damage = 1; // Minimum 1 damage

                    // Create visual grenade
                    AddProjectile(row, col, enemyRow, enemyCol, team, true);

                    enemy->takeDamage(damage);

                    std::cout << "Team " << team << " Warrior at (" << row << "," << col
                        << ") throws grenade at (" << enemyRow << "," << enemyCol
                        << ") for " << damage << " damage!" << std::endl;

                    return true;
                }
            }
        }
    }

    return false;
}

void Warrior::attackMode(
    int map[][MAP_SIZE],
    int safetyMap[][MAP_SIZE],
    std::vector<Unit*>& allUnits)
{
    Unit* nearestEnemy = nullptr;
    int minDistance = 9999;

    for (auto enemy : allUnits)
    {
        if (enemy->getTeam() != team && enemy->isAlive())
        {
            int dist = abs(row - enemy->getRow()) + abs(col - enemy->getCol());
            if (dist < minDistance)
            {
                minDistance = dist;
                nearestEnemy = enemy;
            }
        }
    }

    if (nearestEnemy == nullptr)
    {
        tryUnstuck(map, allUnits);
        return;
    }

    int enemyRow = nearestEnemy->getRow();
    int enemyCol = nearestEnemy->getCol();

    bool needNewPath = currentPath.empty() ||
        pathIndex >= currentPath.size() ||
        (targetRow != enemyRow || targetCol != enemyCol);

    if (needNewPath)
    {
        targetRow = enemyRow;
        targetCol = enemyCol;
        currentPath = findPathAStar(map, safetyMap, targetRow, targetCol, allUnits);
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
                moveTowards(nextPos.first, nextPos.second, map, allUnits);
                pathIndex++;
            }
            else
            {
                currentPath.clear();
                pathIndex = 0;
            }
        }
    }
    else
    {
        if (!isPositionOccupied(enemyRow, enemyCol, allUnits))
        {
            moveTowards(enemyRow, enemyCol, map, allUnits);
        }
        else
        {
            tryUnstuck(map, allUnits);
        }
    }
}

void Warrior::defenseMode(
    int map[][MAP_SIZE],
    int safetyMap[][MAP_SIZE],
    std::vector<Unit*>& allUnits)
{
    if (health < CRITICAL_HEALTH)
    {
        Medic* nearestMedic = nullptr;
        int minDist = 9999;

        for (auto unit : allUnits)
        {
            if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == MEDIC)
            {
                // Cast the Unit* to a Medic*
                Medic* medic = dynamic_cast<Medic*>(unit);
                if (!medic) continue;

                // Only consider this medic unit if it has charges!
                if (medic->hasCharges())
                {
                    int dist = abs(row - unit->getRow()) + abs(col - unit->getCol());
                    if (dist < minDist)
                    {
                        minDist = dist;
                        nearestMedic = medic; // Store the Medic*
                    }
                }
            }
        }

        if (nearestMedic != nullptr)
        {
            int medicRow = nearestMedic->getRow();
            int medicCol = nearestMedic->getCol();

            if (abs(row - medicRow) <= 1 && abs(col - medicCol) <= 1)
            {
                if (ammo > 0)
                    shootAtEnemy(allUnits, map);
                return;
            }

            if (currentPath.empty() || pathIndex >= currentPath.size() ||
                targetRow != medicRow || targetCol != medicCol)
            {
                targetRow = medicRow;
                targetCol = medicCol;
                currentPath = findPathAStar(map, safetyMap, targetRow, targetCol, allUnits);
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
                        moveTowards(nextPos.first, nextPos.second, map, allUnits);
                        pathIndex++;
                    }
                    else
                    {
                        currentPath.clear();
                        pathIndex = 0;
                    }
                }
            }
            else
            {
                moveTowards(medicRow, medicCol, map, allUnits);
            }

            if (ammo > 0)
                shootAtEnemy(allUnits, map);

            return;
        }
    }

    if (needsAmmo && ammo < LOW_AMMO)
    {
        Supply* nearestSupply = nullptr;
        int minDist = 9999;

        for (auto unit : allUnits)
        {
            if (unit->getTeam() == team && unit->isAlive() && unit->getUnitType() == SUPPLY)
            {
                // Cast the Unit* to a Supply*
                Supply* supply = dynamic_cast<Supply*>(unit);
                if (!supply) continue;

                // Only consider this supply unit if it has charges!
                if (supply->hasCharges())
                {
                    int dist = abs(row - unit->getRow()) + abs(col - unit->getCol());
                    if (dist < minDist)
                    {
                        minDist = dist;
                        nearestSupply = supply; // Store the Supply*
                    }
                }
            }
        }

        if (nearestSupply != nullptr)
        {
            int supplyRow = nearestSupply->getRow();
            int supplyCol = nearestSupply->getCol();

            if (abs(row - supplyRow) <= 1 && abs(col - supplyCol) <= 1)
            {
                if (ammo > 0)
                    shootAtEnemy(allUnits, map);
                return;
            }

            if (currentPath.empty() || pathIndex >= currentPath.size() ||
                targetRow != supplyRow || targetCol != supplyCol)
            {
                targetRow = supplyRow;
                targetCol = supplyCol;
                currentPath = findPathAStar(map, safetyMap, targetRow, targetCol, allUnits);
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
                        moveTowards(nextPos.first, nextPos.second, map, allUnits);
                        pathIndex++;
                    }
                    else
                    {
                        currentPath.clear();
                        pathIndex = 0;
                    }
                }
            }
            else
            {
                moveTowards(supplyRow, supplyCol, map, allUnits);
            }

            if (ammo > 0)
                shootAtEnemy(allUnits, map);

            return;
        }
    }

    int coverRow, coverCol;
    if (findNearestCover(map, coverRow, coverCol))
    {
        if (currentPath.empty() || pathIndex >= currentPath.size() ||
            targetRow != coverRow || targetCol != coverCol)
        {
            targetRow = coverRow;
            targetCol = coverCol;
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
                    moveTowards(nextPos.first, nextPos.second, map, allUnits);
                    pathIndex++;
                }
                else
                {
                    currentPath.clear();
                    pathIndex = 0;
                }
            }
        }

        if (ammo > 0)
            shootAtEnemy(allUnits, map);
    }
    else
    {
        int bestRow = row, bestCol = col;
        int minDanger = (safetyMap != nullptr) ? safetyMap[row][col] : 0;

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
        else
        {
            tryUnstuck(map, allUnits);
        }
    }
}

void Warrior::refillAmmo(int amount, int grenadeAmount)
{
    int oldAmmo = ammo;
    int oldGrenades = grenades;

    ammo += amount;
    if (ammo > MAX_AMMO)
        ammo = MAX_AMMO;

    grenades += grenadeAmount;
    if (grenades > MAX_GRENADES)
        grenades = MAX_GRENADES;

    std::cout << "Warrior refillAmmo called: " << oldAmmo << "->" << ammo
        << " ammo, " << oldGrenades << "->" << grenades << " grenades" << std::endl;

    needsAmmo = false;

    std::cout << "Team " << team << " Warrior refilled! Ammo: " << ammo
        << ", Grenades: " << grenades << std::endl;
}
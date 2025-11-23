#include "Unit.h"
#include "glut.h"
#include "CompareNodes.h"
#include <queue>
#include <vector>
#include <cmath>
#include <iostream>

Unit::Unit(int startRow, int startCol, int teamId, int type, char sym)
{
    row = startRow;
    col = startCol;
    team = teamId;
    unitType = type;
    symbol = sym;
    health = MAX_HEALTH;
    alive = true;
    movementCounter = 0;
    escapingTree = false;
    stuckInTreeCounter = 0;  // Initialize escape counter

    // Initialize pattern detection
    posHistoryIndex = 0;
    posHistoryCount = 0;
   
    loopBreakCooldown = 0;
    framesInTree = 0;
    
    for (int i = 0; i < 8; i++)
    {
        posHistory[i][0] = -1;
        posHistory[i][1] = -1;
    }

    // Set movement frequency based on unit type
    switch (unitType)
    {
    case WARRIOR:
        moveFrequency = WARRIOR_MOVE_FREQ;
        break;
    case MEDIC:
        moveFrequency = MEDIC_MOVE_FREQ;  // 4x faster!
        break;
    case SUPPLY:
        moveFrequency = SUPPLY_MOVE_FREQ; // 4x faster!
        break;
    case COMMANDER:
        moveFrequency = COMMANDER_MOVE_FREQ;
        break;
    default:
        moveFrequency = 300;
    }

    // Set team color
    if (team == TEAM_BLUE)
    {
        colorR = Colors::BLUE_R;
        colorG = Colors::BLUE_G;
        colorB = Colors::BLUE_B;
    }
    else
    {
        colorR = Colors::ORANGE_R;
        colorG = Colors::ORANGE_G;
        colorB = Colors::ORANGE_B;
    }

    // Initialize visibility map
    for (int i = 0; i < MAP_SIZE; i++)
        for (int j = 0; j < MAP_SIZE; j++)
            visibilityMap[i][j] = false;
}

Unit::~Unit()
{
}

bool Unit::shouldMoveThisFrame(int globalFrameCounter)
{
    return (globalFrameCounter % moveFrequency == 0);
}

void Unit::takeDamage(int damage)
{
    health -= damage;
    if (health <= 0)
    {
        health = 0;
        alive = false;
    }
}

void Unit::heal(int amount)
{
    health += amount;
    if (health > MAX_HEALTH)
        health = MAX_HEALTH;
}

bool Unit::isValidMove(int map[][MAP_SIZE], int r, int c) const
{
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE)
        return false;

    int cell = map[r][c];

    // Can't move into rocks or water
    if (cell == ROCK || cell == WATER)
        return false;

    return true;
}

bool Unit::isPositionOccupied(int r, int c, std::vector<Unit*>& allUnits) const
{
    for (auto unit : allUnits)
    {
        if (unit != this && unit->isAlive())
        {
            if (unit->getRow() == r && unit->getCol() == c)
                return true;
        }
    }
    return false;
}

bool Unit::isPositionAvailable(int map[][MAP_SIZE], int r, int c, std::vector<Unit*>& allUnits) const
{
    return isValidMove(map, r, c) && !isPositionOccupied(r, c, allUnits);
}

bool Unit::canSeePosition(int map[][MAP_SIZE], int targetRow, int targetCol) const
{
    // Check if target is within visibility range
    int distance = abs(row - targetRow) + abs(col - targetCol);
    if (distance > VISIBILITY_RANGE)
        return false;

    // Bresenham's line algorithm to check line of sight
    int x0 = col, y0 = row;
    int x1 = targetCol, y1 = targetRow;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int x = x0, y = y0;

    while (true)
    {
        // Check if current cell blocks vision
        if (x != x0 || y != y0) // Skip starting position
        {
            if (x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE)
            {
                int cell = map[y][x];
                if (cell == ROCK || cell == TREE)
                    return false; // Vision blocked
            }
        }

        if (x == x1 && y == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }

    return true;
}

void Unit::updateVisibilityMap(int map[][MAP_SIZE])
{
    // Reset visibility map
    for (int i = 0; i < MAP_SIZE; i++)
        for (int j = 0; j < MAP_SIZE; j++)
            visibilityMap[i][j] = false;

    if (!alive)
        return;

    // Check all cells within visibility range
    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            if (canSeePosition(map, i, j))
                visibilityMap[i][j] = true;
        }
    }
}

bool Unit::getVisibility(int r, int c) const
{
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE)
        return false;
    return visibilityMap[r][c];
}

std::vector<std::pair<int, int>> Unit::findPathAStar(
    int map[][MAP_SIZE],
    int safetyMap[][MAP_SIZE],
    int targetRow,
    int targetCol,
    std::vector<Unit*>& allUnits)
{
    std::vector<std::pair<int, int>> path;

    // Check if target is valid
    if (!isValidMove(map, targetRow, targetCol))
        return path;

    std::priority_queue<Node*, std::vector<Node*>, CompareNodes> openList;
    std::vector<Node*> closedList;

    Node* startNode = new Node(row, col);
    startNode->calculateHeuristic(targetRow, targetCol);
    startNode->setG(0);

    openList.push(startNode);

    int deltaRow[] = { 0, -1, 1, 0 };
    int deltaCol[] = { 1, 0, 0, -1 };

    while (!openList.empty())
    {
        Node* current = openList.top();
        openList.pop();

        int distToTarget = abs(current->getRow() - targetRow) + abs(current->getCol() - targetCol);
        if (distToTarget <= 1)
        {
            Node* pathNode = current;
            while (pathNode != nullptr)
            {
                path.insert(path.begin(), { pathNode->getRow(), pathNode->getCol() });
                pathNode = pathNode->getParent();
            }

            delete current;
            while (!openList.empty())
            {
                delete openList.top();
                openList.pop();
            }
            for (auto node : closedList)
                delete node;

            return path;
        }

        closedList.push_back(current);

        for (int dir = 0; dir < 4; dir++)
        {
            int newRow = current->getRow() + deltaRow[dir];
            int newCol = current->getCol() + deltaCol[dir];

            if (!isValidMove(map, newRow, newCol))
                continue;

            double occupiedPenalty = 0;
            if (isPositionOccupied(newRow, newCol, allUnits))
            {
                occupiedPenalty = 50.0;
            }

            bool inClosedList = false;
            for (auto closedNode : closedList)
            {
                if (closedNode->isAt(newRow, newCol))
                {
                    inClosedList = true;
                    break;
                }
            }
            if (inClosedList)
                continue;

            Node* neighbor = new Node(current, newRow, newCol);

            double safetyCost = 0;
            if (safetyMap != nullptr)
            {
                int danger = safetyMap[newRow][newCol];
                safetyCost = danger / 10.0;
            }
            neighbor->setG(current->getG() + 1 + safetyCost + occupiedPenalty);
            neighbor->calculateHeuristic(targetRow, targetCol);

            bool foundBetter = false;
            std::vector<Node*> tempList;

            while (!openList.empty())
            {
                Node* openNode = openList.top();
                openList.pop();

                if (openNode->isAt(newRow, newCol))
                {
                    if (neighbor->getG() < openNode->getG())
                    {
                        delete openNode;
                        foundBetter = true;
                    }
                    else
                    {
                        tempList.push_back(openNode);
                        delete neighbor;
                        neighbor = nullptr;
                        foundBetter = true;
                    }
                }
                else
                {
                    tempList.push_back(openNode);
                }
            }

            for (auto node : tempList)
                openList.push(node);

            if (!foundBetter && neighbor != nullptr)
                openList.push(neighbor);
            else if (foundBetter && neighbor != nullptr)
                openList.push(neighbor);
        }
    }

    for (auto node : closedList)
        delete node;

    return path;
}

bool Unit::findNearestCover(int map[][MAP_SIZE], int& coverRow, int& coverCol)
{
    std::queue<Node*> bfsQueue;
    std::vector<std::vector<bool>> visited(MAP_SIZE, std::vector<bool>(MAP_SIZE, false));

    Node* startNode = new Node(row, col);
    bfsQueue.push(startNode);
    visited[row][col] = true;

    int deltaRow[] = { 0, -1, 1, 0 };
    int deltaCol[] = { 1, 0, 0, -1 };

    while (!bfsQueue.empty())
    {
        Node* current = bfsQueue.front();
        bfsQueue.pop();

        int r = current->getRow();
        int c = current->getCol();

        bool isCover = false;
        for (int dir = 0; dir < 4; dir++)
        {
            int checkRow = r + deltaRow[dir];
            int checkCol = c + deltaCol[dir];
            if (checkRow >= 0 && checkRow < MAP_SIZE && checkCol >= 0 && checkCol < MAP_SIZE)
            {
                if (map[checkRow][checkCol] == ROCK || map[checkRow][checkCol] == TREE)
                {
                    isCover = true;
                    break;
                }
            }
        }

        if (isCover && !(r == row && c == col))
        {
            coverRow = r;
            coverCol = c;

            delete current;
            while (!bfsQueue.empty())
            {
                delete bfsQueue.front();
                bfsQueue.pop();
            }

            return true;
        }

        if (current->getDepth() >= MAX_BFS_DEPTH)
        {
            delete current;
            continue;
        }

        for (int dir = 0; dir < 4; dir++)
        {
            int newRow = r + deltaRow[dir];
            int newCol = c + deltaCol[dir];

            if (isValidMove(map, newRow, newCol) && !visited[newRow][newCol])
            {
                visited[newRow][newCol] = true;
                Node* neighbor = new Node(current, newRow, newCol);
                bfsQueue.push(neighbor);
            }
        }

        delete current;
    }

    return false;
}

void Unit::moveTowards(int targetRow, int targetCol, int map[][MAP_SIZE], std::vector<Unit*>& allUnits)
{
    if (!alive)
        return;

    int rowDiff = targetRow - row;
    int colDiff = targetCol - col;

    std::vector<std::pair<int, int>> directions;

    if (abs(rowDiff) > abs(colDiff))
    {
        if (rowDiff < 0) directions.push_back({ -1, 0 });
        else if (rowDiff > 0) directions.push_back({ 1, 0 });

        if (colDiff < 0) directions.push_back({ 0, -1 });
        else if (colDiff > 0) directions.push_back({ 0, 1 });
    }
    else
    {
        if (colDiff < 0) directions.push_back({ 0, -1 });
        else if (colDiff > 0) directions.push_back({ 0, 1 });

        if (rowDiff < 0) directions.push_back({ -1, 0 });
        else if (rowDiff > 0) directions.push_back({ 1, 0 });
    }

    directions.push_back({ 0, 1 });
    directions.push_back({ 0, -1 });
    directions.push_back({ 1, 0 });
    directions.push_back({ -1, 0 });

    for (auto dir : directions)
    {
        int newRow = row + dir.first;
        int newCol = col + dir.second;

        if (isPositionAvailable(map, newRow, newCol, allUnits))
        {
            row = newRow;
            col = newCol;
            return;
        }
    }
}

void Unit::tryUnstuck(int map[][MAP_SIZE], std::vector<Unit*>& allUnits)
{
    int deltaRow[] = { 0, -1, 1, 0 };
    int deltaCol[] = { 1, 0, 0, -1 };

    for (int i = 0; i < 4; i++)
    {
        int newRow = row + deltaRow[i];
        int newCol = col + deltaCol[i];

        if (isPositionAvailable(map, newRow, newCol, allUnits))
        {
            row = newRow;
            col = newCol;
            return;
        }
    }
}

bool Unit::detectRepeatingPattern(int map[][MAP_SIZE], std::vector<Unit*>& allUnits)
{
    // If in cooldown after breaking a loop, decrement and don't check for patterns
    if (loopBreakCooldown > 0)
    {
        loopBreakCooldown--;
        return false;
    }

    // Add current position to history
    posHistory[posHistoryIndex][0] = row;
    posHistory[posHistoryIndex][1] = col;
    posHistoryIndex = (posHistoryIndex + 1) % 8;
    if (posHistoryCount < 8)
        posHistoryCount++;

    // Need at least 4 positions to detect a 2-position pattern
    if (posHistoryCount < 4)
        return false;

    // Check for 2-position pattern: A->B->A->B (positions 0,2,4,6 same AND positions 1,3,5,7 same)
    if (posHistoryCount >= 4)
    {
        bool twoPattern = true;
        for (int i = 0; i < posHistoryCount - 2; i += 2)
        {
            int idx1 = i;
            int idx2 = i + 2;
            if (idx2 < posHistoryCount)
            {
                if (posHistory[idx1][0] != posHistory[idx2][0] || posHistory[idx1][1] != posHistory[idx2][1])
                {
                    twoPattern = false;
                    break;
                }
            }
        }

        if (twoPattern && (posHistory[0][0] != posHistory[1][0] || posHistory[0][1] != posHistory[1][1]))
        {
            std::cout << "Team " << team << " " << symbol << " detected 2-position loop! Breaking..." << std::endl;

            // Move to position NOT in the pattern
            int deltaRow[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
            int deltaCol[] = { 1, 0, 0, -1, 1, -1, -1, 1 };

            for (int i = 0; i < 8; i++)
            {
                int newRow = row + deltaRow[i];
                int newCol = col + deltaCol[i];

                // Avoid positions in the pattern
                bool inPattern = false;
                for (int j = 0; j < 2 && j < posHistoryCount; j++)
                {
                    if (newRow == posHistory[j][0] && newCol == posHistory[j][1])
                    {
                        inPattern = true;
                        break;
                    }
                }

                if (!inPattern && isPositionAvailable(map, newRow, newCol, allUnits))
                {
                    // Move to escape position
                    row = newRow;
                    col = newCol;

                    // Try to move 1-2 more cells in same direction to really get away
                    for (int extra = 0; extra < 2; extra++)
                    {
                        int furtherRow = row + deltaRow[i];
                        int furtherCol = col + deltaCol[i];
                        if (isPositionAvailable(map, furtherRow, furtherCol, allUnits))
                        {
                            row = furtherRow;
                            col = furtherCol;
                        }
                        else
                            break;
                    }

                    std::cout << "Team " << team << " " << symbol << " escaped 2-pos loop to (" << row << "," << col << ")" << std::endl;

                    // Reset history
                    for (int k = 0; k < 8; k++)
                    {
                        posHistory[k][0] = -1;
                        posHistory[k][1] = -1;
                    }
                    posHistoryCount = 0;
                    posHistoryIndex = 0;
                    loopBreakCooldown = 240; // Wait 4 seconds before checking again
                    return true;
                }
            }
        }
    }

    // Check for 3-position pattern: A->B->C->A->B->C
    if (posHistoryCount >= 6)
    {
        bool threePattern = (posHistory[0][0] == posHistory[3][0] && posHistory[0][1] == posHistory[3][1]) &&
            (posHistory[1][0] == posHistory[4][0] && posHistory[1][1] == posHistory[4][1]) &&
            (posHistory[2][0] == posHistory[5][0] && posHistory[2][1] == posHistory[5][1]);

        if (threePattern)
        {
            std::cout << "Team " << team << " " << symbol << " detected 3-position loop! Breaking..." << std::endl;

            // Move to position NOT in the 3-position pattern
            int deltaRow[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
            int deltaCol[] = { 1, 0, 0, -1, 1, -1, -1, 1 };

            for (int i = 0; i < 8; i++)
            {
                int newRow = row + deltaRow[i];
                int newCol = col + deltaCol[i];

                // Avoid positions in the pattern
                bool inPattern = false;
                for (int j = 0; j < 3; j++)
                {
                    if (newRow == posHistory[j][0] && newCol == posHistory[j][1])
                    {
                        inPattern = true;
                        break;
                    }
                }

                if (!inPattern && isPositionAvailable(map, newRow, newCol, allUnits))
                {
                    row = newRow;
                    col = newCol;
                    std::cout << "Team " << team << " " << symbol << " escaped 3-pos loop to (" << row << "," << col << ")" << std::endl;

                    // Reset history
                    for (int k = 0; k < 8; k++)
                    {
                        posHistory[k][0] = -1;
                        posHistory[k][1] = -1;
                    }
                    posHistoryCount = 0;
                    posHistoryIndex = 0;
                    loopBreakCooldown = 240; // Wait 4 seconds before checking again
                    return true;
                }
            }

            // If couldn't find safe position, reset and try again
            for (int k = 0; k < 8; k++)
            {
                posHistory[k][0] = -1;
                posHistory[k][1] = -1;
            }
            posHistoryCount = 0;
            posHistoryIndex = 0;
            loopBreakCooldown = 240;
            return true;
        }
    }

    // Check for 4-position pattern: A->B->C->D->A->B->C->D
    if (posHistoryCount >= 8)
    {
        bool fourPattern = (posHistory[0][0] == posHistory[4][0] && posHistory[0][1] == posHistory[4][1]) &&
            (posHistory[1][0] == posHistory[5][0] && posHistory[1][1] == posHistory[5][1]) &&
            (posHistory[2][0] == posHistory[6][0] && posHistory[2][1] == posHistory[6][1]) &&
            (posHistory[3][0] == posHistory[7][0] && posHistory[3][1] == posHistory[7][1]);

        if (fourPattern)
        {
            std::cout << "Team " << team << " " << symbol << " detected 4-position loop! Breaking..." << std::endl;

            // Move to position NOT in the 4-position pattern
            int deltaRow[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
            int deltaCol[] = { 1, 0, 0, -1, 1, -1, -1, 1 };

            for (int i = 0; i < 8; i++)
            {
                int newRow = row + deltaRow[i];
                int newCol = col + deltaCol[i];

                // Avoid positions in the pattern
                bool inPattern = false;
                for (int j = 0; j < 4; j++)
                {
                    if (newRow == posHistory[j][0] && newCol == posHistory[j][1])
                    {
                        inPattern = true;
                        break;
                    }
                }

                if (!inPattern && isPositionAvailable(map, newRow, newCol, allUnits))
                {
                    row = newRow;
                    col = newCol;
                    std::cout << "Team " << team << " " << symbol << " escaped 4-pos loop to (" << row << "," << col << ")" << std::endl;

                    // Reset history
                    for (int k = 0; k < 8; k++)
                    {
                        posHistory[k][0] = -1;
                        posHistory[k][1] = -1;
                    }
                    posHistoryCount = 0;
                    posHistoryIndex = 0;
                    loopBreakCooldown = 240; // Wait 4 seconds before checking again
                    return true;
                }
            }

            // If couldn't find safe position, reset and try again
            for (int k = 0; k < 8; k++)
            {
                posHistory[k][0] = -1;
                posHistory[k][1] = -1;
            }
            posHistoryCount = 0;
            posHistoryIndex = 0;
            loopBreakCooldown = 240;
            return true;
        }
    }

    return false;
}

//// Check if unit is stuck in tree
//bool Unit::checkTreeHidingLimit(int map[][MAP_SIZE], std::vector<Unit*>& allUnits)
//{
//    const int MAX_TREE_FRAMES = 600; // 10 seconds at 60 FPS
//
//    // Check if currently in a tree
//    if (map[row][col] == TREE)
//    {
//        framesInTree += moveFrequency;
//    }
//    else
//    {
//        // Not in tree, reset counter
//        framesInTree = 0;
//    }
//
//    if (framesInTree >= MAX_TREE_FRAMES)
//    {
//        // We've been in a tree too long!
//        // Reset the counter to avoid spamming the log
//        framesInTree = 0;
//        std::cout << "Team " << team << " " << symbol << " hiding in tree too long!" << std::endl;
//        return true; // Signal that we are stuck
//    }
//
//    return false; // Not stuck
//}

bool Unit::checkTreeHidingLimit(int map[][MAP_SIZE], std::vector<Unit*>& allUnits)
{
    const int MAX_TREE_FRAMES = 600;

    if (map[row][col] == TREE)
    {
        framesInTree += moveFrequency;

        if (framesInTree >= MAX_TREE_FRAMES)
        {
            std::cout << "Team " << team << " " << symbol << " hiding in tree too long!" << std::endl;
            return true;
        }
    }
    else
    {
        framesInTree = 0;
    }

    return false;
}

bool Unit::findNearestOpenSpace(int map[][MAP_SIZE], int& openRow, int& openCol)
{
    std::queue<Node*> bfsQueue;
    std::vector<std::vector<bool>> visited(MAP_SIZE, std::vector<bool>(MAP_SIZE, false));

    Node* startNode = new Node(row, col);
    bfsQueue.push(startNode);
    visited[row][col] = true;

    while (!bfsQueue.empty())
    {
        Node* current = bfsQueue.front();
        bfsQueue.pop();

        int r = current->getRow();
        int c = current->getCol();

        // Check if this square is NOT a tree (and not our starting position)
        if (map[r][c] != TREE && !(r == row && c == col))
        {
            openRow = r;
            openCol = c;
            delete current;
            while (!bfsQueue.empty()) { delete bfsQueue.front(); bfsQueue.pop(); }
            return true;
        }

        // Limit search depth
        if (current->getDepth() >= MAX_BFS_DEPTH)
        {
            delete current;
            continue;
        }

        int deltaRow[] = { 0, -1, 1, 0 };
        int deltaCol[] = { 1, 0, 0, -1 };

        for (int dir = 0; dir < 4; dir++)
        {
            int newRow = r + deltaRow[dir];
            int newCol = c + deltaCol[dir];

            // Must be a valid move to be part of the path
            if (isValidMove(map, newRow, newCol) && !visited[newRow][newCol])
            {
                visited[newRow][newCol] = true;
                Node* neighbor = new Node(current, newRow, newCol);
                bfsQueue.push(neighbor);
            }
        }
        delete current;
    }
    return false; // No open space found
}

void Unit::show() const
{
    if (!alive)
        return;

    glColor3d(colorR, colorG, colorB);
    glBegin(GL_POLYGON);
    glVertex2d(col + 0.1, row + 0.1);
    glVertex2d(col + 0.9, row + 0.1);
    glVertex2d(col + 0.9, row + 0.9);
    glVertex2d(col + 0.1, row + 0.9);
    glEnd();

    if (team == TEAM_BLUE)
        glColor3d(Colors::BLUE_DARK_R, Colors::BLUE_DARK_G, Colors::BLUE_DARK_B);
    else
        glColor3d(Colors::ORANGE_DARK_R, Colors::ORANGE_DARK_G, Colors::ORANGE_DARK_B);

    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(col + 0.1, row + 0.1);
    glVertex2d(col + 0.9, row + 0.1);
    glVertex2d(col + 0.9, row + 0.9);
    glVertex2d(col + 0.1, row + 0.9);
    glEnd();

    glColor3d(Colors::TEXT_R, Colors::TEXT_G, Colors::TEXT_B);
    glRasterPos2d(col + 0.265, row + 0.7);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, symbol);

    if (health < MAX_HEALTH)
    {
        double healthPercent = (double)health / MAX_HEALTH;

        glColor3d(1.0, 0.0, 0.0);
        glBegin(GL_POLYGON);
        glVertex2d(col + 0.1, row + 0.0);
        glVertex2d(col + 0.9, row + 0.0);
        glVertex2d(col + 0.9, row + 0.1);
        glVertex2d(col + 0.1, row + 0.1);
        glEnd();

        glColor3d(0.0, 1.0, 0.0);
        glBegin(GL_POLYGON);
        glVertex2d(col + 0.1, row + 0.0);
        glVertex2d(col + 0.1 + 0.8 * healthPercent, row + 0.0);
        glVertex2d(col + 0.1 + 0.8 * healthPercent, row + 0.1);
        glVertex2d(col + 0.1, row + 0.1);
        glEnd();
    }
}
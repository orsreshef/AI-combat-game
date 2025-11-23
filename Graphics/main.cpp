#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <iostream>
#include <vector>

#include "Definitions.h"
#include "Node.h"
#include "Unit.h"
#include "Commander.h"
#include "Warrior.h"
#include "Medic.h"
#include "Supply.h"

using namespace std;

// Forward declaration for projectile system
void AddProjectile(double startRow, double startCol, double endRow, double endCol, int team, bool isGrenade);

const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 1000;

// Projectile structure for visual effects
struct Projectile {
    double startRow, startCol;  // Starting position
    double endRow, endCol;      // Target position
    double currentRow, currentCol; // Current position (animated)
    int team;                   // Team color
    bool isGrenade;             // true = grenade, false = bullet
    int framesAlive;            // How long it's been flying
    bool active;                // Still animating
};

// Game state
int map[MAP_SIZE][MAP_SIZE] = { 0 };
int safetyMap[MAP_SIZE][MAP_SIZE] = { 0 };
vector<Unit*> allUnits;
vector<Projectile> activeProjectiles;
bool gameRunning = true;
bool gameOver = false;
int winningTeam = -1;

// Game timing - GLOBAL frame counter for all units
int frameCounter = 0;

void InitMap()
{
    for (int i = 0; i < MAP_SIZE; i++)
        for (int j = 0; j < MAP_SIZE; j++)
            map[i][j] = SPACE;

    for (int cluster = 0; cluster < 8; cluster++)
    {
        int centerRow = 5 + rand() % (MAP_SIZE - 10);
        int centerCol = 5 + rand() % (MAP_SIZE - 10);
        int clusterSize = 2 + rand() % 4;

        for (int i = 0; i < clusterSize; i++)
        {
            int r = centerRow + (rand() % 5) - 2;
            int c = centerCol + (rand() % 5) - 2;
            if (r >= 0 && r < MAP_SIZE && c >= 0 && c < MAP_SIZE)
                map[r][c] = ROCK;
        }
    }

    for (int cluster = 0; cluster < 10; cluster++)
    {
        int centerRow = 5 + rand() % (MAP_SIZE - 10);
        int centerCol = 5 + rand() % (MAP_SIZE - 10);
        int clusterSize = 3 + rand() % 5;

        for (int i = 0; i < clusterSize; i++)
        {
            int r = centerRow + (rand() % 6) - 3;
            int c = centerCol + (rand() % 6) - 3;
            if (r >= 0 && r < MAP_SIZE && c >= 0 && c < MAP_SIZE && map[r][c] == SPACE)
                map[r][c] = TREE;
        }
    }

    for (int river = 0; river < 3; river++)
    {
        int startRow = 10 + rand() % (MAP_SIZE - 20);
        int startCol = rand() % MAP_SIZE;
        int direction = rand() % 2;

        for (int i = 0; i < 15 + rand() % 15; i++)
        {
            if (direction == 0)
            {
                int c = startCol + i;
                if (c >= 0 && c < MAP_SIZE && map[startRow][c] == SPACE)
                    map[startRow][c] = WATER;
            }
            else
            {
                int r = startRow + i;
                if (r >= 0 && r < MAP_SIZE && map[r][startCol] == SPACE)
                    map[r][startCol] = WATER;
            }
        }
    }

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            map[i][j] = SPACE;

    for (int i = MAP_SIZE - 8; i < MAP_SIZE; i++)
        for (int j = MAP_SIZE - 8; j < MAP_SIZE; j++)
            map[i][j] = SPACE;

    map[1][6] = AMMO_DEPOT;
    map[6][1] = MEDICAL_DEPOT;

    map[MAP_SIZE - 2][MAP_SIZE - 7] = AMMO_DEPOT;
    map[MAP_SIZE - 7][MAP_SIZE - 2] = MEDICAL_DEPOT;
}

void InitUnits()
{
    for (auto unit : allUnits)
        delete unit;
    allUnits.clear();

    allUnits.push_back(new Commander(2, 2, TEAM_BLUE));
    allUnits.push_back(new Warrior(2, 4, TEAM_BLUE));
    allUnits.push_back(new Warrior(4, 2, TEAM_BLUE));
    allUnits.push_back(new Medic(6, 2, TEAM_BLUE));      // At medical depot adjacent position
    allUnits.push_back(new Supply(2, 6, TEAM_BLUE));     // At ammo depot adjacent position

    allUnits.push_back(new Commander(MAP_SIZE - 3, MAP_SIZE - 3, TEAM_ORANGE));
    allUnits.push_back(new Warrior(MAP_SIZE - 3, MAP_SIZE - 5, TEAM_ORANGE));
    allUnits.push_back(new Warrior(MAP_SIZE - 5, MAP_SIZE - 3, TEAM_ORANGE));
    allUnits.push_back(new Medic(23, 27, TEAM_ORANGE));  // At medical depot adjacent position (23,28)
    allUnits.push_back(new Supply(27, 23, TEAM_ORANGE)); // At ammo depot adjacent position (28,23)
}

void UpdateSafetyMap()
{
    for (int i = 0; i < MAP_SIZE; i++)
        for (int j = 0; j < MAP_SIZE; j++)
            safetyMap[i][j] = 0;

    for (auto unit : allUnits)
    {
        if (!unit->isAlive())
            continue;

        int team = unit->getTeam();

        for (auto enemy : allUnits)
        {
            if (!enemy->isAlive() || enemy->getTeam() == team)
                continue;

            int enemyRow = enemy->getRow();
            int enemyCol = enemy->getCol();

            if (unit->getVisibility(enemyRow, enemyCol))
            {
                int dangerRadius = SHOOTING_RANGE + 2;
                for (int i = enemyRow - dangerRadius; i <= enemyRow + dangerRadius; i++)
                {
                    for (int j = enemyCol - dangerRadius; j <= enemyCol + dangerRadius; j++)
                    {
                        if (i >= 0 && i < MAP_SIZE && j >= 0 && j < MAP_SIZE)
                        {
                            int dist = abs(i - enemyRow) + abs(j - enemyCol);
                            int danger = max(0, 100 - dist * 10);
                            safetyMap[i][j] = max(safetyMap[i][j], danger);
                        }
                    }
                }
            }
        }
    }
}

void CheckGameOver()
{
    int blueAlive = 0;
    int orangeAlive = 0;

    for (auto unit : allUnits)
    {
        if (unit->isAlive())
        {
            if (unit->getTeam() == TEAM_BLUE)
                blueAlive++;
            else
                orangeAlive++;
        }
    }

    if (blueAlive == 0)
    {
        gameOver = true;
        winningTeam = TEAM_ORANGE;
        gameRunning = false;
        cout << "=== GAME OVER: TEAM ORANGE WINS! ===" << endl;
    }
    else if (orangeAlive == 0)
    {
        gameOver = true;
        winningTeam = TEAM_BLUE;
        gameRunning = false;
        cout << "=== GAME OVER: TEAM BLUE WINS! ===" << endl;
    }
}

void ShowMap()
{
    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            switch (map[i][j])
            {
            case SPACE:
                glColor3d(Colors::BACKGROUND_R, Colors::BACKGROUND_G, Colors::BACKGROUND_B);
                break;
            case ROCK:
                glColor3d(Colors::ROCK_R, Colors::ROCK_G, Colors::ROCK_B);
                break;
            case TREE:
                glColor3d(Colors::TREE_R, Colors::TREE_G, Colors::TREE_B);
                break;
            case WATER:
                glColor3d(Colors::WATER_R, Colors::WATER_G, Colors::WATER_B);
                break;
            case AMMO_DEPOT:
            case MEDICAL_DEPOT:
                glColor3d(Colors::DEPOT_R, Colors::DEPOT_G, Colors::DEPOT_B);
                break;
            }

            glBegin(GL_POLYGON);
            glVertex2d(j, i);
            glVertex2d(j + 1, i);
            glVertex2d(j + 1, i + 1);
            glVertex2d(j, i + 1);
            glEnd();

            if (map[i][j] == ROCK)
            {
                glColor3d(0.3, 0.3, 0.3);
                glBegin(GL_TRIANGLES);
                glVertex2d(j + 0.5, i + 0.2);
                glVertex2d(j + 0.2, i + 0.8);
                glVertex2d(j + 0.8, i + 0.8);
                glEnd();
            }
            else if (map[i][j] == TREE)
            {
                glColor3d(0.0, 0.4, 0.1);
                glBegin(GL_TRIANGLES);
                glVertex2d(j + 0.5, i + 0.2);
                glVertex2d(j + 0.2, i + 0.8);
                glVertex2d(j + 0.8, i + 0.8);
                glEnd();
            }
        }
    }

    glColor3d(0.4, 0.5, 0.4);
    glLineWidth(0.5f);
    for (int i = 0; i <= MAP_SIZE; i++)
    {
        glBegin(GL_LINES);
        glVertex2d(0, i);
        glVertex2d(MAP_SIZE, i);
        glEnd();

        glBegin(GL_LINES);
        glVertex2d(i, 0);
        glVertex2d(i, MAP_SIZE);
        glEnd();
    }
}

void AddProjectile(double startRow, double startCol, double endRow, double endCol, int team, bool isGrenade)
{
    Projectile proj;
    proj.startRow = startRow;
    proj.startCol = startCol;
    proj.endRow = endRow;
    proj.endCol = endCol;
    proj.currentRow = startRow;
    proj.currentCol = startCol;
    proj.team = team;
    proj.isGrenade = isGrenade;
    proj.framesAlive = 0;
    proj.active = true;
    activeProjectiles.push_back(proj);

    std::cout << "Created " << (isGrenade ? "GRENADE" : "BULLET") << " from ("
        << startRow << "," << startCol << ") to (" << endRow << "," << endCol << ")" << std::endl;
}

void UpdateProjectiles()
{
    const double PROJECTILE_SPEED = 0.5; // Cells per frame

    for (auto& proj : activeProjectiles)
    {
        if (!proj.active)
            continue;

        proj.framesAlive++;

        // Move towards target
        double dx = proj.endCol - proj.currentCol;
        double dy = proj.endRow - proj.currentRow;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < PROJECTILE_SPEED || proj.framesAlive > 60)
        {
            // Reached target or timed out
            proj.active = false;
        }
        else
        {
            // Move towards target
            proj.currentCol += (dx / dist) * PROJECTILE_SPEED;
            proj.currentRow += (dy / dist) * PROJECTILE_SPEED;
        }
    }

    // Remove inactive projectiles
    activeProjectiles.erase(
        remove_if(activeProjectiles.begin(), activeProjectiles.end(),
            [](const Projectile& p) { return !p.active; }),
        activeProjectiles.end()
    );
}

void DrawProjectiles()
{
    for (const auto& proj : activeProjectiles)
    {
        if (!proj.active)
            continue;

        // Set color based on team
        if (proj.team == TEAM_BLUE)
            glColor3d(Colors::BLUE_DARK_R, Colors::BLUE_DARK_G, Colors::BLUE_DARK_B);
        else
            glColor3d(Colors::ORANGE_DARK_R, Colors::ORANGE_DARK_G, Colors::ORANGE_DARK_B);

        if (proj.isGrenade)
        {
            // Draw grenade as larger dark brown circle
            glColor3d(0.4, 0.2, 0.1); // Dark brown
            glBegin(GL_POLYGON);
            const int segments = 16;
            const double radius = 0.3; // Larger
            for (int i = 0; i < segments; i++)
            {
                double angle = 2.0 * M_PI * i / segments;
                double x = proj.currentCol + radius * cos(angle);
                double y = proj.currentRow + radius * sin(angle);
                glVertex2d(x, y);
            }
            glEnd();
        }
        else
        {
            // Draw bullet as small black circle
            glColor3d(0.0, 0.0, 0.0); // Black
            glBegin(GL_POLYGON);
            const int segments = 8;
            const double radius = 0.15;
            for (int i = 0; i < segments; i++)
            {
                double angle = 2.0 * M_PI * i / segments;
                double x = proj.currentCol + radius * cos(angle);
                double y = proj.currentRow + radius * sin(angle);
                glVertex2d(x, y);
            }
            glEnd();
        }
    }
}

void UpdateGame()
{
    if (!gameRunning)
        return;

    frameCounter++;

    // Update projectile animations
    UpdateProjectiles();

    // Update safety map every frame
    UpdateSafetyMap();

    // Each unit moves at its own frequency
    for (auto unit : allUnits)
    {
        if (unit->isAlive() && unit->shouldMoveThisFrame(frameCounter))
        {
            unit->update(map, safetyMap, allUnits);
        }
    }

    CheckGameOver();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    ShowMap();

    for (auto unit : allUnits)
    {
        if (unit->isAlive())
            unit->show();
    }

    DrawProjectiles();

    if (gameOver)
    {
        glColor3d(1.0, 1.0, 1.0);
        glRasterPos2d(MAP_SIZE / 2 - 2, MAP_SIZE / 2);
        string msg = (winningTeam == TEAM_BLUE) ? "BLUE TEAM WINS!" : "ORANGE TEAM WINS!";
        for (char c : msg)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
    }

    glutSwapBuffers();
}

// Timer callback for consistent frame rate (60 FPS)
void timer(int value)
{
    UpdateGame();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS (16ms per frame)
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case ' ':
        cout << "\n=== NEW GAME STARTED ===" << endl;
        gameRunning = true;
        gameOver = false;
        winningTeam = -1;
        frameCounter = 0;

        InitMap();
        InitUnits();
        break;

    case 27:
        for (auto unit : allUnits)
            delete unit;
        exit(0);
        break;
    }
}

void init()
{
    srand((unsigned int)time(0));
    glClearColor(Colors::BACKGROUND_R, Colors::BACKGROUND_G, Colors::BACKGROUND_B, 0);
    glOrtho(0, MAP_SIZE, MAP_SIZE, 0, -1, 1);

    InitMap();
    InitUnits();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("AI Combat Simulation - Team vs Team");

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, timer, 0); // Start the timer immediately

    init();

    cout << "==================================" << endl;
    cout << "   AI COMBAT SIMULATION GAME" << endl;
    cout << "==================================" << endl;
    cout << "Blue Team vs Orange Team" << endl;
    cout << "Each team: Commander(C), 2 Warriors(W), Medic(M), Supply(P)" << endl;
    cout << "\nCommander uses Visibility Maps to plan strategy" << endl;
    cout << "Warriors attack with A* pathfinding" << endl;
    cout << "Warriors find cover with BFS" << endl;
    cout << "Medic moves 4X FASTER and heals units" << endl;
    cout << "Supply moves 4X FASTER and resupplies units" << endl;
    cout << "\nControls:" << endl;
    cout << "SPACE - Start new game" << endl;
    cout << "ESC - Exit" << endl;
    cout << "\nPress SPACE to begin!" << endl;
    cout << "==================================" << endl;

    glutMainLoop();

    return 0;
}
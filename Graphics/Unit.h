#pragma once
#include "Definitions.h"
#include "Node.h"
#include <vector>

/**
 * Unit class - Base class for all combat units
 * Handles position, health, visibility, and basic movement
 */
class Unit
{
protected:
    int row, col;           // Current position
    int team;               // TEAM_BLUE or TEAM_ORANGE
    int unitType;           // COMMANDER, WARRIOR, MEDIC, SUPPLY
    int health;             // Current health (0-100)
    bool alive;             // Is unit alive
    char symbol;            // Display symbol (C, W, M, P)
    double colorR, colorG, colorB; // Team color
    int moveFrequency;      // Frames between moves (different per unit type)
    int movementCounter;    // Counter for movement timing
    bool visibilityMap[MAP_SIZE][MAP_SIZE];    // Visibility map for this unit

    // Pattern detection for infinite loops (tracks last 8 positions)
    int posHistory[8][2]; // Stores last 8 (row, col) positions
    int posHistoryIndex;
    int posHistoryCount;

    int framesInTree;      // Tree hiding time limit (10 seconds max)
    int loopBreakCooldown; // Frames to wait after breaking from loop
    bool escapingTree;
    int stuckInTreeCounter;  // Counter for how many times we tried to escape tree


public:
    /**
     * Constructor
     */
    Unit(int startRow, int startCol, int teamId, int type, char sym);

    /**
     * Virtual destructor
     */
    virtual ~Unit();

    // Getters
    int getRow() const { return row; }
    int getCol() const { return col; }
    int getTeam() const { return team; }
    int getUnitType() const { return unitType; }
    int getHealth() const { return health; }
    bool isAlive() const { return alive; }
    char getSymbol() const { return symbol; }
    int getMoveFrequency() const { return moveFrequency; }

    /**
     * Check if unit should move this frame
     */
    bool shouldMoveThisFrame(int globalFrameCounter);

    /**
     * Take damage
     */
    void takeDamage(int damage);

    /**
     * Heal unit
     */
    void heal(int amount);

    /**
     * Check if position is valid for movement (includes depot check)
     */
    bool isValidMove(int map[][MAP_SIZE], int r, int c) const;

    /**
     * Check if position is occupied by another unit
     */
    bool isPositionOccupied(int r, int c, std::vector<Unit*>& allUnits) const;

    /**
     * Check if position is valid AND not occupied
     */
    bool isPositionAvailable(int map[][MAP_SIZE], int r, int c, std::vector<Unit*>& allUnits) const;

    /**
     * Check if can see position (line of sight)
     */
    bool canSeePosition(int map[][MAP_SIZE], int targetRow, int targetCol) const;

    /**
     * Update visibility map for this unit
     */
    void updateVisibilityMap(int map[][MAP_SIZE]);

    /**
     * Get visibility map
     */
    bool getVisibility(int r, int c) const;

    /**
     * Find path using A* algorithm with safety consideration and unit collision
     */
    std::vector<std::pair<int, int>> findPathAStar(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        int targetRow,
        int targetCol,
        std::vector<Unit*>& allUnits
    );

    /**
     * Find nearest cover using BFS
     */
    bool findNearestCover(
        int map[][MAP_SIZE],
        int& coverRow,
        int& coverCol
    );

    /**
     * Find nearest open space (non-tree) using BFS
     */
    bool findNearestOpenSpace(
        int map[][MAP_SIZE],
        int& openRow,
        int& openCol
    );

    /**
     * Move one step towards target with collision avoidance
     */
    void moveTowards(int targetRow, int targetCol, int map[][MAP_SIZE], std::vector<Unit*>& allUnits);

    /**
     * Try to unstuck if unit is stuck in same position
     */
    void tryUnstuck(int map[][MAP_SIZE], std::vector<Unit*>& allUnits);

    /**
     * Detect repeating patterns: A->B, A->B->C, A->B->C->D cycles
     * Returns true if pattern detected and unit forced to break free
     */
    bool detectRepeatingPattern(int map[][MAP_SIZE], std::vector<Unit*>& allUnits);

    /**
     * Check if hiding in tree for too long (>10 seconds) and force exit
     * Returns true if forced to leave tree
     */
    bool checkTreeHidingLimit(int map[][MAP_SIZE], std::vector<Unit*>& allUnits);

    /**
     * Display unit on screen
     */
    virtual void show() const;

    /**
     * Update unit behavior (to be overridden by derived classes)
     */
    virtual void update(
        int map[][MAP_SIZE],
        int safetyMap[][MAP_SIZE],
        std::vector<Unit*>& allUnits
    ) = 0;
};

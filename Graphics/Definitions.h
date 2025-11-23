#pragma once

/**
 * Global definitions and constants for AI Combat Simulation
 */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 // Map cell types
const int SPACE = 0;
const int ROCK = 1;      // Blocks movement, shooting, and visibility
const int TREE = 2;      // Allows movement, blocks shooting and visibility
const int WATER = 3;     // Blocks movement, allows shooting and visibility
const int AMMO_DEPOT = 4;
const int MEDICAL_DEPOT = 5;

// Unit types
const int COMMANDER = 0;
const int WARRIOR = 1;
const int MEDIC = 2;
const int SUPPLY = 3;

// Teams
const int TEAM_BLUE = 0;
const int TEAM_ORANGE = 1;
const int NO_TEAM = -1;

// Movement directions
const int RIGHT = 0;
const int UP = 1;
const int DOWN = 2;
const int LEFT = 3;
const int STAY = -1;

// Game constants
const int MAP_SIZE = 30;
const int NUM_UNITS_PER_TEAM = 5; // Commander + 2 Warriors + Medic + Supply

// Combat parameters
const int MAX_HEALTH = 100;
const int CRITICAL_HEALTH = 40; // Below this, warrior,commander and supplier needs medic
const int MAX_AMMO = 15;
const int LOW_AMMO = 5; // Below this, warrior requests supply
const int MAX_GRENADES = 3;
const int SHOOTING_RANGE = 8;
const int GRENADE_RANGE = 6;
const int VISIBILITY_RANGE = 15;

// AI parameters
const int MAX_BFS_DEPTH = 20; // For finding cover
const int DANGER_THRESHOLD = 30; // Safety map threshold

// Unit stats
const int MEDIC_HEAL_AMOUNT = 100;  // Heal to full health
const int SUPPLY_AMMO_AMOUNT = 10;
const int SUPPLY_GRENADE_AMOUNT = 3;
const int MAX_MEDIC_CHARGES = 1;   // Medic has 1 charge - must return to depot after each heal
const int MAX_SUPPLY_CHARGES = 1;  // Supply has 1 charge - must return to depot after each resupply

// Movement speeds (frames per move) - MEDIC AND SUPPLY ARE 4X FASTER THAN WARRIOR
// Adjusted for 60 FPS: lower values = faster movement
const int WARRIOR_MOVE_FREQ = 30;    // Moves every 0.5 seconds (30 frames at 60 FPS)
const int MEDIC_MOVE_FREQ = 8;       // 4x faster - moves every ~0.13 seconds
const int SUPPLY_MOVE_FREQ = 8;      // 4x faster - moves every ~0.13 seconds
const int COMMANDER_MOVE_FREQ = 30;  // Same as warrior

// Colors
namespace Colors
{
    // Background
    const double BACKGROUND_R = 0.6, BACKGROUND_G = 0.8, BACKGROUND_B = 0.5; // Light green

    // Obstacles
    const double ROCK_R = 0.5, ROCK_G = 0.5, ROCK_B = 0.5;      // Gray
    const double TREE_R = 0.0, TREE_G = 0.6, TREE_B = 0.2;      // Dark green
    const double WATER_R = 0.3, WATER_G = 0.5, WATER_B = 0.9;   // Blue

    // Depots
    const double DEPOT_R = 1.0, DEPOT_G = 1.0, DEPOT_B = 0.0;   // Yellow

    // Team Blue
    const double BLUE_R = 0.4, BLUE_G = 0.6, BLUE_B = 1.0;
    const double BLUE_DARK_R = 0.0, BLUE_DARK_G = 0.0, BLUE_DARK_B = 0.8;

    // Team Orange
    const double ORANGE_R = 1.0, ORANGE_G = 0.6, ORANGE_B = 0.2;
    const double ORANGE_DARK_R = 0.9, ORANGE_DARK_G = 0.4, ORANGE_DARK_B = 0.0;

    // Text
    const double TEXT_R = 0.0, TEXT_G = 0.0, TEXT_B = 0.0;      // Black
    const double TEXT_WHITE_R = 1.0, TEXT_WHITE_G = 1.0, TEXT_WHITE_B = 1.0; // White
}

#pragma once
#include <cmath>

/**
 * Node class - represents a state in the search space for both A* and BFS algorithms
 * Used by all units for pathfinding
 */
class Node
{
private:
    int row, col;   // Position in the map
    double g;       // Cost from start (distance traveled)
    double h;       // Heuristic cost to goal
    double f;       // Total cost f = g + h (used in A*)
    int depth;      // Depth in BFS (used for limited-depth BFS)
    Node* parent;   // Parent node for path reconstruction

public:
    /**
     * Default constructor - creates start node
     */
    Node();

    /**
     * Constructor with position - used for creating specific nodes
     */
    Node(int r, int c);

    /**
     * Constructor for child node - creates a node from parent with new position
     * @param parentNode - parent node
     * @param newRow - new row position
     * @param newCol - new column position
     */
    Node(Node* parentNode, int newRow, int newCol);

    // Getters
    int getRow() const { return row; }
    int getCol() const { return col; }
    double getG() const { return g; }
    double getH() const { return h; }
    double getF() const { return f; }
    int getDepth() const { return depth; }
    Node* getParent() const { return parent; }

    // Setters
    void setG(double value) { g = value; updateF(); }
    void setH(double value) { h = value; updateF(); }
    void setParent(Node* p) { parent = p; }
    void setDepth(int d) { depth = d; }

    /**
     * Calculate Manhattan distance heuristic to target position
     * @param targetRow - target row
     * @param targetCol - target column
     */
    void calculateHeuristic(int targetRow, int targetCol);

    /**
     * Update F value (f = g + h) used in A* algorithm
     */
    void updateF();

    /**
     * Equality operator - compares positions
     */
    bool operator==(const Node& other) const;

    /**
     * Check if this position is the same as given coordinates
     */
    bool isAt(int r, int c) const;
};

// Implementation:
inline Node::Node()
{
    row = col = 0;
    g = h = f = 0;
    depth = 0;
    parent = nullptr;
}

inline Node::Node(int r, int c)
{
    row = r;
    col = c;
    g = h = f = 0;
    depth = 0;
    parent = nullptr;
}

inline Node::Node(Node* parentNode, int newRow, int newCol)
{
    row = newRow;
    col = newCol;
    parent = parentNode;

    if (parent != nullptr)
    {
        g = parent->g + 1; // Each step costs 1
        depth = parent->depth + 1;
    }
    else
    {
        g = 0;
        depth = 0;
    }

    h = 0;
    updateF();
}

inline void Node::calculateHeuristic(int targetRow, int targetCol)
{
    // Manhattan distance heuristic
    h = std::abs(row - targetRow) + std::abs(col - targetCol);
    updateF();
}

inline void Node::updateF()
{
    f = g + h;
}

inline bool Node::operator==(const Node& other) const
{
    return (row == other.row && col == other.col);
}

inline bool Node::isAt(int r, int c) const
{
    return (row == r && col == c);
}



#pragma once
#include "Node.h"

/**
 * CompareNodes class - Comparator for priority queue in A* algorithm
 * Used to maintain min-heap based on F values (lower F = higher priority)
 */
class CompareNodes
{
public:
    /**
     * Comparison operator for priority queue
     * Returns true if n1 should have lower priority than n2
     * Since priority_queue is max-heap by default, we return n1->F > n2->F
     * to make it behave like a min-heap for A*
     * @param n1 - first node to compare
     * @param n2 - second node to compare
     * @return true if n1 has lower priority (higher F value)
     */
    bool operator()(Node* n1, Node* n2)
    {
        return n1->getF() > n2->getF();
    }
};


#ifndef STATE_H
#define STATE_H

#include <memory>
#include <vector>

/**
 * @brief SearchNode represents a state in the state-space Dijkstra search tree.
 */
struct SearchNode {
    int node;                        // Current city/node index
    double fuel;                     // Current fuel remaining in the tank
    double cost;                     // Accumulated monetary cost (fuel purchase + tolls)
    double time;                     // Accumulated travel time
    std::shared_ptr<SearchNode> parent; // Pointer to the predecessor state for path reconstruction
    bool is_refuel_action = false;   // True if this state was reached by buying fuel at the current node
    double fuel_bought = 0.0;        // The amount of fuel bought during this transition

    SearchNode(int n, double f, double c, double t, 
               std::shared_ptr<SearchNode> p = nullptr, 
               bool refuel = false, double fb = 0.0)
        : node(n), fuel(f), cost(c), time(t), parent(p), is_refuel_action(refuel), fuel_bought(fb) {}
};

/**
 * @brief PathSegment represents a reconstructed step in the final routing output.
 */
struct PathSegment {
    int node;
    double fuel;
    double cost;
    double time;
    bool is_refuel_action;
    double fuel_bought;
};

using Path = std::vector<PathSegment>;

#endif // STATE_H

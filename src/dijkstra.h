#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <vector>
#include <memory>
#include "graph.h"
#include "state.h"
#include "fuel_manager.h"
#include "pareto.h"

/**
 * @brief Configuration settings for the route planner.
 */
struct OptimizerSettings {
    double initial_fuel = 10.0;       // Initial fuel level
    double tank_capacity = 50.0;      // Maximum fuel capacity of the tank
    double fuel_step = 1.0;           // Step size for refueling transitions
    double refuel_overhead_time = 0.0; // Extra time penalty per refueling action (e.g. 0.1 hours)
};

/**
 * @brief DijkstraOptimizer executes the state-space Pareto-optimal path search.
 */
class DijkstraOptimizer {
private:
    const Graph& graph;
    FuelManager fuel_manager;
    double initial_fuel;
    double refuel_overhead_time;

    bool debug_mode = false;
    int expanded_states_count = 0;

public:
    DijkstraOptimizer(const Graph& g, const OptimizerSettings& settings);

    void set_debug(bool enable) { debug_mode = enable; }
    int get_expanded_states_count() const { return expanded_states_count; }

    /**
     * @brief Finds all non-dominated (Pareto-optimal) paths from start to destination.
     * @param start_node Start node index.
     * @param dest_node Destination node index.
     * @return A vector of Pareto-optimal completed destination states.
     */
    std::vector<Pareto::DestState> find_paths(int start_node, int dest_node);
};

#endif // DIJKSTRA_H

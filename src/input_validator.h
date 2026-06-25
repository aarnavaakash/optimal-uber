#ifndef INPUT_VALIDATOR_H
#define INPUT_VALIDATOR_H

#include <stdexcept>
#include <string>
#include "graph.h"

/**
 * @brief InputValidator validates network graph structures and vehicle routing queries.
 * Throws std::invalid_argument exceptions for errors.
 */
class InputValidator {
public:
    /**
     * @brief Performs structural checks on loaded nodes and edges.
     */
    static void validate_network(const Graph& graph) {
        const auto& nodes = graph.get_nodes();
        int num_nodes = graph.get_num_nodes();

        for (const auto& node : nodes) {
            if (node.id < 0 || node.id >= num_nodes) {
                throw std::invalid_argument("Error: Node ID " + std::to_string(node.id) + 
                                            " is out of range [0, " + std::to_string(num_nodes - 1) + "].");
            }
            if (node.fuel_price < 0.0) {
                throw std::invalid_argument("Error: Fuel price at Node " + std::to_string(node.id) + 
                                            " (" + node.name + ") cannot be negative (" + 
                                            std::to_string(node.fuel_price) + ").");
            }
        }

        for (int i = 0; i < num_nodes; ++i) {
            for (const auto& edge : graph.get_neighbors(i)) {
                if (edge.to < 0 || edge.to >= num_nodes) {
                    throw std::invalid_argument("Error: Edge from Node " + std::to_string(i) + 
                                                " points to invalid Node " + std::to_string(edge.to) + ".");
                }
                if (edge.fuel_consumption <= 0.0) {
                    throw std::invalid_argument("Error: Edge from Node " + std::to_string(i) + 
                                                " to Node " + std::to_string(edge.to) + 
                                                " has non-positive fuel consumption (" + 
                                                std::to_string(edge.fuel_consumption) + ").");
                }
                if (edge.time < 0.0) {
                    throw std::invalid_argument("Error: Edge from Node " + std::to_string(i) + 
                                                " to Node " + std::to_string(edge.to) + 
                                                " has negative travel time (" + 
                                                std::to_string(edge.time) + ").");
                }
                if (edge.travel_cost < 0.0) {
                    throw std::invalid_argument("Error: Edge from Node " + std::to_string(i) + 
                                                " to Node " + std::to_string(edge.to) + 
                                                " has negative travel cost/toll (" + 
                                                std::to_string(edge.travel_cost) + ").");
                }
            }
        }
    }

    /**
     * @brief Performs checks on routing query values and vehicle parameters.
     */
    static void validate_query(const Graph& graph, int source, int dest, double capacity, double initial_fuel) {
        int num_nodes = graph.get_num_nodes();
        if (source < 0 || source >= num_nodes) {
            throw std::invalid_argument("Error: Source node ID " + std::to_string(source) + " is invalid.");
        }
        if (dest < 0 || dest >= num_nodes) {
            throw std::invalid_argument("Error: Destination node ID " + std::to_string(dest) + " is invalid.");
        }
        if (capacity <= 0.0) {
            throw std::invalid_argument("Error: Tank capacity must be positive (" + std::to_string(capacity) + ").");
        }
        if (initial_fuel < 0.0 || initial_fuel > capacity) {
            throw std::invalid_argument("Error: Initial fuel must be in range [0, " + std::to_string(capacity) + 
                                        "] (value: " + std::to_string(initial_fuel) + ").");
        }

        // No global check here. Dijkstra naturally bypasses impassable edges.
    }
};

#endif // INPUT_VALIDATOR_H

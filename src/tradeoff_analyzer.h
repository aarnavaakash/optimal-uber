#ifndef TRADEOFF_ANALYZER_H
#define TRADEOFF_ANALYZER_H

#include <vector>
#include <memory>
#include <utility>
#include "pareto.h"
#include "state.h"

/**
 * @brief PathSummary holds processed info about a Pareto-optimal path.
 */
struct PathSummary {
    int index;                                    // 1-based index of the path
    double cost;                                  // Total travel cost
    double time;                                  // Total travel time
    double normalized_cost;                       // Normalized cost in [0, 1]
    double normalized_time;                       // Normalized time in [0, 1]
    double score;                                 // Weighted score (lower is better)
    std::vector<int> route;                       // Node ID sequence of the route
    std::vector<std::pair<int, double>> fuel_purchases; // Merged fuel purchases: {node_id, total_fuel_bought}
    std::shared_ptr<SearchNode> end_node;         // Reference to final search node
};

/**
 * @brief TradeoffAnalyzer analyzes and scores Pareto-optimal paths.
 */
class TradeoffAnalyzer {
public:
    /**
     * @brief Normalizes costs/times and computes weighted scores to identify cheapest, fastest, and balanced paths.
     * @param paths Raw Pareto-optimal paths from the Dijkstra engine.
     * @param alpha Cost importance weight (default 0.5).
     * @param beta Time importance weight (default 0.5).
     * @return List of analyzed paths.
     */
    static std::vector<PathSummary> analyze(const std::vector<Pareto::DestState>& paths, 
                                            double alpha = 0.5, 
                                            double beta = 0.5);
};

#endif // TRADEOFF_ANALYZER_H

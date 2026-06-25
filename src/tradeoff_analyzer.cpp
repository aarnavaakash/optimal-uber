#include "tradeoff_analyzer.h"
#include <algorithm>
#include <cmath>
#include <limits>

std::vector<PathSummary> TradeoffAnalyzer::analyze(const std::vector<Pareto::DestState>& paths, 
                                                  double alpha, 
                                                  double beta) {
    std::vector<PathSummary> summaries;
    if (paths.empty()) {
        return summaries;
    }

    double min_cost = std::numeric_limits<double>::max();
    double max_cost = -std::numeric_limits<double>::max();
    double min_time = std::numeric_limits<double>::max();
    double max_time = -std::numeric_limits<double>::max();

    // 1. Reconstruct routes and collect costs/times
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& path_state = paths[i];
        min_cost = std::min(min_cost, path_state.cost);
        max_cost = std::max(max_cost, path_state.cost);
        min_time = std::min(min_time, path_state.time);
        max_time = std::max(max_time, path_state.time);

        PathSummary summary;
        summary.index = static_cast<int>(i) + 1;
        summary.cost = path_state.cost;
        summary.time = path_state.time;
        summary.end_node = path_state.node;

        // Traverse parent pointers backwards to reconstruct the path
        std::vector<std::shared_ptr<SearchNode>> nodes_seq;
        auto curr = path_state.node;
        while (curr != nullptr) {
            nodes_seq.push_back(curr);
            curr = curr->parent;
        }
        std::reverse(nodes_seq.begin(), nodes_seq.end());

        // Extract route node sequence and merge contiguous refueling actions
        std::vector<int> route;
        std::vector<std::pair<int, double>> fuel_purchases;
        
        int last_node = -1;
        for (const auto& sn : nodes_seq) {
            // Append node to route sequence only on city changes
            if (sn->node != last_node) {
                route.push_back(sn->node);
                last_node = sn->node;
            }
            // Merge consecutive refueling actions at the same node
            if (sn->is_refuel_action && sn->fuel_bought > 0.0) {
                if (!fuel_purchases.empty() && fuel_purchases.back().first == sn->node) {
                    fuel_purchases.back().second += sn->fuel_bought;
                } else {
                    fuel_purchases.push_back({sn->node, sn->fuel_bought});
                }
            }
        }

        summary.route = route;
        summary.fuel_purchases = fuel_purchases;
        summaries.push_back(summary);
    }

    // 2. Normalize cost and time, and compute weighted score
    double cost_range = max_cost - min_cost;
    double time_range = max_time - min_time;

    for (auto& s : summaries) {
        s.normalized_cost = (cost_range > Pareto::EPS) ? (s.cost - min_cost) / cost_range : 0.0;
        s.normalized_time = (time_range > Pareto::EPS) ? (s.time - min_time) / time_range : 0.0;
        s.score = alpha * s.normalized_cost + beta * s.normalized_time;
    }

    return summaries;
}

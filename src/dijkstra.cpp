#include "dijkstra.h"
#include <queue>
#include <iostream>
#include <cmath>

DijkstraOptimizer::DijkstraOptimizer(const Graph& g, const OptimizerSettings& settings)
    : graph(g),
      fuel_manager(settings.tank_capacity, settings.fuel_step),
      initial_fuel(settings.initial_fuel),
      refuel_overhead_time(settings.refuel_overhead_time) {}

std::vector<Pareto::DestState> DijkstraOptimizer::find_paths(int start_node, int dest_node) {
    expanded_states_count = 0;

    // Custom comparator for min-heap priority queue
    struct CompareNode {
        bool operator()(const std::shared_ptr<SearchNode>& a, const std::shared_ptr<SearchNode>& b) const {
            // Primarily sort by cost (cheaper first)
            if (std::abs(a->cost - b->cost) > Pareto::EPS) {
                return a->cost > b->cost;
            }
            // Secondarily sort by time (faster first)
            if (std::abs(a->time - b->time) > Pareto::EPS) {
                return a->time > b->time;
            }
            // Tertiarily sort by fuel (more fuel first -> in min-heap sense, smaller is higher priority, so fuel < other.fuel)
            return a->fuel < b->fuel;
        }
    };

    std::priority_queue<std::shared_ptr<SearchNode>,
                        std::vector<std::shared_ptr<SearchNode>>,
                        CompareNode> pq;

    int num_nodes = graph.get_num_nodes();
    // pareto_fronts[u] contains all non-dominated (fuel, cost, time) states at node u
    std::vector<std::vector<Pareto::State>> pareto_fronts(num_nodes);
    std::vector<Pareto::DestState> dest_pareto_front;

    // Validate inputs
    if (start_node < 0 || start_node >= num_nodes || dest_node < 0 || dest_node >= num_nodes) {
        return dest_pareto_front;
    }

    // Initialize search
    double starting_fuel = std::min(initial_fuel, fuel_manager.get_tank_capacity());
    auto start_state = std::make_shared<SearchNode>(start_node, starting_fuel, 0.0, 0.0);
    pq.push(start_state);
    Pareto::update_front(pareto_fronts[start_node], {starting_fuel, 0.0, 0.0});

    while (!pq.empty()) {
        auto curr = pq.top();
        pq.pop();

        // --- Pruning Check 1: Domination by completed destination paths ---
        // If a completed path at destination is strictly better in both cost and time, prune this branch
        bool dominated_by_dest = false;
        for (const auto& dest_s : dest_pareto_front) {
            if (dest_s.cost <= curr->cost + Pareto::EPS && dest_s.time <= curr->time + Pareto::EPS) {
                dominated_by_dest = true;
                break;
            }
        }
        if (dominated_by_dest) {
            continue;
        }

        // --- Pruning Check 2: Domination at the current node ---
        // If this state has been dominated by a state added to pareto_fronts[curr->node] later, prune it.
        bool dominated_at_node = false;
        for (const auto& s : pareto_fronts[curr->node]) {
            if (Pareto::dominates(s, {curr->fuel, curr->cost, curr->time})) {
                dominated_at_node = true;
                break;
            }
        }
        if (dominated_at_node) {
            continue;
        }

        expanded_states_count++;
        if (debug_mode && expanded_states_count % 5000 == 0) {
            std::cout << "[DEBUG] Expanded " << expanded_states_count << " states. PQ size: " << pq.size() << std::endl;
        }

        // --- Goal Check ---
        if (curr->node == dest_node) {
            Pareto::DestState ds = {curr->cost, curr->time, curr};
            Pareto::update_dest_front(dest_pareto_front, ds);
            continue; // Stop expanding from the destination node
        }

        // --- Transition 1: TRAVEL ---
        for (const auto& edge : graph.get_neighbors(curr->node)) {
            if (fuel_manager.can_travel(curr->fuel, edge.fuel_consumption)) {
                double next_fuel = fuel_manager.travel_remaining_fuel(curr->fuel, edge.fuel_consumption);
                double next_cost = curr->cost + edge.travel_cost;
                double next_time = curr->time + edge.time;

                Pareto::State next_state_pareto = {next_fuel, next_cost, next_time};
                
                // If it is non-dominated at the next node, we save it and push to queue
                if (Pareto::update_front(pareto_fronts[edge.to], next_state_pareto)) {
                    auto next_node = std::make_shared<SearchNode>(edge.to, next_fuel, next_cost, next_time, curr, false, 0.0);
                    pq.push(next_node);
                }
            }
        }

        // --- Transition 2: BUY FUEL ---
        double price = graph.get_fuel_price(curr->node);
        if (price < 1e9 - Pareto::EPS) {
            double refuel_amt = fuel_manager.get_refuel_amount(curr->fuel);
            if (refuel_amt > Pareto::EPS) {
                double next_fuel = curr->fuel + refuel_amt;
                double refuel_cost = fuel_manager.calculate_refuel_cost(refuel_amt, price);
                double next_cost = curr->cost + refuel_cost;
                double next_time = curr->time + refuel_overhead_time;

                Pareto::State next_state_pareto = {next_fuel, next_cost, next_time};

                // If buying fuel leads to a non-dominated state, push to queue
                if (Pareto::update_front(pareto_fronts[curr->node], next_state_pareto)) {
                    auto next_node = std::make_shared<SearchNode>(curr->node, next_fuel, next_cost, next_time, curr, true, refuel_amt);
                    pq.push(next_node);
                }
            }
        }
    }

    return dest_pareto_front;
}

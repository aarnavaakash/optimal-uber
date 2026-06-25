#ifndef PARETO_H
#define PARETO_H

#include <vector>
#include <algorithm>
#include <memory>
#include <cmath>
#include "state.h"

namespace Pareto {
    constexpr double EPS = 1e-7;

    struct State {
        double fuel;
        double cost;
        double time;
    };

    /**
     * @brief Checks if State A dominates State B in 3D objective space.
     * State A dominates B if A is at least as good as B in all objectives,
     * and strictly better in at least one objective.
     * We want to maximize fuel, minimize cost, and minimize time.
     */
    inline bool dominates(const State& A, const State& B) {
        return (A.fuel >= B.fuel - EPS) &&
               (A.cost <= B.cost + EPS) &&
               (A.time <= B.time + EPS) &&
               ((A.fuel > B.fuel + EPS) || (A.cost < B.cost - EPS) || (A.time < B.time - EPS));
    }

    /**
     * @brief Checks if two states are virtually identical.
     */
    inline bool is_duplicate(const State& A, const State& B) {
        return std::abs(A.fuel - B.fuel) < EPS &&
               std::abs(A.cost - B.cost) < EPS &&
               std::abs(A.time - B.time) < EPS;
    }

    /**
     * @brief Updates the Pareto front with a new candidate state.
     * If the new state is dominated or is a duplicate of any existing state, returns false.
     * If the new state is non-dominated, inserts it and removes all existing states that it dominates.
     */
    inline bool update_front(std::vector<State>& front, const State& new_state) {
        for (const auto& s : front) {
            if (is_duplicate(s, new_state) || dominates(s, new_state)) {
                return false;
            }
        }

        front.erase(
            std::remove_if(front.begin(), front.end(), [&](const State& s) {
                return dominates(new_state, s);
            }),
            front.end()
        );

        front.push_back(new_state);
        return true;
    }

    /**
     * @brief Pareto state for completed paths at the destination.
     * At the destination, we only optimize two objectives: minimize cost and minimize time.
     */
    struct DestState {
        double cost;
        double time;
        std::shared_ptr<SearchNode> node;
    };

    /**
     * @brief Checks if completed path A dominates completed path B.
     */
    inline bool dest_dominates(const DestState& A, const DestState& B) {
        return (A.cost <= B.cost + EPS) &&
               (A.time <= B.time + EPS) &&
               ((A.cost < B.cost - EPS) || (A.time < B.time - EPS));
    }

    inline bool is_dest_duplicate(const DestState& A, const DestState& B) {
        return std::abs(A.cost - B.cost) < EPS &&
               std::abs(A.time - B.time) < EPS;
    }

    /**
     * @brief Updates the destination's 2D Pareto front.
     */
    inline bool update_dest_front(std::vector<DestState>& front, const DestState& new_state) {
        for (const auto& s : front) {
            if (is_dest_duplicate(s, new_state) || dest_dominates(s, new_state)) {
                return false;
            }
        }

        front.erase(
            std::remove_if(front.begin(), front.end(), [&](const DestState& s) {
                return dest_dominates(new_state, s);
            }),
            front.end()
        );

        front.push_back(new_state);
        return true;
    }
}

#endif // PARETO_H

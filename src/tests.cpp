#include "tests.h"
#include "graph.h"
#include "dijkstra.h"
#include "tradeoff_analyzer.h"
#include <iostream>
#include <cmath>

#define ASSERT_TRUE(x) \
    if (!(x)) { \
        std::cerr << "  [FAIL] Assertion failed: " << #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_EQUAL_INT(a, b) \
    if ((a) != (b)) { \
        std::cerr << "  [FAIL] Assertion failed: " << #a << " (" << (a) << ") == " << #b << " (" << (b) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_NEAR(a, b, tolerance) \
    if (std::abs((a) - (b)) > (tolerance)) { \
        std::cerr << "  [FAIL] Assertion failed: " << #a << " (" << (a) << ") is near " << #b << " (" << (b) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

// Test 1: Simple linear route where no refueling is necessary because the initial fuel is sufficient.
static bool test_no_refuel_necessary() {
    Graph g(3, true); // Directed
    g.add_node(0, "A", 1.5);
    g.add_node(1, "B", 1.5);
    g.add_node(2, "C", 1.5);

    // Node 0 -> 1: fuel = 2, time = 1, cost = 5 (toll)
    // Node 1 -> 2: fuel = 3, time = 2, cost = 10 (toll)
    g.add_edge(0, 1, 10.0, 1.0, 2.0, 5.0);
    g.add_edge(1, 2, 20.0, 2.0, 3.0, 10.0);

    OptimizerSettings settings;
    settings.initial_fuel = 10.0;
    settings.tank_capacity = 10.0;
    settings.fuel_step = 1.0;
    settings.refuel_overhead_time = 0.0;

    DijkstraOptimizer optimizer(g, settings);
    auto paths = optimizer.find_paths(0, 2);

    ASSERT_TRUE(!paths.empty());

    auto summaries = TradeoffAnalyzer::analyze(paths);
    ASSERT_TRUE(!summaries.empty());

    // With 10 initial fuel, we can traverse 2.0 and 3.0 consumption without buying any fuel.
    // Cost should be exactly 5.0 + 10.0 = 15.0 (tolls only).
    // Time should be 1.0 + 2.0 = 3.0.
    ASSERT_NEAR(summaries[0].cost, 15.0, 1e-5);
    ASSERT_NEAR(summaries[0].time, 3.0, 1e-5);
    ASSERT_EQUAL_INT(static_cast<int>(summaries[0].fuel_purchases.size()), 0);

    return true;
}

// Test 2: We start with 0 fuel and must buy fuel before traveling.
static bool test_refuel_obligatory() {
    Graph g(3, true);
    g.add_node(0, "A", 1.0); // Fuel price = 1.0
    g.add_node(1, "B", 2.0); // Fuel price = 2.0
    g.add_node(2, "C", 1.5);

    // Node 0 -> 1: fuel = 4.0, time = 1.0, cost = 0.0
    // Node 1 -> 2: fuel = 3.0, time = 1.0, cost = 0.0
    g.add_edge(0, 1, 10.0, 1.0, 4.0, 0.0);
    g.add_edge(1, 2, 10.0, 1.0, 3.0, 0.0);

    OptimizerSettings settings;
    settings.initial_fuel = 0.0;
    settings.tank_capacity = 10.0;
    settings.fuel_step = 1.0; // Integer step
    settings.refuel_overhead_time = 0.0;

    DijkstraOptimizer optimizer(g, settings);
    auto paths = optimizer.find_paths(0, 2);

    ASSERT_TRUE(!paths.empty());

    auto summaries = TradeoffAnalyzer::analyze(paths);
    ASSERT_TRUE(!summaries.empty());

    // Reconstructing the paths:
    // Path 1 (cheapest): 
    // Start at A (fuel 0). Must buy at least 4.0 units.
    // Option A: Buy exactly 4.0 at A (cost 4.0 * 1.0 = 4.0). Travel A->B. Arrive at B with 0 fuel.
    // At B: Must buy at least 3.0 units to reach C (cost 3.0 * 2.0 = 6.0). Travel B->C. Arrive at C with 0 fuel.
    // Total cost = 4.0 + 6.0 = 10.0.
    // Option B: Buy 7.0 at A (cost 7.0 * 1.0 = 7.0). Travel A->B (arriving with 3.0 fuel).
    // At B: Travel B->C without refueling.
    // Total cost = 7.0.
    // Clearly, Option B (buy 7.0 at A where it is cheaper) is optimal.
    // Total cost should be 7.0. Total time = 2.0.
    // Let's verify that the cheapest path has cost = 7.0.
    
    double min_cost = summaries[0].cost;
    for (const auto& s : summaries) {
        if (s.cost < min_cost) {
            min_cost = s.cost;
        }
    }
    ASSERT_NEAR(min_cost, 7.0, 1e-5);

    return true;
}

// Test 3: Choosing routes and refueling amounts based on node fuel prices.
static bool test_price_aware_refueling() {
    Graph g(3, true);
    g.add_node(0, "A", 2.0); // Expensive fuel
    g.add_node(1, "B", 1.0); // Cheap fuel
    g.add_node(2, "C", 1.5);

    // Edge 0->1: fuel = 4.0, time = 1.0
    // Edge 1->2: fuel = 5.0, time = 1.0
    g.add_edge(0, 1, 10.0, 1.0, 4.0, 0.0);
    g.add_edge(1, 2, 10.0, 1.0, 5.0, 0.0);

    OptimizerSettings settings;
    settings.initial_fuel = 2.0; // Start with 2.0
    settings.tank_capacity = 10.0;
    settings.fuel_step = 1.0;

    DijkstraOptimizer optimizer(g, settings);
    auto paths = optimizer.find_paths(0, 2);

    ASSERT_TRUE(!paths.empty());
    auto summaries = TradeoffAnalyzer::analyze(paths);

    // Start at A with 2.0. Must reach B (need 4.0).
    // So we must buy exactly 2.0 units at A (expensive: 2.0 * 2.0 = 4.0).
    // We arrive at B with 0.0 fuel.
    // To reach C, we need 5.0. We should buy 5.0 at B (cheap: 5.0 * 1.0 = 5.0).
    // Total cost = 4.0 + 5.0 = 9.0.
    
    double min_cost = summaries[0].cost;
    for (const auto& s : summaries) {
        if (s.cost < min_cost) {
            min_cost = s.cost;
        }
    }
    ASSERT_NEAR(min_cost, 9.0, 1e-5);

    return true;
}

// Test 4: Handles cases where the graph is disconnected and no path exists.
static bool test_disconnected_graph() {
    Graph g(4, true);
    g.add_node(0, "A", 1.5);
    g.add_node(1, "B", 1.5);
    g.add_node(2, "C", 1.5);
    g.add_node(3, "D", 1.5);

    g.add_edge(0, 1, 10.0, 1.0, 2.0, 0.0);
    g.add_edge(2, 3, 10.0, 1.0, 2.0, 0.0);

    OptimizerSettings settings;
    DijkstraOptimizer optimizer(g, settings);
    auto paths = optimizer.find_paths(0, 3);

    ASSERT_TRUE(paths.empty());

    return true;
}

// Test 5: Tank capacity is too small to cross an edge.
static bool test_insufficient_tank_capacity() {
    Graph g(3, true);
    g.add_node(0, "A", 1.5);
    g.add_node(1, "B", 1.5);
    g.add_node(2, "C", 1.5);

    g.add_edge(0, 1, 10.0, 1.0, 15.0, 0.0); // Consumption = 15.0
    g.add_edge(1, 2, 10.0, 1.0, 2.0, 0.0);

    OptimizerSettings settings;
    settings.initial_fuel = 10.0;
    settings.tank_capacity = 10.0; // Max fuel is 10, cannot ever hold 15

    DijkstraOptimizer optimizer(g, settings);
    auto paths = optimizer.find_paths(0, 2);

    ASSERT_TRUE(paths.empty());

    return true;
}

// Test 6: Node 1 has no petrol pump (represented by INF price).
// We start with 5 fuel. Edge 0->1 consumes 5 fuel. We arrive at 1 with 0 fuel.
// If we refuel at 1, we fail because there's no pump. So we must have refueled
// at 0 (price 2.0) to reach 2 (which has cheap price 1.0).
// Let's verify that the algorithm never refuels at 1, and only refuels at 0 and/or 2.
static bool test_no_pump_node() {
    Graph g(3, true);
    g.add_node(0, "A", 2.0);
    g.add_node(1, "B", 1e9); // Infinity price -> no pump
    g.add_node(2, "C", 1.0);

    g.add_edge(0, 1, 10.0, 1.0, 5.0, 0.0);
    g.add_edge(1, 2, 10.0, 1.0, 5.0, 0.0);

    OptimizerSettings settings;
    settings.initial_fuel = 5.0;
    settings.tank_capacity = 10.0;
    settings.fuel_step = 1.0;

    DijkstraOptimizer optimizer(g, settings);
    auto paths = optimizer.find_paths(0, 2);

    ASSERT_TRUE(!paths.empty());
    auto summaries = TradeoffAnalyzer::analyze(paths);
    ASSERT_TRUE(!summaries.empty());

    // In all paths, verify that no fuel purchase happened at Node 1 (ID 1)
    for (const auto& s : summaries) {
        for (const auto& fp : s.fuel_purchases) {
            ASSERT_TRUE(fp.first != 1);
        }
    }

    return true;
}

#include "input_validator.h"

// Test 7: Verify that Validator class successfully catches malformed nodes,
// negative fuel prices, and impassable edge fuel requirements.
static bool test_input_validation() {
    // 1. Negative price test
    Graph g1(2, true);
    g1.add_node(0, "A", -5.0); // Invalid negative price
    g1.add_node(1, "B", 1.5);
    g1.add_edge(0, 1, 10.0, 1.0, 2.0, 0.0);
    
    bool caught_neg_price = false;
    try {
        InputValidator::validate_network(g1);
    } catch (const std::invalid_argument&) {
        caught_neg_price = true;
    }
    ASSERT_TRUE(caught_neg_price);

    // 2. Impassable edge test (fuel consumption > tank capacity)
    Graph g2(2, true);
    g2.add_node(0, "A", 1.5);
    g2.add_node(1, "B", 1.5);
    g2.add_edge(0, 1, 10.0, 1.0, 15.0, 0.0); // Consumes 15.0 fuel

    bool caught_impassable = false;
    try {
        // Tank capacity is 10.0, which is less than the edge fuel requirement (15.0)
        InputValidator::validate_query(g2, 0, 1, 10.0, 5.0);
    } catch (const std::invalid_argument&) {
        caught_impassable = true;
    }
    ASSERT_TRUE(caught_impassable);

    // 3. Invalid initial fuel level
    bool caught_fuel_bounds = false;
    try {
        InputValidator::validate_query(g2, 0, 1, 10.0, -1.0); // Negative initial fuel
    } catch (const std::invalid_argument&) {
        caught_fuel_bounds = true;
    }
    ASSERT_TRUE(caught_fuel_bounds);

    return true;
}

bool run_all_tests() {
    std::cout << "[TEST] Running unit-validation tests..." << std::endl;

    bool all_passed = true;

    auto run_test = [&](const char* name, bool (*test_func)()) {
        std::cout << "  Running: " << name << " ... ";
        if (test_func()) {
            std::cout << "PASS" << std::endl;
        } else {
            std::cout << "FAIL" << std::endl;
            all_passed = false;
        }
    };

    run_test("test_no_refuel_necessary", test_no_refuel_necessary);
    run_test("test_refuel_obligatory", test_refuel_obligatory);
    run_test("test_price_aware_refueling", test_price_aware_refueling);
    run_test("test_disconnected_graph", test_disconnected_graph);
    run_test("test_insufficient_tank_capacity", test_insufficient_tank_capacity);
    run_test("test_no_pump_node", test_no_pump_node);
    run_test("test_input_validation", test_input_validation);

    if (all_passed) {
        std::cout << "[TEST] All tests PASSED successfully!\n" << std::endl;
    } else {
        std::cout << "[TEST] Some tests FAILED.\n" << std::endl;
    }

    return all_passed;
}

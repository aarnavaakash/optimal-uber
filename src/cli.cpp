#include "cli.h"
#include "dijkstra.h"
#include "tradeoff_analyzer.h"
#include "json_exporter.h"
#include "input_validator.h"
#include "tests.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

// ANSI Escape Codes for CLI Aesthetics
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

InteractiveCommandLineInterface::InteractiveCommandLineInterface() {
    graph = Graph::generate_sample_graph(42); // Default seed
}

double InteractiveCommandLineInterface::prompt_double(const std::string& prompt, double min_val, double max_val, double default_val) {
    while (true) {
        std::cout << prompt << " [" << default_val << "]: ";
        std::string input;
        std::getline(std::cin, input);
        if (input.empty()) {
            return default_val;
        }
        std::stringstream ss(input);
        double val;
        if (ss >> val && val >= min_val && val <= max_val) {
            return val;
        }
        std::cout << RED << "Invalid input. Please enter a double between " << min_val << " and " << max_val << "." << RESET << std::endl;
    }
}

int InteractiveCommandLineInterface::prompt_int(const std::string& prompt, int min_val, int max_val, int default_val) {
    while (true) {
        std::cout << prompt << " [" << default_val << "]: ";
        std::string input;
        std::getline(std::cin, input);
        if (input.empty()) {
            return default_val;
        }
        std::stringstream ss(input);
        int val;
        if (ss >> val && val >= min_val && val <= max_val) {
            return val;
        }
        std::cout << RED << "Invalid input. Please enter an integer between " << min_val << " and " << max_val << "." << RESET << std::endl;
    }
}

void InteractiveCommandLineInterface::print_banner() const {
    std::cout << CYAN << BOLD;
    std::cout << "========================================================================\n";
    std::cout << "         UBER FUEL-AWARE MULTI-OBJECTIVE ROUTE OPTIMIZATION ENGINE      \n";
    std::cout << "========================================================================\n";
    std::cout << RESET;
}

void InteractiveCommandLineInterface::print_graph_info() const {
    std::cout << "\n" << BOLD << BLUE << "--- Active Graph Topology ---" << RESET << std::endl;
    std::cout << "Total Cities (Nodes): " << graph.get_num_nodes() << std::endl;
    int total_edges = 0;
    for (int i = 0; i < graph.get_num_nodes(); ++i) {
        total_edges += static_cast<int>(graph.get_neighbors(i).size());
    }
    std::cout << "Total Connections (Edges): " << (graph.is_directed() ? total_edges : total_edges / 2) << std::endl;
    
    std::cout << "\n" << BOLD << CYAN << "--------------------------------------------------------------------------------" << RESET << std::endl;
    std::cout << BOLD << std::left << std::setw(8) << "ID"
              << std::setw(18) << "City Name"
              << std::setw(22) << "Coordinates (X, Y)"
              << "Petrol Price ($/unit)" << RESET << std::endl;
    std::cout << BOLD << CYAN << "--------------------------------------------------------------------------------" << RESET << std::endl;
    
    const auto& nodes = graph.get_nodes();
    for (const auto& node : nodes) {
        std::stringstream ss_coord;
        ss_coord << "(" << std::fixed << std::setprecision(2) << node.x << ", " << node.y << ")";
        
        std::stringstream ss_price;
        if (node.fuel_price >= 1e9 - 1.0) {
            ss_price << "INF (No Pump)";
        } else {
            ss_price << "$" << std::fixed << std::setprecision(2) << node.fuel_price;
        }

        std::cout << std::left << std::setw(8) << node.id
                  << std::setw(18) << node.name
                  << std::setw(22) << ss_coord.str()
                  << ss_price.str() << std::endl;
    }
    std::cout << BOLD << CYAN << "--------------------------------------------------------------------------------" << RESET << std::endl;
    std::cout << std::endl;
}

void InteractiveCommandLineInterface::run_optimization() {
    print_banner();
    std::cout << YELLOW << BOLD << "\n--- Configure Vehicle & Query Parameters ---" << RESET << std::endl;
    
    int max_node_id = graph.get_num_nodes() - 1;

    // Use file query settings as defaults if available, otherwise use class variables
    int default_start = file_query_settings.has_query ? file_query_settings.source : current_start;
    int default_dest = file_query_settings.has_query ? file_query_settings.destination : current_dest;
    double default_cap = file_query_settings.has_query ? file_query_settings.tank_capacity : tank_capacity;
    double default_init = file_query_settings.has_query ? file_query_settings.initial_fuel : initial_fuel;

    // Ensure default values are within bounds
    if (default_start < 0 || default_start > max_node_id) default_start = 0;
    if (default_dest < 0 || default_dest > max_node_id) default_dest = max_node_id;
    if (default_cap <= 0.0) default_cap = 50.0;
    if (default_init < 0.0 || default_init > default_cap) default_init = default_cap / 2.0;

    std::string start_prompt = "Enter source node ID (0-" + std::to_string(max_node_id) + ")";
    std::string dest_prompt = "Enter destination node ID (0-" + std::to_string(max_node_id) + ")";
    
    current_start = prompt_int(start_prompt, 0, max_node_id, default_start);
    current_dest = prompt_int(dest_prompt, 0, max_node_id, default_dest);
    
    tank_capacity = prompt_double("Enter fuel tank capacity (max fuel units)", 1.0, 500.0, default_cap);
    initial_fuel = prompt_double("Enter initial fuel level (units in tank)", 0.0, tank_capacity, default_init);
    fuel_step = prompt_double("Enter fuel purchasing step size (units)", 0.1, 10.0, fuel_step);
    refuel_overhead = prompt_double("Enter time penalty for refueling stops (hours)", 0.0, 5.0, refuel_overhead);

    if (current_start == current_dest) {
        std::cout << RED << "Source and destination are the same! No travel required." << RESET << std::endl;
        return;
    }

    // Call InputValidator to validate inputs
    try {
        InputValidator::validate_query(graph, current_start, current_dest, tank_capacity, initial_fuel);
    } catch (const std::invalid_argument& ex) {
        std::cout << RED << BOLD << "\n[Input Error] " << ex.what() << RESET << std::endl;
        return;
    }

    std::cout << GREEN << "\n🚀 Running multi-objective state-space optimization..." << RESET << std::endl;

    OptimizerSettings settings;
    settings.initial_fuel = initial_fuel;
    settings.tank_capacity = tank_capacity;
    settings.fuel_step = fuel_step;
    settings.refuel_overhead_time = refuel_overhead;

    DijkstraOptimizer optimizer(graph, settings);
    optimizer.set_debug(false);

    auto start_time = std::chrono::high_resolution_clock::now();
    auto raw_paths = optimizer.find_paths(current_start, current_dest);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    if (raw_paths.empty()) {
        std::cout << RED << BOLD << "\n❌ [Error] No valid routes found! Vehicle ran out of fuel or graph is disconnected." << RESET << std::endl;
        std::cout << "Expanded Search States: " << optimizer.get_expanded_states_count() << std::endl;
        return;
    }

    // Process Pareto paths
    double alpha = 0.5; // Balanced weights
    double beta = 0.5;
    auto paths = TradeoffAnalyzer::analyze(raw_paths, alpha, beta);

    std::cout << GREEN << BOLD << "✔ Optimization finished successfully!" << RESET << std::endl;
    std::cout << "Search States Expanded : " << optimizer.get_expanded_states_count() << std::endl;
    std::cout << "Pareto-Optimal Paths   : " << paths.size() << std::endl;
    std::cout << "Execution Time         : " << std::fixed << std::setprecision(2) << duration.count() << " ms\n" << std::endl;

    // Identify Cheapest, Fastest, and Balanced paths
    int cheapest_idx = 0;
    int fastest_idx = 0;
    int balanced_idx = 0;

    double min_cost = std::numeric_limits<double>::max();
    double min_time = std::numeric_limits<double>::max();
    double min_score = std::numeric_limits<double>::max();

    for (size_t i = 0; i < paths.size(); ++i) {
        if (paths[i].cost < min_cost) {
            min_cost = paths[i].cost;
            cheapest_idx = static_cast<int>(i);
        }
        if (paths[i].time < min_time) {
            min_time = paths[i].time;
            fastest_idx = static_cast<int>(i);
        }
        if (paths[i].score < min_score) {
            min_score = paths[i].score;
            balanced_idx = static_cast<int>(i);
        }
    }

    // Display summary table
    std::cout << BOLD << CYAN << "========================================================================\n";
    std::cout << "                       OPTIMIZATION SUMMARY TABLE                       \n";
    std::cout << "========================================================================\n" << RESET;
    std::cout << std::left << std::setw(15) << "Path Type" 
              << std::right << std::setw(10) << "Cost ($)" 
              << std::right << std::setw(12) << "Time (hrs)" 
              << std::right << std::setw(12) << "Stops" 
              << "   Route Node Sequence\n";
    std::cout << "------------------------------------------------------------------------\n";

    auto print_row = [](const std::string& label, const PathSummary& p, const std::string& color) {
        std::cout << color << std::left << std::setw(15) << label << RESET
                  << std::right << std::setw(10) << std::fixed << std::setprecision(2) << p.cost
                  << std::right << std::setw(12) << std::fixed << std::setprecision(2) << p.time
                  << std::right << std::setw(12) << p.fuel_purchases.size() << "   ";
        for (size_t r = 0; r < p.route.size(); ++r) {
            std::cout << p.route[r] << (r + 1 < p.route.size() ? "->" : "");
        }
        std::cout << "\n";
    };

    print_row("1. Cheapest", paths[cheapest_idx], GREEN);
    print_row("2. Fastest", paths[fastest_idx], BLUE);
    print_row("3. Balanced", paths[balanced_idx], YELLOW);
    std::cout << "========================================================================\n\n";

    // Ask to show all Pareto paths
    std::cout << "Show details of all " << paths.size() << " Pareto-optimal paths? (y/n) [n]: ";
    std::string ans;
    std::getline(std::cin, ans);
    if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
        std::cout << "\n" << BOLD << CYAN << "--- Detailed Pareto Frontiers ---" << RESET << std::endl;
        for (const auto& p : paths) {
            bool is_special = (p.index - 1 == cheapest_idx || p.index - 1 == fastest_idx || p.index - 1 == balanced_idx);
            
            std::cout << BOLD << (is_special ? YELLOW : WHITE) << "Path #" << p.index << RESET;
            if (p.index - 1 == cheapest_idx) std::cout << GREEN << " [CHEAPEST]" << RESET;
            if (p.index - 1 == fastest_idx) std::cout << BLUE << " [FASTEST]" << RESET;
            if (p.index - 1 == balanced_idx) std::cout << YELLOW << " [BALANCED]" << RESET;
            std::cout << "\n";
            
            std::cout << "  - Total Travel Cost: $" << std::fixed << std::setprecision(2) << p.cost << std::endl;
            std::cout << "  - Total Travel Time: " << std::fixed << std::setprecision(2) << p.time << " hours" << std::endl;
            std::cout << "  - Route sequence   : ";
            for (size_t r = 0; r < p.route.size(); ++r) {
                std::cout << p.route[r] << (r + 1 < p.route.size() ? " -> " : "");
            }
            std::cout << std::endl;

            if (p.fuel_purchases.empty()) {
                std::cout << "  - Fuel Purchases   : None (travels on initial fuel)" << std::endl;
            } else {
                std::cout << "  - Fuel Purchases   : ";
                for (size_t f = 0; f < p.fuel_purchases.size(); ++f) {
                    int nid = p.fuel_purchases[f].first;
                    double amt = p.fuel_purchases[f].second;
                    std::cout << "Buy " << std::fixed << std::setprecision(1) << amt << " units at Node_" << nid 
                              << " ($" << std::fixed << std::setprecision(2) << graph.get_fuel_price(nid) << "/u)"
                              << (f + 1 < p.fuel_purchases.size() ? ", " : "");
                }
                std::cout << std::endl;
            }
            std::cout << "--------------------------------------------------------" << std::endl;
        }
    }

    // Ask to export JSON
    std::cout << "\nExport graph and optimal routes to JSON? (y/n) [y]: ";
    std::string json_ans;
    std::getline(std::cin, json_ans);
    if (json_ans.empty() || json_ans[0] == 'y' || json_ans[0] == 'Y') {
        std::cout << "Enter export filename [route_data.json]: ";
        std::string filename;
        std::getline(std::cin, filename);
        if (filename.empty()) {
            filename = "route_data.json";
        }
        if (JSONExporter::export_data(filename, graph, paths, current_start, current_dest)) {
            std::cout << GREEN << "Successfully exported visualization-ready data to " << filename << RESET << std::endl;
        } else {
            std::cout << RED << "Failed to export data to " << filename << RESET << std::endl;
        }
    }
}

void InteractiveCommandLineInterface::run() {
    while (true) {
        print_banner();
        std::cout << BOLD << "\nMain Menu:" << RESET << std::endl;
        std::cout << "1. Set routing parameters & run optimization" << std::endl;
        std::cout << "2. View active graph network topology" << std::endl;
        std::cout << "3. Load network from config file (e.g. input.txt)" << std::endl;
        std::cout << "4. Regenerate city graph (with a new random seed)" << std::endl;
        std::cout << "5. Run system self-validation tests (unit tests)" << std::endl;
        std::cout << "6. Exit system" << std::endl;
        
        int choice = prompt_int("\nSelect menu option (1-6)", 1, 6, 1);
        
        if (choice == 1) {
            run_optimization();
        } else if (choice == 2) {
            print_graph_info();
        } else if (choice == 3) {
            std::cout << "Enter filepath [input.txt]: ";
            std::string filepath;
            std::getline(std::cin, filepath);
            if (filepath.empty()) {
                filepath = "input.txt";
            }
            Graph new_graph;
            FileQuerySettings new_settings;
            try {
                if (Graph::load_from_file(filepath, new_graph, new_settings)) {
                    graph = new_graph;
                    file_query_settings = new_settings;
                    current_start = 0;
                    current_dest = graph.get_num_nodes() - 1;
                    std::cout << GREEN << "Successfully loaded network with " << graph.get_num_nodes() << " nodes from " << filepath << "!" << RESET << std::endl;
                    if (file_query_settings.has_query) {
                        std::cout << YELLOW << "Query configuration found in file:" << RESET << std::endl;
                        std::cout << "  - Source: " << file_query_settings.source << std::endl;
                        std::cout << "  - Destination: " << file_query_settings.destination << std::endl;
                        std::cout << "  - Tank Capacity: " << file_query_settings.tank_capacity << std::endl;
                        std::cout << "  - Initial Fuel: " << file_query_settings.initial_fuel << std::endl;
                    }
                } else {
                    std::cout << RED << "Failed to load network from " << filepath << "." << RESET << std::endl;
                }
            } catch (const std::invalid_argument& ex) {
                std::cout << RED << BOLD << "\n[Validation Error] " << ex.what() << RESET << std::endl;
            }
        } else if (choice == 4) {
            int seed = prompt_int("Enter new random seed (integer)", 1, 1000000, 42);
            graph = Graph::generate_sample_graph(seed);
            current_start = 0;
            current_dest = graph.get_num_nodes() - 1;
            std::cout << GREEN << "New sample 50-node graph generated with seed " << seed << "!" << RESET << std::endl;
        } else if (choice == 5) {
            run_all_tests();
        } else if (choice == 6) {
            std::cout << GREEN << "\nExiting Uber Fuel-Aware Planner. Have a safe journey!" << RESET << std::endl;
            break;
        }
        
        std::cout << "\nPress Enter to return to main menu...";
        std::string dummy;
        std::getline(std::cin, dummy);
    }
}

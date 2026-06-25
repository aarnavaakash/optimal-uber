#ifndef CLI_H
#define CLI_H

#include "graph.h"

/**
 * @brief InteractiveCommandLineInterface manages the terminal UI and user input flows.
 */
class InteractiveCommandLineInterface {
private:
    Graph graph;
    int current_start = 0;
    int current_dest = 49;
    double tank_capacity = 50.0;
    double initial_fuel = 10.0;
    double fuel_step = 1.0;
    double refuel_overhead = 0.05; // 0.05 hours (~3 mins) default overhead
    FileQuerySettings file_query_settings;

    /**
     * @brief Prompts user for a float with validation.
     */
    double prompt_double(const std::string& prompt, double min_val, double max_val, double default_val);

    /**
     * @brief Prompts user for an integer with validation.
     */
    int prompt_int(const std::string& prompt, int min_val, int max_val, int default_val);

public:
    InteractiveCommandLineInterface();

    /**
     * @brief Launches the main interactive menu loop.
     */
    void run();

    /**
     * @brief Displays project banner and styling.
     */
    void print_banner() const;

    /**
     * @brief Executes the Dijkstra optimization and formats/outputs results.
     */
    void run_optimization();

    /**
     * @brief Prints graph nodes and connectivity summaries.
     */
    void print_graph_info() const;
};

#endif // CLI_H

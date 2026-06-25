#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>

/**
 * @brief Edge represents a connection between two cities.
 */
struct Edge {
    int to;                   // Target node ID
    double distance;          // Distance in kilometers (or units)
    double time;              // Travel time in hours (or units)
    double fuel_consumption;  // Fuel consumed to traverse the edge
    double travel_cost;       // Monetary cost of travel (e.g. tolls)
};

/**
 * @brief Node represents a city with a gas station.
 */
struct Node {
    int id;                   // Unique ID of the node
    std::string name;         // Display name of the city
    double fuel_price;        // Price of fuel per unit at this city
    double x;                 // Coordinate X for mapping/visualization
    double y;                 // Coordinate Y for mapping/visualization
};

/**
 * @brief Graph class representing the city network.
 */
struct FileQuerySettings {
    int source = 0;
    int destination = 0;
    double tank_capacity = 50.0;
    double initial_fuel = 10.0; // Defaults to tank_capacity if not provided
    bool has_query = false;
};

class Graph {
private:
    std::vector<Node> nodes;
    std::vector<std::vector<Edge>> adj;
    bool directed;

public:
    Graph(int num_nodes = 0, bool is_directed = false);

    void add_node(int id, const std::string& name, double fuel_price, double x = 0.0, double y = 0.0);
    void add_edge(int from, int to, double distance, double time, double fuel_consumption, double travel_cost = 0.0);

    const std::vector<Node>& get_nodes() const;
    const std::vector<Edge>& get_neighbors(int node_id) const;
    double get_fuel_price(int node_id) const;
    int get_num_nodes() const;
    bool is_directed() const;

    /**
     * @brief Clears the graph.
     */
    void clear();

    /**
     * @brief Generates a random geometric connected 50-node graph.
     * @param seed Seed for random generation to ensure reproducibility.
     */
    static Graph generate_sample_graph(int seed = 42);

    /**
     * @param filepath Path to the input file.
     * @param out_graph Reference to Graph object that will be populated.
     * @param out_settings Reference to FileQuerySettings structure to populate.
     * @return True if file was parsed and loaded successfully.
     */
    static bool load_from_file(const std::string& filepath, Graph& out_graph, FileQuerySettings& out_settings);
};

#endif // GRAPH_H

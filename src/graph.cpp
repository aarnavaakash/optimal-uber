#include "graph.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <stdexcept>

Graph::Graph(int num_nodes, bool is_directed) : directed(is_directed) {
    if (num_nodes > 0) {
        nodes.reserve(num_nodes);
        adj.resize(num_nodes);
    }
}

void Graph::add_node(int id, const std::string& name, double fuel_price, double x, double y) {
    if (id < 0) {
        throw std::invalid_argument("Node ID cannot be negative.");
    }
    if (id >= static_cast<int>(adj.size())) {
        adj.resize(id + 1);
    }
    
    // Check if node already exists
    for (auto& node : nodes) {
        if (node.id == id) {
            node.name = name;
            node.fuel_price = fuel_price;
            node.x = x;
            node.y = y;
            return;
        }
    }
    nodes.push_back({id, name, fuel_price, x, y});
}

void Graph::add_edge(int from, int to, double distance, double time, double fuel_consumption, double travel_cost) {
    int max_id = std::max(from, to);
    if (max_id >= static_cast<int>(adj.size())) {
        adj.resize(max_id + 1);
    }

    Edge edge_to = {to, distance, time, fuel_consumption, travel_cost};
    adj[from].push_back(edge_to);

    if (!directed) {
        Edge edge_from = {from, distance, time, fuel_consumption, travel_cost};
        adj[to].push_back(edge_from);
    }
}

const std::vector<Node>& Graph::get_nodes() const {
    return nodes;
}

const std::vector<Edge>& Graph::get_neighbors(int node_id) const {
    if (node_id < 0 || node_id >= static_cast<int>(adj.size())) {
        static const std::vector<Edge> empty;
        return empty;
    }
    return adj[node_id];
}

double Graph::get_fuel_price(int node_id) const {
    for (const auto& node : nodes) {
        if (node.id == node_id) {
            return node.fuel_price;
        }
    }
    return 1.5; // Default fallback
}

int Graph::get_num_nodes() const {
    return static_cast<int>(nodes.size());
}

bool Graph::is_directed() const {
    return directed;
}

void Graph::clear() {
    nodes.clear();
    adj.clear();
}

Graph Graph::generate_sample_graph(int seed) {
    const int N = 50;
    Graph g(N, false); // Undirected graph for realistic roadmap

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> coord_dist(10.0, 90.0);
    std::uniform_real_distribution<double> price_dist(1.10, 2.30);
    std::uniform_real_distribution<double> speed_dist(50.0, 95.0);        // km/h
    std::uniform_real_distribution<double> consumption_dist(0.07, 0.13); // fuel units per km
    std::uniform_real_distribution<double> toll_prob(0.0, 1.0);
    std::uniform_real_distribution<double> toll_val(1.5, 6.0);

    // 1. Generate node coordinates and fuel prices
    struct Coord { double x, y; };
    std::vector<Coord> coords(N);
    for (int i = 0; i < N; ++i) {
        coords[i] = {coord_dist(rng), coord_dist(rng)};
        double price = price_dist(rng);
        g.add_node(i, "City_" + std::to_string(i), price, coords[i].x, coords[i].y);
    }

    // Helper lambda to calculate Euclidean distance
    auto get_dist = [&](int i, int j) {
        double dx = coords[i].x - coords[j].x;
        double dy = coords[i].y - coords[j].y;
        return std::sqrt(dx*dx + dy*dy);
    };

    // 2. Guarantee Connectivity (Spanning Tree using nearest connected neighbors)
    // We connect each node i > 0 to the closest node in [0, i-1]
    for (int i = 1; i < N; ++i) {
        int best_j = -1;
        double min_d = std::numeric_limits<double>::max();
        for (int j = 0; j < i; ++j) {
            double d = get_dist(i, j);
            if (d < min_d) {
                min_d = d;
                best_j = j;
            }
        }
        if (best_j != -1) {
            double speed = speed_dist(rng);
            double time = min_d / speed;
            double cons_rate = consumption_dist(rng);
            double fuel = min_d * cons_rate;
            double cost = 0.0;
            if (toll_prob(rng) < 0.20) {
                cost = toll_val(rng);
            }
            g.add_edge(i, best_j, min_d, time, fuel, cost);
        }
    }

    // 3. Add extra edges to create cycles and alternative paths (nearby connections)
    for (int i = 0; i < N; ++i) {
        // Find distances to all other nodes
        std::vector<std::pair<int, double>> neighbors;
        for (int j = 0; j < N; ++j) {
            if (i == j) continue;
            neighbors.push_back({j, get_dist(i, j)});
        }
        std::sort(neighbors.begin(), neighbors.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

        // Add edges to 3 closest neighbors with a high probability (e.g. 70%) if not already connected
        for (int k = 0; k < 3; ++k) {
            int target = neighbors[k].first;
            double d = neighbors[k].second;

            // Check if there is already an edge
            bool exists = false;
            for (const auto& edge : g.get_neighbors(i)) {
                if (edge.to == target) {
                    exists = true;
                    break;
                }
            }

            if (!exists && toll_prob(rng) < 0.70) {
                double speed = speed_dist(rng);
                double time = d / speed;
                double cons_rate = consumption_dist(rng);
                double fuel = d * cons_rate;
                double cost = 0.0;
                if (toll_prob(rng) < 0.15) {
                    cost = toll_val(rng);
                }
                g.add_edge(i, target, d, time, fuel, cost);
            }
        }
    }

    return g;
}

#include "input_validator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool Graph::load_from_file(const std::string& filepath, Graph& out_graph, FileQuerySettings& out_settings) {
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open input file " << filepath << std::endl;
        return false;
    }

    out_graph.clear();
    out_settings.has_query = false;

    std::string line;
    enum ParseMode { NONE, NODES_LEGACY, EDGES_LEGACY, EDGES, PRICES, QUERY } mode = NONE;

    while (std::getline(infile, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        // Handle comment lines and detect format modes
        if (line.rfind("//", 0) == 0 || line.rfind("#", 0) == 0) {
            std::string comment = line;
            std::transform(comment.begin(), comment.end(), comment.begin(), [](unsigned char c) {
                return std::toupper(c);
            });

            if (comment.find("NODE1") != std::string::npos || comment.find("FUELREQUIRED") != std::string::npos) {
                mode = EDGES;
            } else if (comment.find("NODEID") != std::string::npos || comment.find("PETROLPRICE") != std::string::npos) {
                mode = PRICES;
            } else if (comment.find("SOURCE") != std::string::npos || comment.find("TANKCAPACITY") != std::string::npos) {
                mode = QUERY;
            }
            continue;
        }

        // Support legacy section headers
        if (line == "[nodes]") {
            mode = NODES_LEGACY;
            continue;
        } else if (line == "[edges]") {
            mode = EDGES_LEGACY;
            continue;
        }

        std::stringstream ss(line);
        if (mode == NODES_LEGACY) {
            int id;
            std::string name;
            std::string price_str;
            double x = 0.0, y = 0.0;
            if (ss >> id >> name >> price_str) {
                double price = 1e9;
                if (price_str != "INF" && price_str != "inf" && price_str != "-1") {
                    try {
                        price = std::stod(price_str);
                        if (price < 0.0) price = 1e9;
                    } catch (...) {
                        price = 1e9;
                    }
                }
                ss >> x >> y;
                out_graph.add_node(id, name, price, x, y);
            }
        } else if (mode == EDGES_LEGACY) {
            int from, to;
            double distance, time, fuel;
            double cost = 0.0;
            if (ss >> from >> to >> distance >> time >> fuel) {
                ss >> cost;
                out_graph.add_node(from, "City_" + std::to_string(from), 1e9);
                out_graph.add_node(to, "City_" + std::to_string(to), 1e9);
                out_graph.add_edge(from, to, distance, time, fuel, cost);
            }
        } else if (mode == EDGES) {
            int from, to;
            double fuel, time;
            double toll = 0.0;
            if (ss >> from >> to >> fuel >> time) {
                ss >> toll;
                out_graph.add_node(from, "City_" + std::to_string(from), 1e9);
                out_graph.add_node(to, "City_" + std::to_string(to), 1e9);
                out_graph.add_edge(from, to, fuel, time, fuel, toll);
            }
        } else if (mode == PRICES) {
            int id;
            std::string price_str;
            if (ss >> id >> price_str) {
                double price = 1e9;
                if (price_str != "INF" && price_str != "inf" && price_str != "-1") {
                    try {
                        price = std::stod(price_str);
                        if (price < 0.0) price = 1e9;
                    } catch (...) {
                        price = 1e9;
                    }
                }
                out_graph.add_node(id, "City_" + std::to_string(id), price);
            }
        } else if (mode == QUERY) {
            int src, dst;
            double cap;
            double init_f = -1.0;
            if (ss >> src >> dst >> cap) {
                ss >> init_f;
                if (init_f < 0.0) {
                    init_f = cap; // Default to full tank
                }
                out_settings.source = src;
                out_settings.destination = dst;
                out_settings.tank_capacity = cap;
                out_settings.initial_fuel = init_f;
                out_settings.has_query = true;
            }
        }
    }

    if (out_graph.get_num_nodes() == 0) {
        throw std::invalid_argument("Error: Loaded graph has 0 nodes. File might be empty or in wrong format.");
    }

    // Invoke validation on network elements
    InputValidator::validate_network(out_graph);

    // Validate query properties if present
    if (out_settings.has_query) {
        InputValidator::validate_query(out_graph, out_settings.source, out_settings.destination, 
                                      out_settings.tank_capacity, out_settings.initial_fuel);
    }

    return true;
}


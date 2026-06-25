#include "json_exporter.h"
#include <fstream>
#include <iomanip>

bool JSONExporter::export_data(const std::string& filename,
                            const Graph& graph,
                            const std::vector<PathSummary>& paths,
                            int start_node,
                            int dest_node) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        return false;
    }

    // Set floating-point precision for coordinates, price, cost, etc.
    out << std::fixed << std::setprecision(4);

    out << "{\n";
    out << "  \"start_node\": " << start_node << ",\n";
    out << "  \"dest_node\": " << dest_node << ",\n";

    // 1. Export Graph
    out << "  \"graph\": {\n";
    out << "    \"nodes\": [\n";
    const auto& nodes = graph.get_nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        out << "      {\n";
        out << "        \"id\": " << nodes[i].id << ",\n";
        out << "        \"name\": \"" << nodes[i].name << "\",\n";
        out << "        \"fuel_price\": " << nodes[i].fuel_price << ",\n";
        out << "        \"x\": " << nodes[i].x << ",\n";
        out << "        \"y\": " << nodes[i].y << "\n";
        out << "      }" << (i + 1 < nodes.size() ? "," : "") << "\n";
    }
    out << "    ],\n";

    out << "    \"edges\": [\n";
    bool first_edge = true;
    for (const auto& node : nodes) {
        for (const auto& edge : graph.get_neighbors(node.id)) {
            // Note: In an undirected graph, this exports both directions of an edge.
            // This is ideal for visualization frameworks.
            if (!first_edge) {
                out << ",\n";
            }
            first_edge = false;
            out << "      {\n";
            out << "        \"from\": " << node.id << ",\n";
            out << "        \"to\": " << edge.to << ",\n";
            out << "        \"distance\": " << edge.distance << ",\n";
            out << "        \"time\": " << edge.time << ",\n";
            out << "        \"fuel_consumption\": " << edge.fuel_consumption << ",\n";
            out << "        \"travel_cost\": " << edge.travel_cost << "\n";
            out << "      }";
        }
    }
    out << "\n    ]\n";
    out << "  },\n";

    // 2. Export Paths
    out << "  \"paths\": [\n";
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];
        out << "    {\n";
        out << "      \"index\": " << p.index << ",\n";
        out << "      \"cost\": " << p.cost << ",\n";
        out << "      \"time\": " << p.time << ",\n";
        out << "      \"normalized_cost\": " << p.normalized_cost << ",\n";
        out << "      \"normalized_time\": " << p.normalized_time << ",\n";
        out << "      \"score\": " << p.score << ",\n";

        // Route sequence
        out << "      \"route\": [";
        for (size_t r = 0; r < p.route.size(); ++r) {
            out << p.route[r] << (r + 1 < p.route.size() ? ", " : "");
        }
        out << "],\n";

        // Fuel purchases
        out << "      \"fuel_purchases\": [\n";
        for (size_t fp = 0; fp < p.fuel_purchases.size(); ++fp) {
            out << "        {\n";
            out << "          \"node\": " << p.fuel_purchases[fp].first << ",\n";
            out << "          \"amount\": " << p.fuel_purchases[fp].second << "\n";
            out << "        }" << (fp + 1 < p.fuel_purchases.size() ? "," : "") << "\n";
        }
        out << "      ]\n";

        out << "    }" << (i + 1 < paths.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";

    return true;
}

#ifndef JSON_EXPORTER_H
#define JSON_EXPORTER_H

#include <string>
#include <vector>
#include "graph.h"
#include "tradeoff_analyzer.h"

/**
 * @brief JSONExporter formats the graph and path data into a valid JSON file.
 * Useful for debugging and loading into visualization tools (e.g. web maps).
 */
class JSONExporter {
public:
    /**
     * @brief Exports the graph topology and Pareto-optimal routes to a file.
     * @return True if write was successful.
     */
    static bool export_data(const std::string& filename,
                            const Graph& graph,
                            const std::vector<PathSummary>& paths,
                            int start_node,
                            int dest_node);
};

#endif // JSON_EXPORTER_H

#pragma once
#include <vector>
#include <string>

namespace edge_cloud {

struct Solution {
    std::vector<int> assignment;          // assignment[i] = j if task i on server j, or -1 if unassigned
    double total_profit = 0.0;            // Sum of profits of accepted tasks
    double total_energy = 0.0;            // Total energy if energy model is enabled
    int num_accepted = 0;                 // Number of successfully admitted tasks
    double runtime_ms = 0.0;              // Algorithm execution runtime in milliseconds
    std::vector<std::vector<double>> used_resources; // used_resources[j][rho]
    bool is_feasible = false;             // Feasibility status verified by checker
    std::string solver_name;              // Name of the solver generating this solution

    explicit Solution(size_t n_tasks = 0, size_t n_servers = 0, size_t n_res = 0)
        : assignment(n_tasks, -1),
          used_resources(n_servers, std::vector<double>(n_res, 0.0)) {}
};

} // namespace edge_cloud


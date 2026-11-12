#include "solver_baselines.hpp"
#include <algorithm>
#include <chrono>
#include <numeric>

namespace edge_cloud {

// =========================================================================
// Baseline 1: Greedy Value-Density Solver
// =========================================================================
Solution GreedyDensitySolver::solve(const TaskOffloadingInstance& instance) {
    auto start_time = std::chrono::high_resolution_clock::now();

    Solution sol(instance.num_tasks, instance.num_servers, instance.num_resources);
    sol.solver_name = get_name();

    struct Candidate {
        int task_id;
        int server_id;
        double density;
    };

    std::vector<Candidate> candidates;
    for (int i = 0; i < instance.num_tasks; ++i) {
        for (int j = 0; j < instance.num_servers; ++j) {
            if (!instance.is_deadline_feasible[i][j]) continue;

            // Normalized multi-resource cost: sum_rho (r_i^rho / K_j^rho)
            double norm_res_cost = 0.0;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                norm_res_cost += instance.tasks[i].demand[rho] / instance.servers[j].capacity[rho];
            }

            if (norm_res_cost > 0.0) {
                double density = instance.tasks[i].profit / norm_res_cost;
                candidates.push_back({i, j, density});
            }
        }
    }

    // Sort descending by value density
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.density > b.density;
    });

    std::vector<bool> task_assigned(instance.num_tasks, false);
    std::vector<std::vector<double>> rem_cap(instance.num_servers);
    for (int j = 0; j < instance.num_servers; ++j) {
        rem_cap[j] = instance.servers[j].capacity;
    }

    for (const auto& cand : candidates) {
        int i = cand.task_id;
        int j = cand.server_id;

        if (task_assigned[i]) continue;

        // Check capacity
        bool fits = true;
        for (int rho = 0; rho < instance.num_resources; ++rho) {
            if (instance.tasks[i].demand[rho] > rem_cap[j][rho] + 1e-7) {
                fits = false;
                break;
            }
        }

        if (fits) {
            sol.assignment[i] = j;
            task_assigned[i] = true;
            sol.total_profit += instance.tasks[i].profit;
            sol.num_accepted++;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                rem_cap[j][rho] -= instance.tasks[i].demand[rho];
                sol.used_resources[j][rho] += instance.tasks[i].demand[rho];
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    sol.runtime_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    sol.is_feasible = true;

    return sol;
}

// =========================================================================
// Baseline 2: First-Fit Decreasing (FFD) Solver
// =========================================================================
Solution FFDSolver::solve(const TaskOffloadingInstance& instance) {
    auto start_time = std::chrono::high_resolution_clock::now();

    Solution sol(instance.num_tasks, instance.num_servers, instance.num_resources);
    sol.solver_name = get_name();

    // Order tasks descending by profit
    std::vector<int> task_order(instance.num_tasks);
    std::iota(task_order.begin(), task_order.end(), 0);
    std::sort(task_order.begin(), task_order.end(), [&](int a, int b) {
        return instance.tasks[a].profit > instance.tasks[b].profit;
    });

    std::vector<std::vector<double>> rem_cap(instance.num_servers);
    for (int j = 0; j < instance.num_servers; ++j) {
        rem_cap[j] = instance.servers[j].capacity;
    }

    for (int i : task_order) {
        // First-fit across available servers (checking edge nodes first, then cloud)
        for (int j = 0; j < instance.num_servers; ++j) {
            if (!instance.is_deadline_feasible[i][j]) continue;

            bool fits = true;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                if (instance.tasks[i].demand[rho] > rem_cap[j][rho] + 1e-7) {
                    fits = false;
                    break;
                }
            }

            if (fits) {
                sol.assignment[i] = j;
                sol.total_profit += instance.tasks[i].profit;
                sol.num_accepted++;
                for (int rho = 0; rho < instance.num_resources; ++rho) {
                    rem_cap[j][rho] -= instance.tasks[i].demand[rho];
                    sol.used_resources[j][rho] += instance.tasks[i].demand[rho];
                }
                break; // Placed in first feasible server
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    sol.runtime_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    sol.is_feasible = true;

    return sol;
}

// =========================================================================
// Baseline 3: Cloud-Only Solver
// =========================================================================
Solution CloudOnlySolver::solve(const TaskOffloadingInstance& instance) {
    auto start_time = std::chrono::high_resolution_clock::now();

    Solution sol(instance.num_tasks, instance.num_servers, instance.num_resources);
    sol.solver_name = get_name();

    int cloud_id = instance.num_servers - 1; // Cloud server index

    struct CloudCandidate {
        int task_id;
        double density;
    };

    std::vector<CloudCandidate> candidates;
    for (int i = 0; i < instance.num_tasks; ++i) {
        if (!instance.is_deadline_feasible[i][cloud_id]) continue;

        double norm_res_cost = 0.0;
        for (int rho = 0; rho < instance.num_resources; ++rho) {
            norm_res_cost += instance.tasks[i].demand[rho] / instance.servers[cloud_id].capacity[rho];
        }

        if (norm_res_cost > 0.0) {
            candidates.push_back({i, instance.tasks[i].profit / norm_res_cost});
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const CloudCandidate& a, const CloudCandidate& b) {
        return a.density > b.density;
    });

    std::vector<double> cloud_rem_cap = instance.servers[cloud_id].capacity;

    for (const auto& cand : candidates) {
        int i = cand.task_id;
        bool fits = true;
        for (int rho = 0; rho < instance.num_resources; ++rho) {
            if (instance.tasks[i].demand[rho] > cloud_rem_cap[rho] + 1e-7) {
                fits = false;
                break;
            }
        }

        if (fits) {
            sol.assignment[i] = cloud_id;
            sol.total_profit += instance.tasks[i].profit;
            sol.num_accepted++;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                cloud_rem_cap[rho] -= instance.tasks[i].demand[rho];
                sol.used_resources[cloud_id][rho] += instance.tasks[i].demand[rho];
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    sol.runtime_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    sol.is_feasible = true;

    return sol;
}

} // namespace edge_cloud


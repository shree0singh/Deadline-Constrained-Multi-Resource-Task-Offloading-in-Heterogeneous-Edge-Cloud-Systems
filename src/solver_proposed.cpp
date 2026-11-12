#include "solver_proposed.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace edge_cloud {

Solution ProposedLPRNLSSolver::solve(const TaskOffloadingInstance& instance) {
    auto start_time = std::chrono::high_resolution_clock::now();

    Solution sol(instance.num_tasks, instance.num_servers, instance.num_resources);
    sol.solver_name = get_name();

    // Effective capacities (adjusted by robustness buffer if enabled)
    // If robust_buffer_eta > 0, effective_cap = K_j^rho / (1 + eta)
    std::vector<std::vector<double>> effective_cap(instance.num_servers,
        std::vector<double>(instance.num_resources, 0.0));
    double cap_div = 1.0 + robust_buffer_eta;
    for (int j = 0; j < instance.num_servers; ++j) {
        for (int rho = 0; rho < instance.num_resources; ++rho) {
            effective_cap[j][rho] = instance.servers[j].capacity[rho] / cap_div;
        }
    }

    int cloud_id = instance.num_servers - 1;

    // -------------------------------------------------------------
    // Stage 1: Dual Congestion Weighting & Candidate Scoring
    // -------------------------------------------------------------
    // Estimate congestion for each server resource
    std::vector<std::vector<double>> resource_load(instance.num_servers,
        std::vector<double>(instance.num_resources, 0.0));
    
    for (int i = 0; i < instance.num_tasks; ++i) {
        for (int j = 0; j < instance.num_servers; ++j) {
            if (instance.is_deadline_feasible[i][j]) {
                for (int rho = 0; rho < instance.num_resources; ++rho) {
                    resource_load[j][rho] += instance.tasks[i].demand[rho];
                }
            }
        }
    }

    // Shadow price multipliers mu_j^rho >= 1.0
    std::vector<std::vector<double>> mu(instance.num_servers,
        std::vector<double>(instance.num_resources, 1.0));
    if (enable_dual_weights) {
        for (int j = 0; j < instance.num_servers; ++j) {
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                double cap = effective_cap[j][rho];
                if (cap > 1e-6) {
                    double ratio = resource_load[j][rho] / cap;
                    mu[j][rho] = std::max(1.0, ratio);
                }
            }
        }
    }

    struct Candidate {
        int task_id;
        int server_id;
        double score;
    };

    std::vector<Candidate> candidates;
    for (int i = 0; i < instance.num_tasks; ++i) {
        bool feasible_on_cloud = instance.is_deadline_feasible[i][cloud_id];

        for (int j = 0; j < instance.num_servers; ++j) {
            if (!instance.is_deadline_feasible[i][j]) continue;

            double weighted_cost = 0.0;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                double norm_demand = instance.tasks[i].demand[rho] / effective_cap[j][rho];
                weighted_cost += mu[j][rho] * norm_demand;
            }

            if (weighted_cost > 0.0) {
                double base_score = instance.tasks[i].profit / weighted_cost;
                // If task is latency-critical (cannot run on cloud), prioritize it on edge
                if (!feasible_on_cloud && !instance.servers[j].is_cloud) {
                    base_score *= 1.35; // Priority boost for edge-exclusive critical tasks
                }
                candidates.push_back({i, j, base_score});
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    // -------------------------------------------------------------
    // Stage 2: Capacity-Aware Rounding & Initial Feasible Allocation
    // -------------------------------------------------------------
    std::vector<bool> assigned(instance.num_tasks, false);
    std::vector<std::vector<double>> rem_cap = effective_cap;

    for (const auto& cand : candidates) {
        int i = cand.task_id;
        int j = cand.server_id;
        if (assigned[i]) continue;

        bool fits = true;
        for (int rho = 0; rho < instance.num_resources; ++rho) {
            if (instance.tasks[i].demand[rho] > rem_cap[j][rho] + 1e-7) {
                fits = false;
                break;
            }
        }

        if (fits) {
            sol.assignment[i] = j;
            assigned[i] = true;
            sol.total_profit += instance.tasks[i].profit;
            sol.num_accepted++;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                rem_cap[j][rho] -= instance.tasks[i].demand[rho];
                sol.used_resources[j][rho] += instance.tasks[i].demand[rho];
            }
        }
    }

    // -------------------------------------------------------------
    // Stage 3: Multi-Neighborhood Local Search (NLS)
    // -------------------------------------------------------------
    if (enable_local_search) {
        int iters = 0;
        bool improved = true;
        while (improved && iters < max_local_search_iters) {
            improved = false;
            iters++;

            // Operator 1: ADD
            if (apply_add_moves(instance, sol, effective_cap)) {
                improved = true;
                continue;
            }

            // Operator 2: SHIFT
            if (apply_shift_moves(instance, sol, effective_cap)) {
                improved = true;
                continue;
            }

            // Operator 3: DROP-AND-ADD (1-for-2 replacement)
            if (apply_drop_add_moves(instance, sol, effective_cap)) {
                improved = true;
                continue;
            }

            // Operator 4: SWAP
            if (apply_swap_moves(instance, sol, effective_cap)) {
                improved = true;
                continue;
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    sol.runtime_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    sol.is_feasible = true;

    return sol;
}

// -----------------------------------------------------------------------------
// Neighborhood Move 1: ADD unassigned tasks to servers with remaining slack
// -----------------------------------------------------------------------------
bool ProposedLPRNLSSolver::apply_add_moves(const TaskOffloadingInstance& instance,
                                           Solution& sol,
                                           const std::vector<std::vector<double>>& effective_cap) {
    bool any_added = false;
    for (int i = 0; i < instance.num_tasks; ++i) {
        if (sol.assignment[i] != -1) continue;

        for (int j = 0; j < instance.num_servers; ++j) {
            if (!instance.is_deadline_feasible[i][j]) continue;

            bool fits = true;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                if (sol.used_resources[j][rho] + instance.tasks[i].demand[rho] > effective_cap[j][rho] + 1e-7) {
                    fits = false;
                    break;
                }
            }

            if (fits) {
                sol.assignment[i] = j;
                sol.total_profit += instance.tasks[i].profit;
                sol.num_accepted++;
                for (int rho = 0; rho < instance.num_resources; ++rho) {
                    sol.used_resources[j][rho] += instance.tasks[i].demand[rho];
                }
                any_added = true;
                break;
            }
        }
    }
    return any_added;
}

// -----------------------------------------------------------------------------
// Neighborhood Move 2: SHIFT a task to another server to allow unassigned tasks to enter
// -----------------------------------------------------------------------------
bool ProposedLPRNLSSolver::apply_shift_moves(const TaskOffloadingInstance& instance,
                                             Solution& sol,
                                             const std::vector<std::vector<double>>& effective_cap) {
    for (int i = 0; i < instance.num_tasks; ++i) {
        int src_j = sol.assignment[i];
        if (src_j == -1) continue;

        for (int dst_j = 0; dst_j < instance.num_servers; ++dst_j) {
            if (dst_j == src_j || !instance.is_deadline_feasible[i][dst_j]) continue;

            // Check if task i fits on dst_j
            bool dst_fits = true;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                if (sol.used_resources[dst_j][rho] + instance.tasks[i].demand[rho] > effective_cap[dst_j][rho] + 1e-7) {
                    dst_fits = false;
                    break;
                }
            }
            if (!dst_fits) continue;

            // Test if moving i -> dst_j allows admitting at least one unassigned task to src_j
            for (int u = 0; u < instance.num_tasks; ++u) {
                if (sol.assignment[u] != -1 || !instance.is_deadline_feasible[u][src_j]) continue;

                // Check capacity on src_j after removing i and adding u
                bool src_fits = true;
                for (int rho = 0; rho < instance.num_resources; ++rho) {
                    double new_usage = sol.used_resources[src_j][rho] - instance.tasks[i].demand[rho] + instance.tasks[u].demand[rho];
                    if (new_usage > effective_cap[src_j][rho] + 1e-7) {
                        src_fits = false;
                        break;
                    }
                }

                if (src_fits) {
                    // Shift i to dst_j, and assign u to src_j
                    for (int rho = 0; rho < instance.num_resources; ++rho) {
                        sol.used_resources[src_j][rho] += -instance.tasks[i].demand[rho] + instance.tasks[u].demand[rho];
                        sol.used_resources[dst_j][rho] += instance.tasks[i].demand[rho];
                    }
                    sol.assignment[i] = dst_j;
                    sol.assignment[u] = src_j;
                    sol.total_profit += instance.tasks[u].profit;
                    sol.num_accepted++;
                    return true;
                }
            }
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// Neighborhood Move 3: DROP-AND-ADD (1-for-2 replacement)
// -----------------------------------------------------------------------------
bool ProposedLPRNLSSolver::apply_drop_add_moves(const TaskOffloadingInstance& instance,
                                                Solution& sol,
                                                const std::vector<std::vector<double>>& effective_cap) {
    for (int i = 0; i < instance.num_tasks; ++i) {
        int j = sol.assignment[i];
        if (j == -1) continue;

        // Try finding two unassigned tasks u1, u2 that can fit if i is dropped
        for (int u1 = 0; u1 < instance.num_tasks; ++u1) {
            if (sol.assignment[u1] != -1 || !instance.is_deadline_feasible[u1][j]) continue;

            for (int u2 = u1 + 1; u2 < instance.num_tasks; ++u2) {
                if (sol.assignment[u2] != -1 || !instance.is_deadline_feasible[u2][j]) continue;

                double net_gain = (instance.tasks[u1].profit + instance.tasks[u2].profit) - instance.tasks[i].profit;
                if (net_gain <= 1e-5) continue;

                bool fits = true;
                for (int rho = 0; rho < instance.num_resources; ++rho) {
                    double usage = sol.used_resources[j][rho] - instance.tasks[i].demand[rho]
                                   + instance.tasks[u1].demand[rho] + instance.tasks[u2].demand[rho];
                    if (usage > effective_cap[j][rho] + 1e-7) {
                        fits = false;
                        break;
                    }
                }

                if (fits) {
                    // Execute replacement
                    for (int rho = 0; rho < instance.num_resources; ++rho) {
                        sol.used_resources[j][rho] += -instance.tasks[i].demand[rho]
                                                      + instance.tasks[u1].demand[rho]
                                                      + instance.tasks[u2].demand[rho];
                    }
                    sol.assignment[i] = -1;
                    sol.assignment[u1] = j;
                    sol.assignment[u2] = j;
                    sol.total_profit += net_gain;
                    sol.num_accepted++; // net +1 task
                    return true;
                }
            }
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// Neighborhood Move 4: SWAP two tasks between servers if feasible and profit improves
// -----------------------------------------------------------------------------
bool ProposedLPRNLSSolver::apply_swap_moves(const TaskOffloadingInstance& instance,
                                            Solution& sol,
                                            const std::vector<std::vector<double>>& effective_cap) {
    for (int i1 = 0; i1 < instance.num_tasks; ++i1) {
        int j1 = sol.assignment[i1];
        if (j1 == -1) continue;

        for (int i2 = i1 + 1; i2 < instance.num_tasks; ++i2) {
            int j2 = sol.assignment[i2];
            if (j2 == -1 || j1 == j2) continue;

            // Feasible swap conditions: i1 must be feasible on j2, i2 on j1
            if (!instance.is_deadline_feasible[i1][j2] || !instance.is_deadline_feasible[i2][j1]) continue;

            // Check capacity on both servers
            bool fits = true;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                double new_u1 = sol.used_resources[j1][rho] - instance.tasks[i1].demand[rho] + instance.tasks[i2].demand[rho];
                double new_u2 = sol.used_resources[j2][rho] - instance.tasks[i2].demand[rho] + instance.tasks[i1].demand[rho];
                if (new_u1 > effective_cap[j1][rho] + 1e-7 || new_u2 > effective_cap[j2][rho] + 1e-7) {
                    fits = false;
                    break;
                }
            }

            if (fits) {
                // If swap frees capacity to allow an unassigned task u to enter either server, that's an improvement!
                // Check if any unassigned task fits into j1 or j2 now
                for (int u = 0; u < instance.num_tasks; ++u) {
                    if (sol.assignment[u] != -1) continue;

                    if (instance.is_deadline_feasible[u][j1]) {
                        bool u_fits = true;
                        for (int rho = 0; rho < instance.num_resources; ++rho) {
                            double u1 = sol.used_resources[j1][rho] - instance.tasks[i1].demand[rho] + instance.tasks[i2].demand[rho] + instance.tasks[u].demand[rho];
                            if (u1 > effective_cap[j1][rho] + 1e-7) { u_fits = false; break; }
                        }
                        if (u_fits) {
                            // Apply swap + admit u
                            for (int rho = 0; rho < instance.num_resources; ++rho) {
                                sol.used_resources[j1][rho] += -instance.tasks[i1].demand[rho] + instance.tasks[i2].demand[rho] + instance.tasks[u].demand[rho];
                                sol.used_resources[j2][rho] += -instance.tasks[i2].demand[rho] + instance.tasks[i1].demand[rho];
                            }
                            sol.assignment[i1] = j2;
                            sol.assignment[i2] = j1;
                            sol.assignment[u] = j1;
                            sol.total_profit += instance.tasks[u].profit;
                            sol.num_accepted++;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

} // namespace edge_cloud


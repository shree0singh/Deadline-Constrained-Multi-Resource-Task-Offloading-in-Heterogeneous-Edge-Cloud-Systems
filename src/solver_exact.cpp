#include "solver_exact.hpp"
#include <algorithm>
#include <numeric>
#include <iostream>

namespace edge_cloud {

namespace {

struct SearchState {
    int task_idx;
    double current_profit;
    std::vector<int> current_assignment;
    std::vector<std::vector<double>> rem_cap;
};

} // namespace

double ExactBBSolver::compute_lp_relaxation_bound(const TaskOffloadingInstance& instance) {
    // Computes an admissible continuous upper bound for the multi-dimensional GAP:
    // For each server j, the fractional capacity constraint bounds total admitted profit.
    // We solve a surrogate relaxed knapsack for each server using normalized bottleneck weights.
    
    // We can compute the continuous bound by solving the LP relaxation or fractional relaxation:
    // For each task, profit density on server j is pi_i / sum_rho (r_i^rho / K_j^rho).
    // Summing over servers or taking individual task fractions:
    // Let's compute a greedy fractional assignment where tasks can be split across feasible servers:
    struct Candidate {
        int task_id;
        int server_id;
        double profit;
        double weight; // sum_rho (r_i^rho / K_j^rho)
        double density;
    };

    std::vector<Candidate> candidates;
    for (int i = 0; i < instance.num_tasks; ++i) {
        for (int j = 0; j < instance.num_servers; ++j) {
            if (!instance.is_deadline_feasible[i][j]) continue;
            double w = 0.0;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                w += instance.tasks[i].demand[rho] / instance.servers[j].capacity[rho];
            }
            if (w > 0.0) {
                candidates.push_back({i, j, instance.tasks[i].profit, w, instance.tasks[i].profit / w});
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.density > b.density;
    });

    std::vector<double> task_assigned_fraction(instance.num_tasks, 0.0);
    std::vector<std::vector<double>> server_rem_cap(instance.num_servers);
    for (int j = 0; j < instance.num_servers; ++j) {
        server_rem_cap[j] = instance.servers[j].capacity;
    }

    double lp_profit = 0.0;

    for (const auto& cand : candidates) {
        int i = cand.task_id;
        int j = cand.server_id;
        double rem_task_frac = 1.0 - task_assigned_fraction[i];
        if (rem_task_frac <= 1e-7) continue;

        // Check maximum fraction of task i that fits into server j's remaining capacities
        double max_fit_frac = rem_task_frac;
        for (int rho = 0; rho < instance.num_resources; ++rho) {
            double req = instance.tasks[i].demand[rho];
            if (req > 0.0) {
                double fit = server_rem_cap[j][rho] / req;
                if (fit < max_fit_frac) {
                    max_fit_frac = fit;
                }
            }
        }

        if (max_fit_frac > 1e-7) {
            lp_profit += cand.profit * max_fit_frac;
            task_assigned_fraction[i] += max_fit_frac;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                server_rem_cap[j][rho] -= instance.tasks[i].demand[rho] * max_fit_frac;
            }
        }
    }

    return lp_profit;
}

Solution ExactBBSolver::solve(const TaskOffloadingInstance& instance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    nodes_explored = 0;
    reached_timeout = false;

    Solution best_sol(instance.num_tasks, instance.num_servers, instance.num_resources);
    best_sol.solver_name = get_name();
    best_sol.total_profit = 0.0;

    // Order tasks by maximum potential profit density descending to find high-quality solutions fast
    std::vector<int> task_order(instance.num_tasks);
    std::iota(task_order.begin(), task_order.end(), 0);

    std::vector<double> max_density(instance.num_tasks, 0.0);
    for (int i = 0; i < instance.num_tasks; ++i) {
        double best_d = 0.0;
        for (int j = 0; j < instance.num_servers; ++j) {
            if (instance.is_deadline_feasible[i][j]) {
                double w = 0.0;
                for (int rho = 0; rho < instance.num_resources; ++rho) {
                    w += instance.tasks[i].demand[rho] / instance.servers[j].capacity[rho];
                }
                if (w > 0.0 && (instance.tasks[i].profit / w) > best_d) {
                    best_d = instance.tasks[i].profit / w;
                }
            }
        }
        max_density[i] = best_d;
    }

    std::sort(task_order.begin(), task_order.end(), [&](int a, int b) {
        return max_density[a] > max_density[b];
    });

    // Precompute suffix sum of remaining profits for fast upper bounding
    std::vector<double> suffix_profit(instance.num_tasks + 1, 0.0);
    for (int k = instance.num_tasks - 1; k >= 0; --k) {
        suffix_profit[k] = suffix_profit[k + 1] + instance.tasks[task_order[k]].profit;
    }

    // Remaining capacity tracker
    std::vector<std::vector<double>> rem_cap(instance.num_servers);
    for (int j = 0; j < instance.num_servers; ++j) {
        rem_cap[j] = instance.servers[j].capacity;
    }

    std::vector<int> current_assignment(instance.num_tasks, -1);

    // Recursive Branch & Bound lambda
    auto branch_and_bound = [&](auto& self, int depth, double current_profit) -> void {
        nodes_explored++;

        if ((nodes_explored & 4095) == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(now - start_time).count();
            if (elapsed >= timeout_seconds) {
                reached_timeout = true;
                return;
            }
        }

        // Base case: all tasks decided
        if (depth == instance.num_tasks) {
            if (current_profit > best_sol.total_profit) {
                best_sol.total_profit = current_profit;
                best_sol.assignment = current_assignment;
                best_sol.num_accepted = 0;
                for (int j = 0; j < instance.num_servers; ++j) {
                    for (int rho = 0; rho < instance.num_resources; ++rho) {
                        best_sol.used_resources[j][rho] = instance.servers[j].capacity[rho] - rem_cap[j][rho];
                    }
                }
                for (int i = 0; i < instance.num_tasks; ++i) {
                    if (best_sol.assignment[i] != -1) best_sol.num_accepted++;
                }
            }
            return;
        }

        // Bounding check 1: Suffix sum bound
        if (current_profit + suffix_profit[depth] <= best_sol.total_profit + 1e-7) {
            return; // Prune
        }

        int orig_task_id = task_order[depth];
        const auto& t = instance.tasks[orig_task_id];

        // Bounding check 2: Filter impossible remaining tasks
        double filtered_remaining_profit = 0.0;
        for (int k = depth; k < instance.num_tasks; ++k) {
            int rem_id = task_order[k];
            bool can_fit_somewhere = false;
            for (int j = 0; j < instance.num_servers; ++j) {
                if (!instance.is_deadline_feasible[rem_id][j]) continue;
                bool fits = true;
                for (int rho = 0; rho < instance.num_resources; ++rho) {
                    if (instance.tasks[rem_id].demand[rho] > rem_cap[j][rho] + 1e-7) {
                        fits = false;
                        break;
                    }
                }
                if (fits) {
                    can_fit_somewhere = true;
                    break;
                }
            }
            if (can_fit_somewhere) {
                filtered_remaining_profit += instance.tasks[rem_id].profit;
            }
        }

        if (current_profit + filtered_remaining_profit <= best_sol.total_profit + 1e-7) {
            return; // Prune
        }

        // Branching: Try feasible servers for task orig_task_id
        // Try edge servers first, then cloud (sorted by speed or density)
        std::vector<int> candidate_servers;
        for (int j = 0; j < instance.num_servers; ++j) {
            if (!instance.is_deadline_feasible[orig_task_id][j]) continue;
            bool fits = true;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                if (t.demand[rho] > rem_cap[j][rho] + 1e-7) {
                    fits = false;
                    break;
                }
            }
            if (fits) {
                candidate_servers.push_back(j);
            }
        }

        // Explore server assignments
        for (int j : candidate_servers) {
            if (reached_timeout) return;

            // Apply assignment
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                rem_cap[j][rho] -= t.demand[rho];
            }
            current_assignment[orig_task_id] = j;

            self(self, depth + 1, current_profit + t.profit);

            // Backtrack
            current_assignment[orig_task_id] = -1;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                rem_cap[j][rho] += t.demand[rho];
            }
        }

        // Branch: Do not assign task orig_task_id (reject)
        if (!reached_timeout) {
            current_assignment[orig_task_id] = -1;
            self(self, depth + 1, current_profit);
        }
    };

    branch_and_bound(branch_and_bound, 0, 0.0);

    auto end_time = std::chrono::high_resolution_clock::now();
    best_sol.runtime_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    best_sol.is_feasible = true;

    return best_sol;
}

} // namespace edge_cloud

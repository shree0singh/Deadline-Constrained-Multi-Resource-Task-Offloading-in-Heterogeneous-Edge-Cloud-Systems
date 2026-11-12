#pragma once
#include "solver_base.hpp"

namespace edge_cloud {

/**
 * @brief Baseline 1: Greedy Value-Density Heuristic
 * Ranks candidate (task, server) pairs by pi_i / sum_rho (r_i^rho / K_j^rho).
 */
class GreedyDensitySolver : public ISolver {
public:
    std::string get_name() const override {
        return "Greedy Value-Density";
    }

    Solution solve(const TaskOffloadingInstance& instance) override;
};

/**
 * @brief Baseline 2: First-Fit Decreasing (FFD) Heuristic
 * Sorts tasks in descending order of priority, placing each task into
 * the first feasible server that has enough remaining capacity.
 */
class FFDSolver : public ISolver {
public:
    std::string get_name() const override {
        return "First-Fit Decreasing (FFD)";
    }

    Solution solve(const TaskOffloadingInstance& instance) override;
};

/**
 * @brief Baseline 3: Cloud-Only Offloading
 * Restricts all task offloading strictly to the centralized cloud server,
 * reflecting traditional non-edge cloud offloading.
 */
class CloudOnlySolver : public ISolver {
public:
    std::string get_name() const override {
        return "Cloud-Only";
    }

    Solution solve(const TaskOffloadingInstance& instance) override;
};

} // namespace edge_cloud


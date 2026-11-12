#pragma once
#include "solver_base.hpp"
#include <chrono>

namespace edge_cloud {

class ExactBBSolver : public ISolver {
public:
    double timeout_seconds;
    int nodes_explored = 0;
    bool reached_timeout = false;
    double best_upper_bound = 0.0;

    explicit ExactBBSolver(double timeout_seconds = 15.0)
        : timeout_seconds(timeout_seconds) {}

    std::string get_name() const override {
        return "Exact Branch-and-Bound (B&B)";
    }

    Solution solve(const TaskOffloadingInstance& instance) override;

    /**
     * @brief Computes a continuous LP-relaxation upper bound for the instance.
     */
    static double compute_lp_relaxation_bound(const TaskOffloadingInstance& instance);
};

} // namespace edge_cloud


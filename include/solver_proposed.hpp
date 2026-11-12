#pragma once
#include "solver_base.hpp"

namespace edge_cloud {

/**
 * @brief Proposed Algorithm: LP-Relaxation Guided Rounding with Capacity-Aware Repair
 *                             and Multi-Neighborhood Local Search (LPR-NLS).
 */
class ProposedLPRNLSSolver : public ISolver {
public:
    bool enable_local_search;
    bool enable_dual_weights;
    int max_local_search_iters;
    double robust_buffer_eta;

    explicit ProposedLPRNLSSolver(bool enable_local_search = true,
                                  bool enable_dual_weights = true,
                                  int max_local_search_iters = 100,
                                  double robust_buffer_eta = 0.0)
        : enable_local_search(enable_local_search),
          enable_dual_weights(enable_dual_weights),
          max_local_search_iters(max_local_search_iters),
          robust_buffer_eta(robust_buffer_eta) {}

    std::string get_name() const override {
        if (!enable_local_search) {
            return "Proposed LPR (Without Local Search)";
        }
        return "Proposed LPR-NLS (Rounding + Local Search)";
    }

    Solution solve(const TaskOffloadingInstance& instance) override;

private:
    // Neighborhood Operators for Local Search
    bool apply_add_moves(const TaskOffloadingInstance& instance, Solution& sol,
                         const std::vector<std::vector<double>>& effective_cap);
    bool apply_shift_moves(const TaskOffloadingInstance& instance, Solution& sol,
                          const std::vector<std::vector<double>>& effective_cap);
    bool apply_swap_moves(const TaskOffloadingInstance& instance, Solution& sol,
                         const std::vector<std::vector<double>>& effective_cap);
    bool apply_drop_add_moves(const TaskOffloadingInstance& instance, Solution& sol,
                             const std::vector<std::vector<double>>& effective_cap);
};

} // namespace edge_cloud


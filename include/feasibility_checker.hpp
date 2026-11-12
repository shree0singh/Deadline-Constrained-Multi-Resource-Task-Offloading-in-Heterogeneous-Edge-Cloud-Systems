#pragma once
#include "instance.hpp"
#include "solution.hpp"
#include <string>

namespace edge_cloud {

class FeasibilityChecker {
public:
    /**
     * @brief Independently validates all constraints of a solution.
     * @param instance The problem instance
     * @param solution The solution to validate
     * @param error_message Output message describing the first detected violation (if any)
     * @param demand_uncertainty_eta Uncertainty factor eta (default 0.0)
     * @return true if strictly feasible, false otherwise.
     */
    static bool validate(const TaskOffloadingInstance& instance,
                         const Solution& solution,
                         std::string& error_message,
                         double demand_uncertainty_eta = 0.0) {
        if (solution.assignment.size() != static_cast<size_t>(instance.num_tasks)) {
            error_message = "Assignment vector size mismatch.";
            return false;
        }

        std::vector<std::vector<double>> server_usage(
            instance.num_servers, std::vector<double>(instance.num_resources, 0.0));
        double recomputed_profit = 0.0;
        int recomputed_accepted = 0;

        for (int i = 0; i < instance.num_tasks; ++i) {
            int j = solution.assignment[i];
            if (j == -1) continue; // Unassigned task

            if (j < 0 || j >= instance.num_servers) {
                error_message = "Task " + std::to_string(i) + " assigned to invalid server ID " + std::to_string(j);
                return false;
            }

            // Check deadline constraint (i, j) \in F
            if (!instance.is_deadline_feasible[i][j]) {
                error_message = "Deadline violation: Task " + std::to_string(i) +
                                " completion time " + std::to_string(instance.completion_time[i][j]) +
                                "s exceeds deadline " + std::to_string(instance.tasks[i].deadline) +
                                "s on server " + std::to_string(j);
                return false;
            }

            // Accumulate resource demands
            double mult = 1.0 + demand_uncertainty_eta;
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                server_usage[j][rho] += instance.tasks[i].demand[rho] * mult;
            }

            recomputed_profit += instance.tasks[i].profit;
            recomputed_accepted++;
        }

        // Check server capacity limits
        const double EPS = 1e-6;
        for (int j = 0; j < instance.num_servers; ++j) {
            for (int rho = 0; rho < instance.num_resources; ++rho) {
                if (server_usage[j][rho] > instance.servers[j].capacity[rho] + EPS) {
                    error_message = "Capacity violation: Server " + std::to_string(j) +
                                    " resource " + instance.resource_names[rho] +
                                    " used " + std::to_string(server_usage[j][rho]) +
                                    " exceeds capacity " + std::to_string(instance.servers[j].capacity[rho]);
                    return false;
                }
            }
        }

        // Verify profit consistency
        if (std::abs(recomputed_profit - solution.total_profit) > 1e-4) {
            error_message = "Profit mismatch: reported " + std::to_string(solution.total_profit) +
                            " but recomputed " + std::to_string(recomputed_profit);
            return false;
        }

        if (recomputed_accepted != solution.num_accepted) {
            error_message = "Accepted tasks count mismatch: reported " + std::to_string(solution.num_accepted) +
                            " vs recomputed " + std::to_string(recomputed_accepted);
            return false;
        }

        error_message = "Valid and feasible.";
        return true;
    }
};

} // namespace edge_cloud


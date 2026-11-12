#pragma once
#include "task.hpp"
#include <vector>
#include <string>
#include <random>

namespace edge_cloud {

class TaskOffloadingInstance {
public:
    int num_tasks;
    int num_servers;
    int num_resources;                    // e.g. 3: CPU, RAM, Bandwidth
    std::vector<Task> tasks;
    std::vector<Server> servers;

    // Network & Timing matrices: [task_i][server_j]
    std::vector<std::vector<double>> uplink_rate;       // b_{ij} (Mbps)
    std::vector<std::vector<double>> propagation_delay; // tau_{ij} (seconds)
    std::vector<std::vector<double>> completion_time;   // T_{ij} (seconds)
    std::vector<std::vector<bool>> is_deadline_feasible;// F_{ij}: true if T_{ij} <= delta_i

    // Energy matrix e_{ij} (Joules)
    std::vector<std::vector<double>> energy_cost;

    // Names of resource dimensions
    std::vector<std::string> resource_names;

    TaskOffloadingInstance()
        : num_tasks(0), num_servers(0), num_resources(0) {}

    /**
     * @brief Precomputes completion times T_{ij} and feasible set F
     */
    void precompute_timings() {
        completion_time.assign(num_tasks, std::vector<double>(num_servers, 0.0));
        is_deadline_feasible.assign(num_tasks, std::vector<bool>(num_servers, false));
        for (int i = 0; i < num_tasks; ++i) {
            for (int j = 0; j < num_servers; ++j) {
                double t_up = tasks[i].data_size / uplink_rate[i][j];
                double t_proc = tasks[i].computation / servers[j].speed;
                double t_total = t_up + propagation_delay[i][j] + t_proc;
                completion_time[i][j] = t_total;
                is_deadline_feasible[i][j] = (t_total <= tasks[i].deadline);
            }
        }
    }

    /**
     * @brief Generates a synthetic edge-cloud instance according to course specifications.
     * @param n Number of tasks (20 - 500)
     * @param m_edge Number of edge servers (3 - 10)
     * @param seed Random seed for reproducibility
     * @param deadline_slack Factor controlling tightness of deadlines
     */
    static TaskOffloadingInstance generate_synthetic(int n, int m_edge, unsigned int seed = 42,
                                                    double deadline_slack = 1.0);
};

} // namespace edge_cloud


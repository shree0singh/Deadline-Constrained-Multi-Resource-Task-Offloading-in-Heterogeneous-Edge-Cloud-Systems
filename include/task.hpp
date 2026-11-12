#pragma once
#include <vector>
#include <string>

namespace edge_cloud {

/**
 * @brief Represents an offloadable computation task from a mobile/IoT device.
 */
struct Task {
    int id;                           // Task ID (0-indexed)
    double profit;                    // Reward/utility earned if completed before deadline
    double deadline;                  // Latency deadline delta_i (seconds)
    double data_size;                 // Input data size D_i (Megabits)
    double computation;               // Workload W_i (Gigacycles)
    std::vector<double> demand;       // Resource demands r_i^rho (e.g., [CPU cores, RAM GB, BW Mbps])

    Task(int id, double profit, double deadline, double data_size, double computation,
         const std::vector<double>& demand)
        : id(id), profit(profit), deadline(deadline), data_size(data_size),
          computation(computation), demand(demand) {}
};

/**
 * @brief Represents a compute server (Edge server or Cloud data center).
 */
struct Server {
    int id;                           // Server ID (0-indexed)
    std::string name;                 // Descriptive name
    bool is_cloud;                    // True if cloud datacenter, false if edge
    double speed;                     // Processing speed s_j (Gigacycles / second)
    double base_latency;              // Average network propagation delay (seconds)
    std::vector<double> capacity;     // Resource capacities K_j^rho for each resource rho

    Server(int id, const std::string& name, bool is_cloud, double speed,
           double base_latency, const std::vector<double>& capacity)
        : id(id), name(name), is_cloud(is_cloud), speed(speed),
          base_latency(base_latency), capacity(capacity) {}
};

} // namespace edge_cloud


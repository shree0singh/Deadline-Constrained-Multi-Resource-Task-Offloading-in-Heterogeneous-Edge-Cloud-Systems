#include "instance.hpp"
#include <cmath>
#include <iostream>
#include <iomanip>

namespace edge_cloud {

TaskOffloadingInstance TaskOffloadingInstance::generate_synthetic(
    int n, int m_edge, unsigned int seed, double deadline_slack) {

    TaskOffloadingInstance inst;
    inst.num_tasks = n;
    inst.num_servers = m_edge + 1; // m_edge edge nodes + 1 cloud node
    inst.num_resources = 3;        // CPU, RAM, Bandwidth
    inst.resource_names = {"CPU (Cores)", "RAM (GB)", "Bandwidth (Mbps)"};

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist_unif(0.0, 1.0);

    // 1. Configure Servers
    inst.servers.reserve(inst.num_servers);

    // Edge servers: moderate compute capacity, very low latency
    for (int j = 0; j < m_edge; ++j) {
        std::string name = "Edge_" + std::to_string(j);
        double speed = 2.5 + dist_unif(rng) * 1.0; // 2.5 - 3.5 GHz
        double base_lat = 0.003 + dist_unif(rng) * 0.007; // 3 - 10 ms
        
        // Edge capacities
        double cpu_cap = 16.0 + dist_unif(rng) * 16.0;   // 16 - 32 cores
        double ram_cap = 32.0 + dist_unif(rng) * 32.0;   // 32 - 64 GB
        double bw_cap = 100.0 + dist_unif(rng) * 150.0;  // 100 - 250 Mbps
        
        inst.servers.emplace_back(j, name, false, speed, base_lat,
                                 std::vector<double>{cpu_cap, ram_cap, bw_cap});
    }

    // Cloud datacenter: index m_edge, massive capacity, higher latency
    int cloud_idx = m_edge;
    {
        std::string name = "Cloud_DC";
        double speed = 5.0 + dist_unif(rng) * 1.5; // 5.0 - 6.5 GHz
        double base_lat = 0.055 + dist_unif(rng) * 0.035; // 55 - 90 ms
        
        // Scale cloud capacity proportionally to total edge capacity to keep problem balanced
        double cloud_cpu = (16.0 * m_edge) * 1.5;
        double cloud_ram = (32.0 * m_edge) * 1.5;
        double cloud_bw  = (100.0 * m_edge) * 1.5;
        
        inst.servers.emplace_back(cloud_idx, name, true, speed, base_lat,
                                 std::vector<double>{cloud_cpu, cloud_ram, cloud_bw});
    }

    // 2. Configure Network Channels
    inst.uplink_rate.assign(n, std::vector<double>(inst.num_servers, 0.0));
    inst.propagation_delay.assign(n, std::vector<double>(inst.num_servers, 0.0));

    for (int i = 0; i < n; ++i) {
        // Uplink rate to edge servers (local wireless, Wi-Fi 6 / 5G microcell: 40 - 100 Mbps)
        for (int j = 0; j < m_edge; ++j) {
            inst.uplink_rate[i][j] = 40.0 + dist_unif(rng) * 60.0;
            inst.propagation_delay[i][j] = inst.servers[j].base_latency * (0.8 + dist_unif(rng) * 0.4);
        }
        // Uplink rate to cloud (WAN backhaul: 15 - 35 Mbps)
        inst.uplink_rate[i][cloud_idx] = 15.0 + dist_unif(rng) * 20.0;
        inst.propagation_delay[i][cloud_idx] = inst.servers[cloud_idx].base_latency * (0.8 + dist_unif(rng) * 0.4);
    }

    // 3. Configure Tasks
    inst.tasks.reserve(n);
    for (int i = 0; i < n; ++i) {
        double data_size = 2.0 + dist_unif(rng) * 20.0;     // 2 - 22 Mbits
        double computation = 0.5 + dist_unif(rng) * 7.5;   // 0.5 - 8.0 Gigacycles
        
        // Multi-resource demands
        double r_cpu = 1.0 + dist_unif(rng) * 3.0;          // 1 - 4 cores
        double r_ram = 2.0 + dist_unif(rng) * 6.0;          // 2 - 8 GB
        double r_bw  = 5.0 + dist_unif(rng) * 20.0;         // 5 - 25 Mbps
        std::vector<double> demand = {r_cpu, r_ram, r_bw};

        // Profit: value earned upon on-time completion
        double profit = 8.0 * computation + 1.5 * data_size + (5.0 + dist_unif(rng) * 15.0);

        // Calculate reference completion time on fastest edge and on cloud
        double min_edge_time = 1e9;
        for (int j = 0; j < m_edge; ++j) {
            double t = (data_size / inst.uplink_rate[i][j]) +
                       inst.propagation_delay[i][j] +
                       (computation / inst.servers[j].speed);
            if (t < min_edge_time) min_edge_time = t;
        }
        double cloud_time = (data_size / inst.uplink_rate[i][cloud_idx]) +
                            inst.propagation_delay[i][cloud_idx] +
                            (computation / inst.servers[cloud_idx].speed);

        // Partition tasks into latency-critical (tight deadline, infeasible on cloud)
        // and delay-tolerant tasks (feasible on both edge and cloud)
        double deadline = 0.0;
        if (dist_unif(rng) < 0.45) {
            // Latency-critical task: deadline lies between edge completion and cloud completion
            deadline = min_edge_time + (cloud_time - min_edge_time) * (0.2 + dist_unif(rng) * 0.6) * deadline_slack;
            if (deadline < min_edge_time * 1.05) deadline = min_edge_time * 1.05;
        } else {
            // Delay-tolerant task: deadline allows cloud or edge
            deadline = cloud_time * (1.1 + dist_unif(rng) * 0.8) * deadline_slack;
        }

        inst.tasks.emplace_back(i, profit, deadline, data_size, computation, demand);
    }

    // 4. Energy model (Transmission energy + Server processing energy)
    inst.energy_cost.assign(n, std::vector<double>(inst.num_servers, 0.0));
    double p_tx = 0.5; // Transmit power 0.5 Watts
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < inst.num_servers; ++j) {
            double t_tx = inst.tasks[i].data_size / inst.uplink_rate[i][j];
            double t_comp = inst.tasks[i].computation / inst.servers[j].speed;
            double p_server = inst.servers[j].is_cloud ? 20.0 : 8.0; // Server power draw per task
            inst.energy_cost[i][j] = p_tx * t_tx + p_server * t_comp;
        }
    }

    // Precompute completion times and deadline feasibility
    inst.precompute_timings();

    return inst;
}

} // namespace edge_cloud


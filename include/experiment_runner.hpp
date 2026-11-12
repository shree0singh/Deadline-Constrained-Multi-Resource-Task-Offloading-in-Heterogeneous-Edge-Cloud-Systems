#pragma once
#include "instance.hpp"
#include "solution.hpp"
#include "solver_base.hpp"
#include <vector>
#include <string>

namespace edge_cloud {

struct ExperimentResult {
    int num_tasks;
    int num_servers;
    int seed;
    std::string algorithm_name;
    double profit;
    int accepted_tasks;
    double acceptance_rate;
    double runtime_ms;
    double optimality_gap;        // Gap relative to exact OPT or LP bound (%)
    double cpu_util;              // Average CPU utilization across servers (%)
    double ram_util;              // Average RAM utilization across servers (%)
    double bw_util;               // Average BW utilization across servers (%)
    int edge_tasks;               // Tasks running on edge
    int cloud_tasks;              // Tasks running on cloud
    bool is_feasible;
};

class ExperimentRunner {
public:
    static void run_small_scale_benchmark(const std::string& output_csv, int num_seeds = 5);
    static void run_large_scale_benchmark(const std::string& output_csv, int num_seeds = 5);
    static void run_robustness_study(const std::string& output_csv, int num_seeds = 5);
    static void run_ablation_study(const std::string& output_csv, int num_seeds = 5);
};

} // namespace edge_cloud

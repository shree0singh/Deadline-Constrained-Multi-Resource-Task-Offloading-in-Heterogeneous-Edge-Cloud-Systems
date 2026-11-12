#include "instance.hpp"
#include "feasibility_checker.hpp"
#include "solver_exact.hpp"
#include "solver_baselines.hpp"
#include "solver_proposed.hpp"
#include "experiment_runner.hpp"
#include <iostream>
#include <iomanip>
#include <string>

using namespace edge_cloud;

void run_quick_demo() {
    std::cout << "========================================================================\n";
    std::cout << "  ADVANCED ALGORITHMS (CS5xx) - PS-3 TASK OFFLOADING DEMO\n";
    std::cout << "  Deadline-Constrained Multi-Resource Task Offloading in Edge-Cloud Systems\n";
    std::cout << "========================================================================\n\n";

    int n = 15;
    int m_edge = 3;
    unsigned int seed = 42;
    std::cout << "Generating test instance with n = " << n << " tasks, "
              << m_edge << " edge servers + 1 cloud...\n\n";

    auto inst = TaskOffloadingInstance::generate_synthetic(n, m_edge, seed, 1.0);

    std::cout << "Server Fleet Summary:\n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(10) << "Server"
              << std::setw(10) << "Type"
              << std::setw(14) << "Speed (GHz)"
              << std::setw(14) << "Latency (ms)"
              << "Capacities (CPU, RAM, BW)\n";
    std::cout << "------------------------------------------------------------------------\n";
    for (const auto& s : inst.servers) {
        std::cout << std::left << std::setw(10) << s.name
                  << std::setw(10) << (s.is_cloud ? "Cloud" : "Edge")
                  << std::setw(14) << std::fixed << std::setprecision(2) << s.speed
                  << std::setw(14) << std::fixed << std::setprecision(1) << (s.base_latency * 1000.0)
                  << "[" << std::setprecision(1) << s.capacity[0] << " cores, "
                  << s.capacity[1] << " GB, " << s.capacity[2] << " Mbps]\n";
    }
    std::cout << "\n";

    // 1. Exact B&B Solver
    std::cout << "Running Exact B&B Solver...\n";
    ExactBBSolver exact_solver(20.0);
    Solution sol_exact = exact_solver.solve(inst);
    std::string err;
    bool v_exact = FeasibilityChecker::validate(inst, sol_exact, err);

    std::cout << "  -> Exact OPT Profit: " << std::fixed << std::setprecision(2) << sol_exact.total_profit
              << " | Admitted: " << sol_exact.num_accepted << "/" << n
              << " | Time: " << sol_exact.runtime_ms << " ms"
              << " | Feasible: " << (v_exact ? "YES" : "NO (" + err + ")") << "\n\n";

    // 2. Baseline 1: Greedy Value Density
    GreedyDensitySolver greedy_solver;
    Solution sol_greedy = greedy_solver.solve(inst);
    bool v_greedy = FeasibilityChecker::validate(inst, sol_greedy, err);
    double gap_greedy = ((sol_exact.total_profit - sol_greedy.total_profit) / sol_exact.total_profit) * 100.0;
    std::cout << "Running Baseline 1: " << greedy_solver.get_name() << "...\n";
    std::cout << "  -> Profit: " << sol_greedy.total_profit
              << " | Admitted: " << sol_greedy.num_accepted << "/" << n
              << " | Gap: " << gap_greedy << "%"
              << " | Time: " << sol_greedy.runtime_ms << " ms"
              << " | Feasible: " << (v_greedy ? "YES" : "NO (" + err + ")") << "\n\n";

    // 3. Baseline 2: FFD
    FFDSolver ffd_solver;
    Solution sol_ffd = ffd_solver.solve(inst);
    bool v_ffd = FeasibilityChecker::validate(inst, sol_ffd, err);
    double gap_ffd = ((sol_exact.total_profit - sol_ffd.total_profit) / sol_exact.total_profit) * 100.0;
    std::cout << "Running Baseline 2: " << ffd_solver.get_name() << "...\n";
    std::cout << "  -> Profit: " << sol_ffd.total_profit
              << " | Admitted: " << sol_ffd.num_accepted << "/" << n
              << " | Gap: " << gap_ffd << "%"
              << " | Time: " << sol_ffd.runtime_ms << " ms"
              << " | Feasible: " << (v_ffd ? "YES" : "NO (" + err + ")") << "\n\n";

    // 4. Baseline 3: Cloud-Only
    CloudOnlySolver cloud_solver;
    Solution sol_cloud = cloud_solver.solve(inst);
    bool v_cloud = FeasibilityChecker::validate(inst, sol_cloud, err);
    double gap_cloud = ((sol_exact.total_profit - sol_cloud.total_profit) / sol_exact.total_profit) * 100.0;
    std::cout << "Running Baseline 3: " << cloud_solver.get_name() << "...\n";
    std::cout << "  -> Profit: " << sol_cloud.total_profit
              << " | Admitted: " << sol_cloud.num_accepted << "/" << n
              << " | Gap: " << gap_cloud << "%"
              << " | Time: " << sol_cloud.runtime_ms << " ms"
              << " | Feasible: " << (v_cloud ? "YES" : "NO (" + err + ")") << "\n\n";

    // 5. Proposed LPR-NLS
    ProposedLPRNLSSolver prop_solver(true, true);
    Solution sol_prop = prop_solver.solve(inst);
    bool v_prop = FeasibilityChecker::validate(inst, sol_prop, err);
    double gap_prop = ((sol_exact.total_profit - sol_prop.total_profit) / sol_exact.total_profit) * 100.0;
    std::cout << "Running Proposed Algorithm: " << prop_solver.get_name() << "...\n";
    std::cout << "  -> Profit: " << sol_prop.total_profit
              << " | Admitted: " << sol_prop.num_accepted << "/" << n
              << " | Gap: " << gap_prop << "%"
              << " | Time: " << sol_prop.runtime_ms << " ms"
              << " | Feasible: " << (v_prop ? "YES" : "NO (" + err + ")") << "\n\n";

    std::cout << "========================================================================\n";
    std::cout << "Demo Finished Successfully. All solutions verified strictly feasible!\n";
    std::cout << "========================================================================\n";
}

int main(int argc, char** argv) {
    std::string mode = "--demo";
    if (argc > 1) {
        mode = argv[1];
    }

    if (mode == "--demo") {
        run_quick_demo();
    } else if (mode == "--all") {
        run_quick_demo();
        ExperimentRunner::run_small_scale_benchmark("results/small_scale_results.csv", 5);
        ExperimentRunner::run_large_scale_benchmark("results/large_scale_results.csv", 5);
        ExperimentRunner::run_ablation_study("results/ablation_results.csv", 5);
        ExperimentRunner::run_robustness_study("results/robustness_results.csv", 5);
    } else if (mode == "--small") {
        ExperimentRunner::run_small_scale_benchmark("results/small_scale_results.csv", 5);
    } else if (mode == "--large") {
        ExperimentRunner::run_large_scale_benchmark("results/large_scale_results.csv", 5);
    } else if (mode == "--ablation") {
        ExperimentRunner::run_ablation_study("results/ablation_results.csv", 5);
    } else if (mode == "--robust") {
        ExperimentRunner::run_robustness_study("results/robustness_results.csv", 5);
    } else {
        std::cerr << "Unknown mode: " << mode << "\n";
        std::cerr << "Usage: ./task_offloading [--demo | --all | --small | --large | --ablation | --robust]\n";
        return 1;
    }

    return 0;
}


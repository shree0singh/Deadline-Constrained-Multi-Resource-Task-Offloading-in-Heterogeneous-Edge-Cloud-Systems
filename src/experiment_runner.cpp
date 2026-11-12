#include "experiment_runner.hpp"
#include "feasibility_checker.hpp"
#include "solver_exact.hpp"
#include "solver_baselines.hpp"
#include "solver_proposed.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>

namespace edge_cloud {

namespace {

void write_csv_header(std::ofstream& out) {
    out << "num_tasks,num_servers,seed,algorithm,profit,accepted_tasks,acceptance_rate,"
        << "runtime_ms,optimality_gap,cpu_util,ram_util,bw_util,edge_tasks,cloud_tasks,is_feasible\n";
}

void write_result_row(std::ofstream& out, const ExperimentResult& res) {
    out << res.num_tasks << ","
        << res.num_servers << ","
        << res.seed << ",\""
        << res.algorithm_name << "\","
        << std::fixed << std::setprecision(4)
        << res.profit << ","
        << res.accepted_tasks << ","
        << res.acceptance_rate << ","
        << res.runtime_ms << ","
        << res.optimality_gap << ","
        << res.cpu_util << ","
        << res.ram_util << ","
        << res.bw_util << ","
        << res.edge_tasks << ","
        << res.cloud_tasks << ","
        << (res.is_feasible ? "1" : "0") << "\n";
}

void compute_stats(const TaskOffloadingInstance& inst, const Solution& sol,
                   ExperimentResult& res, double reference_opt_or_bound) {
    res.profit = sol.total_profit;
    res.accepted_tasks = sol.num_accepted;
    res.acceptance_rate = (100.0 * sol.num_accepted) / inst.num_tasks;
    res.runtime_ms = sol.runtime_ms;

    if (reference_opt_or_bound > 1e-6) {
        res.optimality_gap = std::max(0.0, ((reference_opt_or_bound - sol.total_profit) / reference_opt_or_bound) * 100.0);
    } else {
        res.optimality_gap = 0.0;
    }

    double tot_cpu_used = 0, tot_cpu_cap = 0;
    double tot_ram_used = 0, tot_ram_cap = 0;
    double tot_bw_used = 0, tot_bw_cap = 0;

    int edge_count = 0;
    int cloud_count = 0;
    int cloud_idx = inst.num_servers - 1;

    for (int i = 0; i < inst.num_tasks; ++i) {
        int j = sol.assignment[i];
        if (j != -1) {
            if (j == cloud_idx) cloud_count++;
            else edge_count++;
        }
    }

    for (int j = 0; j < inst.num_servers; ++j) {
        tot_cpu_used += sol.used_resources[j][0];
        tot_cpu_cap  += inst.servers[j].capacity[0];
        tot_ram_used += sol.used_resources[j][1];
        tot_ram_cap  += inst.servers[j].capacity[1];
        tot_bw_used  += sol.used_resources[j][2];
        tot_bw_cap   += inst.servers[j].capacity[2];
    }

    res.cpu_util = (tot_cpu_cap > 0) ? (tot_cpu_used / tot_cpu_cap) * 100.0 : 0.0;
    res.ram_util = (tot_ram_cap > 0) ? (tot_ram_used / tot_ram_cap) * 100.0 : 0.0;
    res.bw_util  = (tot_bw_cap > 0) ? (tot_bw_used / tot_bw_cap) * 100.0 : 0.0;
    res.edge_tasks = edge_count;
    res.cloud_tasks = cloud_count;

    std::string err;
    res.is_feasible = FeasibilityChecker::validate(inst, sol, err);
    if (!res.is_feasible) {
        std::cerr << "[CRITICAL WARNING] Feasibility check failed: " << err << "\n";
    }
}

} // namespace

void ExperimentRunner::run_small_scale_benchmark(const std::string& output_csv, int num_seeds) {
    std::ofstream out(output_csv);
    write_csv_header(out);

    std::vector<int> task_sizes = {10, 15, 20, 25, 30};
    int m_edge = 3;

    std::cout << "\n=======================================================\n";
    std::cout << "Starting Small-Scale Benchmark (Exact OPT vs Baselines)\n";
    std::cout << "=======================================================\n";

    for (int n : task_sizes) {
        std::cout << "Running instances with N = " << n << " tasks...\n";
        for (int s = 0; s < num_seeds; ++s) {
            unsigned int seed = 1000 + s * 73;
            auto inst = TaskOffloadingInstance::generate_synthetic(n, m_edge, seed, 1.0);

            // 1. Exact B&B Solver
            ExactBBSolver exact_solver(25.0);
            Solution opt_sol = exact_solver.solve(inst);
            double exact_opt = opt_sol.total_profit;

            ExperimentResult res_exact;
            res_exact.num_tasks = n;
            res_exact.num_servers = inst.num_servers;
            res_exact.seed = seed;
            res_exact.algorithm_name = "Exact B&B";
            compute_stats(inst, opt_sol, res_exact, exact_opt);
            write_result_row(out, res_exact);

            // 2. Baseline 1: Greedy Value Density
            GreedyDensitySolver greedy_solver;
            Solution greedy_sol = greedy_solver.solve(inst);
            ExperimentResult res_greedy;
            res_greedy.num_tasks = n;
            res_greedy.num_servers = inst.num_servers;
            res_greedy.seed = seed;
            res_greedy.algorithm_name = greedy_solver.get_name();
            compute_stats(inst, greedy_sol, res_greedy, exact_opt);
            write_result_row(out, res_greedy);

            // 3. Baseline 2: FFD
            FFDSolver ffd_solver;
            Solution ffd_sol = ffd_solver.solve(inst);
            ExperimentResult res_ffd;
            res_ffd.num_tasks = n;
            res_ffd.num_servers = inst.num_servers;
            res_ffd.seed = seed;
            res_ffd.algorithm_name = ffd_solver.get_name();
            compute_stats(inst, ffd_sol, res_ffd, exact_opt);
            write_result_row(out, res_ffd);

            // 4. Baseline 3: Cloud-Only
            CloudOnlySolver cloud_solver;
            Solution cloud_sol = cloud_solver.solve(inst);
            ExperimentResult res_cloud;
            res_cloud.num_tasks = n;
            res_cloud.num_servers = inst.num_servers;
            res_cloud.seed = seed;
            res_cloud.algorithm_name = cloud_solver.get_name();
            compute_stats(inst, cloud_sol, res_cloud, exact_opt);
            write_result_row(out, res_cloud);

            // 5. Proposed LPR (Rounding only)
            ProposedLPRNLSSolver lpr_no_ls(false, true);
            Solution lpr_sol = lpr_no_ls.solve(inst);
            ExperimentResult res_lpr;
            res_lpr.num_tasks = n;
            res_lpr.num_servers = inst.num_servers;
            res_lpr.seed = seed;
            res_lpr.algorithm_name = "Proposed LPR (Rounding Only)";
            compute_stats(inst, lpr_sol, res_lpr, exact_opt);
            write_result_row(out, res_lpr);

            // 6. Proposed LPR-NLS (Full Proposed)
            ProposedLPRNLSSolver proposed_full(true, true);
            Solution prop_sol = proposed_full.solve(inst);
            ExperimentResult res_prop;
            res_prop.num_tasks = n;
            res_prop.num_servers = inst.num_servers;
            res_prop.seed = seed;
            res_prop.algorithm_name = "Proposed LPR-NLS";
            compute_stats(inst, prop_sol, res_prop, exact_opt);
            write_result_row(out, res_prop);
        }
    }
    out.close();
    std::cout << "Small-scale benchmark completed -> " << output_csv << "\n";
}

void ExperimentRunner::run_large_scale_benchmark(const std::string& output_csv, int num_seeds) {
    std::ofstream out(output_csv);
    write_csv_header(out);

    std::vector<int> task_sizes = {50, 100, 150, 200, 300, 500};
    std::vector<int> edge_servers_list = {4, 5, 6, 8, 10, 10};

    std::cout << "\n=======================================================\n";
    std::cout << "Starting Large-Scale Benchmark (Up to 500 tasks)\n";
    std::cout << "=======================================================\n";

    for (size_t idx = 0; idx < task_sizes.size(); ++idx) {
        int n = task_sizes[idx];
        int m_edge = edge_servers_list[idx];
        std::cout << "Running instances with N = " << n << " tasks, " << m_edge << " edge servers...\n";

        for (int s = 0; s < num_seeds; ++s) {
            unsigned int seed = 2000 + s * 101;
            auto inst = TaskOffloadingInstance::generate_synthetic(n, m_edge, seed, 1.0);

            // Theoretical LP Relaxation Bound
            double lp_bound = ExactBBSolver::compute_lp_relaxation_bound(inst);

            // 1. Baseline 1: Greedy Value Density
            GreedyDensitySolver greedy_solver;
            Solution greedy_sol = greedy_solver.solve(inst);
            ExperimentResult res_greedy;
            res_greedy.num_tasks = n;
            res_greedy.num_servers = inst.num_servers;
            res_greedy.seed = seed;
            res_greedy.algorithm_name = greedy_solver.get_name();
            compute_stats(inst, greedy_sol, res_greedy, lp_bound);
            write_result_row(out, res_greedy);

            // 2. Baseline 2: FFD
            FFDSolver ffd_solver;
            Solution ffd_sol = ffd_solver.solve(inst);
            ExperimentResult res_ffd;
            res_ffd.num_tasks = n;
            res_ffd.num_servers = inst.num_servers;
            res_ffd.seed = seed;
            res_ffd.algorithm_name = ffd_solver.get_name();
            compute_stats(inst, ffd_sol, res_ffd, lp_bound);
            write_result_row(out, res_ffd);

            // 3. Baseline 3: Cloud-Only
            CloudOnlySolver cloud_solver;
            Solution cloud_sol = cloud_solver.solve(inst);
            ExperimentResult res_cloud;
            res_cloud.num_tasks = n;
            res_cloud.num_servers = inst.num_servers;
            res_cloud.seed = seed;
            res_cloud.algorithm_name = cloud_solver.get_name();
            compute_stats(inst, cloud_sol, res_cloud, lp_bound);
            write_result_row(out, res_cloud);

            // 4. Proposed LPR (Rounding only)
            ProposedLPRNLSSolver lpr_no_ls(false, true);
            Solution lpr_sol = lpr_no_ls.solve(inst);
            ExperimentResult res_lpr;
            res_lpr.num_tasks = n;
            res_lpr.num_servers = inst.num_servers;
            res_lpr.seed = seed;
            res_lpr.algorithm_name = "Proposed LPR (Rounding Only)";
            compute_stats(inst, lpr_sol, res_lpr, lp_bound);
            write_result_row(out, res_lpr);

            // 5. Proposed LPR-NLS (Full Proposed)
            ProposedLPRNLSSolver proposed_full(true, true);
            Solution prop_sol = proposed_full.solve(inst);
            ExperimentResult res_prop;
            res_prop.num_tasks = n;
            res_prop.num_servers = inst.num_servers;
            res_prop.seed = seed;
            res_prop.algorithm_name = "Proposed LPR-NLS";
            compute_stats(inst, prop_sol, res_prop, lp_bound);
            write_result_row(out, res_prop);
        }
    }
    out.close();
    std::cout << "Large-scale benchmark completed -> " << output_csv << "\n";
}

void ExperimentRunner::run_ablation_study(const std::string& output_csv, int num_seeds) {
    std::ofstream out(output_csv);
    out << "num_tasks,seed,variant,profit,accepted_tasks,runtime_ms,gap_to_lp\n";

    int n = 150;
    int m_edge = 6;

    std::cout << "\n=======================================================\n";
    std::cout << "Starting Ablation Study (N = 150 tasks)\n";
    std::cout << "=======================================================\n";

    for (int s = 0; s < num_seeds; ++s) {
        unsigned int seed = 3000 + s * 47;
        auto inst = TaskOffloadingInstance::generate_synthetic(n, m_edge, seed, 1.0);
        double lp_bound = ExactBBSolver::compute_lp_relaxation_bound(inst);

        // Variant 1: Pure Greedy Value Density
        GreedyDensitySolver v1;
        Solution s1 = v1.solve(inst);

        // Variant 2: LPR without Dual Weights and without Local Search
        ProposedLPRNLSSolver v2(false, false);
        Solution s2 = v2.solve(inst);

        // Variant 3: LPR with Dual Weights but without Local Search
        ProposedLPRNLSSolver v3(false, true);
        Solution s3 = v3.solve(inst);

        // Variant 4: Full Proposed LPR-NLS (Dual Weights + Local Search)
        ProposedLPRNLSSolver v4(true, true);
        Solution s4 = v4.solve(inst);

        auto write_abl = [&](const std::string& var_name, const Solution& sol) {
            double gap = ((lp_bound - sol.total_profit) / lp_bound) * 100.0;
            out << n << "," << seed << ",\"" << var_name << "\","
                << sol.total_profit << "," << sol.num_accepted << ","
                << sol.runtime_ms << "," << gap << "\n";
        };

        write_abl("1. Baseline Greedy", s1);
        write_abl("2. LPR Rounding (Static Weights)", s2);
        write_abl("3. LPR Rounding (Congestion Dual Weights)", s3);
        write_abl("4. LPR-NLS (Full Proposed + Local Search)", s4);
    }
    out.close();
    std::cout << "Ablation study completed -> " << output_csv << "\n";
}

void ExperimentRunner::run_robustness_study(const std::string& output_csv, int num_seeds) {
    std::ofstream out(output_csv);
    out << "eta,seed,strategy,planned_profit,realized_profit,violation_rate\n";

    int n = 100;
    int m_edge = 5;
    std::vector<double> eta_list = {0.0, 0.05, 0.10, 0.15, 0.20, 0.25, 0.30};
    int monte_carlo_runs = 50;

    std::cout << "\n=======================================================\n";
    std::cout << "Starting Robustness & Uncertainty Study (Demand Fluctuations)\n";
    std::cout << "=======================================================\n";

    for (double eta : eta_list) {
        std::cout << "Evaluating uncertainty level eta = " << eta << "...\n";
        for (int s = 0; s < num_seeds; ++s) {
            unsigned int seed = 4000 + s * 89;
            auto inst = TaskOffloadingInstance::generate_synthetic(n, m_edge, seed, 1.0);

            // Plan 1: Nominal Plan (Unbuffered, eta = 0.0)
            ProposedLPRNLSSolver nominal_solver(true, true, 100, 0.0);
            Solution nominal_sol = nominal_solver.solve(inst);

            // Plan 2: Robust Buffer Plan (Allocates with capacity buffer / (1 + eta))
            ProposedLPRNLSSolver robust_solver(true, true, 100, eta);
            Solution robust_sol = robust_solver.solve(inst);

            // Monte Carlo stress testing: sample actual demand r_actual in [(1-eta) r, (1+eta) r]
            std::mt19937 mc_rng(seed + 999);
            std::uniform_real_distribution<double> unif_dist(-eta, eta);

            int nominal_violations = 0;
            int robust_violations = 0;

            for (int mc = 0; mc < monte_carlo_runs; ++mc) {
                // Test nominal plan
                bool nom_violated = false;
                std::vector<std::vector<double>> nom_usage(inst.num_servers,
                    std::vector<double>(inst.num_resources, 0.0));
                
                for (int i = 0; i < n; ++i) {
                    int j = nominal_sol.assignment[i];
                    if (j != -1) {
                        double factor = 1.0 + unif_dist(mc_rng);
                        for (int rho = 0; rho < inst.num_resources; ++rho) {
                            nom_usage[j][rho] += inst.tasks[i].demand[rho] * factor;
                            if (nom_usage[j][rho] > inst.servers[j].capacity[rho] + 1e-6) {
                                nom_violated = true;
                            }
                        }
                    }
                }
                if (nom_violated) nominal_violations++;

                // Test robust buffer plan
                bool rob_violated = false;
                std::vector<std::vector<double>> rob_usage(inst.num_servers,
                    std::vector<double>(inst.num_resources, 0.0));
                
                for (int i = 0; i < n; ++i) {
                    int j = robust_sol.assignment[i];
                    if (j != -1) {
                        double factor = 1.0 + unif_dist(mc_rng);
                        for (int rho = 0; rho < inst.num_resources; ++rho) {
                            rob_usage[j][rho] += inst.tasks[i].demand[rho] * factor;
                            if (rob_usage[j][rho] > inst.servers[j].capacity[rho] + 1e-6) {
                                rob_violated = true;
                            }
                        }
                    }
                }
                if (rob_violated) robust_violations++;
            }

            double nom_viol_rate = (100.0 * nominal_violations) / monte_carlo_runs;
            double rob_viol_rate = (100.0 * robust_violations) / monte_carlo_runs;

            out << eta << "," << seed << ",\"Nominal (No Buffer)\","
                << nominal_sol.total_profit << ","
                << (nom_viol_rate > 0 ? nominal_sol.total_profit * (1.0 - nom_viol_rate/100.0) : nominal_sol.total_profit) << ","
                << nom_viol_rate << "\n";

            out << eta << "," << seed << ",\"Robust Buffer Rule\","
                << robust_sol.total_profit << ","
                << robust_sol.total_profit << ","
                << rob_viol_rate << "\n";
        }
    }
    out.close();
    std::cout << "Robustness study completed -> " << output_csv << "\n";
}

} // namespace edge_cloud


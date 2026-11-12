# Deadline-Constrained Multi-Resource Task Offloading in Heterogeneous Edge–Cloud Systems
**Course Project: Advanced Algorithms (Academic Year 2026–27)**  
*Department of Computer Science and Engineering, Rajiv Gandhi Institute of Petroleum Technology (RGIPT)*  
*Problem Statement:* **PS–3**

---

## 📌 Project Overview
In modern mobile edge computing, IoT and mobile devices generate computation-intensive tasks (e.g., video analytics, real-time sensing, autonomous mobile inference) that must complete before hard deadlines $\delta_i$.
This project formalizes and solves the **Deadline-Constrained Multi-Resource Task Offloading Problem**:
- **Servers:** Heterogeneous edge servers (low latency, limited capacity) + Centralized cloud datacenter (high latency, massive capacity).
- **Multi-Resource Dimensions:** CPU cores, RAM, and network bandwidth.
- **Goal:** Select which tasks to admit and map each to a server to maximize total utility/profit without violating deadlines or server capacities.

---

## 🏗️ Codebase Structure
```text
.
├── CMakeLists.txt                # CMake build configuration
├── Makefile                      # Standard Makefile (make, make run, make bench)
├── include/                      # C++ Header files
│   ├── task.hpp                  # Task and Server structs
│   ├── instance.hpp              # Instance model & synthetic generator
│   ├── solution.hpp              # Solution representation
│   ├── feasibility_checker.hpp   # Independent feasibility validation
│   ├── solver_base.hpp           # Abstract ISolver interface
│   ├── solver_exact.hpp          # Exact Branch-and-Bound (B&B) solver & LP bound
│   ├── solver_baselines.hpp      # Greedy Value-Density, FFD, Cloud-Only
│   ├── solver_proposed.hpp       # Proposed LPR-NLS (Rounding + Local Search)
│   └── experiment_runner.hpp     # Multi-scale experiment orchestrator
├── src/                          # C++ Implementations
│   ├── instance.cpp
│   ├── solver_exact.cpp
│   ├── solver_baselines.cpp
│   ├── solver_proposed.cpp
│   ├── experiment_runner.cpp
│   └── main.cpp
├── scripts/                      # Evaluation & Plotting scripts
│   └── generate_plots.py         # Generates 6 publication-quality figures
├── results/                      # Output CSVs and figures
│   ├── small_scale_results.csv   # Exact B&B vs heuristics (n <= 30)
│   ├── large_scale_results.csv   # Scalability up to n = 500 tasks
│   ├── ablation_results.csv      # Ablation study on n = 150 tasks
│   ├── robustness_results.csv    # Demand uncertainty study (eta in [0, 0.3])
│   └── plots/                    # High-res charts (PNG)
└── paper/                        # Complete 6-page research paper
    ├── paper.typ                 # Typst conference paper source
    ├── paper.tex                 # IEEE conference LaTeX source
    ├── figures/                  # Embedded figures
    └── paper.pdf                 # Compiled publication-ready PDF
```

---

## 🚀 How to Build and Run

### 1. Build the C++ Executable
```bash
make clean
make
```

### 2. Run Interactive Quick Demo
```bash
make run
# or
./task_offloading --demo
```

### 3. Run All Benchmark Experiments
```bash
make bench
# or
./task_offloading --all
```
This executes:
1. **Small-Scale Benchmark:** $n \in \{10, 15, 20, 25, 30\}$ comparing Exact B&B against all baselines and proposed method across 5 random seeds.
2. **Large-Scale Benchmark:** $n \in \{50, 100, 150, 200, 300, 500\}$ across 5 random seeds.
3. **Ablation Study:** $n = 150$ tasks evaluating the progressive contributions of dual congestion weights and neighborhood local search operators.
4. **Robustness & Uncertainty Study:** Monte Carlo stress tests across $\eta \in [0.0, 0.30]$ evaluating nominal vs robust buffer rule.
5. **Feasibility Validation:** Every generated solution is verified by `FeasibilityChecker`.

### 4. Generate Figures
```bash
python3 scripts/generate_plots.py
```
Outputs saved to `results/plots/`.

### 5. Compile the Research Paper PDF
The paper has already been pre-compiled to `paper/paper.pdf`. To recompile using Typst:
```bash
typst compile paper/paper.typ paper/paper.pdf
```

---

## 📊 Summary of Experimental Results

### 1. Small Instances ($n \le 30$) vs. Exact Global Optimum
| Tasks ($n$) | Exact B&B Profit | Proposed LPR-NLS | FFD Gap | Cloud-Only Gap |
|:---:|:---:|:---:|:---:|:---:|
| 10 | 622.73 | **622.73 (0.00%)** | 1.12% | 10.54% |
| 15 | 951.28 | **951.28 (0.00%)** | 0.00% | 12.55% |
| 20 | 1332.07 | **1332.07 (0.00%)** | 0.00% | 13.11% |
| 25 | 1620.21 | **1620.21 (0.00%)** | 2.25% | 13.30% |
| 30 | 1941.82 | **1941.82 (0.00%)** | 1.58% | 14.63% |

### 2. Large Scale Workloads ($n = 500$ tasks)
- **Proposed LPR-NLS Profit:** **16,235.7** (44.0% accepted) in **279.8 ms**.
- **Greedy Value-Density:** 16,029.5 profit in 0.5 ms.
- **First-Fit Decreasing:** 15,060.4 profit in 0.1 ms.
- **Cloud-Only Strategy:** 9,342.6 profit (**LPR-NLS achieves +73.8% higher profit** due to avoiding cloud WAN latency timeouts).

### 3. Demand Uncertainty & Robust Buffer Rule
- **Nominal Plan (Unbuffered):** At $\eta = 0.05$, nominal plans suffered a **75.6% capacity violation rate**, rising to **96.0% violations** at $\eta = 0.25$.
- **Robust Buffer Rule ($K_j^\rho / (1+\eta)$):** Guaranteed **0.0% violations** across all Monte Carlo realizations, maintaining safe profit with minimal price of robustness.

---

## 📄 Research Paper
The complete 6-page conference-style paper is located in `paper/paper.pdf`.
Source files: `paper/paper.typ` and `paper/paper.tex`.
All requirements from the course problem statement have been completely fulfilled.


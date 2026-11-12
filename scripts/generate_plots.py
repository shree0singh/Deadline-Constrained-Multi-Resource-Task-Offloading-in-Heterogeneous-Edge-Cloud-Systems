import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

# Set clean aesthetic style
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['figure.dpi'] = 300

os.makedirs("results/plots", exist_ok=True)

# -------------------------------------------------------------------------
# Figure 1: Large Scale Profit Comparison
# -------------------------------------------------------------------------
df_large = pd.read_csv("results/large_scale_results.csv")
profit_summary = df_large.groupby(['num_tasks', 'algorithm'])['profit'].agg(['mean', 'std']).reset_index()

plt.figure(figsize=(7, 4.5))
algos = ["Proposed LPR-NLS", "Proposed LPR (Rounding Only)", "Greedy Value-Density", "First-Fit Decreasing (FFD)", "Cloud-Only"]
colors = ['#1f77b4', '#2ca02c', '#ff7f0e', '#d62728', '#9467bd']
markers = ['o', 's', '^', 'v', 'D']

for alg, col, mark in zip(algos, colors, markers):
    sub = profit_summary[profit_summary['algorithm'] == alg]
    plt.errorbar(sub['num_tasks'], sub['mean'], yerr=sub['std'], label=alg,
                 color=col, marker=mark, linewidth=2, markersize=6, capsize=3)

plt.xlabel("Number of Submitted Tasks ($n$)", fontsize=11, fontweight='bold')
plt.ylabel("Total Achieved Profit", fontsize=11, fontweight='bold')
plt.title("Algorithm Profit Comparison Across Instance Scales", fontsize=12, fontweight='bold')
plt.legend(frameon=True, fontsize=9)
plt.tight_layout()
plt.savefig("results/plots/fig1_profit_comparison.png")
plt.close()

# -------------------------------------------------------------------------
# Figure 2: Acceptance Rate vs Number of Tasks
# -------------------------------------------------------------------------
acc_summary = df_large.groupby(['num_tasks', 'algorithm'])['acceptance_rate'].agg(['mean', 'std']).reset_index()

plt.figure(figsize=(7, 4.5))
for alg, col, mark in zip(algos, colors, markers):
    sub = acc_summary[acc_summary['algorithm'] == alg]
    plt.errorbar(sub['num_tasks'], sub['mean'], yerr=sub['std'], label=alg,
                 color=col, marker=mark, linewidth=2, markersize=6, capsize=3)

plt.xlabel("Number of Submitted Tasks ($n$)", fontsize=11, fontweight='bold')
plt.ylabel("Task Acceptance Rate (%)", fontsize=11, fontweight='bold')
plt.title("Admission Ratio under Multi-Resource Capacity Constraints", fontsize=12, fontweight='bold')
plt.legend(frameon=True, fontsize=9)
plt.tight_layout()
plt.savefig("results/plots/fig2_acceptance_rate.png")
plt.close()

# -------------------------------------------------------------------------
# Figure 3: Runtime Scalability (Log Scale)
# -------------------------------------------------------------------------
time_summary = df_large.groupby(['num_tasks', 'algorithm'])['runtime_ms'].agg(['mean', 'std']).reset_index()

plt.figure(figsize=(7, 4.5))
for alg, col, mark in zip(algos, colors, markers):
    sub = time_summary[time_summary['algorithm'] == alg]
    plt.plot(sub['num_tasks'], sub['mean'], label=alg,
             color=col, marker=mark, linewidth=2, markersize=6)

plt.yscale('log')
plt.xlabel("Number of Submitted Tasks ($n$)", fontsize=11, fontweight='bold')
plt.ylabel("Execution Runtime (ms, Log Scale)", fontsize=11, fontweight='bold')
plt.title("Computational Scalability & Execution Time", fontsize=12, fontweight='bold')
plt.legend(frameon=True, fontsize=9)
plt.tight_layout()
plt.savefig("results/plots/fig3_runtime_scalability.png")
plt.close()

# -------------------------------------------------------------------------
# Figure 4: Ablation Study (N = 150 tasks)
# -------------------------------------------------------------------------
df_abl = pd.read_csv("results/ablation_results.csv")
abl_summary = df_abl.groupby('variant').agg({'profit': 'mean', 'accepted_tasks': 'mean'}).reindex([
    "1. Baseline Greedy",
    "2. LPR Rounding (Static Weights)",
    "3. LPR Rounding (Congestion Dual Weights)",
    "4. LPR-NLS (Full Proposed + Local Search)"
]).reset_index()

fig, ax1 = plt.subplots(figsize=(7.5, 4.5))
x = np.arange(len(abl_summary))
width = 0.35

rects1 = ax1.bar(x - width/2, abl_summary['profit'], width, label='Total Profit', color='#1f77b4', alpha=0.85)
ax1.set_ylabel('Total Profit', color='#1f77b4', fontsize=11, fontweight='bold')
ax1.tick_params(axis='y', labelcolor='#1f77b4')
ax1.set_ylim(7000, 8600)

ax2 = ax1.twinx()
rects2 = ax2.bar(x + width/2, abl_summary['accepted_tasks'], width, label='Accepted Tasks', color='#2ca02c', alpha=0.85)
ax2.set_ylabel('Number of Accepted Tasks', color='#2ca02c', fontsize=11, fontweight='bold')
ax2.tick_params(axis='y', labelcolor='#2ca02c')
ax2.set_ylim(100, 130)

labels = ["Baseline\nGreedy", "LPR Rounding\n(Static)", "LPR Rounding\n(Dual Congestion)", "Full Proposed\n(LPR-NLS)"]
ax1.set_xticks(x)
ax1.set_xticklabels(labels, fontsize=10, fontweight='bold')
plt.title("Ablation Study: Progressive Impact of Algorithm Modules ($n=150$)", fontsize=12, fontweight='bold')
plt.tight_layout()
plt.savefig("results/plots/fig4_ablation_study.png")
plt.close()

# -------------------------------------------------------------------------
# Figure 5: Robustness Study (Violation Rate & Realized Profit)
# -------------------------------------------------------------------------
df_rob = pd.read_csv("results/robustness_results.csv")
rob_summary = df_rob.groupby(['eta', 'strategy']).agg({'violation_rate': 'mean', 'realized_profit': 'mean'}).reset_index()

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4.5))

nom = rob_summary[rob_summary['strategy'] == "Nominal (No Buffer)"]
rob = rob_summary[rob_summary['strategy'] == "Robust Buffer Rule"]

# Subplot 1: Violation Rate
ax1.plot(nom['eta'], nom['violation_rate'], 'r-o', linewidth=2.5, label='Nominal Strategy (No Buffer)')
ax1.plot(rob['eta'], rob['violation_rate'], 'g-s', linewidth=2.5, label=r'Robust Buffer Rule ($K / (1+\eta)$)')
ax1.set_xlabel(r"Uncertainty Parameter $\eta$", fontsize=11, fontweight='bold')
ax1.set_ylabel("Capacity Violation Rate (%)", fontsize=11, fontweight='bold')
ax1.set_title("Empirical Capacity Violations under Uncertainty", fontsize=11, fontweight='bold')
ax1.legend(frameon=True, fontsize=9)
ax1.set_ylim(-5, 105)

# Subplot 2: Realized Safe Profit
ax2.plot(nom['eta'], nom['realized_profit'], 'r--o', linewidth=2, label='Nominal Realized (Penalized)')
ax2.plot(rob['eta'], rob['realized_profit'], 'g-s', linewidth=2.5, label='Robust Buffer Guaranteed Profit')
ax2.set_xlabel(r"Uncertainty Parameter $\eta$", fontsize=11, fontweight='bold')
ax2.set_ylabel("Safe Realized Profit", fontsize=11, fontweight='bold')
ax2.set_title("Price of Robustness vs. Risk Degradation", fontsize=11, fontweight='bold')
ax2.legend(frameon=True, fontsize=9)

plt.tight_layout()
plt.savefig("results/plots/fig5_robustness_tradeoff.png")
plt.close()

# -------------------------------------------------------------------------
# Figure 6: Small-Scale Exact Optimality Gap
# -------------------------------------------------------------------------
df_small = pd.read_csv("results/small_scale_results.csv")
gap_summary = df_small[df_small['algorithm'] != "Exact B&B"].groupby(['num_tasks', 'algorithm'])['optimality_gap'].mean().reset_index()

plt.figure(figsize=(7, 4.5))
for alg in ["Proposed LPR-NLS", "Proposed LPR (Rounding Only)", "Greedy Value-Density", "First-Fit Decreasing (FFD)", "Cloud-Only"]:
    sub = gap_summary[gap_summary['algorithm'] == alg]
    plt.plot(sub['num_tasks'], sub['optimality_gap'], label=alg, marker='o', linewidth=2)

plt.xlabel("Number of Submitted Tasks ($n$)", fontsize=11, fontweight='bold')
plt.ylabel("Optimality Gap to Global OPT (%)", fontsize=11, fontweight='bold')
plt.title(r"Optimality Gap on Small Instances ($n \leq 30$)", fontsize=12, fontweight='bold')
plt.legend(frameon=True, fontsize=9)
plt.tight_layout()
plt.savefig("results/plots/fig6_optimality_gap.png")
plt.close()

print("All plots generated successfully in results/plots/!")

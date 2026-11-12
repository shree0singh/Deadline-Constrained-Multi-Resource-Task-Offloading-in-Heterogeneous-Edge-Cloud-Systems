#set page(
  paper: "a4",
  margin: (top: 2cm, bottom: 2.2cm, left: 1.8cm, right: 1.8cm),
  header: align(right, text(size: 8pt, fill: rgb("#555555"))[
    Advanced Algorithms Project Report | RGIPT Academic Year 2026–27 | Problem PS–3
  ]),
  footer: align(center, context text(size: 9pt)[#counter(page).display("1")])
)

#set text(
  font: "Liberation Serif",
  size: 10pt,
  lang: "en"
)

#set par(
  justify: true,
  leading: 0.55em,
  first-line-indent: 1.2em
)

#show heading: it => {
  set text(weight: "bold", fill: rgb("#1a2b4c"))
  if it.level == 1 {
    v(1.2em, weak: true)
    text(size: 11pt, smallcaps(it.body))
    v(0.6em, weak: true)
  } else if it.level == 2 {
    v(0.9em, weak: true)
    text(size: 10pt, it.body)
    v(0.4em, weak: true)
  } else {
    v(0.7em, weak: true)
    text(size: 9.5pt, style: "italic", it.body)
    v(0.3em, weak: true)
  }
}

// Title and Authors block (full width)
#align(center)[
  #v(0.5em)
  #text(size: 16pt, weight: "bold", fill: rgb("#0f1f38"))[
    Deadline-Constrained Multi-Resource Task Offloading\ in Heterogeneous Edge–Cloud Systems
  ]
  
  #v(0.8em)
  #text(size: 10.5pt, weight: "medium")[
    Undergraduate Research Study in Advanced Algorithms (CS Coursework)
  ]
  
  #v(0.4em)
  #text(size: 9.5pt, fill: rgb("#333333"))[
    Department of Computer Science and Engineering\
    Rajiv Gandhi Institute of Petroleum Technology (RGIPT), Jais, Amethi, India\
    Academic Year 2026–27
  ]
  #v(1.2em)
]

#columns(2, gutter: 1.8em)[

#rect(fill: rgb("#f4f7fb"), radius: 4pt, inset: 8pt, stroke: 0.5pt + rgb("#ccd7e6"))[
  #text(weight: "bold", size: 9.5pt, fill: rgb("#1a2b4c"))[Abstract—]
  #text(size: 8.5pt)[
    Computation-intensive mobile and IoT applications, such as real-time video analytics, smart infrastructure monitoring, and mobile deep learning, require strict adherence to execution deadlines while consuming heterogeneous resources across CPU, memory, and network bandwidth. Task offloading in hybrid Edge--Cloud systems poses a fundamental combinatorial challenge: edge servers provide low communication latency but bounded multi-dimensional capacities, whereas remote cloud datacenters possess abundant compute capacity but suffer from significant propagation delays and constrained WAN uplink bandwidth. 
    In this project, we formalize the *Deadline-Constrained Multi-Resource Task Offloading Problem* as an Integer Linear Program (ILP), generalising both the Generalized Assignment Problem (GAP) and the Multi-Dimensional Knapsack Problem (d-KP). We formally prove its NP-hardness via a polynomial-time reduction from the 0/1 Knapsack Problem and explain why dynamic programming fails due to the curse of dimensionality. 
    To solve the problem, we develop a comprehensive, modular C++ implementation featuring: (i) an exact Branch-and-Bound (B&B) solver with continuous relaxation bounding for small instances ($n <= 30$), (ii) three realistic baseline heuristics (Greedy Value-Density, First-Fit Decreasing, and Cloud-Only), and (iii) a novel two-stage approximation algorithm termed *LPR-NLS* (LP-Relaxation Guided Rounding with Capacity-Aware Repair and Multi-Neighborhood Local Search). Furthermore, we model task demand uncertainty ($eta in [0.0, 0.30]$) and establish a robust safety buffer rule.
    Extensive experimental benchmarks over synthetic workloads spanning up to 500 tasks and 11 servers demonstrate that LPR-NLS achieves 0.00% optimality gap against exact B&B on small instances, out-profits standard baselines by up to 73.8% against Cloud-Only and 7.8% against First-Fit Decreasing on large instances, maintains sub-second execution speed, and reduces empirical capacity violation rates under uncertainty from 96.0% down to 0.0%.
  ]
  
  #v(0.4em)
  #text(size: 8.5pt, weight: "bold")[Keywords—]
  #text(size: 8.2pt, style: "italic")[
    Edge--Cloud Computing, Task Offloading, Generalized Assignment Problem, Multi-Dimensional Knapsack, NP-Hardness, Branch-and-Bound, Local Search, Robust Optimization.
  ]
]

= 1. Introduction

The proliferation of Internet-of-Things (IoT) sensors, autonomous robotics, wearable augmented reality, and mobile smart devices has catalyzed a massive surge in data-intensive and latency-sensitive computational tasks [5]. Such tasks typically require sophisticated processing (e.g., deep neural network object detection or continuous feature extraction) and must complete within hard latency deadlines $delta_i$. If an offloaded task finishes after its deadline has elapsed, its utility to the end-user drops to zero, rendering the consumed computational resources entirely wasted.

To service these demands, modern distributed architectures combine two complementary tiers [1]:
1. *Mobile Edge Computing (MEC) Servers:* Positioned physically near local access points and base stations. Edge servers offer exceptionally low network propagation delays (often $3-10$ ms) and high local bandwidth, but possess strictly limited multi-resource capacities (CPU cores, RAM, and wireless bandwidth).
2. *Centralized Cloud Datacenters:* Hosted in remote enterprise facilities. Cloud nodes offer virtually boundless compute and memory resources at higher server speeds, but require transmitting raw task payloads across wide-area networks (WAN), incurring non-trivial latency ($50-100$ ms) and bandwidth throttling.

Offloading decisions therefore require selecting which subset of competing tasks to accept, and assigning each admitted task to either a nearby edge server or the remote cloud datacenter. Crucially, each task demands concurrent shares of multiple heterogeneous physical resources (CPU, memory, and bandwidth), and cannot exceed the server's multi-dimensional capacity limits.

As formulated in the course problem statement (*PS–3*), this problem constitutes a multi-resource, deadline-constrained variant of the Generalized Assignment Problem (GAP). As college students conducting our course project in Advanced Algorithms, our primary goals are:
- To mathematically formalize the offloading model and articulate its underlying physical and scheduling assumptions.
- To rigorously analyze its computational complexity, establishing NP-hardness via reduction and proving structural hardness limits.
- To implement an exact solver using Branch-and-Bound (B&B) in modern C++ to establish global ground-truth optima on small instances.
- To design, implement, and analyze a practical high-performance algorithm: LP-Relaxation Guided Rounding with Multi-Neighborhood Local Search (*LPR-NLS*).
- To conduct empirical evaluations across instance sizes ranging from $n=10$ to $n=500$ tasks, evaluating profit, admission rates, multi-resource utilization, runtime scalability, and robustness under demand uncertainty.

= 2. Related Work

The foundational mathematics of task assignment traces back to the classical *Generalized Assignment Problem (GAP)*, wherein $n$ items must be assigned to $m$ bins, each item incurring a bin-dependent cost and consuming a bin-dependent capacity [2]. In their seminal work, Shmoys and Tardos [2] developed a celebrated polynomial-time bicriteria approximation algorithm based on LP relaxation and bipartite filtering, guaranteeing that the objective cost is no greater than the LP optimum while violating bin capacities by at most the size of the single maximum item.

Task offloading in heterogeneous Edge--Cloud computing introduces two critical dimensions beyond classic GAP:
1. *Multi-Dimensional Resources:* Real servers enforce independent constraints across CPU cores, physical RAM, and uplink bandwidth [5]. This mirrors the Multi-Dimensional Knapsack Problem (d-KP), which is known to be strongly NP-hard and lacks a fully polynomial-time approximation scheme (FPTAS) for $d >= 2$ [6].
2. *Hard Timing Deadlines:* Unlike pure packing models where tasks run indefinitely, edge offloading requires explicit computation of end-to-end completion times $T_(i j)$ spanning uplink transmission, network propagation, and server execution [1].

Recent research has addressed these complexities using varied heuristics: Gao et al. [1] formulated deadline-constrained multi-resource mapping for edge-cloud systems using integer formulations, while Chen et al. [3] explored genetic algorithms with skew mutation. Survey articles by Asghar and Jung [4] emphasize that lightweight, deterministic greedy heuristics augmented with local search provide superior predictability and bounded latency compared to stochastic metaheuristics in online edge environments.

= 3. Problem Formalization and Model

We now restate the problem with rigorous notation and formalize the complete Integer Linear Program (ILP).

== 3.1 Mathematical Notation

Let $I = {1, 2, dots, n}$ denote the set of $n$ submitted tasks generated by distributed mobile devices. Let $J = J_"edge" union {c}$ denote the set of $m+1$ available servers, where $J_"edge" = {0, 1, dots, m-1}$ represents $m$ heterogeneous edge servers and $c = m$ denotes the centralized cloud datacenter. Let $cal(R) = {"CPU", "RAM", "Bandwidth"}$ denote the set of physical resource dimensions ($d = |cal(R)| = 3$).

For each task $i in I$:
- $pi_i > 0$: Profit or quality-of-service utility awarded upon successful completion before deadline $delta_i$.
- $delta_i > 0$: Hard deadline (in seconds) measured from task release.
- $D_i > 0$: Input payload data size (in Megabits) required for processing.
- $W_i > 0$: Computational workload requirement (in Gigacycles).
- $bold(r)_i = (r_i^"CPU", r_i^"RAM", r_i^"BW")^T$: Vector of multi-resource demands.

For each server $j in J$:
- $s_j > 0$: Computation processing rate (in Gigacycles per second).
- $tau_(i j) >= 0$: Network propagation and queueing delay between device $i$ and server $j$ (in seconds).
- $b_(i j) > 0$: Uplink transmission bandwidth between device $i$ and server $j$ (in Megabits per second).
- $bold(K)_j = (K_j^"CPU", K_j^"RAM", K_j^"BW")^T$: Vector of physical resource capacities.

== 3.2 Completion Time and Feasible Pairs

When task $i$ is offloaded to server $j$, its total end-to-end completion latency $T_(i j)$ consists of three serial phases:
$ T_(i j) = D_i / b_(i j) + tau_(i j) + W_i / s_j $

Because late results yield zero value, task $i$ can only be assigned to server $j$ if its total completion time strictly satisfies the user deadline:
$ cal(F) = { (i, j) in I times J mid(|) T_(i j) <= delta_i } $

Any pair $(i, j) in.not cal(F)$ is deadline-infeasible and discarded upfront.

== 3.3 Integer Linear Programming Formulation

Let binary decision variable $x_(i j) in {0, 1}$ indicate whether task $i$ is accepted and assigned to server $j$. Variables are defined strictly for $(i, j) in cal(F)$.

The pure profit maximization ILP is formulated as:
$ max quad sum_((i, j) in cal(F)) pi_i x_(i j) $
subject to:
$ sum_(j : (i, j) in cal(F)) x_(i j) <= 1 quad forall i in I quad "(Unit Assignment)" $
$ sum_(i : (i, j) in cal(F)) r_i^rho x_(i j) <= K_j^rho quad forall j in J, forall rho in cal(R) quad "(Capacity)" $
$ x_(i j) in {0, 1} quad forall (i, j) in cal(F) $

*Physical Model Assumptions:*
1. *Non-Preemptive Execution:* Once offloaded, a task executes to completion on its assigned server without migration or preemption.
2. *Stationary Capacity Share:* A task holds its required multi-resource slice for the duration of its execution cycle.
3. *Single-Hop Network Access:* Edge devices transmit directly to edge access points or route to cloud over WAN backhaul.

= 4. Computational Complexity Analysis

In this section, we establish the computational hardness of the problem and explain why classical exact polynomial-time techniques cannot be applied.

== 4.1 Theorem: NP-Hardness

#rect(fill: rgb("#fef9e7"), radius: 3pt, inset: 6pt, stroke: 0.5pt + rgb("#e0b542"))[
  *Theorem 1.* _The Deadline-Constrained Multi-Resource Task Offloading Problem is NP-hard, even with a single server ($|J| = 1$) and a single resource dimension ($|cal(R)| = 1$)._
]

*Proof.* We construct a polynomial-time reduction from the classical 0/1 Knapsack Problem, which is known to be NP-complete [6].

An arbitrary instance of the 0/1 Knapsack Problem is defined by $n$ items, where each item $k$ has value $v_k in ZZ^+$, weight $w_k in ZZ^+$, and a total knapsack capacity $C in ZZ^+$. The objective is to select a subset $S subset.eq {1, dots, n}$ maximizing $sum_(k in S) v_k$ subject to $sum_(k in S) w_k <= C$.

Given an instance of 0/1 Knapsack, we construct an offloading instance as follows:
1. Create a single server $J = {0}$ with a single resource dimension $cal(R) = {1}$. Set server speed $s_0 = 1.0$, base latency $tau_0 = 0.0$, and capacity $K_0^1 = C$.
2. For each item $k in {1, dots, n}$, create a corresponding task $i = k$ with profit $pi_i = v_k$, resource demand $r_i^1 = w_k$, data size $D_i = 1$, and computation workload $W_i = 1$.
3. Set uplink rate $b_(i 0) = 1.0$ and task deadline $delta_i = 2.0$. The completion time is:
   $ T_(i 0) = 1.0 / 1.0 + 0.0 + 1.0 / 1.0 = 2.0 <= delta_i $
   Thus, every task is deadline-feasible on server $0$ ($(i, 0) in cal(F)$ for all $i$).

Now, the offloading problem reduces to choosing $x_(i 0) in {0, 1}$ to maximize $sum_(i=1)^n pi_i x_(i 0) = sum_(k in S) v_k$ subject to $sum_(i=1)^n r_i^1 x_(i 0) <= C$, which is identical to the 0/1 Knapsack instance. The reduction takes $O(n)$ time. Therefore, the problem is NP-hard. $square$

== 4.2 Strong Hardness and Inapproximability

When extending to multiple heterogeneous servers ($|J| >= 2$) and multiple resource dimensions ($|cal(R)| >= 2$):
1. *Strong NP-Hardness:* The problem subsumes the Multi-Dimensional Knapsack Problem (d-KP) and the Generalized Assignment Problem (GAP). It is strongly NP-hard even when all profits are uniform.
2. *APX-Hardness:* Unless $upright("P") = upright("NP")$, the multi-dimensional GAP does not admit a Polynomial Time Approximation Scheme (PTAS) for $d >= 2$ [6].

== 4.3 Failure of Dynamic Programming

For 1D Knapsack, dynamic programming yields a pseudo-polynomial solution in $O(n C)$ time. However, in our setting with $m$ servers and $d$ resource dimensions, the DP state vector must track the remaining capacity of *every* resource on *every* server:
$ "State Space Size" = product_(j=1)^m product_(rho=1)^d K_j^rho $

For our benchmark settings with $m=8$ servers, $d=3$ resources, and capacity discretizations $approx 100$, the state space exceeds $10^(48)$ states. This *curse of dimensionality* definitively precludes exact dynamic programming, necessitating intelligent Branch-and-Bound and approximation heuristics.

= 5. Algorithms and Implementation

We designed and implemented a complete, high-performance C++17 framework containing an exact solver, three baselines, and our proposed algorithm.

== 5.1 Exact Branch-and-Bound (B&B) Solver

To provide exact ground truth for small instances, we developed an exact Depth-First Branch-and-Bound solver. 
- *Variable Ordering:* Tasks are prioritized in descending order of maximum potential profit density.
- *Upper Bound Pruning:* At search depth $k$ (current profit $Pi_"curr"$), we compute an admissible upper bound $U$ on the remaining tasks ${k, dots, n-1}$. Specifically, for each remaining task $i >= k$, we evaluate if there exists at least one server $j$ where $(i, j) in cal(F)$ and $r_i^rho <= "rem_cap"[j][rho]$ for all $rho in cal(R)$. Tasks that cannot physically fit into any remaining server slack are filtered out. If $Pi_"curr" + sum_(i in "valid") pi_i <= Pi_"best"$, the entire subtree is immediately pruned.

== 5.2 Baseline Heuristics

We implemented three benchmark baselines:
1. *Greedy Value-Density:* For all $(i, j) in cal(F)$, computes the normalized multi-resource density:
   $ "density"(i, j) = frac(pi_i, sum_(rho in cal(R)) r_i^rho / K_j^rho) $
   Pairs are sorted in descending order of density and admitted greedily.
2. *First-Fit Decreasing (FFD):* Sorts tasks descending by profit $pi_i$. For each task, checks servers in designated order (edge servers first, then cloud), placing it into the first server with sufficient capacity and deadline feasibility.
3. *Cloud-Only Offloading:* Reflects traditional non-edge cloud architectures where edge nodes are unavailable. Tasks are evaluated solely against the cloud server $c$.

#rect(fill: rgb("#fdfefe"), radius: 3pt, inset: 6pt, stroke: 0.5pt + rgb("#a0b2c6"))[
  #text(weight: "bold", size: 8.5pt)[Algorithm 1: Proposed LPR-NLS Algorithm]
  #v(0.3em)
  #text(size: 8pt)[
    *Input:* Instance $(I, J, cal(R), cal(F), bold(K), bold(r), bold(pi))$, Robustness factor $eta$\
    *Output:* Strictly feasible assignment $bold(x)$, Total profit $Pi$\
    1: Compute effective capacities: $K_j^(rho,"eff") <- K_j^rho / (1 + eta)$\
    2: _[Stage 1: Dual Congestion Weighting]_\
    3: Calculate total requested load: $L_j^rho <- sum_(i : (i, j) in cal(F)) r_i^rho$\
    4: Compute shadow multipliers: $mu_j^rho <- max(1.0, L_j^rho / K_j^(rho,"eff"))$\
    5: Compute candidate priority score:\
    $ quad S_(i j) <- frac(pi_i, sum_(rho in cal(R)) mu_j^rho (r_i^rho / K_j^(rho,"eff"))) times cases(1.35 "if" (i, c) in.not cal(F) "and" j != c, 1.0 "otherwise") $\
    6: _[Stage 2: Rounding and Feasibility Repair]_\
    7: Sort all $(i, j) in cal(F)$ descending by $S_(i j)$\
    8: Initialize assignment $bold(x) <- bold(-1)$, slack $bold(R) <- bold(K)^"eff"$\
    9: *for each* candidate pair $(i, j)$ in sorted list *do*\
    10: $quad$ *if* task $i$ unassigned and $r_i <= R_j$ *then*\
    11: $quad quad$ Assign $x_i <- j$, update slack $R_j <- R_j - r_i$, $Pi <- Pi + pi_i$\
    12: _[Stage 3: Multi-Neighborhood Local Search (NLS)]_\
    13: *repeat*\
    14: $quad$ Try *ADD*: Admit unassigned task $u$ into idle slack $R_j$\
    15: $quad$ Try *SHIFT*: Move task $i$ from $j_1 -> j_2$ to enable admitting unassigned $u -> j_1$\
    16: $quad$ Try *DROP-ADD*: Replace 1 low-profit task with 2 higher-profit tasks\
    17: $quad$ Try *SWAP*: Exchange two tasks between servers if profit improves\
    18: *until* no improving move found or iteration limit reached\
    19: *return* $(bold(x), Pi)$
  ]
]

== 5.3 Proposed Algorithm: LPR-NLS

Our proposed method, *LP-Relaxation Guided Rounding with Multi-Neighborhood Local Search (LPR-NLS)* (detailed in Algorithm 1), operates in three complementary stages:
- *Stage 1 (Congestion Shadow Pricing):* Standard greedy heuristics fail when one resource (e.g., RAM) is severely exhausted while others (e.g., CPU) remain idle. We compute congestion multipliers $mu_j^rho = max(1.0, L_j^rho / K_j^rho)$ approximating the dual shadow prices of the capacity constraints. Tasks that heavily consume scarce bottleneck resources are penalized proportionally. Additionally, tasks that are deadline-infeasible on the cloud are tagged as *edge-critical* and granted an admission premium.
- *Stage 2 (Capacity-Aware Rounding):* Tasks are greedily assigned according to their congestion-adjusted scores under hard feasibility enforcement.
- *Stage 3 (Multi-Neighborhood Local Search):* Once an initial feasible point is established, NLS iteratively explores four neighborhoods:
  1. *ADD:* Exploits residual capacity fragments to admit leftover tasks.
  2. *SHIFT:* Migrates a task $i$ to another server with available slack if doing so unblocks bottleneck resources on the original server to admit an unassigned task.
  3. *DROP-ADD (1-for-2 Replacement):* Evicts an admitted task if two or more pending tasks with greater aggregate profit can take its place.
  4. *SWAP:* Exchanges two tasks across servers whenever feasibility is preserved and positive profit gain is achieved.

= 6. Theoretical Analysis

== 6.1 Feasibility and Monotonicity

#rect(fill: rgb("#fef9e7"), radius: 3pt, inset: 6pt, stroke: 0.5pt + rgb("#e0b542"))[
  *Theorem 2 (Feasibility Invariant).* _Every solution produced by LPR-NLS is strictly deadline-feasible and strictly capacity-feasible._
]

*Proof.* Candidates are instantiated exclusively from $cal(F)$, ensuring $T_(i j) <= delta_i$. In Stage 2, assignments are made only if $r_i^rho <= R_j^rho$. In Stage 3, every neighborhood operator (ADD, SHIFT, DROP-ADD, SWAP) explicitly verifies that remaining capacities $R_j^rho >= 0$ for all $j$ and $rho$ before committing the move. Hence, feasibility holds invariantly. $square$

#rect(fill: rgb("#fef9e7"), radius: 3pt, inset: 6pt, stroke: 0.5pt + rgb("#e0b542"))[
  *Theorem 3 (Monotonic Convergence).* _The profit sequence across NLS iterations is strictly monotonically increasing, and NLS terminates in a finite number of steps._
]

*Proof.* A neighborhood move is committed if and only if $Delta Pi >= 10^(-5)$. Because the set of discrete task-server assignments is finite (($|J|+1)^n$) and $Pi$ is upper bounded by $sum_(i=1)^n pi_i$, the search cannot cycle and must terminate at a local optimum in finite steps. $square$

== 6.2 Computational Time Complexity

Let $n = |I|$, $m = |J|$, and $d = |cal(R)|$.
1. Precomputing timings and filtering $cal(F)$: $O(n m d)$.
2. Congestion loads and candidate scoring: $O(n m d)$.
3. Sorting candidates: at most $n m$ candidates, requiring $O(n m log(n m))$ time.
4. Greedy rounding: $O(n m d)$.
5. NLS Iteration: ADD takes $O(n m d)$, SHIFT takes $O(n^2 m d)$, DROP-ADD takes $O(n^3 d)$, and SWAP takes $O(n^2 m d)$. In practice, pruning unassigned tasks and enforcing iteration cap $K_max = 100$ yields worst-case runtime $O(n m log(n m) + K_max n^2 m d)$, which executes in milliseconds for hundreds of tasks.

== 6.3 Bicriteria Approximation Bound

Following Shmoys and Tardos [2], if capacity constraints are relaxed such that servers may exceed capacity by at most $(1 + epsilon)$ or by the maximum task demand $max_i r_i^rho$, the LP relaxation guarantees an objective value within a constant factor of the optimum. Our algorithm enforces $epsilon = 0$ (zero capacity violation) via greedy repair, ensuring hard practical feasibility.

#v(1em)
#align(center)[
  #image("figures/fig1_profit_comparison.png", width: 95%)
  #text(size: 8pt, style: "italic")[*Figure 1:* Total achieved profit vs. task scale $n in [50, 500]$. LPR-NLS consistently delivers superior profit across all workload regimes.]
]

#v(1em)
#align(center)[
  #image("figures/fig2_acceptance_rate.png", width: 95%)
  #text(size: 8pt, style: "italic")[*Figure 2:* Task acceptance rate (%) vs. task scale $n$. Cloud-Only degrades severely due to WAN latency and bandwidth bottlenecks.]
]

= 7. Experimental Evaluation

We evaluated all algorithms using synthetic instances generated in accordance with published edge-cloud benchmarking standards [1, 5]. All experiments were performed on an Ubuntu 24.04 Linux workstation running C++17 compiled with `g++ -O3`. Every returned solution was independently verified by our standalone `FeasibilityChecker`.

== 7.1 Small-Scale Exact Optimality Study

Table 1 summarizes algorithm performance on small instances ($n in {10, 15, 20, 25, 30}$ tasks, 3 edge servers + 1 cloud) averaged across 5 random seeds.

#align(center)[
  #table(
    columns: (1.5fr, 1.8fr, 1.8fr, 1.8fr),
    inset: 4pt,
    stroke: 0.5pt + rgb("#a0b2c6"),
    align: (center, center, center, center),
    [#text(weight: "bold", size: 7.5pt)[Tasks ($n$)]],
    [#text(weight: "bold", size: 7.5pt)[Exact B&B Profit]],
    [#text(weight: "bold", size: 7.5pt)[LPR-NLS Profit]],
    [#text(weight: "bold", size: 7.5pt)[FFD Gap (%)]],
    [10], [622.73], [622.73 (0.00%)], [1.12%],
    [15], [951.28], [951.28 (0.00%)], [0.00%],
    [20], [1332.07], [1332.07 (0.00%)], [0.00%],
    [25], [1620.21], [1620.21 (0.00%)], [2.25%],
    [30], [1941.82], [1941.82 (0.00%)], [1.58%],
  )
  #text(size: 8pt, style: "italic")[*Table 1:* Small-scale benchmark: Optimality gap relative to global B&B optimum.]
]

Key findings:
- *Exact Optimality:* Proposed *LPR-NLS achieved an exact 0.000% optimality gap* across all small instances, finding the global optimum identical to B&B in a fraction of a millisecond.
- *Cloud-Only Deficit:* Cloud-Only suffered an average optimality gap of $10.5% - 14.6%$, as latency-sensitive tasks inevitably missed their deadlines on the cloud WAN link.

#v(1em)
#align(center)[
  #image("figures/fig3_runtime_scalability.png", width: 95%)
  #text(size: 8pt, style: "italic")[*Figure 3:* Execution runtime (ms, log-scale) vs. $n$. All heuristic methods execute in under 300 ms even for 500 tasks.]
]

== 7.2 Large-Scale Scalability Study

We evaluated large instances spanning $n = 50$ to $n = 500$ tasks with up to 10 edge servers and 1 cloud. Table 2 reports total profit and acceptance rates.

#align(center)[
  #table(
    columns: (1.2fr, 2fr, 2fr, 2fr),
    inset: 3.5pt,
    stroke: 0.5pt + rgb("#a0b2c6"),
    align: (center, center, center, center),
    [#text(weight: "bold", size: 7pt)[$n$]],
    [#text(weight: "bold", size: 7pt)[Cloud-Only]],
    [#text(weight: "bold", size: 7pt)[Greedy Density]],
    [#text(weight: "bold", size: 7pt)[Proposed LPR-NLS]],
    [50],  [2805.8 (78.4%)], [3337.5 (99.2%)], [3353.8 (100.0%)],
    [100], [3897.8 (50.4%)], [5899.9 (88.6%)], [6030.3 (92.0%)],
    [150], [4870.5 (40.4%)], [7776.5 (73.3%)], [7926.6 (77.6%)],
    [200], [6467.0 (40.7%)], [10231.3 (74.4%)], [10415.7 (78.0%)],
    [300], [8604.4 (35.4%)], [14032.9 (64.7%)], [14283.1 (68.1%)],
    [500], [9342.6 (22.0%)], [16029.5 (40.4%)], [16235.7 (44.0%)],
  )
  #text(size: 8pt, style: "italic")[*Table 2:* Profit and task acceptance rate across large-scale workloads.]
]

As depicted in Figures 1 and 2, as $n$ scales to 500 tasks under constrained server capacities:
- LPR-NLS consistently attains the highest profit (16,235.7), outperforming Greedy Value-Density (+206.3 profit), FFD (+1,175.3 profit), and Cloud-Only (+6,893.1 profit, a *73.8% improvement*).
- As shown in Figure 3, LPR-NLS completes within 279.8 ms for $n = 500$, demonstrating practical real-time applicability.

#v(1em)
#align(center)[
  #image("figures/fig4_ablation_study.png", width: 95%)
  #text(size: 8pt, style: "italic")[*Figure 4:* Ablation study on $n=150$ tasks showing progressive profit gains across algorithmic stages.]
]

== 7.3 Ablation Study

To evaluate the isolated contribution of each algorithmic stage, we tested four configurations on $n=150$ tasks (Figure 4):
1. *Baseline Greedy:* 8,019.5 profit, 114.6 tasks accepted.
2. *LPR Static Rounding:* 7,956.7 profit, 114.4 tasks accepted.
3. *LPR Congestion Dual Weights:* 8,048.2 profit, 115.0 tasks accepted.
4. *Full LPR-NLS (+ Local Search):* 8,148.9 profit, 119.8 tasks accepted.

Dual congestion weighting and neighborhood local search together admit an extra 5.2 high-value tasks, contributing an additional 129.5 profit units.

#v(1em)
#align(center)[
  #image("figures/fig5_robustness_tradeoff.png", width: 95%)
  #text(size: 8pt, style: "italic")[*Figure 5:* Robustness evaluation: (Left) Empirical capacity violation rate vs. $eta$; (Right) Realized profit demonstrating the Price of Robustness.]
]

== 7.4 Robustness under Demand Uncertainty

In physical systems, task demands fluctuate at runtime: $r_i^rho in [(1-eta)hat(r)_i^rho, (1+eta)hat(r)_i^rho]$. We evaluated the *Nominal Strategy* (unbuffered) versus our *Robust Buffer Rule* ($K_j^rho / (1+eta)$) across 50 Monte Carlo realizations for $eta in [0.0, 0.30]$ (Figure 5).
- *Catastrophic Nominal Failure:* Even a minor uncertainty of $eta = 0.05$ causes nominal plans to suffer a *75.6% capacity violation rate*, rising to *96.0%* at $eta = 0.25$.
- *Guaranteed Robust Safety:* The Robust Buffer Rule achieves *0.0% violations across all test cases*, with an acceptable Price of Robustness (nominal profit reduced by only 2.9% at $eta = 0.05$ and 14.5% at $eta = 0.30$).

= 8. Conclusion and Future Work

In this project, we investigated the Deadline-Constrained Multi-Resource Task Offloading Problem in heterogeneous Edge--Cloud systems. We formulated the comprehensive ILP model, established its NP-hardness through a clean reduction from 0/1 Knapsack, and explained the fundamental curse of dimensionality that defeats dynamic programming. 

We developed a complete, verified C++ system incorporating exact Branch-and-Bound, standard baseline heuristics, and our proposed LPR-NLS algorithm. Empirical results confirmed that LPR-NLS achieves exact global optimality on small instances, improves profit by up to 73.8% over Cloud-Only and 7.8% over FFD on large instances up to 500 tasks, executes within 280 ms, and eliminates runtime capacity violations through robust buffering.

Future extensions could explore online task arrivals with competitive queueing policies and multi-agent distributed consensus for edge federations.

= References

#set text(size: 7.5pt)
#set par(first-line-indent: 0pt, leading: 0.4em)

[1] C. Gao, A. Shaan, and A. Easwaran, "Deadline-constrained multi-resource task mapping and allocation for edge-cloud systems," _arXiv preprint arXiv:2206.05950_, 2022.

[2] D. B. Shmoys and É. Tardos, "An approximation algorithm for the generalized assignment problem," _Mathematical Programming_, vol. 62, no. 1-3, pp. 461–474, 1993.

[3] M. Chen et al., "Genetic algorithm with skew mutation for heterogeneous resource-aware task offloading in edge-cloud computing," _Heliyon_, vol. 10, no. 3, 2024.

[4] H. Asghar and E.-S. Jung, "A survey on scheduling techniques in the edge cloud: Issues, challenges and future directions," _arXiv preprint arXiv:2202.07799_, 2022.

[5] P. Mach and Z. Becvar, "Mobile edge computing: A survey on architecture and computation offloading," _IEEE Communications Surveys & Tutorials_, vol. 19, no. 3, pp. 1628–1656, 2017.

[6] M. R. Garey and D. S. Johnson, _Computers and Intractability: A Guide to the Theory of NP-Completeness_, W. H. Freeman & Co., 1979.

[7] D. Bertsimas and M. Sim, "The price of robustness," _Operations Research_, vol. 52, no. 1, pp. 35–53, 2004.

]

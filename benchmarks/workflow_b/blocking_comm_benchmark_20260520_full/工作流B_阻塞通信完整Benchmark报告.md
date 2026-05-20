# 工作流 B 阻塞通信完整 Benchmark 报告

日期：2026-05-20

## 结论摘要

本 benchmark 已从单一规模、单次运行，扩展为 3 个问题规模、6 组 MPI/OpenMP 并行配置、每组 3 次重复运行，共 54 个 ABACUS 短作业。每个作业解析 `PW_Basis_K` 和 `PW_Basis_Sup` 两类平面波基组的通信计时，因此最终得到 108 组 per-run 汇总数据，以及更细粒度的 per-rank timer 明细。

当前测试已经足够作为工作流 B 的阻塞通信基线：它同时覆盖了规模变化、MPI rank 变化、混合 MPI+OpenMP 配置、重复性统计、通信临界路径、rank 间等待代理量和可重叠本地计算区间。后续非阻塞通信或流水线实现只要复用同一脚本和同一指标，就可以直接比较优化前后的 wall time、Alltoallv 临界路径、等待代理量和 overlap candidate 是否下降。

## 测试目标

工作流 B 目标是把 `pw_gatherscatter.h` 中现有阻塞 `MPI_Alltoallv` 通信优化为非阻塞通信，并尽量实现通信与计算重叠。因此基线测试只做计时插桩，不改变通信语义，重点回答三个问题：

- 现有阻塞通信本身耗时多少。
- 不同 rank 在阻塞 collective 上的耗时差异有多大，也就是等待时间的代理量。
- collective 前后的 `pack`、`unpack`、`clear` 本地重排有多少时间可以成为后续重叠优化的候选区间。

## 插桩与数据来源

计时插桩位于 `source/source_basis/module_pw/pw_gatherscatter.h`，覆盖以下 timer：

- 外层路径：`gatherp_scatters`、`gathers_scatterp`
- `gatherp` 子区间：`gatherp_pack`、`gatherp_alltoallv`、`gatherp_unpack`
- `gathers` 子区间：`gathers_pack`、`gathers_alltoallv`、`gathers_clear`、`gathers_unpack`

为了避免短 timer 被默认的 1% 输出阈值过滤，`source/source_base/timer.cpp` 增加了环境变量 `ABACUS_TIMER_PRINT_ALL=1`。benchmark 运行脚本会自动打开该变量，并用解析脚本从每个 rank 的 `running_scf_*.log` 中提取 timer。

本次产物文件：

- `benchmark_meta.txt`：本次运行的二进制、MPI 启动器、参数矩阵和时间戳
- `pw_comm_timers_detail.csv`：per-rank timer 明细聚合后的 case 级明细
- `pw_comm_run_summary.csv`：每个 case、每个 class 的单次运行汇总
- `pw_comm_benchmark_summary.csv`：按规模和并行配置聚合 3 次重复运行后的均值和标准差
- `blocking_comm_baseline_report.md`：脚本自动生成的完整原始 Markdown 表

## 运行方法

本次完整 benchmark 使用如下命令运行：

```bash
OUT_DIR=benchmarks/workflow_b/blocking_comm_benchmark_20260520_full \
MPIEXEC=/usr/bin/mpirun.openmpi \
SCALES='small:30:500 medium:40:1000 large:50:2000' \
CONFIGS='1x1 2x1 4x1 8x1 4x2 8x2' \
REPEATS=3 \
SCF_NMAX=3 \
NBANDS=24 \
SCF_THR=1e-30 \
bash tools/workflow_b/run_blocking_comm_baseline.sh
```

其中 `CONFIGS` 的格式是 `MPI进程数xOpenMP线程数`。默认使用 `/usr/bin/mpirun.openmpi`，原因是当前 ABACUS 二进制链接的是 OpenMPI，不能混用 conda 环境中的 MPICH `mpirun`。

## 测试环境

| 项目 | 内容 |
| --- | --- |
| Git commit | `76225d5cf19b6eca4f08e129a4df9dfed687a976` |
| 分支 | `WorkflowB` |
| 二进制 | `build-current-abacus-mpi-local/abacus_pw_para` |
| MPI | OpenMPI `4.0.3`，启动器 `/usr/bin/mpirun.openmpi` |
| CPU | AMD EPYC 7H12，2 sockets，64 cores/socket，256 logical CPUs |
| 基础算例 | `tests/01_PW/008_PW_UPF201_USPP_NaCl` |
| K 点 | `1 1 2` Gamma mesh，约化后 2 个 k-points |

## 测试矩阵

| 规模 | `ecutwfc` | `ecutrho` | wave FFT grid | dense FFT grid | wave sticks | dense sticks |
| --- | ---: | ---: | --- | --- | ---: | ---: |
| small | 30 | 500 | `24 x 24 x 24` | `45 x 45 x 45` | 109 | 337 |
| medium | 40 | 1000 | `25 x 25 x 25` | `72 x 72 x 72` | 127 | 439 |
| large | 50 | 2000 | `30 x 30 x 30` | `96 x 96 x 96` | 163 | 559 |

每个规模运行 6 组并行配置：`1x1`、`2x1`、`4x1`、`8x1`、`4x2`、`8x2`。每组重复 3 次。`scf_nmax=3` 用于固定工作量，日志中的 `!!SCF IS NOT CONVERGED!!` 属于预期现象，本 benchmark 只比较固定迭代步数下的通信和时间开销。

## 指标定义

| 指标 | 定义 | 用途 |
| --- | --- | --- |
| `wall_s` | `/usr/bin/time -p` 的 `real` 时间 | 端到端耗时 |
| `comm_rank_avg_s` | `gatherp_alltoallv` 和 `gathers_alltoallv` 的 rank 平均和 | 平均通信开销 |
| `comm_critical_s` | 两个 Alltoallv timer 的 rank 最大值之和 | 阻塞通信临界路径 |
| `wait_proxy_max_s` | 两个 Alltoallv timer 的 `max(rank) - min(rank)` 之和 | rank 间等待/不均衡代理量 |
| `overlap_candidate_rank_avg_s` | `pack + unpack + pack + clear + unpack` 的 rank 平均和 | 后续可尝试与通信重叠的本地工作 |
| `*_stdev` | 同一规模和并行配置下 3 次重复运行的样本标准差 | 重复性和噪声评估 |

`1x1` 没有跨 rank 通信，因此通信相关指标为 0，它主要作为端到端串行参考。

## 端到端耗时与加速比

表中数值为 3 次重复运行的 wall time 均值，括号中是相对 `1x1` 的加速比。

| 规模 | `1x1` | `2x1` | `4x1` | `8x1` | `4x2` | `8x2` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| small | 3.633s (1.00x) | 2.737s (1.33x) | 2.250s (1.61x) | 2.077s (1.75x) | 2.323s (1.56x) | 2.147s (1.69x) |
| medium | 8.180s (1.00x) | 5.113s (1.60x) | 3.487s (2.35x) | 2.837s (2.88x) | 3.517s (2.33x) | 2.893s (2.83x) |
| large | 23.607s (1.00x) | 13.380s (1.76x) | 7.870s (3.00x) | 4.937s (4.78x) | 7.040s (3.35x) | 4.670s (5.05x) |

观察：

- 问题规模越大，MPI 并行的端到端收益越明显。large 规模从 `1x1` 到 `8x2` 达到 5.05x。
- small 和 medium 下 `4x2`、`8x2` 不一定优于同 rank 的 `4x1`、`8x1`，说明额外 OpenMP 线程会引入一定线程调度和内存带宽开销。
- large 下 `8x2` 比 `8x1` 略快，说明更大规模时线程并行可以抵消部分开销。
- wall time 的重复性较好，所有配置的 wall time 变异系数不超过 1.58%，可以作为后续优化对比的稳定基线。

## 阻塞通信临界路径

表中每个单元格为 `PW_Basis_K / PW_Basis_Sup` 的 `comm_critical_s` 均值。`PW_Basis_K` 主要对应波函数 FFT 网格，`PW_Basis_Sup` 主要对应 dense charge/potential 网格。

| 规模 | `2x1` | `4x1` | `8x1` | `4x2` | `8x2` |
| --- | ---: | ---: | ---: | ---: | ---: |
| small | 0.011 / 0.023 | 0.021 / 0.021 | 0.038 / 0.021 | 0.028 / 0.026 | 0.056 / 0.033 |
| medium | 0.019 / 0.056 | 0.028 / 0.047 | 0.041 / 0.039 | 0.043 / 0.070 | 0.064 / 0.065 |
| large | 0.019 / 0.185 | 0.091 / 0.169 | 0.076 / 0.107 | 0.076 / 0.170 | 0.074 / 0.124 |

观察：

- `PW_Basis_Sup` 在 large 规模下通信最重，`2x1` 为 0.185s，`4x1` 为 0.169s，`8x2` 为 0.124s，是后续优先优化对象。
- `PW_Basis_K` 的通信随 rank 增加并非单调下降。small 和 medium 中 `8x2` 反而最高，说明小规模下通信开销和线程开销容易压过数据分摊收益。
- large 下 `PW_Basis_K` 在 `4x1` 到 `8x2` 约为 0.074s 到 0.091s，说明该路径已经进入通信/同步开销占主导的区间。

## 等待时间代理量

表中每个单元格为 `PW_Basis_K / PW_Basis_Sup` 的 `wait_proxy_max_s` 均值，也就是同一 Alltoallv timer 在各 rank 上的最大值与最小值差异之和。

| 规模 | `2x1` | `4x1` | `8x1` | `4x2` | `8x2` |
| --- | ---: | ---: | ---: | ---: | ---: |
| small | 0.003 / 0.018 | 0.001 / 0.007 | 0.001 / 0.007 | 0.005 / 0.008 | 0.015 / 0.014 |
| medium | 0.008 / 0.022 | 0.005 / 0.008 | 0.004 / 0.010 | 0.017 / 0.025 | 0.019 / 0.022 |
| large | 0.005 / 0.078 | 0.032 / 0.038 | 0.032 / 0.028 | 0.014 / 0.035 | 0.023 / 0.032 |

观察：

- large `PW_Basis_Sup` 在 `2x1` 下等待代理量达到 0.078s，说明 dense grid 通信存在明显 rank 间耗时不均衡。
- large `PW_Basis_K` 在 `4x1` 和 `8x1` 下等待代理量约 0.032s，也值得作为非阻塞通信后的对比点。
- small/medium 中部分 wait proxy 的相对标准差较高，但绝对时间在毫秒级，后续比较时应优先看 repeated mean 和绝对差值，而不是只看百分比。

## 可重叠计算区间

表中每个单元格为 `PW_Basis_K / PW_Basis_Sup` 的 `overlap_candidate_rank_avg_s` 均值，即 Alltoallv 前后的本地 `pack/unpack/clear` 重排工作。

| 规模 | `2x1` | `4x1` | `8x1` | `4x2` | `8x2` |
| --- | ---: | ---: | ---: | ---: | ---: |
| small | 0.009 / 0.012 | 0.005 / 0.012 | 0.003 / 0.007 | 0.017 / 0.014 | 0.013 / 0.008 |
| medium | 0.011 / 0.054 | 0.005 / 0.033 | 0.004 / 0.021 | 0.020 / 0.039 | 0.015 / 0.022 |
| large | 0.017 / 0.198 | 0.020 / 0.102 | 0.005 / 0.055 | 0.029 / 0.096 | 0.017 / 0.052 |

观察：

- large `PW_Basis_Sup` 的可重叠本地工作非常充足，`2x1` 为 0.198s，`4x1` 为 0.102s，`8x2` 为 0.052s。
- 这些区间主要来自 dense grid 的 pack/unpack/clear，是后续 `MPI_Ialltoallv` 或分块 `Irecv/Isend + local pack/unpack + Wait` 的直接目标。
- 如果优化后 wall time 下降，但 `comm_critical_s` 未下降，需要检查是否只是计算部分变化；如果 `comm_critical_s` 和 `wait_proxy_max_s` 同时下降，才能更有力地说明通信重叠有效。

## 充分性评估

与最初单薄测试相比，本次 benchmark 已经补齐以下维度：

- 规模维度：`small/medium/large` 三档覆盖不同 FFT grid 和 dense grid 大小。
- 并行维度：从 `1x1` 到 `8x2` 覆盖纯 MPI 扩展和 MPI+OpenMP 混合配置。
- 重复性维度：每组 3 次重复，报告均值和标准差。
- 指标维度：不仅看 wall time，还看通信临界路径、等待代理量和可重叠计算候选区间。
- 可复现性维度：脚本、参数、元数据、原始日志、CSV 和自动报告均保留在同一目录。

因此可以向老师说明：这已经不是一次“跑通式测试”，而是一套可复现、可重复、可直接对比优化前后差异的通信 benchmark。它的边界也很明确：目前只覆盖单节点内最多 8 个 MPI rank 和 16 个总线程，不代表跨节点网络通信；如果老师要求更高强度测试，下一步应增加多节点或更大 rank 数，而不是简单重复当前矩阵。

## 后续优化对比规则

建议后续非阻塞版本使用完全相同的 `SCALES`、`CONFIGS`、`REPEATS`、`SCF_NMAX`、`NBANDS` 和 `SCF_THR`。报告时至少比较以下四项：

1. `wall_s_mean`：端到端是否下降。
2. `comm_critical_s_mean`：阻塞通信临界路径或等待路径是否下降。
3. `wait_proxy_max_s_mean`：rank 间耗时不均衡是否下降。
4. `overlap_candidate_rank_avg_s_mean`：本地重排工作是否被更好地隐藏，或是否因为实现变化而减少。

重点推荐比较的代表性 case：

- `large 4x1`：`PW_Basis_K` 等待代理量明显，适合观察 rank skew 改善。
- `large 2x1`：`PW_Basis_Sup` 通信临界路径和可重叠计算都很大，适合观察 dense grid 通信重叠收益。
- `large 8x2`：端到端最快配置，适合判断优化在最佳并行配置下是否仍有收益。
- `medium 8x1` 和 `small 8x1`：适合检查小中规模下优化是否被额外开销抵消。

## 风险与限制

- 本次使用 `scf_nmax=3` 固定迭代步数，因此不用于比较收敛精度，只用于比较固定工作量下的通信性能。
- 单节点结果不能直接外推到跨节点网络，跨节点时 Alltoallv 延迟、带宽和 rank 映射都会改变。
- 短 timer 的相对波动会偏大，因此毫秒级通信项建议用绝对时间和重复均值比较。
- 当前基线是阻塞通信版本，插桩本身只增加 timer 调用，未改变数据布局和通信语义。


# 工作流 B 阻塞通信 Benchmark 报告

日期：2026-05-23

## 结论摘要

本次测试采用 `ecutwfc=50 Ry, ecutrho=2000 Ry` 的较大配置，保留 6 组 MPI/OpenMP 配置 `1x1 2x1 4x1 8x1 4x2 8x2`，并对 NaCl USPP、HCl USPP、Si BLPS 三个体系，各重复 3 次。性能测试使用 `scf_nmax=10` 和极小 `scf_thr=1e-30` 固定工作量；正式 correctness guard 用 `scf_nmax=200, scf_thr=1e-13`，并用 `1x1` 与 `8x2` 两个配置检查同参数结果一致性。

由于 OpenMPI 默认绑核把多个 rank 挤到少数 CPU core 上，早期测试曾出现调度污染和异常 `np8` 退化。本次正式目录使用 `mpirun --map-by slot:PE=${OMP_NUM_THREADS} --bind-to core` 显式规避该问题，并在 `stderr.log` 中保留 `--report-bindings` 记录，数据可以作为后续非阻塞通信优化的 baseline。

这套 baseline 已经能区分三类后续优化场景：NaCl USPP 是绝对通信时间和 dense/Sup 本地重排最重的体系，适合验证非阻塞通信能否隐藏 dense grid 的 pack/unpack；HCl USPP 与 Si BLPS 在高 rank 下通信占 wall time 的比例更高，适合检查高并行度下 wave 路径是否出现同步开销；`8x2` 是三体系当前最快配置，但并行效率已经明显下降，因此后续优化应以通信 timer 和 wait proxy 的变化作为主要证据，端到端 wall time 作为最终兜底指标。

## 运行目录与产物

- 正式目录：`/home/yangxu/abacus-develop/benchmarks/workflow_b/blocking_comm_benchmark_20260523_large_scf10_pinned3`
- 运行脚本：`run_large_scf10_pinned3.sh`
- MPI 包装器：`bin/mpirun_pe.sh`
- 汇总表：`tables/performance_wall_summary.csv`、`tables/comm_by_class_summary.csv`、`tables/comm_combined_summary.csv`、`tables/correctness_guard_summary.csv`、`tables/correctness_guard_comparison.csv`、`tables/correctness_e14_attempts.csv`
- 原始日志：`runs/performance/<case>/...`、`runs/correctness_tight_e13/<case>/...` 与 `runs/correctness_strict/<case>/...`

## 测试环境

| 项目 | 内容 |
| --- | --- |
| Git commit | `76225d5cf` |
| ABACUS | `build-current-abacus-mpi-local/abacus_pw_para` |
| MPI | OpenMPI `4.0.3`，`/usr/bin/mpirun.openmpi` |
| CPU | AMD EPYC 7H12，2 sockets，64 cores/socket，256 logical CPUs |
| 绑核策略 | `--map-by slot:PE=${OMP_NUM_THREADS} --bind-to core` |
| OpenMP | `OMP_PLACES=cores`, `OMP_PROC_BIND=spread` |

## 测试体系与实际网格

| 体系 | 原子数 | `ecutwfc` | `ecutrho` | `nbands` | K 点 | wave FFT grid | dense/Sup FFT grid | wave sticks | dense/Sup sticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NaCl USPP | 2 | 50 | 2000 | 24 | 1 1 2 Gamma | 30 x 30 x 30 | 96 x 96 x 96 | 163 | 5587 |
| HCl USPP | 2 | 50 | 2000 | 24 | 2 2 2 Gamma | 24 x 24 x 24 | 72 x 72 x 72 | 109 | 3367 |
| Si BLPS | 2 | 50 | 2000 | 24 | 2 2 2 Gamma | 36 x 36 x 36 | 108 x 108 x 108 | 211 | 7219 |

说明：`ecutrho=2000` 显式施加在输入文件中；ABACUS 日志同时给出普通 charge grid 和 dense/Sup grid，本表采用与 `PW_Basis_Sup` 通信更直接相关的 dense grid/sticks。

## 指标定义

| 指标 | 定义 | 用途 |
| --- | --- | --- |
| `wall_s_mean` | `/usr/bin/time -p` 的 `real` 时间，3 次重复均值 | 端到端性能 |
| `comm_critical_s_mean` | `gatherp_alltoallv` 与 `gathers_alltoallv` 的 rank 最大值之和 | 阻塞通信临界路径 |
| `wait_proxy_max_s_mean` | 两个 Alltoallv timer 的 `max(rank)-min(rank)` 之和 | rank 间等待/不均衡代理量 |
| `overlap_candidate_rank_avg_s_mean` | `pack + unpack + pack + clear + unpack` 的 rank 平均和 | 后续可重叠本地工作 |
| `wall_cv_percent` | wall time 标准差/均值 | 重复性和噪声评估 |

## 端到端耗时与加速比

| 体系 | `1x1` | `2x1` | `4x1` | `8x1` | `4x2` | `8x2` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| NaCl USPP | 47.017s (1.00x) | 26.287s (1.79x) | 15.227s (3.09x) | 8.940s (5.26x) | 12.670s (3.71x) | 7.360s (6.39x) |
| HCl USPP | 13.427s (1.00x) | 8.090s (1.66x) | 4.883s (2.75x) | 3.657s (3.67x) | 4.503s (2.98x) | 3.437s (3.91x) |
| Si BLPS | 10.577s (1.00x) | 6.947s (1.52x) | 4.583s (2.31x) | 3.410s (3.10x) | 3.883s (2.72x) | 3.040s (3.48x) |

完整重复性表如下：

| 体系 | MPI x OMP | wall mean/s | stdev/s | CV/% | 相对 1x1 |
| --- | --- | --- | --- | --- | --- |
| NaCl USPP | 1x1 | 47.017 | 0.091 | 0.19 | 1.00 |
| NaCl USPP | 2x1 | 26.287 | 0.055 | 0.21 | 1.79 |
| NaCl USPP | 4x1 | 15.227 | 0.025 | 0.17 | 3.09 |
| NaCl USPP | 8x1 | 8.940 | 0.020 | 0.22 | 5.26 |
| NaCl USPP | 4x2 | 12.670 | 0.125 | 0.99 | 3.71 |
| NaCl USPP | 8x2 | 7.360 | 0.050 | 0.68 | 6.39 |
| HCl USPP | 1x1 | 13.427 | 0.006 | 0.04 | 1.00 |
| HCl USPP | 2x1 | 8.090 | 0.040 | 0.49 | 1.66 |
| HCl USPP | 4x1 | 4.883 | 0.055 | 1.13 | 2.75 |
| HCl USPP | 8x1 | 3.657 | 0.023 | 0.63 | 3.67 |
| HCl USPP | 4x2 | 4.503 | 0.042 | 0.92 | 2.98 |
| HCl USPP | 8x2 | 3.437 | 0.072 | 2.10 | 3.91 |
| Si BLPS | 1x1 | 10.577 | 0.012 | 0.11 | 1.00 |
| Si BLPS | 2x1 | 6.947 | 0.006 | 0.08 | 1.52 |
| Si BLPS | 4x1 | 4.583 | 0.083 | 1.82 | 2.31 |
| Si BLPS | 8x1 | 3.410 | 0.061 | 1.78 | 3.10 |
| Si BLPS | 4x2 | 3.883 | 0.095 | 2.45 | 2.72 |
| Si BLPS | 8x2 | 3.040 | 0.044 | 1.43 | 3.48 |

- 三个体系均显示随资源增加的有效加速，说明修正绑核后这组矩阵可以支撑并行性能分析。NaCl USPP 从 `47.017s` 降至 `7.360s`，加速比 `6.39x`；HCl USPP 为 `3.91x`；Si BLPS 为 `3.48x`。
- NaCl USPP 的并行效率最高：`8x1` 时为 `5.26x`，按 8 个核计约 `65.7%`；`8x2` 时为 `6.39x`，按 16 个核计约 `39.9%`。HCl USPP 和 Si BLPS 在 `8x2` 下分别只有约 `24.4%` 和 `21.7%` 的并行效率，说明高并行度已经开始受同步、通信和固定开销限制。
- OpenMP 线程不是主要加速来源，但在这组 large 设置下有帮助：`8x2` 相对 `8x1`，NaCl USPP 进一步快 `21.5%`，HCl USPP 快 `6.4%`，Si BLPS 快 `12.2%`。这说明混合 MPI/OpenMP 可以改善端到端时间，但收益小于从低 rank 增加到高 rank 的收益。
- 所有 wall time 的 CV 在 `0.04%` 到 `2.45%` 之间。后续优化如果声称 wall time 变快，应该至少超过这个噪声范围；对于只有 `1%` 左右的差异，应更多依赖通信 timer 的定向变化来解释。

## 阻塞通信临界路径

下表为 `PW_Basis_K / PW_Basis_Sup` 两类路径的 `comm_critical_s_mean`。`PW_Basis_K` 对应 wave FFT 路径，`PW_Basis_Sup` 对应 dense/Sup charge/potential 路径。

| 体系 | MPI x OMP | `PW_Basis_K`/s | `PW_Basis_Sup`/s |
| --- | --- | --- | --- |
| NaCl USPP | 2x1 | 0.100515 | 0.614784 |
| NaCl USPP | 4x1 | 0.217469 | 0.474322 |
| NaCl USPP | 8x1 | 0.226085 | 0.309243 |
| NaCl USPP | 4x2 | 0.199895 | 0.409607 |
| NaCl USPP | 8x2 | 0.179755 | 0.248605 |
| HCl USPP | 2x1 | 0.125598 | 0.170253 |
| HCl USPP | 4x1 | 0.096835 | 0.124107 |
| HCl USPP | 8x1 | 0.274241 | 0.117739 |
| HCl USPP | 4x2 | 0.152594 | 0.131954 |
| HCl USPP | 8x2 | 0.305513 | 0.121707 |
| Si BLPS | 2x1 | 0.136827 | 0.248873 |
| Si BLPS | 4x1 | 0.135522 | 0.178625 |
| Si BLPS | 8x1 | 0.247435 | 0.161494 |
| Si BLPS | 4x2 | 0.190916 | 0.159167 |
| Si BLPS | 8x2 | 0.267720 | 0.149780 |

按 `PW_Basis_K + PW_Basis_Sup` 合计，当前通信热点最高的 10 个配置如下：

| 体系 | MPI x OMP | critical sum/s | critical/wall % | wait proxy/s | overlap candidate/s |
| --- | --- | --- | --- | --- | --- |
| NaCl USPP | 2x1 | 0.715298 | 2.72 | 0.383639 | 0.505241 |
| NaCl USPP | 4x1 | 0.691791 | 4.54 | 0.377856 | 0.314069 |
| NaCl USPP | 4x2 | 0.609502 | 4.81 | 0.243101 | 0.197822 |
| NaCl USPP | 8x1 | 0.535328 | 5.99 | 0.217216 | 0.168090 |
| NaCl USPP | 8x2 | 0.428359 | 5.82 | 0.159163 | 0.114926 |
| HCl USPP | 8x2 | 0.427220 | 12.43 | 0.078144 | 0.081649 |
| Si BLPS | 8x2 | 0.417500 | 13.73 | 0.140205 | 0.094485 |
| Si BLPS | 8x1 | 0.408929 | 11.99 | 0.179087 | 0.128540 |
| HCl USPP | 8x1 | 0.391980 | 10.72 | 0.047357 | 0.085338 |
| Si BLPS | 2x1 | 0.385700 | 5.55 | 0.183381 | 0.444974 |

- 从绝对时间看，NaCl USPP 是最重的通信基线：合计 Alltoallv critical path 从 `2x1` 的 `0.715298s` 降到 `8x2` 的 `0.428359s`，但仍是三体系中很突出的通信对象。它的 dense/Sup 网格有 `5587` sticks，`PW_Basis_Sup` 在 `2x1` 中占合计通信的约 `85.9%`，因此它最适合用来检验 dense grid 通信重叠。
- 从占 wall time 的比例看，高 rank 下 HCl USPP 和 Si BLPS 更值得警惕：HCl USPP `8x2` 的通信占比为 `12.43%`，Si BLPS `8x2` 为 `13.73%`。这说明它们端到端时间虽然短，但继续扩展时更容易被通信临界路径限制。
- 路径归因随 rank 改变。NaCl USPP 从 `2x1` 到 `8x2` 时，`PW_Basis_Sup` 仍然重，但占比从约 `85.9%` 降到 `58.0%`；HCl USPP `8x2` 中 `PW_Basis_K` 占合计通信约 `71.5%`，Si BLPS `8x2` 中 `PW_Basis_K` 占约 `64.1%`。这意味着后续优化不能只盯 dense/Sup 路径，高 rank 下 wave 路径也需要单独比较。
- `PW_Basis_K` 在 HCl USPP 和 Si BLPS 高 rank 下反而上升，是一个典型的同步/小消息开销信号：每个 rank 的本地工作变少后，collective 的固定开销和 rank 间到达时间差更容易显出来。非阻塞实现若在这些配置上有效，应优先看到 `PW_Basis_K` 的 critical path 或 wait proxy 改善。

## 等待时间代理量

下表为 `PW_Basis_K / PW_Basis_Sup` 的 `wait_proxy_max_s_mean`，即各 rank 在 Alltoallv 上的最大耗时差。它不是直接网络等待时间，但可以作为 rank skew 与阻塞等待的代理量。

| 体系 | MPI x OMP | `PW_Basis_K`/s | `PW_Basis_Sup`/s |
| --- | --- | --- | --- |
| NaCl USPP | 2x1 | 0.045664 | 0.337974 |
| NaCl USPP | 4x1 | 0.128588 | 0.249268 |
| NaCl USPP | 8x1 | 0.116573 | 0.100644 |
| NaCl USPP | 4x2 | 0.078461 | 0.164641 |
| NaCl USPP | 8x2 | 0.080408 | 0.078755 |
| HCl USPP | 2x1 | 0.045897 | 0.080158 |
| HCl USPP | 4x1 | 0.017171 | 0.043126 |
| HCl USPP | 8x1 | 0.016369 | 0.030988 |
| HCl USPP | 4x2 | 0.011421 | 0.042289 |
| HCl USPP | 8x2 | 0.024140 | 0.054004 |
| Si BLPS | 2x1 | 0.072779 | 0.110603 |
| Si BLPS | 4x1 | 0.033599 | 0.048239 |
| Si BLPS | 8x1 | 0.114980 | 0.064107 |
| Si BLPS | 4x2 | 0.016624 | 0.018041 |
| Si BLPS | 8x2 | 0.089422 | 0.050783 |

- NaCl USPP 的 rank skew 最明显：`2x1` 合计 wait proxy 为 `0.383639s`，约占合计通信 critical path 的 `53.6%`；`4x1` 也接近 `54.6%`。这说明 dense/Sup 通信不只是“数据大”，还存在明显 rank 间耗时差异，后续应重点观察 `PW_Basis_Sup` 的 wait proxy 是否下降。
- Si BLPS 在 `2x1` 和高 rank 下也有可见 skew：`2x1` 的 wait/comm 约 `47.5%`，`8x2` 仍约 `33.6%`。如果非阻塞通信能减少阻塞等待，Si BLPS 应该能提供比 HCl USPP 更敏感的 wait proxy 对比。
- HCl USPP `8x2` 的通信占 wall time 很高，但 wait/comm 只有约 `18.3%`，说明它的瓶颈更像 collective 固定成本或 wave 路径临界时间，而不是单纯的 rank 不均衡。对 HCl USPP，后续优化不能只看 wait proxy，还要看 `comm_critical_s_mean` 本身是否下降。

## 可重叠计算区间

下表为 `overlap_candidate_rank_avg_s_mean`，由 collective 前后的 pack/unpack/clear 本地重排工作组成。非阻塞通信或分块通信应尽量把这部分本地工作安排到通信进行期间。

| 体系 | MPI x OMP | `PW_Basis_K`/s | `PW_Basis_Sup`/s |
| --- | --- | --- | --- |
| NaCl USPP | 2x1 | 0.059000 | 0.446241 |
| NaCl USPP | 4x1 | 0.040572 | 0.273498 |
| NaCl USPP | 8x1 | 0.016785 | 0.151305 |
| NaCl USPP | 4x2 | 0.047423 | 0.150399 |
| NaCl USPP | 8x2 | 0.020411 | 0.094515 |
| HCl USPP | 2x1 | 0.087775 | 0.131820 |
| HCl USPP | 4x1 | 0.047420 | 0.078730 |
| HCl USPP | 8x1 | 0.034663 | 0.050675 |
| HCl USPP | 4x2 | 0.059066 | 0.057555 |
| HCl USPP | 8x2 | 0.043633 | 0.038016 |
| Si BLPS | 2x1 | 0.111546 | 0.333428 |
| Si BLPS | 4x1 | 0.066453 | 0.210668 |
| Si BLPS | 8x1 | 0.026947 | 0.101593 |
| Si BLPS | 4x2 | 0.085781 | 0.113788 |
| Si BLPS | 8x2 | 0.036676 | 0.057809 |

- NaCl USPP `2x1` 的 overlap candidate 合计 `0.505241s`，约为合计通信 critical path 的 `70.6%`；其中单独 `PW_Basis_Sup` 就有 `0.446241s`。这说明该配置最适合验证“先发起通信，再安排 dense/Sup 的本地 pack/unpack/clear 工作”的思路。
- Si BLPS `2x1` 的 overlap candidate 为 `0.444974s`，甚至略高于合计通信 critical path `0.385700s`。它是另一个适合验证重叠策略的低 rank case，尤其适合确认本地重排能否被合理移到通信等待期间。
- 高 rank 下可重叠工作会明显变少：NaCl USPP `8x2` 的 overlap/wall 约 `1.56%`，HCl USPP `8x2` 约 `2.38%`，Si BLPS `8x2` 约 `3.11%`。因此后续非阻塞实现即使通信 timer 有改善，也不应承诺很大的端到端收益；更现实的目标是降低通信临界路径、降低 wait proxy，并在 wall time 上获得超过噪声的稳定小幅收益。
- 如果后续方案只是把 `MPI_Alltoallv` 换成 `MPI_Ialltoallv + Wait`，但中间没有足够独立的本地工作，收益可能非常有限。真正值得尝试的是把 pack/unpack/clear 的顺序和通信请求生命周期一起设计，或进一步做分块收发，让可重叠区间更靠近通信进行期间。

## 综合瓶颈判断

按当前数据，后续优化优先级可以分成三档：

1. `NaCl USPP 2x1/4x1`：dense/Sup 通信绝对时间大，wait proxy 高，可重叠本地工作也多。它最适合证明非阻塞通信和通信/计算重叠是否真的改善了目标路径。
2. `Si BLPS 2x1`：overlap candidate 充足，同时 wait proxy 明显，适合作为第二个验证点，避免只在 NaCl USPP 单一体系上得出结论。
3. `HCl USPP 8x1/8x2` 与 `Si BLPS 8x1/8x2`：通信占 wall time 高，尤其 wave 路径占比高，适合检查优化在高 rank 场景下是否降低同步开销；但这里可重叠本地工作较少，端到端收益可能不会很大。

因此，后续优化更有说服力的证据链应该是：`PW_Basis_K/PW_Basis_Sup` 分路径 timer 改善、wait proxy 改善、overlap candidate 是否被有效隐藏，最后再说明 wall time 是否超过重复性噪声。

## Correctness Guard

性能测试中故意固定 `scf_nmax=10`，并使用极小 `scf_thr=1e-30` 来降低随机性和保证工作量一致，因此不把 timing run 当作物理收敛结果。但正式 correctness guard 现在使用 `scf_nmax=200, scf_thr=1e-13`，对三个体系分别运行 `1x1` 和 `8x2`：

| 体系 | MPI x OMP | `scf_thr` | 状态 | SCF 步数 | final DRHO | final ETOT/eV | wall/s |
| --- | --- | --- | --- | --- | --- | --- | --- |
| NaCl USPP | 1x1 | 1e-13 | converged | 21 | 5.643e-14 | -1671.8402255281855560 | 83.620 |
| NaCl USPP | 8x2 | 1e-13 | converged | 18 | 5.908e-14 | -1671.8402255278688244 | 10.720 |
| HCl USPP | 1x1 | 1e-13 | converged | 16 | 2.848e-14 | -427.5661059206546497 | 18.840 |
| HCl USPP | 8x2 | 1e-13 | converged | 16 | 2.875e-14 | -427.5661059193822666 | 4.440 |
| Si BLPS | 1x1 | 1e-13 | converged | 10 | 2.404e-14 | -216.0149983621668923 | 10.530 |
| Si BLPS | 8x2 | 1e-13 | converged | 10 | 2.403e-14 | -216.0149983622328875 | 3.040 |

并行一致性比较如下。这里比较的是同一 prepared case、同一 cutoff/nbands/SCF 设置下，`1x1` 与 `8x2` 的 final ETOT 差异：

| 体系 | 参考配置 | 对比配置 | 参考 ETOT/eV | 对比 ETOT/eV | abs diff/eV | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| NaCl USPP | 1x1 | 8x2 | -1671.8402255281855560 | -1671.8402255278688244 | 3.167e-10 | pass |
| HCl USPP | 1x1 | 8x2 | -427.5661059206546497 | -427.5661059193822666 | 1.272e-09 | pass |
| Si BLPS | 1x1 | 8x2 | -216.0149983621668923 | -216.0149983622328875 | 6.600e-11 | pass |

结果说明：三体系均在 `scf_thr=1e-13` 下收敛，final DRHO 均已进入 `10^-14` 量级。跨并行配置的 ETOT 差异为 `10^-10 ~ 10^-9 eV`，说明当前计时插桩、绑核策略和大参数 prepared cases 没有破坏数值路径。这里不追求不同 MPI/OpenMP 配置下逐 bit 一致，因为并行归约顺序和 SCF 迭代路径本来可能导致末位差异；报告中应表述为“同参数下达到约 `10^-9 eV` 量级一致性”。

`scf_thr=1e-14` 的尝试记录如下：

| 体系 | MPI x OMP | 状态 | SCF 步数 | final DRHO | final ETOT/eV | wall/s |
| --- | --- | --- | --- | --- | --- | --- |
| NaCl USPP | 1x1 | converged | 22 | 6.145e-15 | -1671.8402255281660018 | 86.770 |
| NaCl USPP | 8x2 | converged | 20 | 4.786e-15 | -1671.8402255278754183 | 11.620 |
| HCl USPP | 1x1 | failed_psi_norm | 16 | 2.848e-14 |  | 18.530 |
| Si BLPS | 1x1 | failed_psi_norm | 11 | 1.253e+00 |  | 18.080 |

`1e-14` 下 NaCl USPP 的 `1x1/8x2` 均收敛，但 HCl USPP 和 Si BLPS 在 `1x1` 下触发 `psi_norm <= 0.0`，日志提示可能与 `npwx < nbands` 或秩亏有关。这个结果说明继续把阈值机械压到 `1e-14` 会引入求解器稳定性风险；若坚持 `1e-14`，应进一步调整 correctness case 的数值设置，例如降低 `nbands`、增加 `ecutwfc`、换更稳定的对角化/初猜设置，或使用 ABACUS 官方测试中已有高精度 reference 的体系。

后续优化版应在相同 guard 输入下保持收敛状态，并以这里的 final ETOT 作为同参数 baseline。不要直接拿性能测试中这些 large 参数下的能量与原始 `tests` 的 `result.ref` 混比，因为本次显式改变了 `ecutrho`、`nbands` 和 SCF 设置。

## 后续优化对比规则

后续阻塞/非阻塞实现比较时应遵守以下规则：

1. 使用同一批 prepared cases、同一 `large:50:2000`、同一 `CONFIGS`、同一 `REPEATS=3`、同一 `scf_nmax=10` 与 `nbands=24`。
2. 保持相同 MPI/OpenMP 绑核策略，或者在报告中明确说明新的绑核策略，并重新跑 baseline；不要混用默认 OpenMPI 绑核数据。
3. 性能结论优先比较 `wall_s_mean`，并要求改善幅度大于重复噪声。建议阈值为 `max(3%, 2 * combined stdev)`。
4. 通信优化结论必须同时查看 `comm_critical_s_mean` 和 `wait_proxy_max_s_mean`；仅 wall time 下降不足以证明通信路径被优化。
5. 非阻塞实现若改变 timer 结构，应保留等价的 critical path、rank skew proxy 和 overlap candidate 指标，保证可横向比较。
6. 每次性能矩阵完成后至少跑 NaCl USPP 或 Si BLPS 的 correctness guard；涉及通信语义改动时建议两个 guard 都跑。

重点推荐的对比点：NaCl USPP `2x1` 用于观察 dense/Sup 通信重叠，NaCl USPP `8x2` 用于观察最佳端到端配置，Si BLPS `8x1/8x2` 用于检查 wave 路径在高 rank 下的同步开销。

预期收益口径也要保守：在当前单节点 large benchmark 中，最佳配置下通信临界路径占 wall time 约 `5.82%` 到 `13.73%`，可重叠本地工作约占 wall time `1.56%` 到 `3.11%`。因此除非优化同时降低 collective 临界路径和 rank skew，否则端到端收益大概率是几个百分点级别；通信 timer 的结构性改善会比单个 wall time 数字更能说明问题。

## 风险与限制

- 本 benchmark 是单节点结果，不能直接外推到跨节点网络通信。
- timing run 固定迭代步数，不用于讨论最终物理精度；correctness guard 才用于收敛性检查。
- `--report-bindings` 会在 `stderr.log` 中留下启动期输出，但相对本次 wall time 可忽略，并提高了审计性。
- 内部 case 子目录统一使用 `large_rep...` 命名；具体体系由外层目录 `nacl_uspp`、`hcl_uspp`、`si_blps` 和汇总表中的体系名区分。

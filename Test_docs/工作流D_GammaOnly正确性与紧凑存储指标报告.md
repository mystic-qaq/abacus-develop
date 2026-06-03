# 工作流 D GammaOnly 正确性与紧凑存储指标报告

日期：2026-05-23

## 结论摘要

本次在 `GammaOnly` 分支完成了规划全 Gamma 与混合 k 点正确性验证，运行当前分支的现状基线，并明确 GammaOnly 紧凑存储前后的对比指标。

当前分支的核心状态是：`module_pw` 内部已有 `PW_Basis` / `PW_Basis_K` 的半谱 FFT 基础，但真实 PW 生产路径尚未启用 GammaOnly。输入层在 `basis_type=pw` 且 `gamma_only=1` 时会把参数重置为 `0`，`setup_pwrho` 与 `setup_pwwfc` 仍硬编码以 `false` 初始化 PW 基组。因此，本报告中的实测数据是当前 full-complex 路径的 correctness baseline，而非 GammaOnly 性能收益数据。

实测覆盖 4 个 case：全 Gamma、规则多 k 点、USPP 多 k 点、显式 Direct 的 Gamma+非 Gamma 混合 k 点。全部 case 在 `np1_omp1` 与 `np4_omp1` 下 SCF 收敛；带官方 `result.ref` 的三个 case 相对参考能量误差均小于 `5.42e-08 eV`；`np1` 与 `np4` 的并行一致性最大能量差为 `4.19e-08 eV`。

## 运行目录与产物

| 项目 | 路径 |
| --- | --- |
| 正式目录 | `/home/yangxu/abacus-develop/benchmarks/workflow_d/gammaonly_correctness_20260523` |
| 运行脚本 | `run_gammaonly_correctness.sh` |
| 汇总脚本 | `summarize_gammaonly_correctness.py` |
| 汇总表 | `tables/correctness_summary.csv`、`tables/parallel_comparison.csv`、`tables/full_complex_baseline_metrics.csv` |
| 原始日志 | `runs/<case>/<config>/OUT.autotest/running_scf.log` |

复现命令：

```bash
bash benchmarks/workflow_d/gammaonly_correctness_20260523/run_gammaonly_correctness.sh
```

## 测试环境

| 项目 | 内容 |
| --- | --- |
| Git branch | `GammaOnly` |
| Git commit | `abdd675d6` |
| ABACUS binary | `build-current-abacus-mpi-local/abacus_pw_para` |
| Build commit | `abdd675d6 (Sat May 23 09:53:30 2026 +0800)` |
| MPI | OpenMPI `4.0.3` |
| CPU | AMD EPYC 7H12，2 sockets，64 cores/socket，256 logical CPUs |
| 绑核策略 | `--map-by slot:PE=${OMP_NUM_THREADS} --bind-to core --report-bindings` |
| OpenMP | `OMP_PLACES=cores`, `OMP_PROC_BIND=spread` |

## 当前代码状态

| 位置 | 当前行为 | 影响 |
| --- | --- | --- |
| `source/source_io/module_parameter/read_input_item_elec_stru.cpp:785` | PW 且 `gamma_only=1` 时重置为 `false`，并生成 `1 1 1` Gamma KPT | PW 生产计算当前无法通过 INPUT 启用 GammaOnly；混合 k 点若强行设置会被覆盖 |
| `source/source_pw/module_pwdft/setup_pwrho.cpp:83` | `pw_rho->initparameters(false, ...)` | 电荷密度/势函数基组仍走 full-complex |
| `source/source_pw/module_pwdft/setup_pwwfc.cpp:56` | `pw_wfc->initparameters(false, ...)` | 波函数基组仍走 full-complex |
| `source/source_basis/module_pw/pw_basis_k.cpp:81` | 若任一 k 点 `kmaxmod > 0`，全局 `gamma_only=false` | 只能表达“全 Gamma”或“全 full-complex”，不能表达 per-k 混合 |
| `source/source_basis/module_pw/pw_transform_k.cpp:34` / `:98` | complex 路径要求 `gamma_only=false`，real Gamma 路径要求 `gamma_only=true` | CPU 侧已有全局 GammaOnly 分支，但混合 k 点需要新的分派层 |

## 测试矩阵

| case | 来源 | KPT | 用途 |
| --- | --- | --- | --- |
| `si_gamma_1x1x1` | `tests/01_PW/013_PW_ONCV_LDA` | `Gamma 1 1 1 0 0 0` | 全 Gamma correctness baseline |
| `si_mixed_direct_gamma_plus_k` | 由 `si_gamma_1x1x1` 改 KPT | Direct 两点：`(0,0,0)` 与 `(0.25,0,0)` | 显式 Gamma+非 Gamma 混合现状验证 |
| `si_multik_2x2x2` | `tests/01_PW/004_PW_UPF201_Si` | `Gamma 2 2 2 0 0 0` | 规则多 k 点 baseline |
| `nacl_multik_1x1x2` | `tests/01_PW/008_PW_UPF201_USPP_NaCl` | `Gamma 1 1 2 0 0 0` | USPP、多 k 点、force/stress baseline |

## Correctness 结果

| case | config | final ETOT/eV | ref diff/eV | final DRHO | wall/s |
| --- | --- | ---: | ---: | ---: | ---: |
| `si_gamma_1x1x1` | `np1_omp1` | -196.4803276757865 | 1.222e-08 | 9.859e-11 | 1.02 |
| `si_gamma_1x1x1` | `np4_omp1` | -196.4803276338421 | 5.416e-08 | 1.469e-10 | 1.02 |
| `si_mixed_direct_gamma_plus_k` | `np1_omp1` | -201.5730386213709 | N/A | 2.985e-10 | 1.04 |
| `si_mixed_direct_gamma_plus_k` | `np4_omp1` | -201.5730386222985 | N/A | 1.942e-11 | 1.01 |
| `si_multik_2x2x2` | `np1_omp1` | -261.1123684719249 | 8.998e-09 | 7.354e-10 | 1.02 |
| `si_multik_2x2x2` | `np4_omp1` | -261.1123684719171 | 9.006e-09 | 7.354e-10 | 1.09 |
| `nacl_multik_1x1x2` | `np1_omp1` | -1229.8316890476135 | 1.114e-11 | 7.399e-11 | 1.90 |
| `nacl_multik_1x1x2` | `np4_omp1` | -1229.8316890476028 | 4.547e-13 | 7.498e-11 | 1.62 |

并行一致性：

| case | abs(`np1` - `np4`)/eV | 状态 |
| --- | ---: | --- |
| `si_gamma_1x1x1` | 4.194e-08 | pass |
| `si_mixed_direct_gamma_plus_k` | 9.276e-10 | pass |
| `si_multik_2x2x2` | 7.844e-12 | pass |
| `nacl_multik_1x1x2` | 1.069e-11 | pass |

结论：当前 full-complex 路径在全 Gamma、规则多 k 点和显式混合 k 点下均可稳定收敛，MPI 配置变化没有引入可见数值回归。后续 GammaOnly 改造至少应保持这组 baseline 的收敛状态，并满足 `1e-6 Ry` 量级的能量一致性。

## Full-Complex Baseline 指标

我们从当前日志生成了 `tables/full_complex_baseline_metrics.csv`。其中 `npw/stick/FFT grid/timer/wall_s/memory_total_mb_reported` 是日志直接值；`rho_complex_bytes_est/v_complex_bytes_est/psi_complex_bytes_est` 是按 full-complex double 复数 `16 bytes` 估算的结构性存储量，用于后续和 compact 侧同口径比较。

| case | rho npw | dense/V npw | wfc npw | charge grid | dense grid | wfc grid | rho bytes est | V bytes est | psi bytes est |
| --- | ---: | ---: | ---: | --- | --- | --- | ---: | ---: | ---: |
| `si_gamma_1x1x1` | 3143 | N/A | 411 | 24x24x24 | 24x24x24 | 24x24x24 | 50288 | 50288 | 39456 |
| `si_mixed_direct_gamma_plus_k` | 3143 | N/A | 531 | 24x24x24 | 24x24x24 | 24x24x24 | 50288 | 50288 | 50976 |
| `si_multik_2x2x2` | 3143 | N/A | 609 | 24x24x24 | 24x24x24 | 24x24x24 | 50288 | 50288 | 58464 |
| `nacl_multik_1x1x2` | 283 | 3119 | 65 | 9x9x9 | 24x24x24 | 9x9x9 | 4528 | 49904 | 12480 |

由于还未实现真正的 GammaOnly，下面的指标用于后续对照和 sanity check，不作为优化收益结论。当前 performance baseline 如下：

| case | config | wall/s | timer total/s | memory MB | Sup r2g/s | Sup g2r/s | K r2g/s | K g2r/s |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `si_gamma_1x1x1` | `np1_omp1` | 1.02 | 0.25 | 2.075 | 0.01 | 0.00 | 0.02 | 0.02 |
| `si_gamma_1x1x1` | `np4_omp1` | 1.02 | 0.21 | 2.152 | N/A | N/A | 0.01 | 0.01 |
| `si_mixed_direct_gamma_plus_k` | `np1_omp1` | 1.04 | 0.28 | 2.125 | 0.01 | 0.01 | 0.04 | 0.05 |
| `si_mixed_direct_gamma_plus_k` | `np4_omp1` | 1.01 | 0.20 | 2.202 | 0.00 | 0.00 | 0.01 | 0.02 |
| `si_multik_2x2x2` | `np1_omp1` | 1.02 | 0.31 | 2.126 | 0.00 | 0.00 | 0.03 | 0.04 |
| `si_multik_2x2x2` | `np4_omp1` | 1.09 | 0.25 | 2.208 | N/A | N/A | 0.01 | 0.01 |
| `nacl_multik_1x1x2` | `np1_omp1` | 1.90 | 1.15 | 3.1132 | 0.04 | 0.03 | N/A | N/A |
| `nacl_multik_1x1x2` | `np4_omp1` | 1.62 | 0.76 | 3.3082 | 0.02 | 0.01 | 0.01 | 0.01 |

我们仍不能从日志直接得到 `gather_send_bytes/gather_recv_bytes`，表中暂记为 `not_instrumented`；`pack_time_s/unpack_time_s/expanded_tmp_bytes` 在 full-complex 路径中不存在，暂记为 `N/A`。后续 compact 实现后，应在同一 CSV 字段下补 compact 侧数据。

## 全 Gamma 验证设计

全 Gamma 是第一阶段推荐验收范围：仅当所有 k 点均为 Gamma、CPU/MPI 后端、`nspin!=4`、非 SOC 时启用 PW GammaOnly。

| 验证层级 | 测试内容 | 通过标准 |
| --- | --- | --- |
| 模块 round-trip | `PW_Basis` / `PW_Basis_K` 在 `gamma_only=true`、`xprime=true/false` 下执行 `recip2real -> real2recip` | expanded 后 double 最大误差 `<=1e-10`，float 最大误差 `<=1e-5` |
| 半谱映射 | 对比 full G 空间与 compact 的 `ig2isz/igl2isz_k/st_length2D/st_bottom2D` | compact 展开后覆盖 full 独立谱；自共轭边界不重复计权 |
| 生产 SCF | `si_gamma_1x1x1`、Si/NaCl 更大 Gamma case，分别跑 `gamma_only=0/1` | `abs(ETOT_gamma - ETOT_full) <= 1e-6 Ry` |
| 并行一致性 | `np1_omp1`、`np4_omp1`，后续扩展到 `np8_omp1/np8_omp2` | 同一 gamma 设置下 `abs(ETOT_np1 - ETOT_npN) <= 1e-6 Ry` |
| force/stress | NaCl USPP 或 Si force/stress case | force max diff `<=1e-5 eV/Angstrom`，stress max diff `<=1e-3 kbar` |
| 负向 guard | `nspin=4`、SOC、GPU/DSP 后端 | 明确 fallback 或报错，不能静默走错误半谱路径 |

## 混合 k 点验证设计

混合 k 点建议分两阶段。

第一阶段是安全 fallback：当 k 点集合含 Gamma 与非 Gamma 点时，程序保持 full-complex 路径，保留用户 KPT，并输出降级原因。当前 `PW_Basis_K` 层因为 `kmaxmod > 0` 会关闭全局 `gamma_only`，数值上安全；但输入层对 PW 的 `gamma_only=1` 会覆盖 KPT，后续需要改成保留 KPT 并降级。

第二阶段是 per-k 混合：引入 `is_gamma_k[ik]`。Gamma k 点走 r2c/c2r 与 compact layout，非 Gamma k 点走 complex FFT 与 full layout。

| 场景 | case | 预期行为 | 对比对象 |
| --- | --- | --- | --- |
| 显式 Direct 混合 | `si_mixed_direct_gamma_plus_k` | `ik=0` 标为 Gamma，`ik=1` 标为 non-Gamma | full-complex baseline |
| 规则多 k 点 | `si_multik_2x2x2` | 若含 Gamma 点则单独 compact，其余 complex | full-complex baseline |
| USPP 多 k 点 | `nacl_multik_1x1x2` | 检查 rho/V/augmentation charge、force、stress | full-complex baseline |
| 非适用条件 | SOC / `nspin=4` / GPU-DSP | fallback 或拒绝 | 当前 full-complex baseline |

混合实现必须输出或可汇总：`num_gamma_k`、`num_non_gamma_k`、`is_gamma_k[ik]`、每类 k 点的 `npwk`、以及 fallback reason。

## 紧凑存储对比指标

优化后报告应有以下指标，与“理论节省 50% ”对比。

| 类别 | 指标 | 定义 |
| --- | --- | --- |
| G 空间规模 | `npw_full`, `npw_compact`, `npwtot_full`, `npwtot_compact` | full 与 compact 的本地/全局平面波数量 |
| stick 规模 | `nst_full`, `nst_compact`, `nstot_full`, `nstot_compact` | 本地/全局 stick 数 |
| FFT 网格 | `nx,ny,nz`, `fftnx,fftny,fftnz` | compact 后半谱方向应为 `nx/2+1` 或 `ny/2+1` |
| 存储字节 | `bytes_psi_full/compact` | 按 `nks_gamma * nbands * npw * sizeof(complex)` 统计，混合 k 点按类别求和 |
| 存储字节 | `bytes_rho_full/compact`, `bytes_v_full/compact` | density/potential reciprocal layout 的实际数组字节 |
| 峰值内存 | `bytes_expanded_tmp`, `peak_bytes_full/compact` | pack/unpack 或旧接口 expanded view 的临时量必须计入 |
| 数据搬移 | `gather_send_bytes`, `gather_recv_bytes` | 由 `numg/numr` 与元素大小推导，分 `PW_Basis_K` 与 `PW_Basis_Sup` |
| 计时 | `setuptransform`, `distribute_g`, `real2recip`, `recip2real` | 保持 full/compact 可比 |
| 正确性 | `etot_diff_ry`, `force_max_diff`, `stress_max_diff`, `rho_g_l2_diff`, `psi_max_diff` | compact 展开后相对 full baseline 的误差 |
| 实际收益 | `compact_ratio`, `memory_saving_percent` | `npw_compact / npw_full` 与 `1 - bytes_compact / bytes_full` |

建议分别计算：

```text
psi_compact_ratio = bytes_psi_compact / bytes_psi_full
rho_compact_ratio = bytes_rho_compact / bytes_rho_full
v_compact_ratio   = bytes_v_compact   / bytes_v_full
peak_ratio        = peak_bytes_compact_path / peak_bytes_full_path
```

`peak_ratio` 尤其重要。如果为了兼容旧接口长期保留 expanded temporary，峰值内存可能明显低于理论收益，需要如实说明。

## 后续执行顺序

1. 先补 compact pack/unpack round-trip，覆盖 `xprime=true/false` 和自共轭边界。
2. 修改输入层与 `setup_pwrho/setup_pwwfc`，仅在全 Gamma 且物理/后端条件安全时启用 PW GammaOnly。
3. 用本报告的 `si_gamma_1x1x1` 做第一条生产 SCF guard，再加入 NaCl/Si 大一点的 force/stress case。
4. 全 Gamma 通过后，再推进 mixed per-k 分派；在此之前混合 k 点必须明确 fallback，不能静默覆盖 KPT。
5. 最终性能报告加入 compact ratio、实际字节数、gather/scatter 字节数和 FFT timer 对比。

## 风险与限制

- 本次实测是单节点小用例，目标只是得到 correctness smoke 以及基础 baseline，不用于评估性能收益。
- 当前生产路径未启用 PW GammaOnly，因此没有 compact memory saving 的实测数字。
- Direct 混合 k 点 case 是本 benchmark 自建用例，没有官方 `result.ref`，只用于并行一致性和后续 full-complex baseline。
- `BUILD_TESTING` 当前为 OFF，本轮没有重新构建 `MODULE_PW_pw_test`；后续若要覆盖半谱内部 round-trip，应单独启用测试构建或新增轻量 test target。

## AI 使用报告

- 关于 GammaOnly 这条分支的测试，起初 GPT 仅测试了代码的正确性，之后在人为提醒下又测试了一些已经可测的性能作为 baseline 。同时，它也反过来提醒我，目前能做的仅是full-complex 静态规模 baseline，在compact未实现前，大部分指标仍处于无法测试的状态。
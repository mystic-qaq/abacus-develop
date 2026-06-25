# 平面波生成与应用并行优化项目总结

日期：2026-06-26

交付分支：`final`

交付提交：`47d3ce35f Restore PW cache compatibility after task integration`

主要基线：`upstream/develop == origin/develop == 777f50c9c`

项目说明来源：`01_plane_wave.md`

## 1. 项目背景与物理原理理解

本项目围绕 ABACUS 平面波模块的生成、FFT 变换、MPI 数据交换、GammaOnly 存储和缓存复用展开优化。平面波方法的基本出发点是把周期体系中的 Bloch 波函数写成

```text
psi_k(r) = sum_G c_{k,G} exp(i (k + G) dot r)
```

其中 `k` 是布里渊区采样点，`G` 是倒格矢。ABACUS 通过能量截断 `|G+k|^2 <= Ecut` 选取有限数量的平面波，利用 FFT 在实空间和倒空间之间切换。平面波基组的优势是形式统一、FFT 友好、收敛控制直接；代价是数据量随 FFT 网格、截断能和 k 点数快速增长，因此初始化、FFT 拷贝、gather/scatter 通信和重复几何量构造都容易成为热点。

Gamma 点有特殊性。当所有 k 点都是 Gamma 点且实空间函数为实函数时，傅里叶系数满足 Hermitian 共轭关系：

```text
F(-G) = conj(F(G))
```

因此理论上只需要存储一半的倒空间系数，使用 real-to-complex / complex-to-real FFT 路径即可恢复完整物理信息。但这一优化不能无条件使用：混合 Gamma 与非 Gamma k 点、自旋轨道耦合或非实输入都会破坏直接半谱假设。因此本项目中的 GammaOnly 优化重点不是“强行少存一半”，而是在可证明安全的 all-Gamma 场景启用，在混合场景回退，并在所有内积和投影中正确处理自共轭 G 点与普通 G/-G 对的权重。

## 2. 原代码实现理解

ABACUS 的平面波模块主要集中在 `source/source_basis/module_pw/`：

- `PW_Basis`：单 k 点或 charge/potential 相关平面波基组，维护 FFT 网格、G 向量、stick 分布和 transform 工作区。
- `PW_Basis_K`：多 k 点波函数平面波基组，维护 `kvec_d/kvec_c`、每个 k 点的 `npwk`、`igl2isz_k`、`igl2ig_k` 等映射。
- `pw_distributeg.cpp`：统计满足截断条件的 G 点，生成 stick 长度和分布。
- `pw_transform.cpp` / `pw_transform_k.cpp`：实现 `real2recip` 与 `recip2real`，包括 XY FFT、MPI 重排、Z FFT 和 G 系数提取。
- `pw_gatherscatter.h`：在 plane 布局和 stick 布局之间重排数据，并在 MPI pool 内交换数据。

原始流程可以概括为：

1. `initgrids()` 根据晶格和截断能确定 FFT 网格。
2. `initparameters()` 设置平面波截断、GammaOnly、k 点和分布策略。
3. `count_pw_st()` 扫描三维倒空间候选点，统计每个 `(ix, iy)` stick 上满足截断的 `iz` 数。
4. `distribute_g()` 把 sticks 分配给不同 MPI rank。
5. `get_ig2isz_is2fftixy()` 建立平面波索引、stick 索引和 FFT 网格索引之间的映射。
6. FFT transform 中先做本地拷贝和 XY FFT，再通过 `gatherp_scatters` / `gathers_scatterp` 交换 plane/stick 数据，最后做 Z FFT 和系数提取。

这套实现已经具备 MPI + OpenMP 基础并行，但项目文档指出了几类可优化点：

- `count_pw_st` 三重循环适合线程并行。
- FFT 前后数据拷贝和重排是内存带宽型热点。
- `MPI_Alltoallv` 是阻塞通信，通信等待期间难以隐藏延迟。
- gather/scatter 内部连续拷贝可做 SIMD 化。
- GammaOnly 在多 k 点路径尚未完整支持。
- `G`、`gcar`、`gk2`、`gg_uniq` 等几何量在 SCF 中可缓存复用。

## 3. 总体实现原则

本组最终采用了保守、可验证的优化策略：

- 不改变物理模型和公开调用语义，优先优化内部数据搬运、缓存和安全回退。
- 已被上游接收的贡献不在 `final` 中重复制造差异，但在总结中保留为本项目成果。
- 对风险较高的路径设置显式 fallback，例如混合 k 点回退 full-complex，单通信块回退阻塞 Alltoallv。
- MPI 调用增加错误检查，避免通信失败后静默产生错误结果。
- 缓存和通信缓冲区避免高风险 `mutable` 共享状态，最终 `final` 中 `PW_Basis` cache 使用 RAII 和显式失效。
- SIMD helper 按编译器/架构能力选择 AVX2/AVX512 或标量 fallback，保持可移植性。
- 所有性能结论都配合 correctness 表述，避免把噪声或特定 workload 误写成普遍加速。

## 4. 各题实现总结

### 4.1 题目 1：`count_pw_st` 的 OpenMP 并行化

题目要求优化 `pw_distributeg.cpp` 中 `count_pw_st` 的三重循环。该函数扫描候选 G 点并统计 `npwtot`、`nstot`、`st_length2D`、`st_bottom2D` 和边界信息。

最终成果已经进入上游：

- `5d2582d72 Perf: parallelize count_pw_st with OpenMP collapse(2) (#7438)`

实现要点：

- 对外层 stick 扫描引入 OpenMP 并行，当前基线代码中保留了 `#pragma omp parallel for collapse(1)` 和 reduction。
- `npwtot_local`、`nstot_local` 使用加法归约，`rix/riy/lix/liy` 使用 min/max 归约。
- 每个 `(ix, iy)` 对应唯一 `st_length2D[index]` 和 `st_bottom2D[index]`，因此数组写入不存在跨线程冲突。
- `iz` 方向保持串行扫描，保证 `st_bottom2D` 仍记录该 stick 上第一个满足截断条件的 `iz`。
- 相关分支文档分析了 `collapse(2)` 和 `collapse(1)` 的取舍：更激进的二维 collapse 有更大并行粒度，但最终上游版本采用了更稳妥的形式。

预期效果：

- 缩短平面波初始化阶段的截断球扫描时间。
- 在大 FFT 网格和较高截断能下收益更明显。
- 不改变 G 点选择，因此不引入浮点归约顺序之外的物理差异。

规范性、鲁棒性与可移植性：

- OpenMP 宏保护，未启用 OpenMP 时仍可串行编译。
- reduction 明确表达共享变量依赖。
- 不改变 `count_pw_st` 接口和后续分布逻辑。

测试与结果：

- 上游 PR `#7438` 已接收，说明该优化通过了上游代码审查和测试流程。
- 本次 `final` 验收中，`MODULE_PW_pw_test --gtest_filter='PWTEST.*'` 47 个测试通过，覆盖平面波分布、GammaOnly、full_pw 和 transform 组合。

### 4.2 题目 2：MPI gather/scatter 非阻塞优化

题目要求把 `pw_gatherscatter.h` 中阻塞 `MPI_Alltoallv` 改造成非阻塞通信，减少通信等待。

分支与报告：

- 分支：`pr/nonblocking-mpi`
- 报告：`Test_docs/task2_nonblocking_mpi_validation.md`

实现要点：

- `gatherp_scatters` 和 `gathers_scatterp` 保持原有调用接口与输入输出语义。
- MPI-3 环境优先使用 `MPI_Ialltoallv`。
- 老 MPI 环境保留 `MPI_Irecv` / `MPI_Isend` fallback。
- 使用 `detail::mpi_complex_dtype<T>()` 做编译期类型分发，避免运行期 `typeid` 分支。
- 所有 MPI 调用通过 `detail::check_mpi()` 检查返回码，失败时给出可定位错误信息。
- 去掉早期隐藏 work-buffer 方案，保留调用者 buffer 数据流，避免额外大块持久通信空间。

预期效果：

- 在通信占比较大的 PW case 中减少阻塞等待。
- 对小 case 不保证明显加速，但应保持正确性和内存语义清晰。

规范性、鲁棒性与可移植性：

- MPI-3 和老 MPI 都有路径。
- 不依赖 `mutable` 隐藏状态。
- 对 `float` 和 `double` 明确匹配 MPI complex datatype。

分支测试结果：

| Case | MPI ranks | Baseline energy (eV) | Task2 energy (eV) | Abs diff |
| --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/022_PW_CG` | 4 | `-198.2238296277166` | `-198.2238296277166` | `0.0` |
| `tests/01_PW/036_PW_AF` | 4 | `-5866.197297493522` | `-5866.197297493522` | `0.0` |
| `tests/01_PW/089_PW_get_wf_kpar` | 3 | `-211.8789253814144` | `-211.8789253814144` | `0.0` |

性能结果：

- 小规模 `022_PW_CG` 在 `np=2/4/8/12/16` 上总体中性或轻微波动。
- 更重的 `tests/performance/P010_si2_pw` 在 `np=8` 下：
  - baseline：`72.71 s`
  - task2：`72.49 s`
  - 能量完全一致，日志确认 `gathers_ialltoallv` / `gatherp_ialltoallv` 激活。

### 4.3 题目 3：FFT 变换数据拷贝与重排的 OpenMP/cache 优化

题目要求优化 `pw_transform.cpp` 中 FFT 变换相关的数据拷贝和重排循环。

部分成果已经进入上游：

- `128d8d8d4 Refine complex buffer copies and add round-trip tests for module_pw (#7412)`
- `f4af81009 perf(pw_basis): optimize FFT data reordering with memcpy SIMD vectori... (#7432)`
- `d05769ab7 Perf: OpenMP cache blocking and SIMD for PW_Basis FFT transform copy routines (#7439)`

最终代码中的实现要点：

- `pw_transform.cpp` 中引入 `pw_transform_cache_block = 128` 和 `block_end()`。
- 连续拷贝循环按块处理，并配合 `#pragma omp parallel for schedule(static)` 与 `#pragma omp simd`。
- 对 `nrxx`、`npw`、`nxyz`、`ig2isz` 等成员提前缓存到局部变量，降低循环内成员访问和别名分析压力。
- 覆盖 complex-to-complex 路径和 GammaOnly real-to-complex / complex-to-real 路径。
- 新增 `test_transform_omp.cpp`，验证多线程 transform 的 round-trip 一致性。

预期效果：

- 提升 FFT transform 前后内存搬运效率。
- 在大网格、多线程场景下改善 cache 行为和向量化概率。
- 不改变 FFT 数学过程，理论上结果应逐元素一致或在严格容差内一致。

规范性、鲁棒性与可移植性：

- OpenMP 和 SIMD 指令均有宏保护。
- 尾块通过 `block_end()` 处理，避免越界。
- 对 GammaOnly 和 full-complex 两条路径分别处理，避免把复数输入错误丢虚部。

分支/上游测试结果：

- `transform_omp_threads_complex_roundtrip_consistency`：通过，最大误差 `0`。
- `transform_omp_threads_real_gamma_and_add_consistency`：通过，最大误差 `0`。
- 小网格、奇数/非整除尾块、大网格线程一致性均通过。
- 性能测试中，256^3 网格、20 次重复的 disabled benchmark 显示：
  - 1 线程：`29.34 s`
  - 8 线程：`4.53 s`，约 `6.48x`
  - 16 线程：`3.71 s`，约 `7.91x`

### 4.4 题目 4：`PW_Basis_K` 多 k 点 GammaOnly 完整实现

题目要求让 `PW_Basis_K` 在 all-Gamma 多 k 点场景下真正启用 GammaOnly 半谱路径，并保证混合 k 点安全。

分支与报告：

- 分支：`GammaOnly`
- 报告：`Test_docs/task4_gammaonly_validation.md`

实现要点：

- `PW_Basis_K::initparameters()` 使用 `gamma_k_tolerance = 1e-12` 判断每个 k 点是否为 Gamma。
- 只有当 `gamma_only_in == true` 且所有 k 点都是 Gamma 时，`PW_Basis_K::gamma_only` 才保持 true。
- 任意非 Gamma k 点都会安全回退 full-complex，避免半谱错误应用到混合 k 点。
- `setuptransform()` 中初始化 `GammaCompact`，基于真实 `G` 与 `-G` 关系构造自共轭标记和权重。
- `setupIndGk()` 记录每个 `igl` 的 Gamma compact inner-product weight。
- `PW_Basis_K::real2recip/recip2real` 支持 real GammaOnly 路径；complex real-space input 若有非零虚部则显式拒绝。
- `ElecStatePW::cal_becsum`、`Charge_Mixing`、`DiagoCG`、`DiagoIterAssist`、`HSolverPW` 等路径补充 Gamma compact 权重，避免简单 `*2.0` 造成 `G=0` 或自共轭点过计数。

预期效果：

- all-Gamma 多 k 点保留半谱 FFT 和半谱波函数存储，降低 `npw/npwk/npwx`。
- 混合 k 点不误启用半谱，保证结果正确。
- 内积、USPP augmentation、CG/subspace diagonalization 与 full-complex 语义保持一致。

规范性、鲁棒性与可移植性：

- 不按数组下标假设 G/-G 配对，而是通过 `GammaCompact` 从真实 FFT/G 映射建立关系。
- 对非实输入显式报错，不静默丢弃虚部。
- 单元测试覆盖 all-Gamma 激活、mixed-k 回退和 complex input 拒绝。

测试结果：

| 测试 | 结果 |
| --- | --- |
| `AllGammaMultiKUsesHalfSpectrum` | 通过 |
| `MixedGammaAndNonGammaFallsBackToFullComplex` | 通过 |
| `GammaRealForwardMatchesFullComplex` | 通过 |
| `GammaProjectedInverseMatchesFullComplex` | 通过 |
| `GammaComplexRealSpaceInputRejectsNonRealData` | 通过 |

正确性：

- 固定回归算例 `022_PW_CG`、`036_PW_AF`、`089_PW_get_wf_kpar` 与 baseline 打印精度一致。
- all-Gamma repeated-Gamma 高精度审计中，在匹配 `scf_thr=1e-12` 和 `pw_diag_thr=1e-12` 后：
  - `np=1` full-complex vs GammaOnly 差异约 `2.186e-10 eV`
  - `np=4` full-complex vs GammaOnly 差异约 `2.122e-10 eV`

内存效果：

| Mode | MPI ranks | Total plane waves | `npwx` |
| --- | ---: | ---: | ---: |
| full-complex | 1 | `12627` | `12627` |
| GammaOnly | 1 | `6603` | `6603` |
| full-complex | 4 | `12627` | `3157` |
| GammaOnly | 4 | `6603` | `1652` |

即总平面波数和 `npwx` 均下降约 `47.7%`。

风险说明：

- 该实现满足 `1e-9 eV` 最低验收目标，但 repeated-Gamma case 没有稳定证明 `1e-14 eV`。这主要与 SCF 收敛和退化路径有关，不应在 PR 中声称已经达到 `1e-14`。

### 4.5 题目 5：gather/scatter SIMD 向量化

题目要求优化 `pw_gatherscatter.h` 中的 pack/unpack 数据重排循环，使用 SIMD 提升连续拷贝效率。

相关分支与文档：

- 分支：`feat/SIMD`
- 文档：`homework_docs/Task5_SIMD_optimization_report.md`
- 上游相关 PR：`#7412`、`#7432`

最终实现要点：

- 新增 `source/source_basis/module_pw/pw_simd_copy.h`。
- 提供 `ModulePW::simd_copy_n<T>(dest, src, count)`，把 `std::complex<T>` 的连续存储视为 `2 * n_complex` 个标量复制。
- AVX512、AVX2、标量三类实现按编译宏选择。
- AVX512 默认可通过宏禁用，避免部分 CPU 512-bit 指令降频导致负收益。
- 使用 unaligned load/store，避免假设 `std::vector` 或外部 buffer cache-line 对齐。
- `gatherp_scatters` 和 `gathers_scatterp` 的 serial/self-copy/pack/unpack 路径调用 `simd_copy_n`。

预期效果：

- 在 MPI 通信前后的本地连续复制阶段减少内存搬运时间。
- 与题 2/7 的非阻塞通信不冲突，可以作为底层拷贝 helper 复用。

规范性、鲁棒性与可移植性：

- `static_assert` 限制只支持 `float` 和 `double`。
- 无 AVX 支持时自动回退标量循环。
- 不改变 MPI 交换数据量和布局语义。

测试结果：

- `feat/SIMD` suite 使用 `gaas_small`、`gaas_medium`、`gaas_large`，MPI `1/2/4`，OpenMP `1/2/4`，共 27 个配置。
- baseline 和 SIMD 两套 suite 均 `27/27` 成功，每组 3 次 repeat。
- 性能中位数多数持平，部分配置有小幅收益，例如：
  - `gaas_medium np=4 omp=4`：speedup `1.125`
  - `gaas_large np=2 omp=4`：speedup `1.062`
  - `gaas_large np=1 omp=1`：speedup `1.054`

结论：题 5 更适合描述为“低风险、可移植的拷贝内核优化”，在部分大 case 有收益，在总 wall time 上不保证普遍显著提升。

### 4.6 题目 6：GammaOnly 紧凑存储

题目要求为 GammaOnly 下的电荷密度、势函数和波函数设计紧凑存储，利用 `F(-G)=conj(F(G))` 减少内存。

相关分支：

- `WorkflowA-q6`
- 关键提交：`f6fef9871 add gamma only storage compact and relative test`

最终实现要点：

- 新增 `CompactGammaData<T>`，用于独立表示 logical full G-space 与 compact representative 之间的关系。
- 支持显式 `minus_g_index` 映射，也支持单元测试使用的 half-by-index 默认映射。
- 提供 `compress_from()`、`decompress_to()`、`memory_saving_ratio()` 等接口。
- 新增 `GammaCompact`，面向 `PW_Basis` 的真实半谱布局：
  - 识别自共轭 G 点。
  - 记录 `conjugate_weight`，普通 G 权重为 2，自共轭 G 权重为 1。
  - 支持 compact/full 之间 expand/pack，用于验证或需要 full spectrum 的路径。
- `final` 中没有把所有分支原型接口都暴露为生产路径，而是把 `GammaCompact` 权重和 half-spectrum helper 与题 4 的多 k GammaOnly 生产路径结合。

预期效果：

- 在 GammaOnly 场景下把 full G-space 的冗余关系显式建模。
- 为题 4 的加权内积、混合回退和 full/compact 对照提供基础设施。
- 对需要保留倒空间数据的场景提供约 50% 理论存储下降空间。

规范性、鲁棒性与可移植性：

- `CompactGammaData` 使用 `std::vector` 管理存储和映射。
- 非法 index、空指针压缩/解压等情况抛出标准异常。
- 不依赖特定 SIMD/MPI 环境。

测试结果：

- `WorkflowA-q6` 的 `test_gamma_compact.cpp` 覆盖：
  - 显式 `minus_g_index` 下压缩/解压。
  - 奇偶 logical size。
  - `G=0` 自共轭虚部强制为 0。
  - compact size 和 memory saving ratio。
  - 无本地 `-G` partner 时 fallback 行为。
- `final` 中通过 `PW_Basis_K` GammaOnly serial tests 和 `PWTEST.*` 继续覆盖实际生产路径。

### 4.7 题目 7：FFT 通信与计算 overlap

题目要求在 FFT transform 通信阶段实现双缓冲 overlap。

分支与报告：

- 分支：`pr/fft-transform-overlap`
- 报告：`Test_docs/task7_fft_overlap_validation.md`

实现要点：

- `gatherp_scatters` 和 `gathers_scatterp` 在多块场景下按 block 切分 sticks。
- 为两个 block 准备双缓冲：当前 block 非阻塞通信时准备下一 block，本 block 通信完成后立即 unpack。
- MPI-3 使用 block-level `MPI_Ialltoallv`；老 MPI 保留 `MPI_Irecv` / `MPI_Isend` fallback。
- `overlap_block_sticks()` 根据 `nst` 和 block span 选择 block 大小，避免固定大缓冲。
- 单通信块 case 没有可 overlap 的“下一块”，因此显式走 `gatherp_single_block_fallback` / `gathers_single_block_fallback`。
- 延迟分配双缓冲，避免单块 fallback 也分配大工作区。

预期效果：

- 多块通信时隐藏部分 MPI 等待时间。
- 单块小 case 保持接近 upstream 的阻塞路径，不为 overlap 付出额外内存成本。

规范性、鲁棒性与可移植性：

- MPI-3 和老 MPI 均支持。
- MPI 错误统一检查。
- block buffer 使用局部 `std::vector`，生命周期明确。
- timer 名称区分 `*_single_block_fallback` 和 `*_overlap_comm`，便于验证实际路径。

正确性结果：

| Case | MPI ranks | Baseline energy (eV) | Task7 energy (eV) | Abs diff |
| --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/022_PW_CG` | 4 | `-198.2238296277166` | `-198.2238296277166` | `0.0` |
| `tests/01_PW/036_PW_AF` | 4 | `-5866.197297493522` | `-5866.197297493522` | `0.0` |
| `tests/01_PW/089_PW_get_wf_kpar` | 3 | `-211.8789253814144` | `-211.8789253814144` | `0.0` |

性能结果：

- `022_PW_CG` 小 case 在 `np=2/4/8/12/16` 上整体中性，部分 rank 略快或略慢。
- `P010_si2_pw np=8`：当前 delayed-buffer fallback 与 baseline 基本持平：
  - baseline `73.19 s`
  - task7 delayed-buffer fallback `73.22 s`
- 合成高截断 multi-block case `004_PW_UPF201_Si, ecutwfc=1500, scf_nmax=1, np=2`：
  - baseline：`14.77 s`
  - task7 block-buffer overlap：`14.47 s`
  - 约 `2.0%` speedup，能量打印一致，日志观察到 `gatherp_overlap_comm`。

风险说明：

- 已证明 multi-block overlap 可工作且有小幅收益，但本地可完成的真实生产 case 多数仍是单块 fallback。更大规模集群/MPI 栈上仍需复测。

### 4.8 题目 8：平面波预计算与缓存复用

题目要求识别 SCF 中可复用的平面波几何数据，加入懒加载缓存和失效机制。

相关分支与文档：

- 分支：`feat/cache-reuse`
- 文档：`Revise_docs/report08_cache_reuse.md`
- 最终修复提交：`47d3ce35f Restore PW cache compatibility after task integration`

最终实现要点：

- `PW_Basis` 新增 `CacheStats`，包括：
  - `local_pw_hits`
  - `local_pw_misses`
  - `uniqgg_hits`
  - `uniqgg_misses`
  - `cache_bytes`
- `collect_local_pw()` 缓存 `gg`、`gdirect`、`gcar`。
- `collect_uniqgg()` 缓存 `ig2igg`、`gg_uniq`，并优先复用已经存在的 `gg`。
- 缓存存储使用 `std::unique_ptr<T[]>`，析构时通过 `clear_owned_cache()` 统一释放。
- `invalidate_cache()` 在 `initmpi()`、`initgrids()`、`initparameters()`、`setfullpw()` 和 `get_ig2isz_is2fftixy()` 等状态变化点调用。
- cache signature 包含 lattice、FFT grid、`npw` 和 `G/GT/GGT`，避免状态改变后误命中。
- 最终修复中恢复了任务 8 测试需要的 `reset_cache_stats()` 和 `get_cache_stats()`，同时避免使用 `mutable`。
- `MODULE_PW_cache_bench` 中为 benchmark 对象补充 MPI 初始化，修复无效 communicator 问题。

预期效果：

- 减少 SCF 中重复构造 G 相关数组、去重 `|G|^2` 和排序映射的开销。
- 提供 cache hit/miss 和缓存字节数，便于后续定位真实收益。

规范性、鲁棒性与可移植性：

- RAII 管理缓存，不手写裸 `delete[]` 所有权。
- 缓存失效集中化。
- 使用 mutex 保护构建/失效路径，计数器使用 atomic。
- 不依赖 GPU/MPI 特定行为，MPI benchmark 显式初始化 communicator。

测试结果：

- `MODULE_PW_cache_bench` 当前运行结果显示：
  - `collect_local_pw_cache_hit.calls = 2000`
  - `collect_uniqgg_cache_hit.calls = 2000`
  - build 与 MPI 运行均通过。
- 历史 task8 suite 使用 `gaas_small`、`gaas_medium`，MPI `1/2/4`，OpenMP `1/2/4`，共 18 组 baseline/cache 对比。
  - 1 组更快，13 组持平，4 组略慢。
  - speedup 范围 `0.833` 到 `1.111`，中位数 `1.0`。

结论：题 8 当前价值主要是机制建设、正确性和可观测性；wall time 尚未证明稳定大幅提升。

## 5. 分工合作与分支整合

本项目采用任务分支并行开发、最终分支选择性整合的方式：

| 任务 | 主要分支/来源 | 状态 |
| --- | --- | --- |
| 题 1 | `origin/feat/openmp-collapse-for-loop`，上游 `#7438` | 已上游接收 |
| 题 2 | `pr/nonblocking-mpi` | 已整合进 `final` |
| 题 3 | `WorkflowA` / `feat/fft-copy-block-simd`，上游 `#7412/#7432/#7439` | 多项已上游接收，剩余与 final 协同 |
| 题 4 | `GammaOnly` | 已整合进 `final` |
| 题 5 | `feat/SIMD`，上游 `#7412/#7432` | 部分已上游接收，SIMD helper 在 final 保留 |
| 题 6 | `WorkflowA-q6` | compact helper 与 Gamma 权重逻辑整合进 `final` |
| 题 7 | `pr/fft-transform-overlap` | 已整合进 `final` |
| 题 8 | `feat/cache-reuse` | 已整合并修复兼容性 |

整合策略：

1. 先以各任务分支单独验证，不直接污染 `final`。
2. 对已经进上游的 PR，不重复制造大 diff，而是在 `final` 基于 `upstream/develop` 继承其成果。
3. 对 2/4/7 三个重点分支，在 `wt-task2`、`wt-task4`、`wt-task7` 中单独构建、测试、写验证报告，再选择性整合。
4. `final` 中不保留全部历史文档和大体积 benchmark 结果，只保留核心 `Test_docs` 和必要脚本，保持交付分支相对干净。
5. 在 `7046c8dfa` 整合 2/4/7 后发现任务 8 cache API 被覆盖，随后用 `47d3ce35f` 恢复 `PW_Basis` cache 兼容接口并重新验收。

值得强调的是，不能只看 `final` 相对 `upstream/develop` 的 diff 判断贡献，因为部分优化已经被 upstream 吸收。例如：

- `5d2582d72 (#7438)`：题 1 `count_pw_st` OpenMP。
- `128d8d8d4 (#7412)`：complex buffer copy 与 PW round-trip 测试。
- `f4af81009 (#7432)`：FFT 数据重排 memcpy/SIMD 优化。
- `d05769ab7 (#7439)`：PW_Basis FFT copy cache blocking/SIMD。

这些已经成为当前上游基线的一部分，但仍然是本项目完成度的重要成果。

## 6. 最终整合后的测试

### 6.1 构建

构建目录：`build-pw-final-clean`

构建命令：

```bash
cmake --build build-pw-final-clean --target \
  MODULE_PW_cache_bench MODULE_PW_cache_bench_serial MODULE_PW_pw_test \
  MODULE_PW_basis_pw_serial MODULE_PW_basis_pw_k_serial abacus_pw_para -j 16
```

结果：通过，当前复查为 `ninja: no work to do`。

### 6.2 单元测试

| 测试命令 | 结果 |
| --- | --- |
| `OMP_NUM_THREADS=1 mpirun --allow-run-as-root -np 4 ./build-pw-final-clean/source/source_basis/module_pw/test/MODULE_PW_pw_test --gtest_filter='PWTEST.*'` | 47 passed，1 disabled |
| `./build-pw-final-clean/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_serial` | 13 passed |
| `./build-pw-final-clean/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_k_serial` | 12 passed，1 skipped |
| `OMP_NUM_THREADS=1 mpirun --allow-run-as-root -np 2 ./build-pw-final-clean/source/source_basis/module_pw/MODULE_PW_cache_bench` | 通过，cache hit 指标正常 |

### 6.3 应用 smoke 测试

测试环境：

- `OMP_NUM_THREADS=1`
- 临时复制输入到 `/tmp/abacus_finalmd_*`
- `pseudo_dir` 改为 `/root/abacus-develop/tests/PP_ORB`
- 可执行文件：`build-pw-final-clean/abacus_pw_para`

| Case | MPI ranks | Final energy (eV) | Reference (eV) | Abs diff (eV) |
| --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/022_PW_CG` | 2 | `-198.2238296277165` | `-198.2238296207179` | `6.998590e-09` |
| `tests/01_PW/036_PW_AF` | 2 | `-5866.197297502572` | `-5866.197297502891` | `3.192326e-10` |
| `tests/01_PW/089_PW_get_wf_kpar` | 3 | `-211.8789253814144` | `-211.8789253813293` | `8.509460e-11` |

说明：

- `022_PW_CG` 原始输入使用 `scf_thr=1e-8`，因此相对 `result.ref` 的 `7e-9 eV` 差异符合该输入精度水平。
- `036_PW_AF` 和 `089_PW_get_wf_kpar` 均达到 `1e-10 eV` 量级。
- 2/4/7 分支级回归测试中，三例与 baseline 在打印精度上均为 `0.0` 差异。

## 7. 最终结果与结论

本项目围绕 `01_plane_wave.md` 的 8 个任务完成了从算法理解、分支实现、测试验证到最终整合的完整流程。最终交付可以概括为：

- 题 1、题 3 的多项 OpenMP/SIMD 低层优化已被上游接收，成为当前 `develop` 的一部分。
- 题 2 在 gather/scatter 中实现了非阻塞 MPI 路径和错误检查，正确性达到打印精度一致，重 case 有小幅收益。
- 题 4 实现了 all-Gamma 多 k 点 half-spectrum 路径，混合 k 点安全回退，内存指标下降约 `47.7%`，高精度能量差达到 `~2.1e-10 eV`。
- 题 5 提供了可移植 SIMD copy helper，部分 GaAs benchmark 有小幅加速，多数总时间中性。
- 题 6 建立了 Gamma compact 数据结构和真实 G/-G 权重 helper，为题 4 的生产路径提供正确内积语义。
- 题 7 实现了双缓冲 block overlap，真实小 case 自动 fallback，中等合成 multi-block case 观察到约 `2%` 加速。
- 题 8 实现了 PW cache 复用、统计、RAII 和失效机制，最终修复了整合后与任务 8 测试的接口冲突。

最终 `final` 分支状态：

- 本地 `HEAD` 与 `origin/final` 均为 `47d3ce35f`。
- 工作树干净。
- 构建、PW 单元测试、cache benchmark 和三个 PW 应用 smoke case 均通过。

## 8. 仍需注意的风险

1. GammaOnly 题 4 当前通过 `1e-9 eV` 验收，但没有稳定证明 repeated-Gamma case 达到 `1e-14 eV`。若要向上游发 PR，应继续寻找更稳健的收敛设置或更合适的高精度验证 case。
2. 题 7 overlap 的端到端性能收益依赖 workload 和 MPI 栈。本地多数实际小 case 是 single-block fallback，multi-block 加速来自高截断临时 case。
3. 题 8 cache 的总 wall time 在当前 suite 中大多中性，收益更可能体现在特定重复调用路径、初始化热点或未来更细粒度 timer 中。
4. `final` 保留了必要测试报告，但没有把所有历史 benchmark 大文件都放入交付分支；如需完整复现实验，应参考各任务分支和 `/root/abacus_validation_runs` 中的原始运行目录。

总体来看，项目达到了“代码能编译运行、关键计算结果与基线高度一致、部分任务有明确性能或内存收益、分支整合后可通过最终验收”的交付目标。其中最成熟、最适合向上游继续推进的成果是已被接收的 OpenMP/SIMD 小步优化，以及可以进一步收敛精度后提交的 GammaOnly 多 k 点支持。

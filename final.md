# 平面波生成与应用并行优化项目总结

日期：2026-06-26

交付分支：`final`

交付提交：`47d3ce35f Restore PW cache compatibility after task integration`

主要基线：`upstream/develop == origin/develop == 777f50c9c`

项目要求：`01_plane_wave.md`

## 1. 项目背景与物理原理理解

本项目围绕 ABACUS 平面波模块的生成、FFT 变换、MPI 数据交换、GammaOnly 存储和缓存复用展开优化。平面波方法的基本出发点是把周期体系中的 Bloch 波函数写成

```text
psi_k(r) = sum_G c_{k,G} exp(i (k + G) dot r)
```

其中 `k` 是布里渊区采样点，`G` 是倒格矢。ABACUS 通过能量截断 `|G+k|^2 <= Ecut` 选取有限数量的平面波，利用 FFT 在实空间和倒空间之间切换。平面波基组的优势是形式统一、FFT 友好、收敛控制直接；代价是数据量随 FFT 网格、截断能和 k 点数快速增长，因此初始化、FFT 拷贝、gather/scatter 通信和重复几何量构造都容易成为热点。

而 Gamma 点有特殊性。当所有 k 点都是 Gamma 点且实空间函数为实函数时，傅里叶系数满足 Hermitian 共轭关系：

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

这套实现已经具备 MPI + OpenMP 基础并行，但也有几类可优化点：

- `count_pw_st` 三重循环适合线程并行。
- FFT 前后数据拷贝和重排是内存带宽型热点。
- `MPI_Alltoallv` 是阻塞通信，通信等待期间难以隐藏延迟。
- gather/scatter 内部连续拷贝可做 SIMD 化。
- GammaOnly 在多 k 点路径尚未完整支持。
- `G`、`gcar`、`gk2`、`gg_uniq` 等几何量在 SCF 中可缓存复用。

## 3. 总体实现原则

我们小组最终采用了保守、可验证的优化策略：

- 不改变物理模型和公开调用语义，优先优化内部数据搬运、缓存和安全回退。
- 对风险较高的路径，设置显式 fallback，例如混合 k 点回退 full-complex，单通信块回退阻塞 Alltoallv。
- 增加错误检查，比如 MPI 调用，避免通信失败后静默产生错误结果。
- 保证代码的鲁棒性，比如按照 PR 时老师给予的反馈，在缓存和通信缓冲区避免高风险 `mutable` 共享状态，最终 `final` 中 `PW_Basis` cache 使用 RAII 和显式失效。
- 保证代码的可移植性，比如 SIMD helper 按编译器/架构能力选择 AVX2/AVX512 或标量 fallback，
- 文档中所有性能结论都配合严谨的表述，避免把随机噪声或特定 workload 误写成普遍加速。

## 4. 各题实现总结

### 4.1 题目 1：`count_pw_st` 的 OpenMP 并行化

题目要求优化 `pw_distributeg.cpp` 中 `count_pw_st` 的三重循环。该函数扫描候选 G 点并统计 `npwtot`、`nstot`、`st_length2D`、`st_bottom2D` 和边界信息。

最终成果已经通过 PR 进入上游：

- `5d2582d72 Perf: parallelize count_pw_st with OpenMP collapse(2) (#7438)`

具体实现：

- 在 `source/source_basis/module_pw/pw_distributeg.cpp` 的 `PW_Basis::count_pw_st()` 中，把原先串行更新成员变量的统计过程拆成局部变量：`npwtot_local`、`nstot_local`、`lix_local/rix_local`、`liy_local/riy_local`。
- 对 `ix` 外层循环增加 `#pragma omp parallel for collapse(1)`，并为平面波总数、stick 总数和边界变量分别声明加法、min、max reduction。
- 保留 `iy` 和 `iz` 的原有扫描顺序；每个线程只写自己负责的 `(ix, iy)` 对应的 `st_length2D[index]` 和 `st_bottom2D[index]`，因此没有对同一数组元素的并发写冲突。
- `iz` 循环中的 `length` 仍然是局部变量，第一次满足截断条件时写入 `st_bottom2D[index]`，确保 stick 底部坐标语义不变。
- 循环结束后再把局部归约结果写回 `this->npwtot`、`this->nstot`、`this->lix/rix/liy/riy`，没有改变 `count_pw_st()` 的函数接口和后续 `distribution_method1/2()` 的调用方式。

预期效果：

- 缩短平面波初始化阶段的截断球扫描时间。
- 在大 FFT 网格和较高截断能下收益更明显。
- 不改变 G 点选择，因此不引入浮点归约顺序之外的物理差异。

规范性、鲁棒性与可移植性：

- OpenMP 宏保护，未启用 OpenMP 时仍可串行编译。
- reduction 明确表达共享变量依赖。
- 不改变 `count_pw_st` 接口和后续分布逻辑。

测试与结果：

- 上游 PR `#7438` 已接收，说明该优化通过了上游审核者严格的代码审查和测试流程。
- 合并到 `final` 验收时，`MODULE_PW_pw_test --gtest_filter='PWTEST.*'` 47 个测试通过，覆盖平面波分布、GammaOnly、full_pw 和 transform 组合。

### 4.2 题目 2：MPI gather/scatter 非阻塞优化

题目要求把 `pw_gatherscatter.h` 中阻塞 `MPI_Alltoallv` 改造成非阻塞通信，减少通信等待。

实现分支：`pr/nonblocking-mpi`

具体实现：

- 在 `source/source_basis/module_pw/pw_gatherscatter.h` 的 `detail` namespace 中增加 `mpi_complex_dtype<T>()`，对 `double` 返回 `MPI_DOUBLE_COMPLEX`，对 `float` 返回 `MPI_COMPLEX`，使 `gatherp_scatters<T>()` 和 `gathers_scatterp<T>()` 不再依赖运行期类型判断。
- 同一文件中增加 `detail::check_mpi(ierr, where)`，把 `MPI_Ialltoallv`、`MPI_Irecv`、`MPI_Isend`、`MPI_Wait/Waitall` 等调用统一包起来；一旦返回码不是 `MPI_SUCCESS`，错误信息会带上调用位置，便于定位通信失败。
- 在任务 2 分支的 `PW_Basis::gatherp_scatters()` 中，原先的 `MPI_Alltoallv(out, ..., in, ...)` 通信路径被改为非阻塞启动：MPI-3 编译环境走 `MPI_Ialltoallv`，随后显式 `MPI_Wait`；不支持 MPI-3 时按 rank 循环提交 `MPI_Irecv` 和 `MPI_Isend`，再 `MPI_Waitall`。
- `PW_Basis::gathers_scatterp()` 采用对称修改：先保持原来的 pack/unpack 布局语义，再把跨 rank 的 all-to-all 数据交换改为同一套非阻塞 helper；整合到 `final` 后，多块通信路径继续复用这套非阻塞 helper，单块小 case 的阻塞 fallback 在题 7 中单独说明。
- 通信临时数据使用函数内局部 `std::vector<std::complex<T>>` 或调用者传入的 `in/out` buffer，不再依赖隐藏的持久 work-buffer，也没有使用 `mutable` 缓存绕过 const 语义。
- 串行或单 rank 路径仍然直接做本地 copy，MPI 路径只改变通信启动/等待方式，不改变 `numr/startg/numg/startr` 等计数和位移数组的含义。

预期效果：

- 在通信占比较大的 PW case 中，减少阻塞通信的等待时间。

规范性、鲁棒性与可移植性：

- MPI-3 和老 MPI 都有路径。
- 不依赖 `mutable` 隐藏状态。
- 对 `float` 和 `double` 数据类型，明确匹配 MPI complex datatype。
- 对小 case 不保证明显加速，但保持正确性和内存语义清晰。
- 
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
  - 虽然几乎可以认为：在当前算例下非阻塞通信带来的优化很小，但证明这一点本身也算是有价值的吧
  - 能量完全一致，日志确认 `gathers_ialltoallv` / `gatherp_ialltoallv` 激活。

### 4.3 题目 3：FFT 变换数据拷贝与重排的 OpenMP/cache 优化

题目要求优化 `pw_transform.cpp` 中 FFT 变换相关的数据拷贝和重排循环。

我们小组的部分成果已经通过 PR 进入上游：

- `128d8d8d4 Refine complex buffer copies and add round-trip tests for module_pw (#7412)`
- `d05769ab7 Perf: OpenMP cache blocking and SIMD for PW_Basis FFT transform copy routines (#7439)`

具体实现：

- 在 `source/source_basis/module_pw/pw_transform.cpp` 的匿名 namespace 中增加 `constexpr int pw_transform_cache_block = 128` 和 `block_end(begin, size)`，所有被优化的拷贝循环都按固定块处理尾部。
- `real2recip()` 和 `recip2real()` 中原先直接遍历 `nrxx`、`npw`、`nstnz` 的循环，被改为外层 `for (ib += pw_transform_cache_block)` 分块、内层 `#pragma omp parallel for schedule(static)` 或 `#pragma omp simd` 的结构。
- 循环开始前把 `this->nrxx`、`this->npw`、`this->nxyz`、`this->ig2isz`、`this->ig2ixyz_gpu` 等成员读到局部 const 变量，减少循环内重复成员访问，并帮助编译器做别名和向量化分析。
- full-complex 路径仍调用原来的 `fftxyfor/fftxybac`；GammaOnly 实数路径则把 real input 拷入 `rspace` 后调用 `fftxyr2c`，逆变换调用 `fftxyc2r` 后再写回 real 或虚部为 0 的 complex output。
- 对 add 模式保留原有 `factor` 累加语义：非 add 时覆盖输出，add 时在目标数组上累加缩放后的 transform 结果。
- 在 `source/source_basis/module_pw/test/test_transform_omp.cpp` 中新增多线程 round-trip 测试，分别覆盖 full-complex 和 GammaOnly real transform，防止分块和 OpenMP 改写引入索引错误。

预期效果：

- 提升 FFT transform 前后内存搬运效率。
- 在大网格、多线程场景下改善 cache 行为和向量化概率。
- 不改变 FFT 数学过程，结果应逐元素一致或在严格容差内一致。

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

题目要求：让 `PW_Basis_K` 在 all-Gamma 多 k 点场景下真正启用 GammaOnly 半谱路径，并保证混合 k 点安全。

实现分支：`GammaOnly`

具体实现：

- 在 `source/source_basis/module_pw/pw_basis_k.cpp` 中增加 `gamma_k_tolerance = 1.0e-12`，`PW_Basis_K::initparameters()` 先计算每个 `kvec_c[ik]` 的模长，填充新增成员 `is_gamma_k[ik]`。
- `PW_Basis_K::gamma_only` 的赋值改为 `gamma_only_in && all_gamma_k`；只要任意 k 点不是 Gamma，就保持 full-complex FFT 尺寸，避免混合 k 点误走半谱。
- 同一函数中把 `cutoff_kmaxmod` 改成 `all_gamma_k ? 0.0 : kmaxmod`，all-Gamma 时不再因为重复 Gamma k 点扩大 G 截断球；随后按 `gamma_only` 调整 `fftnx/fftny` 为 half-spectrum 尺寸。
- 在 `source/source_basis/module_pw/pw_basis_k.h` 中增加 `bool* is_gamma_k`、`std::vector<double> igl2gamma_weight_k` 和 `get_gamma_weight(ik, igl)`，把“这个 k 点是否 Gamma”和“这个 compact G 的内积权重”作为显式状态保存。
- `PW_Basis_K::setuptransform()` 在 `gamma_only == true` 时初始化基类的 `gamma_compact`；`setupIndGk()` 建立 `igl2ig_k` 后，用 `gamma_compact.conjugate_weight(ig)` 填充 `igl2gamma_weight_k`。
- `source/source_basis/module_pw/pw_transform_k.cpp` 中补齐 `PW_Basis_K::real2recip/recip2real` 的 GammaOnly real transform 路径；complex real-space input 若虚部超过容差则直接报错，避免静默丢弃虚部。
- 在电子态、电荷混合和对角化相关代码中，把原先粗略的 Gamma `2.0` 因子替换为 `get_gamma_weight()`：普通 G/-G 对权重为 2，自共轭点权重为 1，避免 `G=0` 或 Nyquist 边界过计数。

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

即总平面波数和 `npwx` 均下降约 `47.7%`，几乎达到了预期的结果。

但也有一些风险值得说明：

- 该实现虽然经过几次修改，结果能够收敛，且误差在 `1e-9 eV` 级别，但我们认为仍没有达到科学计算所需的精度，因此没有贸然提交 PR 。

### 4.5 题目 5：gather/scatter SIMD 向量化

题目要求优化 `pw_gatherscatter.h` 中的 pack/unpack 数据重排循环，使用 SIMD 提升连续拷贝效率。

实现分支：`feat/SIMD`，且我们小组提交的 PR 也已经被上游接受

具体实现：

- `128d8d8d4 (#7412)` 修改 `source/source_basis/module_pw/pw_gatherscatter.h`，把多处手写 `for` 循环形式的 complex 连续拷贝收敛为更清楚的连续 buffer copy 表达。
- 早期分支中较依赖编译器解释的 `pragma GCC ivdep` 方案没有作为最终上游形式保留；上游接收版改用标准 C++ 的 `std::copy_n`，让连续 `std::complex<T>` 数组复制的语义更明确。
- `gatherp_scatters()` 和 `gathers_scatterp()` 中的 serial/self-copy、pack、unpack 路径被逐一梳理，减少了“按元素复制复数”的重复代码，降低后续和题 2/7 通信改动冲突的概率。
- 同一个 PR 补充 `PW_Basis` / `PW_Basis_K` 的 complex transform round-trip 测试，用 `real2recip()` 和 `recip2real()` 往返验证拷贝表达变化没有破坏 FFT 数据布局。

因为原来是在阻塞通信的语义下，因此 `final` 整合时：

- 在 `source/source_basis/module_pw/` 下新增 `pw_simd_copy.h`，把 SIMD copy 作为独立小 helper，而不是散落在 gather/scatter 主逻辑中。
- `ModulePW::simd_copy_n<T>(dest, src, count)` 的 `count` 定义为标量个数；调用方通过 `reinterpret_cast<T*>(complex_ptr)` 把 `n` 个 `std::complex<T>` 表示成 `2 * n` 个 `float/double` 标量。
- `pw_simd_copy.h` 中分别实现 `simd_detail::copy_n_impl(float*)` 和 `copy_n_impl(double*)`；编译器支持 AVX512 时使用 `_mm512_loadu/storeu`，支持 AVX2 时使用 `_mm256_loadu/storeu`，否则落到普通标量循环。
- AVX512 路径由 `SIMD_COPY_DISABLE_AVX512` 宏控制，避免在容易降频的 CPU 上强制使用 512-bit 指令；所有 SIMD load/store 都使用 unaligned 版本，不假设 `std::vector` 或外部 buffer 对齐。
- `simd_copy_n()` 内部用 `static_assert` 限制 `T` 只能是 `float` 或 `double`，避免把非浮点类型误传给 SIMD helper。
- `pw_gatherscatter.h` 中的 serial/self-copy/pack/unpack 连续复制点调用 `ModulePW::simd_copy_n()`，其余 MPI 计数、位移和数据布局保持不变。

预期效果：

- 在 MPI 通信前后的本地连续复制阶段减少内存搬运时间。
- 与非阻塞通信不冲突，可以作为底层拷贝 helper 复用。

规范性、鲁棒性与可移植性：

- `static_assert` 限制只支持 `float` 和 `double`。
- 无 AVX 支持时自动回退标量循环。
- 不改变 MPI 交换数据量和布局语义。

测试结果：

- 新增/强化了 module_pw complex transform round-trip 测试，覆盖 `PW_Basis` 与 `PW_Basis_K` 的往返变换一致性。
- `feat/SIMD` suite 使用 `gaas_small`、`gaas_medium`、`gaas_large`，MPI `1/2/4`，OpenMP `1/2/4`，共 27 个配置。
- baseline 和 SIMD 两套 suite 均 `27/27` 成功，每组 3 次 repeat。
- 性能中位数多数持平，部分配置有小幅收益，例如：
  - `gaas_medium np=4 omp=4`：speedup `1.125`
  - `gaas_large np=2 omp=4`：speedup `1.062`
  - `gaas_large np=1 omp=1`：speedup `1.054`

结论：我们小组实现了低风险、可移植的连续拷贝路径整理与拷贝内核优化，主要贡献是 `#7412` 中对 complex buffer copy 的标准化表达和 round-trip 测试补强，以及后来 `final` 中保留的 SIMD copy helper。从结果来看，性能上在部分大 case 有收益，但在总 wall time 上不保证普遍显著提升。

### 4.6 题目 6：GammaOnly 紧凑存储

题目要求为 GammaOnly 下的电荷密度、势函数和波函数设计紧凑存储，利用 `F(-G)=conj(F(G))` 减少内存。

实现分支：`WorkflowA-q6`

具体实现：

- 在 `source/source_basis/module_pw/compact_gamma_data.h` 中新增模板类 `CompactGammaData<FPTYPE>`，内部用 `data_` 保存 compact representative，用 `rep_index_`、`need_conj_`、`self_conj_` 描述 logical full G-space 到 compact 存储的映射。
- `CompactGammaData::reset(logical_size)` 提供单元测试可用的 half-by-index 默认映射；`reset(logical_size, minus_g_index)` 接受显式 `G -> -G` index 映射，非法 index 会抛出 `std::out_of_range`。
- `compress_from()` 从 dense reciprocal 数组压缩到 compact 存储；自共轭项只保留实部。`decompress_to()` 按 `need_conj_` 决定直接复制或取共轭，恢复 logical full view。
- `memory_saving_ratio()`、`dense_bytes()`、`compact_bytes()` 提供可观测的存储收益指标，便于测试和报告直接输出内存下降比例。
- 在 `source/source_basis/module_pw/gamma_compact.h/.cpp` 中新增 `GammaCompact`，面向真实 `PW_Basis` half-spectrum 布局构造 `self_conj_`、`conjugate_weight_`、`compact_conj_`、`c2f_`、`f2c_` 和 `conj_of_`。
- `GammaCompact::initialize(const PW_Basis*)` 在 `distribute_g()` 和 `ig2isz/is2fftixy` 建好后调用，基于真实 FFT grid 中的 G 和 `-G` 关系识别自共轭点，而不是按数组下标硬配对。
- `expand_to_full()` 和 `pack_from_full()` 用于 compact/full spectrum 对照验证；生产路径中主要复用 `conjugate_weight()`，并与题 4 的 `PW_Basis_K` 多 k GammaOnly 权重逻辑结合。

预期效果：

- 在 GammaOnly 场景下把 full G-space 的冗余关系显式建模。
- 为 GammaOnly 分支的加权内积、混合回退和 full/compact 对照提供基础设施。
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

实现分支：`pr/fft-transform-overlap`

具体实现：

- 在 `source/source_basis/module_pw/pw_gatherscatter.h` 的 `detail` namespace 中增加 `overlap_block_sticks(nst, span)`，根据 stick 数和每个 stick 对应的数据跨度估计 block 大小，避免对所有 case 固定分配大通信缓冲。
- `PW_Basis::gatherp_scatters()` 先计算 `block_sticks`、`nst_max` 和 `num_blocks`；当 `num_blocks == 1` 时直接进入 `gatherp_single_block_fallback`，继续使用原来的 pack、阻塞 `MPI_Alltoallv`、unpack 流程。
- 多 block 时在函数内部创建一个 `std::vector<std::complex<T>> commbuf`，切成两个 send buffer 和两个 recv buffer；`BlockState blocks[2]` 保存每个 block 的 `sendcounts/recvcounts/sdispls/rdispls` 和 MPI request。
- `prepare_block()` 负责当前 block 的 pack 和本 rank 自拷贝，然后启动非阻塞通信：MPI-3 走 `MPI_Ialltoallv`，老 MPI 走逐 rank 的 `MPI_Irecv` / `MPI_Isend`。
- 主循环采用双缓冲：先 `prepare_block(current)`，下一轮如果还有 block 就 `prepare_block(next)`，随后 `finalize_block(current)` 等待当前通信完成并 unpack；这样下一 block 的本地 pack 可以和当前 block 的通信等待发生重叠。
- `PW_Basis::gathers_scatterp()` 按同样结构实现反向 gather/scatter；timer 名称分别记录 `gatherp_overlap_comm`、`gathers_overlap_comm` 和单块 fallback，方便从日志确认实际路径。
- 双缓冲只在 `num_blocks > 1` 时分配，单块小算例不会为 overlap 额外分配通信工作区。

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

当然也有一些风险值得说明：

- 我们已经已证明 multi-block overlap 可工作且有非常小的收益，但本地可完成的真实生产 case 中，多数仍是单块 fallback。更大规模集群/MPI 栈上，仍需复测。

### 4.8 题目 8：平面波预计算与缓存复用

题目要求识别 SCF 中可复用的平面波几何数据，加入懒加载缓存和失效机制。

实现分支：`feat/cache-reuse`

实现演进（这一题经过了多次改进，主要是为了取得预期效果和与其他小组成员的改动兼容）：

- 第一版缓存先在 `collect_local_pw()`、`collect_uniqgg()` 和历史分支中的 `PW_Basis_K::collect_local_pw()` 上验证“参数不变时跳过重复构建”这一方向。
- 后续提交逐步补齐工程边界：缓存失效时清空公开指针，明确缓存 storage 的所有权，避免 valid 标志失效但旧数据仍可见。
- 为避免并发读写状态不一致，缓存构建、命中判断、失效和统计读取统一纳入 mutex 保护，计数器使用 atomic。
- 为避免晶格、FFT 网格或倒格矢状态变化后静默误命中，引入 cache signature，把 `lat0`、`tpiba/tpiba2`、FFT grid、`npw` 和 `G/GT/GGT` 等决定缓存内容的状态纳入命中条件。
- 在 2/4/7 任务整合后，`47d3ce35f` 恢复并收敛 `PW_Basis` cache API，保留 `reset_cache_stats()`、`get_cache_stats()` 和 benchmark 所需路径，修复整合中出现的接口兼容问题。

具体实现：

- 在 `source/source_basis/module_pw/pw_basis.h` 中新增 `PW_Basis::CacheStats`，包含 `local_pw_hits`、`local_pw_misses`、`uniqgg_hits`、`uniqgg_misses` 和 `cache_bytes`；对外提供 `get_cache_stats()` 与 `reset_cache_stats()`。
- 同一头文件中删除 `PW_Basis` 的拷贝构造和拷贝赋值，避免带缓存所有权的对象被浅拷贝；缓存存储改为 `std::unique_ptr<double[]>`、`std::unique_ptr<Vector3<double>[]>`、`std::unique_ptr<int[]>`。
- `PW_Basis` 增加 `local_pw_cache_valid`、`uniqgg_cache_valid`、`cache_mutex` 和 atomic hit/miss 计数器；构建、命中判断、失效和统计读取都通过 mutex/atomic 管理，没有使用 `mutable` 绕过 const。
- 在 `pw_basis.cpp` 中实现 `clear_owned_cache()`、`invalidate_cache()`、`invalidate_cache_unlocked()`、`make_cache_signature()`、`cache_signature_matches()` 和 `get_cache_stats_unlocked()`，把缓存释放、失效和状态签名比较集中到一处。
- `collect_local_pw()` 首次调用时仍按原算法构造 `gg`、`gdirect`、`gcar`，但结果写入 unique_ptr 管理的 storage，并把公开指针指向 storage；后续 signature 匹配时直接命中缓存并增加 `local_pw_hits`。
- `collect_uniqgg()` 缓存 `ig2igg` 和 `gg_uniq`，同时优先复用已经存在的 `gg`，避免为去重 `|G|^2` 再重复计算本地 G 模长。
- `CacheSignature` 记录 `lat0`、`tpiba/tpiba2`、`nx/ny/nz`、`fftnx/fftny/fftnz`、`npw` 和 `G/GT/GGT`；当晶格、FFT 网格、截断或分布改变时不会误命中旧缓存。
- 在 `pw_init.cpp`、`pw_distributeg.cpp` 等会改变平面波几何状态的入口调用 `invalidate_cache()`，例如 `initmpi()`、`initgrids()`、`initparameters()`、`setfullpw()`、`get_ig2isz_is2fftixy()`。
- `source/source_basis/module_pw/test_serial/pw_cache_bench.cpp` 为 MPI benchmark 对象补充 `initmpi()` 初始化，修复整合后无效 communicator 导致的编译/运行问题；`47d3ce35f` 同时恢复任务 8 测试依赖的 cache stats API。

`PW_Basis_K` 边界说明：

- 历史 `feat/cache-reuse` 分支曾为 `PW_Basis_K::collect_local_pw()` 探索 `gcar/gk2` 重复调用缓存，并用 micro-benchmark 验证了默认参数重复调用和 `erf` 参数变化场景下的收益。
- 最终 `final` 整合时，为降低与 GammaOnly 实现中多 k 点 GammaOnly、GPU/CPU 数据指针和 `gk2` 参数路径的耦合风险，没有把 `PW_Basis_K` 的完整 cache stats/hit-miss 机制作为生产接口保留。
- 因此，最终交付中的稳定生产路径主要是 `PW_Basis` 的 `gg/gdirect/gcar/ig2igg/gg_uniq` 缓存；`PW_Basis_K` 的历史结果作为可行性验证和后续优化方向记录。

预期效果：

- 减少 SCF 中重复构造 G 相关数组、去重 `|G|^2` 和排序映射的开销。
- 提供 cache hit/miss 和缓存字节数，便于后续定位真实收益。

规范性、鲁棒性与可移植性：

- RAII 管理缓存，不手写裸 `delete[]` 所有权。
- 缓存失效集中化。
- 使用 mutex 保护构建/失效路径，计数器使用 atomic。
- 不依赖 GPU/MPI 特定行为，MPI benchmark 显式初始化 communicator。

测试结果：

- `test/test1-1-1.cpp` 验证 `PW_Basis` cache stats 的 build/hit/invalidate 行为：
  - 首次构建后 miss 增加。
  - 重复调用后 hit 增加且 `cache_bytes > 0`。
  - 失效后 `cache_bytes` 回到 0，避免旧缓存被误报告为有效。
- `MODULE_PW_basis_pw_serial` 和 `PWTEST.*` 继续覆盖 `PW_Basis` 基础构建、分布和 transform 组合，验证缓存接口未破坏原有平面波路径。
- `MODULE_PW_cache_bench_serial` / `MODULE_PW_cache_bench` 保留为串行和 MPI 环境下的 cache 可观测性工具。
- `MODULE_PW_cache_bench` 当前运行结果显示：
  - `collect_local_pw_cache_hit.calls = 2000`
  - `collect_uniqgg_cache_hit.calls = 2000`
  - build 与 MPI 运行均通过。
- 历史 micro-benchmark 中，重复调用路径显示数量级收益：
  - `PW_Basis.collect_local_pw.repeat` 中位数约 `255.5x`。
  - `PW_Basis.collect_uniqgg.repeat` 中位数约 `2284.4x`。
  - `PW_Basis_K.collect_local_pw.repeat` 历史分支中位数约 `342.7x`。
  - `PW_Basis_K.collect_local_pw(1.0, 0.5, 0.2)` 历史分支中位数约 `463.0x`，说明 `erf` 参数变化时仍可复用部分几何量。
  - 值得说明的是，这里的测试是单独针对 `collect` 函数的重复调用获得的收益，针对端到端的SCF里可能收益不会如此明显。
- 历史 task8 suite 使用 `gaas_small`、`gaas_medium`，MPI `1/2/4`，OpenMP `1/2/4`，共 18 组 baseline/cache 对比。
  - 1 组更快，13 组持平，4 组略慢。
  - speedup 范围 `0.833` 到 `1.111`，中位数 `1.0`。

结论：我们构建的 micro-benchmark 可以证明重复调用路径能从“每次重建”变为“首轮构建、后续命中”，局部收益非常明确；但端到端 GaAs suite 多数仍为中性，因此并不是稳定的 wall-time 加速。但我们的贡献中的机制建设、正确性、缓存失效安全性和可观测性，也是有价值的。

## 5. 分工合作与分支整合

我们小组采用分工后在不同任务分支并行开发、最终整合的方式：

| 任务 | 主要分支/来源 | 状态 |
| --- | --- | --- |
| 题 1 | `origin/feat/openmp-collapse-for-loop`，上游 `#7438` | 已上游接收 |
| 题 2 | `pr/nonblocking-mpi` | 已整合进 `final` |
| 题 3 | `WorkflowA` / `feat/fft-copy-block-simd`，上游 `#7412/#7439` | 多项已上游接收，剩余与 final 协同 |
| 题 4 | `GammaOnly` | 已整合进 `final` |
| 题 5 | `feat/SIMD`，本小组上游 `#7412` | complex copy 整理已上游接收，SIMD helper 在 final 保留 |
| 题 6 | `WorkflowA-q6` | compact helper 与 Gamma 权重逻辑整合进 `final` |
| 题 7 | `pr/fft-transform-overlap` | 已整合进 `final` |
| 题 8 | `feat/cache-reuse` | 已整合，并修复兼容性 |

整合策略：

1. 先以各任务分支单独构建、测试、验证，再选择性整合到 `final`中。
2. 由于我们小组有 3 条 PR 已经被上游仓库接收，因此对已经进上游的 PR，我们不重复制造 diff，而是在 `final` 基于 `upstream/develop` 继承其成果。
3. `final` 中不保留其他分支中的历史文档和大体积 benchmark 结果，只保留核心 `Test_docs` 和必要脚本，保持分支相对干净。
4. 处理冲突：比如在整合 2/4/7 后发现 8 的 cache API 被覆盖，随后恢复 `PW_Basis` cache 兼容接口并重新验收。

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

本项目围绕 `01_plane_wave.md` ，完成了从算法理解、分支实现、测试验证到最终整合的完整流程。最终交付：

- 题 1、题 3 的多项 OpenMP/SIMD 低层优化已通过 PR 被上游接收，成为 `develop` 的一部分。
- 题 2 在 gather/scatter 中实现了非阻塞 MPI 路径和错误检查，正确性达到打印精度一致，重 case 下才有很小幅的收益。
- 题 4 实现了 all-Gamma 多 k 点 half-spectrum 路径，混合 k 点安全回退，内存指标下降约 `47.7%`，高精度能量差达到 `~2.1e-10 eV`。
- 题 5 提供了可移植 SIMD copy helper，部分 GaAs benchmark 有小幅加速，多数总时间中性，部分优化通过 PR 被上游接收。
- 题 6 建立了 Gamma compact 数据结构和真实 G/-G 权重 helper，为题 4 的生产路径提供了正确内积语义。
- 题 7 实现了双缓冲 block overlap，真实算例中小 case 会自动 fallback，中等合成 multi-block case 中，观察到约 `2%` 加速。
- 题 8 实现了 PW cache 复用、统计、RAII 和失效机制，最终修复了整合后与任务 8 测试的接口冲突。

最终 `final` 分支状态：保证工作树干净。对 `final` 分支还进行了构建、PW 单元测试等，均通过，确保正确性以及改动不冲突、性能不回退。

## 8. 改进空间

虽然我们已经完成了要求的全部内容以及一些个性化的扩展内容，且一些改进被上游接收，但也需要指出一些问题中改进空间仍然存在：

1. 关于 GammaOnly 的完整实现题 4 经过几次尝试，解决了 GammaOnly 情况下计算不收敛的问题，且精度能达到 `1e-9 eV` 级别。但我们感觉，在科学计算领域，这个误差仍不被接收。精度问题暂时没有合理地解决。
2. 题 7 overlap 的端到端性能收益，依赖 workload 和 MPI 栈。本地多数实际小 case 是 single-block fallback，multi-block 加速来自高截断临时 case。
3. 题 8 cache 的总 wall time 在当前 suite 中大多中性，收益更可能体现在特定重复调用路径、初始化热点或未来更细粒度 timer 中。
4. `final` 保留了必要测试报告，但没有把所有历史 benchmark 大文件都放入分支；如需完整复现实验，应参考各任务分支和 `/root/abacus_validation_runs` 中的原始运行目录。

总体来看，项目已经达到了代码能编译运行、关键计算结果与基线高度一致、部分任务有明确性能或内存收益、分支整合后可通过最终验收的交付目标。

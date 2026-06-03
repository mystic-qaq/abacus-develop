# WorkflowB 后续优化方向思考

> **当前状态**：WorkflowB 分支已完成与 `collaborate` 分支的同步，并消除了所有 `mutable` 关键字。以下内容记录在完成这些工作之后，对 WorkflowB 后续可以做什么的分析与建议，供决策参考。
>
> **编写日期**：2026-06-03

---

## 一、已完成工作的回顾

在进入后续方向之前，先确认当前 WorkflowB 已具备的能力：

| 特性 | 状态 | 说明 |
| --- | --- | --- |
| 非阻塞 MPI gather/scatter | ✅ 已完成 | `MPI_Isend` / `MPI_Irecv` + `MPI_Waitsome`，通信与 unpack 重叠 |
| 独立 sendbuf/recvbuf | ✅ 已完成 | 通过 `acquire_comm_workbuf<T>()` 提供，生命周期与输入/输出解耦 |
| 向量化 pack/unpack | ✅ 已完成 | `reinterpret_cast` + `__restrict__` + `#pragma GCC ivdep`，生成 SIMD 友好的循环 |
| 局部平面波缓存 | ✅ 已完成 | `collect_local_pw` / `collect_uniqgg` 带 hit/miss 统计与 atomic 线程安全 |
| 工作缓冲区管理 | ✅ 已完成 | `thread_local` vector，线程安全、无 mutable |
| 缓存锁机制 | ✅ 已完成 | 基于 `std::atomic_flag` 的轻量级 spinlock，无 mutable |
| FFT 计时覆盖 | ✅ 已完成 | pack / alltoallv / unpack 各阶段均有 `ModuleBase::timer` 计时点 |
| 正确性验证框架 | ✅ 已完成 | `test_comm_roundtrip`、`test_transform_omp`、`test_count_pw_st` |

---

## 二、短中期可推进的方向（按优先级排序）

### 2.1 Stick-Block 双缓冲流水线（优先级：高）

**当前状态**：非阻塞 MPI 已经让通信与 unpack 重叠，但 z 方向 FFT（`fftzfor` / `fftzbac`）仍然需要等**本进程全部 stick** 的完整 z 数据就绪才能开始。这意味着通信未完全结束前，FFT 计算单元是空闲的。

**改进方向**：将 stick 按 block 切分，当一个 block 的全部数据到达后，立即启动该 block 的 z-FFT，同时继续等待其余 block 的通信完成。

```
当前模型（阶段 1）：
  recv_all_sticks() ──────────────────→ unpack_all() → fftz(ALL sticks)
                                        ↑ FFT 空闲     ↑ 计算开始

目标模型（阶段 2）：
  recv_block[0] → unpack[0] → fftz(block[0])
  recv_block[1] → unpack[1] → fftz(block[1])     ← 与上文时间重叠
  recv_block[2] → unpack[2] → fftz(block[2])
  ...
```

**实现要点**：
- 需要为 block 粒度重新计算 counts/displacements（每 block 内所有 peer 贡献的条数）
- 需要双缓冲（至少两个 block 的缓冲区，一个正在 FFT 时另一个接收）
- FFT 接口可能需要支持 stick 子集上的批量 `plan_many`（或使用现有的 batch 参数）
- 需要实验确定最优 block 大小（太小：FFT 调用开销超过收益；太大：重叠率下降）

**收益预估**：对于 stick 数多（`nst` 大）、通信时间与 FFT 时间量级接近的场景，可额外获得 10-30% 的通信隐藏率。

**风险**：
- 代码复杂度显著增加
- 小规模（`nst` 小）场景可能因 block 拆分额外开销而退化
- FFTW 的 `plan_many` 在子集上的行为需要验证

---

### 2.2 NUMA 感知的缓冲区放置（优先级：高）

**当前状态**：`acquire_comm_workbuf` 使用 `thread_local std::vector`，由 CRT 默认内存分配器管理。在多 socket 系统上，通信缓冲区可能被分配到远离 NIC 的 NUMA 节点，导致 MPI 传输时额外跨 socket 流量。

**改进方向**：
- 在 `PW_Basis` 初始化时，查询当前进程绑定的 NUMA 节点
- 使用 `libnuma` 或 `mbind` / `move_pages` 将 workbuf 绑定到本地 NUMA 节点
- 或者使用 MPI 的 `MPI_Alloc_mem`（它通常会分配 NIC 近端内存）

**收益预估**：多 socket 系统上，大消息通信延迟降低 10-20%。

**风险**：需要引入 `libnuma` 依赖，或增加 `MPI_Alloc_mem` 的抽象层。

---

### 2.3 MPI 进展引擎集成（优先级：中）

**当前状态**：WorkflowB 使用轮询式 `MPI_Waitsome` 循环来推动 MPI 进展。这依赖于应用代码频繁进入 MPI 调用。如果 unpack 计算量很大，可能长时间不进入 MPI 进展。

**改进方向**：
- 如果 MPI 实现支持后台进展线程（如 MPICH 的异步进展），可以通过环境变量或 MPI_Info 启用
- 或者在 unpack 循环中插入 `MPI_Test` 轻量探测
- 对于不支持后台进展的 MPI 实现，考虑使用 OpenMP 任务（`#pragma omp task`）让一个线程专做通信进展

**收益预估**：对于 unpack 计算量大的场景（大 `nz` 或大 `nst`），可提升通信重叠率 5-15%。

**风险**：OpenMP 任务与 MPI 的交互需要 `MPI_THREAD_MULTIPLE` 支持（当前可能仅为 `MPI_THREAD_FUNNELED`）。

---

### 2.4 自适应通信策略选择（优先级：中）

**当前状态**：WorkflowB 固定使用点对点 `MPI_Isend` / `MPI_Irecv`。这不是在所有场景下都最优：
- 消息非常小时：集合通信 `MPI_Ialltoallv` 的内部实现可能更高效（减少软件调度开销）
- 消息非常大时：点对点通常是更好的选择（允许更灵活的流控）
- 进程数非常多时：点对点的 request 数量激增，`Waitsome` 的轮询开销不可忽略

**改进方向**：
- 在 `PW_Basis::setuptransform()` 或首次通信时，根据消息大小和 `poolnproc` 自动选择策略
- 消息大小阈值可通过环境变量或输入参数配置
- 可实现的策略选项：
  - `AUTO`：自动选择
  - `POINT_TO_POINT`：当前的 Isend/Irecv（大消息默认）
  - `IALLTOALLV`：使用 `MPI_Ialltoallv`（小消息/多进程默认）
  - `IALLTOALLW`：非均匀块大小场景

**收益预估**：小消息场景延迟降低 10-30%；极端多进程场景避免 request 数组爆炸。

**风险**：需要充分测试不同 MPI 实现的 `MPI_Ialltoallv` 行为差异。

---

### 2.5 显式 SIMD 加速 pack/unpack（优先级：中）

**当前状态**：vectorization 完全依赖编译器的自动向量化（通过 `reinterpret_cast` + `#pragma GCC ivdep` 提示）。这在 GCC 上通常有效，但：
- 对于已知为 2 的幂的 `nz` / `nplane`（常见于 FFT 网格），显式 SIMD 可使用对齐加载/存储
- 编译器自动向量化受 `-O2` / `-O3` 影响，结果不够确定
- `ivdep` 在某些编译器版本上可能被忽略

**改进方向**：
- 为最常见的 `nz` 和 `nplane` 值提供手工 unroll + SIMD intrinsic (AVX2/AVX-512) 的 fast path
- 使用编译期检测（`#ifdef __AVX2__`）选择实现
- 保留当前编译器自动向量化路径作为 fallback

**收益预估**：pack/unpack 阶段吞吐量提升 1.5-2×（对于 `nz` 为 2 的幂的场景）。

**风险**：
- 增加代码量和维护负担
- AVX-512 在某些 CPU 上会降频（需要运行时选择）
- 需要对不同数据类型（float/double）分别实现

---

### 2.6 GPU-Aware MPI 路径（优先级：中）

**当前状态**：GPU 路径（CUDA/ROCm）中，FFT 在 GPU 上执行，但 gather/scatter 通信使用的是 CPU 上的 `PW_Basis` 路径。这意味着每次通信前需要 GPU→CPU 拷贝、通信后 CPU→GPU 拷贝。

**改进方向**：
- 利用 CUDA-aware MPI（`MPI_Isend` / `MPI_Irecv` 直接接受 GPU 指针）
- 需要 GPU 版本的 `acquire_comm_workbuf`（使用 `cudaMalloc` / `hipMalloc` 分配 device buffer）
- GPU 端的 pack/unpack 可以用 CUDA kernel 实现（替代 CPU OpenMP 循环）

**收益预估**：GPU 路径中消除 GPU↔CPU 拷贝开销，FFT 通信阶段延迟降低 2-5×。

**风险**：需要 GPUDirect RDMA 支持；CUDA-aware MPI 在不同 MPI 实现中成熟度不一。

---

## 三、长期的战略方向

### 3.1 FFT 全流水线重构

理想情况下，整个 3D FFT 变换应该被组织成流水线：

```
[Irecv slab_i-1] ─→ [fftxy slab_i] ─→ [pack slab_i] ─→ [Isend slab_i]
                                                              ↓
[Isend slab_i] ─→ [recv slab_i] ─→ [unpack slab_i] ─→ [fftz slab_i]
```

即二维 FFT、通信、一维 FFT 三个阶段之间实现完全的流水线重叠。这需要：
- FFT 接口从整体批处理变为流式增量接口
- 上下游调度逻辑统一管理
- 可能需要引入 task-graph 或 DAG 执行框架

**收益**：理论上可将通信开销几乎完全隐藏。

**风险**：工程量极大，需要全面重构 FFT 调用链。

---

### 3.2 性能模型与自动调优

建立 WorkflowB 通信路径的性能模型，根据输入参数（`nst`、`nz`、`nplane`、`poolnproc`、消息大小）预测最优配置：

- 最优 block 大小（stick-block pipelining）
- 最优通信策略（点对点 vs 集合）
- 最优 OpenMP 线程分配（pack/unpack vs FFT）
- 最优 NUMA 绑定策略

可以使用离线 profiling + 线性回归/决策树建模，运行时查表选择配置。

---

### 3.3 缓存统计的运行时暴露

当前 `CacheStats`（hit/miss/bytes）已经统计完成但缺少暴露渠道。可以：
- 通过 `GlobalV` 或日志系统定期输出缓存命中率
- 提供环境变量控制缓存行为（强制禁用、强制失效、定期刷新）
- 在长时间 SCF 运行中，利用统计信息决定是否需要调整缓存策略

---

## 四、不需要立即做但值得记录的想法

1. **MPI 错误恢复**：当前 `WARNING_QUIT` 遇到不支持的类型直接退出。可以改为 fallback 到阻塞 `MPI_Alltoallv`，确保即使编译配置异常也能正确运行（牺牲性能但不牺牲正确性）。

2. **消息合并**：`gatherp_scatters` 和 `gathers_scatterp` 连续调用时（如在 `real2recip` 后紧跟 `recip2real`），考虑 buffer 复用避免重复分配。

3. **CPU 亲和性绑定**：在 OpenMP 并行区域前后，显式绑定线程到连续的 core，提高 pack/unpack 的缓存局部性。

4. **FFTW wisdom 持久化**：FFT plan 的创建成本很高，但当前每次初始化都重建。可以将 wisdom 写入文件，跨运行复用。

5. **测试矩阵自动化**：编写脚本自动在各种 `{npw, nproc, nst, nz, float/double, gamma_only}` 组合下运行正确性测试，并生成性能对比报告。

6. **多 k 点通信合并**：`PW_Basis_K` 中每个 k 点独立调用 gather/scatter。如果多个 k 点共享相同的一级分布，可以考虑批量通信减少延迟。

---

## 五、对当前实现的几点观察

1. **thread_local workbuf 的选择是合理的**：在 `const` API 不变的前提下，`thread_local` 是消除 `mutable` 的最干净方案。唯一的 trade-off 是每个线程持有独立的缓冲区（内存总量 = 线程数 × 最大消息大小），但对于 HPC 场景，这通常是可以接受的。如果未来需要更精细的控制，可以考虑引入 `PW_Basis` 级别的 buffer pool。

2. **cache_spinlock 是适合当前场景的**：锁竞争极低（仅在首次访问时）、临界区短（缓存填充），用 spinlock 替代 mutex 避免了内核态切换。但需要注意：如果未来 `collect_local_pw` 的调用频率显著增加（如在 SCF 收敛困难时），应考虑 fallback 到 adaptive lock（先 spin 若干次，然后 yield/sleep）。

3. **与 collaborate 同步后代码质量明显提升**：collaborate 分支的向量化 hints 和缓存统计是质量很高的工程化改进。建议以后 WorkflowB 的重要改动也先推送到 collaborate 分支 review 后再合并。

4. **计时覆盖已较完善**：pack / alltoallv / unpack 都有独立计时点。可以基于这些数据，在 CI 中自动生成通信时间占比报告，帮助及早发现性能退化。

---

## 六、结论

WorkflowB 当前已经达到了一个很好的阶段性成果：非阻塞通信、向量化 pack/unpack、缓存复用、线程安全的内存管理都已就位。**接下来的最大性能收益机会是 stick-block 双缓冲流水线（2.1）和 NUMA 感知优化（2.2）**。建议优先在这两个方向投入，验证收益后再逐步推进自适应通信策略和显式 SIMD。

所有上述方向的实现都应该遵循一个原则：**先建立正确的性能基线，再进行优化改造，每次改动都通过正确性回归测试**。这样可以确保优化不是以牺牲正确性为代价的。

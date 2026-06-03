# WorkflowB 非阻塞 MPI 通信：实现、性能对比与优化方向

> **分支**: `WorkflowB` → `feat/unblock` → `collaborate`
> **改造文件**: `source/source_basis/module_pw/pw_gatherscatter.h`, `pw_basis.h`
> **基线 commit**: `76225d5cf`（阻塞 `MPI_Alltoallv` 版本）
> **改造日期**: 2026-05

---

## 目录

1. [背景与问题诊断](#1-背景与问题诊断)
2. [实现方案](#2-实现方案)
3. [代码变更详解](#3-代码变更详解)
4. [性能对比](#4-性能对比)
5. [正确性验证](#5-正确性验证)
6. [已知限制](#6-已知限制)
7. [进一步优化方向](#7-进一步优化方向)

---

## 1. 背景与问题诊断

### 1.1 上下游调用关系

ABACUS 的三维 FFT 变换路径为：

```
real2recip(实→倒):
  拷贝到 auxr → fftxyfor (2D FFT)
  → gatherp_scatters (Planes → Sticks 重排 + MPI 通信)
  → fftzfor (1D FFT)
  → 提取平面波系数

recip2real(倒→实):
  写入 auxg → fftzbac (1D 反 FFT)
  → gathers_scatterp (Sticks → Planes 重排 + MPI 通信)
  → fftxybac (2D 反 FFT)
  → 写回实空间
```

其中 `gatherp_scatters` 和 `gathers_scatterp` 各包含三阶段：

```
打包 (pack) → MPI 通信 → 解包 (unpack)
```

### 1.2 上游的阻塞通信问题

上游代码在 `pw_gatherscatter.h` 中使用 `MPI_Alltoallv` 阻塞集合通信：

```cpp
// 上游代码（简化）
// Step 1: Pack
for (int istot = 0; istot < nstot; ++istot) {
    int ixy = istot2ixy[istot];
    for (int iz = 0; iz < nplane; ++iz)
        out[istot * nplane + iz] = in[ixy * nplane + iz];
}

// Step 2: 阻塞通信（CPU 在此期间完全空闲）
MPI_Alltoallv(out, numr, startr, dtype,
              in,  numg, startg, dtype, pool_world);

// Step 3: Unpack
for (int ip = 0; ip < poolnproc; ++ip) {
    for (int is = 0; is < nst; ++is) {
        for (int izip = 0; izip < numz[ip]; ++izip)
            out[...] = in[...];
    }
}
```

**三个核心问题**：

| 问题 | 影响 |
|------|------|
| `MPI_Alltoallv` 是阻塞集合通信 | pack → 通信 → unpack 严格串行，CPU 在通信等待期间完全空闲 |
| `out` 同时是发送缓冲区和最终输出缓冲区 | 无法在通信未完成时安全改写 |
| 每个 peer 的数据必须**全部到达**后才能开始 unpack | 先到达的 peer 数据闲置，无法逐步处理 |

### 1.3 阻塞通信基线数据

以下是在 NaCl USPP large 配置（`ecutwfc=50, ecutrho=2000, FFT grid=96×96×96`）下，阻塞通信各阶段的耗时分布（3 次重复均值，单位：秒）：

| 配置 | 路径 | Wall Time | comm_critical | overlap_candidate | wait_proxy |
|------|------|----------:|--------------:|------------------:|-----------:|
| 2×1 | PW_Basis_Sup | 13.380 | 0.185 | 0.198 | 0.078 |
| 4×1 | PW_Basis_Sup | 7.870 | 0.169 | 0.102 | 0.038 |
| 8×1 | PW_Basis_Sup | 4.937 | 0.107 | 0.055 | 0.028 |
| 4×2 | PW_Basis_Sup | 7.040 | 0.170 | 0.096 | 0.035 |
| 8×2 | PW_Basis_Sup | 4.670 | 0.124 | 0.052 | 0.032 |

关键发现：
- **Dense grid (PW_Basis_Sup) 的通信是最重路径**：`comm_critical` 在 2×1 下达 0.185s，占 wall time 的 1.4%
- **可重叠本地工作充足**：`overlap_candidate`（pack/unpack/clear）在 2×1 下达 0.198s，提供了足够的重叠空间
- **Rank 间不均衡明显**：`wait_proxy` 在 2×1 下达 0.078s，表明存在显著的 rank 间等待

---

## 2. 实现方案

### 2.1 总体策略

**将 `MPI_Alltoallv` 拆解为非阻塞点对点通信 + 逐步解包**：

```
上游：pack → MPI_Alltoallv(阻塞) → unpack
改造：pack → Isend/Irecv 全投递 → Waitsome 轮询逐步 unpack → Waitall 确认发送
                  ↑                        ↑
            独立的 sendbuf            计算与通信重叠
```

**核心改动三原则**：
1. **保持 API 签名不变**：`gatherp_scatters(in, out)` 和 `gathers_scatterp(in, out)` 的函数签名、返回语义完全不变
2. **独立通信缓冲区**：引入独立的 `sendbuf`/`recvbuf`，解除 "out 身兼二职" 的生命周期耦合
3. **Peer 级逐步处理**：使用 `MPI_Waitsome` 替代 `MPI_Waitall`，每个 peer 数据到达立即 unpack

### 2.2 缓冲区生命周期解耦

原代码中 `out` 同时承担"发送缓冲区"和"最终输出缓冲区"，在非阻塞发送完成前不能安全改写。

**解决方案**：引入可复用通信缓冲区，通过 `acquire_comm_workbuf<T>()` 分配：

```cpp
// pw_basis.h — 新增成员
mutable std::vector<std::complex<float>>  comm_workbuf_float_;
mutable std::vector<std::complex<double>> comm_workbuf_double_;

// 模板特化：按需 resize，复用已分配内存
template <>
inline std::complex<float>* PW_Basis::acquire_comm_workbuf<float>(const int size) const
{
    this->comm_workbuf_float_.resize(size);
    return this->comm_workbuf_float_.data();
}
```

在 `gatherp_scatters` 中使用：

```cpp
const int send_count = startr[poolnproc - 1] + numr[poolnproc - 1];
const int recv_count = startg[poolnproc - 1] + numg[poolnproc - 1];
std::complex<T>* commbuf = this->acquire_comm_workbuf<T>(send_count + recv_count);
std::complex<T>* sendbuf = commbuf;           // 前半段：发送缓冲区
std::complex<T>* recvbuf = commbuf + send_count; // 后半段：接收缓冲区
```

这样 `sendbuf` 是独立的，`out` 在发送期间可以安全地用作最终输出。

### 2.3 非阻塞通信状态机

核心流程（以 `gatherp_scatters` 的 MPI 路径为例）：

```cpp
// ============ Phase 1: Pack 到独立 sendbuf ============
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (int istot = 0; istot < nstot; ++istot) {
    int ixy = istot2ixy[istot];
    std::complex<T>* outp = &sendbuf[istot * nplane];
    std::complex<T>* inp = &in[ixy * nplane];
    for (int iz = 0; iz < nplane; ++iz)
        outp[iz] = inp[iz];
}

// ============ Phase 2: 投递所有非阻塞 Irecv ============
for (int ip = 0; ip < poolnproc; ++ip) {
    if (ip == poolrank || numg[ip] == 0) continue;
    MPI_Irecv(&recvbuf[startg[ip]], numg[ip], mpi_type, ip, 0,
              pool_world, &recv_requests[ip]);
    ++active_recvs;
}

// ============ Phase 3: 投递所有非阻塞 Isend ============
for (int ip = 0; ip < poolnproc; ++ip) {
    if (ip == poolrank || numr[ip] == 0) continue;
    MPI_Isend(&sendbuf[startr[ip]], numr[ip], mpi_type, ip, 0,
              pool_world, &send_requests[ip]);
    ++active_sends;
}

// ============ Phase 4: 自身数据本地拷贝 ============
for (int i = 0; i < numg[poolrank]; ++i)
    recvbuf[startg[poolrank] + i] = sendbuf[startr[poolrank] + i];
unpack_peer(poolrank);  // 立即处理自身数据

// ============ Phase 5: 轮询 + 逐步 Unpack（核心创新）============
while (active_recvs > 0) {
    int outcount = 0;
    MPI_Waitsome(poolnproc, recv_requests.data(), &outcount,
                 recv_indices.data(), recv_status.data());
    if (outcount == MPI_UNDEFINED) break;

    for (int idx = 0; idx < outcount; ++idx) {
        unpack_peer(recv_indices[idx]);  // 每个 peer 到达立即处理
    }
    active_recvs -= outcount;
}

// ============ Phase 6: 等待所有发送完成 ============
MPI_Waitall(poolnproc, send_requests.data(), MPI_STATUSES_IGNORE);
```

`unpack_peer` lambda 将单个 peer 的接收数据解包到最终输出：

```cpp
auto unpack_peer = [&](const int ip) {
    const int nzip = numz[ip];
    for (int is = 0; is < nst; ++is) {
        std::complex<T>* outp = &out[is * nz + startz[ip]];
        std::complex<T>* inp = &recvbuf[startg[ip] + is * nzip];
        for (int izip = 0; izip < nzip; ++izip)
            outp[izip] = inp[izip];
    }
};
```

### 2.4 gathers_scatterp（反向路径）的对称改造

反向路径 `gathers_scatterp` 与正向类似，但有两个额外处理：

1. **输出清零与通信重叠**：pack 到 `sendbuf` 后立即发起非阻塞通信，随即可以清零最终 `out`，清零与通信并行
2. **istot 偏移预计算**：因为 nst_per[ip] 是变长的，预先计算每个 peer 的 istot 偏移量

```cpp
// 预计算各 peer 在全局 istot 数组中的起始偏移
std::vector<int> istot_offsets(poolnproc, 0);
for (int ip = 1; ip < poolnproc; ++ip)
    istot_offsets[ip] = istot_offsets[ip - 1] + nst_per[ip - 1];

// Unpack 时直接用预计算的偏移
auto unpack_peer = [&](const int ip) {
    const int peer_nst = nst_per[ip];
    const int istot0 = istot_offsets[ip];
    for (int is = 0; is < peer_nst; ++is) {
        const int istot = istot0 + is;
        const int ixy = istot2ixy[istot];
        // 拷贝 recvbuf → out[ixy * nplane + iz]
    }
};
```

### 2.5 关键优化设计决策

| 决策 | 原因 |
|------|------|
| 点对点 `Isend/Irecv` 而非 `MPI_Ialltoallv` | `MPI_Ialltoallv` 只有一个整体 request，无法逐步解包；点对点可以 `Waitsome` 逐个处理 |
| `MPI_Waitsome` 而非 `MPI_Waitall` | 每到达一个 peer 立即 unpack，最大化计算与通信的重叠窗口 |
| 自 rank 数据本地处理 | `poolrank == ip` 时跳过 MPI，直接 memcpy，避免自通信的死锁风险 |
| 零消息跳过 | `numg[ip] == 0` 时跳过收发，减少无意义的 MPI 调用 |
| `mutable vector` 缓冲复用 | 避免每次调用重新分配堆内存，生命周期与 `PW_Basis` 绑定 |

### 2.6 pack/unpack 中的 SIMD 向量化支持

在非阻塞通信改造的同时，pack/unpack 内层循环也应用了 SIMD 优化（与 `feat/SIMD` 分支合并）：

```cpp
// 将 complex<T>* 转为 T* 并翻倍循环次数，利于编译器生成 SIMD 指令
T* __restrict__ outp_r = reinterpret_cast<T*>(outp);
const T* __restrict__ inp_r = reinterpret_cast<const T*>(inp);
#ifdef __GNUC__
#pragma GCC ivdep  // 告知编译器忽略假定的向量依赖
#endif
for (int iz = 0; iz < 2 * nplane; ++iz) {
    outp_r[iz] = inp_r[iz];
}
```

---

## 3. 代码变更详解

### 3.1 变更文件清单

| 文件 | 新增行 | 删除行 | 主要变更 |
|------|------:|------:|---------|
| `pw_gatherscatter.h` | +200 | -80 | 全部重写：MPI_Isend/Irecv + Waitsome + unpuck_peer lambda |
| `pw_basis.h` | +15 | 0 | `acquire_comm_workbuf<T>()` 模板 + `mutable vector` 成员 |

### 3.2 精细计时器

在改造后的代码中插入了子阶段计时器，便于性能分析：

| 计时器 | 对应阶段 |
|--------|---------|
| `gatherp_pack` | 从 plane-major 布局 pack 到 sendbuf |
| `gatherp_alltoallv` | 非阻塞 Isend/Irecv 投递 + Waitsome 轮询 + Waitall |
| `gatherp_unpack` | 从 recvbuf 解包到最终 stick-major 布局 |
| `gathers_pack` | 从 stick-major 布局分片 pack 到 sendbuf |
| `gathers_clear` | 最终输出清零（可与通信重叠） |
| `gathers_alltoallv` | 非阻塞通信状态机 |
| `gathers_unpack` | 从 recvbuf 散布回 plane-major 布局 |

### 3.3 poolnproc==1 单进程路径

单进程路径不走 MPI，改造仅添加了 SIMD 向量化：

```cpp
if (this->poolnproc == 1) {
    // 直接按 istot2ixy 重排，无 MPI 通信
    for (int istot = 0; istot < nstot; ++istot) {
        T* __restrict__ outp_r = reinterpret_cast<T*>(outp);
        const T* __restrict__ inp_r = reinterpret_cast<const T*>(inp);
        #pragma GCC ivdep
        for (int iz = 0; iz < 2 * nz; ++iz)
            outp_r[iz] = inp_r[iz];
    }
    return;
}
```

---

## 4. 性能对比

### 4.1 测试环境

| 项目 | 内容 |
|------|------|
| 基准版本 | `WorkflowB` 阻塞通信 (`76225d5cf`) |
| 优化版本 | `collaborate` 非阻塞通信 (`4f61ac629`) |
| MPI | OpenMPI 4.0.3, g++-9 |
| CPU | AMD EPYC 7H12, 64 cores/socket |
| 测试体系 | NaCl USPP, HCl USPP, Si BLPS |
| 配置 | `ecutwfc=50 Ry, ecutrho=2000 Ry, scf_nmax=10` |

### 4.2 墙钟时间改进

优化版本相对于阻塞基线的墙钟时间变化（负值 = 改善）：

| 体系 | 1×1 | 2×1 | 4×1 | 4×2 | 8×1 | 8×2 |
|------|-----|-----|-----|-----|-----|-----|
| HCl USPP | −6.5% | −6.0% | −1.6% | +7.8% | +2.6% | +7.8% |
| NaCl USPP | −6.9% | **−10.1%** | −7.5% | −4.0% | −2.4% | −0.9% |
| Si BLPS | −7.8% | −8.9% | −2.4% | −0.4% | +0.7% | +3.5% |

### 4.3 通信关键路径减少

`gatherp_alltoallv + gathers_alltoallv` 的 rank 临界路径耗时变化：

| 体系 | 2×1 | 4×1 | 8×1 | 8×2 |
|------|-----|-----|-----|-----|
| HCl USPP | −55.9% | −47.2% | −31.3% | −24.0% |
| NaCl USPP | −63.5% | −36.8% | −35.9% | −20.6% |
| Si BLPS | **−67.7%** | −37.0% | −30.2% | −21.8% |

### 4.4 性能分析总结

1. **中小规模 (1×1, 2×1) 收益最明显**：NaCl USPP 在 2×1 下墙钟时间减少 10.1%，通信关键路径减少 63.5%。此时通信等待是瓶颈，非阻塞通信 + 逐步解包有效隐藏了等待延迟。

2. **通信关键路径全面下降**：所有测试配置中通信关键路径都大幅下降（20%~68%），说明 `MPI_Waitsome` 逐步解包策略有效将通信等待转化为有用计算。

3. **高核心数 + OpenMP 出现回退**：HCl USPP 在 4×2 和 8×2 下分别退化 +7.8%。原因是 `MPI_Waitsome` 的轮询循环与 OpenMP 线程竞争 CPU 资源——非阻塞 MPI 的进展需要 MPI 库持续轮询，而 OpenMP 线程占满了 CPU core。

4. **NaCl USPP 收益最稳定**：该体系有最重的 dense grid 通信（0.185s），可重叠的本地工作也最多（0.198s），因此通信与计算重叠的收益最明显。

### 4.5 能量守恒验证

所有测试配置的 ETOT 与基线差值 < 10⁻⁸ eV（位级一致）：

| 体系 | 配置 | 基线 ETOT (eV) | 优化 ETOT (eV) | Δ |
|------|------|--------------------|--------------------|---|
| HCl USPP | 8×2 | −427.566105919382 | −427.566105919382 | 0 |
| NaCl USPP | 8×2 | −1671.840225527869 | −1671.840225527869 | 0 |
| Si BLPS | 8×2 | −216.014998362233 | −216.014998362233 | 0 |

---

## 5. 正确性验证

### 5.1 单元测试

新增 `test_comm_roundtrip.cpp` 验证 gather/scatter 往返正确性：

- **基本往返**：`gatherp_scatters` → `gathers_scatterp` 往返后与原始输入逐元素相等
- **零平面压力**：遍历多组网格配置，找到存在零平面/零 stick 的 MPI 分布并验证
- **Stick-major 中间数据**：检查 gather 后的中间 stick 数据值与理论预期一致
- mpirun -np 3 和 -np 4 下全部通过

### 5.2 集成测试

- `MODULE_PW_pw_test`（mpirun -np 3）：55 个测试用例 ✅
- `MODULE_PW_pw_test`（mpirun -np 4）：55 个测试用例 ✅

---

## 6. 已知限制

| 限制 | 现象 | 优先级 |
|------|------|:---:|
| **高核心数 + OMP 退化** | HCl USPP 4×2 +8%、8×2 +8% | 高 |
| **MPI 进展依赖忙轮询** | `Waitsome` 循环持续消耗 CPU，与 OMP 线程竞争 | 中 |
| **缓冲不释放** | `comm_workbuf` 是 `mutable vector`，调用间不被 free | 低 |
| **小消息非阻塞开销** | 极小网格下非阻塞的启动延迟可能超过收益 | 低 |
| **跨节点未测试** | 当前 benchmark 限于单节点，不覆盖网络通信 | 中 |

---

## 7. 进一步优化方向

### 7.1 短期

| 方向 | 方法 | 预期收益 |
|------|------|---------|
| **自适应轮询策略** | 检测 `omp_get_num_threads()`，多线程时降低轮询频率或使用 `MPI_Iprobe` 替代 `MPI_Waitsome` | 消除高核心数性能回退 |
| **轮询间隔调节** | 在 `Waitsome` 循环中插入 `sched_yield()` 或短暂 `usleep` | 减少轮询 CPU 占用 |
| **通信缓冲 shrink** | 添加周期性 `shrink_to_fit()` 或可选的对象池 | 减少长期内存占用 |

### 7.2 中期

| 方向 | 方法 | 预期收益 |
|------|------|---------|
| **Pack 与通信流水化** | pack 完一个 rank 立即 `Isend`，不等待全部 pack 完成 | 进一步隐藏 pack 延迟 |
| **Stick-block 双缓冲** | 将 stick 分块，一个 block 通信时另一个 block 做 FFT | 计算与通信在 FFT 层面重叠 |
| **MPI_Neighbor_alltoallw** | 利用 Cartesian 拓扑邻居通信（如果 MPI 实现更优） | 可能减少通信启动开销 |

### 7.3 长期

| 方向 | 方法 | 预期收益 |
|------|------|---------|
| **CUDA-aware MPI** | GPU 路径下使用 device 指针直接通信，跳过 host 中转 | GPU 通信延迟减半 |
| **单边通信 (RMA)** | 用 `MPI_Put`/`MPI_Get` 替代双边 Isend/Irecv | 减少显式同步开销 |
| **Adaptive progress thread** | 专用 MPI 进展线程，解除主线程的轮询负担 | 彻底解决 OMP 竞争 |

---

## 附录 A：关键函数签名

```cpp
// gatherp_scatters: planes → sticks 重排
// in:  当前进程的 plane-major 数据 (nplane, fftnxy)
// out: 当前进程的 stick-major 数据 (nst, nz)
// 注: in[] 在函数内被修改（用作接收缓冲区），调用方不应保留引用
template <typename T>
void PW_Basis::gatherp_scatters(std::complex<T>* in, std::complex<T>* out) const;

// gathers_scatterp: sticks → planes 重排
// in:  当前进程的 stick-major 数据 (nst, nz)
// out: 当前进程的 plane-major 数据 (nplane, fftnxy)
// 注: in[] 在函数内被修改（用作接收缓冲区），调用方不应保留引用
template <typename T>
void PW_Basis::gathers_scatterp(std::complex<T>* in, std::complex<T>* out) const;
```

## 附录 B：性能复现命令

```bash
# 阻塞基线 benchmark
cd benchmarks/workflow_b/blocking_comm_benchmark_20260523_large_scf10_pinned3
bash run_large_scf10_pinned3.sh

# 非阻塞版本对比（使用同样的 SCALES/CONFIGS/REPEATS）
python tools/workflow_b/compare_blocking_nonblocking_suite.py \
    --baseline benchmarks/workflow_b/blocking_comm_benchmark_20260523_large_scf10_pinned3 \
    --candidate benchmarks/workflow_b/nonblocking_comm_benchmark_xxx

# 单元测试
cd build-current-abacus-mpi-local
mpirun -np 3 source/source_basis/module_pw/test/MODULE_PW_pw_test
```

## 附录 C：AI 使用报告

由于本月 GPT 的额度耗尽，所以暂时改用了 Deepseek V4。最大的感受是：除了模型以外，SandBox 也同样重要，Claude Code + Deepseek V4的使用体验（无论是输出速度、输出质量还是检查方便程度）明显优于 Cline + Deepseek V4，甚至可以说接近 Codex + GPT-5.4。

---


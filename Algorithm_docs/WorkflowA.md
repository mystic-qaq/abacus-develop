# Workflow A: OpenMP 并行化与局部优化算法文档

## 1. 总体范围与涉及模块

### 1.1 目标文件清单

Workflow A 涉及以下 3 个源文件中的特定函数/类：

| 文件路径 | 涉及函数/类 | 对应题目 |
| ------- | ---------- | ------- |
| `source/source_basis/module_pw/pw_distributeg.cpp` | `count_pw_st()`, `distribute_g()` | 题1 |
| `source/source_basis/module_pw/pw_transform.cpp` | `PW_Basis::real2recip()`, `PW_Basis::recip2real()` | 题3 |
| `source/source_basis/module_pw/pw_gatherscatter.h` | `gatherp_scatters()`, `gathers_scatterp()` | 题3 |

### 1.2 模块职责划分

- `pw_distributeg.cpp`: 负责将截断球内的平面波 $(i_x, i_y, i_z)$ 按 stick（即 $(i_x, i_y)$ 列）分配到各 MPI 进程。`count_pw_st` 是其前置步骤，用于统计每个 $(i_x, i_y)$ 上的有效 $i_z$ 数量（stick 长度）。
- `pw_transform.cpp`: 负责实空间与倒易空间之间的 FFT 变换。`real2recip` 与 `recip2real` 是双向对称路径，内部包含数据拷贝、2D-FFT、Alltoallv 通信、1D-FFT、系数提取/注入。
- `pw_gatherscatter.h`: 负责 `real2recip` 中 planes 布局到 sticks 布局的 Pack（`gatherp_scatters`），以及 `recip2real` 中 sticks 布局到 planes 布局的 Unpack（`gathers_scatterp`）。

### 1.3 与其他 Workflow 的边界

- **Workflow B（MPI 通信改造）**: 本 Workflow 仅对 `gatherp_scatters` / `gathers_scatterp` 中的**本地数据重排循环**进行 OpenMP 并行与 Cache 优化，**不改造**其内部的 `MPI_Alltoallv` 阻塞通信本身。
- **Workflow C（Gamma Only / SIMD / 流水线）**: 本 Workflow 的优化代码路径需保持与 Gamma Only 标志、`xprime` 标志的兼容，但不新增 Gamma Only 的紧凑存储或 SIMD 指令。

---

## 2. 题 1: `count_pw_st` 三重循环并行化

### 2.1 代码位置与现有逻辑

**函数**: `void PW_Basis::count_pw_st(int* st_length2D, int* st_bottom2D)`  
**文件**: `pw_distributeg.cpp`

现有逻辑为三重嵌套循环扫描整数倒格矢空间：

```cpp
for (int ix = ix_start; ix <= ix_end; ++ix)
    for (int iy = iy_start; iy <= iy_end; ++iy)
        for (int iz = iz_start; iz <= iz_end; ++iz)
        {
            ModuleBase::Vector3<double> f(ix, iy, iz);
            double modulus = f * (this->GGT * f);  // 6次乘加
            if (modulus <= this->ggecut || this->full_pw)
            {
                int x = ix; if(x < 0) x += this->nx;
                int y = iy; if(y < 0) y += this->ny;
                int ixy = x * this->fftny + y;
                st_length2D[ixy]++;                  // 竞争点 A
                if (st_length2D[ixy] == 1)
                    st_bottom2D[ixy] = iz;           // 竞争点 B
            }
        }
```

**输出**:

- `st_length2D[ixy]`: 位置 `ixy` 处 stick 的平面波数 $L[\text{ixy}]$
- `st_bottom2D[ixy]`: 该 stick 的最小有效 $i_z$（底部索引）
- 后续扫描统计全局 `nstot`（有效 stick 数）与 `npwtot`（总平面波数）

### 2.2 数学形式化与符号定义

| 符号 | 代码变量 | 定义 |
| ---- | -------- | ---- |
| $\mathcal{I}_x, \mathcal{I}_y, \mathcal{I}_z$ | `ix, iy, iz` | 整数倒格矢索引，范围由 `xprime`/`gamma_only` 标志决定 |
| $\mathbf{M}$ | `this->GGT` | 度规矩阵 $3 \times 3$, $\vert\mathbf{G}\vert^2 = \mathbf{f} \cdot (\mathbf{M} \mathbf{f})$ |
| $E_{\text{cut}}$ | `this->ggecut` | 截断能量, $\vert\mathbf{G}\vert^2$ 上界 |
| $N_x^{\text{fft}}, N_y^{\text{fft}}$ | `fftnx`, `fftny` | FFT 网格 $x$, $y$ 维度 |
| $n_{\text{xy}}$ | `fftnxy` | $N_x^{\text{fft}} \cdot N_y^{\text{fft}}$ |
| $\text{ixy}$ | `ixy` | 线性化网格索引, $\text{ixy} = x \cdot N_y^{\text{fft}} + y$，其中 $x = i_x\mod N_x$, $y = i_y\mod N_y$ |
| $L[\text{ixy}]$ | `st_length2D[ixy]` | stick 长度，该 $(i_x, i_y)$ 上满足截断的 $i_z$ 个数 |

**循环边界形式化**:

| 标志组 | `ix_start` | `ix_end` | `iy_start` | `iy_end` |
| ------ | --------- | ------- | --------- | ------- |
| 普通 | $-\lfloor N_x/2 \rfloor$ | $\lfloor N_x/2 \rfloor$ | $-\lfloor N_y/2 \rfloor$ | $\lfloor N_y/2 \rfloor$ |
| `xprime = true` | $0$ | $N_x^{\text{fft}} - 1$ | $-\lfloor N_y/2 \rfloor$ | $\lfloor N_y/2 \rfloor$ |
| `xprime = false` | $-\lfloor N_x/2 \rfloor$ | $\lfloor N_x/2 \rfloor$ | $0$ | $N_y^{\text{fft}} - 1$ |

**不变式**:

- $\sum_{\text{ixy}=0}^{n_{\text{xy}}-1} \mathbb{I}(L[\text{ixy}] > 0) = n_{\text{stot}}$
- $\sum_{\text{ixy}=0}^{n_{\text{xy}}-1} L[\text{ixy}] = n_{\text{pw}}$

### 2.3 数据依赖与竞争分析

三重循环的迭代空间大小约为 $N_x N_y N_z \sim O(N^3)$。并行化前必须识别以下竞争：

| 竞争点 | 变量 | 竞争类型 | 根因 |
| ----- | ---- | ------- | ---- |
| A | `st_length2D[ixy]` | 原子性竞争 | 不同 `iz` 层可能映射到相同 `(ix, iy)` → 同一 `ixy` |
| B | `st_bottom2D[ixy]` | 条件竞争 | `== 1` 判断与赋值非原子，且多线程可能同时满足条件 |
| C | `nstot` | 归约竞争 | 多线程同时累加全局 stick 计数 |
| D | `npwtot` | 归约竞争 | 多线程同时累加全局平面波计数 |

**关键观察**: 给定固定的 $(i_x, i_y)$，所有 $i_z$ 都映射到**同一个** `ixy`。因此：

- **不可并行化 `iz` 循环**（写冲突到同一 `ixy`）。
- **可以并行化 `ix`-`iy` 组合**（不同 $(i_x, i_y)$ 映射到不同 `ixy`，无结构竞争）。
- **可以按 `ix` 分段并行**（每个线程处理一段连续的 `ix`，包含完整的 `iy` 和 `iz`）。

### 2.4 线程安全解决方案

#### 2.4.1 方案A: 线程私有累加数组

每个线程分配私有的 `st_len_priv[fftnxy]` 和 `st_bot_priv[fftnxy]`，在临界区外完成全部累加，最后归约到全局数组。

```cpp
#pragma omp parallel
{
    int tid = omp_get_thread_num();
    int nthreads = omp_get_num_threads();

    // 私有数组（初始化为0）
    std::vector<int> st_len_priv(fftnxy, 0);
    std::vector<int> st_bot_priv(fftnxy, std::numeric_limits<int>::max());

    // 按 ix 静态分段
    int nx_range = ix_end - ix_start + 1;
    int chunk = nx_range / nthreads;
    int rem = nx_range % nthreads;
    int ix_local_start = ix_start + tid * chunk + std::min(tid, rem);
    int ix_local_end = ix_local_start + chunk - 1 + (tid < rem ? 1 : 0);

    for (int iz = iz_start; iz <= iz_end; ++iz)
        for (int ix = ix_local_start; ix <= ix_local_end; ++ix)
            for (int iy = iy_start; iy <= iy_end; ++iy)
            {
                // ... 计算 modulus ...
                if (modulus <= ggecut)
                {
                    int ixy = ...;
                    st_len_priv[ixy]++;
                    if (iz < st_bot_priv[ixy]) st_bot_priv[ixy] = iz;
                }
            }

    // 临界区归约（或使用 #pragma omp critical）
    #pragma omp critical
    {
        for (int ixy = 0; ixy < fftnxy; ++ixy)
        {
            st_length2D[ixy] += st_len_priv[ixy];
            if (st_bot_priv[ixy] < st_bottom2D[ixy])
                st_bottom2D[ixy] = st_bot_priv[ixy];
        }
    }
}
```

**复杂度**:

- 时间: $O(N_x N_y N_z / T + n_{\text{xy}} \cdot T)$（计算 + 归约）
- 空间: $O(n_{\text{xy}} \cdot T)$。典型场景 $n_{\text{xy}} < 10^5, T \leq 64$，总私有内存 $< 25\text{MB}$，可接受。

#### 2.4.2 方案B: 原子操作

若内存受限，可对 `st_length2D` 使用原子递增：

```cpp
#pragma omp parallel for collapse(2)
for (int iz = iz_start; iz <= iz_end; ++iz)
    for (int ix = ix_start; ix <= ix_end; ++ix)
        for (int iy = iy_start; iy <= iy_end; ++iy)
            if (modulus <= ggecut)
            {
                #pragma omp atomic
                st_length2D[ixy]++;
                // st_bottom2D 仍需原子比较或临界区保护
            }
```

**代价**: 每次有效 G 点触发一次原子操作。当 $n_{\text{pw}} \sim 10^5$ 时开销明显，仅作为内存受限时的备选。

### 2.5 并行调度策略

**负载均衡分析**: 截断球在 x-y 平面投影对称，中心区域 $(i_x \approx 0)$ 平面波略多，但差异通常不超过 2x。按 `ix` 静态分段已足够均衡。

| 策略 | 通信/同步开销 | 内存开销 | 适用场景 |
| ---- | ----------- | ------- | ------- |
| `schedule(static)` + 私有数组 | $O(n_{\text{xy}} \cdot T)$ 归约 | $O(n_{\text{xy}} \cdot T)$ | **推荐**，通用场景 |
| `schedule(static)` + 原子操作 | $O(n_{\text{pw}})$ 原子延迟 | $O(1)$ | 内存受限、线程数少 |
| `schedule(dynamic, 1)` | $O(N_x \log T)$ 调度开销 | 取决于实现 | **不推荐**，本场景负载已均衡 |

**归约优化**: 当 $n_{\text{xy}}$ 较大时，可将 `st_length2D` 分段为 $T$ 个 block，各线程归约到不同 block，最后串行合并，减少临界区竞争。

### 2.6 正确性验证要求

1. **结果一致性**: 并行版本的 `st_length2D`、`st_bottom2D`、`nstot`、`npwtot` 必须与串行版本按位一致。
2. **边界测试**:
   - 不同网格大小 $36^3, 40^3, 42^3$
   - 不同截断能量
   - `xprime=true/false`、`gamma_only=true/false`、`full_pw=true/false` 组合
3. **线程安全测试**: 在 TSan/Helgrind 下运行，确认无 data race。

---

## 3. 题 3: FFT 数据重排与内存访问优化

### 3.1 代码位置与数据流

`real2recip`（正向路径）包含三阶段本地数据移动（无 MPI 阶段）：

| 阶段 | 操作 | 访存量 | 所在位置 |
| ---- | ---- | ------ | ------- |
| P1 | 实空间 `in` → `auxr` 填充/转置 | $O(n_{\text{rxx}})$ | `pw_transform.cpp:real2recip` |
| P2 | `gatherp_scatters_pack`: planes → sticks 重排 | $O(n_{\text{stot}} \cdot n_{\text{plane}})$ | `pw_gatherscatter.h:gatherp_scatters` |
| P3 | `gathers_scatterp_unpack`: sticks → planes 重排 | $O(n_{\text{st}} \cdot N_z)$ | `pw_gatherscatter.h:gathers_scatterp` |

**注**: `recip2real` 为严格逆路径，P2/P3 角色互换，优化策略对称。

### 3.2 P1: `real2recip` 入口数据拷贝

#### 3.2.1 现有内存访问模式分析

以普通模式（非 gamma_only）为例：

```cpp
# pragma omp parallel for
for (int ir = 0; ir < this->nrxx; ++ir)
    auxr[ir] = in[ir];
```

或更复杂的二维转置形式：

```cpp
for (int ixy = 0; ixy < nxy; ++ixy)
    for (int iz = 0; iz < nplane; ++iz)
        auxr[ixy * nplane + iz] = in[ixy + iz * nxy];
```

| 数组 | 访问模式 | 步长 | 局部性 |
| ---- | ------- | ---- | ------ |
| `auxr` (写) | 连续 | 1 | 好，流式写入 |
| `in` (读) | 跨步 | $n_{\text{xy}}$ | 差，每次 `iz` 递增跳跃 $n_{\text{xy}}$ |

当 $n_{\text{xy}}$ 很大（如 $256 \times 256 = 65536$）时，`in` 的跨步读取导致每次均可能 L1/L2 cache miss。

#### 3.2.2 Tiling 优化与循环重排

通过交换循环顺序并引入分块（tiling），使 `in` 的读取落入同一 cache line：

```cpp
const int TILE_SIZE = 16;  // 根据 L1 cache 调整
for (int iz_block = 0; iz_block < nplane; iz_block += TILE_SIZE)
{
    int iz_end = std::min(iz_block + TILE_SIZE, nplane);
    for (int ixy = 0; ixy < nxy; ++ixy)
        for (int iz = iz_block; iz < iz_end; ++iz)
            auxr[ixy * nplane + iz] = in[ixy + iz * nxy];
}
```

**原理**: `iz` 内层循环连续访问 `in[ixy + iz * nxy]` 时，由于 `iz` 递增 1，地址跳跃 $n_{\text{xy}} \times \text{sizeof(complex)}$。Tiling 后，一个 tile 内处理固定范围的 `iz`，使得 `in` 的访问模式在 `ixy` 维度上局部化。

**Tile 大小选择**: L1 cache 约 32KB，`complex<double>` 16 bytes。建议 `TILE_SIZE = 16`，单个 tile 占用 $16 \times 64 = 1024$ bytes，远小于 L1。

#### 3.2.3 OpenMP 并行策略

```cpp
# pragma omp parallel for collapse(2) schedule(static)
for (int iz_block = 0; iz_block < nplane; iz_block += TILE_SIZE)
    for (int ixy = 0; ixy < nxy; ++ixy)
    {
        int iz_end = std::min(iz_block + TILE_SIZE, nplane);
        for (int iz = iz_block; iz < iz_end; ++iz)
            auxr[ixy * nplane + iz] = in[ixy + iz * nxy];
    }
```

- `collapse(2)` 将 `iz_block` 与 `ixy` 合并为单一迭代空间，提高负载均衡性。
- 若 `nplane` 较小（如 1~4），可直接 `collapse(2)` 原始循环；若 `nplane` 很大（>256），建议保留 tiling 结构。

### 3.3 P2: `gatherp_scatters` Pack 阶段

#### 3.3.1 间接寻址与跨步读取问题

`gatherp_scatters_pack` 将 planes 布局（`in[ixy * nplane + iz]`）重组为 sticks 布局（`out[pos + iz]`），准备 `MPI_Alltoallv` 发送。

```cpp
// 原始逻辑
for (int istot = 0; istot < nstot; ++istot)
{
    int ixy = istot2ixy[istot];           // (A) 间接寻址
    int target_proc = fftixy2ip[ixy];     // (B) 二次间接寻址
    int pos = offset[target_proc];        // (C) 动态偏移
    for (int iz = 0; iz < nplane; ++iz)
        out[pos + iz] = in[ixy * nplane + iz];  // (D) 跨步读取
    offset[target_proc] += nplane;
}
```

| 操作 | 访问模式 | 瓶颈 |
| ---- | ------- | ---- |
| (A) `istot2ixy[istot]` | 顺序读取 | 无 |
| (B) `fftixy2ip[ixy]` | 间接寻址（随机） | L1 miss（若 stick 分布不均匀） |
| (C) `offset[target_proc]` | 原子/临界区更新 | 线程竞争 |
| (D) `in[ixy * nplane + ...]` | 基地址跳跃 | cache 预取失效 |

#### 3.3.2 预计算偏移与无锁并行

**核心优化**: 通过 `exclusive_scan` 预先计算每个 stick 在输出缓冲区中的绝对位置，消除 `offset` 的动态更新竞争。

```cpp
// 预计算阶段（串行，O(nstot)）
std::vector<int> out_pos(nstot);
std::vector<int> proc_local_idx(poolnproc, 0);

for (int istot = 0; istot < nstot; ++istot)
{
    int ixy = istot2ixy[istot];
    int target = fftixy2ip[ixy];
    out_pos[istot] = startg[target] + proc_local_idx[target] * nplane;
    proc_local_idx[target]++;
}

// 并行拷贝阶段（无锁，完全独立）
# pragma omp parallel for schedule(static)
for (int istot = 0; istot < nstot; ++istot)
{
    int ixy = istot2ixy[istot];
    int pos = out_pos[istot];
    const int in_base = ixy * nplane;
    for (int iz = 0; iz < nplane; ++iz)
        out[pos + iz] = in[in_base + iz];
}
```

**收益**:

- 消除 `offset` 的原子操作或临界区。
- `istot` 间完全独立，线性扩展至 $T$ 线程。

#### 3.3.3 Stick 局部排序优化

`distribution_method1` 按 stick 长度降序排序后，`istot2ixy` 的 `ixy` 顺序被打乱，导致 `in` 的读取地址跳跃（`ixy` 不连续），cache 预取失效。

**优化**: 在并行拷贝前，对当前进程负责发送的 sticks 按 `ixy` **局部排序**后批量拷贝。由于 `exclusive_scan` 已固定了每个 stick 的输出位置，排序仅改变拷贝顺序，不影响正确性。

```cpp
// 建立 (istot, ixy) 对，按 ixy 排序
std::vector<std::pair<int,int>> istot_ixy_list;
for (int istot = 0; istot < nstot; ++istot)
    istot_ixy_list.emplace_back(istot, istot2ixy[istot]);
std::sort(istot_ixy_list.begin(), istot_ixy_list.end(),
          [](auto& a, auto& b){ return a.second < b.second; });

// 按排序后的顺序并行拷贝
# pragma omp parallel for schedule(static)
for (int idx = 0; idx < istot_ixy_list.size(); ++idx)
{
    int istot = istot_ixy_list[idx].first;
    int ixy = istot_ixy_list[idx].second;
    int pos = out_pos[istot];
    // ... 拷贝 ...
}
```

**注意**: 若使用 `distribution_method2` 按 $(i_x, i_y)$ 扫描顺序分配，`istot2ixy` 天然单调递增，无需额外排序。

### 3.4 P3: `gathers_scatterp` Unpack 阶段

#### 3.4.1 跨步写入问题

`gathers_scatterp_unpack` 将接收后的 sticks 数据写回 planes 布局：

```cpp
ZEROS(out, nplane * nxy);
for (int is = 0; is < nst; ++is)
{
    int ixy = is2fftixy[is];              // 间接寻址
    for (int iz = 0; iz < nplane; ++iz)
        out[ixy * nplane + iz] = in[is * nplane + iz];  // 跨步写入
}
```

| 数组 | 访问模式 | 局部性 |
| ---- | ------- | ------ |
| `in` (读) | 连续（`is * nplane + iz`） | 好 |
| `out` (写) | 跨步（`ixy * nplane` 基地址跳跃） | 差 |

#### 3.4.2 并行化策略

与 P2 对称，预计算每个本地 stick 的输出位置后无锁并行：

```cpp
// 预计算（若 is2fftixy 单调，则无需排序）
# pragma omp parallel for schedule(static)
for (int is = 0; is < nst; ++is)
{
    int ixy = is2fftixy[is];
    int out_base = ixy * nplane;
    int in_base = is * nplane;
    for (int iz = 0; iz < nplane; ++iz)
        out[out_base + iz] = in[in_base + iz];
}
```

若 `is2fftixy` 非单调（method1 导致），同样可先按 `ixy` 对本地 sticks 排序后再拷贝。

### 3.5 Cache 优化总结

| 优化项 | 适用阶段 | 方法 | 预期收益 |
| ------ | ------- | ---- | ------- |
| Tiling | P1 | `iz` 分块，改善 `in` 读取局部性 | L2/L3 miss 减少 20-40% |
| 预计算偏移 | P2/P3 | `exclusive_scan` 替代动态 `offset` | 消除临界区，线性扩展 |
| Stick 局部排序 | P2/P3 | 按 `ixy` 排序后批量拷贝 | 改善 `in`/`out` 的读取/写入局部性 |
| 合并读写 | P2/P3 | 若 `nplane` 为 2 的幂，后续可配合 SIMD（题5） | 2-4x 加速（未来工作） |

---

## 4. 现有逻辑局限性与解决方向

### 4.1 `count_pw_st` 的局限性

1. **串行瓶颈**: 三重循环完全串行，时间复杂度 $O(N_x N_y N_z)$。对于大体系 $N \sim 100$ ，初始化耗时显著。
2. **负载不均衡边缘**: 虽然截断球投影大致均匀，但中心区域 $(i_x \approx 0)$ 的 stick 长度略长，静态分段的极个别线程可能多 10-20% 工作量。
3. **内存冗余**: 线程私有数组方案需 $O(n_{\text{xy}} \cdot T)$ 临时内存，在 $T > 64$ 且 $n_{\text{xy}} > 10^6$ 时可能占用数百 MB。
4. **边界处理复杂**: `xprime` 与 `gamma_only` 的组合导致 `ix_start/ix_end` 有多种分支，OpenMP 的 `schedule` 需适配不同边界。

**解决方向**:

- 采用 **静态分段 + 私有数组** 作为默认方案，在内存受限时 fallback 到原子操作。
- 对 `ix` 范围做动态任务窃取（`schedule(guided)`）仅当实测负载不均时启用。
- 将私有数组分配在栈上（`alloca` 或 VLA）以减少堆分配开销，或复用线程局部存储（TLS）缓冲区。

### 4.2 FFT 数据重排的局限性

1. **跨步访问固有**: planes 布局与 sticks 布局的维度不一致（`ixy` vs `istot`），导致转置类操作必然存在跨步读写，Cache 友好度有理论上限。
2. **间接寻址开销**: `istot2ixy` 和 `fftixy2ip` 的间接查找在 P2 阶段引入随机访存，难以通过编译器自动向量化。
3. **`MPI_Alltoallv` 阻塞**: 虽然本 Workflow 不改造 MPI，但 P2/P3 的 Pack/Unpack 与通信串行执行，Pack 完成后才能启动通信，存在等待间隙。
4. **Gamma Only 路径差异**: `gamma_only` 时 `fftnx = nx/2 + 1`，循环边界变化， tiling 的 `TILE_SIZE` 需重新标定。

**解决方向**:

- P1 阶段采用 **Tiling + `collapse(2)`** 最大化数据复用。
- P2/P3 阶段通过 **预计算偏移 + 无锁 OpenMP** 将 Pack/Unpack 并行化，使其与后续 Workflow B 的**非阻塞 MPI** 衔接时，Pack/Unpack 本身不再成为瓶颈。
- 若 `distribution_method2` 被采用，其天然的 `ixy` 单调性可省去排序开销，**建议在题1/题3的测试基准中对比 method1 与 method2 的总耗时**。
- 对于 `nplane` 为 2 的幂的场景，后续 Workflow（题5 SIMD）可进一步将内层 `iz` 循环向量化。

### 4.3 与后续 Workflow 的衔接

- **Workflow B（MPI 非阻塞）**: 本 Workflow 的 P2/P3 无锁并行化是前置条件。只有当 Pack/Unpack 本身足够快时，Overlap 通信与计算才有意义。
- **Workflow C（Gamma Only 紧凑存储）**: 本 Workflow 的所有优化需保持与 `gamma_only` 标志兼容。Gamma Only 时 P1 的 `r2c` 路径数据布局不同， tiling 边界需以 `fftnx`（而非 `nx`）为基准。
- **题5（SIMD）**: 本 Workflow 的 Tiling 和预计算偏移为 SIMD 提供了规整的内存访问模式（连续拷贝），降低了向量化难度。

---

## 5. 测试与验证规范

### 5.1 单元测试要求

**题1 单元测试**:

1. **正确性**: 对比并行与串行输出的 `st_length2D`、`st_bottom2D`、`nstot`、`npwtot`，要求逐元素相等。
2. **边界**: 测试最小网格 $4 \times 4 \times 4$ 、非立方网格 $36 \times 24 \times 48$ 、`full_pw=true`（全网格）。
3. **线程安全**: 在 1, 2, 4, 8, 16 线程下运行，结果一致。

**题3 单元测试**:

1. **正确性**: 对给定随机输入 `in`，对比优化前后 `auxr` / `out` 缓冲区内容，要求逐元素相等（浮点精确相等，因仅为数据移动）。
2. **性能**: 使用 `omp_get_wtime()` 测量 P1/P2/P3 阶段耗时，对比不同线程数（1, 2, 4, 8）的加速比。

### 5.2 性能基准测试

推荐测试体系:

| 体系 | 原子数 | 平面波数 | 网格大小 | 备注 |
| ---- | ------ | ------- | ------- | ---- |
| Al fcc | 1 | ~1000 | $36^3$ | Gamma only 适用 |
| Si diamond | 2 | ~2000 | $40^3$ | Gamma only 适用 |
| NaCl rocksalt | 2 | ~1800 | $42^3$ | Gamma only 适用 |

**基准指标**:

| 优化项 | 当前时间 | 目标时间 | 最低加速比 |
| ------ | ------- | ------- | --------- |
| `count_pw_st` | $T_1$ | $T_1 / 4$ | 4x (4线程) |
| FFT 数据拷贝 (P1) | $T_3$ | $T_3 / 4$ | 4x (4线程) |
| `gatherp_scatters_pack` (P2) | $T_{P2}$ | $T_{P2} / T$ | 线性扩展 |

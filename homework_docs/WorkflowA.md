---
workflow: A
title: "OpenMP 并行化与局部优化算法文档"
owner: 姚远
tasks: [1, 3]
modules: [module_pw]
---

# Workflow A: OpenMP 并行化与局部优化算法文档

> **文档目标**：将 `source/source_basis/module_pw` 中 `count_pw_st` 三重循环与 FFT 数据重排循环的隐式数据依赖、内存访问模式、并行化约束显式化为可指导实现、可验证正确性、可分析负载的形式化算法文档。
>
> **范围限定**：
> - **题1**：`pw_distributeg.cpp` 中 `count_pw_st` 三重循环的 OpenMP 并行化与线程安全处理
> - **题3**：`pw_transform.cpp` 与 `pw_gatherscatter.h` 中 FFT 前后数据拷贝/重排循环的内存访问模式与调度策略优化
>
> **不涉及**：MPI 通信改造（属 Workflow B）、Gamma Only 完整实现（题4）、SIMD 向量化（题5）、双缓冲流水线（题7）。

---

## Layer 1: 数学形式化与符号系统

### 1.1 符号表

| 符号 | 代码变量 | 定义 | 类型/范围 |
|------|----------|------|-----------|
| $\mathcal{I}_x$ | `ix` | x 方向循环索引 | $\mathbb{Z}$, $[\text{ix\_start}, \text{ix\_end}]$ |
| $\mathcal{I}_y$ | `iy` | y 方向循环索引 | $\mathbb{Z}$, $[\text{iy\_start}, \text{iy\_end}]$ |
| $\mathcal{I}_z$ | `iz` | z 方向循环索引 | $\mathbb{Z}$, $[\text{iz\_start}, \text{iz\_end}]$ |
| $\mathbf{G}(i_x, i_y, i_z)$ | `f` (Vector3) | 倒格矢（整数坐标系） | $\mathbb{Z}^3$ |
| $\mathbf{M}$ | `this->GGT` | 度规矩阵（$3 \times 3$） | $\mathbb{R}^{3 \times 3}$ |
| $E_{\text{cut}}$ | `this->ggecut` | 平面波截断能量 ($G^2$ 上界) | $\mathbb{R}_{>0}$ |
| $N_x$ | `nx` | 物理 x 方向网格维度 | $\mathbb{N}$ |
| $N_y$ | `ny` | 物理 y 方向网格维度 | $\mathbb{N}$ |
| $N_z$ | `nz` | z 方向网格维度 | $\mathbb{N}$ |
| $N_x^{\text{fft}}$ | `fftnx` | FFT x 方向维度 | $\mathbb{N}$ |
| $N_y^{\text{fft}}$ | `fftny` | FFT y 方向维度 | $\mathbb{N}$ |
| $n_{\text{xy}}$ | `nxy` / `fftnxy` | $N_x^{\text{fft}} \cdot N_y^{\text{fft}}$, 2D FFT 网格总点数 | $\mathbb{N}$ |
| $n_{\text{stot}}$ | `nstot` | 全局 sticks 总数 | $\mathbb{N}$ |
| $n_{\text{pw}}$ | `npwtot` | 全局平面波总数 | $\mathbb{N}$ |
| $L[\text{ixy}]$ | `st_length2D[ixy]` | 位置 $\text{ixy}$ 处 stick 包含的平面波数 | $\mathbb{N}$ |
| $n_{\text{plane}}$ | `nplane` | 当前进程持有的 z-plane 层数 | $\mathbb{N}$ |
| $n_{\text{rxx}}$ | `nrxx` | 当前进程实空间局部网格大小 $= n_{\text{plane}} \cdot n_{\text{xy}}$ | $\mathbb{N}$ |
| $n_{\text{st}}$ | `nst` | 本地 sticks 数 | $\mathbb{N}$ |
| $T$ | — | 线程数（OpenMP `num_threads`） | $\mathbb{N}$ |

### 1.2 数学关系

#### 1.2.1 截断球判断

对于给定的整数倒格矢 $(i_x, i_y, i_z)$，构建 $\mathbf{G}(i_x, i_y, i_z) = (i_x, i_y, i_z)$，判断其是否位于截断球内：

$$
\mathbf{G}(i_x, i_y, i_z) \cdot \big(\mathbf{M} \cdot \mathbf{G}(i_x, i_y, i_z)\big) \leq E_{\text{cut}}
$$

其中 $\mathbf{M} = \mathbf{G}_{\text{rec}} \cdot \mathbf{G}_{\text{rec}}^T$（即 `GGT`），$\mathbf{G}_{\text{rec}}$ 为倒格矢矩阵。

#### 1.2.2 Stick 位置编码

在 x-y 平面中，stick 的位置由 $(i_x, i_y)$ 唯一确定，线性化为：

$$
\text{ixy} = x \cdot N_y^{\text{fft}} + y
$$

其中：

$$
x = \begin{cases}
i_x         & \text{if } i_x \geq 0 \\
i_x + N_x   & \text{if } i_x < 0
\end{cases}
\qquad
y = \begin{cases}
i_y         & \text{if } i_y \geq 0 \\
i_y + N_y   & \text{if } i_y < 0
\end{cases}
$$

即将整数倒格矢 $(i_x, i_y)$ 映射到第一象限（通过周期性平移）。

#### 1.2.3 Pack/Unpack 索引映射

**`gatherp_scatters` (planes → sticks)**：

- 发送侧（planes 布局）：`in[ixy * nplane + iz]`
  - `ixy ∈ [0, nxy)`：x-y 平面上的网格索引
  - `iz ∈ [0, nplane)`：本地 z-plane 层索引
- 中转缓冲（sticks 布局）：`out[istot * nplane + iz]`
  - `istot ∈ [0, nstot)`：全局 stick 编号
  - `ixy = istot2ixy[istot]`：从全局 stick 编号反查网格位置
- 接收侧（sticks 布局）：本地 stick 数据按 `(is, iz)` 排列

**`gathers_scatterp` (sticks → planes)**：上述映射的严格逆过程。

#### 1.2.4 循环边界形式化

| 标志组 | `ix_start` | `ix_end` | `iy_start` | `iy_end` |
|--------|-----------|---------|-----------|---------|
| 普通 | $-\lfloor N_x/2 \rfloor$ | $\lfloor N_x/2 \rfloor$ | $-\lfloor N_y/2 \rfloor$ | $\lfloor N_y/2 \rfloor$ |
| `xprime = true` | $0$ | $N_x^{\text{fft}} - 1$ | $-\lfloor N_y/2 \rfloor$ | $\lfloor N_y/2 \rfloor$ |
| `xprime = false` | $-\lfloor N_x/2 \rfloor$ | $\lfloor N_x/2 \rfloor$ | $0$ | $N_y^{\text{fft}} - 1$ |

**不变式**：

$$
\sum_{\text{ixy}=0}^{n_{\text{xy}}-1} \mathbb{I}(L[\text{ixy}] > 0) = n_{\text{stot}}
$$

$$
\sum_{\text{ixy}=0}^{n_{\text{xy}}-1} L[\text{ixy}] = n_{\text{pw}}
$$

---

## Layer 2: 核心数据结构形式化定义

### 2.1 `ixy2st`: 网格位置 → stick 存在性

- **函数签名**: $\text{ixy2st}: [0, n_{\text{xy}}) \to \{0, 1\}$
- **代码实现**: `st_length2D[ixy] > 0`
- **数学定义**:

  $$
  \text{ixy2st}(\text{ixy}) = \mathbb{I}\big(L[\text{ixy}] > 0\big)
  $$

- **不变式**: $\sum_{\text{ixy}} \text{ixy2st}(\text{ixy}) = n_{\text{stot}}$
- **并行化约束**: 该函数由 `st_length2D[ixy]` 的值间接表达，在并行 `count_pw_st` 中为累加目标，存在线程竞争风险。

### 2.2 `loop2ixy`: 循环索引 → 线性网格索引

- **函数签名**: $\text{loop2ixy}: \mathcal{I}_x \times \mathcal{I}_y \to [0, n_{\text{xy}})$
- **代码实现**: `const int ixy = x * this->fftny + y`
- **数学定义**:

  $$
  \text{loop2ixy}(i_x, i_y) = \text{pos}(i_x) \cdot N_y^{\text{fft}} + \text{pos}(i_y)
  $$

  其中 $\text{pos}(v) = v$ if $v \geq 0$ else $v + N_{\{x,y\}}$.

- **不变式**: 映射为双射（在有效 $i_x, i_y$ 范围内），不产生 $\text{ixy}$ 碰撞。

### 2.3 `st_count`: stick 平面波计数

- **函数签名**: $\text{st\_count}: [0, n_{\text{xy}}) \to \mathbb{N}$
- **代码实现**: 内层循环中对 `st_length2D[ixy]` 的累加操作 `++`
- **数学定义**:

  $$
  \text{st\_count}(\text{ixy}) = \left|\left\{ i_z \in \mathcal{I}_z \mid \mathbf{G}(i_x, i_y, i_z) \cdot (\mathbf{M} \cdot \mathbf{G}(i_x, i_y, i_z)) \leq E_{\text{cut}} \right\}\right|
  $$

- **不变式**: $L[\text{ixy}] = \text{st\_count}(\text{ixy})$（写完后的终态）
- **线程安全约束**: 不同 $\text{ixy}$ 互斥访问 $L[\text{ixy}]$，无结构竞争；全局累加 `nstot`、`npwtot` 存在竞争。

### 2.4 `copy_planes`: 实空间平面拷贝映射

- **函数签名**: $\text{copy\_planes}: [0, n_{\text{rxx}}) \to [0, \text{fft\_size})$
- **代码实现**: 循环变量 `ir`, `ixy`, `iz`
- **数学定义**（按两层嵌套）:

  $$
  \text{auxr}[i_{\text{xy}} \cdot n_{\text{plane}} + i_z] \leftarrow \text{in}[\chi(i_z, i_{\text{xy}})]
  $$

  其中 $\chi$ 为输入数组的索引函数，依赖于实际调用上下文。

- **并行化约束**: 目标 `auxr[ixy * nplane + iz]` 的访问模式对不同的 `(ixy, iz)` 互斥，可完全并行。

---

## Layer 3: 算法伪代码与正确性分析

### 3.1 `count_pw_st` — 三重循环扫描

```
算法: count_pw_st — 截断球扫描与 stick 统计
输入:
    - this->ggecut   (E_cut, 截断半径平方)
    - this->GGT      (M, 度规矩阵)
    - this->nx, this->ny, this->nz  (物理网格维度)
    - this->fftnx, this->fftny      (FFT 维度)
    - this->gamma_only, this->xprime, this->full_pw (模式标志)
输出:
    - st_length2D[ixy]  (每个 stick 的平面波数, 初值为 0)
    - st_bottom2D[ixy]  (每个 stick 的起始 z 索引)
    - this->nstot        (全局 sticks 总数)
    - this->npwtot       (全局平面波总数)
    - this->lix, this->rix  (活跃 stick 在 x 方向的最小/最大 ixy)
    - this->liy, this->riy  (活跃 stick 在 y 方向的最小/最大 ixy)
前置条件:
    - nx, ny, nz 已初始化
    - fftnx, fftny >= nx, ny
    - ggecut >= 0
后置条件:
    - Σ L[ixy] = npwtot
    - Σ I(L[ixy] > 0) = nstot
    - ∀ ixy 满足 L[ixy] > 0: st_bottom2D[ixy] 为该 stick 的最小有效 iz
时间复杂度: O(nx * ny * nz) ～ O(N^3)
通信复杂度: 无 (纯本地计算)
```

**伪代码**:

```
count_pw_st(st_length2D, st_bottom2D):
    // 初始化
    ZEROS(st_length2D, fftnxy)          // 所有 stick 计数归零
    ZEROS(st_bottom2D, fftnxy)          // 底部位置归零
    nstot_local  = 0                    // 本地线程的 sticks 计数
    npwtot_local = 0                    // 本地线程的平面波计数

    // 确定扫描边界
    设置 ix_start, ix_end, iy_start, iy_end
    设置 iz_start, iz_end
    // 例如普通模式: iz_start = -floor(nz/2), iz_end = floor(nz/2)

    // 三重循环扫描
    for iz = iz_start to iz_end:
        for ix = ix_start to ix_end:
            for iy = iy_start to iy_end:
                // 计算 G 矢量的模平方
                f = Vector3(ix, iy, iz)
                modulus_sq = f · (GGT · f)    // 矩阵-向量乘法, 6次乘加
                if modulus_sq <= ggecut:
                    // 找到有效的平面波
                    // 将 (ix, iy) 映射到 [0, fftnx) x [0, fftny)
                    x = ix; if x < 0: x += nx
                    y = iy; if y < 0: y += ny
                    ixy = x * fftny + y

                    // 累加 stick 的平面波计数: 竞争点!
                    st_length2D[ixy] += 1     // (A) 线程不安全点

                    // 记录 stick 的最小 iz（底部）
                    if st_length2D[ixy] == 1:
                        st_bottom2D[ixy] = iz  // (B) 条件竞争点

    // 归约各线程的局部计数
    // 从 st_length2D 统计 nstot 和 npwtot
    scan_all_ixy:
        if st_length2D[ixy] > 0:
            nstot_local  += 1
            npwtot_local += st_length2D[ixy]
            更新 liy, riy, lix, rix

    return

// 主调函数中归约
nstot  = reduction_sum(nstot_local)          // (C) 全局竞争点
npwtot = reduction_sum(npwtot_local)         // (D) 全局竞争点
liy = reduction_min(liy_local); ...          // 边界值归约
```

---

### 3.1.1 线程安全分析与解决方案（题1核心）

#### 竞争类型分析

| 竞争点 | 变量 | 竞争类型 | 严重性 | 频率 |
|--------|------|---------|--------|------|
| (A) | `st_length2D[ixy]` | 原子性: 不同线程可能同时写同一 `ixy` | 高 | 每次有效 G 点 |
| (B) | `st_bottom2D[ixy]` | 条件竞争: `== 1` 判断与赋值之间可能被其他线程打断 | 中 | 每个 stick 仅一次 |
| (C) | `nstot` | 归约竞争: 多线程同时累加 | 中 | O(nxy) |
| (D) | `npwtot` | 归约竞争: 多线程同时累加 | 中 | O(nxy) |

#### 竞争 (A) 的根本原因

在最内层循环中，不同的 $(i_x, i_y)$ 对映射到**不同的** $\text{ixy}$。但单个线程迭代所有的 $(i_x, i_y)$，并不会出现两个线程同时写同一个 $\text{ixy}$ 的情况——等等，实际上在**并行化**后，若按 `iz` 分层并行，则不同的 `iz` 层可能操作相同的 `(ix, iy)`，从而冲突到同一个 `ixy`。

**正确的并行化策略**：

外循环 `iz` 的迭代间存在依赖：所有 `iz` 都对同一个 `st_length2D[ixy]` 进行累加。因此：

- **不能**并行化 `iz` 循环（写冲突）
- **可以**并行化 `ix`-`iy` 组合（给定 `iz` 时，$(i_x,i_y)$ 映射到不冲突的 `ixy`）
- **可以**按 `ix` 分段并行（每个线程处理一部分 `ix` 范围，包含完整的 `iy` 和 `iz`）

#### 推荐方案: 按 `ix` 分段的并行 + 每线程私有累加

```
// 方案 A: 每线程私有 st_length2D + 最终归约 (内存高效、无竞争)
#pragma omp parallel
{
    int tid = omp_get_thread_num();
    int nthreads = omp_get_num_threads();

    // 每个线程分配私有的累加数组
    int* st_len_priv  = new int[fftnxy];  ZEROS(st_len_priv, fftnxy);
    int* st_bot_priv  = new int[fftnxy];  fill(st_bot_priv, fftnxy, MAX_INT);

    // 按 ix 分段
    int ix_chunk = (ix_end - ix_start + 1) / nthreads;
    int ix_local_start = ix_start + tid * ix_chunk;
    int ix_local_end   = (tid == nthreads-1) ? ix_end : ix_local_start + ix_chunk - 1;

    for iz = iz_start to iz_end:
        for ix = ix_local_start to ix_local_end:
            for iy = iy_start to iy_end:
                f = Vector3(ix, iy, iz)
                modulus_sq = f · (GGT · f)
                if modulus_sq <= ggecut:
                    x = ix; if x < 0: x += nx
                    y = iy; if y < 0: y += ny
                    ixy = x * fftny + y
                    st_len_priv[ixy]++              // 无竞争: 线程私有
                    if (iz < st_bot_priv[ixy]):
                        st_bot_priv[ixy] = iz        // 无竞争: 线程私有

    // 临界区归约 (或使用 #pragma omp critical)
    #pragma omp critical
    {
        for ixy = 0 to fftnxy-1:
            st_length2D[ixy] += st_len_priv[ixy]
            st_bottom2D[ixy] = min(st_bottom2D[ixy], st_bot_priv[ixy])
    }

    delete[] st_len_priv;
    delete[] st_bot_priv;
}

// 后续统计 (无需线程间通信，因为 st_length2D 已归约完毕)
scan_all_ixy:
    if st_length2D[ixy] > 0:
        nstot  += 1
        npwtot += st_length2D[ixy]
```

**算法复杂度**：

- 时间复杂度: $O(N_x N_y N_z / T + n_{\text{xy}} \cdot T)$（计算 + 归约）
- 空间复杂度: $O(n_{\text{xy}} \cdot T)$（每线程私有数组）
- 加速比上界: 受限于归约开销 $O(n_{\text{xy}} \cdot T)$，当 $N_z$ 较小时加速有限

#### 替代方案: 原子操作

若 $n_{\text{xy}} \cdot T$ 的内存开销不可接受，可使用原子操作：

```
#pragma omp parallel for
for iz = iz_start to iz_end:
    for ix = ix_start to ix_end:
        for iy = iy_start to iy_end:
            // ... 计算 ...
            if modulus_sq <= ggecut:
                #pragma omp atomic
                st_length2D[ixy]++      // 原子递增
```

**代价**: 每次有效 G 点触发一次原子操作，在平面波数 $n_{\text{pw}} \sim 10^5$ 级别时原子操作开销明显。推荐使用线程私有累加方案。

---

### 3.2 FFT 数据重排 — `real2recip` 中的拷贝循环

```
算法: FFT 前数据 Pack (实空间 → auxr 填充)
输入:
    - in[0 .. nrxx-1]          (实空间数据，按 (ixy, iz) 排列)
    - nplane, nxy              (网格参数)
    - this->gamma_only         (是否为 gamma only 模式)
输出:
    - auxr[0 .. fft_size-1]   (FFT 输入缓冲区，按 FFT 库要求的布局排列)
前置条件:
    - nplane == numz[my_rank]
    - nrxx == nplane * nxy
后置条件:
    - auxr 中数据已按 FFT 约定重整完毕
时间复杂度: O(nrxx) = O(nplane * nxy)
通信复杂度: 无
```

**伪代码** (普通模式, 非 gamma_only):

```
// 输入: in[ir], 其中 ir = iz * nxy + ixy (或 ixy * nplane + iz, 视布局而定)
// 目标: auxr[ixy * nplane + iz] ← in[iz * nxy + ixy]
// 即二维转置

for ixy = 0 to nxy - 1:
    for iz = 0 to nplane - 1:
        auxr[ixy * nplane + iz] = in[ixy + iz * nxy]    // (1) 顺序读写
```

**内存访问模式分析**：

读写内核 `auxr[ixy * nplane + iz] = in[ixy + iz * nxy]`

| 数组 | 访问模式 | 步长 | 局部性 |
|------|---------|------|--------|
| `auxr` (写) | 连续写入 | 1 ($\text{nplane} > 1$ 时每次 iz 递增 1) | 极好（流式写入, 可合并写入） |
| `in` (读) | 跨步读取 | $n_{\text{xy}}$（每次 iz 递增跳 $n_{\text{xy}}$ 个元素） | 差（跨步较大时 cache miss 高） |

当 $n_{\text{xy}}$ 很大（例如 $256 \times 256 = 65536$）时，`in` 的跨步读取可能导致每次读取都 miss L1/L2 cache。

**优化方案**:

**方案 1: 交换循环顺序（tiling）**

```
// 按 tile 分块，提高 in 的 cache 局部性
const int TILE_SIZE = 16;
for iz_block = 0 to nplane-1 step TILE_SIZE:
    iz_end_block = min(iz_block + TILE_SIZE, nplane)
    for ixy = 0 to nxy-1:
        for iz = iz_block to iz_end_block-1:
            auxr[ixy * nplane + iz] = in[ixy + iz * nxy]
```

Tile 大小选择: 对于 L1 cache 32KB, `complex<double>` 16 bytes, 建议 `TILE_SIZE` 使得 `TILE_SIZE * cache_line_size` ≈ L1 大小 / 4。例如 `TILE_SIZE = 16`, 则一个 tile 占用 $16 \times 64 = 1024$ bytes（远小于 L1）。

**方案 2: OpenMP 并行 + tiling**

```
#pragma omp parallel for collapse(2) schedule(static)
for iz_block = 0 to nplane-1 step TILE_SIZE:
    for ixy_block = 0 to nxy-1 step TILE_SIZE:
        处理 tile (ixy_block, iz_block)
```

`collapse(2)` 将两层 tile 循环合并为一个大迭代空间，提高负载均衡性。

---

### 3.3 FFT 数据重排 — `gatherp_scatters` 的 Pack 阶段

```
算法: gatherp_scatters_pack — 将 planes 布局重组为 sticks 布局
输入:
    - in[0 .. fft_size-1]       (planes 布局: in[ixy * nplane + iz])
    - istot2ixy[0 .. nstot-1]  (全局 stick -> ixy 映射)
    - fftixy2ip[0 .. nxy-1]    (ixy -> MPI rank 映射)
    - numg[0 .. nproc-1]       (发送给各进程的数据量)
输出:
    - out[0 .. sum(numg)-1]    (sticks 布局, 准备 MPI_Alltoallv 发送)
前置条件:
    - in 已完成 x-y 方向 2D FFT
后置条件:
    - out 中 stick 数据按顺序排列: 先按目标 rank, 再按 stick 的 ixy 递增, 再按 iz
时间复杂度: O(nstot * nplane)
通信复杂度: 无 (此阶段为本地重排)
```

**伪代码**:

```
// 构造 sticks 顺序的发送缓冲区
gatherp_scatters_pack(in, out, istot2ixy, fftixy2ip, numg, nstot, nplane, nproc):
    offset[0 .. nproc-1] = 0           // 每进程的输出偏移
    // 按全局 stick 顺序遍历
    for istot = 0 to nstot-1:
        ixy = istot2ixy[istot]         // (A) 间接寻址读取
        target_proc = fftixy2ip[ixy]   // (B) 二次间接寻址
        pos = offset[target_proc]
        // 将该 stick 的所有 z 层拷贝到连续位置
        for iz = 0 to nplane-1:
            out[pos + iz] = in[ixy * nplane + iz]    // (C) 跨步读取
        offset[target_proc] += nplane
```

**内存访问模式分析**:

| 操作 | 访问模式 | 瓶颈 |
|------|---------|------|
| (A) `istot2ixy[istot]` | 顺序读取 | 无 |
| (B) `fftixy2ip[ixy]` | 间接寻址（随机） | L1 cache miss (若 stick 分布不均匀) |
| (C) `in[ixy * nplane + ...]` | 跨步读取 ($\text{ixy} \cdot n_{\text{plane}}$) | 步长 = $n_{\text{plane}}$，随机跳跃 |
| `out[pos + iz]` | 连续写入 | 无 |

**关键问题**: `(C)` 中，`ixy` 的值随 `istot` 变化而**跳跃**（例如 `istot2ixy[0] = 1234`, `istot2ixy[1] = 5678`），导致从 `in` 读取的步长不固定，cache 预取失效。

**优化方案: 按 ixy 排序 sticks**

若 sticks 能按 `ixy` 递增顺序排列（`distribution_method2` 的保证），则 `istot2ixy` 单调递增，读取 `in` 时地址接近连续：

```
// 若 istot2ixy 单调递增, 则:
// istot=0 -> ixy=10, istot=1 -> ixy=12, istot=2 -> ixy=15 ...
// in[10*nplane], in[12*nplane], in[15*nplane] 接近顺序访问
```

`distribution_method1` 按长度排序后 sticks 的 `ixy` 顺序被打乱，`gatherp_scatters` 的内存访问变差。解决方式：

- 在 `gatherp_scatters` 内部，对当前进程需要发送的 sticks 按 `ixy` **局部排序**后批量拷贝
- 或使用 `distribution_method2`（sticks 数均衡, 保持 ixy 顺序）

---

### 3.4 FFT 数据重排 — `gathers_scatterp` 的 Unpack 阶段

```
算法: gathers_scatterp_unpack — 将 sticks 布局重组为 planes 布局
输入:
    - in[0 .. nst * nz - 1]             (sticks 布局, MPI_Alltoallv 接收后)
    - is2fftixy[0 .. nst-1]            (本地 stick -> ixy 映射)
输出:
    - out[0 .. fft_size-1]             (planes 布局: out[ixy * nplane + iz])
前置条件:
    - in 已完成 z 方向 1D IFFT
后置条件:
    - out 中各 ixy 平面数据连续存放, 准备 x-y 方向 IFFT
时间复杂度: O(nst * nz)
```

**伪代码**:

```
gathers_scatterp_unpack(in, out, is2fftixy, nst, nplane):
    // out 通常需要先清零 (若存在无 stick 的 ixy)
    ZEROS(out, nplane * nxy)

    for is = 0 to nst-1:
        ixy = is2fftixy[is]              // 间接寻址
        for iz = 0 to nplane-1:
            out[ixy * nplane + iz] = in[is * nplane + iz]  // 跨步写入
```

**内存访问分析**: `out[ixy * nplane + iz]` 为跨步写入（`ixy` 决定基地址），`in[is * nplane + iz]` 为连续读取。

**优化**: 与 `gatherp_scatters` 对称，若 `is2fftixy` 单调，写入模式接近连续。

---

## Layer 4: 并行策略与调度分析

### 4.1 `count_pw_st` 并行调度策略

#### 4.1.1 负载均衡分析

三重循环的迭代空间大小为：

$$
|\text{space}| = (i_x^{\text{end}} - i_x^{\text{start}} + 1) \times (i_y^{\text{end}} - i_y^{\text{start}} + 1) \times N_z \approx N_x N_y N_z
$$

按 `ix` 分段：

- 总块数: $N_x$（或半空间 $N_x / 2$）
- 每块计算量: $\approx N_y \cdot N_z$（均匀）
- **负载均衡性**: 极好，各块的截断球内有效 G 点数统计均匀（截断球在 x-y 平面上的投影对称）

#### 4.1.2 调度策略对比

| 策略 | 通信开销 | 内存开销 | 适用场景 |
|------|---------|---------|---------|
| `schedule(static)` + 私有数组 | $O(n_{\text{xy}} \cdot T)$ | $O(n_{\text{xy}} \cdot T)$ | $n_{\text{xy}}$ 较小时 |
| `schedule(static)` + 原子操作 | 原子操作 $O(n_{\text{pw}})$ | $O(1)$ | $n_{\text{xy}}$ 较大、线程数较少 |
| `schedule(dynamic, 1)` | 调度开销 $O(N_x \log T)$ | 取决于具体实现 | 负载不均匀场景（非此场景） |

**推荐**: `schedule(static)` + 线程私有数组。平面波计算场景中 $n_{\text{xy}}$ 通常 $< 10^5$，$T \leq 64$，私有数组总内存 $< 10^5 \times 64 \times 4\text{B} \approx 25 \text{MB}$，可接受。

#### 4.1.3 归约开销

归约阶段遍历 $n_{\text{xy}}$ 对两个数组做逐元素求和/取 min:

$$
T_{\text{reduce}} = O\left(\frac{n_{\text{xy}}}{T} + \alpha \cdot T\right)
$$

其中 $\alpha$ 为临界区开销。当 $n_{\text{xy}}$ 较大时，可分段归约以减少临界区竞争。

### 4.2 FFT 数据重排调度策略（题3核心）

#### 4.2.1 `real2recip` 路径的三阶段数据移动

| 阶段 | 操作 | 访存量 | 可并行性 |
|------|------|--------|---------|
| P1: 填充 `auxr` | `in → auxr` 转置拷贝 | $O(n_{\text{rxx}})$ | 完全并行（无依赖） |
| P2: `gatherp_scatters_pack` | `auxr → send_buffer` 重排 | $O(n_{\text{stot}} \cdot n_{\text{plane}})$ | sticks 间无依赖 |
| P3: `gathers_scatterp_unpack` | `recv_buffer → out` 重排 | $O(n_{\text{st}} \cdot N_z)$ | sticks 间无依赖 |

#### 4.2.2 P1 填充 `auxr` 的并行化

```
#pragma omp parallel for collapse(2) schedule(static)
for ixy = 0 to nxy-1:
    for iz = 0 to nplane-1:
        auxr[ixy * nplane + iz] = in[ixy + iz * nxy]
```

**注意**: 若 `nplane` 较小（例如 1~4），`collapse(2)` 合并后迭代数 = `nxy * nplane`，负载足够。若 `nplane` 很大（例如 > 256），可不 `collapse`，仅并行外层 `ixy`。

#### 4.2.3 P2/P3 的并行化

P2（`gatherp_scatters_pack`）和 P3（`gathers_scatterp_unpack`）的 sticks 间操作无依赖，可直接并行：

```
// P2 并行：每个线程处理一批 sticks
#pragma omp parallel for schedule(static)
for istot = 0 to nstot-1:
    ixy = istot2ixy[istot]
    target = fftixy2ip[ixy]
    // 需要原子操作保护 offset[target] 的修改
    // 或者预先计算好每个 stick 的输出位置（使用 exclusive scan）
    pos = output_pos[istot]      // 预先计算，无竞争
    for iz = 0 to nplane-1:
        out[pos + iz] = in[ixy * nplane + iz]
```

**关键优化**: 预先通过 `exclusive_scan(numg)` 计算每个 stick 的输出起始位置，消除 `offset` 的原子操作依赖。这样每个 `istot` 的访问完全独立，可无锁并行。

**`exclusive_scan` 的伪代码**:

```
// 输入: numg[0..nproc-1] (每个 rank 的数据量)
// 输出: offset[0..nproc]   (exclusive scan)
offset[0] = 0
for ip = 0 to nproc-1:
    offset[ip+1] = offset[ip] + numg[ip]

// 然后对于 istot, 其所属 rank 内的偏移为:
// rank_start = offset[target_proc]
// rank_local_offset = (前面已处理的属于该 rank 的 sticks 数) * nplane
// pos = rank_start + rank_local_offset
```

**`rank_local_offset` 的预计算**: 需要在并行前预先遍历 `istot2ixy` 和 `fftixy2ip` 统计每个 stick 在其目标 rank 中的局部序号。可在 `distribution_method1/2` 完成后一次性建立该映射表。

#### 4.2.4 内存访问与 Cache 优化总结

| 优化项 | 方法 | 预期收益 |
|--------|------|---------|
| Tiling (P1) | 将 iz 循环分块，使 `in` 的跨步读取落入 cache | 减少 L2/L3 miss，约 20-40% 加速 |
| 预计算偏移 (P2/P3) | `exclusive_scan` 替代原子 `offset` 累加 | 消除临界区开销，线性扩展 |
| Stick 局部排序 (P2) | 对要发送的 sticks 按 `ixy` 排序后再批量拷贝 | 改善 `in[ixy * nplane]` 的读取局部性 |
| 合并读写 (P3) | 若 `nplane` 为 2 的幂次，使用 SIMD 批量拷贝 | 约 2-4x 加速（配合题5 SIMD） |

### 4.3 负载均衡直观分析

`count_pw_st` 按 `ix` 分段：各段的计算量取决于该 `ix` 值对应的有效平面波数。由于截断球在 x-y 平面上投影的对称性，中心区域（$i_x \approx 0$）的平面波数较多，边缘区域较少。但差异通常不超过 2x，使用 `schedule(static)` 影响很小。

FFT 重排阶段：sticks 数 $n_{\text{st}}$ 通常在 MPI 进程间已均匀分配（`distribution_method2` 保证），每个进程内的 P2/P3 负载自然均衡。若使用 `distribution_method1`（按平面波数分配），同一个进程内 sticks 的 `st_length` 差异大，但 P2/P3 的数据量与 `nplane`（均匀）而非 `st_length` 相关，故负载仍均衡。

---

## 附录 A: 复杂度速查表

| 算法 | 时间复杂度 | 空间复杂度 | 并行加速比上界 |
|------|-----------|-----------|---------------|
| `count_pw_st` (串行) | $O(N_x N_y N_z)$ | $O(n_{\text{xy}})$ | — |
| `count_pw_st` (并行, 私有数组) | $O(N_x N_y N_z / T + n_{\text{xy}} \cdot T)$ | $O(n_{\text{xy}} \cdot T)$ | $\min(T, N_x N_y N_z / (n_{\text{xy}} \cdot T))$ |
| `count_pw_st` (并行, 原子操作) | $O(N_x N_y N_z / T + n_{\text{pw}})$ | $O(n_{\text{xy}})$ | $\min(T, N_x N_y N_z / n_{\text{pw}})$ |
| `gatherp_scatters_pack` | $O(n_{\text{stot}} \cdot n_{\text{plane}})$ | $O(n_{\text{stot}})$ | $T$ (预计算偏移后) |
| `gathers_scatterp_unpack` | $O(n_{\text{st}} \cdot N_z)$ | $O(1)$ | $T$ |
| `real2recip` 中 P1 填充 | $O(n_{\text{plane}} \cdot n_{\text{xy}})$ | $O(1)$ | $T$ |

---

## 附录 B: 变量命名对照表

| 数学符号 | 代码变量 (C++) | 含义 |
|----------|---------------|------|
| $E_{\text{cut}}$ | `this->ggecut` | 平面波截断能量 (G^2 上界) |
| $\mathbf{M}$ | `this->GGT` | 度规矩阵 ($3 \times 3$) |
| $N_x, N_y, N_z$ | `nx`, `ny`, `nz` | 物理网格维度 |
| $N_x^{\text{fft}}, N_y^{\text{fft}}$ | `fftnx`, `fftny` | FFT 网格 x, y 维度 |
| $n_{\text{xy}}$ | `fftnxy` | $N_x^{\text{fft}} \cdot N_y^{\text{fft}}$ |
| $n_{\text{stot}}$ | `nstot` | 全局 sticks 总数 |
| $n_{\text{pw}}$ | `npwtot` | 全局平面波总数 |
| $n_{\text{st}}$ | `nst` | 本地 sticks 数 |
| $n_{\text{plane}}$ | `nplane` | 本地 z-plane 层数 |
| $L[\text{ixy}]$ | `st_length2D[ixy]` | stick 包含的平面波数 |
| $\text{ixy}$ | `ixy` | x-y 平面网格线性索引 |
| $\text{istot2ixy}$ | `istot2ixy` | 全局 stick → ixy 映射 |
| $\text{is2fftixy}$ | `is2fftixy` | 本地 stick → ixy 映射 |
| $\text{fftixy2ip}$ | `fftixy2ip` | ixy → MPI rank 映射 |
| $T$ | `num_threads` | OpenMP 线程数 |

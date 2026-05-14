# module_pw 算法文档

> **模块**: ABACUS `source/source_basis/module_pw` —— 平面波基组与 FFT 变换  
> **作者**: 算法文档工程师（基于源码与注释自动生成）  
> **版本**: 1.0  
> **约定**: 本分析采用 `xprime=true`（对称轴在 x 方向），Gamma-only 时 `fftnx` 取偶数。

---

## Layer 1: 数学形式化与符号系统

### 1.1 符号表

| 符号 | 代码变量 | 定义 | 类型/范围 |
|------|----------|------|-----------|
| $\Omega$ | `omega` | 晶胞体积 | $\mathbb{R}_{>0}$ |
| $\mathbf{a}_1, \mathbf{a}_2, \mathbf{a}_3$ | `latvec` | 实空间晶格矢量 (以 `lat0` 为单位) | $\mathbb{R}^{3\times 3}$ |
| $\mathbf{b}_1, \mathbf{b}_2, \mathbf{b}_3$ | `G` / `GT` | 倒空间晶格矢量，$\mathbf{b}_i \cdot \mathbf{a}_j = 2\pi\delta_{ij}$ | $\mathbb{R}^{3\times 3}$ |
| $\mathbf{g}$ | `gdirect` | 倒格矢 (direct coordinates) | $\mathbb{Z}^3$ |
| $\mathbf{g}^{\text{car}}$ | `gcar` | 倒格矢 (Cartesian, 单位 $\text{lat0}^{-1}$) | $\mathbb{R}^3$ |
| $\mathbf{k}$ | `kvec_d` / `kvec_c` | Bloch 波矢 (direct/Cartesian) | $\mathbb{R}^3$ |
| $g_{\text{cut}}$ | `gridecut` | FFT 网格截断半径平方，$g_{\text{cut}} = \text{ecut}/(2\pi/\text{lat0})^2$ | $\mathbb{R}_{>0}$ |
| $G_{\text{cut}}$ | `ggecut` | 平面波截断半径平方，$G_{\text{cut}} = \text{ecut}/(2\pi/\text{lat0})^2$ | $\mathbb{R}_{>0}$ |
| $N_x, N_y, N_z$ | `fftnx`/`nx`, `fftny`/`ny`, `nfftz`/`nz` | FFT 网格三维度 | $\mathbb{N}$ |
| $N_{xy}$ | `fftnxy` / `nxy` | $N_x \cdot N_y$，每个 z-plane 的网格点数 | $\mathbb{N}$ |
| $N_{xyz}$ | `nxyz` | $N_x \cdot N_y \cdot N_z$，FFT 网格总点数 | $\mathbb{N}$ |
| $n_{\text{pw}}$ | `npw` | 当前进程的平面波总数（截断球 $|\mathbf{g}|^2 < G_{\text{cut}}$ 内） | $\mathbb{N}$ |
| $n_{\text{pw},k}$ | `npwk[ik]` | k 点 ik 的平面波数（$|\mathbf{g}+\mathbf{k}|^2 < G_{\text{cut}}$ 内） | $\mathbb{N}$ |
| $n_{\text{st}}$ | `nst` | 当前进程拥有的 sticks 数 | $\mathbb{N}$ |
| $n_{\text{st,tot}}$ | `nstot` | 全局 sticks 总数（截断球投影到 $(i_x,i_y)$ 平面的非零点数） | $\mathbb{N}$ |
| $n_{\text{plane}}$ | `nplane` | 当前进程拥有的 z-plane 层数 | $\mathbb{N}$ |
| $n_{\text{proc}}$ | `poolnproc` | pool 内 MPI 进程数 | $\mathbb{N}$ |
| $p$ | `poolrank` | 当前进程在 pool 内的 rank，$p \in [0, n_{\text{proc}})$ | $\mathbb{N}$ |
| $\text{lat0}$ | `lat0` | 晶格长度单位 (Bohr) | $\mathbb{R}_{>0}$ |

### 1.2 平面波基组展开

**实空间与倒空间的 Fourier 关系**：

$$
f(\mathbf{r}) = \frac{1}{\sqrt{\Omega}} \sum_{\mathbf{g}} c(\mathbf{g}) e^{i\mathbf{g}\cdot\mathbf{r}}, \quad
c(\mathbf{g}) = \frac{1}{\sqrt{\Omega}} \int_{\Omega} f(\mathbf{r}) e^{-i\mathbf{g}\cdot\mathbf{r}} d\mathbf{r}
$$

其中 $\mathbf{g} = i_x \mathbf{b}_1 + i_y \mathbf{b}_2 + i_z \mathbf{b}_3$，$(i_x, i_y, i_z) \in \mathbb{Z}^3$，且满足截断条件 $|\mathbf{g}|^2 < G_{\text{cut}}$。

**离散化后**（FFT 网格上的近似）：

$$
c(\mathbf{g}) \approx \frac{1}{N_x N_y N_z} \sum_{i_x=0}^{N_x-1} \sum_{i_y=0}^{N_y-1} \sum_{i_z=0}^{N_z-1} f(\mathbf{r}_{i_x,i_y,i_z}) e^{-i\mathbf{g}\cdot\mathbf{r}_{i_x,i_y,i_z}}
$$

### 1.3 多 k 点等价变换

对于 Bloch 函数 $\psi_{n\mathbf{k}}(\mathbf{r}) = e^{i\mathbf{k}\cdot\mathbf{r}} u_{n\mathbf{k}}(\mathbf{r})$，其中 $u_{n\mathbf{k}}$ 是晶格周期函数，其平面波展开为：

$$
u_{n\mathbf{k}}(\mathbf{r}) = \frac{1}{\sqrt{\Omega}} \sum_{\mathbf{g}} c_{n}(\mathbf{g},\mathbf{k}) e^{i\mathbf{g}\cdot\mathbf{r}}
$$

由此：
$$
\psi_{n\mathbf{k}}(\mathbf{r}) = \frac{1}{\sqrt{\Omega}} \sum_{\mathbf{g}} c_{n}(\mathbf{g},\mathbf{k}) e^{i(\mathbf{g}+\mathbf{k})\cdot\mathbf{r}}
$$

**关键等价变换**（代码中用以统一 FFT 核）：

定义 $f'(\mathbf{r}) = e^{-i\mathbf{k}\cdot\mathbf{r}} f(\mathbf{r})$，则：

$$
f(\mathbf{r}) = \frac{1}{\sqrt{\Omega}} \sum_{\mathbf{g}} c(\mathbf{g},\mathbf{k}) e^{i(\mathbf{g}+\mathbf{k})\cdot\mathbf{r}}
\Longleftrightarrow
f'(\mathbf{r}) = \frac{1}{\sqrt{\Omega}} \sum_{\mathbf{g}} c(\mathbf{g},\mathbf{k}) e^{i\mathbf{g}\cdot\mathbf{r}}
$$

这样 $f'(\mathbf{r})$ 的平面波展开使用纯 $\mathbf{g}$（不显含 $\mathbf{k}$），可直接套用标准 FFT 框架。系数提取时截断条件变为 $|\mathbf{g}+\mathbf{k}|^2 < G_{\text{cut}}$。

### 1.4 Gamma-only 不变式

当计算仅在 $\Gamma$ 点 ($\mathbf{k}=0$) 且波函数为实函数时：

- **对称性**: $c(-\mathbf{g}) = c(\mathbf{g})^*$（Hermitian 对称）
- **存储约束**: 仅需存储半空间平面波。当 `xprime=true` 时，存储 $i_x \geq 0$ 的半空间（含 $i_x=0$ 时 $i_y \geq 0$ 部分）
- **FFT 维度约束**: 由于实数 FFT 的 Hermitian 冗余，对称轴所在维度必须取偶数。`xprime=true` 时 $N_x$ 为偶数；`xprime=false` 时 $N_y$ 为偶数
- **r2c/c2r 路径**: 利用实数到复数的 FFT 变换（`fftxyr2c` / `fftxyc2r`），计算量和存储量约为全复数路径的 $1/2$

---

## Layer 2: 核心数据结构形式化定义

### 2.1 映射 `ig2isz`: 平面波索引 → (stick 索引, z 索引)

- **函数签名**: $\text{ig2isz}: [0, n_{\text{pw}}) \to [0, n_{\text{st}} \cdot N_z)$
- **代码实现**: `int* ig2isz`
- **数学定义**: 设 $\text{ig}$ 为本地平面波编号，对应倒格矢 $(i_x, i_y, i_z)$（以 FFT 网格坐标表示）。令 $i_s = \text{fftixy2is}(i_x + i_y \cdot N_x)$（实际为 `is2fftixy` 的逆映射）为该 $(i_x, i_y)$ 对应的本地 stick 索引，则：

  $$
  \text{ig2isz}[\text{ig}] = i_s \cdot N_z + i_z
  $$

  解码方式: `is = ig2isz[ig] / nz`, `iz = ig2isz[ig] % nz`

- **不变式**:
  1. $\forall \text{ig} \in [0, n_{\text{pw}}), \; 0 \leq \text{ig2isz}[\text{ig}] < n_{\text{st}} \cdot N_z$
  2. $\forall \text{ig}_1 \neq \text{ig}_2$, 若 $\text{ig2isz}[\text{ig}_1] = \text{ig2isz}[\text{ig}_2]$，则两平面波对应相同 $(i_x,i_y,i_z)$，仅在 Gamma-only 且 $c(-\mathbf{g}) \neq c(\mathbf{g})$ 允许（实际不应出现）
  3. 解码后的 `is` 满足 $0 \leq \text{is} < n_{\text{st}}$，且 $\text{is2fftixy}[\text{is}]$ 映射到唯一的 $(i_x, i_y)$

- **与相邻映射的关系**: $\text{ig2isz}$ 与 `is2fftixy` 组合可得完整 $(i_x,i_y,i_z)$:

  $$
  \text{ig} \xrightarrow{\text{ig2isz}} (i_s, i_z) \xrightarrow{\text{is2fftixy}} (i_x, i_y) \longrightarrow (i_x, i_y, i_z)
  $$

### 2.2 映射 `is2fftixy`: stick 索引 → FFT 网格 $(i_x, i_y)$

- **函数签名**: $\text{is2fftixy}: [0, n_{\text{st}}) \to [0, N_x \cdot N_y)$
- **代码实现**: `int* is2fftixy`
- **数学定义**: 将本地 stick 编号 $i_s$ 映射为 FFT 网格中 $(i_x, i_y)$ 的线性索引：

  $$
  \text{is2fftixy}[i_s] = i_y + i_x \cdot \texttt{fftny}
  $$

  解码方式: `ix = is2fftixy[is] / fftny`, `iy = is2fftixy[is] % fftny`

- **不变式**:
  1. $\forall i_s^{(1)} < i_s^{(2)}$，有 $\text{is2fftixy}[i_s^{(1)}] < \text{is2fftixy}[i_s^{(2)}]$（严格单调递增，保证 stick 顺序一致性）
  2. 所有 `is2fftixy[is]` 的值互不相同
  3. 使用 `fftny`（而非 `ny`）作为 stride：`fftny` 是 FFT 库要求的 y 维度，可能因库的对齐要求而大于 $N_y$

- **与相邻映射的关系**: 与 `fftixy2is` 互为逆映射（仅在本地 sticks 的定义域内）:
  $\text{fftixy2is}[\text{is2fftixy}[i_s]] = i_s$

### 2.3 映射 `istot2ixy`: 全局 stick 编号 → $(i_x, i_y)$

- **函数签名**: $\text{istot2ixy}: [0, n_{\text{st,tot}}) \to [0, N_x \cdot N_y)$
- **代码实现**: `int* istot2ixy`
- **数学定义**: 全局 stick 编号 $i_{\text{stot}}$ 到 $(i_x, i_y)$ 网格坐标的线性索引。该映射由 distribution 算法在全局范围内统一确定：
  - 遍历所有满足截断条件的 $(i_x, i_y)$
  - 按 $(i_x, i_y)$ 的某种顺序（`distributeg_method1` 按 stick 长度排序；`distributeg_method2` 按 $(i_x, i_y)$ 的扫描顺序）排列为全局 stick 列表
  - $\text{istot2ixy}[i_{\text{stot}}] = i_y + i_x \cdot \texttt{fftny}$

- **不变式**:
  1. $\forall i_{\text{stot}}$, $\text{istot2ixy}[i_{\text{stot}}]$ 对应截断球内唯一的 $(i_x, i_y)$
  2. 对于 `PW_Basis_Sup`（dense grid），$\text{istot2ixy}$ 的顺序必须继承 smooth grid 的 `istot2ixy` 顺序（即 dense 的全局 stick 顺序是 smooth 的全局 stick 顺序的超集）

- **与相邻映射的关系**: 全局 stick 到进程的分配由 `fftixy2ip` 决定：stick $i_{\text{stot}}$ 属于进程 $p = \text{fftixy2ip}[\text{istot2ixy}[i_{\text{stot}}]]$

### 2.4 映射 `fftixy2ip`: $(i_x, i_y)$ → MPI rank

- **函数签名**: $\text{fftixy2ip}: [0, N_x \cdot N_y) \to [0, n_{\text{proc}})$
- **代码实现**: `int* fftixy2ip`
- **数学定义**: 对于 FFT 网格中的每个 $(i_x, i_y)$，指定负责该 stick 的 MPI 进程 rank。非 stick 位置（截断球外的 $(i_x, i_y)$）通常标记为 $-1$ 或未定义。

- **不变式**:
  1. $\forall i_{\text{stot}} \in [0, n_{\text{st,tot}})$，令 $\text{ixy} = \text{istot2ixy}[i_{\text{stot}}]$，则 $\text{fftixy2ip}[\text{ixy}] \in [0, n_{\text{proc}})$
  2. 负载均衡约束:
     - Method 1: 各进程的 $\sum_{i_s} \text{st\_length2D}(i_s)$ 尽量均衡
     - Method 2: 各进程的 `nst_per[p]` 尽量均衡（$\max_p \text{nst\_per}[p] - \min_p \text{nst\_per}[p] \leq 1$）

- **与相邻映射的关系**: 与 `istot2ixy` 和 stick 分配算法（`nst_per`, `npw_per`）共同决定数据分布拓扑

### 2.5 `startz`, `numz`: z-plane 分布

- **函数签名**:
  - $\text{numz}: [0, n_{\text{proc}}) \to \mathbb{N}$，每进程的 z-plane 数量
  - $\text{startz}: [0, n_{\text{proc}}) \to [0, N_z)$，每进程的 z 起始层索引
- **代码实现**: `int* startz`, `int* numz`
- **数学定义**: 将 $N_z$ 层 z-plane 尽可能均匀地分配给 $n_{\text{proc}}$ 个进程。设 $q = \lfloor N_z / n_{\text{proc}} \rfloor$，$r = N_z \bmod n_{\text{proc}}$，则：

  $$
  \text{numz}[p] = \begin{cases} q + 1 & \text{if } p < r \\ q & \text{otherwise} \end{cases}
  $$

  $$
  \text{startz}[0] = 0, \quad \text{startz}[p+1] = \text{startz}[p] + \text{numz}[p]
  $$

  进程 $p$ 拥有 z-plane 层 $\text{startz}[p]$ 到 $\text{startz}[p] + \text{numz}[p] - 1$。

- **不变式**:
  1. $\sum_{p=0}^{n_{\text{proc}}-1} \text{numz}[p] = N_z$
  2. 各进程区间无重叠、无间隙
  3. 当前进程 $p$: `nplane = numz[p]`, `startz_current = startz[p]`

- **边界情况**: 若 $n_{\text{proc}} > N_z$，则部分进程 $\text{numz}[p] = 0$（即 `nplane=0`），这些进程在 FFT 和 Alltoallv 中不持有实空间数据

### 2.6 `numg`, `numr`, `startg`, `startr`: MPI_Alltoallv 参数

- **函数签名**: 均为 $[0, n_{\text{proc}}) \to \mathbb{N}$
- **代码实现**: `int* numg`, `int* numr`, `int* startg`, `int* startr`

- **数学定义**:
  - `numg[ip]`: 当前进程发送给进程 `ip` 的数据量（以复数为单位）

    $$
    \text{numg}[ip] = \text{nst\_per}[\text{poolrank}] \times \text{numz}[ip]
    $$

    其中 `nst_per[poolrank]` 是当前进程拥有的 sticks 数（$n_{\text{st}}$），`numz[ip]` 是目标进程 `ip` 的 z-plane 数。

  - `numr[ip]`: 当前进程从进程 `ip` 接收的数据量

    $$
    \text{numr}[ip] = \text{nst\_per}[ip] \times \text{numz}[\text{poolrank}]
    $$

  - `startg[ip]` / `startr[ip]`: 发送/接收缓冲区中的起始偏移

    $$
    \text{startg}[0] = 0, \quad \text{startg}[ip] = \sum_{j=0}^{ip-1} \text{numg}[j]
    $$
    $$
    \text{startr}[0] = 0, \quad \text{startr}[ip] = \sum_{j=0}^{ip-1} \text{numr}[j]
    $$

- **一致性约束**:
  1. $\sum_{p=0}^{n_{\text{proc}}-1} \text{numg}[p] = n_{\text{st,tot}} \cdot N_z$（全局数据守恒）
  2. $\sum_{p=0}^{n_{\text{proc}}-1} \text{numr}[p] = n_{\text{st,tot}} \cdot N_z$

### 2.7 补充映射 `igl2isz_k` (PW_Basis_K)

- **函数签名**: $\text{igl2isz\_k}: [0, n_{\text{pw},k}) \times [0, n_{\text{ks}}) \to [0, n_{\text{st}} \cdot N_z)$
- **代码实现**: `int* igl2isz_k`
- **数学定义**: 对于 k 点 `ik`，本地平面波索引 `igl` 到 `(stick, z)` 的映射。与 `ig2isz` 的关系：$|\mathbf{g}+\mathbf{k}|^2 < G_{\text{cut}}$ 是 $|\mathbf{g}|^2 < G_{\text{cut}}$ 的子集，因此 `igl2isz_k[ik][igl]` 的值必出现在 `ig2isz` 的某个条目中。

- **辅助映射 `igl2ig_k`**:
  - $\text{igl2ig\_k}[ik][igl]$: k 点 `ik` 的第 `igl` 个平面波对应全局 `ig` 索引，满足 $\text{ig2isz}[\text{igl2ig\_k}[ik][igl]] = \text{igl2isz\_k}[ik][igl]$

---

## Layer 3: 算法伪代码与流程图

### 3.1 `initgrids`: FFT 网格初始化

```
算法: initgrids —— FFT 网格维度确定
输入:
    lat0:     晶格长度单位 (Bohr)
    latvec:   3×3 晶格矢量矩阵 (以 lat0 为单位)
    gridecut: FFT 网格截断能量 (Ry)
    (或直接指定 N1, N2, N3)
输出:
    无显式输出，设置以下成员变量:
    - nx, ny, nz: 最终 FFT 网格维度
    - nxy, nxyz: Nx*Ny, Nx*Ny*Nz
    - G, GT, GGT: 倒空间变换矩阵
前置条件:
    - lat0 > 0, latvec 非奇异, gridecut >= 0
后置条件:
    - nx, ny, nz 均为 2^a·3^b·5^c·7^d 形式的"好"数（FFT 高效）
    - nx, ny, nz 满足: 截断球内所有 G 点均可被 FFT 网格分辨
    - gamma_only 且 xprime==true 时 nx 为偶数
    - (可选) full_pw 模式下各维度加 1
时间复杂度: O(Nx·Ny·Nz)（扫描全网格求最大 |G| 分量）
通信复杂度: 无

步骤:
    // 1. 构建倒空间变换矩阵
    G = 2π * latvec^{-T}          // 倒格矢的 Cartesian 表示
    GT = G^T
    GGT = G * G^T                 // 度量矩阵: |g|^2 = f·(GGT·f)

    // 2. 由 gridecut 计算最小 i_box（每个方向上的最大 Miller 指数）
    //    |g|^2 = (i_x b_1 + i_y b_2 + i_z b_3)^2 < gridecut
    对每个方向 d ∈ {x, y, z}:
        i_box[d] = ceil( max_{|g|^2 < gridecut} |i_d| )

    // 3. 扫描所有满足 |i_d| ≤ i_box[d] 的 (i_x, i_y, i_z)，计算最大 |i_x|, |i_y|, |i_z|
    max_x = max_y = max_z = 0
    for ix in [-i_box[0], i_box[0]]:
        for iy in [-i_box[1], i_box[1]]:
            for iz in [-i_box[2], i_box[2]]:
                if |g(ix,iy,iz)|^2 < gridecut:
                    max_x = max(max_x, abs(ix))
                    max_y = max(max_y, abs(iy))
                    max_z = max(max_z, abs(iz))

    // 4. 搜索满足 2^a·3^b·5^c·7^d 可分解的最小 FFT 维度
    //    且 ≥ 2 * max_d + 1 以保证 Nyquist 采样
    nx = find_nearest_2357(2 * max_x + 1)
    ny = find_nearest_2357(2 * max_y + 1)
    nz = find_nearest_2357(2 * max_z + 1)

    // 5. Gamma-only 奇偶性处理
    if gamma_only:
        if xprime == true:
            nx = (nx % 2 == 0) ? nx : nx + 1    // nx 取偶数
        else:
            ny = (ny % 2 == 0) ? ny : ny + 1    // ny 取偶数

    // 6. full_pw 模式: 各维度加 1（用于某些边界条件处理）
    if full_pw:
        nx += 1; ny += 1; nz += 1

    // 7. 设置派生变量
    nxy = nx * ny
    nxyz = nx * ny * nz
    fftny = gamma_only ? ny : ny（实际用 FFT 库对齐后的值）
```

### 3.2 `distributeg_method1`: 按 plane wave 数分布 sticks

```
算法: distributeg_method1 —— 按 plane wave 总数均衡分布 sticks
输入:
    nstot:          全局 sticks 总数
    st_length2D[]:  每个全局 stick 的 z 方向长度（即该 (ix,iy) 上截断球内的 z 层数）
    poolnproc:      pool 内进程数
输出:
    nst_per[0..poolnproc-1]:   各进程分配的 sticks 数量
    npw_per[0..poolnproc-1]:   各进程分配的 plane wave 数量
    fftixy2ip[0..Nx·Ny-1]:     (ix,iy) → rank 映射
    istot2ixy[0..nstot-1]:     全局 stick → (ix,iy) 映射
前置条件:
    - nstot ≥ poolnproc (保证每进程至少 1 个 stick)
    - st_length2D[i] > 0 for all i
后置条件:
    - 各进程的 npw_per[p] (即 ∑_{i∈进程p} st_length2D[i]) 尽量均衡
    - 所有 sticks 恰好分配一次
时间复杂度: O(nstot · log nstot)（排序主导）
通信复杂度: 无（本地计算，但需前置的全局 sticks 信息收集）

步骤:
    // 1. 统计各 (ix,iy) 的 stick 长度 st_length2D，并收集全局 stick 信息
    //    (此步通常在分布前的扫描中完成)

    // 2. 构建 stick 列表: (ixy, length) 对，按 length 降序排序
    stick_list = [(ixy[i], st_length2D[i]) for i in range(nstot)]
    sort stick_list by length DESCENDING
    // 记录排序后的 istot2ixy: 即 stick_list[i].ixy

    // 3. 贪心分配: 每次将当前最长的 stick 分配给 plane wave 总数最少的进程
    for p in [0, poolnproc):
        npw_per[p] = 0
        nst_per[p] = 0
        initialize empty stick list for proc p

    for each (ixy, length) in stick_list:
        // 找到 npw_per[] 最小的进程
        p_min = argmin(npw_per)
        将该 stick 分配至进程 p_min:
            fftixy2ip[ixy] = p_min
            npw_per[p_min] += length
            nst_per[p_min] += 1

    // 4. 建立每个进程的本地 stick 映射 (is2fftixy, ig2isz 等)
    //    （在 distribute_g() 后续步骤中完成）
```

### 3.3 `distributeg_method2`: 按 sticks 数均衡分布

```
算法: distributeg_method2 —— 按 sticks 数量均衡分布
输入:
    nstot:       全局 sticks 总数
    poolnproc:   pool 内进程数
    (ixy 扫描顺序隐式确定)
输出:
    nst_per[0..poolnproc-1]:   各进程分配的 sticks 数量
    fftixy2ip[0..Nx·Ny-1]:     (ix,iy) → rank 映射
    istot2ixy[0..nstot-1]:     全局 stick → (ix,iy) 映射
前置条件:
    - nstot ≥ poolnproc
后置条件:
    - max_p nst_per[p] - min_p nst_per[p] ≤ 1 (sticks 数严格均衡)
    - sticks 按 (ix,iy) 扫描顺序轮询分配
时间复杂度: O(nstot)
通信复杂度: 无

步骤:
    // 1. 按 (ix,iy) 的顺序统计所有有效的 sticks
    //    扫描顺序: for ix in [0, Nx): for iy in [0, Ny):
    //    仅保留截断球内的 (ix,iy)，建立 istot2ixy 列表
    stick_count = 0
    for ix in [0, Nx):
        for iy in [0, Ny):
            if (ix,iy) 在截断球内有有效 z 分量:
                istot2ixy[stick_count] = iy + ix * fftny
                stick_count += 1
    // nstot = stick_count

    // 2. 轮询分配: 按 istot2ixy 顺序依次分配给各进程
    for p in [0, poolnproc):
        nst_per[p] = floor(nstot / poolnproc)
    remainder = nstot % poolnproc
    for p in [0, remainder):
        nst_per[p] += 1

    // 3. 为每个进程分配具体的 sticks
    current_stick = 0
    for p in [0, poolnproc):
        for i in [0, nst_per[p]):
            ixy = istot2ixy[current_stick]
            fftixy2ip[ixy] = p
            current_stick += 1

    // 4. 计算 npw_per[p]（可选，用于负载分析）
    for p in [0, poolnproc):
        npw_per[p] = 0
        for each stick owned by p:
            npw_per[p] += st_length2D[stick]
```

### 3.4 `distributer`: z-plane 划分

```
算法: distributer —— z-plane 均匀划分
输入:
    nz:         FFT 网格 z 维度
    poolnproc:  pool 内进程数
    poolrank:   当前进程 rank
输出:
    startz[0..poolnproc-1]:  各进程的 z 起始层
    numz[0..poolnproc-1]:    各进程的 z 层数
    nplane:                  当前进程的 z 层数 (= numz[poolrank])
    nrxx:                    当前进程实空间数据量 (= nplane * nxy)
前置条件:
    - nz ≥ 1, poolnproc ≥ 1
后置条件:
    - Σ_p numz[p] = nz
    - 区间 [startz[p], startz[p]+numz[p]) 互不相交且覆盖 [0, nz)
    - 若 poolnproc > nz: 多余进程 nplane=0, 其 numg/numr 相应为 0
时间复杂度: O(poolnproc)
通信复杂度: 无

步骤:
    npz = nz / poolnproc        // 整数除法: 每进程基础 z 层数
    modz = nz % poolnproc       // 余数: 前 modz 个进程多 1 层
    startz[0] = 0
    for ip in [0, poolnproc):
        numz[ip] = npz
        if ip < modz:
            numz[ip] += 1
        if ip < poolnproc - 1:
            startz[ip+1] = startz[ip] + numz[ip]
        if ip == poolrank:
            nplane = numz[ip]
            startz_current = startz[ip]
    nrxx = numz[poolrank] * nxy
```

### 3.5 `real2recip` (CPU 路径): 实空间 → 倒空间

```
算法: real2recip (CPU 路径)
输入:
    in[0..nrxx-1]:    实空间数据 (复数或实数，按 (z, x, y) 或 GPU 兼容布局)
    (可选) add:       是否累加到输出
    (可选) factor:    累加时的缩放因子
输出:
    out[0..npw-1]:    倒空间平面波系数 c(g)
前置条件:
    - nplane == numz[my_rank]
    - 输入数据布局与 FFT 库约定一致
    - ig2isz 和 is2fftixy 已正确建立
    - fft_bundle 已初始化
后置条件:
    - forall ig in [0, npw), out[ig] = (1/Nxyz) * Σ_r in[r] * exp(-i g · r)
    - 若 gamma_only: c(-g) = c(g)^* 隐含满足
    - ig2isz[ig] 定位的 auxg 元素包含 FFT 结果的对应系数
时间复杂度: O(Nx·Ny·Nz · log(Nx·Ny·Nz))
通信复杂度: 1 次 MPI_Alltoallv，数据量 ≈ nstot · Nz · sizeof(complex)

步骤:
    // ── 非 Gamma-only 路径 ──
    // 1. 填充实空间缓冲区 auxr (若 gamma_only 则用 r2c 路径)
    for ir in [0, nrxx):
        auxr[ir] = in[ir]        // 直接拷贝（复数输入）

    // 1a. Gamma-only 实数输入路径:
    //     将实数数据填充到 rspace 缓冲区（nx 维度减半存储）
    //     for ix in [0, nx): for ipy in [0, npy): rspace[ix*npy+ipy] = in[...]
    //     fftxyr2c(rspace, auxr)  // 实数→复数 2D FFT

    // 2. x-y 方向 2D FFT (正变换: r → g_xy)
    //    每个进程对其拥有的 nplane 层 planes 独立执行
    fftxyfor(auxr, auxr)          // in-place: auxr[iz] 层完成 x-y FFT
    //    此时 auxr 按 z-plane 分块，每层已完成 x-y 方向的 Fourier 变换

    // 3. planes → sticks 重排 + MPI 全局交换
    //    通过 istot2ixy 和 fftixy2ip 将 (iz, ixy) 数据重排为按 stick 连续
    gatherp_scatters(auxr, auxg)
    //    内部: MPI_Alltoallv(auxr, numg, startg, ..., auxg, numr, startr, ...)
    //    之后 auxg 按 sticks 分块: 第 is 个 stick 的 Nz 个 z 层连续存储

    // 4. z 方向 1D FFT (正变换: g_xy → g)
    //    每个进程对其拥有的 nst 个 sticks 独立执行
    fftzfor(auxg, auxg)           // in-place: 每个 stick 的 Nz 层完成 z 方向 FFT
    //    此时 auxg 包含完整的 3D FFT 结果，按 (stick, z) 排列

    // 5. 按 ig2isz 提取截断球内的 npw 个平面波系数
    tmpfac = 1.0 / nxyz           // 归一化因子
    for ig in [0, npw):
        if add:
            out[ig] += factor * tmpfac * auxg[ig2isz[ig]]
        else:
            out[ig] = tmpfac * auxg[ig2isz[ig]]
```

### 3.6 `recip2real` (CPU 路径): 倒空间 → 实空间

```
算法: recip2real (CPU 路径) —— real2recip 的严格逆操作
输入:
    in[0..npw-1]:     倒空间平面波系数 c(g)
    (可选) add:       是否累加到输出
    (可选) factor:    累加时的缩放因子
输出:
    out[0..nrxx-1]:   实空间数据 (复数或实数)
前置条件:
    - ig2isz 已正确建立
    - fft_bundle 已初始化
后置条件:
    - 若输入为 real2recip 的输出（未截断），则 out ≈ 原始输入（机器精度内）
    - forall ir in [0, nrxx), out[ir] = Σ_g in[ig] * exp(i g · r_ir)（不含 1/√Ω 归一化）
时间复杂度: O(Nx·Ny·Nz · log(Nx·Ny·Nz))
通信复杂度: 1 次 MPI_Alltoallv，数据量 ≈ nstot · Nz · sizeof(complex)

步骤:
    // 1. 清零 auxg 缓冲区
    for i in [0, nst * nz):
        auxg[i] = 0

    // 2. 按 ig2isz 写入截断球内的平面波系数
    for ig in [0, npw):
        auxg[ig2isz[ig]] = in[ig]
    //    其余位置保持 0（对应截断球外的频率分量）

    // 3. z 方向 1D IFFT (逆变换: g → g_xy)
    fftzbac(auxg, auxg)          // in-place: 每个 stick 的 Nz 层完成 z 方向逆 FFT

    // 4. sticks → planes 重排 + MPI 全局交换
    gathers_scatterp(auxg, auxr)
    //    内部: MPI_Alltoallv(auxg, numr, startr, ..., auxr, numg, startg, ...)
    //    注意 send/recv 的角色互换: 各进程将其持有的 sticks 数据发回原 z-plane 进程

    // 5. x-y 方向 2D IFFT (逆变换: g_xy → r)
    fftxybac(auxr, auxr)         // in-place: auxr 每层完成 x-y 方向逆 FFT

    // 5a. Gamma-only 实数输出路径:
    //     fftxyc2r(auxr, rspace)  // 复数→实数 2D 逆 FFT
    //     for ix in [0, nx): for ipy in [0, npy): out[ix*npy+ipy] = rspace[...]

    // 6. 输出实空间数据
    for ir in [0, nrxx):
        if add:
            out[ir] += factor * auxr[ir]       // 复数输出(取实部若实数输入)
        else:
            out[ir] = auxr[ir]
```

### 3.7 `k_point_mapping` (PW_Basis_K): 构造 $G+k$ 截断映射

```
算法: setupIndGk —— k 点平面波截断映射构造
输入:
    npw:           全局平面波数 (|g|^2 < G_cut)
    nks:           pool 内 k 点数
    kvec_c[ik]:    Cartesian k 矢量
    ggecut:        截断能量阈值 (G+K)^2 < ggecut
输出:
    npwk[0..nks-1]:          各 k 点的平面波数
    npwk_max:                所有 k 点中最大的 npwk
    igl2isz_k[0..npwk_max*nks-1]:  (igl, ik) → (is, iz) 映射
    igl2ig_k[0..npwk_max*nks-1]:   (igl, ik) → ig 映射
前置条件:
    - PW_Basis::ig2isz 已建立（全局 |g|^2 截断）
    - kvec_c 已设置
后置条件:
    - forall ik, igl: |g_{igl2ig_k[ik][igl]} + k|^2 < ggecut
    - igl2isz_k[ik][igl] 解码后必在 PW_Basis::ig2isz 的定义域内
时间复杂度: O(npw) 每 k 点
通信复杂度: 无

步骤:
    for ik in [0, nks):
        npwk[ik] = 0
        // 遍历 PW_Basis 的全局平面波列表
        for ig in [0, npw):
            // 解码 ig → (ix, iy, iz) → Cartesian G
            isz = ig2isz[ig]
            iz = isz % nz
            is = isz / nz
            ixy = is2fftixy[is]
            ix = ixy / fftny
            iy = ixy % fftny
            // 折叠到 [-nx/2, nx/2) 范围
            if ix >= nx/2 + 1: ix -= nx
            if iy >= ny/2 + 1: iy -= ny
            if iz >= nz/2 + 1: iz -= nz

            G_car = ix * b1 + iy * b2 + iz * b3
            G_plus_k = G_car + kvec_c[ik]
            gk2 = |G_plus_k|^2

            if gk2 < ggecut:
                igl = npwk[ik]
                igl2isz_k[ik * npwk_max + igl] = isz  // 复用 ig2isz 的编码值
                igl2ig_k[ik * npwk_max + igl] = ig     // 记录对应全局 ig
                npwk[ik]++
        npwk_max = max(npwk_max, npwk[ik])
```

---

## Layer 4: 并行策略与通信分析

### 4.1 数据分布模型

`module_pw` 采用 **2D 数据分布**策略：实空间按 z-planes 划分，倒空间按 sticks 划分。

#### 实空间视图（z-plane 分布）

- 每个进程持有 $N_z$ 个 z-planes 中的连续一段：`startz[p]` 到 `startz[p]+numz[p]-1`
- 每层 plane 是完整的 $N_x \times N_y$ 二维网格
- 进程 $p$ 的实空间数据量: $\texttt{nrxx} = \text{numz}[p] \cdot N_x \cdot N_y$

```
        Nx →                    Nx →
   ┌─────────────┐        ┌─────────────┐
Ny │  plane 0    │  proc0 Ny │  plane 0    │
↓  │  plane 1    │        ↓  │  plane 1    │
   │  plane 2    │  proc1   │  plane 2    │
   │  plane 3    │           │  plane 3    │
   └─────────────┘        └─────────────┘
     z-plane 连续段           z-plane 连续段
```

#### 倒空间视图（stick 分布）

- 每个 $(i_x, i_y)$ 对应一根 stick（贯穿所有 $N_z$ 层的列）
- 截断球内的有效 sticks 通过 `fftixy2ip` 分配给各进程
- 进程 $p$ 的倒空间数据量: $\text{nst\_per}[p] \cdot N_z$ 个复数

```
        sticks →               sticks →
   ┌──────────────────┐   ┌──────────────────┐
Nz │ stick_0 (proc0)  │ Nz│ stick_3 (proc1)  │
↓  │ stick_1 (proc0)  │ ↓ │ stick_4 (proc1)  │
   │ stick_2 (proc0)  │   │ stick_5 (proc1)  │
   └──────────────────┘   └──────────────────┘
```

### 4.2 MPI_Alltoallv 通信详解

`real2recip` 和 `recip2real` 中各包含一次全局通信，用于在 planes 视图和 sticks 视图之间切换。

#### `gatherp_scatters` (planes → sticks)

- **语义**: "Gather planes, scatter sticks"
- **操作**: 各进程将其 z-plane 数据按 stick 所属关系发往对应进程
- **数据流**:

  进程 $p$ 发送: $\text{numg}[q] = \text{nst\_per}[p] \cdot \text{numz}[q]$ 个元素给进程 $q$（即进程 $p$ 的所有 sticks 在进程 $q$ 的 z-planes 上的数据片段）

  进程 $p$ 接收: $\text{numr}[q] = \text{nst\_per}[q] \cdot \text{numz}[p]$ 个元素来自进程 $q$（即进程 $q$ 的 sticks 上属于进程 $p$ 的 z-planes 的数据）

#### `gathers_scatterp` (sticks → planes)

- **语义**: "Gather sticks, scatter planes" (`gatherp_scatters` 的逆操作)
- **数据流**: send 和 recv 的角色互换，`numg` 和 `numr` 的定义也互换
- **一致性**: `gathers_scatterp` 使用的 send/recv 计数必须与 `gatherp_scatters` 互为转置

#### 通信量上限

总通信量（每个 Alltoallv）:

$$
T_{\text{comm}} \approx \alpha \cdot n_{\text{proc}} + \beta \cdot n_{\text{st,tot}} \cdot N_z \cdot \text{sizeof(complex)}
$$

其中 $\alpha$ 为消息延迟（约 $\mu s$ 量级），$\beta$ 为每字节传输时间的倒数（带宽）。

每个进程的发送量和接收量均为 $\text{nst\_per}[p] \cdot N_z$ 量级（经过调度后各进程基本均衡）。

### 4.3 负载均衡分析

#### Method 1（按 plane wave 数均衡）

- **目标**: 各进程的 $n_{\text{pw}}$（即 $\sum_{\text{stick}} \text{st\_length2D}$）尽量接近
- **策略**: 将 sticks 按长度降序排列，贪心分配给当前 plane wave 总数最少的进程
- **适用场景**: sticks 长度差异大（截断球在 $(i_x, i_y)$ 投影不均匀，尤其是原点附近 stick 极长）
- **优点**: FFT 沿 z 方向的计算量与 $n_{\text{pw}}$ 成正比，均衡 plane wave 数即均衡 FFT 计算量
- **缺点**: 各进程 sticks 数可能不均衡，导致 Alltoallv 通信量不均衡（通信量正比于 `nst_per[p] * Nz`）

#### Method 2（按 stick 数均衡）

- **目标**: 各进程的 $n_{\text{st}}$ 差值不超过 1
- **策略**: 按 $(i_x, i_y)$ 扫描顺序轮询分配
- **适用场景**: sticks 长度相对均匀（大截断球、均匀网格时 sticks 长度差异小），或通信开销主导时
- **优点**: Alltoallv 通信量各进程严格均衡（因 `numg/numr` 正比于 `nst_per[p]`）
- **缺点**: 若存在极长 stick（通过原点），该 stick 所在进程 FFT 计算量显著高于其他进程

#### 极端情况

- **最大负载不均衡**: 原点 stick 长度 = $N_z$，而其他 sticks 长度远小于 $N_z$。Method 2 将原点 stick 分配给某一进程后，该进程计算量 $\approx N_z \cdot \text{nst\_per}$，而其他进程 $\approx \text{avg\_st\_length} \cdot \text{nst\_per}$，不均衡因子可达 $N_z / \text{avg\_st\_length}$
- **Method 1 应对**: 将极长 stick 单独分配给某一进程后，该进程虽然 FFT 重但只持有少量 sticks，通信开销降低，整体时间可能更优
- **z-plane 分布**: `distributer` 保证各进程 z-plane 数差 ≤ 1，计算量均衡。但当 $n_{\text{proc}} > N_z$ 时，部分进程 `nplane=0`，不参与实空间 FFT

### 4.4 PW_Basis_K 的额外并行约束

- **继承性**: `PW_Basis_K` 继承自 `PW_Basis`，sticks 分布和 z-plane 分布完全继承，通信拓扑（`numg`, `numr`, `startg`, `startr`）不变
- **差异**: 仅 `igl2isz_k` 和 `igl2ig_k` 在不同 k 点间变化（每个 k 点的截断球是 $G_{\text{cut}}$ 截断球的一个子集）
- **内存**: `igl2isz_k` 存储所有 k 点的映射，大小为 $n_{\text{ks}} \times \text{npwk\_max}$，可能显著大于 `ig2isz`
- **计算顺序**: 通常遍历 k 点依次调用 `real2recip(in, out, ik)`，每次使用对应 ik 的 `igl2isz_k` 和 `npwk[ik]`

### 4.5 GPU 路径约束

- **限制**: 当前 GPU 版本要求 `poolnproc == 1`（每个 GPU 一个进程），不涉及 stick 分布的 MPI 通信
- **特点**: 数据保留在 GPU 内存中，`fftxyfor`、`fftzfor` 等使用 CUDA/ROCm FFT 库
- **映射**: `ig2ixyz_gpu` 直接将 ig 映射到 3D FFT box 的线性索引，绕过 stick 表示
- **数据传输**: CPU↔GPU 的数据传输发生在 `real2recip`/`recip2real` 的入口和出口，而非 FFT 中间步骤

### 4.6 PW_Basis_Sup 顺序一致性不变式

`PW_Basis_Sup`（dense grid）的 `ig2isz` 和 `is2fftixy` 必须**继承** smooth grid 的 stick 顺序。具体而言：

- Smooth grid 的 `istot2ixy` 定义了全局 stick 的排序
- Dense grid 的 `istot2ixy` 必须是该排序的扩展（新增的 $(i_x, i_y)$ sticks 追加在末尾）
- 违反此不变式会导致 USPP（超软赝势）计算中的 augmented charge 的 stick 对应关系出错

---

## 附录 A: 复杂度速查表

| 算法 | 时间复杂度 | 通信复杂度 | 空间复杂度 |
|------|-----------|-----------|-----------|
| `initgrids` | $O(N_x N_y N_z)$ | 无 | $O(N_x N_y N_z)$（临时扫描） |
| `distributeg_method1` | $O(n_{\text{st,tot}} \log n_{\text{st,tot}})$ | 无（需全局 sticks 信息） | $O(n_{\text{st,tot}})$ |
| `distributeg_method2` | $O(n_{\text{st,tot}})$ | 无 | $O(n_{\text{st,tot}})$ |
| `distributer` | $O(n_{\text{proc}})$ | 无 | $O(n_{\text{proc}})$ |
| `getstartgr` | $O(n_{\text{proc}})$ | 无 | $O(n_{\text{proc}})$ |
| `real2recip` | $O(N_x N_y N_z \log(N_x N_y N_z))$ | $O(n_{\text{st,tot}} \cdot N_z)$ | $O(N_x N_y N_z)$ |
| `recip2real` | 同上 | 同上 | 同上 |
| `collect_local_pw` | $O(n_{\text{pw}})$ | 无 | $O(n_{\text{pw}})$ |
| `collect_uniqgg` | $O(n_{\text{pw}} \log n_{\text{pw}})$ | 无 | $O(n_{\text{pw}})$ |
| `setupIndGk` (k_point_mapping) | $O(n_{\text{pw}})$ 每 k 点 | 无 | $O(n_{\text{ks}} \cdot \text{npwk\_max})$ |
| `setuptransform` | $O(n_{\text{proc}}) + \text{distribute} + \text{FFT setup}$ | 无 | $O(N_x N_y N_z)$ |

> 注: $N_x N_y N_z$ 通常是 $O(n_{\text{pw}})$ 至 $O(n_{\text{pw}}^{3/2})$ 量级。

---

## 附录 B: 变量命名对照表

| 数学符号 | 代码变量 (C++) | 物理/数学含义 |
|----------|---------------|--------------|
| $N_x, N_y, N_z$ | `fftnx`/`nx`, `fftny`/`ny`, `nfftz`/`nz` | FFT 网格三维度 |
| $N_{xy}$ | `fftnxy` / `nxy` | $N_x \cdot N_y$ |
| $N_{xyz}$ | `nxyz` | $N_x \cdot N_y \cdot N_z$ |
| $n_{\text{pw}}$ | `npw` | 当前进程平面波数（$$|\mathbf{g}|^2 < G_{\text{cut}}$$） |
| $n_{\text{pw},k}$ | `npwk[ik]` | k 点 ik 的平面波数 |
| $n_{\text{st}}$ | `nst` / `nst_per[poolrank]` | 当前进程 sticks 数 |
| $n_{\text{st,tot}}$ | `nstot` | 全局 sticks 数 |
| $n_{\text{plane}}$ | `nplane` | 当前进程 z-plane 层数 |
| $\text{ig2isz}$ | `ig2isz` | 平面波 ig → (stick, z) 映射数组 |
| $\text{is2fftixy}$ | `is2fftixy` | 本地 stick → (ix, iy) 线性索引 |
| $\text{istot2ixy}$ | `istot2ixy` | 全局 stick → (ix, iy) 线性索引 |
| $\text{fftixy2ip}$ | `fftixy2ip` | (ix, iy) 网格点 → MPI rank |
| $\text{startz}[p]$ | `startz[p]` | 进程 p 的 z 起始层 |
| $\text{numz}[p]$ | `numz[p]` | 进程 p 的 z 层数 |
| $\text{startz\_current}$ | `startz_current` | 当前进程的 z 起始层 |
| $\text{numg}[p]$ | `numg[p]` | Alltoallv: 发给 p 的数据量 |
| $\text{numr}[p]$ | `numr[p]` | Alltoallv: 从 p 接收的数据量 |
| $\text{startg}[p]$ | `startg[p]` | Alltoallv: 发送缓冲区的起始偏移 |
| $\text{startr}[p]$ | `startr[p]` | Alltoallv: 接收缓冲区的起始偏移 |
| $\text{nst\_per}[p]$ | `nst_per[p]` | 进程 p 拥有的 sticks 数 |
| $\text{npw\_per}[p]$ | `npw_per[p]` | 进程 p 拥有的平面波数 |
| $\text{igl2isz\_k}$ | `igl2isz_k` | k 点下 (igl, ik) → (is, iz) |
| $\text{igl2ig\_k}$ | `igl2ig_k` | k 点下 (igl, ik) → ig |
| $\text{gg}$ | `gg` | 当前进程各平面波的 $$|\mathbf{g}|^2$$ |
| $\text{gg\_uniq}$ | `gg_uniq` | 去重排序后的 $$|\mathbf{g}|^2$$ |
| $\text{ngg}$ | `ngg` | 不同 $$|\mathbf{g}|^2$$ 值的数量 |
| $g_{\text{cut}}$ | `gridecut` | FFT 网格截断半径平方 |
| $G_{\text{cut}}$ | `ggecut` | 平面波截断半径平方 |
| $\Omega$ | `omega` | 晶胞体积 |
| $\text{lat0}$ | `lat0` | 晶格长度单位 (Bohr) |
| $\mathbf{G}$ | `G` | 倒格矢变换矩阵 (3×3) |
| $\mathbf{G}^T\mathbf{G}$ | `GGT` | 度量矩阵 (3×3) |
| $\mathbf{k}$ | `kvec_d` / `kvec_c` | Bloch 波矢 (direct / Cartesian) |
| `nrxx` | `nrxx` | 当前进程实空间点数 = `nplane * nxy` |
| `nmaxgr` | `nmaxgr` | `max(npw, nrxx)`（用于缓冲区分配） |
| `xprime` | `xprime` | Gamma-only 对称轴方向标志 |
| `gamma_only` | `gamma_only` | 是否仅 Γ 点计算 |
| `full_pw` | `full_pw` | 是否使用完整平面波模式 |

---

## 附录 C: 不变式与一致性条件汇总

| 编号 | 不变式 | 来源 |
|------|--------|------|
| INV-1 | $\forall$ ig, `ig2isz[ig]` 解码所得 `is` 满足 `is2fftixy[is]` 唯一对应一个 $(i_x, i_y)$ | ig2isz 定义 |
| INV-2 | `is2fftixy[is]` 在 $i_s$ 上严格单调递增 | 分布算法 |
| INV-3 | $\sum_p \text{numz}[p] = N_z$，区间无重叠无间隙 | distributer |
| INV-4 | $\sum_p \text{numg}[p] = \sum_p \text{numr}[p] = n_{\text{st,tot}} \cdot N_z$ | Alltoallv 一致性 |
| INV-5 | `real2recip` 与 `recip2real` 的步骤严格对称 | 逆变换正确性 |
| INV-6 | Dense grid 的 `istot2ixy` 继承 smooth grid 的 stick 顺序 | PW_Basis_Sup |
| INV-7 | Gamma-only 时 `fftnx`（xprime=true）或 `fftny`（xprime=false）为偶数 | FFT 维度约束 |
| INV-8 | $$|\mathbf{g}+\mathbf{k}|^2 < G_{\text{cut}}$$ 是 $$|\mathbf{g}|^2 < G_{\text{cut}}$$ 的子集 | k 点截断 |
| INV-9 | `npwk[ik] ≤ npw` 对所有 k 点成立 | k 点截断 |
| INV-10 | 每个 $(i_x, i_y)$ 至多对应一个 `istot2ixy` 条目 | 分布唯一性 |
# Workflow C: 底层数据吞吐优化与缓存复用分析

当跨节点并行的效率在通信优化方面提升后，底层的单核运算效率、向量化执行吞吐率和寄存器换页频次就会成为新的核心瓶颈。Workflow C 的主要方向是在不改变大逻辑和外部接口的前提下，对最底层的算力进行优化。

## 0. Workflow 涉及的文件与模块范围

本 workflow 主要覆盖两类低层热点：一类是平面波数据的 gather/scatter 拷贝重排，一类是平面波基底与 $G+K$ 相关的只读/不变数据构建与复用。

- `source/source_basis/module_pw/pw_gatherscatter.h`
  - `gatherp_scatters`：进程内或进程间的平面波分块重排，内层包含连续拷贝循环。
  - `gathers_scatterp`：与上面互为逆操作的拷贝重排路径，同样包含大量标量拷贝循环。
- `source/source_basis/module_pw/pw_basis.cpp`
  - `PW_Basis::collect_local_pw()`：构建当前进程的局域平面波集合，生成 `gg/gdirect/gcar` 等数组。
  - `PW_Basis::collect_uniqgg()`：对全体平面波做筛选/去重并计算派生量（如 $GGT \cdot f$）。
- `source/source_basis/module_pw/pw_basis_k.cpp`
  - `PW_Basis_K::setupIndGk()`：基于 $G+K$ 构建索引映射，统计 `npwk` 并生成 `igl2isz_k/igl2ig_k` 等结构。

以下调用链均可在 module_pw 的实现或测试/注释中直接对应到。

- **实空间/倒空间变换路径**（`pw_transform.cpp`）
  - `PW_Basis::real2recip()` -> `fftxyfor()` -> `gatherp_scatters()` -> `fftzfor()` -> 按 `ig2isz` 打包输出。
  - `PW_Basis::recip2real()` -> 按 `ig2isz` 解包 -> `fftzbac()` -> `gathers_scatterp()` -> `fftxybac()` -> 写回实空间。
- **平面波基底初始化路径**（`pw_basis_k.cpp` 与 `pw_init.cpp`）
  - `initgrids()` -> `initparameters()` -> `setuptransform()`
  - `setuptransform()` 内部顺序：`distribute_r()` -> `distribute_g()` -> `getstartgr()` -> `setupIndGk()` -> `fft_bundle.initfft()`。
- **平面波几何量构建路径**（`pw_basis.cpp` + `pw_basis_k.h` 使用示例）
  - `setuptransform()` 完成后，调用 `collect_local_pw()` 生成 `gg/gdirect/gcar`，再调用 `collect_uniqgg()` 构建 `gg_uniq/ig2igg`。

以上模块互相独立但共享同一类性能瓶颈：对连续内存的标量拷贝缺乏显式向量化提示，以及对不变数据的重复遍历与重建。

## 1. 痛点：缺乏 SIMD 优化的散列拷贝

### 1.1 模块实现逻辑

`gatherp_scatters` 与 `gathers_scatterp` 的核心逻辑是把一维连续的平面波数据块按进程/平面/索引重排。
典型实现是双层或多层循环，外层定位分块边界，内层按 `iz`/`izip` 逐元素拷贝。
分布在 `pw_gatherscatter.h` 中的拷贝重排操作大多是标量循环，典型模式如下：

```cpp
// 旧的逐个元素赋值循环
for (int iz = 0; iz < nplane; ++iz)
{
    outp[iz] = inp[iz];
}
```

这类循环在 `gatherp_scatters` 与 `gathers_scatterp` 中多处出现，贯穿单进程路径与 MPI 路径。其性能主要受制于内层连续拷贝的吞吐率，以及编译器对别名与对齐的保守判断。

从算法流程上看，MPI 路径包含三段明确的重排：

- 对于 gatherp_scatters（从 Planes 转换为 Sticks），重排包括：
  - 本地重排 1（打包提取）：提取有效网格点，把稀疏的平面数据 (nplane, fftnxy) 重排为紧凑的全局有效切片 (nplane, nstot)，便于一次性 MPI_Alltoallv。
  - 进程间交换（矩阵转置）：MPI_Alltoallv 完成不同进程之间的数据切片交换，将 (nplane, nstot) 交换为按来源进程分块的片段集合 (numz[ip], ns, poolnproc)。
  - 本地重排 2（解包拼接）：将按进程分块的碎片片段 (numz[ip], ns, poolnproc)，沿 Z 轴拼接重排为当前进程负责的完整柱状数据 (nz, ns)。

- 对于 gathers_scatterp（从 Sticks 还原为 Planes），重排包括：
  - 本地重排 1（切断打包）：将当前进程负责的完整柱状数据 (nz, ns) 按目标进程所需的厚度切断，打包重排为按目标进程分块的集合 (numz[ip], ns, poolnproc)，便于一次性 MPI_Alltoallv。
  - 进程间交换（矩阵转置）：MPI_Alltoallv 完成不同进程之间的数据片段交换，将发件缓冲区的 (numz[ip], ns, poolnproc) 交换为当前进程所辖楼层接收到的紧凑有效薄片 (nplane, nstot)。
  - 本地重排 2（补零散布）：在对输出网格进行全局清零后，把紧凑的有效薄片 (nplane, nstot) 重新散布并映射回包含无效真空区域的稀疏平面网格 (nplane, fftnxy)。

当 `poolnproc == 1` 时，代码不走 MPI 路径，而是直接按 `istot2ixy` 在本地做连续拷贝。此时 `gatherp_scatters` 与 `gathers_scatterp` 的本质都是“按 stick 索引重排 + z 连续拷贝”，避免了额外的通信打包。

此外在这段代码里，使用一维数组来存储二维的数据，保证了内存的连续性以及兼容MPI和OpenMP。首先第一步，对于一个xy平面，用`ixy`存储这个点在平面上的索引，使用公式 $ixy = y\times nx+x$ 。第二步，由于每个stick的内存地址是连续的，因此为了寻找空间坐标为`ixy`和`iz`的点，它的索引就是 $ixy\times nz+iz$ 。

### 1.2 具体算法实施细节

以下细节对应 `pw_gatherscatter.h` 的实际实现，便于与代码逐行对照。

- **单进程路径**（`poolnproc == 1`）
    - `gatherp_scatters`：对每个 stick `is`，通过 `ixy = istot2ixy[is]` 找到平面索引，随后执行连续拷贝 `out[is*nz + iz] = in[ixy*nz + iz]`。
    - `gathers_scatterp`：先把 `out[0..nrxx)` 清零，再按 `ixy = istot2ixy[is]` 做连续拷贝 `out[ixy*nz + iz] = in[is*nz + iz]`。
- **多进程路径的三段式打包与交换**
    - **阶段 A：本地打包/切片**
        - `gatherp_scatters`：把稀疏平面 `(nplane, fftnxy)` 提取成紧凑 `(nplane, nstot)`，拷贝位置为 `out[istot*nplane + iz] = in[ixy*nplane + iz]`。
        - `gathers_scatterp`：把 `(nz, ns)` 切成按目标进程分块的 `(numz[ip], ns, poolnproc)`，拷贝位置为 `out[startg[ip] + is*nzip + izip] = in[startz[ip] + is*nz + izip]`。
    - **阶段 B：进程间交换**
        - 使用 `MPI_Alltoallv`，发送缓冲区为 `out`，接收缓冲区为 `in`。其中 `numg/startg` 表示发送端分块，`numr/startr` 表示接收端分块。
    - **阶段 C：本地解包/拼接**
        - `gatherp_scatters`：把接收得到的 `(numz[ip], ns, poolnproc)` 片段拼接成 `(nz, ns)`，拷贝位置为 `out[startz[ip] + is*nz + izip] = in[startg[ip] + is*nzip + izip]`。
        - `gathers_scatterp`：先清零 `out[0..nrxx)`，再把 `(nplane, nstot)` 解包回稀疏平面 `(nplane, fftnxy)`，拷贝位置为 `out[ixy*nplane + iz] = in[istot*nplane + iz]`。

这些实现细节决定了此路径的开销结构：同一数据至少经历两次本地重排和一次全量通信，因此 SIMD 和缓存友好性非常关键。

### 1.3 逻辑局限性
- **自动向量化不稳定**：`outp` 与 `inp` 可能别名，编译器不敢做 aggressive vectorize。
- **连续访问未显式标注**：内层循环逻辑上连续，但缺少 `simd` 或 `restrict` 语义提示。
- **拷贝粒度过小**：单元素赋值使得 cache line 无法充分填充，拷贝带宽难以饱和。

### 1.4 初步解决方向
- 在 `iz/izip` 内层循环上显式添加 `#pragma omp simd`，减少编译器不确定性。
- 若调用约定允许，增加 `__restrict__` 或等价约束来声明不重叠。
- 对连续区域尝试 `memcpy` 或按 cache line 的块拷贝模式，以提高带宽利用率。


## 2. 痛点：循环内只读/不变数据的冗余重建

### 2.1 模块实现逻辑

`PW_Basis` 与 `PW_Basis_K` 负责构建平面波基底相关的几何量、筛选量以及 $G+K$ 的索引映射。典型流程如下：

- `PW_Basis::collect_local_pw()`
  - 读取晶胞与截断参数，将可用平面波映射为 `gg/gdirect/gcar` 等数组。
  - 每次调用都会重新分配并计算这些数组。
- `PW_Basis::collect_uniqgg()`
  - 再次遍历全部平面波，构建去重后的 `gg` 与派生量（如 $GGT \cdot f$ ）。
- `PW_Basis_K::setupIndGk()`
  - 对每个 `(ik, ig)` 计算 $|G+K|^2$ 并统计 `npwk`，然后再次遍历生成 `igl2isz_k/igl2ig_k`。

其中 `collect_uniqgg()` 的现有算法要点是：

- 对每个 `ig` 计算 $G^2 = f \cdot (GGT \cdot f)$ 并存入 `tmpgg`。
- 用 `heapsort` 对 `tmpgg` 排序并记录 `sortindex`。
- 线性扫描去重：对相近的 $G^2$ （阈值 `1.0e-8`）求平均，构建 `gg_uniq`，同时生成 `ig2igg` 映射。

`setupIndGk()` 的现有算法要点是：

- 第一次遍历 `(ik, ig)` 计算 $|G+K|^2$ 并计数得到 `npwk[ik]`，同时用 `MPI_Allreduce` 检查全局是否有平面波。
- 第二次遍历填充 `igl2isz_k/igl2ig_k` 索引映射。
- 末尾调用 `get_ig2ixyz_k()` 完成后续索引派生。

### 2.2 具体算法实施细节

#### 2.2.1 `collect_local_pw()` 的逐步计算
- **索引解码**：
    - `isz = ig2isz[ig]`，`iz = isz % nz`，`is = isz / nz`，`ixy = is2fftixy[is]`。
    - `ix = ixy / fftny`，`iy = ixy % fftny`。
- **坐标折返**：
    - 若 `ix >= nx/2 + 1` 则 `ix -= nx`，`iy` 与 `iz` 同理，确保坐标落入对称范围。
- **几何量计算**：
    - `f = (ix, iy, iz)`，`gg[ig] = f · (GGT · f)`。
    - `gdirect[ig] = f`，`gcar[ig] = f · G`。
    - 若 `gg[ig] < 1e-8`，记录 `ig_gge0` 并检查重复 Gamma 点。

#### 2.2.2 `collect_uniqgg()` 的排序去重
- **逐点重算 `tmpgg`**：对每个 `ig` 重新计算 $G^2$ 并写入 `tmpgg`。
- **排序与索引重建**：调用 `heapsort(npw, tmpgg, sortindex)`，保证 `sortindex` 给出从小到大的能量序。
- **阈值合并**：扫描 `tmpgg`，当 $|tmpgg[ig] - tmpgg2[igg]| > 1e-8$ 时新开一个 bin，否则累加并求平均。
- **映射生成**：按 `sortindex` 把每个 `ig` 归属到 `ig2igg`，并最终写出 `gg_uniq`。

#### 2.2.3 `setupIndGk()` 的双遍历构建
- **遍历 1：统计 `npwk`**
    - 对每个 `ik`，遍历所有 `ig`，计算 `gk2 = cal_GplusK_cartesian(ik, ig).norm2()`。
    - 若 `gk2 <= gk_ecut`，计数 `ng` 并更新 `npwk_max`。
    - `MPI_Allreduce` 检查全局是否有平面波，若 `ng == 0` 则发出警告。
- **遍历 2：填充映射**
    - 对每个 `ik` 再次遍历 `ig`，将满足阈值的 `ig` 写入 `igl2isz_k` 与 `igl2ig_k`。
    - 末尾调用 `get_ig2ixyz_k()` 补充派生索引。

### 2.3 逻辑局限性
- **生命周期与不变性未绑定**：`gg/gdirect/gcar` 与 `gk2` 等应与 `latvec/ggecut/kvec` 绑定，但当前每次调用均重建。
- **双重遍历与重复计算**：`setupIndGk()` 对同一 `(ik, ig)` 重复计算 $|G+K|^2$ 。
- **频繁分配导致抖动**：重复 `new/resize` 使得缓存失效，降低局域性。

### 2.4 初步解决方向
- **缓存复用**：将 `gg/gdirect/gcar` 与 `gk2` 的构建与生命周期绑定到 `ggecut` 或晶胞变化上，非必要不重建。
- **单次遍历复用结果**：在 `setupIndGk()` 中一次计算 `gk2` 后复用统计与索引构建，避免双重遍历。
- **失效规则明确化**：仅当 `latvec`、`ggecut` 或 `kvec` 变化时标记失效，其他路径直接复用。
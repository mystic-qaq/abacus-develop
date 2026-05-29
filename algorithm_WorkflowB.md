# WorkflowB：平面波 FFT 通信与计算重叠算法文档

## 1. WorkflowB 相关的原仓库模块

这条工作流中我们主要关注的是平面波基组中三维 FFT 的数据重排与 MPI 通信。

原仓库已经把三维 FFT 拆成“本地二维 FFT + 跨进程重排 + 本地一维 FFT”的结构。WorkflowB 的改造点集中在跨进程重排阶段，但它同时也依赖前后的分布映射和 FFT 调用链。

| 模块 | 相关文件 | 与 WorkflowB 相关之处 |
| --- | --- | --- |
| 平面波基类与通信映射成员 | `source/source_basis/module_pw/pw_basis.h`、`pw_basis.cpp` | 定义 `PW_Basis` 中的 FFT 网格、stick 分布、z-plane 分布、All-to-All 计数与位移数组。 |
| 实空间网格分布 | `source/source_basis/module_pw/pw_distributer.cpp` | 按 z 方向把实空间 FFT 网格分到各进程，生成 `numz`、`startz`、`nplane`、`nrxx`。 |
| 倒空间 stick 分布 | `source/source_basis/module_pw/pw_distributeg.cpp` | 把倒空间 plane-wave stick 分到各进程，生成 `nst_per`、`nst`、`nstot`、`istot2ixy`、`fftixy2ip`、`ig2isz` 等映射。 |
| 多 k 点平面波基类 | `source/source_basis/module_pw/pw_basis_k.cpp` | `PW_Basis_K` 复用 `PW_Basis` 的分布和通信映射，并额外建立 k 点相关的 `igl2isz_k` 映射。 |
| 正反 FFT 调用链 | `source/source_basis/module_pw/pw_transform.cpp`、`pw_transform_k.cpp` | 在 `real2recip` / `recip2real` 中调用 gather/scatter 通信内核，是 WorkflowB 影响上层数值路径的位置。 |
| gather/scatter 通信内核 | `source/source_basis/module_pw/pw_gatherscatter.h` | 当前 WorkflowB 最直接的改造对象，包含 `gatherp_scatters` 和 `gathers_scatterp` 两个模板函数。 |
| FFT 后端封装 | `source/source_base/module_fft/fft_bundle.*`、`fft_cpu.*` 等 | 提供 `fftxyfor`、`fftzfor`、`fftzbac`、`fftxybac`、`fftxyr2c`、`fftxyc2r` 等接口，决定通信前后的计算粒度。 |

可见，WorkflowB 并不是一个独立计算流程，而是嵌入现有 `PW_Basis` / `PW_Basis_K` FFT 变换路径中的通信优化工作：我们希望保持外部 FFT 接口和数值语义不变，优化 `pw_gatherscatter.h` 中的通信等待与局部重排顺序。

## 2. 与 WorkflowB 相关模块的实现逻辑

### 2.1 `PW_Basis` 初始化与通信映射

`PW_Basis::setuptransform()` 是 WorkflowB 所需映射的入口，当前顺序为：

```text
distribute_r()
distribute_g()
getstartgr()
fft_bundle.clear()
fft_bundle.initfft(...)
fft_bundle.setupFFT()
```

其中 `distribute_r()` 负责实空间 z-plane 分布：

- `numz[ip]`：第 `ip` 个进程持有的 z-plane 数；
- `startz[ip]`：第 `ip` 个进程持有 z-plane 的全局起点；
- `nplane = numz[poolrank]`：当前进程本地 z-plane 数；
- `nrxx = numz[poolrank] * nxy`：当前进程本地实空间网格大小。

`distribute_g()` 负责倒空间 stick 分布：

- `nst_per[ip]`：第 `ip` 个进程持有的 stick 数；
- `nst = nst_per[poolrank]`：当前进程持有的 stick 数；
- `nstot`：所有进程的 stick 总数；
- `istot2ixy[istot]`：全局 stick 到 FFT xy 网格位置的映射；
- `fftixy2ip[ixy]`：某个 xy stick 属于哪个进程；
- `ig2isz[ig]`：当前进程本地平面波系数到 `(stick, z)` 线性位置的映射。

`getstartgr()` 在这两类分布映射之上生成 MPI All-to-All 所需数组：

```text
numg[ip] = nst_per[poolrank] * numz[ip]
numr[ip] = nst_per[ip]       * numz[poolrank]

startg[ip] = prefix_sum(numg)
startr[ip] = prefix_sum(numr)
```

这四个数组是通信改造边界条件。`numr/startr` 描述从当前 z-plane 拥有者发往各 stick 拥有者的数据布局；`numg/startg` 描述当前 stick 拥有者从各 z-plane 拥有者接收的数据布局。

### 2.2 `PW_Basis_K` 的多 k 点扩展逻辑

`PW_Basis_K::setuptransform()` 与 `PW_Basis::setuptransform()` 的基本分布流程一致，也会调用：

```text
distribute_r()
distribute_g()
getstartgr()
```

区别是 `PW_Basis_K` 额外调用 `setupIndGk()`，为每个 k 点建立 `igl2isz_k` 等索引映射。通信本身仍复用 `gatherp_scatters` 和 `gathers_scatterp`，所以我们的基础通信改造不需要为单 k 点和多 k 点分别开发算法，差异主要在通信完成后的系数提取或回填阶段：`PW_Basis` 使用 `ig2isz`，`PW_Basis_K` 使用 `igl2isz_k`。

### 2.3 正向变换 `real2recip` 的路径

`PW_Basis::real2recip` 和 `PW_Basis_K::real2recip` 的核心路径可以概括为：

```text
实空间输入
  -> 拷贝到 auxr 或 rspace
  -> xy 方向 FFT
  -> gatherp_scatters(auxr, auxg)
  -> z 方向 FFT
  -> 按 ig2isz 或 igl2isz_k 提取平面波系数
```

其中 `gatherp_scatters` 的职责是把“按 z-plane 分布的 xy-FFT 数据”转换成“按 stick 分布的 z-FFT 数据”。经过该函数后，当前进程持有的 `auxg` 应包含本进程负责的 `nst` 条 stick，每条 stick 都具有完整的 `nz` 方向数据，随后才能调用 `fftzfor`。

在该方向上，我们可以尝试隐藏的是 gather 通信完成前后的局部 pack/unpack 开销；但在不改 FFT 接口的前提下，`fftzfor` 仍需要等待本进程全部 stick 的完整 z 数据就绪。

### 2.4 反向变换 `recip2real` 的 WorkflowB 路径

`PW_Basis::recip2real` 和 `PW_Basis_K::recip2real` 的核心路径可以概括为：

```text
倒空间输入
  -> 按 ig2isz 或 igl2isz_k 写入 auxg
  -> z 方向反 FFT
  -> gathers_scatterp(auxg, auxr)
  -> xy 方向反 FFT 或 c2r
  -> 写回实空间输出
```

其中 `gathers_scatterp` 的职责与 `gatherp_scatters` 相反：把“按 stick 分布的 z-FFT 数据”转换回“按 z-plane 分布的 xy-FFT 数据”。经过该函数后，当前进程持有的 `auxr` 应恢复为本进程负责的实空间 z-plane 布局，随后才能执行 `fftxybac` 或 `fftxyc2r`。

在该方向上，我们的一个可利用点是最终输出 `auxr` 需要先清零，再把收到的 stick 数据 unpack 到对应 xy 位置。如果通信缓冲区与最终输出缓冲区解耦，清零和部分 unpack 可以与剩余通信重叠。

### 2.5 `gatherp_scatters` 的当前实现逻辑

`gatherp_scatters(in, out)` 的输入输出语义为：

- `in`：当前进程持有的 z-plane 上的 xy-FFT 数据，逻辑形状可理解为 `(nplane, fftny, fftnx)`；
- `out`：当前进程最终应持有的 stick-major 数据，逻辑形状可理解为 `(nst, nz)`；
- 函数注释中明确 `in[] will be changed`，调用方不能假设 `in` 在函数返回后保持原值。

多进程路径当前分三步。

第一步，把 `in` 中本地 z-plane 数据按照全局 stick 顺序 pack 到 `out`：

```text
out[istot * nplane + iz] = in[ixy * nplane + iz]
```

此时 `out` 临时充当发送缓冲区，逻辑布局是 `(nstot, nplane)`。

第二步，调用阻塞式 `MPI_Alltoallv`：

```text
MPI_Alltoallv(
    out, numr, startr, dtype,
    in,  numg, startg, dtype,
    pool_world
)
```

发送方向使用 `numr/startr`，表示当前 z-plane 拥有者发给各 stick 拥有者的数据；接收方向使用 `numg/startg`，表示当前 stick 拥有者从各 z-plane 拥有者收取的数据。

第三步，把通信后写入 `in` 的接收数据 unpack 到最终 `out`：

```text
out[is * nz + startz[ip] + izip]
    = in[startg[ip] + is * numz[ip] + izip]
```

因此，当前实现里 `out` 先是发送缓冲区，后是最终输出缓冲区；`in` 先是输入数据，后是接收缓冲区。

### 2.6 `gathers_scatterp` 的当前实现逻辑

`gathers_scatterp(in, out)` 的输入输出语义为：

- `in`：当前进程持有的完整 stick 数据，逻辑形状可理解为 `(nst, nz)`；
- `out`：当前进程最终应持有的 z-plane 上 xy-FFT 数据，逻辑形状可理解为 `(nplane, fftny, fftnx)`；
- 函数注释同样允许 `in` 在函数内被通信接收数据覆盖。

多进程路径当前也分三步。

第一步，把 `in` 中每条本地 stick 在各目标进程 z-plane 上的片段 pack 到 `out`：

```text
out[startg[ip] + is * numz[ip] + izip]
    = in[is * nz + startz[ip] + izip]
```

此时 `out` 临时充当发送缓冲区，逻辑布局是按目标进程聚合的 `(numz[ip], nst)` 片段。

第二步，调用阻塞式 `MPI_Alltoallv`：

```text
MPI_Alltoallv(
    out, numg, startg, dtype,
    in,  numr, startr, dtype,
    pool_world
)
```

发送方向使用 `numg/startg`，表示当前 stick 拥有者把各 z 段发给对应 z-plane 拥有者；接收方向使用 `numr/startr`，表示当前 z-plane 拥有者从各 stick 拥有者收取 xy 位置数据。

第三步，先清零最终 `out`，再把通信后写入 `in` 的数据 unpack 回 xy 网格：

```text
out[:] = 0
out[ixy * nplane + iz] = in[istot * nplane + iz]
```

因此，当前实现里 `out` 先是发送缓冲区，后又被清零并作为最终输出缓冲区。这一点是反向路径引入非阻塞通信时最需要处理的生命周期问题。

### 2.7 FFT 后端对重叠粒度的约束

`FFT_Bundle` 负责把 `PW_Basis` 的 FFT 请求转发到具体后端。CPU 路径中，z 方向 FFT 使用 `plan_many` 形式批量处理 `nst` 条长度为 `nz` 的序列。

对我们来说，这带来两个约束：
1. `gatherp_scatters` 返回前，`auxg` 必须已经是完整 stick-major 布局，否则 `fftzfor` 不能开始；
2. `gathers_scatterp` 返回前，`auxr` 必须已经是完整 plane-major 布局，否则后续 xy 方向反 FFT 或 c2r 不能开始。

因此，第一阶段的重叠空间主要在通信内核内部：让已到达 peer 的数据尽早 unpack，或让输出清零与通信并行。若要进一步把 z-FFT 本身也与通信流水化，还需要额外改造 FFT 接口或引入 stick-block 粒度。

## 3. 当前实现的局限性与初步解决方向

| 当前局限性 | 影响 | 初步解决方向 |
| --- | --- | --- |
| `gatherp_scatters` 和 `gathers_scatterp` 使用阻塞式 `MPI_Alltoallv` | pack、通信、unpack 严格串行，通信期间没有可见的计算/重排 overlap | 改为显式 `MPI_Irecv` / `MPI_Isend`，并用 `MPI_Waitsome` 在 peer 数据到达后立即执行对应 unpack。 |
| 当前缓冲区生命周期复用较重 | `out` 同时承担发送缓冲区和最终输出缓冲区，`in` 同时承担输入和接收缓冲区，非阻塞发送未完成前不能安全改写 `out` | 引入独立 `sendbuf`，必要时保留独立或可复用的 `recvbuf`，把“发送数据生命周期”和“最终输出生命周期”分开。 |
| 直接把 `MPI_Alltoallv` 替换为 `MPI_Ialltoallv` 收益有限 | `MPI_Ialltoallv` 只有一个整体 request，若马上 `MPI_Wait` 基本不产生 overlap，也难以及时知道哪个 peer 已经到达 | 第一阶段优先使用点对点非阻塞通信；若使用 `MPI_Ialltoallv`，也只作为接口层改造而非主要性能方案。 |
| `gathers_scatterp` 需要清零最终输出 | 当前 `out` 在通信前已经作为发送缓冲区使用，不能在非阻塞发送未完成时直接清零 | 先 pack 到独立 `sendbuf`，发起非阻塞通信后即可清零最终 `out`，再随接收完成逐步 unpack。 |
| z 方向 FFT 需要完整 stick 数据 | peer 级接收完成只能提前 unpack，不能直接启动完整 `fftzfor` / `fftzbac` | 基础版本先实现通信与 unpack overlap；第二阶段再评估 stick-block 双缓冲，使一个 block 通信时另一个 block 做 z-FFT 或系数提取。 |
| 当前消息按 peer 聚合，不按 stick-block 聚合 | peer 到达不等于某个 stick-block 已完整到达，限制了更细粒度计算流水线 | 若进入第二阶段，需要为 stick-block 重新计算局部 counts/displacements 或封装 block offset。 |
| 临时缓冲区可能带来额外内存和分配开销 | 小规模算例中可能抵消非阻塞收益 | 先用局部临时数组保证正确性；性能稳定后再考虑把 workspace 缓存在 `PW_Basis` 生命周期内。 |
| MPI 异步进展依赖具体实现 | 如果 MPI 库缺少后台进展，单纯发起非阻塞通信不一定产生理想 overlap | `Waitsome` 循环持续进入 MPI，并把每次完成后的 unpack 控制在适中粒度，避免长时间脱离 MPI 进展。 |
| OpenMP 与 MPI 线程级别需保持一致 | 在 OpenMP 并行区内直接调用 MPI 可能引入线程安全问题 | 保持 MPI 调用在 OpenMP 并行区外；pack/unpack 仍可保留 OpenMP 并行循环。 |
| CPU/MPI 路径与 GPU/DSP 路径不完全一致 | 部分加速器路径使用不同 FFT 入口或要求 `poolnproc == 1`，不能直接套用同一通信改造 | WorkflowB 第一阶段限定在 CPU/MPI 的 `PW_Basis` 与 `PW_Basis_K` 路径；加速器路径作为后续兼容性评估。 |

综上，当前工作流的当前安全边界是：不改变上层 FFT 变换接口，不改变 `ig2isz` / `igl2isz_k` 所定义的数值布局，先把 `pw_gatherscatter.h` 中的阻塞通信改造成可逐步 unpack 的非阻塞通信，再根据性能数据决定是否继续深入到 FFT 粒度的流水线改造。

可按以下顺序推进：

1. 保持 `gatherp_scatters` / `gathers_scatterp` 的函数签名和返回语义不变，先补充计时点，建立阻塞版本基线。
2. 抽出 pack/unpack 的局部 helper 或 lambda，确保重构前后数值结果一致。
3. 引入独立 `sendbuf`，先继续使用阻塞 `MPI_Alltoallv`，验证缓冲区拆分本身不改变结果。
4. 将通信替换为 `MPI_Irecv` / `MPI_Isend` + `MPI_Waitsome`，对已完成 peer 立即 unpack，最后 `MPI_Waitall` 确认发送完成。
5. 对 `poolnproc == 1`、`float/double`、`gamma_only`、普通 complex FFT、`PW_Basis`、`PW_Basis_K` 分别做正确性回归。
6. 在基础非阻塞版本稳定后，再评估 stick-block 双缓冲与 z-FFT 子区间接口，作为进一步的通信与计算重叠方案。

## 4. 已实现的第一阶段版本（2026-05-27）

当前 `WorkflowB` 分支已经把 CPU/MPI 路径中的第一阶段改造落到 `source/source_basis/module_pw/pw_gatherscatter.h`，实现边界与计划保持一致：

- 不改 `PW_Basis` / `PW_Basis_K` / `PW_Basis_Sup` 的公开 FFT 接口；
- 不改 `ig2isz` / `igl2isz_k` 数值布局；
- 不引入运行时开关；
- `poolnproc == 1` 快路径保持原样；
- GPU / DSP 路径不在本阶段范围内。

### 4.1 实现入口与 workspace

`PW_Basis` 现在新增了仅供内部实现使用的可复用发送暂存区：

```cpp
mutable std::vector<std::complex<float>> comm_sendbuf_float_;
mutable std::vector<std::complex<double>> comm_sendbuf_double_;
```

并通过 `acquire_comm_sendbuf<T>(size)` 惰性扩容。这样做的目的有两个：

1. 把“发送数据生命周期”从最终输出缓冲区中剥离出来，允许通信未结束时继续写最终输出；
2. 避免每次 gather/scatter 都临时 `new[]` / `delete[]`。

目前没有单独的持久 `recvbuf`。第一阶段仍复用原有 `in` 作为接收缓冲区，这与原阻塞实现的内存语义一致。

### 4.2 `gatherp_scatters` 的 request 生命周期

当前实现顺序如下：

1. 先把本地 plane-major 数据 pack 到独立 `sendbuf`，布局仍是 `(nstot, nplane)`。
2. 对所有 `numg[ip] > 0` 且 `ip != poolrank` 的 peer 先发 `MPI_Irecv(&in[startg[ip]], ...)`。
3. 再对所有 `numr[ip] > 0` 且 `ip != poolrank` 的 peer 发 `MPI_Isend(&sendbuf[startr[ip]], ...)`。
4. self peer 不走 MPI，请求保持 `MPI_REQUEST_NULL`。
5. 进入 `MPI_Waitsome` 循环：哪个 peer 的接收先完成，就立刻把 `in[startg[ip] ...]` unpack 到最终 `out[is * nz + startz[ip] ...]`。
6. 全部接收完成后，再用一次 `MPI_Waitall` 等待剩余 send request 结束。

这里的关键点是：接收和 unpack 已经按 peer 粒度流水化，但 `fftzfor` 仍然要等函数整体返回后才能开始，因此第一阶段重叠只发生在通信内核内部。

### 4.3 `gathers_scatterp` 的 request 生命周期

反向路径采用同样的模式：

1. 先把 stick-major 输入 pack 到独立 `sendbuf`，布局仍是按 peer 聚合的 `(numz[ip], nst)`。
2. 对所有 `numr[ip] > 0` 且 `ip != poolrank` 的 peer 发 `MPI_Irecv(&in[startr[ip]], ...)`。
3. 对所有 `numg[ip] > 0` 且 `ip != poolrank` 的 peer 发 `MPI_Isend(&sendbuf[startg[ip]], ...)`。
4. 因为发送已经完全脱离最终输出 `out`，此时可以立即执行 `gathers_clear`，把 plane-major 输出区清零。
5. self peer 直接把 `sendbuf[startg[self] ...]` 拷回 `in[startr[self] ...]`，然后立即 unpack。
6. 后续 peer 在 `MPI_Waitsome` 中一旦完成，就按 `istot0 = startr[ip] / nplane` 计算该 peer 的全局 stick 区间，并直接 unpack 到最终 plane-major `out`。
7. 全部接收结束后，再 `MPI_Waitall` 等剩余发送完成。

因此，第一阶段的反向路径已经把“clear + 部分 unpack”与剩余 MPI 等待交织起来，但还没有进入更细的 stick-block/z-FFT 流水线。

### 4.4 self peer 与零长度 peer 的处理

当前版本显式区分三种 peer：

- `ip == poolrank`：不发 MPI request，只做本地内存拷贝，然后立即 unpack；
- `count == 0`：既不发 `Irecv` 也不发 `Isend`，对应 request 保持 `MPI_REQUEST_NULL`；
- 普通非零 peer：进入 `Irecv/Isend/Waitsome/Waitall` 生命周期。

这样做的直接收益是避免空消息、避免对 self peer 的不必要 MPI 调用，也让 `Waitsome` 的边界更稳定。

### 4.5 timer 语义

为了兼容现有 benchmark 解析脚本，timer 名称保持不变，但语义已经收紧为：

- `gatherp_pack` / `gathers_pack`：仅统计本地 pack；
- `gatherp_unpack` / `gathers_unpack`：仅统计本地 unpack；
- `gathers_clear`：仅统计最终 plane-major 输出清零；
- `gatherp_alltoallv` / `gathers_alltoallv`：只统计 MPI request post、`MPI_Waitsome` 和 `MPI_Waitall` 中的 MPI 等待时间，不再把 unpack/clear 混进去。

这意味着新的 `*_alltoallv` timer 仍可被旧脚本识别，但更接近“通信临界路径”本身。

### 4.6 CPU/MPI 适用边界

当前实现只对 `pw_transform.cpp` / `pw_transform_k.cpp` 的 CPU/MPI gather/scatter 内核生效：

- `PW_Basis` complex 路径和 gamma 路径都走同一套非阻塞通信内核；
- `PW_Basis_K` 通过既有调用链自动继承；
- `PW_Basis_Sup` 也继承同一通信实现，但本阶段的新增定向 roundtrip 单测只把 `PW_Basis` 作为“任意 plane-major 数据”的稳定不变量检查，`PW_Basis_Sup` 仍通过既有 `test_sup` 并行回归覆盖其分布与调用路径；
- GPU / DSP / 跨节点网络行为仍需单独评估，当前文档和基准结论仅对应单节点 CPU + OpenMPI。

# 工作流 B：平面波 FFT 通信与计算重叠算法文档

## 1. 流程合理性结论

按照 `计划书.md` 的工作流划分，工作流 B 对应题目 2 和题目 7：把 `pw_gatherscatter.h` 中 FFT 重排阶段的阻塞式 `MPI_Alltoallv` 改造为非阻塞通信，并进一步探索通信与计算重叠。因此，单独创建 `AlgorithmDocs/plane-wave-assignment` 分支、先写算法文档、再进入代码实现，是合理的。

需要补充的一点是：单纯把 `MPI_Alltoallv` 替换为 `MPI_Ialltoallv` 并立刻 `MPI_Wait`，只能降低接口层面的阻塞程度，通常不能带来有效 overlap。当前代码还复用了同一块数组作为发送缓冲区和最终输出缓冲区，若不先拆分缓冲区或调整流水线粒度，等待期间几乎没有可安全执行的计算。因此，本工作流的改造重点应是：

1. 保持现有 `gatherp_scatters` / `gathers_scatterp` 对外接口不变；
2. 将发送缓冲区、接收缓冲区和最终输出缓冲区的生命周期显式化；
3. 使用 `MPI_Irecv` / `MPI_Isend` 加 `MPI_Waitsome` 或 `MPI_Waitall`，把可提前完成的 unpack、清零、局部重排隐藏在剩余通信之后；
4. 在低风险版本通过验证后，再评估按 stick-block 的双缓冲流水线，以进一步隐藏 Z-FFT 或后续系数提取的开销。

## 2. 涉及文件与职责

本工作流主要覆盖以下文件：

| 文件 | 当前职责 | 工作流 B 关注点 |
| --- | --- | --- |
| `source/source_basis/module_pw/pw_gatherscatter.h` | 定义 `gatherp_scatters` 和 `gathers_scatterp` 两个模板函数 | 两处阻塞 `MPI_Alltoallv`、通信缓冲区复用、pack/unpack 重排 |
| `source/source_basis/module_pw/pw_transform.cpp` | `PW_Basis` 的实空间/倒空间 FFT 变换 | `real2recip` 和 `recip2real` 调用 gather/scatter 的上下游依赖 |
| `source/source_basis/module_pw/pw_transform_k.cpp` | `PW_Basis_K` 的多 k 点 FFT 变换 | 与单 k 点共享 gather/scatter 通信路径 |
| `source/source_basis/module_pw/pw_basis.cpp` | `setuptransform` 和 `getstartgr` | 初始化 All-to-All 计数与位移数组 |
| `source/source_basis/module_pw/pw_distributer.cpp` | 实空间 z-plane 划分 | 初始化 `startz`、`numz`、`nplane`、`nrxx` |
| `source/source_basis/module_pw/pw_basis.h` | 通信映射和 FFT 成员声明 | 明确 `numg`、`numr`、`startg`、`startr` 等数组含义 |

## 3. 当前 FFT 通信流程

### 3.1 初始化阶段

`PW_Basis::setuptransform` 的顺序是：

```text
distribute_r()
distribute_g()
getstartgr()
fft_bundle.initfft(...)
fft_bundle.setupFFT()
```

其中 `distribute_r()` 按 z 方向把实空间 FFT 网格分到各进程：

- `numz[ip]`：第 `ip` 个进程持有的 z-plane 数；
- `startz[ip]`：第 `ip` 个进程的起始 z-plane；
- `nplane = numz[poolrank]`：当前进程持有的 z-plane 数；
- `nrxx = numz[poolrank] * nxy`：当前进程实空间局部网格大小。

`distribute_g()` 负责把 reciprocal-space 的 stick 分到各进程，生成：

- `nst_per[ip]`：第 `ip` 个进程拥有的 stick 数；
- `nst = nst_per[poolrank]`：当前进程拥有的 stick 数；
- `istot2ixy[istot]`：全局第 `istot` 个 stick 对应的 x-y 网格位置；
- `fftixy2ip[ixy]`：某个 x-y stick 属于哪个进程；
- `ig2isz[ig]`：局部平面波系数映射到 stick 内 `(is, iz)` 的线性位置。

`getstartgr()` 进一步生成 All-to-All 所需计数和位移：

```text
numg[ip] = nst_per[poolrank] * numz[ip]
numr[ip] = nst_per[ip]       * numz[poolrank]

startg[ip] = prefix_sum(numg)
startr[ip] = prefix_sum(numr)
```

它们的含义是：

- `numr/startr` 描述“当前进程按本地 z-plane 向各 stick 拥有者发送多少数据”；
- `numg/startg` 描述“当前进程作为 stick 拥有者，从各 z-plane 拥有者接收多少数据”。

### 3.2 `real2recip` 正向变换

`PW_Basis::real2recip` 和 `PW_Basis_K::real2recip` 的核心路径为：

```text
实空间输入
  -> 拷贝到 auxr 或 rspace
  -> xy 方向 FFT
  -> gatherp_scatters(auxr, auxg)
  -> z 方向 FFT
  -> 按 ig2isz/igl2isz_k 提取平面波系数
```

此处 `gatherp_scatters` 的任务是：把“按 z-plane 分布的 xy-FFT 数据”重排并通信成“按 stick 分布的 z-FFT 数据”。

### 3.3 `recip2real` 反向变换

`PW_Basis::recip2real` 和 `PW_Basis_K::recip2real` 的核心路径为：

```text
倒空间输入
  -> 填入 auxg 的 stick-major 布局
  -> z 方向反 FFT
  -> gathers_scatterp(auxg, auxr)
  -> xy 方向反 FFT 或 c2r
  -> 写回实空间输出
```

此处 `gathers_scatterp` 的任务与 `gatherp_scatters` 相反：把“按 stick 分布的 z-FFT 数据”通信并重排回“按 z-plane 分布的 xy-FFT 数据”。

## 4. 当前 `MPI_Alltoallv` 与缓冲区布局

### 4.1 `gatherp_scatters(in, out)`

函数注释中的逻辑数据形状是：

- `in`：`(nplane, fftny, fftnx)`，即当前进程持有的 z-plane 上的 xy-FFT 数据；
- `out`：`(nst, nz)`，即当前进程拥有的 stick 上完整 z 方向数据。

多进程路径分三步。

第一步，pack 本地 z-plane 数据：

```text
for istot in [0, nstot):
    ixy = istot2ixy[istot]
    out[istot * nplane + iz] = in[ixy * nplane + iz]
```

此时 `out` 临时作为发送缓冲区，逻辑布局为：

```text
sendbuf_r2g[istot][local_z]
```

第二步，阻塞 All-to-All：

```text
MPI_Alltoallv(
    out, numr, startr, dtype,
    in,  numg, startg, dtype,
    pool_world
)
```

这里发送与接收计数分别是：

- send：`numr[ip] = nplane * nst_per[ip]`，发给第 `ip` 个 stick 拥有者；
- recv：`numg[ip] = nst * numz[ip]`，从第 `ip` 个 z-plane 拥有者接收当前进程所拥有 stick 的 z 段。

第三步，unpack 接收数据到最终 `out`：

```text
for ip in [0, poolnproc):
    for is in [0, nst):
        out[is * nz + startz[ip] + izip] =
            in[startg[ip] + is * numz[ip] + izip]
```

关键约束：

- 当前实现中 `out` 同时承担“发送缓冲区”和“最终输出缓冲区”；
- `in` 在通信后被接收数据覆盖，函数注释也明确 `in[] will be changed`；
- 因为 `MPI_Alltoallv` 是阻塞调用，第三步只有在所有接收完成后才能开始。

### 4.2 `gathers_scatterp(in, out)`

函数注释中的逻辑数据形状是：

- `in`：`(nst, nz)`，即当前进程拥有的完整 stick 数据；
- `out`：`(nplane, fftny, fftnx)`，即当前进程持有 z-plane 上的 xy-FFT 数据。

多进程路径同样分三步。

第一步，pack 当前 stick 在各目标进程 z-plane 上的片段：

```text
for ip in [0, poolnproc):
    for is in [0, nst):
        out[startg[ip] + is * numz[ip] + izip] =
            in[is * nz + startz[ip] + izip]
```

此时 `out` 临时作为发送缓冲区，逻辑布局为：

```text
sendbuf_g2r[ip][local_stick][z_on_ip]
```

第二步，阻塞 All-to-All：

```text
MPI_Alltoallv(
    out, numg, startg, dtype,
    in,  numr, startr, dtype,
    pool_world
)
```

这里发送与接收计数分别是：

- send：`numg[ip] = nst * numz[ip]`，把当前进程拥有的 stick 的一段 z 数据发给第 `ip` 个 z-plane 拥有者；
- recv：`numr[ip] = nplane * nst_per[ip]`，从第 `ip` 个 stick 拥有者接收当前 z-plane 上对应 x-y 位置的数据。

第三步，清零最终输出并 unpack：

```text
out[:] = 0

for istot in [0, nstot):
    ixy = istot2ixy[istot]
    out[ixy * nplane + iz] = in[istot * nplane + iz]
```

关键约束：

- 当前实现中 `out` 同时承担“发送缓冲区”和“最终输出缓冲区”；
- 如果直接换成非阻塞发送，在发送完成前不能清零或写 `out`；
- `in` 通信后被接收数据覆盖，符合当前函数注释。

## 5. 当前同步逻辑

生产路径中没有显式 `MPI_Barrier`。真正的同步来自以下几类：

1. `MPI_Alltoallv` 的阻塞完成语义：所有发送缓冲区可重用、所有接收缓冲区可读取之后才返回；
2. OpenMP `parallel for` 默认隐式 barrier：pack 循环结束后才进入 MPI 调用，unpack 循环结束后才返回上层 FFT；
3. 初始化阶段的 `MPI_Allreduce` / `MPI_Bcast`：用于构造全局计数、stick 映射和 FFT 网格信息，不在每次 FFT 的热路径内；
4. `FFT_Bundle::fftzfor` / `fftzbac` 的批量 FFT plan：当前 Z-FFT plan 一次处理 `ns = nst` 个长度为 `nz` 的序列，因此在不改 FFT 接口时，Z-FFT 必须等所有 stick 的完整 z 数据就绪。

这说明工作流 B 的第一阶段最好聚焦 gather/scatter 内部，把“通信完成后才做的局部 unpack/清零”尽量提前到各 peer 数据到达之后，而不是一开始就改动 FFT plan。

## 6. 非阻塞改造方案

### 6.1 阶段一：低风险非阻塞通信骨架

目标：在不改变外部接口和数值语义的前提下，引入非阻塞通信骨架。

建议新增内部工具逻辑：

```text
mpi_complex_type<T>()
post_all_recvs(recvbuf, recvcounts, rdispls, tag)
post_all_sends(sendbuf, sendcounts, sdispls, tag)
waitsome_recvs_and_unpack(...)
waitall_sends(...)
```

类型映射保持现有行为：

- `T = double` -> `MPI_DOUBLE_COMPLEX`
- `T = float` -> `MPI_COMPLEX`

边界处理：

- `poolnproc == 1` 保留现有快速路径；
- `count == 0` 的 peer 不投递消息，或投递前统一过滤；
- MPI 调用保持在 OpenMP 并行区外，符合 `MPI_THREAD_FUNNELED` 的使用习惯；
- 所有非阻塞发送完成前，发送缓冲区不能释放或修改；
- 接收分段 unpack 完成前，接收缓冲区对应片段不能复用。

### 6.2 阶段二：拆分发送缓冲区以释放最终输出

这是获得 overlap 的关键。

#### `gatherp_scatters`

当前 `out` 同时是发送缓冲区和最终 `auxg` 输出。改造后：

```text
sendbuf = 临时数组，大小 sum(numr) = nplane * nstot
recvbuf = 可复用 in，大小 sum(numg) = nst * nz
out     = 最终 auxg 输出
```

算法：

```text
1. pack in -> sendbuf
2. post Irecv(recvbuf + startg[ip], numg[ip]) for all ip
3. post Isend(sendbuf + startr[ip], numr[ip]) for all ip
4. while 仍有未完成接收:
       Waitsome(recv_requests)
       对每个已完成 peer ip:
           unpack recvbuf[startg[ip] ...] -> out[is * nz + startz[ip] ...]
5. Waitall(send_requests)
6. 返回；out 已经是完整 stick-major 布局
```

可重叠部分：

- peer `ip` 的接收一完成，就可以 unpack 该 peer 的 z 段；
- unpack `ip` 的内存重排可以与其他 peer 的网络传输重叠；
- 由于发送使用独立 `sendbuf`，unpack 可以安全写入 `out`，不会破坏尚未完成的发送。

#### `gathers_scatterp`

当前 `out` 同时是发送缓冲区和最终 `auxr` 输出。改造后：

```text
sendbuf = 临时数组，大小 sum(numg) = nst * nz
recvbuf = 可复用 in，大小 sum(numr) = nplane * nstot
out     = 最终 auxr 输出
```

算法：

```text
1. pack in -> sendbuf
2. post Irecv(recvbuf + startr[ip], numr[ip]) for all ip
3. post Isend(sendbuf + startg[ip], numg[ip]) for all ip
4. out[:] = 0
5. while 仍有未完成接收:
       Waitsome(recv_requests)
       对每个已完成 peer ip:
           unpack recvbuf[startr[ip] ...] -> out[ixy * nplane + iz]
6. Waitall(send_requests)
7. 返回；out 已经是完整 plane-major 布局
```

可重叠部分：

- `out[:] = 0` 可在非阻塞通信发出后执行；
- 任一 stick 拥有者的数据到达后，可立即 unpack 到对应 `ixy` 位置；
- 因为发送使用独立 `sendbuf`，清零和 unpack 不会破坏尚未完成的发送。

### 6.3 阶段三：按 peer 的 `Waitsome` unpack

`MPI_Ialltoallv` 只有一个总 request，无法暴露“某个 peer 的数据已经到达”。因此如果目标是把 unpack 和剩余通信重叠，推荐使用显式 `MPI_Irecv` / `MPI_Isend`。

建议结构：

```text
recv_reqs[]      // 只保存 count > 0 且非 self 的接收请求
recv_peers[]     // recv_reqs 下标到 peer rank 的映射
send_reqs[]      // 只保存 count > 0 且非 self 的发送请求

for ip:
    if ip == poolrank:
        local_copy_or_direct_unpack(ip)
    else:
        MPI_Irecv(...)

for ip:
    if ip != poolrank:
        MPI_Isend(...)

while active_recv > 0:
    MPI_Waitsome(...)
    unpack_completed_peers(...)

MPI_Waitall(send_reqs)
```

self 数据可以不经过 MPI，直接 copy 或直接走对应 peer 的 unpack 逻辑。这能减少一次本地回环消息，也能降低实现对 MPI self-send 行为的依赖。

注意：在 `gathers_scatterp` 中，self 数据若直接 unpack 到最终 `out`，必须发生在 `out[:] = 0` 之后；否则本地数据会被清零步骤覆盖。更稳妥的写法是先把 self 数据也放在临时接收布局里，等清零完成后统一调用同一套 peer unpack 逻辑。

### 6.4 阶段四：FFT 级双缓冲流水线

题目 7 要求更进一步的 FFT 通信与计算 overlap。当前限制是：

- `fftzfor` / `fftzbac` 使用 FFTW `plan_many` 一次处理 `ns = nst` 个 z 序列；
- 每个 stick 的 Z-FFT 都需要所有进程贡献完整 z 段；
- 现有 gather/scatter 消息按 peer 聚合，不能直接在第一个 peer 到达后启动某个 stick 的完整 Z-FFT。

因此，完整流水线需要额外改造，建议作为第二轮优化：

```text
stick-block 0 通信中     -> stick-block -1 做 Z-FFT / 系数提取
stick-block 1 通信中     -> stick-block 0  做 Z-FFT / 系数提取
...
```

前置条件：

1. 将 `nst` 个 stick 切成若干 block；
2. 为每个 block 生成局部 `sendcounts/recvcounts/displs` 或可复用的 block offset；
3. 增加或封装只处理 stick 子区间的 Z-FFT 接口；
4. 使用两个 block buffer 交替作为通信缓冲区和计算缓冲区；
5. 保证输出顺序仍与原 `ig2isz` / `igl2isz_k` 映射一致。

这个阶段的代码侵入性高于阶段二、三，建议等基础非阻塞版本通过正确性和性能测试后再实施。

## 7. 正确性约束

改造必须保持以下语义不变：

1. `gatherp_scatters` 返回时，`out[is * nz + iz]` 与原实现完全一致；
2. `gathers_scatterp` 返回时，`out[ixy * nplane + iz]` 与原实现完全一致，不存在旧值残留；
3. `in[] will be changed` 的现有约定可以保留，但不能额外破坏调用方仍需读取的数据；
4. `float` 和 `double` 两条模板路径都必须覆盖；
5. `gamma_only`、`xprime`、单 k 点 `PW_Basis`、多 k 点 `PW_Basis_K` 不应出现通信布局分叉，因为通信只依赖已经初始化好的 `fftnx/fftny/nst/nz` 等映射；
6. 非 MPI 或 `poolnproc == 1` 路径保持当前行为；
7. MPI 调用错误应至少在 debug 或开发阶段可诊断，避免静默返回错误结果。

## 8. 性能评估方案

建议增加以下计时分段，先用于本分支验证，最终是否保留可再决定：

| 计时项 | 含义 |
| --- | --- |
| `gather_pack` | `gatherp_scatters` 第一段 pack |
| `gather_post` | 投递 Irecv/Isend |
| `gather_wait_unpack` | Waitsome + peer unpack |
| `gather_wait_send` | 等待未完成发送 |
| `scatter_pack` | `gathers_scatterp` 第一段 pack |
| `scatter_zero` | 反向路径最终输出清零 |
| `scatter_wait_unpack` | Waitsome + peer unpack |
| `real2recip_total` | 端到端正向变换 |
| `recip2real_total` | 端到端反向变换 |

测试矩阵：

- 进程数：1、2、4、8；
- 线程数：1、2、4、8；
- 体系：优先使用计划书中推荐的 NaCl 大体系测试通信，使用 Al 或 Si 做快速正确性回归；
- 精度：`double` 必测，若仓库启用 single/mixing，则补测 `float`；
- 模式：普通 complex FFT 必测，Gamma Only 路径做回归检查。

判定标准：

1. 数值结果与阻塞版本一致，能量或数组误差满足项目要求；
2. `gather/scatter` 中 `MPI` 等待时间下降，或 `wait + unpack` 总时间小于原阻塞通信加 unpack 时间；
3. 端到端 `real2recip` / `recip2real` 时间不回退；
4. 小进程数、小体系允许收益不明显，但不应出现显著额外内存或调度开销。

## 9. 推荐实现顺序

1. 提交本文档，冻结工作流 B 的目标、边界和风险；
2. 给 `gatherp_scatters` / `gathers_scatterp` 添加更细的 timer，建立阻塞版本基线；
3. 抽出 pack/unpack 小工具或局部 lambda，先保证重构前后结果一致；
4. 引入独立 `sendbuf`，仍使用阻塞 `MPI_Alltoallv`，验证缓冲区拆分不改变结果；
5. 将阻塞 All-to-All 改为显式 `MPI_Irecv` / `MPI_Isend` + `MPI_Waitsome`，实现 peer 级 unpack overlap；
6. 跑正确性和性能测试，比较阻塞版本、拆缓冲版本、非阻塞版本；
7. 视收益决定是否进入 stick-block 双缓冲和 Z-FFT 子区间接口设计。

## 10. 主要风险与规避

| 风险 | 影响 | 规避方式 |
| --- | --- | --- |
| 仅替换为 `MPI_Ialltoallv` 但没有可重叠工作 | 性能收益很小 | 使用独立发送缓冲区和 peer 级 `Waitsome` unpack |
| 发送缓冲区过早释放或被写 | 随机错误或死锁 | `MPI_Waitall(send_reqs)` 前不修改、不释放 |
| `out` 仍被发送请求引用时被清零 | 反向路径结果错误 | 发送数据放入独立 `sendbuf` |
| MPI 实现缺少异步进展 | overlap 收益受限 | `Waitsome` 循环持续进入 MPI，避免长时间只做本地计算 |
| OpenMP 与 MPI 线程级别不匹配 | 线程安全问题 | MPI 调用只放在 OpenMP 并行区外 |
| 临时缓冲区频繁分配 | 小体系性能回退 | 后续可把 workspace 缓存在 `PW_Basis` 生命周期内，先以正确性为优先 |
| 分块 Z-FFT 改动过大 | 影响 FFT 接口和 GPU/DSP 后端 | 先完成 peer 级 overlap，再单独评估 stick-block 方案 |

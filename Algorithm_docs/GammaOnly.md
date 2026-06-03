# GammaOnly：平面波半谱 FFT 与紧凑存储算法文档

## 1. GammaOnly 相关的原仓库模块

这条工作流对应计划书中的工作流D，主要覆盖题目4、题目6和附加题：在 Gamma 点物理条件成立时，利用实空间函数的共轭对称性

```text
F(-G) = conj(F(G))
```

减少倒空间独立分量、FFT 网格和数据搬移开销，并进一步设计电荷密度、势函数、波函数的紧凑存储与 pack/unpack 接口。

原仓库中 `module_pw` 已经有一部分 GammaOnly 基础能力：`PW_Basis` 和 `PW_Basis_K` 都带有 `gamma_only` 标志，CPU FFT 后端也提供 r2c/c2r 的半谱路径。但从生产 PW 路径看，输入层目前会把 PW 的 `gamma_only` 重置为 0，`setup_pwrho` / `setup_pwwfc` 也显式以 `false` 初始化 PW 基组。因此，本工作流的重点不是从零发明 GammaOnly，而是把已有半谱机制梳理清楚，再补齐多 k 点、紧凑存储和生产路径接入。

| 模块 | 相关文件 | 与 GammaOnly 相关之处 |
| --- | --- | --- |
| 输入参数与全局开关 | `source/source_io/module_parameter/read_input_item_elec_stru.cpp`、`read_set_globalv.cpp` | 当前 `gamma_only` 用户参数主要面向 LCAO；PW 情况会被重置为 0，是生产路径启用 GammaOnly 的第一层限制。 |
| PW 生产初始化 | `source/source_pw/module_pwdft/setup_pwrho.cpp`、`setup_pwwfc.cpp` | 电荷密度基组 `PW_Basis` 和波函数基组 `PW_Basis_K` 当前都以 `false` 初始化 `gamma_only`。 |
| 平面波基类与半谱网格 | `source/source_basis/module_pw/pw_basis.h`、`pw_init.cpp`、`pw_basis.cpp` | `PW_Basis::initparameters` 根据 `gamma_only` 与 `xprime` 设置 `fftnx/fftny` 半谱维度。 |
| 多 k 点平面波基类 | `source/source_basis/module_pw/pw_basis_k.h`、`pw_basis_k.cpp` | `PW_Basis_K::initparameters` 根据 k 点集合决定是否保留 `gamma_only`，并建立 `igl2isz_k` / `igl2ig_k` 映射。 |
| G 空间 stick 分布 | `source/source_basis/module_pw/pw_distributeg.cpp`、`pw_distributeg_method1.cpp` | `count_pw_st`、`collect_st` 在 GammaOnly 下缩小 x 或 y 扫描范围，只生成半谱 stick。 |
| 实/倒空间 FFT 调用链 | `source/source_basis/module_pw/pw_transform.cpp`、`pw_transform_k.cpp` | GammaOnly 的实数输入/输出路径使用 `fftxyr2c` 和 `fftxyc2r`，非 Gamma 路径使用 complex FFT。 |
| FFT 后端封装 | `source/source_base/module_fft/fft_bundle.cpp`、`fft_cpu.cpp`、`fft_cpu_float.cpp` | CPU 后端为 GammaOnly 创建 r2c/c2r FFTW plan，并按 `xprime` 决定半谱方向。 |
| gather/scatter 通信重排 | `source/source_basis/module_pw/pw_gatherscatter.h` | 通信逻辑不直接判断 GammaOnly，但依赖 `fftnx/fftny/nst/nstot/istot2ixy` 等半谱后的映射。 |
| 加速器路径 | `pw_transform_k.cpp` GPU 特化、`pw_transform_k_dsp.cpp` | GPU/DSP 的 `PW_Basis_K` 路径目前多处 `assert(gamma_only == false)`，不能直接复用 CPU 半谱路径。 |
| 现有测试 | `source/source_basis/module_pw/test/*`、`test_serial/pw_basis_k_test.cpp` | 已有若干 `gamma_only` 与 `xprime` 组合的模块测试；也有测试确认非 Gamma k 点集合会关闭 `PW_Basis_K::gamma_only`。 |

## 2. 与 GammaOnly 相关模块的实现逻辑

### 2.1 输入层与生产 PW 初始化

输入层中，`gamma_only` 参数当前标注为主要用于 localized orbitals。若 `basis_type == "pw"` 且用户设置 `gamma_only = true`，`read_input_item_elec_stru.cpp` 会把该参数重置为 `false`，同时自动生成只含 Gamma 点的 KPT 文件。这说明当前生产 PW 计算可以被改成“只取 Gamma 点”，但不会真正启用 PW 半谱 GammaOnly 算法。

生产 PW 初始化也体现了同样的限制：

```text
setup_pwrho:
  pw_rho->initparameters(false, 4.0 * ecutwfc)
  pw_rhod->initparameters(false, ecutrho)

setup_pwwfc:
  pw_wfc->initparameters(false, ecutwfc, nks, kvec_d)
```

因此，工作流D如果要进入真实 PW 生产路径，至少需要同步修改三层逻辑：

1. 输入层不再无条件禁止 PW GammaOnly，而是先做物理条件和功能条件判定；
2. `setup_pwrho` / `setup_pwwfc` 将判定后的 GammaOnly 状态传给 `PW_Basis` / `PW_Basis_K`；
3. 下游电荷、势、波函数和 IO 接口能识别半谱紧凑布局。

### 2.2 `PW_Basis` 的半谱网格初始化

`PW_Basis::initparameters` 是单 k 点 GammaOnly 网格设置入口。当前逻辑为：

```text
gamma_only = gamma_only_in
fftnx = nx
fftny = ny

if gamma_only:
    if xprime:
        fftnx = nx / 2 + 1
    else:
        fftny = ny / 2 + 1

fftnz = nz
fftnxy = fftnx * fftny
fftnxyz = fftnxy * fftnz
```

这里 `xprime` 决定半谱沿哪个方向压缩：

- `xprime == true`：保留非负 x 半轴，`fftnx = nx / 2 + 1`；
- `xprime == false`：保留非负 y 半轴，`fftny = ny / 2 + 1`。

这套半谱维度会继续传给 `distribute_g()`、`getstartgr()` 和 `fft_bundle.initfft()`，因此后续 stick 数量、通信计数和 FFT workspace 都会建立在半谱网格上。

### 2.3 `PW_Basis_K` 的 GammaOnly 判定与 k 点映射

`PW_Basis_K::initparameters` 在多 k 点场景下额外读取 `kvec_d` / `kvec_c`，并计算所有 k 点的最大模长 `kmaxmod`：

```text
gamma_only = gamma_only_in
if kmaxmod > 0:
    gamma_only = false
```

也就是说，当前 `PW_Basis_K` 支持的是“全部 k 点都是 Gamma 点时才能启用全局 GammaOnly”。只要 k 点集合中存在非 Gamma 点，整个 `PW_Basis_K` 都退回 full complex FFT 和完整 G 空间布局。

随后 `setuptransform()` 会执行：

```text
distribute_r()
distribute_g()
getstartgr()
setupIndGk()
fft_bundle.initfft(..., gamma_only, xprime)
fft_bundle.setupFFT()
```

其中 `setupIndGk()` 会按每个 k 点的 `|G+k|^2 <= gk_ecut` 建立：

- `npwk[ik]`：第 `ik` 个 k 点的本地平面波数量；
- `npwk_max`：所有 k 点中的最大本地平面波数量；
- `igl2isz_k[ik * npwk_max + igl]`：k 点局部平面波索引到 `(stick, z)` 的映射；
- `igl2ig_k[ik * npwk_max + igl]`：k 点局部平面波索引到 `PW_Basis` 全局本地 `ig` 的映射。

在全 GammaOnly 情况下，所有 k 点本质上都应是 Gamma 点，`igl2isz_k` 仍可复用半谱后的 `ig2isz`；在混合 k 点情况下，当前全局 `gamma_only` 布尔值无法表达“某些 k 点半谱、某些 k 点全谱”的差异。

### 2.4 G 空间 stick 分布与 `count_pw_st`

`PW_Basis::count_pw_st` 用于统计截断球内的 plane-wave stick，并生成：

- `st_length2D[ixy]`：某个 xy stick 上的平面波数量；
- `st_bottom2D[ixy]`：该 stick 的 z 方向起点；
- `nstot`：总 stick 数；
- `npwtot`：总平面波数。

非 GammaOnly 时，x/y/z 扫描范围覆盖正负方向。GammaOnly 时，扫描范围会在半谱方向被截断：

```text
if gamma_only and xprime:
    ix_start = 0
    ix_end   = fftnx - 1

if gamma_only and !xprime:
    iy_start = 0
    iy_end   = fftny - 1
```

`collect_st` 中也有相同的半谱扫描范围，用于收集、排序并分配 stick。`get_ig2isz_is2fftixy` 则把当前进程拥有的 stick 映射成 `ig2isz` 和 `is2fftixy`，并在 `xprime` 情况下维护 `ng_xeq0`，用于记录 `gx == 0` 的 G 分量数量。

因此，GammaOnly 的半谱不是只发生在 FFT 后端，而是在 G 空间分布阶段就已经减少了参与后续计算和通信的 stick 集合。

### 2.5 FFT 后端中的 r2c/c2r 半谱路径

`FFT_Bundle::initfft` 会把 `gamma_only` 和 `xprime` 传给 CPU FFT 后端。`FFT_CPU::initfft` 中会再次设置半谱维度，`setupFFT` 中则按 `xprime` 创建不同的 FFTW plan。

当 `xprime == true` 时：

```text
real2recip:
  planxr2c 处理 x 方向 real -> complex
  planyfor 处理 y 方向 complex FFT

recip2real:
  planybac 处理 y 方向反 FFT
  planxc2r 处理 x 方向 complex -> real
```

当 `xprime == false` 时：

```text
real2recip:
  planyr2c 处理 y 方向 real -> complex
  planxfor1 处理 x 方向 complex FFT

recip2real:
  planxbac1 处理 x 方向反 FFT
  planyc2r 处理 y 方向 complex -> real
```

z 方向 FFT 仍然是 complex-to-complex 的 `fftzfor` / `fftzbac`。因此当前 GammaOnly 半谱主要节省 xy 平面上一半独立谱分量，并通过 G 空间 stick 分布减少后续 z-FFT 和 gather/scatter 的数据规模。

### 2.6 `PW_Basis` 的实/倒空间变换路径

`PW_Basis::real2recip(const FPTYPE* in, ...)` 根据 `gamma_only` 分派：

```text
if gamma_only:
    实数输入 -> rspace
    fftxyr2c(rspace, auxr)
else:
    实数输入 -> auxr 的复数实部
    fftxyfor(auxr, auxr)

gatherp_scatters(auxr, auxg)
fftzfor(auxg, auxg)
out[ig] = auxg[ig2isz[ig]] / nxyz
```

`PW_Basis::recip2real(const complex* in, FPTYPE* out, ...)` 的反向路径为：

```text
auxg[:] = 0
auxg[ig2isz[ig]] = in[ig]
fftzbac(auxg, auxg)
gathers_scatterp(auxg, auxr)

if gamma_only:
    fftxyc2r(auxr, rspace)
    rspace -> 实数输出
else:
    fftxybac(auxr, auxr)
    real(auxr) -> 实数输出
```

因此，单 k 点 `PW_Basis` 内部已经具备完整的 CPU GammaOnly FFT 变换骨架：半谱 G 空间分布、r2c/c2r xy-FFT、complex z-FFT、gather/scatter 重排和 `ig2isz` 系数映射。

### 2.7 `PW_Basis_K` 的实/倒空间变换路径

`PW_Basis_K` 也已经有 GammaOnly 相关的 CPU 模板重载：

- `real2recip(const FPTYPE* in, complex* out, ik, ...)`：要求 `gamma_only == true`，走 `fftxyr2c`；
- `recip2real(const complex* in, FPTYPE* out, ik, ...)`：要求 `gamma_only == true`，走 `fftxyc2r`；
- complex 输入/输出版本要求 `gamma_only == false`，走 full complex FFT。

正向 GammaOnly 路径为：

```text
实数输入
  -> rspace
  -> fftxyr2c
  -> gatherp_scatters
  -> fftzfor
  -> 按 igl2isz_k 提取第 ik 个 k 点的系数
```

反向 GammaOnly 路径为：

```text
倒空间系数
  -> 按 igl2isz_k 写入 auxg
  -> fftzbac
  -> gathers_scatterp
  -> fftxyc2r
  -> 写回实数输出
```

这说明 `PW_Basis_K` 的 CPU 代码并非完全没有 GammaOnly 分支；真正的限制是当前只有全局 `gamma_only`，并且生产初始化和输入层没有接入 PW GammaOnly。

### 2.8 gather/scatter 与 GammaOnly 的关系

`pw_gatherscatter.h` 中的 `gatherp_scatters` / `gathers_scatterp` 不直接判断 `gamma_only`。它们只依赖初始化阶段生成的：

```text
fftnx, fftny, nst, nstot, istot2ixy,
numz, startz, numg, numr, startg, startr
```

当 GammaOnly 缩小 `fftnx` 或 `fftny` 后，stick 总数、通信计数和 xy 映射都会随之缩小。也就是说，gather/scatter 在设计上可以复用同一套逻辑，但必须保证半谱布局下的 `istot2ixy`、`ig2isz`、`igl2isz_k` 与 FFT 后端的内存布局一致。

### 2.9 当前紧凑存储状态

当前 `module_pw` 内部的半谱机制已经减少了 `PW_Basis` / `PW_Basis_K` 层面的 plane-wave 数量和 FFT 半谱维度，但我们可以更进一步：对电荷密度、势函数、波函数形成统一紧凑存储格式，并提供透明 pack/unpack 接口。

从现有生产初始化看，`setup_pwrho` 和 `setup_pwwfc` 仍使用非 GammaOnly PW 基组，因此下游 `rho(G)`、`V(G)`、`psi(G)` 还没有统一建立在半谱紧凑容器上。后续要做的不是只在 FFT 前后少算一半，而是要让上层数据结构能明确表达：

```text
full complex layout       : 显式存储 G 和 -G
gamma compact layout      : 只存储独立半谱分量
expanded temporary layout : 在需要兼容旧接口时临时展开
```

## 3. 当前实现的局限性与初步解决方向

| 当前局限性 | 影响 | 初步解决方向 |
| --- | --- | --- |
| PW 输入层会把 `gamma_only` 重置为 0 | 用户无法在真实 PW 生产计算中直接启用 module_pw 的半谱路径 | 在输入层增加 PW GammaOnly 条件判定，满足条件时保留 `gamma_only`；不满足时给出明确降级原因。 |
| `setup_pwrho` / `setup_pwwfc` 显式传入 `false` | 即使 KPT 只有 Gamma 点，电荷密度和波函数基组仍按 full complex 初始化 | 将判定后的 `gamma_only_pw` 传入 `PW_Basis` / `PW_Basis_K`，同时保持非 Gamma 情况的旧行为。 |
| `PW_Basis_K` 只有全局 `gamma_only` 布尔值 | 只支持“所有 k 点都是 Gamma”或“全部 full complex”，无法表达混合 k 点 | 第一阶段只支持全 Gamma k 点集合；第二阶段引入 per-k `is_gamma_k[ik]`，对 Gamma k 点走 r2c/c2r，对非 Gamma k 点走 complex FFT。 |
| 半谱方向由全局 `xprime` 决定 | 混合 k 点若共用同一 `PW_Basis_K`，半谱和全谱的 FFT 网格维度不同，难以共享一个 `fftnx/fftny` | 全 Gamma 阶段沿用全局 `xprime`；混合阶段考虑拆成 Gamma basis 与 non-Gamma basis，或引入双布局 workspace。 |
| `distribute_g` / `count_pw_st` 只按全局 `gamma_only` 扫描 | 无法为同一 k 点集合同时生成半谱和全谱 G 空间映射 | 全 Gamma 用现有半谱扫描；混合 k 点需按 k 点分类生成映射，或为 Gamma/非 Gamma 分别维护 `ig2isz` / `igl2isz_k`。 |
| `PW_Basis_K` GPU/DSP 路径要求 `gamma_only == false` | 加速器后端不能直接启用 GammaOnly | 第一阶段限定 CPU/MPI 路径；后续为 GPU/DSP 分别补 r2c/c2r 或明确 fallback 到 CPU。 |
| 当前紧凑存储没有统一容器 | 电荷密度、势函数、波函数仍可能按旧 full complex 接口传递，节省不彻底 | 设计 `GammaCompact` 类或轻量结构体，记录半谱方向、维度、是否含 Gamma 边界分量，并提供 pack/unpack。 |
| 共轭边界处理容易出错 | `G=0`、Nyquist 面、`gx=0` 或 `gy=0` 边界分量不能简单乘 2，否则能量和密度会偏差 | 在 pack/unpack 中显式处理自共轭分量；利用 `ng_xeq0` 等已有信息建立边界权重。 |
| 下游算符可能假设 full G 空间 | 非局域势、应力、电荷/势 IO、对称性操作等可能直接遍历 `npw` 或 `npwtot` | 先梳理所有 `rho_basis->gamma_only`、`PARAM.globalv.gamma_only_pw`、`ig2isz` 使用点，给旧接口提供只读展开视图。 |
| 物理适用性判定不足 | SOC、非共线、自旋相关复杂情况可能不满足实函数/时间反演前提 | 在输入层和初始化层集中判定：至少排除 `nspin=4`、SOC、非共线、非 Gamma k 点；共线自旋可作为单独验证项。 |
| 现有测试主要是模块级 | 缺少真实 PW 端到端 GammaOnly 与 full complex 的能量/力/密度对比 | 增加 Si/NaCl 全 Gamma 正确性测试、Fe 或 SOC/非共线降级测试、compact round-trip 测试。 |

综上，GammaOnly 工作流的核心安全边界是：先复用原仓库已有的 CPU 半谱 FFT 和半谱 stick 分布能力，把它从模块测试路径接入到 PW 生产路径；随后再推进统一紧凑存储、边界共轭权重和混合 k 点分派。这样可以把风险分成“启用已有半谱算法”和“扩展完整 GammaOnly 存储体系”两个阶段。

可能的实现顺序如下：
1. 先冻结安全边界：第一阶段只支持 CPU/MPI、全 Gamma k 点集合，不支持混合 k 点和 GPU/DSP GammaOnly。
2. 在 `PW_Basis` / `PW_Basis_K` 层补充更明确的 GammaOnly 单元测试，覆盖 `xprime=true/false`、`float/double`、`real2recip/recip2real` round-trip。
3. 将 `setup_pwrho` / `setup_pwwfc` 的硬编码 `false` 改为受控开关，但默认仍可降级到旧路径，先验证生产 PW 能走通半谱初始化。
4. 设计半谱紧凑存储结构，至少包含半谱方向、原始 full 维度、compact 维度、边界权重和底层数据指针。
5. 为 `rho(G)`、`V(G)`、`psi(G)` 提供 pack/unpack 或 expanded view，先保证旧下游接口可无感使用，再逐步改造成原生 compact 遍历。
6. 在全 Gamma 正确后，再评估混合 k 点：引入 `is_gamma_k[ik]`，为 Gamma k 点和非 Gamma k 点分别选择 r2c/c2r 或 complex FFT 路径。
7. 最后评估 GPU/DSP 半谱支持，决定是实现原生 r2c/c2r 后端，还是在 GammaOnly 模式下显式 fallback。

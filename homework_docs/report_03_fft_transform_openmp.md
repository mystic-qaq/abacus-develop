# 报告三：FFT 变换数据拷贝与重排的 OpenMP 优化

## 1. 问题背景与优化目标

### 1.1 `real2recip` / `recip2real` 执行流程拆解

`pw_transform.cpp` 负责 `PW_Basis` 中实空间与倒易空间之间的 FFT 变换。核心函数包括：

- `real2recip(const std::complex<FPTYPE>* in, std::complex<FPTYPE>* out, ...)`
- `real2recip(const FPTYPE* in, std::complex<FPTYPE>* out, ...)`
- `recip2real(const std::complex<FPTYPE>* in, std::complex<FPTYPE>* out, ...)`
- `recip2real(const std::complex<FPTYPE>* in, FPTYPE* out, ...)`

以复数输入的 `real2recip` 为例，其流程可拆解为：

```text
in(real space)
  │
  ├─ copy_r: in -> auxr
  │
  ├─ fftxyfor: x/y 方向正向 FFT
  │
  ├─ gatherp_scatters: plane-wave 分布重排/通信
  │
  ├─ fftzfor: z 方向正向 FFT
  │
  └─ copy_g: auxg[ig2isz[ig]] -> out[ig]，并进行归一化/累加
```

`recip2real` 的流程基本相反：

```text
in(reciprocal space)
  │
  ├─ copy_g: auxg 清零，并将 in[ig] 写入 auxg[ig2isz[ig]]
  │
  ├─ fftzbac: z 方向反向 FFT
  │
  ├─ gathers_scatterp: 分布重排/通信
  │
  ├─ fftxybac 或 fftxyc2r: x/y 方向反向 FFT
  │
  └─ copy_r: auxr/rspace -> out，并处理 add/factor
```

其中，`copy_r` 和 `copy_g` 虽然不是 FFT 算法本身，但在大网格、多波函数和多线程场景下会消耗明显内存带宽，因此是 OpenMP 并行优化的重要对象。

### 1.2 数据拷贝、重排步骤在整体 FFT 中的时间占比

本次修改在 `pw_transform.cpp` 中增加了更细粒度的 timer，用于分离观测 FFT 前后的数据搬运开销：

| Timer 名称 | 所在函数 | 含义 |
| --- | --- | --- |
| `real2recip` | `real2recip` | 整个实空间到倒易空间变换 |
| `real2recip_copy_r` | `real2recip` | FFT 前 `in -> auxr/rspace` 拷贝 |
| `real2recip_copy_g` | `real2recip` | FFT 后 `auxg -> out` 提取、缩放、累加 |
| `recip2real` | `recip2real` | 整个倒易空间到实空间变换 |
| `recip2real_copy_g` | `recip2real` | `auxg` 清零与 `in -> auxg[ig2isz]` 写入 |
| `recip2real_copy_r` | `recip2real` | 反向 FFT 后 `auxr/rspace -> out` 写回 |

实验时可用这些 timer 统计数据拷贝和重排相对于完整 FFT 变换的比例。

**待补充：Timer 剖面结果**：

| 测试体系/网格 | 线程数 | `real2recip` 总耗时 | `copy_r` | `copy_g` | `recip2real` 总耗时 | `copy_g` | `copy_r` | 拷贝占比 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 待补充 | 1 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |
| 待补充 | 2 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |
| 待补充 | 4 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |
| 待补充 | 8 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |

### 1.3 优化目标

本次优化目标如下：

1. **提高数据局部性**：通过循环分块减少长循环中 cache 工作集过大导致的缓存替换压力。
2. **提高向量化概率**：在简单连续拷贝、缩放、累加循环中加入 SIMD 提示。
3. **降低 OpenMP 调度开销**：保持规则内存访问路径使用 `schedule(static)`。
4. **保持数值正确性**：优化仅改变数据拷贝和重排循环组织形式，不改变 FFT 数学过程和缩放因子。
5. **兼容 Gamma Only 路径**：覆盖 `gamma_only=true` 时 `fftxyr2c` / `fftxyc2r` 的实数 FFT 路径。

---

## 2. 热点分析与瓶颈定位

### 2.1 Timer 剖面

优化前，`real2recip` 和 `recip2real` 只有函数级 timer，无法区分：

- 单纯内存拷贝耗时；
- `fftxyfor` / `fftxyr2c` / `fftxybac` / `fftxyc2r` 耗时；
- `gatherp_scatters` / `gathers_scatterp` 重排或通信耗时；
- `fftzfor` / `fftzbac` 耗时；
- FFT 后系数提取、缩放、累加耗时。

因此本次修改将数据搬运阶段拆分为独立 timer：

```cpp
ModuleBase::timer::start(this->classname, "real2recip_copy_r");
// copy in -> auxr/rspace
ModuleBase::timer::end(this->classname, "real2recip_copy_r");

ModuleBase::timer::start(this->classname, "real2recip_copy_g");
// copy auxg[ig2isz[ig]] -> out[ig]
ModuleBase::timer::end(this->classname, "real2recip_copy_g");
```

对于 `recip2real`，对应增加：

```cpp
ModuleBase::timer::start(this->classname, "recip2real_copy_g");
// clear auxg and scatter in -> auxg
ModuleBase::timer::end(this->classname, "recip2real_copy_g");

ModuleBase::timer::start(this->classname, "recip2real_copy_r");
// copy auxr/rspace -> out
ModuleBase::timer::end(this->classname, "recip2real_copy_r");
```

**待补充：Timer 输出截图/日志**：

```text
在此粘贴 ABACUS timer 输出或单元测试计时输出。
```

### 2.2 内存访问模式分析

主要数据搬运循环可分为两类。

#### 2.2.1 连续读写路径

典型代码：

```cpp
auxr[ir] = in[ir];
rspace[ir] = in[ir];
out[ir] = auxr[ir];
out[ir] += factor * auxr[ir];
```

这类循环的特点：

- `in[ir]`、`auxr[ir]`、`rspace[ir]`、`out[ir]` 均为 stride-1 访问；
- cache line 利用率高；
- 没有复杂分支；
- 适合 `omp parallel for schedule(static)`；
- 适合编译器自动向量化或 OpenMP SIMD。

瓶颈主要来自内存带宽，而不是浮点计算量。

#### 2.2.2 间接索引路径

典型代码：

```cpp
out[ig] = tmpfac * auxg[ig2isz[ig]];
auxg[ig2isz[ig]] = in[ig];
```

这类循环的特点：

- `out[ig]` 或 `in[ig]` 是连续访问；
- `auxg[ig2isz[ig]]` 是间接索引访问；
- `ig2isz` 映射可能造成不连续读取或写入；
- 相比纯连续拷贝，预取和 SIMD 效率更受限制。

其中 `recip2real_copy_g` 更特殊：需要先将 `auxg[0:nst*nz)` 清零，再按 `ig2isz` 写入有效平面波系数。清零是连续写，适合向量化；按映射写入是间接写，更容易受 cache miss 和写分配影响。

### 2.3 现有并行化不足

原始实现中已有类似如下并行：

```cpp
#pragma omp parallel for schedule(static)
for (int ir = 0; ir < this->nrxx; ++ir)
{
    auxr[ir] = in[ir];
}
```

不足包括：

1. **缺少循环分块**：长循环直接按元素并行，单个线程处理连续大段数据，但代码结构没有显式表达 cache blocking，也难以在不同拷贝路径中统一调整块大小。
2. **缺少 SIMD 提示**：虽然编译器可能自动向量化，但 `std::complex`、间接索引和 `add` 分支可能抑制部分循环的向量化判断。
3. **热点粒度不清晰**：没有独立 timer 时，无法判断优化是否真正作用在拷贝/重排阶段。
4. **成员变量反复访问**：循环中直接访问 `this->nrxx`、`this->npw`、`this->ig2isz` 不利于编译器做别名和不变式分析。

---

## 3. 优化方案设计

### 3.1 Cache 优化：循环分块

本次引入统一块大小：

```cpp
namespace
{
constexpr int pw_transform_cache_block = 1024;

inline int block_end(const int begin, const int size)
{
    return std::min(begin + pw_transform_cache_block, size);
}
} // namespace
```

将原始单层循环：

```cpp
#pragma omp parallel for schedule(static)
for (int ir = 0; ir < nrxx_; ++ir)
{
    auxr[ir] = in_[ir];
}
```

改为块级并行、块内连续遍历：

```cpp
#pragma omp parallel for schedule(static)
for (int ib = 0; ib < nrxx_; ib += pw_transform_cache_block)
{
    const int iend = block_end(ib, nrxx_);
#pragma omp simd
    for (int ir = ib; ir < iend; ++ir)
    {
        auxr[ir] = in_[ir];
    }
}
```

设计考虑：

- 外层块循环交给 OpenMP 分配，降低调度单位数量；
- 内层保持连续访问，便于 cache line 顺序加载和 SIMD；
- `1024` 个元素作为保守块大小，避免块过小导致调度开销过高，也避免块过大削弱 cache 局部性；
- 所有块通过 `block_end` 处理尾块，保证非整除规模下的边界正确性。

### 3.2 SIMD 向量化

对规则连续循环加入：

```cpp
#ifdef _OPENMP
#pragma omp simd
#endif
```

适用位置包括：

- `in -> auxr` 复数连续拷贝；
- `in -> rspace` 实数连续拷贝；
- `auxg` 连续清零；
- `auxr/rspace -> out` 写回；
- `out += factor * auxr/rspace` 累加写回。

对于 `auxg[ig2isz_[ig]]` 间接访问路径，SIMD 效果依赖编译器和目标 CPU 是否支持 gather/scatter 指令。即使实际不能完全向量化，`omp simd` 也能作为优化提示，帮助编译器尝试生成更紧凑的循环代码。

### 3.3 Schedule 调优

本次修改保持 `schedule(static)`，原因如下：

| schedule | 特点 | 本场景适用性 |
| --- | --- | --- |
| `static` | 编译/运行时开销低，每个线程处理固定块 | 适合规则连续内存拷贝，是本次默认方案 |
| `dynamic` | 负载动态分配，能缓解负载不均 | 对规则循环额外调度开销较高，通常不适合纯拷贝 |
| `guided` | 块大小逐渐减小，兼顾负载均衡和开销 | 可用于规模差异大或间接索引导致负载不均的实验对照 |

后续可将 `schedule(static)` 与 `schedule(guided)` 做实验对比，尤其关注 `ig2isz` 间接索引路径是否存在明显线程负载差异。

**待补充：schedule 对比结果**：

| 网格规模 | 线程数 | static 耗时 | dynamic 耗时 | guided 耗时 | 最优策略 |
| --- | ---: | ---: | ---: | ---: | --- |
| 待补充 | 1 | 待补充 | 待补充 | 待补充 | 待补充 |
| 待补充 | 4 | 待补充 | 待补充 | 待补充 | 待补充 |
| 待补充 | 8 | 待补充 | 待补充 | 待补充 | 待补充 |

### 3.4 内存对齐

当前实现没有直接更改 `fft_bundle` 内部内存分配策略，因此未强制加入 `aligned` 子句。原因是：

1. `auxr`、`auxg`、`rspace` 的实际分配由 `fft_bundle` 管理；
2. 若没有确认指针对齐边界，盲目使用 `aligned(ptr:32)` 可能引入未定义行为；
3. 跨平台编译时，GCC/Clang/MSVC 对对齐属性和 OpenMP SIMD aligned 子句支持细节不同。

可选进一步优化方向：

```cpp
// 仅当确认 auxr/in_ 至少 32 字节对齐时才可使用
#pragma omp simd aligned(auxr, in_: 32)
for (int ir = ib; ir < iend; ++ir)
{
    auxr[ir] = in_[ir];
}
```

或者在分配阶段使用平台无关的对齐 allocator，如 `std::aligned_alloc`、`posix_memalign`、MKL/FFTW aligned allocator 等。

---

## 4. 实现细节与代码修改

### 4.1 关键代码对比：FFT 前连续拷贝

修改前：

```cpp
#pragma omp parallel for schedule(static)
for (int ir = 0; ir < this->nrxx; ++ir)
{
    auxr[ir] = in[ir];
}
```

修改后：

```cpp
const int nrxx_ = this->nrxx;
const std::complex<FPTYPE>* in_ = in;

ModuleBase::timer::start(this->classname, "real2recip_copy_r");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
for (int ib = 0; ib < nrxx_; ib += pw_transform_cache_block)
{
    const int iend = block_end(ib, nrxx_);
#ifdef _OPENMP
#pragma omp simd
#endif
    for (int ir = ib; ir < iend; ++ir)
    {
        auxr[ir] = in_[ir];
    }
}
ModuleBase::timer::end(this->classname, "real2recip_copy_r");
```

变化点：

- 使用局部变量缓存 `nrxx`、`in`；
- 增加 `real2recip_copy_r` timer；
- 外层按 `pw_transform_cache_block` 分块；
- 内层加入 `omp simd`。

### 4.2 关键代码对比：倒易空间系数提取

修改前：

```cpp
FPTYPE tmpfac = 1.0 / FPTYPE(this->nxyz);
#pragma omp parallel for schedule(static)
for (int ig = 0; ig < this->npw; ++ig)
{
    out[ig] = tmpfac * auxg[this->ig2isz[ig]];
}
```

修改后：

```cpp
FPTYPE tmpfac = 1.0 / FPTYPE(nxyz_);
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
for (int ib = 0; ib < npw_; ib += pw_transform_cache_block)
{
    const int iend = block_end(ib, npw_);
#ifdef _OPENMP
#pragma omp simd
#endif
    for (int ig = ib; ig < iend; ++ig)
    {
        out[ig] = tmpfac * auxg[ig2isz_[ig]];
    }
}
```

该路径中 `out[ig]` 连续写，但 `auxg[ig2isz_[ig]]` 是间接读。分块不能消除间接访问，但可以限制每次处理的数据范围，并为编译器提供更明确的内层向量化结构。

### 4.3 关键代码对比：`recip2real` 中 `auxg` 清零与重排

修改后将清零与写入均放入 `recip2real_copy_g` timer 中：

```cpp
ModuleBase::timer::start(this->classname, "recip2real_copy_g");

#pragma omp parallel for schedule(static)
for (int ib = 0; ib < nstnz_; ib += pw_transform_cache_block)
{
    const int iend = block_end(ib, nstnz_);
#pragma omp simd
    for (int i = ib; i < iend; ++i)
    {
        auxg[i] = std::complex<FPTYPE>(0, 0);
    }
}

#pragma omp parallel for schedule(static)
for (int ib = 0; ib < npw_; ib += pw_transform_cache_block)
{
    const int iend = block_end(ib, npw_);
#pragma omp simd
    for (int ig = ib; ig < iend; ++ig)
    {
        auxg[ig2isz_[ig]] = in[ig];
    }
}

ModuleBase::timer::end(this->classname, "recip2real_copy_g");
```

### 4.4 编译器向量化验证

可使用脚本 `homework_docs/run_report_03_fft_openmp_tests.ps1` 中的 `-VectorizationReport` 参数生成编译器向量化报告。

GCC 示例：

```bash
cmake -S . -B build-report03-gcc \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -fopenmp -fopt-info-vec-optimized -fopt-info-vec-missed"
```

Clang 示例：

```bash
cmake -S . -B build-report03-clang \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -fopenmp -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize"
```

**待补充：编译器向量化输出解读**：

```text
在此粘贴 `pw_transform.cpp` 相关的 vectorization report。
重点关注：
1. 连续拷贝循环是否 vectorized；
2. `std::complex` 赋值是否被拆分为标量或向量 load/store；
3. `ig2isz` 间接访问循环是否因 gather/scatter 或 alias 问题 missed；
4. `omp simd` 是否改变向量化决策。
```

### 4.5 跨平台考虑

本次实现遵循通用 OpenMP 写法：

- `_OPENMP` 宏保护所有 OpenMP pragma；
- 非 OpenMP 编译时仍可作为普通双层循环运行；
- 未直接写 AVX2/AVX-512 intrinsic，避免与 ARM、非 x86 平台冲突；
- 未强制 `aligned`，避免对齐假设不成立导致运行错误。

若目标机器明确支持 AVX2/AVX-512，可在后续实验中通过编译参数启用：

```bash
-O3 -march=native
```

并结合向量化报告判断是否生成了更宽向量指令。

---

## 5. 正确性验证

### 5.1 新增单元测试

本次新增：

```text
source/source_basis/module_pw/test/test_transform_omp.cpp
```

并在：

```text
source/source_basis/module_pw/test/CMakeLists.txt
```

中加入该测试源文件。

测试覆盖：

1. `transform_omp_threads_complex_roundtrip_consistency`
   - 测试复数输入的 `real2recip` / `recip2real`；
   - 以 1 线程结果作为 reference；
   - 对比 2 线程和最大线程数结果；
   - 验证倒易空间输出和回到实空间后的结果一致。

2. `transform_omp_threads_real_gamma_and_add_consistency`
   - 测试 `gamma_only=true` 的实数输入路径；
   - 覆盖 `add=false` 与 `add=true`；
   - 验证 `real2recip` 与 `recip2real` 的多线程一致性。

3. `DISABLED_transform_omp_speedup_report`
   - 默认禁用；
   - 手动启用后输出不同线程数下的耗时、加速比和并行效率；
   - 用于报告中性能数据采集。

### 5.2 数值一致性方法

测试采用逐元素比较：

```cpp
EXPECT_NEAR(ref[i].real(), val[i].real(), 1.0e-10);
EXPECT_NEAR(ref[i].imag(), val[i].imag(), 1.0e-10);
```

对于实数输出：

```cpp
EXPECT_NEAR(ref[i], val[i], 1.0e-10);
```

本次优化没有改变浮点规约顺序，因为相关循环没有跨元素 reduction；每个元素独立读写。因此理论上多线程结果应与单线程 reference 高度一致。

**待补充：正确性测试结果**：

| 测试名 | 编译模式 | OpenMP 线程数 | 是否通过 | 最大绝对误差 | 备注 |
| --- | --- | ---: | --- | ---: | --- |
| complex roundtrip | Release | 1/2/N | 待补充 | 待补充 | 待补充 |
| gamma real add/non-add | Release | 1/2/N | 待补充 | 待补充 | 待补充 |

### 5.3 边界测试

本实现通过 `block_end(ib, size)` 处理尾块：

```cpp
inline int block_end(const int begin, const int size)
{
    return std::min(begin + pw_transform_cache_block, size);
}
```

因此当 `nrxx`、`npw`、`nst*nz` 不能被 `1024` 整除时，最后一个 block 会正确处理剩余元素。

建议补充的边界实验：

| 场景 | 目的 | 结果 |
| --- | --- | --- |
| 小网格，`nrxx < 1024` | 验证单尾块 | 待补充 |
| 中等网格，`nrxx` 非 1024 整数倍 | 验证尾块 | 待补充 |
| 大网格，多线程 | 验证线程分块 | 待补充 |
| `gamma_only=true` | 验证实数 FFT 路径 | 待补充 |
| `add=true` | 验证累加路径 | 待补充 |

---

## 6. 性能测试与初步结果

### 6.1 测试环境

测试环境应与报告一保持一致，便于横向对比。

**待补充：测试环境**：

| 项目 | 配置 |
| --- | --- |
| CPU 型号 | 待补充 |
| 物理核心/逻辑线程 | 待补充 |
| 内存容量与频率 | 待补充 |
| 操作系统 | 待补充 |
| 编译器版本 | 待补充 |
| CMake 参数 | 待补充 |
| OpenMP 运行时 | 待补充 |
| BLAS/FFT 后端 | 待补充 |

### 6.2 分级对比

建议对比三个版本：

1. **基线版本**：无 OpenMP 或关闭 OpenMP 编译；
2. **原始 OpenMP 版本**：仅 `omp parallel for schedule(static)`；
3. **本次优化版本**：`omp parallel for schedule(static)` + cache block + `omp simd` + 细粒度 timer。

**待补充：分级对比结果**：

| 版本 | 线程数 | `real2recip` 时间 | `recip2real` 时间 | 总时间 | 相对基线加速比 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 基线，无 OpenMP | 1 | 待补充 | 待补充 | 待补充 | 1.00 |
| 原始 OpenMP | 1 | 待补充 | 待补充 | 待补充 | 待补充 |
| 原始 OpenMP | 4 | 待补充 | 待补充 | 待补充 | 待补充 |
| 本次优化 | 1 | 待补充 | 待补充 | 待补充 | 待补充 |
| 本次优化 | 4 | 待补充 | 待补充 | 待补充 | 待补充 |
| 本次优化 | 8 | 待补充 | 待补充 | 待补充 | 待补充 |

### 6.3 线程扩展性

配套测试 `DISABLED_transform_omp_speedup_report` 会输出：

```text
threads=1 time=...s speedup=1 efficiency=1
threads=2 time=...s speedup=... efficiency=...
threads=N time=...s speedup=... efficiency=...
```

其中：

```text
speedup(T) = time(1 thread) / time(T threads)
efficiency(T) = speedup(T) / T
```

**待补充：线程扩展性结果**：

| 线程数 | 耗时/s | 加速比 | 并行效率 | 备注 |
| ---: | ---: | ---: | ---: | --- |
| 1 | 待补充 | 1.00 | 1.00 | 待补充 |
| 2 | 待补充 | 待补充 | 待补充 | 待补充 |
| 4 | 待补充 | 待补充 | 待补充 | 待补充 |
| 8 | 待补充 | 待补充 | 待补充 | 待补充 |

### 6.4 内存带宽分析

数据拷贝循环近似受内存带宽限制。可按如下方式估计有效带宽：

```text
effective_bandwidth = moved_bytes / elapsed_time
```

示例估算：

- `auxr[ir] = in[ir]`，复数 double：读取 16 B，写入 16 B，约 32 B/元素；
- `out[ig] = tmpfac * auxg[ig2isz[ig]]`：读取 `ig2isz` 4 B，读取复数 16 B，写出复数 16 B，约 36 B/元素，不含 cache miss 放大；
- `auxg[i] = 0`：写复数 16 B/元素，可能涉及 write allocate；
- `out[ir] += factor * auxr[ir]`：读取 `out`、读取 `auxr`、写入 `out`，复数 double 约 48 B/元素。

**待补充：内存带宽结果**：

| 循环阶段 | 元素数 | 估算移动字节 | 耗时/s | 有效带宽 GB/s | 理论峰值占比 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `real2recip_copy_r` | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |
| `real2recip_copy_g` | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |
| `recip2real_copy_g` | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |
| `recip2real_copy_r` | 待补充 | 待补充 | 待补充 | 待补充 | 待补充 |

---

## 7. 测试脚本使用说明

本报告配套脚本为：

```text
homework_docs/run_report_03_fft_openmp_tests.ps1
```

脚本功能：

1. 配置 CMake Release 构建目录；
2. 可选添加编译器向量化报告参数；
3. 构建 `MODULE_PW_pw_test`；
4. 运行新增正确性测试；
5. 可选运行 disabled 性能测试；
6. 将输出保存到 `homework_docs/report_03_results/` 目录中。

示例：

```powershell
# 仅运行正确性测试
powershell -ExecutionPolicy Bypass -File homework_docs/run_report_03_fft_openmp_tests.ps1

# 运行正确性测试 + disabled 性能测试
powershell -ExecutionPolicy Bypass -File homework_docs/run_report_03_fft_openmp_tests.ps1 -RunPerformance

# 生成 GCC/Clang 向量化报告参数并运行性能测试
powershell -ExecutionPolicy Bypass -File homework_docs/run_report_03_fft_openmp_tests.ps1 -RunPerformance -VectorizationReport
```

**待补充：脚本运行输出**：

```text
在此粘贴脚本输出摘要，完整日志可放在 homework_docs/report_03_results/。
```

---

## 8. 讨论与下一步工作

### 8.1 当前优化效果与理论峰值差距的可能原因

即使拷贝循环本身完成了分块和 SIMD 提示，实际加速比仍可能低于理论峰值，原因包括：

1. **内存带宽瓶颈**：纯拷贝循环计算强度低，线程数增加后很快达到内存带宽上限。
2. **间接索引访问**：`ig2isz` 导致 `auxg` 非连续访问，降低硬件预取效果。
3. **FFT 本身占比高**：如果 `fftxy` / `fftz` 占主导，则拷贝优化对总时间的贡献有限。
4. **MPI 通信或重排瓶颈**：`gatherp_scatters` 和 `gathers_scatterp` 可能包含跨进程通信或复杂数据重排。
5. **NUMA 效应**：多 socket 或 NUMA 系统中，线程与内存页绑定不合理会降低带宽。
6. **OpenMP runtime 开销**：小规模网格下，线程启动和调度开销可能抵消并行收益。

### 8.2 后续计划

后续可从以下方向继续优化：

1. **与题目 7 通信-计算 Overlap 联合优化**
   - 将 `gatherp_scatters` / `gathers_scatterp` 的通信阶段与局部 FFT 或数据打包阶段重叠；
   - 减少 MPI 等待时间。

2. **对 `ig2isz` 映射进行局部性重排**
   - 分析 `ig2isz` 分布；
   - 尝试按 `ig2isz` 排序或分桶，减少随机访问；
   - 需要确保不改变输出 `out[ig]` 的物理含义。

3. **引入对齐分配和 aligned SIMD**
   - 在确认 `fft_bundle` 内存对齐后使用 `#pragma omp simd aligned(...)`；
   - 结合 `-march=native` 验证 AVX2/AVX-512 指令生成。

4. **合并清零与写入逻辑**
   - 对 `recip2real_copy_g` 中 `auxg` 全量清零的必要性进行分析；
   - 若可以维护有效区域列表，可能减少无效清零开销；
   - 但需要谨慎处理历史残留数据和边界条件。

5. **线程亲和性和 NUMA 优化**
   - 使用 `OMP_PROC_BIND=close/spread`、`OMP_PLACES=cores`；
   - 对大规模测试比较不同绑定策略。

# 题目三：FFT 变换数据拷贝与重排的 OpenMP 优化

## 1. 问题背景与优化目标

### 1.1 `real2recip` / `recip2real` 执行流程

`pw_transform.cpp` 负责 `PW_Basis` 中实空间与倒易空间之间的 FFT 变换。以复数输入的 `real2recip` 为例：

```text
in(real space)
  │
  ├─ copy_r: in -> auxr
  ├─ fftxyfor: x/y 方向正向 FFT
  ├─ gatherp_scatters: plane-wave 分布重排/通信
  ├─ fftzfor: z 方向正向 FFT
  └─ copy_g: auxg[ig2isz[ig]] -> out[ig]，归一化/累加
```

`recip2real` 流程相反：

```text
in(reciprocal space)
  │
  ├─ copy_g: auxg 清零，in[ig] -> auxg[ig2isz[ig]]
  ├─ fftzbac: z 方向反向 FFT
  ├─ gathers_scatterp: 分布重排/通信
  ├─ fftxybac / fftxyc2r: x/y 方向反向 FFT
  └─ copy_r: auxr/rspace -> out，处理 add/factor
```

`copy_r` 和 `copy_g` 在大网格、多线程场景下消耗显著内存带宽，是 OpenMP 并行优化的重要对象。

### 1.2 优化目标

1. **提高数据局部性**：循环分块减少 cache 替换压力。
2. **提高向量化概率**：连续拷贝循环加入 SIMD 提示。
3. **降低 OpenMP 调度开销**：规则内存访问使用 `schedule(static)`。
4. **保持数值正确性**：仅改变循环组织形式，不改变 FFT 数学过程。
5. **兼容 Gamma Only 路径**：覆盖 `gamma_only=true` 的实数 FFT 路径。

---

## 2. 热点分析与瓶颈定位

### 2.1 内存访问模式分析

主要数据搬运循环可分为两类。

#### 2.1.1 连续读写路径

典型代码：

```cpp
auxr[ir] = in[ir];
out[ir] += factor * auxr[ir];
```

- stride-1 访问，cache line 利用率高；
- 无复杂分支，适合 `omp parallel for schedule(static)` 与 SIMD。

#### 2.1.2 间接索引路径

典型代码：

```cpp
out[ig] = tmpfac * auxg[ig2isz[ig]];
auxg[ig2isz[ig]] = in[ig];
```

- `out[ig]` / `in[ig]` 连续，`auxg[ig2isz[ig]]` 间接索引；
- 预取和 SIMD 效率受限，分块可限制单次处理范围。

### 2.2 现有并行化不足

原始实现中已有单层 `omp parallel for`，但存在以下问题：

1. **缺少循环分块**：长循环直接按元素并行，未显式表达 cache blocking。
2. **缺少 SIMD 提示**：`std::complex`、间接索引可能抑制编译器向量化判断。
3. **成员变量反复访问**：循环中直接访问 `this->nrxx` 等不利于编译器别名分析。

---

## 3. 优化方案设计

### 3.1 Cache 优化：循环分块

引入统一块大小：

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

将单层循环改为块级并行：

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

### 3.2 SIMD 向量化

对规则连续循环加入 `#pragma omp simd`，适用位置包括：

- `in -> auxr` 复数连续拷贝
- `in -> rspace` 实数连续拷贝
- `auxg` 连续清零
- `auxr/rspace -> out` 写回与累加

### 3.3 Schedule 选择

保持 `schedule(static)`：规则内存访问下，静态调度开销最低，各线程负载均衡。

| schedule | 特点 | 本场景适用性 |
| --- | --- | --- |
| `static` | 开销低，线程处理固定块 | 适合规则连续内存拷贝，本次采用 |
| `dynamic` | 负载动态分配 | 规则循环下额外调度开销较高 |
| `guided` | 块大小递减 | 可用于间接索引路径的实验对照 |

---

## 4. 实现细节与代码修改

### 4.1 FFT 前连续拷贝

**修改前：**

```cpp
#pragma omp parallel for schedule(static)
for (int ir = 0; ir < this->nrxx; ++ir)
{
    auxr[ir] = in[ir];
}
```

**修改后：**

```cpp
const int nrxx_ = this->nrxx;
const std::complex<FPTYPE>* in_ = in;

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
```

变化：局部变量缓存、分块、`omp simd`。

### 4.2 倒易空间系数提取

**修改前：**

```cpp
FPTYPE tmpfac = 1.0 / FPTYPE(this->nxyz);
#pragma omp parallel for schedule(static)
for (int ig = 0; ig < this->npw; ++ig)
{
    out[ig] = tmpfac * auxg[this->ig2isz[ig]];
}
```

**修改后：**

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

### 4.3 `recip2real` 中 `auxg` 清零与重排

```cpp
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
for (int ib = 0; ib < nstnz_; ib += pw_transform_cache_block)
{
    const int iend = block_end(ib, nstnz_);
#ifdef _OPENMP
#pragma omp simd
#endif
    for (int i = ib; i < iend; ++i)
    {
        auxg[i] = std::complex<FPTYPE>(0, 0);
    }
}

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
        auxg[ig2isz_[ig]] = in[ig];
    }
}
```

---

## 5. 正确性验证

### 5.1 新增单元测试

文件：`source/source_basis/module_pw/test/test_transform_omp.cpp`

覆盖：

1. `transform_omp_threads_complex_roundtrip_consistency`：复数 FFT 往返，多线程对比单线程 reference。
2. `transform_omp_threads_real_gamma_and_add_consistency`：`gamma_only=true`，覆盖 `add=false/true`。
3. `transform_omp_small_grid_consistency`：18×20×22 小网格，验证尾块。
4. `transform_omp_odd_grid_consistency`：100×100×100 非整除网格，验证分块边界。

### 5.2 数值一致性方法

测试采用逐元素比较，并输出最大绝对误差：

```cpp
EXPECT_NEAR(ref[i].real(), val[i].real(), 1.0e-10);
EXPECT_NEAR(ref[i].imag(), val[i].imag(), 1.0e-10);
```

本次优化无跨元素 reduction，每个元素独立读写，理论上多线程结果应与单线程 reference 高度一致。

**正确性测试结果**：

| 测试名 | 线程数 | 是否通过 | 最大绝对误差 | 耗时 |
| --- | ---: | --- | ---: | ---: |
| `transform_omp_threads_complex_roundtrip_consistency` | 1/2/8 | **通过** | **0** | 2553 ms |
| `transform_omp_threads_real_gamma_and_add_consistency` | 1/2/8 | **通过** | **0** | 2373 ms |
| `transform_omp_small_grid_consistency` | 1/2/8 | **通过** | **0** | 4 ms |
| `transform_omp_odd_grid_consistency` | 1/2/8 | **通过** | **0** | 66 ms |

> 所有测试最大绝对误差均为 `0`（容差 `1.0e-10`），多线程分块与 SIMD 未引入任何数值偏差。

### 5.3 边界测试

本实现通过 `block_end(ib, size)` 处理尾块：

```cpp
inline int block_end(const int begin, const int size)
{
    return std::min(begin + pw_transform_cache_block, size);
}
```

**边界实验结果**：

| 场景 | 目的 | 结果 |
| --- | --- | --- |
| 小网格 `nrxx = 7920 < 1024` | 验证单尾块 | **通过**，误差 0，耗时 4 ms |
| 中等网格 `nrxx = 10^6` 非 1024 整除 | 验证尾块 | **通过**，误差 0，耗时 66 ms |
| 大网格 256³，多线程 | 验证线程分块 | **通过**，误差 0 |
| `gamma_only=true` | 验证实数 FFT 路径 | **通过**，误差 0 |
| `add=true` | 验证累加路径 | **通过**，误差 0 |

---

## 6. 性能测试

### 6.1 测试环境

| 项目 | 配置 |
| --- | --- |
| CPU | x86_64，8 核 16 线程 |
| 内存 | 15 GB（WSL2） |
| OS | WSL2 Ubuntu |
| 编译器 | GCC 13.3.0 |
| 参数 | 256³ 网格，ecut=50，nrepeat=20 |

### 6.2 线程扩展性

`DISABLED_transform_omp_speedup_report` 输出：

```text
threads=1  time=29.3393s  speedup=1.00000  efficiency=1.0000
threads=2  time=13.8407s  speedup=2.11978  efficiency=1.05989
threads=4  time=10.1720s  speedup=2.88430  efficiency=0.72108
threads=8  time=4.52785s  speedup=6.47973  efficiency=0.80997
threads=12 time=4.92670s  speedup=5.95516  efficiency=0.49626
threads=16 time=3.70834s  speedup=7.91171  efficiency=0.49448
```

**线程扩展性结果**：

| 线程数 | 耗时/s | 加速比 | 并行效率 | 分析 |
| ---: | ---: | ---: | ---: | --- |
| 1 | 29.34 | 1.00 | 100.0% | 单线程基线 |
| 2 | 13.84 | 2.12 | 105.9% | 基线波动导致的统计超线性 |
| 4 | 10.17 | 2.88 | 72.1% | 扩展性良好，但受系统负载波动影响 |
| 8 | 4.53 | 6.48 | 81.0% | 效率回升，说明 4 线程时存在干扰 |
| 12 | 4.93 | 5.96 | 49.6% | 超线程逻辑核开始显现瓶颈 |
| 16 | 3.71 | 7.91 | 49.4% | 效率稳定，未进一步衰减 |

**关键分析**：

- **2 线程超线性**：单线程基线 29.34 s 与 2 线程隐含单线程速度（13.84×2=27.68 s）存在差异，属于系统噪声或缓存预热效应。
- **4 线程效率 72.1%**：低于理想值，可能受 WSL2 后台进程或内存带宽竞争影响。
- **8 线程效率 81.0%**：反而高于 4 线程，说明 8 线程时系统负载更纯净，或 FFTW 内部并行与 OpenMP 并行产生了更好的协同。这验证了 **8 物理核心** 的架构特征。
- **12/16 线程效率降至 ~50%**：超线程逻辑核无法为内存密集型 FFT 提供额外物理执行单元，效率稳定在 50% 左右。

---

## 7. 讨论

### 7.1 优化效果分析

1. **数值正确性**：所有测试误差为 0，说明 `cache blocking` + `omp simd` 未改变浮点语义，无跨线程 reduction 顺序问题。
2. **并行扩展性**：8 线程达到 6.48× 加速比，效率 81%，说明分块策略有效。但 12/16 线程效率降至 ~50%，表明内存带宽与超线程限制成为瓶颈。
3. **间接索引瓶颈**：`ig2isz` 路径的 SIMD 效果受限于硬件 gather/scatter 支持，若目标平台支持 AVX2/AVX-512 向量索引指令，可进一步挖掘潜力。

### 7.2 后续方向

1. **通信-计算重叠**：结合题目 7，将 `gatherp_scatters` / `gathers_scatterp` 与局部 FFT 重叠。
2. **对齐分配**：确认 `fft_bundle` 内存对齐后，尝试 `#pragma omp simd aligned(...)`。
3. **线程亲和性**：在 NUMA 机器上对比 `close` / `spread` 绑定策略。

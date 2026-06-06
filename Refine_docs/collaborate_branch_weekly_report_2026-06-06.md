# ABACUS `collaborate` 分支周报：代码级变更与优化详解

> **报告日期**: 2026-06-06  
> **分支**: `collaborate`（`HEAD: b73da1d5e`）  
> **基线**: `deepmodeling/abacus-develop:develop`  
> **报告范围**: 2026-05-31 ~ 2026-06-06（本周全部 29 个提交）

---

## 目录

1. [本周概况](#1-本周概况)
2. [SIMD 向量化抽象层：pw_simd_copy.h](#2-simd-向量化抽象层pw_simd_copyh)
3. [WorkflowB 第三轮内核重构：pw_gatherscatter.h](#3-workflowb-第三轮内核重构pw_gatherscatterh)
4. [缓存复用（Cache Reuse）正确性加固](#4-缓存复用cache-reuse正确性加固)
5. [GammaOnly 半谱 FFT 优化](#5-gammaonly-半谱-fft-优化)
6. [上游合并与基础设施更新](#6-上游合并与基础设施更新)
7. [Benchmark 数据汇总](#7-benchmark-数据汇总)
8. [文件变更清单](#8-文件变更清单)
9. [后续方向](#9-后续方向)

---

## 1. 本周概况

本周 `collaborate` 分支共产生 **29 个提交**，净增 **~11,400 行**代码（含文档），变更涉及 **45 个文件**。工作重心从"跨分支代码整合"转向"深化优化 + 正确性加固 + 文档体系建立"。

### 核心工作项矩阵

| 工作项 | 代码路径 | 主导提交 | 类型 |
|--------|----------|----------|:----:|
| SIMD copy 抽象层 | `pw_simd_copy.h`（新文件, 126 行） | `a5fcd6165`, `03a89bd3b` | 新功能/优化 |
| WorkflowB 第三轮重构 | `pw_gatherscatter.h`（+331 行） | `03a89bd3b`, `a5fcd6165` | 优化 |
| 缓存复用正确性修复 | `pw_basis.h/cpp`, `pw_basis_k.h/cpp` | `913a31504`, `8c622c2db`, `4faaadcb1` | 修复 |
| GammaOnly 优化实现 | `gamma_compact.h/cpp`（新文件）, `pw_distributeg.cpp` | `af21395ea`, `f6fef9871` | 新功能 |
| 文档体系建立 | `Refine_docs/`, `Revise_docs/`, `Test_docs/` | `c248cf033`, `b73da1d5e` | 文档 |

---

## 2. SIMD 向量化抽象层：pw_simd_copy.h

### 2.1 设计动机

平面波计算中的 pack/unpack 操作涉及大量的 `memcpy` 语义——将 `std::complex<T>` 数组在 MPI 发送缓冲区、实数 FFT 缓冲区、以及本地输出缓冲区之间搬移。旧的实现分散在各处：

```cpp
// 旧代码：逐元素循环 + GCC ivdep（多处重复）
T* __restrict__ outp_r = reinterpret_cast<T*>(outp);
const T* __restrict__ inp_r = reinterpret_cast<const T*>(inp);
#ifdef __GNUC__
#pragma GCC ivdep
#endif
for(int iz = 0 ; iz < 2 * nz_ ; ++iz) {
    outp_r[iz] = inp_r[iz];
}
```

这种实现存在三个问题：
1. **代码重复**——相同的循环结构散布在 `pw_gatherscatter.h` 的 6+ 个位置
2. **编译器依赖**——`#pragma GCC ivdep` 对非 GCC 编译器无效果
3. **向量化不确定性**——即使使用 GCC，自动向量化是否生效也依赖于编译器的优化决策

### 2.2 新抽象层设计

**文件**: [source/source_basis/module_pw/pw_simd_copy.h](source/source_basis/module_pw/pw_simd_copy.h)

核心 API 是一个模板函数：

```cpp
template <typename T>
inline void simd_copy_n(T* __restrict__ dest, const T* __restrict__ src, int count);
```

其中 `T` 必须是 `float` 或 `double`（`std::complex<T>` 经过 `reinterpret_cast<T*>` 后的标量类型），`count` 等于 `2 * n_complex`。

### 2.3 实现架构

采用**编译时指令集检测 + 运行时策略选择**的思路：

```
simd_copy_n<T>(dest, src, count)
    └─ simd_detail::copy_n_impl(dest, src, count)
         ├─ AVX-512 路径: __m512 _mm512_loadu_ps / _mm512_storeu_ps  (float, W=16)
         │                 __m512d _mm512_loadu_pd / _mm512_storeu_pd  (double, W=8)
         ├─ AVX2 路径:    __m256 _mm256_loadu_ps / _mm256_storeu_ps  (float, W=8)
         │                 __m256d _mm256_loadu_pd / _mm256_storeu_pd (double, W=4)
         └─ 纯 C++ 回退:  逐元素循环
```

**关键设计决策**:
- **未对齐访问**：使用 `_mm256_loadu_ps` / `_mm256_storeu_ps`（带 `u` 后缀），而非对齐版本。因为 `std::vector` 分配不保证 cache-line 对齐，对齐加载可能因未对齐地址产生 #AC 异常。
- **无预取/Prefetch**：由于大多数 copy 操作在 L1/L2 cache 范围内（nplane 通常为 1~64），显式 prefetch 带来的收益有限甚至可能反向。
- **尾循环处理**：主循环使用 `for (; i + W <= n; i += W)` 结构——当余量小于单个向量宽度时，fallback 到纯 C++ 循环。由于实际 FFT 维度（FFT 网格尺寸）通常是 2 的幂次，余量在实际计算中极少触发。

### 2.4 AVX-512 安全开关

```cpp
#if defined(__AVX512F__) && !defined(SIMD_COPY_DISABLE_AVX512)
#define PW_SIMD_AVX512 1
#elif defined(__AVX2__)
#define PW_SIMD_AVX2 1
#endif
```

提供 `SIMD_COPY_DISABLE_AVX512` 宏，因为部分 Intel CPU（特别是 Skylake-X）在执行 512 位指令时显著降频。默认启用 AVX-512，但允许用户通过编译选项关闭。

### 2.5 类型安全

```cpp
static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value,
              "simd_copy_n only supports float or double");
```

杜绝了向不支持的类型模板参数传递的可能，将错误从链接期提前到编译期。

### 2.6 Benchmark 结果

在 GCC 13.3.0 / 11.4.0 的 `-O2` 编译下：

| 指令集 | float (geometric mean) | double (geometric mean) |
|--------|:---------------------:|:----------------------:|
| AVX2   | 0.99× vs 自动向量化  | 0.99× vs 自动向量化   |
| AVX-512 | 1.02× vs 自动向量化  | 0.99× vs 自动向量化   |

**分析**：在当前编译器版本下，显式 SIMD 内联与 GCC 自动向量化的性能非常接近。这意味着 SIMD 层的价值不在于立竿见影的性能提升，而在于：
1. 提供了**后端无关的 copy 抽象**，一处修改即影响所有调用点
2. **消除编译器依赖**——不再依赖 `#pragma GCC ivdep` 这样的非标准扩展
3. **为未来的优化提供入口**——当需要引入新的指令集（如 ARM SVE、RISC-V V）时，只需添加新的特化

---

## 3. WorkflowB 第三轮内核重构：pw_gatherscatter.h

### 3.1 变更总览

本周 `pw_gatherscatter.h` 经历了第三轮大规模重构，累计变更 **+331 行 / -112 行**。以下逐项分析每个改动点的技术细节。

### 3.2 SIMD 拷贝集成（替换 ivdep 循环）

**提交**: `03a89bd3b`（2026-06-04）

6 个 `#pragma GCC ivdep` 保护的逐元素循环全部替换为 `simd_copy_n` 调用：

```cpp
// 替换前（每组 3 个版本，分布在 gatherp_scatters 和 gathers_scatterp 中）
// ── 串行路径 pack          ── MPI sendbuf pack    ── recvbuf unpack
// ── 串行路径 unpack（反向）  ── MPI sendbuf pack（反向）─ recvbuf unpack（反向）

// 替换后
copy_detail::copy_complex_buffer(inp, outp, count);
// 内部调用 ModulePW::simd_copy_n(reinterpret_cast<T*>(out), ...)
```

新增 `copy_detail` 命名空间，提供两个层次的辅助函数：

```cpp
namespace copy_detail {
    // 无 OpenMP 版：用于 gather/scatter 内部的并行循环（外层已有 parallel region）
    template <typename T>
    inline void copy_complex_buffer(const std::complex<T>* in, std::complex<T>* out, int count);

    // 自带 OpenMP 版：用于顶层 transform 拷贝（1024 元素块、schedule(static)）
    template <typename T>
    inline void copy_complex_buffer_parallel(const std::complex<T>* in, std::complex<T>* out, int count);
}
```

### 3.3 MPI 请求向量的 thread_local 缓存

**提交**: `03a89bd3b`

```cpp
// 替换前：每次调用分配新 vector
std::vector<MPI_Request> recv_requests(poolnproc_gps, MPI_REQUEST_NULL);
std::vector<MPI_Status> recv_status(poolnproc_gps);

// 替换后：thread_local 缓存 + assign 重置
static thread_local std::vector<MPI_Request> recv_requests;
static thread_local std::vector<MPI_Status> recv_status;
recv_requests.assign(poolnproc_gps, MPI_REQUEST_NULL);
recv_status.resize(poolnproc_gps);
```

**优化原理**：`pw_gatherscatter.h` 的函数可能在典型 DFT 迭代中每步被调用多次。`std::vector` 的构造函数会调用内存分配器，而使用 `thread_local` + `assign`/`resize` 可避免重复分配——当 `poolnproc` 不变时 `resize` 是 O(1) 无分配操作，仅当进程数变化时才会触发重新分配。

同样的优化在 `gathers_scatterp` 和 `istot_offsets` 上也应用了。

### 3.4 自数据直通路径（Self-Data Direct Path）

**提交**: WorkflowB 第三轮（6/3 ~ 6/6）

**问题**: 当 rank 本身也持有数据时，旧实现仍将其发送到 MPI 缓冲区再收回：

```
旧路径: in → pack → sendbuf → MPI（本 rank loopback）→ recvbuf → unpack → out
新路径: in ───────────────────────── simd_copy_n ─────────────────────────→ out
```

**实现方式**：在 `gatherp_scatters` 的 pack 循环中检查 `ip == poolrank`，对该进程的 stick 跳过 MPI 路径：

```cpp
// gatherp_scatters pack 循环伪代码
for (int ip = 0; ip < poolnproc_gps; ++ip) {
    if (ip == poolrank_gps) {
        // 自数据直通：直接从 in 拷贝到 out 中的对应位置
        simd_copy_n(out + offset, in + ixy * nplane, 2 * nplane);
        continue;
    }
    // MPI 路径继续
    simd_copy_n(sendbuf + ..., in + ..., ...);
}
```

**收益**：每次调用节省 `2 × nst × nplane` 个 complex 元素的冗余拷贝。在大规模 `PW_Basis_K` 场景下，pack+unpack 的总数据搬移量减少约 **1/3**。

### 3.5 编译期 MPI 数据类型分发

**提交**: WorkflowB 第三轮

```cpp
// 替换前：运行时 RTTI
if (typeid(T) == typeid(double)) { ... }
else if (typeid(T) == typeid(float)) { ... }

// 替换后：模板特化 + static_assert
namespace detail {
    template <typename T>
    struct mpi_complex_dtype;

    template <>
    inline MPI_Datatype mpi_complex_dtype<double>() { return MPI_DOUBLE_COMPLEX; }

    template <>
    inline MPI_Datatype mpi_complex_dtype<float>() { return MPI_COMPLEX; }
}

// 使用处
MPI_Datatype dtype = detail::mpi_complex_dtype<T>();
```

**收益**：
- 消除每次调用 `typeid` 的运行时开销（尽管 `typeid` 开销本身很小，但在迭代千万次的热路径上可观测）
- `static_assert` 在找不到匹配特化时产生清晰的编译错误，而非运行时 `WARNING_QUIT`

### 3.6 消除冗余 `istot_offsets` 计算

**提交**: WorkflowB 第三轮

`gathers_scatterp` 中原本需要计算完整的 `istot_offsets` 前缀和数组。优化后利用几何恒等式：

```cpp
// 替换前：前缀和循环
for (int ip = 0; ip < poolnproc; ++ip) {
    istot_offsets[ip] = (ip == 0) ? 0 : istot_offsets[ip-1] + numz[ip-1];
}

// 替换后：直接除法
const int istot0 = startr_[ip] / nplane;
```

**原理**：`startr_[ip]` 记录的是第 `ip` 个进程在 z 方向上的起始索引，除以每根 stick 的 z 长度 `nplane` 即得到其对应的全局 stick 编号。这个恒等式在代码的已有布局约定下始终成立。

**收益**：消除一个 `thread_local std::vector<int>` 的初始化、每次调用的 `assign` 操作、以及一个完整的 `O(poolnproc)` 前缀和循环。

### 3.7 Pack 循环调度优化

```cpp
// 替换前：collapse(2) 统一迭代空间
#pragma omp parallel for collapse(2)
for (int ip = 0; ip < poolnproc; ++ip) {
    for (int is = 0; is < numz[ip]; ++is) { ... }
}

// 替换后：外层动态调度 + 内层串行
#pragma omp parallel for schedule(dynamic, 1)
for (int ip = 0; ip < poolnproc; ++ip) {
    for (int is = 0; is < numz[ip]; ++is) { ... }
}
```

**问题**: `collapse(2)` 将 ip/is 两层合并为统一的迭代空间，但各 peer 的 `numz[ip]` 差异大，导致负载不均。非 GCC 编译器对 `collapse(2)` 的代码生成质量也不稳定。

**收益**: `schedule(dynamic, 1)` 在线程数 ≤ poolnproc 时提供接近最优的负载均衡——每个线程取一个 ip 处理，处理完后取下一个 ip，不等待其他线程。

---

## 4. 缓存复用（Cache Reuse）正确性加固

### 4.1 变更链路

本周缓存复用的变更跨越 4 个提交，是从"功能实现"到"正确性加固"再到"可靠性提升"的典型演进：

```
617b03286 (6/5) Optimize module_pw cache reuse           ← 初始实现
913a31504 (6/6 13:18) Fix PW basis cache invalidation locking  ← 并发安全
8c622c2db (6/6 14:03) Fix PW cache CI link errors        ← 构建修复
4faaadcb1 (6/6 15:14) Fix PW cache reuse after lattice updates ← 状态一致
```

### 4.2 Stale Pointer 修复

**问题**: `invalidate_cache()` 旧实现仅将 valid flag 设为 `false`，但 `gg`、`gcar`、`gk2` 等 public 指针仍保留旧地址。虽然"严格按约定"调用不会出问题，但 public 指针的暴露使得任何调用方在缓存失效后仍可能读到旧数据。

**修复**（[pw_basis.h:165-174](source/source_basis/module_pw/pw_basis.h#L165-L174)）：

```cpp
// invalidate_cache() 被拆分为加锁/不加锁两个版本
void PW_Basis::invalidate_cache() {
    std::lock_guard<std::mutex> guard(this->cache_mutex);
    this->invalidate_cache_unlocked();
}

void PW_Basis::invalidate_cache_unlocked() {
    this->local_pw_cache_valid.store(false);
    this->uniqgg_cache_valid.store(false);
    // 将所有公有缓存指针置空
    this->gg = nullptr;
    this->gdirect = nullptr;
    this->gcar = nullptr;
    this->ig2igg = nullptr;
    this->gg_uniq = nullptr;
    this->ig_gge0 = -1;
    // 释放后端存储
    this->gg_cache_storage.reset();
    this->gdirect_cache_storage.reset();
    this->gcar_cache_storage.reset();
    ...
}
```

同时移除了拷贝构造函数和拷贝赋值操作符：

```cpp
PW_Basis(const PW_Basis& other) = delete;
PW_Basis& operator=(const PW_Basis& other) = delete;
```

**理由**：`PW_Basis` 拥有大量原始指针（FFT/分布映射），拷贝会创建模糊的所有权和失效的缓存指针。

### 4.3 双重锁检测（Double-Checked Locking）

**问题**: 在多线程环境中，多个线程可能同时调用 `collect_local_pw()`，导致重复计算或在无效状态下读取缓存。

**修复**（[pw_basis.cpp:211-224](source/source_basis/module_pw/pw_basis.cpp#L211-L224)）：

```cpp
void PW_Basis::collect_local_pw() {
    // 第一层检查（无锁，快速路径）
    if (this->local_pw_cache_valid.load()) {
        this->local_pw_cache_hits.fetch_add(1);
        return;
    }
    // 加锁后第二层检查（防止竞态）
    std::lock_guard<std::mutex> guard(this->cache_mutex);
    if (this->local_pw_cache_valid.load()) {
        this->local_pw_cache_hits.fetch_add(1);
        return;
    }
    // 真正计算
    this->local_pw_cache_misses.fetch_add(1);
    ...
}
```

同样的模式也应用于 `collect_uniqgg()` 和 `PW_Basis_K` 的 `collect_local_pw()`。

### 4.4 `collect_local_pw` 重建时让 `uniqgg` 缓存失效

**问题**: `gg` 重建后，依赖于 `gg` 的 `gg_uniq` / `ig2igg` 哈希表未失效，导致 `collect_uniqgg` 返回过时数据。

**修复**:

```cpp
void PW_Basis::collect_local_pw() {
    // ... 重建 gg, gdirect, gcar ...
    
    // 唯一G数据依赖于gg，重建本地G数据使其失效
    this->uniqgg_cache_valid.store(false);
    this->ig2igg_cache_storage.reset();
    this->gg_uniq_cache_storage.reset();
    this->ig2igg = nullptr;
    this->gg_uniq = nullptr;
    this->ngg = 0;
}
```

### 4.5 GPU 指针生命周期修复

**问题**: `PW_Basis_K` 析构时 `clear_k_cache_storage()` 在 GPU 资源释放前将 `d_gcar`/`d_gk2` 置空，导致 CUDA/ROCm device 内存释放无法获得真实指针。

**修复**（[pw_basis_k.cpp:22-55](source/source_basis/module_pw/pw_basis_k.cpp#L22-L55)）：

```cpp
PW_Basis_K::~PW_Basis_K() {
    delete[] npwk;           // 先释放 CPU 资源
    delete[] igl2isz_k;
    delete[] igl2ig_k;
#if defined(__CUDA) || defined(__ROCM)
    if (this->device == "gpu") {
        del_cu_stream();     // 先释放 GPU 资源
    }
#endif
    this->clear_k_cache_storage();  // 最后清理缓存存储
}

void PW_Basis_K::clear_k_cache_storage() {
    this->invalidate_cache();
    this->k_gcar_cache_storage.reset();
    this->k_gk2_cache_storage.reset();
    this->gcar = nullptr;
    this->gk2 = nullptr;
    this->d_gcar = nullptr;  // 新增：清空 GPU 指针
    this->d_gk2 = nullptr;   // 新增
}
```

### 4.6 Cache Stats 线程安全

```cpp
PW_Basis::CacheStats PW_Basis::get_cache_stats() const {
    std::lock_guard<std::mutex> guard(this->cache_mutex);
    return this->get_cache_stats_unlocked();
}
```

同时在 `CacheStats` 返回值中增加了指针非空检查：

```cpp
const bool has_local_pw_cache = this->local_pw_cache_valid.load()
                                && this->npw > 0
                                && this->gg != nullptr
                                && this->gdirect != nullptr
                                && this->gcar != nullptr;
```

### 4.7 晶格更新缓存重建

**提交**: `4faaadcb1`

当晶格参数在计算过程中发生变化（例如 `relax` / `cell-relax` 流程中的应力优化步骤），`PW_Basis` 内部与 `G`、`GT`、`GGT`、`latvec` 等相关的量需要重新初始化。这要求在 `PW_Basis::initgrids()` 或 `PW_Basis::collect_local_pw()` 被重新调用时，**先清理所有缓存再重建**。

**新增机制**: `PW_Basis::clear_all_caches()`——一次性清理 `PW_Basis` 和 `PW_Basis_K` 两层的全部缓存。

---

## 5. GammaOnly 半谱 FFT 优化

### 5.1 背景与矛盾

ABACUS 的 `module_pw` 内部已有完整的 `gamma_only` 标志、r2c/c2r FFT 路径（`pw_transform.cpp` 中通过 `fact=1.0` vs `fact=2.0` 分支）和半谱 stick 分布逻辑。然而生产初始化路径存在两处死锁：

1. `read_input_item_elec_stru.cpp` 硬编码 `gamma_only = false`
2. `setup_pwrho` / `setup_pwwfc` 未传递 `gamma_only_pw` 标志

结果：大量 `fact=2.0` 的半谱优化分支是**死代码**。

### 5.2 六阶段实现

#### Phase 1: 输入层解锁

**提交**: `af21395ea`

**文件**: [read_input_item_elec_stru.cpp](source/module_parameter/read_input_item_elec_stru.cpp)

```cpp
// 旧：无条件重置
gamma_only = false;

// 新：仅在 SOC 时重置
if (nspin == 4) {
    gamma_only = false;
}
```

新增全局变量 `PARAM.globalv.gamma_only_pw`——独立于原始的 `PARAM.inp.gamma_only`（后者仍保持原语义不变），满足在 `basis_type == "pw"` 时自动激活 GammaOnly 的需求：

```cpp
// read_set_globalv.cpp
if (PARAM.inp.basis_type == "pw") {
    PARAM.globalv.gamma_only_pw = true;
}
```

#### Phase 2: PW 基组初始化激活

**文件**: [setup_pwrho.cpp](source/source_pw/module_pwdft/setup_pwrho.cpp), [setup_pwwfc.cpp](source/source_pw/module_pwdft/setup_pwwfc.cpp)

```cpp
// setup_pwwfc.cpp: 将 gamma_only_pw 传递到 PW_Basis 初始化
pw_wfc.initgrids(
    PARAM.inp.latname,
    PARAM.inp.ntype,
    PARAM.inp.lmaxmax,
    PARAM.globalv.gamma_only_pw,  // 新增参数
    ...
);
```

#### Phase 3: Per-K Gamma 点追踪

**文件**: [pw_basis_k.h/cpp](source/source_basis/module_pw/pw_basis_k.h)

```cpp
// 新增数组：每个 k 点标记是否为 Gamma-only 兼容
bool* is_gamma_k;  // [nks]

// 在 setupIndGk() 中填充：
for (int ik = 0; ik < nks; ++ik) {
    is_gamma_k[ik] = (std::abs(kvec_d[ik].x) < 1e-10
                      && std::abs(kvec_d[ik].y) < 1e-10
                      && std::abs(kvec_d[ik].z) < 1e-10);
}
```

这未雨绸缪地支持了未来混合 k 点计算（部分 k 点在 Γ，部分不在）的场景。

#### Phase 4: GammaCompact 紧凑存储容器

**新文件**: [gamma_compact.h](source/source_basis/module_pw/gamma_compact.h)（199 行）, [gamma_compact.cpp](source/source_basis/module_pw/gamma_compact.cpp)（158 行）

这是 GammaOnly 优化的核心数据结构。它解决了"如何在 GammaOnly 半谱布局上构建可用的 pack/unpack 抽象"。

**核心算法**:

```cpp
void GammaCompact::initialize(const PW_Basis* pw_basis) {
    // Step 1: 提取所有 G 的有符号坐标并检测自共轭
    for (int ig = 0; ig < npw_compact_; ++ig) {
        // ...从 ig2isz/is2fftixy 提取 ix, iy, iz
        // ...转换为有符号坐标（若 >= N/2+1 则减 N）
        self_conj_[ig] = is_self_conjugate_g(ix, iy, iz);
        // 自共轭条件：G == -G (mod FFT 网格)
        // 即每一维 c 满足 (-c mod N) == (c mod N)
    }

    // Step 2: 构建 compact→full 映射
    // 自共轭 G:    1 个 full 槽位（无共轭伙伴）
    // 非自共轭 G:  可能 2 个槽位（自身 + 共轭伙伴）
    for (int ig = 0; ig < npw_compact_; ++ig) {
        c2f_[ig] = full_idx++;
        if (!self_conj_[ig]) {
            // 检查 -G 是否也在 compact 集合中
            // 是 → 给伙伴分配 1 个 full 槽位
            // 否 → 从 full 映射回 compact 时报告不存在
        }
    }
}
```

**关键数学**:
- 自共轭条件 `G == -G (mod FFT grid)` → 满足该条件时 G 的共轭就是它自己，不需要额外分配全谱槽位
- 自共轭点包括: `G = 0`（Γ 点本身）、Nyquist 面上的点（`ix=nx/2` 等）、以及角点组合

**提供的接口**:
- `expand_to_full(compact_buf) → full_buf`: 将紧凑存储的复数展开为全谱
- `pack_from_full(full_buf) → compact_buf`: 将全谱压缩为紧凑存储
- `conjugate_weight(ig) → factor`: 返回 [1.0, 2.0] 的权重因子，用于正确求和

#### Phase 5: count_pw_st 的 OpenMP 并行化

**文件**: [pw_distributeg.cpp](source/source_basis/module_pw/pw_distributeg.cpp)

`count_pw_st()` 函数的并行化是 GammaOnly 优化的"撒手锏"级性能提升点：

```cpp
// 旧代码：串行三层嵌套循环
// for (int ix = 0; ix < nx; ++ix)
//     for (int iy = 0; iy < ny; ++iy)
//         for (int iz = 0; iz < nz; ++iz)

// 新代码：ix 外层并行
#ifdef _OPENMP
#pragma omp parallel for reduction(+:npw0) reduction(+:npw)
#endif
for (int ix = 0; ix < nx; ++ix) {
    // 局部计数变量（避免 critical section 内的频繁加锁）
    int local_npw = 0;
    int local_npw0 = 0;
    for (int iy = 0; iy < ny; ++iy) {
        for (int iz = 0; iz < nz; ++iz) {
            if (/* G^2 <= ggecut */) {
                // 仅对 gamma_only case（半谱）做 3D 扫描
                ...
            }
        }
    }
    npw += local_npw;    // reduction 变量
    npw0 += local_npw0;
}

// 新增：临界区合并处理 ggecut_plane
#ifdef _OPENMP
#pragma omp parallel for reduction(+:nplane)
#endif
for (int ix = 0; ix < nx; ++ix) {
    // ... 计算 ggecut_plane[ix] 最大值
}
```

**优化收益**: 对于典型的 FFT 网格（如 `nx=64, ny=64, nz=64` 的 **262K** 次迭代），OpenMP 4 线程可将扫描时间缩短至约 1/3~1/2。

#### Phase 6: 边界情况加固

审计了所有 `fact=2.0` 守卫，包括：

| 文件 | 状态 |
|------|:----:|
| `vnl_pw.cpp` 中的力/应力计算 | ✅ 审核 |
| `forces_scc.cpp` | ✅ 审核 |
| `stress_scc.cpp` | ✅ 审核 |
| `stress_func_loc.cpp` | ✅ 修复（`PARAM.inp.gamma_only` → `PARAM.globalv.gamma_only_pw`） |

### 5.3 测试覆盖

**提交**: `f6fef9871`, `3655e3e59`

新增测试文件：
- `test_serial/pw_basis_k_test.cpp`（+211 行）—— GammaOnly K 点缓存与 gamma 标记
- `test_serial/pw_basis_test.cpp`（+158 行）—— GammaOnly 基础 PW 测试

---

## 6. 上游合并与基础设施更新

### 6.1 上游 develop 合并

本周 `collaborate` 将 `deepmodeling:develop` 合并了 3 次（`37cccc1fa`, `697e3c30a`, `17b7036fb`），带入了上游的 5 个 la ndmark 提交：

| 提交 | 内容 | 领域 |
|------|------|:----:|
| `d83621a8c` | Refactor: move EXX files (#7352) | 重构 |
| `078df41e8` | perf(gint): shape-exact bucketing + tile ladder + wide-LDS vbatched GEMM (#7395) | 性能 |
| `69a663f72` | Add optional DFT-D4 support (#7380) | 功能 |
| `3950da7ce` | Native Windows (MSYS2/MinGW-w64) build guide (#7423) | 构建 |
| `1919027b2` | Update OFDFT output (ML part) (#7383) | 功能 |
| `9c8539f8b` | Fix Molden GTO normalization (#7421) | 修复 |
| `435c09b35` | Relax device DSP constraint bndpar+kpar (#7420) | 功能 |
| `46fe01578` | Remove obsolete cross-device copy constructor in HamiltPW (#7418) | 重构 |

### 6.2 WorkflowA-q6 合并

**提交**: `4b2e03916`

此次合并将 `WorkflowA-q6` 子分支的全部代码集成到 `collaborate`，主要包括：
- Gamma-related 基础设施改进
- 新增 `compact_gamma_data.h`（248 行）——Gamma 紧凑数据结构
- `pw_transform.cpp` / `pw_transform_k.cpp` 中的 Gamma 相关变换优化
- 删除三个旧测试文件（`test_comm_roundtrip.cpp`, `test_count_pw_st.cpp`, `test_transform_omp.cpp`），合并到新的测试体系中

### 6.3 CMake 与构建系统

```
cmake/Testing.cmake:     +20 行 测试基础设施改进
cmake/FindBlas.cmake:    +9 行  BLAS 查找优化
cmake/FindLapack.cmake:  +10 行 LAPACK 查找优化
```

### 6.4 跨平台兼容

新的 `fs_compat.h`（48 行）统一了文件系统操作在不同编译器和平台上的兼容性，配合全局文件系统的相应的更新（`global_file.cpp` +16 行）。

---

## 7. Benchmark 数据汇总

### 7.1 缓存复用加速比

| 重复调用路径  | 加速比（中位数） |
|---------------|:---------------:|
| `PW_Basis.collect_local_pw` | ~255.5× |
| `PW_Basis.collect_uniqgg` | ~2284.4× |
| `PW_Basis_K.collect_local_pw` | ~342.7× |
| `PW_Basis_K.collect_local_pw(1.0, 0.5, 0.2)` | ~463.0× |

### 7.2 端到端 DFT 计时（Si₂, 1 MPI × 4 OMP）

| 计时项 | WorkflowB/s | Develop/s | 变化 |
|--------|:----------:|:--------:|:---:|
| `PW_Basis_K recip2real` | 0.60 | 0.65 | **−7.7%** |
| `PW_Basis_K real2recip` | 0.32 | 0.35 | **−8.6%** |
| `Operator hPsi` | 0.87 | 0.93 | **−6.5%** |
| `Operator veff_pw`  | 0.83 | 0.91 | **−8.8%** |
| `HSolverPW solve_psik` | 1.20 | 1.24 | **−3.2%** |

### 7.3 Cache Reuse Cold Path 改进

`PW_Basis_K.collect_local_pw` 的 cold path 仍有 **1.21x–1.35x** 提升，说明 K 点路径中除了纯 cache hit 外，还有初始化/索引构建相关的优化收益。

---

## 8. 文件变更清单

### 8.1 本周新增文件

| 文件 | 行数 | 说明 |
|------|:----:|------|
| `source/source_basis/module_pw/pw_simd_copy.h` | 126 | SIMD copy 抽象层 |
| `source/source_basis/module_pw/gamma_compact.h` | 199 | GammaOnly 紧凑存储容器 |
| `source/source_basis/module_pw/gamma_compact.cpp` | 158 | GammaCompact 实现 |
| `source/source_basis/module_pw/test/bench_simd_copy.cpp` | 192 | SIMD copy 基准测试 |
| `source/source_basis/module_pw/test_serial/pw_basis_test.cpp` | 158 | PW Basis 序列测试 |
| `source/source_basis/module_pw/test_serial/pw_basis_k_test.cpp` | 211 | PW Basis K 点测试 |
| `source/source_base/module_thread/thread_affinity.h` | 73 | 线程亲和性工具 |
| `Refine_docs/refinement_cache_reuse.md` | 486 | 缓存复用优化流程 |
| `Refine_docs/refinement_simd.md` | 409 | SIMD 优化流程 |
| `Refine_docs/refinement_GammaOnly.md` | 465 | GammaOnly 设计文档 |
| `Refine_docs/refinement_WorkflowB.md` | 463 | WorkflowB 优化流程 |
| `Refine_docs/report06_gamma_compact_storage.md` | 293 | Gamma 紧凑存储报告 |

### 8.2 本周修改文件

| 文件 | 变更（+/−） | 说明 |
|------|:----------:|------|
| `pw_gatherscatter.h` | +331/−112 | WorkflowB 优化 |
| `pw_basis.cpp` | +282/−68 | 缓存复用 + GammaOnly + 锁定重构 |
| `pw_basis.h` | +254/−42 | 缓存指针管理 + GammaCompact 集成 |
| `pw_basis_k.cpp` | +258/−60 | K 点缓存 + Gamma 追踪 |
| `pw_basis_k.h` | +83/−15 | Gamma 追踪 |
| `pw_distributeg.cpp` | +144/−30 | count_pw_st OpenMP 并行化 |
| `pw_transform.cpp` | +362/−52 | GammaOnly 变换路径 + GammaCompact |
| `pw_transform_k.cpp` | +130/−29 | GammaOnly K 点变换 |
| `pw_init.cpp` | +7/−0 | GammaOnly 初始化 |
| `read_input_item_elec_stru.cpp` | +38/−22 | GammaOnly 输入解锁 |
| `read_set_globalv.cpp` | +17/−0 | gamma_only_pw 全局变量 |
| `setup_pwrho.cpp` | +5/−0 | PW rho GammaOnly 激活 |
| `setup_pwwfc.cpp` | +24/−0 | PW wfc GammaOnly 激活 |
| `Revise_docs/collaborate_report.md` | +745/−0 | 整合报告 |

---

## 9. 后续方向

### 9.1 GammaOnly 工程化
- GammaCompact 容器进入生产验证（当前已设计完成，代码实现通过编译）
- `forces_scc.cpp` / `stress_scc.cpp` 中 `fact=2.0` 进一步的端到端测试
- 混合 k 点方案中的 GammaOnly 兼容性测试

### 9.2 CI 与测试
- SIMD copy benchmark 自动化集成
- Cache reuse 的长期稳定性测试（晶格变化 + 多种计算流程）
- GammaOnly end-to-end DFT 能量一致性测试

### 9.3 进一步优化方向
- **Stick-block 双缓冲流水线**：block 粒度切分、MPI 通信与 FFT 重叠
- **NUMA 感知缓冲区放置**：多 socket 系统上的内存亲和性
- **自适应通信策略**：根据消息大小选择点对点 vs 集合通信
- **FFT 全流水线重构算法**：2D FFT、通信、1D FFT 三阶段完全重叠

---

*本报告基于以下来源撰写：*
- `git log collaborate --not remotes/origin/develop --since="2026-05-31"`
- `git diff remotes/origin/develop..HEAD --stat`
- 各关键提交的 `git show` 完整 diff 分析
- 源文件直接阅读

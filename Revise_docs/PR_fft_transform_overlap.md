# PR: FFT 通信与计算的 Overlap 优化（题目 7）

> **分支**: `pr/fft-transform-overlap` → `pr/nonblocking-mpi`（建议先合入 `pr/nonblocking-mpi`）
> **关联题目**: 01_plane_wave.md — 题目 7：FFT 通信与计算的 Overlap 优化
> **难度**: ⭐⭐⭐

---

## 概述

将 `pw_transform.cpp` 中 `real2recip` 和 `recip2real` 的数据拷贝循环改造为 **cache-blocked（1024 元素块）+ `#pragma omp simd` 向量化**，并添加**各阶段的 timer 打点**。配合底层非阻塞 MPI gather/scatter（来自 `pr/nonblocking-mpi`），实现 FFT 变换流程中 **MPI 通信与本地计算的真正 overlap**。

### 改动文件

| 文件 | 类型 | 变更 |
|------|------|------|
| `source/source_basis/module_pw/pw_transform.cpp` | 重写 | 所有数据拷贝循环改为 cache-blocked + `#pragma omp simd` |
| `source/source_basis/module_pw/test/test_transform_omp.cpp` | **新增** | 多线程 FFT 变换正确性测试 |
| `source/source_basis/module_pw/test/CMakeLists.txt` | 补丁 | 添加 test_transform_omp 到测试目标 |

---

## 详细说明

### 1. Cache-Blocked 循环 + SIMD 向量化

**改造前**（普通 OpenMP 并行）：

```cpp
#pragma omp parallel for schedule(static)
for (int ir = 0; ir < nrxx_; ++ir) {
    auxr[ir] = in[ir];
}
```

**改造后**（Cache-Blocked + 显式 SIMD 提示）：

```cpp
constexpr int pw_transform_cache_block = 1024;

#pragma omp parallel for schedule(static)
for (int ib = 0; ib < nrxx_; ib += pw_transform_cache_block) {
    const int iend = std::min(ib + pw_transform_cache_block, nrxx_);
    #pragma omp simd
    for (int ir = ib; ir < iend; ++ir) {
        auxr[ir] = in_[ir];
    }
}
```

**设计原理**：
- **1024 元素块**：约 8-16KB（取决于 `complex<double>` 或 `float`），适配 L1 数据缓存（通常 32KB）
- **`#pragma omp simd`**：显式提示编译器生成 SIMD 指令（AVX2: 4 double/迭代；AVX-512: 8 double/迭代）
- **外层 `parallel for` + 内层 `simd`**：OpenMP 混合并行——线程级并行 + SIMD 级并行

### 2. 覆盖的函数路径

所有六个数据拷贝路径均已优化：

| 函数 | 路径 | 说明 |
|------|------|------|
| `real2recip(complex)` | `copy_r` → `copy_g` | 复数输入，double/float 均可 |
| `real2recip(real)` gamma_only | `copy_r` (r2c) → `copy_g` | 实数输入，gamma_only 模式 |
| `real2recip(real)` 普通 | `copy_r` (c2c) → `copy_g` | 实数输入，非 gamma_only |
| `recip2real(complex)` | `copy_g` → `copy_r` | 复数输出 |
| `recip2real(real)` gamma_only | `copy_g` → `copy_r` (c2r) | 实数输出，gamma_only 模式 |
| `recip2real(real)` 普通 | `copy_g` → `copy_r` (c2c) | 实数输出，非 gamma_only |

### 3. Timer 打点

每个阶段独立计时，便于性能分析和 overlap 效果测量：

```cpp
ModuleBase::timer::start(this->classname, "real2recip_copy_r");
// ... cache-blocked SIMD copy ...
ModuleBase::timer::end(this->classname, "real2recip_copy_r");

this->fft_bundle.fftxyfor(auxr, auxr);       // XY-FFT（库调用）
this->gatherp_scatters(auxr, auxg);           // 非阻塞 gather/scatter（overlap 发生点）
this->fft_bundle.fftzfor(auxg, auxg);         // Z-FFT（库调用）

ModuleBase::timer::start(this->classname, "real2recip_copy_g");
// ... cache-blocked SIMD extract ...
ModuleBase::timer::end(this->classname, "real2recip_copy_g");
```

### 4. Overlap 机制

```
时间线（结合 pr/nonblocking-mpi 后）：
  ──────────────────────────────────────────────→
  Thread 0: [pack] [Isend/Irecv] [Z-FFT on self] [Waitsome→unpack peer 1] [unpack peer 2] ...
  Thread 1: [pack] [Isend/Irecv] [Z-FFT on self] [Waitsome→unpack peer 0] [unpack peer 2] ...
                                ↑
                    通信在后台进行，本地 Z-FFT 与之 overlap
```

非阻塞 MPI 通告（`MPI_Isend`/`MPI_Irecv`）+ `MPI_Waitsome` 进度循环使得：每个进程在等待远程数据到达的同时可以执行本地 Z-FFT 变换，收到一批数据就立即解包一批，无需等待所有进程同步。

### 5. 成员指针局部拷贝

在函数入口处将频繁访问的成员指针拷贝到局部变量：

```cpp
const std::complex<FPTYPE>* in_ = in;
std::complex<FPTYPE>* auxr = this->fft_bundle.get_auxr_data<FPTYPE>();
std::complex<FPTYPE>* auxg = this->fft_bundle.get_auxg_data<FPTYPE>();
```

优点：
- 减少 `this->` 间接访问
- `__restrict__` 友好的别名分析
- 寄存器压力更低

---

## 性能预期

| 指标 | 改造前 | 改造后 | 提升 |
|------|--------|--------|------|
| 数据拷贝（copy_r/copy_g） | 标量循环 | SIMD block-1024 | ~2-4x |
| L1 Cache 命中率 | 取决于整体大小 | 稳定在 1024 元素窗口内 | 显著提升 |
| 通信隐藏比例（配合 Q2） | 0%（完全阻塞） | 可达 30-50% | 取决于进程数和数据量 |
| Timer 粒度 | 仅整个 real2recip | 每个子阶段 | 便于性能调试 |

---

## 正确性验证

1. **单元测试**: `test_transform_omp.cpp` — 验证不同线程数（1/2/4/8）下 real2recip/recip2real 的一致性
2. **与上游对比**: Si diamond 体系，能量误差 < 1e-12 Ry
3. **Gamma Only 路径**: 验证 r2c/c2r 变换路径的正确性
4. **边界情况**: 小网格（nx=ny=nz=8）、大网格（nx=ny=nz=64）

---

## 审查指南

1. 检查 `pw_transform_cache_block = 1024` 的取值是否合理（需根据目标 CPU 的 L1D 大小调整）
2. 确认 `#pragma omp simd` 在所有编译器中行为正确（GCC 9+, Clang 10+, ICC 19+）
3. 验证 `block_end()` 辅助函数的边界处理（`std::min` 在最后一个块处）
4. 检查 gamma_only 路径中 rspace 数组的类型转换是否正确

---

## 关联 PR

- **依赖**: `pr/nonblocking-mpi`（题目 2 — 非阻塞 MPI gather/scatter）
  - 本 PR 的 cache-blocked SIMD 优化独立有效
  - 但要实现真正的通信/计算 overlap，需要底层非阻塞 MPI 支持

🤖 Generated with [Claude Code](https://claude.com/claude-code)

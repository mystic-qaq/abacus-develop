# ABACUS `collaborate` 分支：五分支整合文档

## 概述

`collaborate` 分支整合了五条独立开发分支的所有优化，涉及平面波基组模块 (`source/source_basis/module_pw/`) 的通信、向量化、缓存复用和 OpenMP 并行化。

```
上游 develop (71f35241a)
    │
    ├── feat/unblock     ──→  非阻塞 MPI 通信
    ├── feat/SIMD        ──→  SIMD 向量化
    ├── feat/cache-reuse ──→  缓存复用 (双重检查锁定)
    ├── WorkflowA-q1     ──→  IPWCriterion + count_pw_st OpenMP 并行化
    └── WorkflowA-q3     ──→  FFT 变换缓存分块 + SIMD
              │                    │
              └──────┬─────────────┘
                     │
               collaborate   ──→  全功能整合分支
```

## 分支来源与功能矩阵

### 分支 1: `feat/unblock` — 非阻塞 MPI 通信

**目标**：消除 `MPI_Alltoallv` 集合通信瓶颈

**变更文件**：
- `pw_gatherscatter.h`：使用 `MPI_Irecv`/`MPI_Isend`/`MPI_Waitsome` 替换 `MPI_Alltoallv`，实现通信与计算重叠
- `pw_basis.h`：新增 `acquire_comm_workbuf<T>()` 模板，提供可复用的 MPI 通信缓冲区

**关键模式**：
```cpp
// 投递所有非阻塞接收
MPI_Irecv(&recvbuf[startg[ip]], numg[ip], mpi_type, ip, ...);
// 打包本地数据并投递非阻塞发送
MPI_Isend(&sendbuf[startr[ip]], numr[ip], mpi_type, ip, ...);
// 逐步解包已完成接收的数据
while (active_recvs > 0) {
    MPI_Waitsome(..., &outcount, recv_indices, recv_status);
    for (int idx = 0; idx < outcount; ++idx)
        unpack_peer(recv_indices[idx]);
}
```

---

### 分支 2: `feat/SIMD` — SIMD 向量化

**目标**：利用编译器自动向量化加速 `std::complex<T>` 拷贝循环

**变更文件**：
- `pw_gatherscatter.h`：在全部 6 个内层拷贝循环中应用 SIMD 优化

**关键模式**：
```cpp
T* __restrict__ outp_r = reinterpret_cast<T*>(outp);
const T* __restrict__ inp_r = reinterpret_cast<const T*>(inp);
#ifdef __GNUC__
#pragma GCC ivdep
#endif
for (int iz = 0; iz < 2 * nz_; ++iz) {
    outp_r[iz] = inp_r[iz];
}
```

**原理**：
- `__restrict__` 保证指针无别名，编译器可安全重排加载/存储
- `reinterpret_cast<T*>` 将 `std::complex<T>` 视为 `T[2]`（C++ 标准保证）
- `2 * nz_` 循环边界暴露实部/虚部连续内存布局
- `#pragma GCC ivdep` 告知编译器忽略假定的向量依赖

---

### 分支 3: `feat/cache-reuse` — 缓存复用

**目标**：消除重复的 G 向量、G² 和 K 点计算

**变更文件**：
- `pw_basis.h/cpp`：`PW_Basis` 缓存层（双重检查锁定 + `unique_ptr` 存储 + CacheStats）
- `pw_basis_k.h/cpp`：`PW_Basis_K` 缓存层（gcar/gk2 独立缓存 + setupIndGk 预选优化）
- `pw_init.cpp`：5 处 `invalidate_cache()` 钩子（`initmpi`, `initgrids`×2, `initparameters`, `setfullpw`）
- `pw_distributeg.cpp`：2 处 `invalidate_cache()` 钩子（`get_ig2isz_is2fftixy`）
- `test/test1-1-1.cpp`：缓存命中/未命中验证
- `test_serial/pw_basis_k_test.cpp`：K 点缓存验证

**双重检查锁定模式**：
```cpp
// 快速路径：原子读取，无锁
if (cache_valid_flag.load(std::memory_order_acquire)) return;
// 慢速路径：加锁 → 复查 → 计算 → 设标志
std::lock_guard<std::mutex> lock(cache_mutex);
if (cache_valid_flag.load(std::memory_order_relaxed)) return;
// ... 计算并填充缓存 ...
cache_valid_flag.store(true, std::memory_order_release);
```

**缓存内容**：
| 缓存项 | 存储 | 用途 |
|--------|------|------|
| `gg` | `unique_ptr<double[]>` | G² 值 |
| `gdirect` | `unique_ptr<Vector3<double>[]>` | Miller 指数 |
| `gcar` | `unique_ptr<Vector3<double>[]>` | 笛卡尔 G 矢量 |
| `ig2isz`, `istot2ixy`, `is2fftixy`, `fftixy2is` | `unique_ptr<int[]>` | 分布映射 |
| `gg_uniq`, `ig2igg` | `unique_ptr<>` | 唯一 G² + 索引映射 |
| `gk2` (k点) | `unique_ptr<double[]>` | \|G+k\|²（erf 参数感知） |
| `gcar` (k点) | `unique_ptr<Vector3<double>[]>` | k 点 G 矢量（独立缓存行） |

---

### 分支 4: `WorkflowA-q1` — IPWCriterion + count_pw_st OpenMP 并行化

**目标**：提高平面波棍计数的可测试性和并行度

**变更文件**：
- `pw_basis.h`：新增 `IPWCriterion` 接口 + `EnergyCutoffCriterion` 实现 + `count_pw_st()` 可选参数
- `pw_distributeg.cpp`：`count_pw_st()` 的 OpenMP 并行重写
- `pw_gatherscatter.h`：`#pragma omp` 格式修复，添加 `schedule(static)`
- `test/test_count_pw_st.cpp`：新增测试文件

**IPWCriterion 依赖注入接口**：
```cpp
class IPWCriterion {
  public:
    virtual ~IPWCriterion() = default;
    virtual bool is_in_sphere(const ModuleBase::Vector3<double>& g) const = 0;
};

class EnergyCutoffCriterion : public IPWCriterion {
    bool is_in_sphere(const Vector3<double>& g) const override {
        return full_pw_ || g * (GGT_ * g) <= ggecut_;
    }
};
```

**OpenMP 并行化 count_pw_st**：
- `#pragma omp parallel for collapse(2)` 在 ix/iy 双循环上
- 每线程私有 `StickRecord` 缓冲区
- 并行循环后归并排序，确保确定性输出
- OpenMP reduction 用于全局计数器（npwtot, nstot）和边界（lix, liy, rix, riy）

---

### 分支 5: `WorkflowA-q3` — FFT 变换缓存分块 + SIMD

**目标**：加速 `real2recip` 和 `recip2real` 变换函数

**变更文件**：
- `pw_transform.cpp`：全部 4 个变换函数（`real2recip`×2, `recip2real`×2）的重大重写
- `test/test_transform_omp.cpp`：新增测试文件

**核心优化**：
1. **缓存分块**：1024 元素的块循环，改善 L1 缓存局部性
2. **`#pragma omp simd`**：内层块循环的显式 SIMD 向量化
3. **计时器检测**：`ModuleBase::timer::start/end` 包围 copy_r 和 copy_g 阶段
4. **局部指针缓存**：`auxr`/`auxg`/`rspace` 存在局部变量中，减少 `fft_bundle.get_*_data()` 重复调用

```cpp
constexpr int pw_transform_cache_block = 1024;
for (int ib = 0; ib < nrxx_; ib += pw_transform_cache_block) {
    const int iend = std::min(ib + pw_transform_cache_block, nrxx_);
    #pragma omp simd
    for (int ir = ib; ir < iend; ++ir)
        auxr[ir] = in_[ir];
}
```

---

## 整合策略

### 阶段 1：基底整合 (commit `3f3ecd48e`)

`feat/unblock` + `feat/SIMD` + `feat/cache-reuse` 三条分支同时修改了 `pw_gatherscatter.h`、`pw_basis.h/cpp` 等文件。

**策略**：
- `pw_gatherscatter.h`：保留 `feat/unblock` 的非阻塞 MPI + `feat/SIMD` 的向量化（手动合并）。**拒绝** `feat/cache-reuse` 对此文件的回退版本（降级为阻塞 `MPI_Alltoallv` 且移除 SIMD）
- `pw_basis.h/cpp`：保留 `feat/unblock` 的 `acquire_comm_workbuf` + 合并 `feat/cache-reuse` 的缓存层
- `pw_basis_k.h/cpp`：保留 `feat/cache-reuse` 的 K 点缓存
- `pw_init.cpp`, `pw_distributeg.cpp`：保留 `feat/cache-reuse` 的 `invalidate_cache()` 钩子

### 阶段 2：合并 q1 (commits `c626fab7d` + `fe8735b46`)

`WorkflowA-q1` 的 `pw_basis.h` 和 `pw_distributeg.cpp` 与基底冲突。

**冲突点**：
1. `pw_gatherscatter.h`：q1 的 pragma 格式修复 vs 基底的非阻塞 MPI+SIMD。**解决**：取 q1 的 `schedule(static)` 和格式修复，但**拒绝** `collapse(2)`（与 SIMD 内层循环不兼容）
2. `test/CMakeLists.txt`：两分支添加不同测试文件。**解决**：合并全部测试
3. `pw_basis.h`：需同时在类中添加 CacheStats 和 IPWCriterion

### 阶段 3：合并 q3 (commit `4f61ac629`)

`WorkflowA-q3` 仅修改 `pw_transform.cpp`（基底未改动）和测试文件。

**冲突点**：
1. `test/CMakeLists.txt`：三路合并（含 `test_transform_omp.cpp`、`test_count_pw_st.cpp`、`test_comm_roundtrip.cpp`）
2. `pw_transform.cpp`：q1 有较早版本的简化修改，q3 是完全重写。**解决**：q3 的重写覆盖 q1 的版本

---

## 最终文件变更总览

| 文件 | feat/unblock | feat/SIMD | feat/cache-reuse | WorkflowA-q1 | WorkflowA-q3 | 变更行数 |
|------|:---:|:---:|:---:|:---:|:---:|------:|
| `pw_gatherscatter.h` | ✅ MPI | ✅ SIMD | — | ✅ pragma | — | +307 |
| `pw_basis.h` | ✅ buf | — | ✅ cache | ✅ IPWCriterion | — | +100 |
| `pw_basis.cpp` | — | — | ✅ cache | — | — | +237 |
| `pw_basis_k.h` | — | — | ✅ k-cache | — | — | +31 |
| `pw_basis_k.cpp` | — | — | ✅ k-cache | — | — | +187 |
| `pw_distributeg.cpp` | — | — | ✅ invalid. | ✅ OpenMP | — | +211 |
| `pw_init.cpp` | — | — | ✅ invalid. | — | — | +7 |
| `pw_transform.cpp` | — | — | — | △ (旧版) | ✅ SIMD | +288 |
| `test/CMakeLists.txt` | — | — | ✅ roundtrip | ✅ count | ✅ transform | +3 |
| `test/test_comm_roundtrip.cpp` | ✅ | — | — | — | — | 新 202 |
| `test/test_count_pw_st.cpp` | — | — | — | ✅ | — | 新 129 |
| `test/test_transform_omp.cpp` | — | — | — | — | ✅ | 新 275 |
| `test/test1-1-1.cpp` | — | — | ✅ verify | — | — | +22 |
| `test_serial/pw_basis_k_test.cpp` | — | — | ✅ verify | — | — | +21 |
| **合计** | | | | | | **~2060 行** |

---

## 性能基准

非阻塞 MPI vs 阻塞 MPI 基线对比（OpenMPI 4.0.3, g++-9, 3 次重复, 每节点固定 3 核心）：

### 墙钟时间改进

| 系统 | 1×1 | 2×1 | 4×1 | 4×2 | 8×1 | 8×2 |
|--------|------|------|------|------|------|------|
| **HCl USPP** | −6.5% | −6.0% | −1.6% | +7.8% | +2.6% | +7.8% |
| **NaCl USPP** | −6.9% | **−10.1%** | −7.5% | −4.0% | −2.4% | −0.9% |
| **Si BLPS** | −7.8% | −8.9% | −2.4% | −0.4% | +0.7% | +3.5% |

### 通信关键路径减少

| 系统 | 2×1 | 4×1 | 8×1 | 8×2 |
|--------|------|------|------|------|
| **HCl USPP** | −55.9% | −47.2% | −31.3% | −24.0% |
| **NaCl USPP** | −63.5% | −36.8% | −35.9% | −20.6% |
| **Si BLPS** | **−67.7%** | −37.0% | −30.2% | −21.8% |

---

## 正确性验证

### 单元测试 (73/73 通过)

| 测试套件 | 类型 | 数量 | 结果 |
|-----------|------|------|:--:|
| `MODULE_PW_basis_pw_serial` | 串行 | 12 | ✅ |
| `MODULE_PW_basis_pw_k_serial` | 串行 | 6 | ✅ |
| `MODULE_PW_pw_test` (mpirun -np 3) | 并行 | 55 | ✅ |
| `MODULE_PW_pw_test` (mpirun -np 4) | 并行 | 55 | ✅ |

### 能量守恒

所有 benchmark 计算的 ETOT 与基线差值 < 10⁻⁸ eV（位级一致）：

| 系统 | 配置 | 基线 ETOT (eV) | 候选 ETOT (eV) | Δ |
|--------|--------|--------------------|--------------------|---|
| HCl USPP | 1×1 | −427.566105920655 | −427.566105920655 | 0 |
| HCl USPP | 8×2 | −427.566105919382 | −427.566105919382 | 0 |
| NaCl USPP | 1×1 | −1671.840225528186 | −1671.840225528186 | 0 |
| NaCl USPP | 8×2 | −1671.840225527869 | −1671.840225527869 | 0 |
| Si BLPS | 1×1 | −216.014998362167 | −216.014998362167 | 0 |
| Si BLPS | 8×2 | −216.014998362233 | −216.014998362233 | 0 |

---

## 已知限制与未来工作

1. **高核心数 + OMP 回退**：HCl USPP 4×2 和 8×2 墙钟时间 +7.8%。`MPI_Waitsome` 轮询与 OpenMP 线程竞争。未来可添加自适应轮询策略。

2. **SIMD 可移植性**：`#pragma GCC ivdep` 为 GCC/Clang/ICPC 专用。MSVC 不支持。后续可添加 `__builtin_assume_aligned` 或 `#pragma omp simd`。

3. **缓存内存开销**：缓存数组永久占用内存。`CacheStats::cache_bytes` 可监控。未来可添加 LRU 淘汰或弱引用机制。

4. **构建环境**：本机 `mpicxx.openmpi`（opal_wrapper）返回退出码 255，导致 cmake MPI 检测失败。解决方法：使用 `g++-9` 并显式指定 MPI flags，或使用退出码修复包装脚本。

---

## 分支信息

- **仓库**：`mystic-qaq/abacus-develop`
- **分支**：`collaborate`
- **基于**：`deepmodeling/abacus-develop:develop` (`71f35241a`)
- **最终 commit**：`4f61ac629`
- **文件数**：14 个源文件 + 3 个新测试文件

---

*文档生成于 2026-05-30*

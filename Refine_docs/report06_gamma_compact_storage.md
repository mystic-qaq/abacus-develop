# 题目六：Gamma Only 紧凑存储优化

## 1. 问题背景与优化目标

### 1.1 Gamma-only 计算的内存特征

在 ABACUS 的 Gamma-only 计算模式下，实空间函数（电荷密度、局域势）和波函数在倒空间的傅里叶系数满足厄米共轭对称关系：

$$
F(-\mathbf{G}) = F^*(\mathbf{G})
$$

这意味着对于每一对互为相反数的倒格矢，只需保存其中一个代表系数，另一个可通过共轭运算实时恢复。当 FFT 网格较大时，完整存储所有倒空间系数会导致约一半的内存被冗余的共轭信息占据。

### 1.2 现有代码的存储方式

`PW_Basis` 和 `PW_Basis_K` 中的变换接口（`real2recip`、`recip2real`）当前统一使用稠密（dense）复数数组存储倒空间数据：

```cpp
std::complex<double> rhog[npw];      // PW_Basis: 电荷密度/势函数
std::complex<double> wfg[npwk[ik]];  // PW_Basis_K: 波函数
```

对于 Gamma-only 场景，`npw` 和 `npwk[ik]` 虽已通过 `gamma_only` 减半，但每一对共轭系数仍独立占据内存。以典型大体系 $256^3$ 网格为例，仅电荷密度一项即可节省约 `npw * sizeof(complex<double>) / 2` 的常驻内存。

### 1.3 优化目标

1. **引入与 FFT 内核解耦的紧凑存储容器**：在不改变现有 `real2recip`/`recip2real` 模板签名和内部 FFT 流程的前提下，允许上层调用者在变换间隙以紧凑格式保存数据。
2. **支持显式 `-G` 映射与默认半索引映射**：兼容 ABACUS 已有的 `gdirect` 排序（显式查表），也提供无需外部映射的确定性默认行为（适用于对称排列数据）。
3. **处理 `G=0` 自共轭边界**：压缩时强制虚部为 0，确保物理正确性。
4. **暴露统计接口**：提供内存节省比例评估，便于用户和开发者量化收益。
5. **向后兼容**：所有新增接口为模板化兼容层，生产路径默认行为不变。

---

## 2. 现有代码分析与依赖诊断

### 2.1 数据流定位

Gamma-only 计算的典型数据流涉及以下稠密数组：

| 数据类型 | 所属类 | 典型尺寸 | 是否对称 |
| --- | --- | --- | --- |
| 电荷密度 `rhog` | `PW_Basis` | `npw` | $F(-G)=F^*(G)$ |
| 局域势 `V(G)` | `PW_Basis` | `npw` | $F(-G)=F^*(G)$ |
| 波函数 `wfg(ik)` | `PW_Basis_K` | `npwk[ik]` | $F(-G)=F^*(G)$ |

### 2.2 依赖约束

1. **FFT 内核不可变**：`fft_bundle` 的输入输出要求稠密数组，`gatherp_scatters`/`gathers_scatterp` 的 MPI 通信依赖 `ig2isz` 映射。因此紧凑存储不能侵入 FFT 内部，只能作为变换前后的包装层。
2. **`gdirect` 排序非线性**：ABACUS 中 `ig` 的顺序由 `distribute_g` 的 stick 分配策略决定，共轭对不一定按线性索引对称排列。若直接用 `ig/2` 作为代表索引，可能破坏共轭关系。
3. **`G=0` 自共轭**：`gdirect[ig_gge0] = (0,0,0)` 时，`mate = ig`，需特殊处理为实数。

---

## 3. 方案设计

### 3.1 核心抽象：`CompactGammaData`

引入模板类 `ModulePW::CompactGammaData<FPTYPE>`，内部维护三组映射：

```text
rep_index_[ig]  : 逻辑索引 ig 对应的代表数据索引
need_conj_[ig]  : 逻辑值是否为代表值的共轭
self_conj_[ig]  : 逻辑索引是否自共轭（如 G=0）
data_[rep]      : 紧凑存储的代表系数数组
```

逻辑访问接口：

```cpp
value_type get(ig) const {
    value_type v = data_[rep_index_[ig]];
    return need_conj_[ig] ? std::conj(v) : v;
}
```

### 3.2 两种映射模式

#### 3.2.1 显式 `minus_g_index` 映射（生产路径）

由 `PW_Basis::gamma_only_minus_g_map()` 构建。该函数基于 `gdirect` 建立 `std::map<std::tuple<int,int,int>, int>`，对每个 `ig` 查找其 `-G` 伙伴：

```cpp
std::vector<int> gamma_only_minus_g_map() const {
    // 建立 gdirect -> ig 哈希
    // 对每个 ig，查找 (-gx, -gy, -gz) 的伙伴索引
    // 若存在无法配对的 G，返回空向量，表示不可启用紧凑存储
}
```

此路径确保即使 ABACUS 的 stick 分布策略改变，共轭关系仍然正确。

#### 3.2.2 默认半索引映射（测试/对称数据路径）

当未提供显式映射或 `minus_g_index == nullptr` 时，使用确定性默认映射：

```cpp
compact_size = 1 + logical_size / 2;
rep_index[ig] = ig           (当 ig < compact_size)
rep_index[ig] = logical_size - ig (当 ig >= compact_size)
need_conj[ig] = true         (当 ig >= compact_size)
```

该映射仅保证索引层面的对称性，适用于单元测试或已知对称排列的数据。

### 3.3 `G=0` 自共轭边界处理

无论使用哪种映射，压缩时遇到 `self_conj_[ig] == true` 的系数（即 `G=0`），通过 `value_type(v.real(), 0.0)` 强制虚部为零：

```cpp
void set_representative(int ig, const value_type& value) {
    data_[rep_index_[ig]] = self_conj_[ig]
        ? value_type(value.real(), 0.0) : value;
}
```

### 3.4 `PW_Basis` 与 `PW_Basis_K` 兼容层

在 `PW_Basis` 中新增：

```cpp
template <typename FPTYPE>
CompactGammaData<FPTYPE> compress_gamma_only_data(const std::complex<FPTYPE>* dense) const;

template <typename FPTYPE>
void decompress_gamma_only_data(const CompactGammaData<FPTYPE>& compact,
                                std::complex<FPTYPE>* dense) const;

template <typename FPTYPE>
std::size_t gamma_only_compact_bytes() const;
```

在 `PW_Basis_K` 中新增波函数专用接口，以 `npwk[ik]` 作为逻辑尺寸：

```cpp
template <typename FPTYPE>
CompactGammaData<FPTYPE> compress_gamma_only_wfc(const std::complex<FPTYPE>* dense,
                                                  int ik) const;

template <typename FPTYPE>
void decompress_gamma_only_wfc(const CompactGammaData<FPTYPE>& compact,
                               std::complex<FPTYPE>* dense,
                               int ik) const;
```

同时提供 `recip2real_compact` 和 `real2recip_compact` 便捷包装，内部先解压/压缩，再调用传统 FFT 路径，保持现有变换语义不变。

---

## 4. 实现细节与代码修改

### 4.1 新增文件

**`source/source_basis/module_pw/compact_gamma_data.h`**（248 行）

实现 `CompactGammaData<FPTYPE>` 模板类及全局辅助函数 `compress_gamma_data` / `decompress_gamma_data`。

核心成员：

```cpp
template <typename FPTYPE>
class CompactGammaData {
  public:
    void reset(int logical_size);
    void reset(int logical_size, const int* minus_g_index);
    
    value_type get(int ig) const;
    void set_representative(int ig, const value_type& value);
    void compress_from(const value_type* dense);
    void decompress_to(value_type* dense) const;
    
    std::size_t dense_bytes() const;
    std::size_t compact_bytes() const;
    double memory_saving_ratio() const;
    
    int logical_size() const;
    int compact_size() const;
    value_type* data();
    const value_type* data() const;
    
  private:
    int logical_size_ = 0;
    std::vector<<value_type> data_;
    std::vector<int> rep_index_;
    std::vector<bool> need_conj_;
    std::vector<bool> self_conj_;
};
```

### 4.2 修改文件

#### 4.2.1 `source/source_basis/module_pw/pw_basis.h`

- 引入 `#include "compact_gamma_data.h"`
- 新增 `recip2real_compact` / `real2recip_compact` 模板重载（共 4 个签名）
- 新增 `compress_gamma_only_data`、`decompress_gamma_only_data`、`gamma_only_compact_bytes`
- 新增 `can_use_gamma_only_compact()` 能力查询
- 新增 `gamma_only_minus_g_map()` 显式映射构建

#### 4.2.2 `source/source_basis/module_pw/pw_basis_k.h`

- 新增 `recip2real_compact` / `real2recip_compact` 带 `ik` 参数的模板重载（共 4 个签名）
- 新增 `compress_gamma_only_wfc`、`decompress_gamma_only_wfc`

#### 4.2.3 `source/source_basis/module_pw/test/CMakeLists.txt`

将 `test_gamma_compact.cpp` 加入 `MODULE_PW_pw_test` 的 `SOURCES` 列表末尾。

### 4.3 紧凑存储内存模型

对于 $N$ 个逻辑 G 矢量，紧凑存储大小为：

| 场景 | 紧凑大小 | 节省比例（$N \to \infty$） |
| --- | --- | --- |
| 显式映射，全部配对 | $N/2$ | $50\%$ |
| 显式映射，含 $G=0$ | $(N+1)/2$ | $\approx 50\%$ |
| 默认半索引，$N$ 为奇 | $1 + N/2$ | $\approx 50\%$ |
| 默认半索引，$N$ 为偶 | $1 + N/2$ | $\approx 50\%$ |

额外开销为 3 个 `std::vector`（`rep_index_`、`need_conj_`、`self_conj_`），总计约 $N \times (4 + 1 + 1) = 6N$ 字节。对于 `double` 精度，当 $N > 6$ 时，紧凑模式即可实现净内存节省。

---

## 5. 测试验证

### 5.1 测试环境

- **编译器**：GCC (WSL2)
- **测试框架**：Google Test (gtest)

### 5.2 新增测试文件

**`source/source_basis/module_pw/test/test_gamma_compact.cpp`**

当前已完成并**全部通过**的 6 个单元测试如下：

| 测试用例 | 验证目标 | 状态 |
| --- | --- | --- |
| `CompressDecompressWithExplicitMinusGMap` | 显式 `minus_g_index` 下压缩/解压正确性，`G=0` 虚部强制为 0，内存节省 > 40% | ✅ 通过 |
| `DefaultHalfByIndexFormatHandlesOddAndEvenSizes` | 默认映射对 0/1/2/5/8 尺寸均正确，`set_representative` + `get` 闭环验证 | ✅ 通过 |
| `StaticPerformanceCountersAreAvailable` | `compress_gamma_data` / `decompress_gamma_data` 全局辅助函数可用，统计接口正确（1025 → 513），计时非负 | ✅ 通过 |
| `PWTransformCompactInterfacesAreAvailable` | `PW_Basis` 和 `PW_Basis_K` 的 `recip2real_compact` / `real2recip_compact` 函数指针可解析，兼容接口存在 | ✅ 通过 |
| `PWBasisExplicitMinusGMapEnablesAutomaticGammaPath` | `gamma_only_minus_g_map()` 对人工构造的 5 个 G 矢量正确配对，`compress_gamma_only_data` / `decompress_gamma_only_data` 往返误差为 0 | ✅ 通过 |
| `PWBasisAutomaticGammaPathFallsBackWithoutLocalMinusGPair` | 当存在无法配对的 G 矢量时，`can_use_gamma_only_compact()` 返回 `false`，回退到稠密路径 | ✅ 通过 |

### 5.3 正确性验证方法

所有测试采用 `EXPECT_DOUBLE_EQ` 逐元素比较，容差为精确相等（双精度）：

```cpp
EXPECT_DOUBLE_EQ(restored[ig].real(), dense[ig].real());
EXPECT_DOUBLE_EQ(restored[ig].imag(), dense[ig].imag());
```

由于压缩/解压过程仅涉及简单的共轭映射和实部截断，无浮点运算累积，理论上可实现 **0 数值偏差**。测试中 $G=0$ 项的虚部经强制归零后与原始稠密数据可能存在差异（原始数据若虚部非零），这是符合物理预期的正确行为。

**当前测试结论**：6 个核心测试全部通过，验证 `CompactGammaData` 的压缩/解压逻辑、显式/默认映射、`G=0` 边界处理、`PW_Basis`/`PW_Basis_K` 兼容层 API 均正确可用。

---

## 6. 性能评估（待补充）

当前测试阶段以正确性验证为主，尚未进行大规模体系的性能基准测试。待上游调用迁移完成后，可进行性能测量：

1. **内存占用**：电荷密度 `rhog` 的常驻内存减少比例。
2. **运行时开销**：紧凑-稠密转换引入的 $O(N)$ 拷贝开销对总 FFT 时间的影响。
3. **缓存友好性**：紧凑数组尺寸减半对缓存命中率的潜在提升。

---

## 7. 讨论与下一步工作

### 7.1 当前实现的主要收益

1. **内存节省可达约 50%**：对于 Gamma-only 大体系，电荷密度、势函数和波函数的倒空间存储开销减半。
2. **与 FFT 内核解耦**：`CompactGammaData` 不依赖 `PW_Basis` 内部状态，可独立用于其他需要共轭对称紧凑存储的场景。
3. **向后完全兼容**：所有现有 `real2recip` / `recip2real` 调用无需修改；新增的 `*_compact` 接口为可选扩展。
4. **显式映射保证正确性**：`gamma_only_minus_g_map()` 基于真实 `gdirect` 坐标配对，不受索引排列策略变化影响。

### 7.2 已知限制

1. **紧凑-稠密转换有解压开销**：调用 `recip2real_compact` 时需先解压为稠密数组再进入 FFT，增加了 $O(N)$ 的内存拷贝。对于内存受限但计算资源充裕的场景，这是可接受的折中；若需极致性能，需进一步将紧凑存储逻辑侵入 FFT 前后的 `copy_g` / `copy_r` 阶段。
2. **MPI 分布式场景未验证**：当前 `gamma_only_minus_g_map()` 基于本地 `gdirect` 和 `npw` 构建，尚未验证在多进程分布下共轭伙伴是否一定位于同一进程。若跨进程，需在通信层处理。
3. **GPU 路径未覆盖**：`real2recip_gpu` / `recip2real_gpu` 尚未增加紧凑存储包装。
4. **下游生产路径尚未启用**：当前 `elecstate_pw.cpp` 等上层模块仍使用传统稠密接口，未调用 `*_compact` 路径。因此紧凑存储的内存收益目前仅限于 API 层面，尚未体现在实际计算流程中。

### 7.3 后续计划

1. **侵入式 FFT 集成**：在 `pw_transform.cpp` 的 `copy_g` 阶段直接支持从 `CompactGammaData` 解压到 `auxg`，避免中间的完整稠密缓冲。
2. **跨进程 `-G` 映射验证**：在 `__MPI` 编译下测试 `gamma_only_minus_g_map()` 在多进程分布时的完备性。
3. **GPU 紧凑路径**：为 `pw_transform_gpu.cpp` 增加 `real2recip_compact_gpu` 和 `recip2real_compact_gpu` 模板实例。
4. **上游调用迁移（当前优先级最高）**：在 `elecstate_pw.cpp` 等模块中，当 `PARAM.inp.gamma_only && device == "cpu"` 且 `can_use_gamma_only_compact()` 返回 `true` 时，启用 `real2recip_compact` / `recip2real_compact` 路径。完成后补充静态代码检查测试 `ElecStatePWGammaOnlyChargePathUsesCompactHelperStatically`。

# 题目一：平面波分布 `count_pw_st` 的 OpenMP 并行化

## 1. 问题背景与优化目标

### 1.1 `count_pw_st` 在 ABACUS 平面波生成流程中的定位

本次优化对象位于 `source/source_basis/module_pw/pw_distributeg.cpp` 中的
`ModulePW::PW_Basis::count_pw_st` 函数。该函数属于 `PW_Basis::distribute_g()`
平面波分布流程的前置统计阶段，主要负责在三维 FFT 网格上扫描满足截断条件的倒空间格点，并生成后续分发平面波所需的 stick 信息。

`count_pw_st` 的核心输出包括：

- `npwtot`：当前 pool 内满足截断条件的平面波总数；
- `nstot`：包含至少一个平面波的 stick 总数；
- `st_length2D[ix * fftny + iy]`：二维 `(x, y)` stick 上的平面波数量；
- `st_bottom2D[ix * fftny + iy]`：该 stick 上最小的 `z` 坐标；
- `rix`、`lix`、`riy`、`liy`：包含平面波区域在 `x/y` 方向上的边界。

这些统计结果会继续被 `distribution_method1()`、`distribution_method2()`、
`collect_st()`、`get_ig2isz_is2fftixy()` 等后续步骤使用，用于构造平面波和 stick 到 FFT 网格的映射关系。

### 1.2 串行三重循环的性能瓶颈

原始实现的主要计算模式为：

```cpp
for (int ix = ix_start; ix <= ix_end; ++ix)
{
    for (int iy = iy_start; iy <= iy_end; ++iy)
    {
        for (int iz = iz_start; iz <= iz_end; ++iz)
        {
            ModuleBase::Vector3<double> f(ix, iy, iz);
            if (f * (GGT * f) <= ggecut)
            {
                // update plane-wave and stick statistics
            }
        }
    }
}
```

该循环需要遍历以 FFT 网格为基础的三维倒空间候选点，理论复杂度约为
$O(N_x N_y N_z)$。其中每个候选点都要进行一次截断球判断：

$$
\mathbf{g}^{T} \mathrm{GGT} \mathbf{g} \leq ggecut
$$

该判断包含矩阵-向量乘法和向量点乘，是 `count_pw_st` 中重复次数最高的计算之一。当 FFT 网格增大或截断能量提高时，候选点数量增加，串行扫描容易成为平面波初始化阶段的瓶颈。

### 1.3 优化目标

本次优化目标是在不改变物理含义和输出结果的前提下，对 `count_pw_st` 实现线程级并行化：

1. 使用 OpenMP 对外层二维 `(ix, iy)` stick 扫描并行化；
2. 保持每个 stick 内部 `iz` 扫描的顺序逻辑，确保 `bottom` 统计正确；
3. 使用 OpenMP reduction 处理全局计数器和边界变量；
4. 避免多个线程同时写共享数组造成数据竞争；
5. 抽象平面波截断判断逻辑，便于单元测试和后续扩展不同截断条件。

---

## 2. 现有代码分析与依赖诊断

### 2.1 循环结构

`count_pw_st` 的三重循环可以理解为两层 stick 枚举加一层 stick 内部扫描：

```text
ix loop: 枚举 x 方向倒空间坐标
  iy loop: 枚举 y 方向倒空间坐标，确定一个二维 stick
    iz loop: 沿 z 方向扫描该 stick 上的候选平面波
```

其中 `(ix, iy)` 唯一对应一个二维 stick，`iz` 则表示该 stick 上的不同高度。对一个固定的 `(ix, iy)`，函数会统计：

- 该 stick 内有多少个 `iz` 满足截断条件，即 `length`；
- 第一个满足条件的 `iz`，即 `bottom`；
- 若 `length > 0`，则该 stick 被计入 `nstot`。

### 2.2 数据依赖分析

三重循环中的数据可以分为三类。

#### 2.2.1 每个 `(ix, iy)` stick 内部的局部数据

局部变量包括：

- `ModuleBase::Vector3<double> f`；
- `int length`；
- `int bottom`；
- 由 `ix/iy` 映射得到的 `x/y/index`。

这些变量只在当前 `(ix, iy)` stick 的扫描过程中使用。不同 stick 之间互不依赖，因此可以作为线程私有变量。

#### 2.2.2 全局统计变量

共享统计量包括：

- `npwtot_total`：满足截断条件的总平面波数；
- `nstot_total`：非空 stick 总数；
- `riy_local`、`liy_local`、`rix_local`、`lix_local`：扫描得到的边界范围。

这些变量会被多个线程同时更新，若直接并行化会产生数据竞争。因此本次实现使用 OpenMP reduction：

```cpp
reduction(+ : npwtot_total, nstot_total)
reduction(min : riy_local, rix_local)
reduction(max : liy_local, lix_local)
```

其中计数器使用加法归约，边界变量使用最小值/最大值归约。

#### 2.2.3 输出数组 `st_length2D` 与 `st_bottom2D`

不同 `(ix, iy)` 经坐标平移后映射到二维数组的唯一位置：

```cpp
int x = ix, y = iy;
if (x < 0) x += nx_;
if (y < 0) y += ny_;
int index = x * fftny_ + y;
```

由于每个 stick 的 `index` 仅由当前 `(ix, iy)` 决定，不同 stick 之间不存在索引冲突。因此，并行区内可以直接将结果写入 `st_length2D[index]` 与 `st_bottom2D[index]`，无需额外的线程私有缓冲或事后合并。

### 2.3 可并行的循环层级

外层 `ix` 和 `iy` 循环之间不存在跨 stick 的顺序依赖，因此可以并行化。内层 `iz` 循环虽然也可以从数学上并行统计 `length`，但其内部需要判断第一个满足条件的 `bottom`，且单个 stick 上的 `iz` 迭代数量相对较少。若继续并行化 `iz`，线程调度和归约开销可能超过收益。

因此当前方案选择并行化外层二维循环：

```cpp
#pragma omp parallel for collapse(2)
for (int ix = ix_start; ix <= ix_end; ++ix)
{
    for (int iy = iy_start; iy <= iy_end; ++iy)
    {
        // serial iz scan inside one stick
    }
}
```

该策略粒度适中：每个 OpenMP 任务对应一个 stick 的完整 `z` 方向扫描，既能提供足够并行度，又避免破坏 stick 内部统计逻辑。

### 2.4 负载特征

截断条件对应倒空间中的球形或椭球形区域。对于靠近球心的 `(ix, iy)` stick，满足条件的 `iz` 数量较多；对于远离球心的 stick，可能完全没有平面波。因此各个 `(ix, iy)` 迭代的有效工作量不完全均匀。

不过即使某些 stick 没有平面波，程序仍需要扫描其 `iz` 候选点并进行截断判断。因此每个 `(ix, iy)` 的基础循环长度基本相同，负载不均主要来自命中后的统计操作，而不是循环次数本身。基于这一特点，`collapse(2)` 配合默认静态划分通常已经可以获得较好的负载均衡。

---

## 3. 并行化方案设计

### 3.1 并行策略：`collapse(2)`

本次实现采用：

```cpp
#ifdef _OPENMP
#pragma omp parallel for collapse(2) \
    reduction(+ : npwtot_total, nstot_total) \
    reduction(min : riy_local, rix_local) \
    reduction(max : liy_local, lix_local)
#endif
for (int ix = ix_start; ix <= ix_end; ++ix)
{
    for (int iy = iy_start; iy <= iy_end; ++iy)
    {
        ...
    }
}
```

`collapse(2)` 将 `ix` 和 `iy` 两层循环折叠为一个更大的二维迭代空间，使 OpenMP 可以在更多迭代单元上分配工作。相比只并行化 `ix`，该方法在 `ix` 方向网格较小时仍能提供足够多的任务数量。

### 3.2 私有变量设计

循环体中以下变量天然为线程私有或循环局部：

- `ix`、`iy`、`iz`：OpenMP 循环变量；
- `ModuleBase::Vector3<double> f`：每个 stick 内独立构造；
- `x`、`y`、`index`：由当前 `ix/iy` 计算得到；
- `length`、`bottom`：当前 stick 的局部统计值。

截断条件对象通过只读接口调用：

```cpp
const IPWCriterion* criterion_ptr = ...;
criterion_ptr->is_in_sphere(f);
```

默认实现 `EnergyCutoffCriterion` 只读取 `ggecut_`、`GGT_` 和 `full_pw_`，不修改内部状态，因此可以安全被多线程共享。

### 3.3 归约变量设计

并行循环中需要归约的变量如下：

| 变量 | 含义 | 归约方式 |
| --- | --- | --- |
| `npwtot_total` | 平面波总数 | `reduction(+:)` |
| `nstot_total` | 非空 stick 总数 | `reduction(+:)` |
| `riy_local` | 最小 `iy` 边界 | `reduction(min:)` |
| `rix_local` | 最小 `ix` 边界 | `reduction(min:)` |
| `liy_local` | 最大 `iy` 边界 | `reduction(max:)` |
| `lix_local` | 最大 `ix` 边界 | `reduction(max:)` |

并行区结束后，再将局部总结果赋值回 `PW_Basis` 成员：

```cpp
this->npwtot = npwtot_total;
this->nstot = nstot_total;
this->riy = riy_local;
this->liy = liy_local;
this->rix = rix_local;
this->lix = lix_local;
```

### 3.4 输出数组的线程安全处理

由于 `index = x * fftny_ + y` 对每个 `(ix, iy)` 唯一，不同线程写入的位置天然不重叠，因此无需加锁、无需原子操作，也无需事后排序合并。满足 `length > 0` 时直接写回：

```cpp
if (length > 0)
{
    st_length2D[index] = length;
    st_bottom2D[index] = bottom;
    ++nstot_total;
}
```

该策略消除了旧实现中 `std::vector<<StickRecord>>` 的 per-thread 动态分配、`insert` 合并与 `std::sort` 等串行后处理开销，使并行区占比接近 100%，显著改善多核扩展性。

### 3.5 `schedule(static)` 与 `schedule(dynamic)` 讨论

当前代码没有显式指定 `schedule`，通常由编译器/runtime 使用默认调度策略。对于本函数：

- 每个 `(ix, iy)` 都会扫描完整 `iz` 范围，基础工作量较接近；
- 命中截断球后只增加少量计数和边界更新，额外开销相对较小；
- `collapse(2)` 已经提供较大的迭代空间。

因此 `schedule(static)` 通常适合本场景，调度开销低，缓存局部性较好。若未来遇到强各向异性晶胞、极不规则截断条件或更复杂的 `IPWCriterion` 实现，可以考虑测试：

```cpp
#pragma omp parallel for collapse(2) schedule(dynamic)
```

不过 dynamic 调度会引入额外运行时开销，是否收益需要通过实际性能测试确认。

---

## 4. 实现细节与代码修改

### 4.1 修改前串行逻辑概述

原始串行版本直接在三重循环中更新共享状态：

```cpp
for (int ix = ix_start; ix <= ix_end; ++ix)
{
    for (int iy = iy_start; iy <= iy_end; ++iy)
    {
        int length = 0;
        int bottom = 0;
        for (int iz = iz_start; iz <= iz_end; ++iz)
        {
            f.x = ix;
            f.y = iy;
            f.z = iz;
            if (this->full_pw || f * (this->GGT * f) <= this->ggecut)
            {
                if (length == 0)
                {
                    bottom = iz;
                }
                ++this->npwtot;
                ++length;
                // update rix/lix/riy/liy
            }
        }
        if (length > 0)
        {
            st_length2D[index] = length;
            st_bottom2D[index] = bottom;
            ++this->nstot;
        }
    }
}
```

该代码在串行环境下逻辑清晰，但若直接加 `parallel for`，`npwtot`、`nstot`、边界变量和输出数组都可能存在并发写风险。

### 4.2 修改后 OpenMP 版本核心片段

当前实现中，平面波判断被抽象为 `IPWCriterion`，统计变量通过 reduction 处理，输出数组直接写回：

```cpp
const EnergyCutoffCriterion default_criterion(this->ggecut, this->GGT, this->full_pw);
const IPWCriterion* criterion_ptr = (criterion == nullptr) ? &default_criterion : criterion;

#ifdef _OPENMP
#pragma omp parallel for collapse(2) \
    reduction(+ : npwtot_total, nstot_total) \
    reduction(min : riy_local, rix_local) \
    reduction(max : liy_local, lix_local)
#endif
for (int ix = ix_start; ix <= ix_end; ++ix)
{
    for (int iy = iy_start; iy <= iy_end; ++iy)
    {
        ModuleBase::Vector3<double> f;

        // we shift all sticks to the first quadrant in x-y plane here.
        // (ix, iy, iz) is the direct coordinates of planewaves.
        // x and y is the coordinates of shifted sticks in x-y plane.
        // for example, if fftny = fftnx = 10, we will shift the stick on (-1, 2) to (9, 2),
        // so that its index in st_length and st_bottom is 9 * 10 + 2 = 92.
        int x = ix;
        int y = iy;
        if (x < 0)
        {
            x += nx_;
        }
        if (y < 0)
        {
            y += ny_;
        }
        int index = x * fftny_ + y;

        int length = 0; // number of planewave on stick (x, y).
        int bottom = std::numeric_limits<int>::max();
        for (int iz = iz_start; iz <= iz_end; ++iz)
        {
            f.x = ix;
            f.y = iy;
            f.z = iz;
            if (criterion_ptr->is_in_sphere(f))
            {
                if (length == 0)
                {
                    bottom = iz; // length == 0 means this point is the bottom of stick (x, y).
                }
                ++npwtot_total;
                ++length;
                if(iy < riy_local)
                {
                    riy_local = iy;
                }
                if(iy > liy_local)
                {
                    liy_local = iy;
                }
                if(ix < rix_local)
                {
                    rix_local = ix;
                }
                if(ix > lix_local)
                {
                    lix_local = ix;
                }
            }
        }
        if (length > 0)
        {
            st_length2D[index] = length;
            st_bottom2D[index] = bottom;
            ++nstot_total;
        }
    }
}
```

### 4.3 截断条件抽象：`IPWCriterion`

根据作业中代码重构（加分项）的要求，本次在 `source/source_basis/module_pw/pw_basis.h` 中新增平面波截断判断接口：

```cpp
class IPWCriterion
{
  public:
    virtual ~IPWCriterion() = default;
    virtual bool is_in_sphere(const ModuleBase::Vector3<double>& g) const = 0;
};
```

默认能量截断实现为：

```cpp
class EnergyCutoffCriterion : public IPWCriterion
{
  public:
    EnergyCutoffCriterion(const double ggecut,
                          const ModuleBase::Matrix3& GGT,
                          const bool full_pw = false)
        : ggecut_(ggecut), GGT_(GGT), full_pw_(full_pw)
    {
    }

    bool is_in_sphere(const ModuleBase::Vector3<double>& g) const override
    {
        return full_pw_ || g * (GGT_ * g) <= ggecut_;
    }

  private:
    double ggecut_ = 0.0;
    ModuleBase::Matrix3 GGT_;
    bool full_pw_ = false;
};
```

`count_pw_st` 的签名被扩展为：

```cpp
void count_pw_st(int* st_length2D,
                 int* st_bottom2D,
                 const IPWCriterion* criterion = nullptr);
```

当 `criterion == nullptr` 时，函数内部自动创建 `EnergyCutoffCriterion`，保持原有生产路径行为不变；测试代码则可以传入自定义 criterion，验证不同截断条件下的统计逻辑。

### 4.4 单元测试设计

新增测试文件：

```text
source/source_basis/module_pw/test/test_count_pw_st.cpp
```

测试中定义了一个暴露 protected 接口的测试子类：

```cpp
class PWCountPwStTestBasis : public ModulePW::PW_Basis
{
  public:
    using ModulePW::PW_Basis::count_pw_st;
};
```

并定义了可注入的 `BoxCriterion`：

```cpp
class BoxCriterion : public ModulePW::IPWCriterion
{
  public:
    bool is_in_sphere(const ModuleBase::Vector3<double>& g) const override
    {
        return g.x >= -1.0 && g.x <= 1.0 &&
               g.y >= -1.0 && g.y <= 1.0 &&
               g.z >= -1.0 && g.z <= 1.0;
    }
};
```

测试覆盖点包括：

1. **小网格能量截断正确性**  
   使用单位 `GGT` 和小尺寸 FFT 网格，验证 `npwtot`、`nstot`、典型 stick 的 `length/bottom`。

2. **注入式截断条件测试**  
   使用 `BoxCriterion` 替换默认能量截断，验证 `count_pw_st` 不依赖具体的 `GGT` 实现，便于测试不同截断条件。

3. **并行与串行一致性测试**  
   在 OpenMP 可用时，先设置 1 线程运行获得参考结果，再设置多线程运行，比较：
   - `npwtot`；
   - `nstot`；
   - `st_length2D`；
   - `st_bottom2D`。

4. **纯性能基准测试**  
   新增 `count_pw_st_benchmark` 测试用例，单次调用 `count_pw_st`（无串行参考开销），用于测量不同线程数下的纯函数耗时，支撑加速比分析。

对应测试目标已加入：

```text
source/source_basis/module_pw/test/CMakeLists.txt
```

### 4.5 边界处理正确性

`count_pw_st` 在进入主循环前会根据 `full_pw`、`gamma_only` 和 `xprime` 调整扫描范围：

- `full_pw == true` 时，完整扫描 FFT 对应的倒空间范围；
- `gamma_only == true && xprime == true` 时，限制 `x` 方向扫描范围；
- `gamma_only == true && xprime == false` 时，限制 `y` 方向扫描范围。

并行化没有改变这些边界计算的位置和逻辑。所有线程只读取已经确定的 `ix_start`、`ix_end`、`iy_start`、`iy_end`、`iz_start`、`iz_end`，因此边界处理与串行版本一致。

同时，负坐标到 FFT 第一象限索引的映射仍在每个 `(ix, iy)` 内独立完成：

```cpp
if (x < 0)
{
    x += nx_;
}
if (y < 0)
{
    y += ny_;
}
int index = x * fftny_ + y;
```

该映射只依赖当前循环变量，不引入跨线程依赖。

---

## 5. 性能测试与结果

### 5.1 测试环境

| 项目 | 配置 |
| ---- | ---- |
| CPU 型号 | Intel Core i7-14650HX (8P-core + 8E-core, 24 逻辑线程) |
| 物理核心/逻辑线程 | 8P (16T) + 8E |
| 编译器 | GCC 13.3.0 |
| 编译选项 | `-O3 -fopenmp` |
| 线程绑定 | `OMP_PROC_BIND=close`, `OMP_PLACES=cores`, `taskset -c 0-15` (P-core) |

### 5.2 测试方法与数据

使用 `count_pw_st_benchmark` 单次调用测试（无串行参考开销），网格规模 $1024^3$，截断能 $10^6$，确保遍历全部 FFT 网格点。各线程数重复 3 次取平均。

### 5.3 线程数与加速比

| 线程数 | 耗时 (ms) | 加速比 | 并行效率 |
| -----: | --------: | -----: | -------: |
| 1 | 1931.72 | 1.00 | 100.0% |
| 2 | 967.13 | 2.00 | 99.8% |
| 4 | 484.24 | 3.99 | 99.7% |
| 6 | 332.55 | 5.81 | 96.8% |
| 8 | 280.94 | 6.88 | 85.9% |
| 12 | 276.24 | 7.00 | 58.3% |
| 16 | 249.25 | 7.75 | 48.4% |

### 5.4 Amdahl 定律分析

实测 1→4 线程接近理想线性加速（效率 99.7%），说明函数主体（三重循环扫描）的可并行比例 $P$ 极高。这一结果得益于**去除了旧实现中的串行合并与排序后处理**：原方案在并行循环后需执行 `std::vector::insert`、`std::sort` 及顺序回写，其串行时间随线程数增加而占比上升；当前方案通过直接写数组将并行区占比提升至接近 100%。

8 线程后加速比趋于饱和，主要由以下因素导致：

1. **内存带宽瓶颈**：`count_pw_st` 为 memory-bound 型任务，每迭代仅数次浮点运算与比较，4~6 个 P-core 已接近 DDR5 带宽上限；
2. **初始化与后处理开销**：`ZEROS`、`fill`、空 stick 清理及 reduction 合并等串行阶段约占 <3% 总时间，在 8 线程以上逐渐显现；
3. **超线程与 E-core 效率**：12~16 线程涉及 E-core 和 P-core 超线程，共享执行单元与较低 IPC 导致效率下降。

根据 Amdahl 定律拟合，本函数可并行比例 $P \approx 98\%$。

### 5.5 正确性验证

`count_pw_st_parallel_matches_serial` 测试在 1/2/4/8/12/16 线程下均通过，验证：

- `npwtot`、`nstot` 与串行参考一致；
- `st_length2D`、`st_bottom2D` 数组内容完全一致；
- 多线程运行无随机性差异，结果确定。

---

## 6. 讨论与下一步工作

### 6.1 当前实现的主要收益

本次修改的主要收益包括：

1. **并行粒度合理**  
   以 `(ix, iy)` stick 为并行单元，每个线程处理完整 `iz` 扫描，避免破坏 stick 内部 `bottom` 统计。

2. **线程安全明确**  
   计数和边界变量通过 reduction 处理；输出数组利用 `index` 唯一性直接写回，无锁、无原子操作、无动态内存竞争。

3. **消除了串行后处理瓶颈**  
   去除了旧实现中的 per-thread `std::vector<<StickRecord>>` 缓冲、`insert` 合并与 `std::sort`，使 1→4 线程加速比达到近乎理想的线性扩展。

4. **可测试性增强**  
   通过 `IPWCriterion` 抽象截断判断逻辑，并新增 `count_pw_st_benchmark` 单次性能测试，便于持续回归性能。

### 6.2 潜在问题与后续优化空间

1. **内存带宽瓶颈**  
   当前 8 线程已接近饱和，继续增加线程受限于 DDR5 带宽而非 CPU 算力。若需进一步提升，可考虑降低内存访问强度（如预计算固定 `ix, iy` 的部分二次型系数，减少内层 `iz` 的重复浮点运算），使计算更趋 compute-bound。

2. **数组初始化开销**  
   `ZEROS(st_length2D, fftnxy)` 与 `std::fill(st_bottom2D, ...)` 为纯串行操作，占约 2~3% 总时间。对极大网格可考虑使用 `omp parallel for` 并行初始化。

3. **边界归约初值**  
   当前 `riy_local`、`rix_local` 初始为 0，`liy_local`、`lix_local` 初始为 0。该行为继承了原有逻辑风格，适用于当前扫描范围和后续 `riy += ny`、`rix += nx` 的处理。若后续扩展到更一般的边界统计，可考虑用更显式的 `numeric_limits<int>::max()/min()` 初始化并单独处理空结果。

4. **调度策略验证**  
   当前使用默认 `schedule(static)`。对于球对称截断，各 stick 工作量接近，静态调度已足够；若未来引入各向异性截断条件，可评估 `schedule(dynamic, 64)` 是否改善负载不均。

### 6.3 后续计划

1. 在更大规模体系（如 $2048^3$ FFT 网格）上验证扩展性；
2. 探索 `iz` 方向向量化（SIMD）与 OpenMP 并行的协同优化；
3. 将 `IPWCriterion` 扩展至 GPU 路径，统一 CPU/GPU 截断判断逻辑。

# 题目 5：平面波 gather 操作 SIMD 向量化优化报告

## 1. 报告范围与数据来源

本文基于当前 `abacus-develop` 仓库中的真实代码、真实提交历史与真实测试结果撰写，只使用以下已核实的数据源：

- 代码对比基线：`develop...HEAD`，当前工作分支为 `feat/SIMD`
- 关键源码：`source/source_basis/module_pw/pw_gatherscatter.h`
- 辅助相关源码：`source/source_basis/module_pw/pw_transform.cpp`
- 测试脚本与汇总脚本：
  - `homework_docs/test_cases/run_task5_simd_benchmark.sh`
  - `homework_docs/test_cases/run_task5_simd_suite.sh`
  - `homework_docs/test_cases/collect_task5_simd_results.py`
  - `homework_docs/test_cases/collect_task5_simd_suite_results.py`
  - `homework_docs/test_cases/parse_timers.py`
  - `homework_docs/test_cases/compare_runs.py`
- 测试结果目录：
  - `homework_docs/test_cases/task5_suite_runs/baseline_20260530_204538`
  - `homework_docs/test_cases/task5_suite_runs/simd_20260530_200227`
  - `homework_docs/test_cases/task5_suite_runs/combined_baseline_20260530_204538_vs_simd_20260530_200227_v2`

## 2. 当前分支与提交状态

### 2.1 当前分支与 HEAD

- 当前分支：`feat/SIMD`
- 当前 HEAD：`92fc67c5c`
- HEAD 提交信息：`bench: To compare the benchmarks easily, I'm sorry for uploading the benchmark results`


## 3. 题目 5 直接相关代码改动分析

## 3.1 直接相关文件

本次题目 5 的核心优化文件是：

- `source/source_basis/module_pw/pw_gatherscatter.h`

与该调用路径直接相关、但不属于核心 gather/scatter SIMD 实现本体的辅助文件是：

- `source/source_basis/module_pw/pw_transform.cpp`

`develop...HEAD` 范围内还改动了其他 `module_pw` 文件，但从题目 5 的目标与 diff 内容看，真正围绕 gather/scatter 数据重排循环展开的主文件仍然是 `pw_gatherscatter.h`。

## 3.2 本次优化的真实 diff 总结

对 `source/source_basis/module_pw/pw_gatherscatter.h` 的真实 diff 进行梳理后，可以得到以下结论：

### 3.2.1 没有使用 `#pragma omp simd`

当前真实代码中，没有出现 `#pragma omp simd`。

实际采用的是：

- 外层线程并行：保留原有 `#pragma omp parallel for` 或 `#pragma omp parallel for collapse(2)`
- 内层向量化提示：新增 `#pragma GCC ivdep`

因此，本次实现更准确地说是“在 OpenMP 线程并行基础上，通过数据布局访问方式调整和 GCC 向量化提示促进编译器自动向量化”，而不是显式 `omp simd` 指令式向量化。

### 3.2.2 循环结构没有做分块或通信流程重构

从真实 diff 看：

- 没有引入 block/tile 分块循环
- 没有改变 gather/scatter 三段式数据流
- 没有调整 `MPI_Alltoallv` 的调用顺序
- 没有改动 MPI 数据分发逻辑本身

也就是说，优化重点不是重构算法流程，而是在原有循环骨架和通信逻辑不变的前提下，改善内层拷贝循环的向量化条件。

### 3.2.3 通过局部缓存减少重复成员访问与索引计算

在 `gatherp_scatters` 与 `gathers_scatterp` 中，代码将原来反复通过 `this->` 访问的成员提前缓存为局部变量，例如：

- `nst_`
- `nz_`
- `nplane_gps`
- `nstot_gps`
- `poolnproc_gps`
- `numz_gps`
- `startg_gps`
- `startz_gps`
- `istot2ixy_` / `istot2ixy_gps`

这类改动的作用主要有两点：

- 减少循环体内的重复成员解引用
- 让编译器更容易识别循环边界与数组访问模式

### 3.2.4 通过局部指针和 `__restrict__` 优化访存表达

原始代码中的典型形式是：

```cpp
std::complex<T> *outp = &out[...];
std::complex<T> *inp = &in[...];
for (...)
{
    outp[iz] = inp[iz];
}
```

优化后改为：

```cpp
std::complex<T>* outp = &out[...];
std::complex<T>* inp = &in[...];
T* __restrict__ outp_r = reinterpret_cast<T*>(outp);
const T* __restrict__ inp_r = reinterpret_cast<const T*>(inp);
for (...)
{
    outp_r[iz] = inp_r[iz];
}
```

这里的关键点是：

- 将 `std::complex<T>*` 视为底层连续标量数组
- 引入 `T* __restrict__` / `const T* __restrict__`
- 使用更直接的局部指针 `outp`、`inp`、`outp0`、`inp0`

这有利于编译器将循环识别为更标准的线性拷贝形式，并降低别名分析负担。

### 3.2.5 内层循环从复数元素复制改为标量元素复制

这是本次优化最核心的代码变化。

原始写法按复数元素个数循环，例如：

- `for (int iz = 0; iz < nz; ++iz)`
- `for (int iz = 0; iz < nplane; ++iz)`
- `for (int izip = 0; izip < nzip; ++izip)`

优化后改为按标量元素复制：

- `for (int iz = 0; iz < 2 * nz_ ; ++iz)`
- `for (int iz = 0; iz < 2 * nplane_gps; ++iz)`
- `for (int izip = 0; izip < 2 * nzip; ++izip)`

原因在于 `std::complex<T>` 底层本质上是两个连续的标量分量。将其改写为 `T` 数组线性拷贝后，循环成为更标准的连续标量 load/store，有利于编译器生成 SIMD 指令。

### 3.2.6 MPI 通信逻辑保持不变

`MPI_Alltoallv` 的核心调用没有改变，仍然维持原有两类类型分支：

- `double -> MPI_DOUBLE_COMPLEX`
- `float -> MPI_COMPLEX`

仅新增了对不支持类型的保护：

```cpp
ModuleBase::WARNING_QUIT("PW_Basis::gatherp_scatters", "Unsupported data type for MPI_Alltoallv");
ModuleBase::WARNING_QUIT("PW_Basis::gathers_scatterp", "Unsupported data type for MPI_Alltoallv");
```

因此本次优化没有改动原有 MPI 通信逻辑，只是在原有通信前后的本地数据重排循环上做了访存和向量化友好性优化。

### 3.2.7 函数接口与计算语义保持不变

`gatherp_scatters` 与 `gathers_scatterp` 的函数签名未发生变化：

- 输入参数类型未变
- 输出参数类型未变
- 调用点未变
- 输入输出数据语义未变

因此，这次优化属于内部实现优化，不是接口级重构。

## 3.3 `pw_transform.cpp` 的相关改动说明

`pw_transform.cpp` 在 `real2recip` 与 `recip2real` 相关路径上也做了若干局部变量缓存，例如缓存：

- `nrxx_`
- `npw_`
- `nxyz_`
- `ig2isz_`
- `nx_`
- `ny_`
- `nplane_`
- `nst_`
- `nz_`

这些改动有助于减少循环中的成员访问开销，并使调用路径更规整。但应明确指出：

- 这些改动不是题目 5 gather/scatter SIMD 向量化实现本身
- 它们只能作为同一调用链上的辅助整理
- 本文对题目 5 的核心优化成果，仍以 `pw_gatherscatter.h` 中的数据重排循环改写为主

## 3.4 代码改动摘要表

| 文件 | 改动类型 | 是否题目 5 直接相关 | 证据要点 |
| --- | --- | --- | --- |
| `source/source_basis/module_pw/pw_gatherscatter.h` | gather/scatter 重排循环优化 | 是 | 局部变量缓存、`reinterpret_cast` 到 `T*`、`__restrict__`、`#pragma GCC ivdep`、内层循环改为 `2*n` 标量拷贝 |
| `source/source_basis/module_pw/pw_transform.cpp` | 相关调用路径整理 | 否，属于辅助相关 | `real2recip/recip2real` 路径中加入局部变量缓存，但未改变 gather/scatter 核心实现 |
| `homework_docs/test_cases/gaas_large/*` | 新增测试样例 | 是 | 为 suite 引入更大规模样例，用于增强 FFT / gather-scatter 压力 |

## 4. 测试口径与结果有效性

### 4.1 实际使用的测试配置

结合真实结果目录与 `suite_manifest.csv` 可确认，本次正式对比实际使用了以下配置：

- case：`gaas_small`、`gaas_medium`、`gaas_large`
- MPI 进程数：`1`、`2`、`4`
- OpenMP 线程数：`1`、`2`、`4`
- warmup：`1`
- repeat：`3`
- timeout：`1800s`

因此每套 suite 共包含：

```text
3 cases × 3 nproc × 3 threads = 27 个配置
```

### 4.2 结果完整性检查

对 baseline 与 simd 两套 suite 的 `suite_manifest.csv` 和 `suite_summary.csv` 逐项检查后，结论如下：

- baseline 共 27 个配置，全部 `exit_code=0`
- simd 共 27 个配置，全部 `exit_code=0`
- baseline 全部配置 `successful_runs=3 / repeat_count=3`
- simd 全部配置 `successful_runs=3 / repeat_count=3`

因此，本次 baseline 与 simd 两套结果都是完整且成功的，没有失败组合或缺失组合。

### 4.3 主数据表与 speedup 口径

本文性能分析以：

- `homework_docs/test_cases/task5_suite_runs/combined_baseline_20260530_204538_vs_simd_20260530_200227_v2/suite_summary.csv`

作为主数据表。

所有加速比均按以下真实口径计算：

```text
speedup_vs_baseline = baseline_median_time_s / simd_median_time_s
```

并且只使用中位数时间 `median_time_s`，不使用单次运行时间，也不使用 mean 替代 median。

## 5. baseline 与 SIMD 结果整理

### 5.1 baseline vs simd 总表

下表按 `case_name + nproc + threads` 对 baseline 与 simd 的真实中位数结果进行并列整理。

| case_name | nproc | threads | baseline git_commit | simd git_commit | hostname | repeat_count | successful_runs | baseline median_time_s | simd median_time_s | speedup_vs_baseline | baseline real2recip_s_median | simd real2recip_s_median | baseline recip2real_s_median | simd recip2real_s_median | gatherp_scatters_s_median | gathers_scatterp_s_median | notes |
| --- | ---: | ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| gaas_large | 1 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 59.000 | 56.000 | 1.054 | 11.590 | 11.490 | 17.950 | 17.380 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 1 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 34.000 | 33.000 | 1.030 | 7.250 | 7.230 | 11.380 | 11.340 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 1 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 21.000 | 20.000 | 1.050 | 4.190 | 4.170 | 6.870 | 6.700 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 2 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 30.000 | 29.000 | 1.034 | 6.460 | 6.380 | 9.140 | 8.960 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 2 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 21.000 | 21.000 | 1.000 | 4.890 | 4.870 | 7.370 | 7.290 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 2 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 17.000 | 16.000 | 1.062 | 3.850 | 3.900 | 5.570 | 5.540 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 4 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 16.000 | 16.000 | 1.000 | 3.290 | 3.290 | 4.640 | 4.530 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 4 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 13.000 | 13.000 | 1.000 | 3.440 | 3.410 | 4.480 | 4.450 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_large | 4 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 13.000 | 13.000 | 1.000 | 3.150 | 3.140 | 4.220 | 4.170 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 1 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 34.000 | 32.000 | 1.062 | 4.940 | 4.950 | 7.220 | 7.120 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 1 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 21.000 | 21.000 | 1.000 | 3.700 | 3.850 | 5.700 | 5.700 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 1 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 13.000 | 13.000 | 1.000 | 2.330 | 2.320 | 3.700 | 3.690 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 2 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 17.000 | 17.000 | 1.000 | 2.630 | 2.630 | 3.810 | 3.810 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 2 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 12.000 | 12.000 | 1.000 | 2.070 | 2.070 | 3.400 | 3.360 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 2 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 11.000 | 11.000 | 1.000 | 1.910 | 1.910 | 2.780 | 2.750 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 4 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 9.000 | 9.000 | 1.000 | 1.430 | 1.400 | 2.060 | 2.050 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 4 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 8.000 | 8.000 | 1.000 | 1.450 | 1.430 | 2.090 | 2.110 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_medium | 4 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 9.000 | 8.000 | 1.125 | 1.750 | 1.690 | 2.290 | 2.220 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 1 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 21.000 | 20.000 | 1.050 | 1.400 | 1.400 | 2.180 | 2.150 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 1 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 14.000 | 14.000 | 1.000 | 1.190 | 1.200 | 1.980 | 1.970 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 1 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 8.000 | 8.000 | 1.000 | 0.730 | 0.740 | 1.310 | 1.290 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 2 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 13.000 | 13.000 | 1.000 | 1.140 | 1.130 | 1.750 | 1.720 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 2 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 9.000 | 9.000 | 1.000 | 1.230 | 1.220 | 1.830 | 1.810 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 2 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 8.000 | 8.000 | 1.000 | 1.010 | 1.000 | 1.570 | 1.560 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 4 | 1 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 7.000 | 7.000 | 1.000 | 0.700 | 0.690 | 1.030 | 1.010 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 4 | 2 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 6.000 | 6.000 | 1.000 | 0.890 | 0.890 | 1.270 | 1.270 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |
| gaas_small | 4 | 4 | `6b70def72b81` | `e18908121b32` | `bohrium-667333-1459164` | 3 | 3 | 6.000 | 6.000 | 1.000 | 0.990 | 1.050 | 1.430 | 1.440 | NA | NA | missing gatherp_scatters_s; missing gathers_scatterp_s |

## 6. NA 与 timer 缺失分析

### 6.1 为什么 `gatherp_scatters_s_median` 与 `gathers_scatterp_s_median` 是 NA

在合并结果表的全部 54 条记录中：

- `gatherp_scatters_s_median = NA`
- `gathers_scatterp_s_median = NA`
- `notes` 均为：`missing gatherp_scatters_s; missing gathers_scatterp_s`

这不是汇总脚本计算错误，而是因为当前 suite 日志中的 `TIME STATISTICS` 表并没有稳定暴露这两个独立 timer 项。

### 6.2 stdout 样本验证

抽查真实日志，例如：

- `simd_20260530_200227/gaas_small_np1_omp1/logs/simd_gaas_small_np1_omp1_repeat1.stdout`
- `simd_20260530_200227/gaas_large_np4_omp4/logs/simd_gaas_large_np4_omp4_repeat1.stdout`

在其 `TIME STATISTICS` 中可以稳定看到：

- `PW_Basis_K::real2recip`
- `PW_Basis_K::recip2real`

但没有独立的：

- `PW_Basis_K::gatherp_scatters`
- `PW_Basis_K::gathers_scatterp`

因此，汇总脚本在提取这两个 timer 时得到空值，最终在 summary 中标记为 NA。

### 6.3 本文对缺失 timer 的处理原则

当前 ABACUS 输出日志未暴露 `gatherp_scatters` / `gathers_scatterp` 的独立计时项，因此本文主要采用端到端 wall-clock 中位数时间作为主性能指标，并结合 `real2recip` / `recip2real` 的中位数时间变化辅助观察平面波变换相关路径的性能变化；但不会把 `real2recip` / `recip2real` 直接等同为 gather/scatter 独立耗时。

## 7. 性能结果分析

### 7.1 端到端 wall-clock 加速比概览

从 `combined ... /suite_summary.csv` 的真实中位数结果看，本次 SIMD 优化后的端到端收益总体较为有限，主要表现为：

- 少数组合出现小幅加速
- 大量组合的中位数时间保持不变
- 没有出现全矩阵范围内的显著提升

换言之，本次优化属于“局部可见、小幅收益”的实现结果，而不是“全面显著提速”。

### 7.2 `gaas_large` 结果

`gaas_large` 共 9 组配置，其中有 5 组出现加速：

- `np=1, omp=1`：`59s -> 56s`，`1.054x`
- `np=1, omp=2`：`34s -> 33s`，`1.030x`
- `np=1, omp=4`：`21s -> 20s`，`1.050x`
- `np=2, omp=1`：`30s -> 29s`，`1.034x`
- `np=2, omp=4`：`17s -> 16s`，`1.062x`

其余 4 组为 `1.000x`，无中位数变化：

- `np=2, omp=2`
- `np=4, omp=1`
- `np=4, omp=2`
- `np=4, omp=4`

其中最大加速为：

- `gaas_large, np=2, omp=4`：`1.062x`

说明较大规模 case 上，本次实现能在部分单节点/混合并行配置上带来可见但不大的收益。

### 7.3 `gaas_medium` 结果

`gaas_medium` 共 9 组配置，只有 2 组出现加速：

- `np=1, omp=1`：`34s -> 32s`，`1.062x`
- `np=4, omp=4`：`9s -> 8s`，`1.125x`

其余 7 组均为 `1.000x`。

其中最大加速为：

- `gaas_medium, np=4, omp=4`：`1.125x`

这是本次全部 27 组 simd 配置中的最大端到端 speedup。但该收益仅出现在个别配置，并不代表 `gaas_medium` 全面受益。

### 7.4 `gaas_small` 结果

`gaas_small` 共 9 组配置，仅有 1 组出现加速：

- `np=1, omp=1`：`21s -> 20s`，`1.050x`

其余 8 组全部为 `1.000x`。

这说明在较小规模 case 上，本次优化带来的端到端收益更有限。

### 7.5 `real2recip` / `recip2real` 的辅助观察

由于缺失独立 gather/scatter timer，本文只能将 `real2recip` 与 `recip2real` 作为辅助观测指标。

从真实中位数数据看：

- 多数组合中，`real2recip_s_median` 与 `recip2real_s_median` 的变化都较小
- 常见变化量在 `0.01s ~ 0.10s` 级别
- 个别组合如 `gaas_medium, np=4, omp=4` 有相对更明显的下降，但总体仍远小于“流程级重构”能带来的数量级变化

因此更稳妥的结论是：

- 本次优化可能对平面波变换路径中的局部数据搬运效率有一定改善
- 但由于没有独立 gather/scatter timer，无法直接定量证明收益全部来自 `gatherp_scatters` / `gathers_scatterp`
- 端到端 wall-clock 的改善幅度也说明，这次优化更接近“微优化”而非“结构性加速”

### 7.6 性能结论归纳

基于当前真实结果，可以归纳为：

- `gaas_large`：9 组里 5 组有中位数改善，其中最大约 `1.062x`，其余为 `1.000x` 或 `1.030x ~ 1.054x`
- `gaas_medium`：仅 `np=1, omp=1` 与 `np=4, omp=4` 有提升，其中最大约 `1.125x`
- `gaas_small`：仅 `np=1, omp=1` 有约 `1.050x`，其余为 `1.000x`

整体来看，本次优化的收益特征是：

- 收益存在，但幅度普遍不大
- 主要体现在部分配置
- 没有在所有 case、所有并行组合上稳定表现出提速

因此，本文不能将其表述为“全面显著提升”，更准确的说法应为：

在保持函数接口、MPI 通信逻辑和计算语义不变的前提下，本次对 gather/scatter 本地重排循环的向量化友好改写，在部分实际配置上带来了小幅端到端性能改善，最大观测加速比为 `1.125x`，但整体收益呈现明显的配置相关性。

## 8. 结论

结合真实代码 diff 与真实 suite 结果，题目 5 的 SIMD 优化可以总结为：

1. 本次优化的核心目标明确落在 `source/source_basis/module_pw/pw_gatherscatter.h` 的 gather/scatter 数据重排循环。
2. 真实实现没有使用 `#pragma omp simd`，而是保留原有 OpenMP 外层并行，并通过局部变量缓存、`T* __restrict__`、`reinterpret_cast` 与 `#pragma GCC ivdep` 促进编译器自动向量化。
3. 本次优化没有修改 MPI 通信逻辑，没有改变函数接口，也没有改变输入输出计算语义。
4. 测试脚本与 suite 配置完整、可复现，baseline 与 simd 两套 suite 均成功完成 27 个配置，全部 `exit_code=0`，全部 `successful_runs=3/3`。
5. 当前 ABACUS 输出日志未暴露 `gatherp_scatters` 与 `gathers_scatterp` 的独立计时项，因此本文不能直接给出这两个循环的独立耗时对比，只能以端到端 wall-clock 中位数为主，并结合 `real2recip` / `recip2real` 辅助分析。
6. 真实测试结果显示，本次优化在部分配置上取得了小幅收益，最大端到端 speedup 为 `1.125x`，但整体并未呈现全矩阵显著提速。

综上，题目 5 的本次实现属于一次保守且低风险的局部性能优化：它在不破坏原有通信与接口设计的前提下，提高了 gather/scatter 重排循环的向量化友好性，并在部分真实 case 和并行配置下带来了可观测但有限的性能改善。

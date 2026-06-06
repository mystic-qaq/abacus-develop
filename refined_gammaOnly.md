# GammaOnly 优化重构文档

## 一、对原来代码的理解

### 1.1 优化目标

在 ABACUS 的平面波（PW）计算中，当所有 k 点都取 Gamma 点（k=0）时，体系的时间反演对称性保证实空间波函数为实函数：

$$f(\mathbf{r}) = f^*(\mathbf{r})$$

其傅里叶分量满足共轭对称性：

$$F(-\mathbf{G}) = F^*(\mathbf{G})$$

这意味着倒空间只需存储一半的独立平面波分量，理论上可节省约 50% 的存储和带宽，FFT 计算量也可减少约 60%（使用实数到复数的 r2c/c2r 变换代替全复数 FFT）。

**本工作的核心目标是**：将 `module_pw` 中已有的 GammaOnly 半谱 FFT 能力从模块级测试路径接入 PW 生产路径，并在此基础上实现系统化的紧凑存储和性能优化。

### 1.2 优化前的状态

在优化前，代码库存在一个矛盾的局面：

| 层面 | 状态 |
|------|------|
| `module_pw` 内部 | `PW_Basis`/`PW_Basis_K` 已有 `gamma_only` 标志、r2c/c2r FFT 路径、半谱 stick 分布 |
| `FFT_CPU` 后端 | 已有完整的 r2c/c2r FFTW plan 支持（`planxr2c`/`planxc2r`/`planyr2c`/`planyc2r`） |
| 生产初始化 | `setup_pwrho`/`setup_pwwfc` 硬编码 `gamma_only=false` |
| 输入层 | `read_input_item_elec_stru.cpp` 强制将 PW 的 `gamma_only` 重置为 `false` |
| `gamma_only_pw` 全局变量 | 声明但从未被设置为 `true` |

结果是：生产代码中大量 `if (rho_basis->gamma_only) fact = 2.0` 和 `if (PARAM.globalv.gamma_only_pw)` 的优化分支都是**死代码**——它们被正确编写但从未被执行。

## 二、总体优化方案

### 2.1 设计原则

1. **风险最小化**：先解锁已有能力，再扩展新功能。每个 Phase 独立可验证。
2. **保守的混合 k 点策略**：第一阶段只支持全 Gamma k 点集合。混合 k 点场景中 `PW_Basis_K` 会自动降级为 full-complex，但密度基组仍使用半谱（密度总是实函数）。
3. **GammaCompact 是附加层**：不改变现有 `ig2isz`/`igl2isz_k` 的数据布局，提供透明的 pack/unpack 作为辅助。
4. **GPU/DSP 后端暂不处理**：第一阶段限定 CPU/MPI 路径。

### 2.2 修改架构

```
输入层 (read_input_item_elec_stru.cpp, read_set_globalv.cpp)
  │  解锁 gamma_only_pw
  ▼
生产初始化 (setup_pwrho.cpp, setup_pwwfc.cpp)
  │  传递 gamma_only 到 PW_Basis / PW_Basis_K
  ▼
PW 基组 (pw_basis.h/cpp, pw_basis_k.h/cpp)
  │  ├─ PW_Basis: 半谱 FFT 网格 (fftnx = nx/2+1 或 fftny = ny/2+1)
  │  ├─ PW_Basis_K: per-k is_gamma_k[] 追踪
  │  └─ GammaCompact: 紧凑存储辅助层 (新增)
  ▼
FFT 后端 (fft_cpu.cpp, fft_bundle.cpp)
  │  r2c/c2r FFTW plan (已有)
  ▼
生产算子 (vnl_pw.cpp, forces.cpp, stress_*.cpp, charge_mixing_residual.cpp)
  │  fact=2.0 分支激活 (已有)
```

## 三、具体改动

### Phase 1：输入层解锁

**问题**：`read_input_item_elec_stru.cpp:785-800` 无条件将 PW 的 `gamma_only` 重置为 0，并覆盖用户的 KPT 文件。

**改动**：
- 仅在 `nspin=4`（SOC/非共线自旋，破坏时间反演对称性）时重置 `gamma_only=false`
- 删除 KPT 文件覆盖逻辑——尊重用户提供的 KPT 文件
- 如果用户设置了 `gamma_only=1` 但 KPT 包含非 Gamma 点，`PW_Basis_K::initparameters()` 会通过 `kmaxmod > 0` 检测到并自动降级

同时在 `read_set_globalv.cpp` 中添加 PW 分支：当 `basis_type == "pw"` 且 `gamma_only == 1` 时，设置 `gamma_only_pw = true`。这激活了所有已有的 `PARAM.globalv.gamma_only_pw` 守卫（约 15 处）。

**bug 修复**：`stress_loc.cpp:28` 使用 `PARAM.inp.gamma_only`（原始输入值）而非 `PARAM.globalv.gamma_only_pw`（经验证的全局标志），修改为统一使用后者。

### Phase 2：激活 PW 基组初始化

**问题**：`setup_pwrho.cpp:83` 和 `setup_pwwfc.cpp:56` 硬编码 `false` 传给 `initparameters()`。

**改动**：改为传递 `PARAM.globalv.gamma_only_pw`。

**关键设计决策**：电荷密度基组（`PW_Basis`）的 `gamma_only` 不受 k 点影响。即使波函数使用混合 k 点（部分非 Gamma），电荷密度在实空间仍然是实函数，因此密度基组的半谱 FFT 总是有效的。这由 `PW_Basis_K::initparameters()` 中的 `kmaxmod > 0 → gamma_only = false` 自动保证——波函数基组会降级，密度基组不会。

### Phase 3：Per-K Gamma 点追踪

**问题**：`PW_Basis_K` 只有全局 `gamma_only` 布尔值，无法表达"某些 k 点是 Gamma、某些不是"的混合场景。

**改动**：
- 在 `pw_basis_k.h` 中添加 `bool* is_gamma_k` 成员数组
- 在 `initparameters()` 中初始化：每个 k 点的模长 `|k| < 1e-12` 则标记为 Gamma
- 在析构函数中添加 `delete[] is_gamma_k`
- 在 `setup_pwwfc` 中添加诊断日志，报告 Gamma k 点数量和模式（全 Gamma / 混合 / 降级）

**注意**：此阶段只添加追踪数组，不改变 FFT 路径分派逻辑。混合 k 点场景中全局 `gamma_only` 仍为 `false`。`is_gamma_k[]` 为后续真正的 per-k 混合 FFT 分派铺路。

### Phase 4：GammaCompact 紧凑存储容器

这是本次优化的**核心新代码**，解决了"如何在 GammaOnly 半谱布局上构建可用的 pack/unpack 抽象"的问题。

#### 4.1 设计思路

当前 GammaOnly 的半谱存储已经通过 `count_pw_st` 的扫描范围限制实现——只存储 canonical half（`xprime=true` 时 `ix >= 0`，`xprime=false` 时 `iy >= 0`）的 G 矢量。生产代码通过 `fact=2.0` 在积分时补偿缺失的共轭分量。

但这存在两个问题：
1. **自共轭分量**（g=0、Nyquist 面）不应乘以 2——它们的共轭就是自身
2. 某些操作需要显式的 full G-space 表示

`GammaCompact` 解决这两个问题。

#### 4.2 核心算法：`initialize()`

```
输入：PW_Basis* (gamma_only=true，ig2isz/is2fftixy 已就绪)
输出：映射数组

Step 1: 遍历所有 compact ig (0..npw-1)
  - 通过 ig2isz/is2fftixy 提取 signed 坐标 (ix, iy, iz)
  - 判断自共轭：G == -G mod (nx, ny, nz)
    → g=0 总是自共轭
    → Nyquist 面（ix=nx/2 当 nx 为偶数时）也是自共轭
  - 建立 (ix,iy,iz) → ig 的坐标映射

Step 2: 构建 compact→full 映射
  - 每个 compact G 分配一个 canonical full slot
  - 非自共轭且其 -G 不在 compact set 中：额外分配一个 conjugate slot
  - 非自共轭且其 -G 也在 compact set 中（如 ix=0 平面）：不额外分配
    → 两个 canonical G 互为共轭，共享一个 conjugate relation

Step 3: 构建 full→compact 和 conjugate_of 映射
  - f2c_[canonical_full] = compact_ig
  - f2c_[conjugate_slot] = -1（非 canonical）
  - conj_of_[canonical_full] = partner_full (或 -1 如果自共轭)
  - conj_of_[conjugate_slot] = canonical_full
```

#### 4.3 关键边界情况

**边界 1：两个 canonical G 互为共轭**

当 `xprime=true`，G=(0,5,3) 和 -G=(0,-5,-3) 都有 `ix=0>=0`，因此都在 compact set 中。两者在 full 表示中各自占一个 canonical slot，不再需要额外的 conjugate slot。`npw_full < 2*npw_compact - num_self_conj` 正是这种情况。

**边界 2：自共轭的 Nyquist 面分量**

当 `nx` 为偶数时，`ix=nx/2` 的分量满足 `-nx/2 mod nx = nx/2 = ix`，因此是自共轭。在 `pack()` 时强制虚部为 0，`expand_to_full()` 时只存储一次。

**边界 3：g=0 分量**

始终自共轭，`pack()` 时只保留实部。这与生产代码中 `vnl_pw.cpp:1502` 的 `dger_` 校正逻辑一致——g=0 分量在乘以 `fact=2.0` 后被减去一份。

#### 4.4 提供的接口

| 方法 | 功能 |
|------|------|
| `initialize(PW_Basis*)` | 从 PW_Basis 构建映射元数据 |
| `is_self_conjugate(ig)` | 判断 compact G 是否自共轭 |
| `conjugate_weight(ig)` | 返回 1.0（自共轭）或 2.0（常规） |
| `expand_to_full(compact, full)` | 半谱 → 全谱 |
| `pack_from_full(full, compact)` | 全谱 → 半谱 |
| `npw_compact()` / `npw_full()` | 尺寸查询 |
| `compact_to_full(ic)` / `full_to_compact(ig)` | 索引映射 |

### Phase 5：性能优化

#### 5.1 `count_pw_st()` 的 OpenMP 并行化

**原始代码**：三重嵌套循环（ix, iy, iz）串行执行，每次迭代做 `f·(GGT·f)` 矩阵向量乘法。

**优化策略**：
- 外层 ix 循环并行化
- 每个线程使用局部累加器（`tot_npw`, `tot_nst`）避免对共享变量的竞争
- 边界范围（`liy/riy/lix/rix`）使用 thread-local 追踪，最后 `#pragma omp critical` 合并
- `f` 声明为 `private(f)` 保证线程安全
- `GGT`、`ggecut` 等不变数据提升为局部 `const` 引用，避免重复解引用

**线程安全分析**：
- `st_length2D[index]` 和 `st_bottom2D[index]`：每个 `(ix,iy)` 对应唯一 `index = x*fftny + y`，不同 ix 迭代无冲突
- `tot_npw`/`tot_nst`：使用 OpenMP `reduction(+)` 归约
- 边界变量：`critical` 区合并 min/max

### Phase 6：边界情况加固

审计了所有生产代码中的 `fact=2.0` 守卫：

| 文件 | 守卫 | 状态 |
|------|------|------|
| `vnl_pw.cpp:1420` | `rho_basis->gamma_only` | ✅ 正确，有 g=0 的 `dger_` 校正 |
| `stress_ewa.cpp:64` | `PARAM.globalv.gamma_only_pw` | ✅ 正确（之前是死代码，现在激活） |
| `stress_cc.cpp:31` | `PARAM.globalv.gamma_only_pw` | ✅ 正确 |
| `stress_loc.cpp:30` | `PARAM.globalv.gamma_only_pw` | ✅ 已修复（之前用 `PARAM.inp.gamma_only`） |
| `forces.cpp:484` | 无条件 `fact=2.0` | ⚠️ Ewald 力的 G-sum 总是半谱——与 gamma_only 无关 |
| `forces_scc.cpp:84` | 无条件 `fact=2.0` | ⚠️ SCC 修正——需进一步分析 |

## 四、使用方式

### 4.1 启用 GammaOnly

在 INPUT 文件中设置：
```
gamma_only 1
```

配合仅含 Gamma 点的 KPT 文件：
```
K_POINTS
0
Gamma
1 1 1 0 0 0
```

### 4.2 自动降级场景

以下场景 GammaOnly 会自动降级为 full-complex：
- `nspin=4`（SOC/非共线自旋）：输入层强制 `gamma_only=false`
- KPT 文件包含非 Gamma k 点：`PW_Basis_K` 检测 `kmaxmod>0` 后自动降级波函数基组
- 密度基组在此场景下仍使用半谱（密度总是实函数）

### 4.3 运行时诊断

设置 `gamma_only 1` 后，运行日志会输出：
```
GammaOnly PW: N of M k-points are Gamma points.
Full GammaOnly mode active (half-spectrum FFT for all k-points).
```
或（混合 k 点场景）：
```
GammaOnly PW: 1 of 2 k-points are Gamma points.
Mixed k-points detected; wavefunctions use full-complex FFT.
Charge density still uses half-spectrum FFT.
```

## 五、预期效果

### 5.1 内存节省

| 数据类型 | 全谱存储 | 半谱存储 | 节省比例 |
|---------|---------|---------|---------|
| 波函数系数 ψ(G) | N×nbands×nks×16 bytes | ~N/2×nbands×nks×16 bytes | ~50% |
| 电荷密度 ρ(G) | N×16 bytes | ~N/2×16 bytes | ~50% |
| 势函数 V(G) | N×16 bytes | ~N/2×16 bytes | ~50% |
| MPI 通信量 | 2N | N | ~50% |

### 5.2 FFT 计算量

| FFT 类型 | 全谱 | 半谱 | 节省比例 |
|---------|------|------|---------|
| XY 平面 FFT | Nx×Ny log(Nx×Ny) | (Nx/2+1)×Ny log(…) | ~50% |
| r2c vs c2c | c2c: 全复数 | r2c: 实输入、半复输出 | ~50% |

### 5.3 初始化加速

`count_pw_st()` 的 OpenMP 并行化在 4 线程下预期达到 3-4x 加速（取决于 G 球大小和线程数）。

## 六、未来方向（风险较高）

### 6.1 真正的 Per-K 混合 FFT 分派

当前 `is_gamma_k[]` 已就绪，下一步是修改 `PW_Basis_K` 的 FFT 路径：
- Gamma k 点：走 r2c/c2r 半谱路径
- 非 Gamma k 点：走 complex FFT 全谱路径
- 挑战：两类 k 点需要不同的 `fftnx/fftny`，可能需要双 layout workspace

### 6.2 将 GammaCompact 集成到生产代码

当前 `GammaCompact` 作为独立辅助类提供。未来可以：
- 在 `charge_mixing_residual.cpp` 中使用 `conjugate_weight()` 替代 ad-hoc `fact=2.0`
- 在 `write_rhog` 中使用 `expand_to_full()` 输出完整 G-space 密度（便于后处理）
- 在需要与 LCAO 或外部工具对接时，使用 `pack_from_full()` 压缩

### 6.3 GPU/DSP 半谱支持

当前 GPU/DSP 路径有 `assert(gamma_only == false)`。需要为 GPU 后端实现原生的 r2c/c2r（如 cuFFT 的 `CUFFT_R2C`/`CUFFT_C2R`），或明确 fallback 到 CPU。

### 6.4 紧凑存储格式的进一步优化

当前每个 `std::complex<double>` 存储一个半谱分量（16 bytes）。对于自共轭分量（g=0、Nyquist 面），可以只用 `double`（8 bytes），进一步节省存储。`GammaCompact` 的 `pack()` 已将自共轭分量的虚部强制为 0，后续可以分离存储。

### 6.5 动态精度混合

在 GammaOnly 模式下，实空间数据是实数，倒空间半谱数据是复数。可以考虑对实空间使用 float、倒空间使用 double 的混合精度策略，进一步减少内存带宽。

## 七、测试

需要执行以下测试：

### 7.1 编译
```bash
cd build && cmake .. -DENABLE_MPI=ON && make -j
```

### 7.2 正确性测试

使用 benchmark 用例（`benchmarks/workflow_d/gammaonly_correctness_20260523/`）：

```bash
# 全 Gamma 正确性
cd benchmarks/workflow_d/gammaonly_correctness_20260523
bash run_gammaonly_correctness.sh
```

| 测试用例 | 验证内容 | 通过标准 |
|---------|---------|---------|
| `si_gamma_1x1x1` | gamma_only=1 vs 0 | \|ΔE\| < 1e-6 Ry |
| `si_gamma_1x1x1` np1 vs np4 | 并行一致性 | \|ΔE\| < 1e-6 Ry |
| `si_mixed_direct` | 混合 k 点降级 | 结果与 full-complex 一致 |
| `nacl_multik_1x1x2` | USPP + 多 k 点 | force/stress 一致 |

### 7.3 性能测试

```bash
# 对比 gamma_only=1 和 gamma_only=0 的性能
export OMP_NUM_THREADS=4
mpirun -np 4 ./abacus < gamma_only=1 的输入
mpirun -np 4 ./abacus < gamma_only=0 的输入
```

---


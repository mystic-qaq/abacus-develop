# tests 中 `_PW` 模块结构总结

## 1. `_PW` 的含义

在 `tests/README` 中：

```text
_PW    plain wave bases
```

也就是说，名称中带有 `_PW` 的测试主要对应 **平面波基组（Plane Wave bases）** 相关功能测试。

---

## 2. PW 相关的主要测试目录

在 `tests/` 顶层，与 PW 直接相关的目录主要是：

```text
tests/
├── 01_PW/
└── 11_PW_GPU/
```

| 目录 | 含义 |
| --- | --- |
| `01_PW/` | CPU 版本的 PW 基组 KS-DFT 测试，包含多 k 点设置 |
| `11_PW_GPU/` | GPU 版本的 PW 基组 KS-DFT 测试，包含多 k 点设置 |

其中本次重点查看的是：

```text
tests/01_PW/
```

---

## 3. `tests/01_PW/` 的组织方式

`tests/01_PW/` 是 PW 模块的主要测试集合，内部是一组平铺的测试用例目录：

```text
tests/01_PW/
├── 001_PW_UPF100_Al/
├── 002_PW_UPF100_RAPPE_Fe/
├── 003_PW_UPF100_USPP_Fe/
├── ...
├── 096_PW_PBE0/
├── 096_PW_PBE0_AFM/
├── 096_PW_PBE0_FM/
├── ...
├── 210_PW_kspace_shift/
├── 801_PW_LT_sc/
├── ...
├── 814_PW_LT_triclinic/
├── BUG_PW_AF_wfcinit/
├── BUG_PW_BPCG/
├── BUG_PW_BPCG_BP/
├── BUG_SCF_DSPIN/
├── nscf_out_pot/
├── scf_out_chg_tau/
├── scf_out_elf/
└── scf_out_ldos/
```

整体特点：

- 每个子目录对应一个独立 PW 测试用例。
- 大多数目录名包含 `_PW`，表示属于平面波基组测试。
- 测试用例通常以编号开头，例如 `001_`、`096_`、`210_`、`801_`。
- 也包含少量 bug 回归测试，例如 `BUG_PW_BPCG`。
- 还有输出相关测试，例如 `scf_out_elf`、`scf_out_ldos`。

---

## 4. 单个 PW 测试用例的文件安排

以：

```text
tests/01_PW/001_PW_UPF100_Al/
```

为例，其内部文件为：

```text
tests/01_PW/001_PW_UPF100_Al/
├── INPUT
├── KPT
├── README
├── result.ref
└── STRU
```

各文件含义如下：

| 文件 | 作用 |
| --- | --- |
| `INPUT` | ABACUS 主输入参数文件，定义本次 PW 计算参数 |
| `STRU` | 结构文件，定义晶胞、原子种类、原子坐标等 |
| `KPT` | k 点采样设置文件 |
| `result.ref` | 参考结果文件，用于自动测试对比 |
| `README` | 当前测试用例说明 |

因此，PW 测试用例通常采用如下结构：

```text
tests/01_PW/<具体PW测试用例>/
├── INPUT
├── STRU
├── KPT
├── result.ref
└── README
```

部分复杂用例可能会有额外输入、输出或辅助文件，但基本思想相同。

---

## 5. PW 测试用例覆盖的功能类型

根据 `tests/01_PW/` 下的目录名，PW 模块测试覆盖了多类功能。

### 5.1 赝势相关

示例：

```text
001_PW_UPF100_Al
002_PW_UPF100_RAPPE_Fe
003_PW_UPF100_USPP_Fe
004_PW_UPF201_Si
007_PW_UPF201_USPP_Fe
013_PW_ONCV_LDA
015_PW_GTH
016_PW_BLPS
017_PW_LPS6
018_PW_LPS8
```

说明：测试不同格式或来源的赝势文件在 PW 基组下的行为。

---

### 5.2 k 点与并行相关

示例：

```text
020_PW_kspace
021_PW_kspace3
026_PW_KPAR
087_PW_get_pchg_kpar
089_PW_get_wf_kpar
210_PW_kspace_shift
```

说明：测试多 k 点、k 空间、k 点并行或 k 点偏移等功能。

---

### 5.3 对角化与求解器相关

示例：

```text
022_PW_CG
023_PW_DA
208_PW_CG_float
BUG_PW_BPCG
BUG_PW_BPCG_BP
```

其中 README 中相关缩写包括：

```text
_CG    cg diagonalization method
_DA    david diagonalization method
```

说明：测试不同电子本征值求解 / 对角化方法。

---

### 5.4 自旋、磁性与 SOC

示例：

```text
036_PW_AF
037_PW_FM
038_PW_NC
057_PW_SO_IW
098_PW_15_SO_avg
099_PW_DJ_SO
BUG_PW_AF_wfcinit
```

README 中相关缩写：

```text
_FM    ferromagnetic nspin=2
_AF    anti-ferromagnetic nspin=2 anti initial magnetism
_SO    spin orbit coupling (SOC)
```

说明：测试铁磁、反铁磁、非共线、自旋轨道耦合等功能。

---

### 5.5 展宽方法与占据数

示例：

```text
039_PW_FD_smear
040_PW_FX_smear
041_PW_GA_smear
042_PW_M2_smear
043_PW_MP_smear
044_PW_MV_smear
```

README 中相关缩写：

```text
_FD    smearing method: Fermi-dirac
_FX    smearing method: Fixed occupations
_M2    smearing method: mp2
_MP    smearing method: Methfessel-Paxton
_MV    smearing method: Marzari-Vanderbilt
_SG    smearing method: Gaussian
```

说明：测试不同电子占据 / smearing 方法。

---

### 5.6 电荷混合方法

示例：

```text
045_PW_BD_chgmix
046_PW_KK_chgmix
047_PW_PK_chgmix
048_PW_PL_chgmix
049_PW_PU_chgmix
```

README 中相关缩写：

```text
_PL    mixing_type plain mixing
_KK    mixing_type kerker mixing
_PU    mixing_type pulay mixing
_PK    mixing_type pulay-kerker mixing
_BD    mixing_type broyden mixing
```

说明：测试不同 SCF 电荷混合策略。

---

### 5.7 输出相关

示例：

```text
052_PW_OB
053_PW_OD
055_PW_OW
056_PW_IW
075_PW_CHG_BINARY
085_PW_get_pchg
086_PW_get_wf
088_PW_get_pchg_sepk
090_PW_VWR
nscf_out_pot
scf_out_chg_tau
scf_out_elf
scf_out_ldos
```

README 中相关缩写：

```text
_OB    output bands file
_OD    output DOS file
_OW    output wave functions
_OC    output charge density
_OK    output kinetic energy density
```

说明：测试能带、DOS、波函数、电荷密度、势函数、ELF、LDOS 等输出。

---

### 5.8 结构优化、晶胞优化与分子动力学

示例：

```text
058_PW_RE_MB
059_PW_RE_MB_traj
060_PW_RE_MG
061_PW_RE_NEW
063_PW_CR
064_PW_CR_fix_a
065_PW_CR_fix_ab
066_PW_CR_fix_abc
067_PW_CR_fix_ac
068_PW_CR_fix_b
069_PW_CR_fix_bc
070_PW_CR_fix_c
071_PW_CR_move
092_PW_MSST
093_PW_MSST2
094_PW_NPT
095_PW_NVT
101_PW_MD_1O
102_PW_MD_2O
```

README 中相关缩写：

```text
_RE    relax calculation
_CR    cell-relax calculation
_MD    molecular dynamics
_MG    move ions method: cg
_MB    move ions method: bfgs
_1O    first-order charge extrapolation
_2O    second-order charge extrapolation
```

说明：测试离子弛豫、晶胞弛豫、固定晶格方向、分子动力学及电荷外推等功能。

---

### 5.9 外场、溶剂、带电体系等物理功能

示例：

```text
076_PW_elec_add
077_PW_elec_minus
078_PW_S2_elec_add
079_PW_S2_elec_minus
080_PW_dipole
081_PW_efield
082_PW_gatefield
083_PW_sol_H2
084_PW_sol_H2O
```

说明：测试加减电子、偶极修正、电场、门电场、隐式溶剂等。

---

### 5.10 杂化泛函、VDW、Wannier90 等高级功能

示例：

```text
091_PW_CR_VDW3
096_PW_PBE0
096_PW_PBE0_AFM
096_PW_PBE0_FM
100_PW_W90
205_PW_SCAN
206_PW_SCAN_S2
209_PW_DFTHALF
```

README 中相关缩写：

```text
_XX    EXX
_VD    VDW
```

说明：测试 PBE0、SCAN、DFT-half、VDW 修正、Wannier90 接口等高级功能。

---

### 5.11 晶格类型测试

示例：

```text
801_PW_LT_sc
802_PW_LT_fcc
803_PW_LT_bcc
804_PW_LT_hex
805_PW_LT_trigonal
806_PW_LT_st
807_PW_LT_bct
808_PW_LT_so
809_PW_LT_baco
810_PW_LT_fco
811_PW_LT_bco
812_PW_LT_sm
813_PW_LT_bacm
814_PW_LT_triclinic
```

说明：测试 PW 下不同晶格类型的结构定义与处理。

---

## 6. 自动测试运行方式中与 PW 相关的部分

根据 `tests/README`，测试运行方式是：

1. 修改：

```text
tests/integrate/general_info
```

设置测试参数，例如并行进程数。

1. 设置自动测试脚本中的 `abacus` 路径。

2. 进入具体测试目录，例如：

```text
tests/01_PW/001_PW_UPF100_Al/
```

1. 运行单个测试：

```bash
./../integrate/Single.sh $parameter
```

其中 `$parameter` 可以是：

```text
空 / debug / ref
```

`ref` 用于重新生成 `result.ref`。

---

## 7. PW 模块结构一句话总结

`tests` 中的 `_PW` 模块主要由 `01_PW/` 和 `11_PW_GPU/` 组成，其中 `01_PW/` 是 CPU 平面波测试主目录，内部按“一个文件夹一个测试用例”的方式组织；每个用例通常包含 `INPUT`、`STRU`、`KPT`、`result.ref`、`README`，覆盖赝势、k 点、对角化、自旋、smearing、电荷混合、输出、结构优化、MD、外场、杂化泛函、晶格类型以及 bug 回归等 PW 功能。

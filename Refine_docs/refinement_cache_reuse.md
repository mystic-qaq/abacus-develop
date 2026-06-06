# 一、`feat/cache-reuse` 优化过程报告

日期：2026-06-06 
分支：`feat/cache-reuse` 
基线：`develop` 
范围：`source/source_basis/module_pw`

## 1. 为什么写这份报告

与其说这是”优化前后性能对比”，不如说它想回答一个问题：**这条分支上的十几个 commit 到底是怎么一步步走到今天的？**

每个提交当时看到了什么问题、上一版哪里还不够、这次改了什么、后来又为什么继续修——把这些串起来，比只看加速比更能看出一次优化是怎么”长出来”的。

最终数据：

| 重复调用路径 | 加速比（中位数） |
|---|---|
| `PW_Basis.collect_local_pw` | ~255.5x |
| `PW_Basis.collect_uniqgg` | ~2284.4x |
| `PW_Basis_K.collect_local_pw` | ~342.7x |
| `PW_Basis_K.collect_local_pw(1.0, 0.5, 0.2)` | ~463.0x |

数值看起来很夸张，但核心逻辑其实很简单：**参数没变就别重复算**。这条分支从头到尾就在做这一件事。

## 2. 四个阶段，一条主线

回看提交历史，优化过程大体可以分成四段：

1. **先让缓存跑起来**——在 `PW_Basis` / `PW_Basis_K` 里把重复构建的数据存下来，验证”能不能复用”这件事本身成立
2. **再从能用变成稳**——处理生命周期、失效条件、对象所有权和测试覆盖，不让缓存”能用但不敢用”
3. **再搭好验证链路**——从单元测试到真实 benchmark，跑脚本、采数据、出报告，让优化效果可量化
4. **最后收掉边界问题**——锁、CI 链接、晶格变化后的误命中，一个不漏

下面按 commit 顺序逐一展开。

## 3. commit历史

### 3.1 `27b8d8102` — 第一版缓存，从 0 到 1

**当时的状态是**：`collect_local_pw()`、`collect_uniqgg()`、`PW_Basis_K::collect_local_pw()` 每次调用都从头分配、重新构建——`gg`、`gdirect`、`gcar`、`ig2igg`、`gg_uniq`、`gk2`，一个不落。但参数没变的时候，这些数据明明可以留着下次直接用的。

**这一版的做法很直接**：

- `PW_Basis` 里加了 `local_pw_cache_valid`、`uniqgg_cache_valid` 和 `invalidate_cache()`——缓存有效时直接返回，不用重建
- `collect_uniqgg()` 还能蹭 `collect_local_pw()` 已经算好的 `gg`，省掉重复劳动
- `PW_Basis_K` 也如法炮制，加 `gk_cache_valid`，`erf_*` 参数没变就跳过重建
- 在初始化路径上补上 `invalidate_cache()`，首批测试验证重复调用后指针和数据都不变

**这步解决的核心问题是**：证明了”重复调用不再重复构建”这个方向是走得通的。第一次把”每次必重建”改成了”命中就不建”。

**没来得及处理的**：命中条件还是粗糙的布尔值；拷贝和对象所有权的风险没动；多线程还没考虑；真实 workload 下的收益也不知道。

---

### 3.2 `3614ad20a` — 让性能可测、可复现

第一版缓存跑起来了，但只能靠单元测试说”嗯，指针没变”。没有真实 workload，就回答不了”到底快了多少”这个问题。

这一步把大量精力放在了 `homework_docs/test_cases/` 上——写脚本、采数据、搭起从真实 case 到计时解析到批量跑数的完整链路。

**这步的意义是**：优化不光要在代码层面成立，还得能在实验层面重复验证。后面的所有决策都建立在这套体系之上。

---

### 3.4 `9f9ab1eba` — benchmark 脚本化

之前 benchmark 多少靠手工跑，偶然性大。这个提交搞了一套 task8 cache benchmark 的采集脚本、README 和汇总工具，让 baseline 和 cache 两组数据可以统一采集、统一比较。它没改 `module_pw` 一行代码，但后续所有”优化是否有效”的证据都来自这里。

---

### 3.5 `faa843d01` — benchmark 数据补齐

在脚本基础上，补齐了 cache-reuse 的跑数结果、日志和汇总文件。这是分支第一次有了比较完整的”缓存版本”跑数记录。

---

### 3.11 `617b03286` — 从”能用”到”可靠”

第一版缓存虽然跑通了，但稍微往深处看，问题不少：

- `PW_Basis` 有自定义拷贝构造，复制一份对象出来，缓存指针可能悬空，所有权也说不清
- `invalidate_cache()` 只把 valid 标志置 false，但公开指针不一定清空
- `get_cache_stats()` 判断缓存字节数不够严格，指针都空了还可能报告”缓存存在”
- `PW_Basis_K::setupIndGk()` 里还有重复扫描，效率没拉满

**这个提交的改法很干脆：**

- 删掉 `PW_Basis` 的拷贝构造和赋值操作，明确说”这对象不该被复制”
- 强化 `invalidate_cache_unlocked()`：`gg`、`gdirect`、`gcar`、`ig2igg`、`gg_uniq`、`ig_gge0` 全都清干净
- `get_cache_stats()` 只有在缓存**确实有效且指针非空**时才计数字节数
- `PW_Basis_K` 里明确了谁是谁的所有者——`k_gcar_cache_storage` / `k_gk2_cache_storage` 就是公开 `gcar` / `gk2` 的持有者，缓存无效时 `get_gcar_data()` / `get_gk2_data()` 返回 `nullptr`
- `setupIndGk()` 复用已选中的 `ig` 列表，减少重复 cutoff 扫描
- 顺便修了 `test-big.cpp` 里一个危险的切片写法

**这一步做完了，缓存就不只是”布尔值层面的命中”了**——对象所有权清楚了，失效后状态干净了，接口返回可靠了。

还没收尾的：并发锁保护还需要继续收紧；`setupIndGk()` 的优化后来部分被回退过；晶格变化后缓存该不该命中，判断还不够严格。

---

### 3.13 `4e79b28bd` — 去掉实验痕迹

之前为了看缓存行为，在 `collect_local_pw()`、`collect_uniqgg()`、`setupIndGk()` 等地方加了额外的 timer。实验归实验，长期留在代码里不合适。

这个提交把新增的 timer 全移掉了——不是说计时没用，而是实验性埋点不应该带进主逻辑。

---

### 3.14 `913a31504` — 给缓存加把锁

**之前的做法有个竞态风险**：判断缓存是否命中的逻辑有一部分在加锁之前——先看一眼 `cache_valid`，再决定要不要进锁。另一个线程可能就在这一眼和一锁之间把缓存状态改了。

读接口也一样：`get_cache_stats()`、`get_gcar_data()`、`get_gk2_data()` 不加锁也可能读到不一致的状态。

**这个提交的做法：**

- `invalidate_cache()` 变成：上锁 → 调 `invalidate_cache_unlocked()`
- 新增 `invalidate_cache_unlocked()` 和 `get_cache_stats_unlocked()` 两个内部接口
- 读写路径全部纳入锁保护，去掉了之前的”乐观快速返回”
- `collect_local_pw()` 重建时，不仅要把自己的数据清掉，连 `ig2igg` 和 `gg_uniq` 的缓存 storage 也一并清空

**这步解决的核心问题是**：让”缓存有效位”和”缓存本体”真正同步，不再有”valid 说有效但数据是旧的”这种半吊子状态。

唯一没堵上的口子是：如果晶格矩阵被直接改掉（不走 `invalidate_cache()`），缓存仍然可能误命中——这留给了下一步。

---

### 3.15 `8c622c2db` — 不让 CI 卡住

有些虚函数实现写在 `.cpp` 里，在某些链接场景下可能让虚表或符号解析出问题。这个提交把 `invalidate_cache()` 和 `invalidate_cache_unlocked()` 的默认实现挪到头文件里内联定义，`PW_Basis_K` 的版本也一样处理。

不是性能修复，但很重要——优化做得再好，CI 过不了也合不进主线。

---

### 3.16 `4faaadcb1` — 最后一块拼图：什么时候绝不能复用

前面的缓存命中条件说到底就两样：`cache_valid` + 部分参数检查。但如果晶格、倒格矢矩阵、FFT 网格在对象内部变了——没走显式失效的话——旧缓存就可能被静默复用。

这类 bug 最危险：它不崩、不报错、编译也通过，只是默默地给你一个不对的结果。

**这个提交的做法是在 `PW_Basis` 里引入 `CacheSignature`：**

- 把 `lat0`、`tpiba`、`tpiba2`、`nx/ny/nz`、`fftnx/fftny/fftnz`、`npw`、`G/GT/GGT` 这些真正决定缓存内容的状态打包成一个签名
- 每次命中缓存前，不光看 `cache_valid`，还要拿当前状态和签名比对一遍
- 签名不匹配？老老实实重建，然后记下新的签名

对应的测试也很直白：先命中一次，然后故意改掉晶格矩阵，验证缓存确实 miss、结果也确实变了。

> 如果说前面的提交解决的是”缓存怎么复用”，那这一提交解决的就是”什么时候绝不能复用”。性能再好，跑出错误结果也毫无意义。

## 4. 回过头看这条问题收敛链

如果把整条分支看作”问题一步步暴露、一步步补上”的过程，路线大致是这样的：

1. **先看到重复构建很浪费** → 做了第一版 `cache_valid` 复用
2. **然后发现”能复用”不等于”工程上安全”** → 处理失效路径、指针清空、拷贝语义、测试覆盖
3. **再发现只看单测不够** → 搭 benchmark 脚本、baseline、汇总流程
5. **接着暴露出锁问题** → 读写全上锁，storage 和 valid 位同步失效
6. **最后发现晶格变化后会误命中** → 引入 `CacheSignature`，命中条件从布尔值升级为签名匹配

每一步的触发因素都是前一步的”还不够”，而不是一开始就规划好的。这大概就是所谓”工程化迭代”的样子——先抓住主要瓶颈，再补边界，再补验证，最后收敛正确性。

## 5. 每一步值在哪

### 5.1 第一版缓存（`27b8d8102`）为什么值钱

它证明了方向是对的。`collect_local_pw`、`collect_uniqgg`、`PW_Basis_K::collect_local_pw` 这些函数确实是可缓存的，而不是”看起来像热点，实际上不能复用”。没有这一步，后面所有锁、签名、测试和基准都无从谈起。

### 5.2 中间几轮工程打磨为什么必不可少

`617b03286`、`913a31504`、`8c622c2db` 这几提交看起来不像加速比那么”显眼”，但少了哪一步都危险：

- 不清空 storage → valid 位失效了但旧指针还在，半死不活的状态最坑
- 不处理拷贝语义 → 缓存所有权一锅粥
- 不把判断放进锁 → 命中到不一致状态
- CI 链接不过 → 优化再好也进不了主线

这些提交把”优化代码”变成了”可维护、可测试、可集成的优化代码”。

### 5.3 最后一个正确性补丁为什么最关键

`4faaadcb1` 堵住的是最危险的一类 bug——不是崩溃，不是编译不过，而是**静默地复用了错的缓存**。没有 signature 机制，性能数字再好看，结果也不可信。这一步是在给整条优化链”盖章”：它不只是快，而且应该是对的。

## 6. benchmark 数据怎么说

这次 micro-benchmark 的结果和分支上的演进逻辑是对得上的：

### 6.1 `PW_Basis.collect_local_pw`

| 版本 | 中位数耗时 |
|---|---|
| `develop` | 0.0882 s |
| `feat/cache-reuse` | 0.0003 s |
| **加速比** | **~255.5x** |

本地 PW 数据不再重复构建，直接命中缓存，效果立竿见影。

### 6.2 `PW_Basis.collect_uniqgg`

| 版本 | 中位数耗时 |
|---|---|
| `develop` | 0.7546 s |
| `feat/cache-reuse` | 0.0003 s |
| **加速比** | **~2284.4x** |

不需要再每次排序、去重，稳定命中缓存。这也是整条分支收益最大的单点。

### 6.3 `PW_Basis_K.collect_local_pw`

| 版本 | 中位数耗时 |
|---|---|
| `develop` | 0.1091 s |
| `feat/cache-reuse` | 0.0003 s |
| **加速比** | **~342.7x** |

K 点路径上的 `gcar` / `gk2` 缓存确实成立。

### 6.4 `PW_Basis_K.collect_local_pw(1.0, 0.5, 0.2)`

| 版本 | 中位数耗时 |
|---|---|
| `develop` | 0.1930 s |
| `feat/cache-reuse` | 0.0004 s |
| **加速比** | **~463.0x** |

这项结果很有意思——`erf` 参数变了，`gk2` 确实得重建，但 **`gcar` 还在缓存里**。这正是缓存粒度做细之后的收益：不是”全有或全无”，而是该失效的失效、该复用的复用。

## 7. 总体评价

回过头看 `feat/cache-reuse` 这条分支，它不是一个”一次写完大改”的提交，而是一个典型的工程化迭代过程：

1. 从热点识别出发，先建立缓存复用能力
2. 再花功夫解决生命周期、失效和所有权问题
3. 再搭 benchmark、baseline 和文档链路
4. 然后去掉实验性噪音，让实现收敛
5. 最后补上锁一致性和晶格签名校验，完成正确性闭环

最终结果体现为：`module_pw` 的重复调用路径从”每次都重建”变成了”首轮构建，后续命中”，性能收益显著，且状态变化时不会误命中旧缓存。

但这条分支的价值不只是”跑得更快”——说起来可能是这四件事：

- **知道哪里可以缓存**
- **知道什么时候必须失效**
- **知道怎样证明它真的更快**
- **知道怎样保证它快得是对的**

## 8. 总结

1. **这次优化的核心不是改 FFT 本身，而是让 `module_pw` 在参数没变的时候别重复算**——`gg`、`gdirect`、`gcar`、`ig2igg`、`gg_uniq`、`gk2`，该省的全省掉。
2. **优化过程不是一次拍脑袋写完的，而是”先做复用、再补失效、再补锁、最后补状态签名”，一步步收敛的。**
3. **最终收益集中在重复调用路径，`PW_Basis.collect_uniqgg` 中位数提升约 2284 倍——说明缓存复用确实命中了真正的热点。**



# 二、module_pw 缓存复用性能对比

日期：2026-06-06  
对比分支：`develop` vs `feat/cache-reuse`  
对比范围：`source/source_basis/module_pw`

## 1. 本次补充的计时点

为了定位缓存复用收益，本次在 `module_pw` 内部补了最小必要的 timer：

- `PW_Basis::collect_local_pw`
- `PW_Basis::collect_local_pw_cache_hit`
- `PW_Basis::collect_local_pw_cache_build`
- `PW_Basis::collect_uniqgg`
- `PW_Basis::collect_uniqgg_cache_hit`
- `PW_Basis::collect_uniqgg_cache_build`
- `PW_Basis_K::collect_local_pw`
- `PW_Basis_K::collect_local_pw_cache_hit`
- `PW_Basis_K::collect_local_pw_build_gcar`
- `PW_Basis_K::collect_local_pw_build_gk2`

说明：

- `feat/cache-reuse` 上保留了命中/构建分支的细分 timer。
- `develop` 基线 worktree 只补了等价的“构建路径” timer，用来做公平基准，不包含缓存实现本身。

## 2. 基准方法

### 2.1 构建方式

为避免 MPI 环境对 `timer.cpp` 的影响，最终性能数据使用串行构建：

```bash
cmake -S /home/aunixt/abacus-develop -B /home/aunixt/abacus-develop/build-bench-serial \
  -DENABLE_MPI=OFF -DUSE_OPENMP=OFF -DUSE_ELPA=OFF -DBUILD_TESTING=OFF
cmake --build /home/aunixt/abacus-develop/build-bench-serial --target MODULE_PW_cache_bench
```

`develop` 基线在独立 worktree 中执行同样流程：

```bash
git -C /home/aunixt/abacus-develop worktree add /home/aunixt/abacus-develop-develop develop
cmake -S /home/aunixt/abacus-develop-develop -B /home/aunixt/abacus-develop-develop/build-bench-serial \
  -DENABLE_MPI=OFF -DUSE_OPENMP=OFF -DUSE_ELPA=OFF -DBUILD_TESTING=OFF
cmake --build /home/aunixt/abacus-develop-develop/build-bench-serial --target MODULE_PW_cache_bench
```

### 2.2 基准程序

新增基准程序：`MODULE_PW_cache_bench`

测试内容：

- `PW_Basis.setuptransform`
- `PW_Basis.collect_local_pw` 首次调用
- `PW_Basis.collect_local_pw` 重复 2000 次
- `PW_Basis.collect_uniqgg` 首次调用
- `PW_Basis.collect_uniqgg` 重复 2000 次
- `PW_Basis_K.setuptransform`
- `PW_Basis_K.collect_local_pw` 首次调用
- `PW_Basis_K.collect_local_pw` 重复 2000 次
- `PW_Basis_K.collect_local_pw(1.0, 0.5, 0.2)` 重复 2000 次

统计口径：

- 外层 wall time：基准程序直接测量
- 内层 timer：`ModuleBase::timer` 累积结果
- 每个分支各跑 3 次，结论使用中位数，避免单次抖动

## 3. 结果汇总

### 3.1 关键结论

1. `setuptransform` 基本无变化，说明优化没有破坏初始化主路径。
2. `PW_Basis.collect_local_pw` 的重复调用从持续重建，变成几乎纯命中路径，中位数加速约 `255.5x`。
3. `PW_Basis.collect_uniqgg` 的重复调用收益最大，中位数加速约 `2284.4x`。
4. `PW_Basis_K.collect_local_pw` 的重复调用中位数加速约 `342.7x`。
5. 即使传入新 `erf` 参数导致 `gk2` 需要重建，`PW_Basis_K` 仍然复用了 `gcar`，该路径中位数加速约 `463.0x`。

### 3.2 中位数对比表

单位：秒

| 场景                                           |     develop | feat/cache-reuse |     提升 |
| ---------------------------------------------- | ----------: | ---------------: | -------: |
| `PW_Basis.setuptransform.wall`                 | 0.001487681 |      0.001502684 |    0.99x |
| `PW_Basis.collect_local_pw.first.wall`         | 0.000117131 |      0.000124510 |    0.94x |
| `PW_Basis.collect_local_pw.repeat.wall`        | 0.088178164 |      0.000345102 |  255.52x |
| `PW_Basis.collect_uniqgg.first.wall`           | 0.000440547 |      0.000401324 |    1.10x |
| `PW_Basis.collect_uniqgg.repeat.wall`          | 0.754634649 |      0.000330313 | 2284.40x |
| `PW_Basis_K.setuptransform.wall`               | 0.000259432 |      0.000217700 |    1.19x |
| `PW_Basis_K.collect_local_pw.first.wall`       | 0.000134854 |      0.000121473 |    1.11x |
| `PW_Basis_K.collect_local_pw.repeat.wall`      | 0.109138201 |      0.000318489 |  342.68x |
| `PW_Basis_K.collect_local_pw.gk2_rebuild.wall` | 0.193014060 |      0.000416850 |  463.04x |

## 4. 原始样本

### 4.1 feat/cache-reuse

#### Run 1

| 指标                                           |        数值 |
| ---------------------------------------------- | ----------: |
| `PW_Basis.setuptransform.wall`                 | 0.001464156 |
| `PW_Basis.collect_local_pw.repeat.wall`        | 0.000462526 |
| `PW_Basis.collect_uniqgg.repeat.wall`          | 0.000362283 |
| `PW_Basis_K.collect_local_pw.repeat.wall`      | 0.000318489 |
| `PW_Basis_K.collect_local_pw.gk2_rebuild.wall` | 0.000416850 |

#### Run 2

| 指标                                           |        数值 |
| ---------------------------------------------- | ----------: |
| `PW_Basis.setuptransform.wall`                 | 0.001733625 |
| `PW_Basis.collect_local_pw.repeat.wall`        | 0.000345102 |
| `PW_Basis.collect_uniqgg.repeat.wall`          | 0.000330313 |
| `PW_Basis_K.collect_local_pw.repeat.wall`      | 0.000373748 |
| `PW_Basis_K.collect_local_pw.gk2_rebuild.wall` | 0.000529150 |

#### Run 3

| 指标                                           |        数值 |
| ---------------------------------------------- | ----------: |
| `PW_Basis.setuptransform.wall`                 | 0.001502684 |
| `PW_Basis.collect_local_pw.repeat.wall`        | 0.000314556 |
| `PW_Basis.collect_uniqgg.repeat.wall`          | 0.000287104 |
| `PW_Basis_K.collect_local_pw.repeat.wall`      | 0.000317902 |
| `PW_Basis_K.collect_local_pw.gk2_rebuild.wall` | 0.000416157 |

### 4.2 develop

#### Run 1

| 指标                                           |        数值 |
| ---------------------------------------------- | ----------: |
| `PW_Basis.setuptransform.wall`                 | 0.001473400 |
| `PW_Basis.collect_local_pw.repeat.wall`        | 0.086802023 |
| `PW_Basis.collect_uniqgg.repeat.wall`          | 0.754634649 |
| `PW_Basis_K.collect_local_pw.repeat.wall`      | 0.109138201 |
| `PW_Basis_K.collect_local_pw.gk2_rebuild.wall` | 0.193014060 |

#### Run 2

| 指标                                           |        数值 |
| ---------------------------------------------- | ----------: |
| `PW_Basis.setuptransform.wall`                 | 0.001487681 |
| `PW_Basis.collect_local_pw.repeat.wall`        | 0.088178164 |
| `PW_Basis.collect_uniqgg.repeat.wall`          | 0.745032054 |
| `PW_Basis_K.collect_local_pw.repeat.wall`      | 0.104775062 |
| `PW_Basis_K.collect_local_pw.gk2_rebuild.wall` | 0.176847280 |

#### Run 3

| 指标                                           |        数值 |
| ---------------------------------------------- | ----------: |
| `PW_Basis.setuptransform.wall`                 | 0.001664659 |
| `PW_Basis.collect_local_pw.repeat.wall`        | 0.105299161 |
| `PW_Basis.collect_uniqgg.repeat.wall`          | 0.965178448 |
| `PW_Basis_K.collect_local_pw.repeat.wall`      | 0.134283688 |
| `PW_Basis_K.collect_local_pw.gk2_rebuild.wall` | 0.266846180 |

## 5. 内部 timer 观察

### 5.1 feat/cache-reuse

代表性现象：

- `timer.PW_Basis.collect_local_pw_cache_hit.calls = 2000`
- `timer.PW_Basis.collect_uniqgg_cache_hit.calls = 2000`
- `timer.PW_Basis_K.collect_local_pw_cache_hit.calls = 3999`
- `timer.PW_Basis_K.collect_local_pw_build_gcar.calls = 1`
- `timer.PW_Basis_K.collect_local_pw_build_gk2.calls = 2`

解释：

- `PW_Basis` 的两条缓存路径在首轮构建后，后续 2000 次全部命中。
- `PW_Basis_K` 在默认参数重复调用时，只有首轮需要构建。
- 改变 `erf` 参数后，`gk2` 需要重新构建，但 `gcar` 仍然保持复用。

### 5.2 develop

代表性现象：

- `timer.PW_Basis.collect_local_pw_cache_build.calls = 2001`
- `timer.PW_Basis.collect_uniqgg_cache_build.calls = 2001`
- `timer.PW_Basis_K.collect_local_pw_build_gcar.calls = 4001`
- `timer.PW_Basis_K.collect_local_pw_build_gk2.calls = 4001`

解释：

- 基线分支每次调用都走完整构建，没有任何命中路径。
- 这与重复调用 wall time 的数量级差异完全一致。

## 6. 验证记录

在 `feat/cache-reuse` 上新增并通过的回归测试：

- `PWBasisTEST.CacheCollectionRecordsTimers`
- `PWBasisKTEST.CollectLocalPWRecordsTimers`

测试命令：

```bash
cd /home/aunixt/abacus-develop/build-tests-mpi/source/source_basis/module_pw/test_serial
./MODULE_PW_basis_pw_serial --gtest_filter=PWBasisTEST.CacheCollectionRecordsTimers
./MODULE_PW_basis_pw_k_serial --gtest_filter=PWBasisKTEST.CollectLocalPWRecordsTimers
```

结果：两项均通过。

## 7. 最终结论

`feat/cache-reuse` 在 `module_pw` 中的缓存复用优化是有效且收益非常显著的，主要收益集中在重复调用路径：

- `PW_Basis.collect_local_pw`
- `PW_Basis.collect_uniqgg`
- `PW_Basis_K.collect_local_pw`

其中最关键的是：

- `develop` 重复调用仍然持续分配并重建数据。
- `feat/cache-reuse` 已经将这部分开销压缩到首轮构建，后续主要变成 cache hit。
- `PW_Basis_K` 在参数部分变化时还能保留 `gcar` 复用，说明缓存粒度设计是合理的。

如果后续需要，我建议直接把这份基准保留在仓库里，后面可以继续扩展成 CI 可重复的 micro-benchmark。



## 三、附录：AI使用报告

由于之前每次让AI改代码总是不太理想，也总是遗漏一些问题，因此进一步探索了vibe coding技巧，发现可以在claude code和codex上安装superpowers skills提升AI编程的纪律性：遵循TDD规范、测试时创建Git Worktree等等，提升了coding效率。

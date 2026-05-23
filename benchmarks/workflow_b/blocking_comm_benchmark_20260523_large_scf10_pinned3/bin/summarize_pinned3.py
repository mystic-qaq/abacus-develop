#!/usr/bin/env python3
from __future__ import annotations

import csv
import re
from pathlib import Path


ROOT = Path("/home/yangxu/abacus-develop/benchmarks/workflow_b/blocking_comm_benchmark_20260523_large_scf10_pinned3")
CASES = {
    "nacl_uspp": "NaCl USPP",
    "hcl_uspp": "HCl USPP",
    "si_blps": "Si BLPS",
}
CASE_DIRS = {
    "nacl_uspp": ROOT / "runs/performance/nacl_uspp",
    "hcl_uspp": ROOT / "runs/performance/hcl_uspp",
    "si_blps": ROOT / "runs/performance/si_blps",
}
CASE_LOGS = {
    key: path / "large_rep01_np8_omp2/OUT.workflowB_large_r01_np8_omp2/running_scf_1.log"
    for key, path in CASE_DIRS.items()
}
CONFIG_ORDER = ["1x1", "2x1", "4x1", "8x1", "4x2", "8x2"]
NONBASE_CONFIG_ORDER = ["2x1", "4x1", "8x1", "4x2", "8x2"]
FORMAL_GUARD_DIR = ROOT / "runs/correctness_tight_e13"
E14_ATTEMPT_DIR = ROOT / "runs/correctness_strict"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open() as handle:
        return list(csv.DictReader(handle))


def write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("")
        return
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def parse_input(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text().splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        parts = stripped.split()
        if len(parts) >= 2:
            values[parts[0]] = parts[1]
    return values


def parse_atom_count(path: Path) -> int:
    lines = [line.strip() for line in path.read_text().splitlines()]
    try:
        idx = lines.index("ATOMIC_POSITIONS") + 2
    except ValueError:
        return 0
    total = 0
    while idx < len(lines):
        if not lines[idx]:
            idx += 1
            continue
        if idx + 2 >= len(lines):
            break
        try:
            count = int(float(lines[idx + 2].split()[0]))
        except (ValueError, IndexError):
            break
        total += count
        idx += 3 + count
    return total


def parse_kpt(path: Path) -> str:
    lines = [line.strip() for line in path.read_text().splitlines() if line.strip()]
    if len(lines) >= 4:
        mesh = lines[3].split()
        if len(mesh) >= 3:
            mode = lines[2]
            return f"{mesh[0]} {mesh[1]} {mesh[2]} {mode}"
    return ""


def first_match(pattern: str, text: str, default: str = "") -> str:
    match = re.search(pattern, text, re.S)
    return match.group(1) if match else default


def parse_grid_info(log_path: Path) -> dict[str, object]:
    text = log_path.read_text(errors="ignore")
    wave_grid = first_match(r"FFT grid for wave functions = \[\s*([0-9]+,\s*[0-9]+,\s*[0-9]+)\s*\]", text)
    dense_grid = first_match(r"fft grid for dense charge/potential = \[\s*([0-9]+,\s*[0-9]+,\s*[0-9]+)\s*\]", text)
    wave_sticks = first_match(
        r"SETUP PLANE WAVES FOR WAVE FUNCTIONS.*?Number of sticks on FFT x-y plane =\s*([0-9]+)",
        text,
    )
    dense_sticks = first_match(
        r"SETUP PLANE WAVES FOR DENSE CHARGE/POTENTIAL.*?Number of sticks =\s*([0-9]+)",
        text,
    )
    plane_waves = first_match(r"Number of total plane waves =\s*([0-9]+)", text)
    return {
        "wave_fft_grid": wave_grid.replace(",", " x"),
        "dense_fft_grid": dense_grid.replace(",", " x"),
        "wave_sticks": int(wave_sticks) if wave_sticks else "",
        "dense_sticks": int(dense_sticks) if dense_sticks else "",
        "plane_waves": int(plane_waves) if plane_waves else "",
    }


def system_rows() -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for key, label in CASES.items():
        prepared = ROOT / "prepared_cases" / key
        inputs = parse_input(prepared / "INPUT")
        row: dict[str, object] = {
            "case": label,
            "atoms": parse_atom_count(prepared / "STRU"),
            "ecutwfc": inputs.get("ecutwfc", ""),
            "ecutrho": inputs.get("ecutrho", ""),
            "nbands": inputs.get("nbands", ""),
            "k_points": parse_kpt(prepared / "KPT"),
        }
        row.update(parse_grid_info(CASE_LOGS[key]))
        rows.append(row)
    return rows


def wall_rows() -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for key, label in CASES.items():
        summary = [
            row for row in read_csv(CASE_DIRS[key] / "pw_comm_benchmark_summary.csv")
            if row["class_name"] == "PW_Basis_K"
        ]
        by_cfg = {f"{row['nproc']}x{row['omp']}": row for row in summary}
        baseline = float(by_cfg["1x1"]["wall_s_mean"])
        for cfg in CONFIG_ORDER:
            row = by_cfg[cfg]
            wall = float(row["wall_s_mean"])
            stdev = float(row["wall_s_stdev"])
            rows.append(
                {
                    "case": label,
                    "config": cfg,
                    "wall_s_mean": wall,
                    "wall_s_stdev": stdev,
                    "wall_cv_percent": stdev / wall * 100.0 if wall else 0.0,
                    "speedup_vs_1x1": baseline / wall if wall else 0.0,
                }
            )
    return rows


def comm_by_class_rows() -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for key, label in CASES.items():
        summary = read_csv(CASE_DIRS[key] / "pw_comm_benchmark_summary.csv")
        for row in summary:
            cfg = f"{row['nproc']}x{row['omp']}"
            if cfg == "1x1":
                continue
            rows.append(
                {
                    "case": label,
                    "config": cfg,
                    "class_name": row["class_name"],
                    "wall_s_mean": float(row["wall_s_mean"]),
                    "comm_critical_s_mean": float(row["comm_critical_s_mean"]),
                    "comm_critical_s_stdev": float(row["comm_critical_s_stdev"]),
                    "wait_proxy_max_s_mean": float(row["wait_proxy_max_s_mean"]),
                    "wait_proxy_max_s_stdev": float(row["wait_proxy_max_s_stdev"]),
                    "overlap_candidate_rank_avg_s_mean": float(row["overlap_candidate_rank_avg_s_mean"]),
                    "overlap_candidate_rank_avg_s_stdev": float(row["overlap_candidate_rank_avg_s_stdev"]),
                    "gatherp_comm_critical_s_mean": float(row["gatherp_comm_critical_s_mean"]),
                    "gathers_comm_critical_s_mean": float(row["gathers_comm_critical_s_mean"]),
                }
            )
    rows.sort(key=lambda row: (list(CASES.values()).index(row["case"]), CONFIG_ORDER.index(row["config"]), row["class_name"]))
    return rows


def comm_combined_rows(by_class: list[dict[str, object]]) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str], list[dict[str, object]]] = {}
    for row in by_class:
        grouped.setdefault((str(row["case"]), str(row["config"])), []).append(row)

    rows: list[dict[str, object]] = []
    for case in CASES.values():
        for cfg in NONBASE_CONFIG_ORDER:
            parts = grouped[(case, cfg)]
            wall = float(parts[0]["wall_s_mean"])
            comm = sum(float(row["comm_critical_s_mean"]) for row in parts)
            wait = sum(float(row["wait_proxy_max_s_mean"]) for row in parts)
            overlap = sum(float(row["overlap_candidate_rank_avg_s_mean"]) for row in parts)
            rows.append(
                {
                    "case": case,
                    "config": cfg,
                    "wall_s_mean": wall,
                    "comm_critical_sum_s": comm,
                    "comm_fraction_percent": comm / wall * 100.0 if wall else 0.0,
                    "wait_proxy_sum_s": wait,
                    "overlap_candidate_sum_s": overlap,
                }
            )
    return rows


def read_real_seconds(stderr_path: Path) -> float:
    for line in stderr_path.read_text(errors="ignore").splitlines():
        match = re.match(r"^real\s+([-+0-9.eE]+)$", line.strip())
        if match:
            return float(match.group(1))
    return 0.0


def parse_guard_run(case_key: str, suite_dir: Path, cfg: str, scf_thr: str) -> dict[str, object]:
    label = CASES[case_key]
    case_dir = suite_dir / case_key / f"large_rep01_{cfg}"
    stdout = case_dir / "stdout.log"
    out_dir = case_dir / f"OUT.workflowB_large_r01_{cfg.replace('_', '_')}"
    logs = sorted(out_dir.glob("running_scf_*.log")) if out_dir.exists() else []
    text = "\n".join(log.read_text(errors="ignore") for log in logs)
    stdout_text = stdout.read_text(errors="ignore") if stdout.exists() else ""
    cg_lines = re.findall(
        r"^\s*CG([0-9]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)",
        stdout_text,
        re.M,
    )
    final_energy = first_match(r"!FINAL_ETOT_IS\s+([-+0-9.eE]+)\s+eV", text)
    if "#SCF IS CONVERGED#" in text:
        status = "converged"
    elif "psi_norm <= 0.0" in text or "psi_norm <= 0.0" in stdout_text:
        status = "failed_psi_norm"
    elif "!!SCF IS NOT CONVERGED!!" in text:
        status = "not_converged"
    elif case_dir.exists():
        status = "failed"
    else:
        status = "missing"
    cfg_match = re.match(r"np([0-9]+)_omp([0-9]+)", cfg)
    cfg_label = f"{cfg_match.group(1)}x{cfg_match.group(2)}" if cfg_match else cfg
    return {
        "case": label,
        "config": cfg_label,
        "scf_nmax": 200,
        "scf_thr": scf_thr,
        "status": status,
        "scf_iterations": int(cg_lines[-1][0]) if cg_lines else "",
        "final_drho": float(cg_lines[-1][3]) if cg_lines else "",
        "final_etot_ev": float(final_energy) if final_energy else "",
        "wall_s": read_real_seconds(case_dir / "stderr.log") if case_dir.exists() else "",
        "case_dir": str(case_dir) if case_dir.exists() else "",
    }


def correctness_rows() -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for key in CASES:
        for cfg in ["np1_omp1", "np8_omp2"]:
            rows.append(parse_guard_run(key, FORMAL_GUARD_DIR, cfg, "1e-13"))
    return rows


def correctness_comparison_rows(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    by_case: dict[str, dict[str, dict[str, object]]] = {}
    for row in rows:
        by_case.setdefault(str(row["case"]), {})[str(row["config"])] = row
    out: list[dict[str, object]] = []
    for case in CASES.values():
        lhs = by_case.get(case, {}).get("1x1")
        rhs = by_case.get(case, {}).get("8x2")
        if not lhs or not rhs:
            continue
        lhs_e = lhs["final_etot_ev"]
        rhs_e = rhs["final_etot_ev"]
        diff = abs(float(lhs_e) - float(rhs_e)) if isinstance(lhs_e, float) and isinstance(rhs_e, float) else ""
        out.append(
            {
                "case": case,
                "reference_config": "1x1",
                "comparison_config": "8x2",
                "reference_etot_ev": lhs_e,
                "comparison_etot_ev": rhs_e,
                "abs_etot_diff_ev": diff,
                "status": "pass" if lhs["status"] == "converged" and rhs["status"] == "converged" else "fail",
            }
        )
    return out


def e14_attempt_rows() -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for key in CASES:
        for cfg in ["np1_omp1", "np8_omp2"]:
            case_dir = E14_ATTEMPT_DIR / key / f"large_rep01_{cfg}"
            if case_dir.exists():
                rows.append(parse_guard_run(key, E14_ATTEMPT_DIR, cfg, "1e-14"))
    return rows


def fmt(value: object, spec: str = ".3f") -> str:
    if isinstance(value, float):
        return format(value, spec)
    return str(value)


def md_table(rows: list[dict[str, object]], columns: list[tuple[str, str, str | None]]) -> list[str]:
    lines = [
        "| " + " | ".join(label for _, label, _ in columns) + " |",
        "| " + " | ".join("---" for _ in columns) + " |",
    ]
    for row in rows:
        cells = []
        for key, _, spec in columns:
            value = row.get(key, "")
            cells.append(fmt(value, spec) if spec else str(value))
        lines.append("| " + " | ".join(cells) + " |")
    return lines


def pivot_wall_table(rows: list[dict[str, object]]) -> list[str]:
    lines = [
        "| 体系 | `1x1` | `2x1` | `4x1` | `8x1` | `4x2` | `8x2` |",
        "| --- | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    by_case_cfg = {(str(row["case"]), str(row["config"])): row for row in rows}
    for case in CASES.values():
        cells = [case]
        for cfg in CONFIG_ORDER:
            row = by_case_cfg[(case, cfg)]
            cells.append(f"{float(row['wall_s_mean']):.3f}s ({float(row['speedup_vs_1x1']):.2f}x)")
        lines.append("| " + " | ".join(cells) + " |")
    return lines


def class_pair_rows(rows: list[dict[str, object]], metric: str) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str], dict[str, dict[str, object]]] = {}
    for row in rows:
        grouped.setdefault((str(row["case"]), str(row["config"])), {})[str(row["class_name"])] = row
    out: list[dict[str, object]] = []
    for case in CASES.values():
        for cfg in NONBASE_CONFIG_ORDER:
            pair = grouped[(case, cfg)]
            out.append(
                {
                    "case": case,
                    "config": cfg,
                    "pw_basis_k": float(pair["PW_Basis_K"][metric]),
                    "pw_basis_sup": float(pair["PW_Basis_Sup"][metric]),
                }
            )
    return out


def write_report(
    systems: list[dict[str, object]],
    walls: list[dict[str, object]],
    comm_class: list[dict[str, object]],
    comm_combined: list[dict[str, object]],
    correctness: list[dict[str, object]],
    correctness_compare: list[dict[str, object]],
    e14_attempts: list[dict[str, object]],
) -> None:
    hot = sorted(comm_combined, key=lambda row: float(row["comm_critical_sum_s"]), reverse=True)[:10]
    report = ROOT / "reports" / "工作流B_阻塞通信large_scf10_pinned3报告.md"
    report.parent.mkdir(parents=True, exist_ok=True)
    lines: list[str] = [
        "# 工作流 B 阻塞通信 large/scf10 Benchmark 报告",
        "",
        "日期：2026-05-23",
        "",
        "## 结论摘要",
        "",
        "本次重跑采用 `large: ecutwfc=50 Ry, ecutrho=2000 Ry`，保留 6 组 MPI/OpenMP 配置 `1x1 2x1 4x1 8x1 4x2 8x2`，并对 NaCl USPP、HCl USPP、Si BLPS 三个体系各重复 3 次。性能测试使用 `scf_nmax=10` 和极小 `scf_thr=1e-30` 固定工作量；正式 correctness guard 已改为 `scf_nmax=200, scf_thr=1e-13`，并用 `1x1` 与 `8x2` 两个配置检查同参数结果一致性。",
        "",
        "说明：原先 `scf_thr=1e-9` 只适合作为 sanity check，不足以支撑严格 correctness 论证。本次额外尝试了 `scf_thr=1e-14`：NaCl USPP 可以收敛，但 HCl USPP 与 Si BLPS 在接近 `10^-14` 的 DRHO 后继续迭代触发 `psi_norm <= 0.0`，因此不把 `1e-14` 作为三体系正式 guard。正式报告保留该失败记录，而不是把失败包装成通过。",
        "",
        "前一次异常的 `np8` 退化不是 ABACUS large 配置本身的问题，而是 OpenMPI 默认绑核把多个 rank 挤到少数 CPU core 上导致的调度污染。本次正式目录使用 `mpirun --map-by slot:PE=${OMP_NUM_THREADS} --bind-to core`，并在 `stderr.log` 中保留 `--report-bindings` 记录，数据可以作为后续非阻塞通信优化的 baseline。",
        "",
        "## 运行目录与产物",
        "",
        f"- 正式目录：`{ROOT}`",
        "- 运行脚本：`run_large_scf10_pinned3.sh`",
        "- MPI 包装器：`bin/mpirun_pe.sh`",
        "- 汇总表：`tables/performance_wall_summary.csv`、`tables/comm_by_class_summary.csv`、`tables/comm_combined_summary.csv`、`tables/correctness_guard_summary.csv`、`tables/correctness_guard_comparison.csv`、`tables/correctness_e14_attempts.csv`",
        "- 原始日志：`runs/performance/<case>/...`、`runs/correctness_tight_e13/<case>/...` 与 `runs/correctness_strict/<case>/...`",
        "",
        "## 测试环境",
        "",
        "| 项目 | 内容 |",
        "| --- | --- |",
        "| Git commit | `76225d5cf` |",
        "| ABACUS | `build-current-abacus-mpi-local/abacus_pw_para` |",
        "| MPI | OpenMPI `4.0.3`，`/usr/bin/mpirun.openmpi` |",
        "| CPU | AMD EPYC 7H12，2 sockets，64 cores/socket，256 logical CPUs |",
        "| 绑核策略 | `--map-by slot:PE=${OMP_NUM_THREADS} --bind-to core` |",
        "| OpenMP | `OMP_PLACES=cores`, `OMP_PROC_BIND=spread` |",
        "",
        "## 测试体系与实际网格",
        "",
    ]
    lines.extend(
        md_table(
            systems,
            [
                ("case", "体系", None),
                ("atoms", "原子数", None),
                ("ecutwfc", "`ecutwfc`", None),
                ("ecutrho", "`ecutrho`", None),
                ("nbands", "`nbands`", None),
                ("k_points", "K 点", None),
                ("wave_fft_grid", "wave FFT grid", None),
                ("dense_fft_grid", "dense/Sup FFT grid", None),
                ("wave_sticks", "wave sticks", None),
                ("dense_sticks", "dense/Sup sticks", None),
            ],
        )
    )
    lines.extend(
        [
            "",
            "说明：`ecutrho=2000` 显式施加在输入文件中；ABACUS 日志同时给出普通 charge grid 和 dense/Sup grid，本表采用与 `PW_Basis_Sup` 通信更直接相关的 dense grid/sticks。",
            "",
            "## 指标定义",
            "",
            "| 指标 | 定义 | 用途 |",
            "| --- | --- | --- |",
            "| `wall_s_mean` | `/usr/bin/time -p` 的 `real` 时间，3 次重复均值 | 端到端性能 |",
            "| `comm_critical_s_mean` | `gatherp_alltoallv` 与 `gathers_alltoallv` 的 rank 最大值之和 | 阻塞通信临界路径 |",
            "| `wait_proxy_max_s_mean` | 两个 Alltoallv timer 的 `max(rank)-min(rank)` 之和 | rank 间等待/不均衡代理量 |",
            "| `overlap_candidate_rank_avg_s_mean` | `pack + unpack + pack + clear + unpack` 的 rank 平均和 | 后续可重叠本地工作 |",
            "| `wall_cv_percent` | wall time 标准差/均值 | 重复性和噪声评估 |",
            "",
            "## 端到端耗时与加速比",
            "",
        ]
    )
    lines.extend(pivot_wall_table(walls))
    lines.extend(
        [
            "",
            "完整重复性表如下：",
            "",
        ]
    )
    lines.extend(
        md_table(
            walls,
            [
                ("case", "体系", None),
                ("config", "MPI x OMP", None),
                ("wall_s_mean", "wall mean/s", ".3f"),
                ("wall_s_stdev", "stdev/s", ".3f"),
                ("wall_cv_percent", "CV/%", ".2f"),
                ("speedup_vs_1x1", "相对 1x1", ".2f"),
            ],
        )
    )
    lines.extend(
        [
            "",
            "观察：三个体系均显示随 MPI rank 增加的有效加速，且 `8x2` 均为当前矩阵中最快或接近最快的配置。NaCl USPP 从 `47.017s` 降至 `7.360s`，加速比 `6.39x`；HCl USPP 为 `3.91x`；Si BLPS 为 `3.48x`。这说明本次矩阵可以支撑并行性能分析，不再是“小体系无法证明多核加速”的设计。",
            "",
            "## 阻塞通信临界路径",
            "",
            "下表为 `PW_Basis_K / PW_Basis_Sup` 两类路径的 `comm_critical_s_mean`。`PW_Basis_K` 对应 wave FFT 路径，`PW_Basis_Sup` 对应 dense/Sup charge/potential 路径。",
            "",
        ]
    )
    lines.extend(
        md_table(
            class_pair_rows(comm_class, "comm_critical_s_mean"),
            [
                ("case", "体系", None),
                ("config", "MPI x OMP", None),
                ("pw_basis_k", "`PW_Basis_K`/s", ".6f"),
                ("pw_basis_sup", "`PW_Basis_Sup`/s", ".6f"),
            ],
        )
    )
    lines.extend(
        [
            "",
            "按 `PW_Basis_K + PW_Basis_Sup` 合计，当前通信热点最高的 10 个配置如下：",
            "",
        ]
    )
    lines.extend(
        md_table(
            hot,
            [
                ("case", "体系", None),
                ("config", "MPI x OMP", None),
                ("comm_critical_sum_s", "critical sum/s", ".6f"),
                ("comm_fraction_percent", "critical/wall %", ".2f"),
                ("wait_proxy_sum_s", "wait proxy/s", ".6f"),
                ("overlap_candidate_sum_s", "overlap candidate/s", ".6f"),
            ],
        )
    )
    lines.extend(
        [
            "",
            "观察：NaCl USPP 的 dense/Sup 路径最重，尤其 `2x1` 的 `PW_Basis_Sup` critical path 为 `0.614784s`。Si BLPS 与 HCl USPP 在高 rank 下 `PW_Basis_K` critical path 上升，说明 wave 路径在小/中等实际工作量中更容易进入同步开销主导区间。",
            "",
            "## 等待时间代理量",
            "",
            "下表为 `PW_Basis_K / PW_Basis_Sup` 的 `wait_proxy_max_s_mean`，即各 rank 在 Alltoallv 上的最大耗时差。它不是直接网络等待时间，但可以作为 rank skew 与阻塞等待的代理量。",
            "",
        ]
    )
    lines.extend(
        md_table(
            class_pair_rows(comm_class, "wait_proxy_max_s_mean"),
            [
                ("case", "体系", None),
                ("config", "MPI x OMP", None),
                ("pw_basis_k", "`PW_Basis_K`/s", ".6f"),
                ("pw_basis_sup", "`PW_Basis_Sup`/s", ".6f"),
            ],
        )
    )
    lines.extend(
        [
            "",
            "观察：NaCl USPP `2x1` 的 `PW_Basis_Sup` wait proxy 为 `0.337974s`，是最明显的不均衡点；NaCl `4x1/8x1` 的 `PW_Basis_K` 也在 `0.12s` 量级。后续如果非阻塞实现有效，应该优先看到这些项下降。",
            "",
            "## 可重叠计算区间",
            "",
            "下表为 `overlap_candidate_rank_avg_s_mean`，由 collective 前后的 pack/unpack/clear 本地重排工作组成。非阻塞通信或分块通信应尽量把这部分本地工作安排到通信进行期间。",
            "",
        ]
    )
    lines.extend(
        md_table(
            class_pair_rows(comm_class, "overlap_candidate_rank_avg_s_mean"),
            [
                ("case", "体系", None),
                ("config", "MPI x OMP", None),
                ("pw_basis_k", "`PW_Basis_K`/s", ".6f"),
                ("pw_basis_sup", "`PW_Basis_Sup`/s", ".6f"),
            ],
        )
    )
    lines.extend(
        [
            "",
            "观察：NaCl USPP `2x1` 的 `PW_Basis_Sup` overlap candidate 为 `0.446241s`，NaCl `4x1` 为 `0.273498s`，是最值得优先尝试通信/计算重叠的对象。HCl/Si 的 overlap candidate 较小，但可用于检查优化开销是否抵消收益。",
            "",
            "## Correctness Guard",
            "",
            "性能测试故意固定 `scf_nmax=10`，并使用极小 `scf_thr=1e-30` 来降低随机性和保证工作量一致，因此不把 timing run 当作物理收敛结果。正式 correctness guard 现在使用 `scf_nmax=200, scf_thr=1e-13`，对三个体系分别运行 `1x1` 和 `8x2`：",
            "",
        ]
    )
    lines.extend(
        md_table(
            correctness,
            [
                ("case", "体系", None),
                ("config", "MPI x OMP", None),
                ("scf_thr", "`scf_thr`", None),
                ("status", "状态", None),
                ("scf_iterations", "SCF 步数", None),
                ("final_drho", "final DRHO", ".3e"),
                ("final_etot_ev", "final ETOT/eV", ".16f"),
                ("wall_s", "wall/s", ".3f"),
            ],
        )
    )
    lines.extend(
        [
            "",
            "并行一致性比较如下。这里比较的是同一 prepared case、同一 cutoff/nbands/SCF 设置下，`1x1` 与 `8x2` 的 final ETOT 差异：",
            "",
        ]
    )
    lines.extend(
        md_table(
            correctness_compare,
            [
                ("case", "体系", None),
                ("reference_config", "参考配置", None),
                ("comparison_config", "对比配置", None),
                ("reference_etot_ev", "参考 ETOT/eV", ".16f"),
                ("comparison_etot_ev", "对比 ETOT/eV", ".16f"),
                ("abs_etot_diff_ev", "abs diff/eV", ".3e"),
                ("status", "状态", None),
            ],
        )
    )
    lines.extend(
        [
            "",
            "结果说明：三体系均在 `scf_thr=1e-13` 下收敛，final DRHO 均已进入 `10^-14` 量级。跨并行配置的 ETOT 差异为 `10^-10 ~ 10^-9 eV`；这比 `1e-9` guard 严格得多，但并没有达到“跨 MPI/OpenMP 配置小数点后 14 位完全相同”。因此报告中不应声称 14 位一致，只能声称在当前并行归约/迭代路径下达到约 `10^-9 eV` 量级的一致性。",
            "",
            "`scf_thr=1e-14` 的尝试记录如下：",
            "",
        ]
    )
    lines.extend(
        md_table(
            e14_attempts,
            [
                ("case", "体系", None),
                ("config", "MPI x OMP", None),
                ("status", "状态", None),
                ("scf_iterations", "SCF 步数", None),
                ("final_drho", "final DRHO", ".3e"),
                ("final_etot_ev", "final ETOT/eV", ".16f"),
                ("wall_s", "wall/s", ".3f"),
            ],
        )
    )
    lines.extend(
        [
            "",
            "`1e-14` 下 NaCl USPP 的 `1x1/8x2` 均收敛，但 HCl USPP 和 Si BLPS 在 `1x1` 下触发 `psi_norm <= 0.0`，日志提示可能与 `npwx < nbands` 或秩亏有关。这个结果说明继续把阈值机械压到 `1e-14` 会引入求解器稳定性风险；若老师坚持 14 位完全一致，应进一步调整 correctness case 的数值设置，例如降低 `nbands`、增加 `ecutwfc`、换更稳定的对角化/初猜设置，或使用 ABACUS 官方测试中已有高精度 reference 的体系。",
            "",
            "后续优化版应在相同 guard 输入下保持收敛状态，并以这里的 final ETOT 作为同参数 baseline；不要直接拿这些 large 参数下的能量与原始 `tests` 的 `result.ref` 混比，因为本次显式改变了 `ecutrho`、`nbands` 和 SCF 设置。",
            "",
            "## 后续优化对比规则",
            "",
            "后续阻塞/非阻塞实现比较时应遵守以下规则：",
            "",
            "1. 使用同一批 prepared cases、同一 `large:50:2000`、同一 `CONFIGS`、同一 `REPEATS=3`、同一 `scf_nmax=10` 与 `nbands=24`。",
            "2. 保持相同 MPI/OpenMP 绑核策略，或者在报告中明确说明新的绑核策略，并重新跑 baseline；不要混用默认 OpenMPI 绑核数据。",
            "3. 性能结论优先比较 `wall_s_mean`，并要求改善幅度大于重复噪声。建议阈值为 `max(3%, 2 * combined stdev)`。",
            "4. 通信优化结论必须同时查看 `comm_critical_s_mean` 和 `wait_proxy_max_s_mean`；仅 wall time 下降不足以证明通信路径被优化。",
            "5. 非阻塞实现若改变 timer 结构，应保留等价的 critical path、rank skew proxy 和 overlap candidate 指标，保证可横向比较。",
            "6. 每次性能矩阵完成后至少跑 NaCl USPP 或 Si BLPS 的 correctness guard；涉及通信语义改动时建议两个 guard 都跑。",
            "",
            "重点推荐的对比点：NaCl USPP `2x1` 用于观察 dense/Sup 通信重叠，NaCl USPP `8x2` 用于观察最佳端到端配置，Si BLPS `8x1/8x2` 用于检查 wave 路径在高 rank 下的同步开销。",
            "",
            "## 风险与限制",
            "",
            "- 本 benchmark 是单节点结果，不能直接外推到跨节点网络通信。",
            "- timing run 固定迭代步数，不用于讨论最终物理精度；correctness guard 才用于收敛性检查。",
            "- `--report-bindings` 会在 `stderr.log` 中留下启动期输出，但相对本次 wall time 可忽略，并提高了审计性。",
            "- 内部 case 子目录统一使用 `large_rep...` 命名；具体体系由外层目录 `nacl_uspp`、`hcl_uspp`、`si_blps` 和汇总表中的体系名区分。",
            "",
        ]
    )
    report.write_text("\n".join(lines))


def main() -> None:
    systems = system_rows()
    walls = wall_rows()
    comm_class = comm_by_class_rows()
    comm_combined = comm_combined_rows(comm_class)
    correctness = correctness_rows()
    correctness_compare = correctness_comparison_rows(correctness)
    e14_attempts = e14_attempt_rows()

    write_csv(ROOT / "tables/system_parameters.csv", systems)
    write_csv(ROOT / "tables/performance_wall_summary.csv", walls)
    write_csv(ROOT / "tables/comm_by_class_summary.csv", comm_class)
    write_csv(ROOT / "tables/comm_combined_summary.csv", comm_combined)
    write_csv(ROOT / "tables/correctness_guard_summary.csv", correctness)
    write_csv(ROOT / "tables/correctness_guard_comparison.csv", correctness_compare)
    write_csv(ROOT / "tables/correctness_e14_attempts.csv", e14_attempts)
    write_report(systems, walls, comm_class, comm_combined, correctness, correctness_compare, e14_attempts)


if __name__ == "__main__":
    main()

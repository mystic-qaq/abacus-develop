#!/usr/bin/env python3
"""Summarize a Workflow B multi-case blocking communication benchmark suite."""

from __future__ import annotations

import argparse
import csv
import re
import statistics
from pathlib import Path


FINAL_ETOT_RE = re.compile(r"!FINAL_ETOT_IS\s+(?P<energy>[-+0-9.eE]+)\s+eV")
REAL_RE = re.compile(r"^real\s+(?P<seconds>[-+0-9.eE]+)$")


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
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


def manifest_rows(suite_dir: Path, kind: str) -> list[dict[str, str]]:
    return read_csv(suite_dir / "meta" / f"{kind}_manifest.csv")


def performance_label_map(suite_dir: Path) -> dict[str, str]:
    rows = manifest_rows(suite_dir, "performance")
    return {Path(row["case_dir"]).name: row["label"] for row in rows}


def mean(values: list[float]) -> float:
    return sum(values) / len(values) if values else 0.0


def stdev(values: list[float]) -> float:
    return statistics.stdev(values) if len(values) > 1 else 0.0


def wall_table(run_rows: list[dict[str, str]], label_map: dict[str, str]) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str, str], list[float]] = {}
    for row in run_rows:
        if row["class_name"] != "PW_Basis_K":
            continue
        label = label_map.get(row["case"], row["case"])
        grouped.setdefault((label, row["nproc"], row["omp"]), []).append(float(row["wall_s"]))

    baseline: dict[str, float] = {}
    for (label, nproc, omp), values in grouped.items():
        if nproc == "1" and omp == "1":
            baseline[label] = mean(values)

    out: list[dict[str, object]] = []
    for (label, nproc, omp), rows in sorted(grouped.items()):
        wall = mean(rows)
        sigma = stdev(rows)
        base = baseline.get(label, wall)
        out.append(
            {
                "case": label,
                "config": f"{nproc}x{omp}",
                "nproc": int(nproc),
                "omp": int(omp),
                "wall_s_mean": wall,
                "wall_s_stdev": sigma,
                "wall_cv_percent": (sigma / wall * 100.0) if wall else 0.0,
                "speedup_vs_1x1": (base / wall) if wall else 0.0,
            }
        )
    return out


def combined_comm_table(run_rows: list[dict[str, str]], label_map: dict[str, str]) -> list[dict[str, object]]:
    per_run: dict[tuple[str, str, str, str], list[dict[str, str]]] = {}
    for row in run_rows:
        label = label_map.get(row["case"], row["case"])
        key = (label, row["case"], row["nproc"], row["omp"])
        per_run.setdefault(key, []).append(row)

    grouped: dict[tuple[str, str, str], list[dict[str, float]]] = {}
    for (label, case_name, nproc, omp), rows in per_run.items():
        wall = float(rows[0]["wall_s"])
        comm = sum(float(row["comm_critical_s"]) for row in rows)
        wait = sum(float(row["wait_proxy_max_s"]) for row in rows)
        overlap = sum(float(row["overlap_candidate_rank_avg_s"]) for row in rows)
        grouped.setdefault((label, nproc, omp), []).append(
            {
                "wall_s": wall,
                "comm_critical_sum_s": comm,
                "wait_proxy_sum_s": wait,
                "overlap_candidate_sum_s": overlap,
            }
        )

    out: list[dict[str, object]] = []
    for (label, nproc, omp), rows in sorted(grouped.items()):
        if nproc == "1" and omp == "1":
            continue
        wall = mean([row["wall_s"] for row in rows])
        comm = mean([row["comm_critical_sum_s"] for row in rows])
        wait = mean([row["wait_proxy_sum_s"] for row in rows])
        overlap = mean([row["overlap_candidate_sum_s"] for row in rows])
        out.append(
            {
                "case": label,
                "config": f"{nproc}x{omp}",
                "nproc": int(nproc),
                "omp": int(omp),
                "wall_s_mean": wall,
                "comm_critical_sum_s": comm,
                "comm_fraction_percent": (comm / wall * 100.0) if wall else 0.0,
                "wait_proxy_sum_s": wait,
                "overlap_candidate_sum_s": overlap,
            }
        )
    return out


def read_real_seconds(stderr_path: Path) -> float:
    if not stderr_path.exists():
        return 0.0
    for line in stderr_path.read_text(errors="ignore").splitlines():
        match = REAL_RE.match(line.strip())
        if match:
            return float(match.group("seconds"))
    return 0.0


def parse_final_energy(case_dir: Path) -> float | None:
    for log in sorted(case_dir.glob("OUT.*/running_scf*.log")):
        text = log.read_text(errors="ignore")
        matches = FINAL_ETOT_RE.findall(text)
        if matches:
            return float(matches[-1])
    return None


def reference_energy(base_case: Path) -> float | None:
    ref = base_case / "result.ref"
    if not ref.exists():
        return None
    for line in ref.read_text(errors="ignore").splitlines():
        parts = line.split()
        if parts and parts[0] == "etotref" and len(parts) >= 2:
            return float(parts[1])
    return None


def convergence_status(case_dir: Path) -> str:
    text = "\n".join(path.read_text(errors="ignore") for path in case_dir.glob("OUT.*/running_scf*.log"))
    if "!!SCF IS NOT CONVERGED!!" in text:
        return "not_converged"
    if "converged" in text.lower() or "convergence" in text.lower():
        return "converged"
    return "unknown"


def correctness_table(rows: list[dict[str, str]]) -> list[dict[str, object]]:
    out: list[dict[str, object]] = []
    for row in rows:
        case_dir = Path(row["case_dir"])
        base_case = Path(row["base_case"])
        energy = parse_final_energy(case_dir)
        ref = reference_energy(base_case)
        out.append(
            {
                "case": row["label"],
                "config": f'{row["nproc"]}x{row["omp"]}',
                "nproc": int(row["nproc"]),
                "omp": int(row["omp"]),
                "scf_nmax": int(row["scf_nmax"]),
                "status": convergence_status(case_dir),
                "wall_s": read_real_seconds(case_dir / "stderr.log"),
                "final_etot_ev": energy if energy is not None else "",
                "ref_etot_ev": ref if ref is not None else "",
                "abs_etot_diff_ev": abs(energy - ref) if energy is not None and ref is not None else "",
                "case_dir": case_dir,
            }
        )
    return out


def markdown_table(rows: list[dict[str, object]], columns: list[str], formats: dict[str, str] | None = None) -> list[str]:
    formats = formats or {}
    lines = [
        "| " + " | ".join(columns) + " |",
        "| " + " | ".join("---" for _ in columns) + " |",
    ]
    for row in rows:
        cells: list[str] = []
        for column in columns:
            value = row.get(column, "")
            fmt = formats.get(column)
            if fmt and isinstance(value, (int, float)):
                cells.append(format(value, fmt))
            else:
                cells.append(str(value))
        lines.append("| " + " | ".join(cells) + " |")
    return lines


def write_report(
    suite_dir: Path,
    perf_wall: list[dict[str, object]],
    perf_comm: list[dict[str, object]],
    correctness: list[dict[str, object]],
) -> None:
    reports_dir = suite_dir / "reports"
    reports_dir.mkdir(parents=True, exist_ok=True)
    top_comm = sorted(perf_comm, key=lambda row: float(row["comm_critical_sum_s"]), reverse=True)[:12]
    lines = [
        "# Workflow B blocking communication benchmark refresh",
        "",
        "This refreshed baseline uses fixed 10-step SCF workloads for timing and separate longer convergence runs as correctness guards.",
        "",
        "## Scope",
        "",
        "- Timing runs use `scf_nmax=10` with an intentionally tight `scf_thr` so that each case performs the same number of electronic iterations.",
        "- Correctness guard runs preserve each source case's convergence threshold and use longer `scf_nmax`.",
        "- Results are single-node measurements and should not be interpreted as multi-node network behavior.",
        "",
        "## End-to-End Timing",
        "",
    ]
    lines.extend(
        markdown_table(
            perf_wall,
            ["case", "config", "wall_s_mean", "wall_s_stdev", "wall_cv_percent", "speedup_vs_1x1"],
            {
                "wall_s_mean": ".3f",
                "wall_s_stdev": ".3f",
                "wall_cv_percent": ".2f",
                "speedup_vs_1x1": ".2f",
            },
        )
    )
    lines.extend(["", "## Communication Hotspots", ""])
    lines.extend(
        markdown_table(
            top_comm,
            [
                "case",
                "config",
                "comm_critical_sum_s",
                "comm_fraction_percent",
                "wait_proxy_sum_s",
                "overlap_candidate_sum_s",
            ],
            {
                "comm_critical_sum_s": ".6f",
                "comm_fraction_percent": ".2f",
                "wait_proxy_sum_s": ".6f",
                "overlap_candidate_sum_s": ".6f",
            },
        )
    )
    lines.extend(["", "## Correctness Guards", ""])
    lines.extend(
        markdown_table(
            correctness,
            [
                "case",
                "config",
                "status",
                "wall_s",
                "final_etot_ev",
                "ref_etot_ev",
                "abs_etot_diff_ev",
            ],
            {
                "wall_s": ".3f",
                "final_etot_ev": ".10f",
                "ref_etot_ev": ".10f",
                "abs_etot_diff_ev": ".6e",
            },
        )
    )
    lines.extend(
        [
            "",
            "## Interpretation Notes",
            "",
            "- `comm_critical_sum_s` is the sum of `PW_Basis_K` and `PW_Basis_Sup` rank-critical Alltoallv timers.",
            "- `wait_proxy_sum_s` is a rank-skew proxy, not a direct measurement of network waiting time.",
            "- The timing runs are designed for performance comparison, not for physically converged energies.",
            "- The correctness guards are the runs to cite when discussing convergence and numerical sanity.",
            "",
        ]
    )
    (reports_dir / "blocking_comm_benchmark_refresh_report.md").write_text("\n".join(lines))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--suite-dir", type=Path, required=True)
    args = parser.parse_args()
    suite_dir = args.suite_dir.expanduser().resolve()
    label_map = performance_label_map(suite_dir)

    perf_runs = read_csv(suite_dir / "tables" / "performance" / "pw_comm_run_summary.csv")
    perf_wall = wall_table(perf_runs, label_map)
    perf_comm = combined_comm_table(perf_runs, label_map)
    correctness = correctness_table(manifest_rows(suite_dir, "correctness"))

    write_csv(suite_dir / "tables" / "performance_wall_summary.csv", perf_wall)
    write_csv(suite_dir / "tables" / "performance_comm_combined_summary.csv", perf_comm)
    write_csv(suite_dir / "tables" / "correctness_guard_summary.csv", correctness)
    write_report(suite_dir, perf_wall, perf_comm, correctness)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Parse ABACUS timer logs for Workflow B blocking communication baselines."""

from __future__ import annotations

import argparse
import csv
import re
import statistics
from pathlib import Path


TIMER_NAMES = {
    "gatherp_scatters",
    "gatherp_pack",
    "gatherp_alltoallv",
    "gatherp_unpack",
    "gathers_scatterp",
    "gathers_pack",
    "gathers_alltoallv",
    "gathers_clear",
    "gathers_unpack",
}

ROW_RE = re.compile(
    r"^\s*(?P<class_name>\S+)\s+"
    r"(?P<name>\S+)\s+"
    r"(?P<time>[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s+"
    r"(?P<calls>\d+)\s+"
    r"(?P<avg>[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s+"
    r"(?P<per>[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)"
)

RANK_RE = re.compile(r"(?:_cpu|_)(?P<rank>\d+)\.log$")
CASE_RE = re.compile(
    r"(?:nacl_)?(?:(?P<scale>[A-Za-z0-9-]+)_)?(?:rep(?P<repeat>\d+)_)?np(?P<nproc>\d+)_omp(?P<omp>\d+)"
)
REAL_RE = re.compile(r"^real\s+(?P<seconds>[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)$")


def parse_timer_rows(log_path: Path) -> dict[tuple[str, str], dict[str, float | int]]:
    rows: dict[tuple[str, str], dict[str, float | int]] = {}
    for line in log_path.read_text(errors="ignore").splitlines():
        match = ROW_RE.match(line)
        if not match:
            continue
        name = match.group("name")
        class_name = match.group("class_name")
        if name not in TIMER_NAMES or not class_name.startswith("PW_Basis"):
            continue
        rows[(class_name, name)] = {
            "time": float(match.group("time")),
            "calls": int(match.group("calls")),
            "avg": float(match.group("avg")),
            "per": float(match.group("per")),
        }
    return rows


def rank_from_path(log_path: Path) -> int:
    match = RANK_RE.search(log_path.name)
    return int(match.group("rank")) if match else 0


def case_config(case_dir: Path) -> dict[str, str]:
    match = CASE_RE.search(case_dir.name)
    if not match:
        return {"scale": "", "repeat": "", "nproc": "", "omp": ""}
    return {
        "scale": match.group("scale") or "single",
        "repeat": match.group("repeat") or "1",
        "nproc": match.group("nproc"),
        "omp": match.group("omp"),
    }


def find_logs(case_dir: Path) -> list[Path]:
    logs = sorted(case_dir.glob("OUT.*/running_*_cpu*.log"))
    if logs:
        return logs
    return sorted(case_dir.glob("OUT.*/running_*.log"))


def wall_time(case_dir: Path) -> float:
    stderr = case_dir / "stderr.log"
    if not stderr.exists():
        return 0.0
    for line in stderr.read_text(errors="ignore").splitlines():
        match = REAL_RE.match(line.strip())
        if match:
            return float(match.group("seconds"))
    return 0.0


def summarize_case(case_dir: Path) -> list[dict[str, str | int | float]]:
    logs = find_logs(case_dir)
    if not logs:
        raise FileNotFoundError(f"no running logs found below {case_dir}")

    config = case_config(case_dir)
    wall_s = wall_time(case_dir)
    per_rank: dict[int, dict[tuple[str, str], dict[str, float | int]]] = {}
    for log_path in logs:
        per_rank[rank_from_path(log_path)] = parse_timer_rows(log_path)

    keys = sorted({key for rows in per_rank.values() for key in rows})
    rows_out: list[dict[str, str | int | float]] = []
    for class_name, name in keys:
        values = [float(rows.get((class_name, name), {}).get("time", 0.0)) for rows in per_rank.values()]
        calls = [int(rows.get((class_name, name), {}).get("calls", 0)) for rows in per_rank.values()]
        if not values:
            continue
        rank_min = min(values)
        rank_max = max(values)
        rank_avg = sum(values) / len(values)
        call_max = max(calls) if calls else 0
        rows_out.append(
            {
                "case": case_dir.name,
                "scale": config["scale"],
                "repeat": config["repeat"],
                "nproc": config["nproc"],
                "omp": config["omp"],
                "wall_s": wall_s,
                "rank_count": len(per_rank),
                "class_name": class_name,
                "timer": name,
                "calls_max": call_max,
                "rank_avg_s": rank_avg,
                "rank_min_s": rank_min,
                "rank_max_s": rank_max,
                "wait_proxy_avg_s": max(0.0, rank_avg - rank_min),
                "wait_proxy_max_s": max(0.0, rank_max - rank_min),
                "rank_avg_per_call_s": rank_avg / call_max if call_max else 0.0,
                "rank_max_per_call_s": rank_max / call_max if call_max else 0.0,
            }
        )
    return rows_out


def aggregate_summary(rows: list[dict[str, str | int | float]]) -> list[dict[str, str | float]]:
    by_case_class: dict[tuple[str, str], dict[str, dict[str, str | int | float]]] = {}
    for row in rows:
        by_case_class.setdefault((str(row["case"]), str(row["class_name"])), {})[str(row["timer"])] = row

    summary = []
    for (case, class_name), timers in sorted(by_case_class.items()):
        def max_s(name: str) -> float:
            return float(timers.get(name, {}).get("rank_max_s", 0.0))

        def avg_s(name: str) -> float:
            return float(timers.get(name, {}).get("rank_avg_s", 0.0))

        def wait_s(name: str) -> float:
            return float(timers.get(name, {}).get("wait_proxy_max_s", 0.0))

        comm_critical = max_s("gatherp_alltoallv") + max_s("gathers_alltoallv")
        comm_avg = avg_s("gatherp_alltoallv") + avg_s("gathers_alltoallv")
        wait_proxy = wait_s("gatherp_alltoallv") + wait_s("gathers_alltoallv")
        overlap_compute = (
            avg_s("gatherp_pack")
            + avg_s("gatherp_unpack")
            + avg_s("gathers_pack")
            + avg_s("gathers_clear")
            + avg_s("gathers_unpack")
        )
        summary.append(
            {
                "case": case,
                "scale": str(next(iter(timers.values())).get("scale", "")),
                "repeat": str(next(iter(timers.values())).get("repeat", "")),
                "nproc": str(next(iter(timers.values())).get("nproc", "")),
                "omp": str(next(iter(timers.values())).get("omp", "")),
                "wall_s": float(next(iter(timers.values())).get("wall_s", 0.0)),
                "class_name": class_name,
                "comm_rank_avg_s": comm_avg,
                "comm_critical_s": comm_critical,
                "wait_proxy_max_s": wait_proxy,
                "overlap_candidate_rank_avg_s": overlap_compute,
                "gatherp_comm_critical_s": max_s("gatherp_alltoallv"),
                "gathers_comm_critical_s": max_s("gathers_alltoallv"),
            }
        )
    return summary


def mean(values: list[float]) -> float:
    return sum(values) / len(values) if values else 0.0


def stdev(values: list[float]) -> float:
    return statistics.stdev(values) if len(values) > 1 else 0.0


def aggregate_repeats(rows: list[dict[str, str | float]]) -> list[dict[str, str | int | float]]:
    grouped: dict[tuple[str, str, str, str], list[dict[str, str | float]]] = {}
    for row in rows:
        key = (str(row["scale"]), str(row["nproc"]), str(row["omp"]), str(row["class_name"]))
        grouped.setdefault(key, []).append(row)

    out: list[dict[str, str | int | float]] = []
    metrics = [
        "wall_s",
        "comm_rank_avg_s",
        "comm_critical_s",
        "wait_proxy_max_s",
        "overlap_candidate_rank_avg_s",
        "gatherp_comm_critical_s",
        "gathers_comm_critical_s",
    ]
    for (scale, nproc, omp, class_name), group in sorted(grouped.items()):
        row_out: dict[str, str | int | float] = {
            "scale": scale,
            "nproc": nproc,
            "omp": omp,
            "class_name": class_name,
            "repeats": len(group),
        }
        for metric in metrics:
            values = [float(row[metric]) for row in group]
            row_out[f"{metric}_mean"] = mean(values)
            row_out[f"{metric}_stdev"] = stdev(values)
        out.append(row_out)
    return out


def write_csv(path: Path, rows: list[dict[str, str | int | float]]) -> None:
    if not rows:
        return
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def write_markdown(path: Path, detail_rows: list[dict[str, str | int | float]]) -> None:
    run_summary = aggregate_summary(detail_rows)
    benchmark_summary = aggregate_repeats(run_summary)
    lines = [
        "# Workflow B blocking communication benchmark",
        "",
        "Interpretation:",
        "",
        "- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.",
        "- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).",
        "- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.",
        "- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.",
        "",
        "## Repeated Summary",
        "",
        "| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |",
        "| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |",
    ]
    for row in benchmark_summary:
        lines.append(
            "| {scale} | {nproc} | {omp} | {class_name} | {repeats} | "
            "{wall_s_mean:.3f} +/- {wall_s_stdev:.3f} | "
            "{comm_critical_s_mean:.6f} +/- {comm_critical_s_stdev:.6f} | "
            "{wait_proxy_max_s_mean:.6f} +/- {wait_proxy_max_s_stdev:.6f} | "
            "{overlap_candidate_rank_avg_s_mean:.6f} +/- {overlap_candidate_rank_avg_s_stdev:.6f} |".format(**row)
        )

    lines.extend(
        [
            "",
            "## Per-Run Summary",
            "",
            "| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |",
            "| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |",
        ]
    )
    for row in run_summary:
        lines.append(
            "| {case} | {scale} | {repeat} | {nproc} | {omp} | {class_name} | {wall_s:.3f} | "
            "{comm_rank_avg_s:.6f} | {comm_critical_s:.6f} | "
            "{wait_proxy_max_s:.6f} | {overlap_candidate_rank_avg_s:.6f} |".format(**row)
        )

    lines.extend(
        [
            "",
            "## Detail",
            "",
            "| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |",
            "| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |",
        ]
    )
    for row in detail_rows:
        lines.append(
            "| {case} | {class_name} | {timer} | {calls_max} | {rank_avg_s:.6f} | "
            "{rank_min_s:.6f} | {rank_max_s:.6f} | {wait_proxy_max_s:.6f} |".format(**row)
        )
    path.write_text("\n".join(lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("case_dirs", nargs="+", type=Path)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    detail_rows: list[dict[str, str | int | float]] = []
    for case_dir in args.case_dirs:
        detail_rows.extend(summarize_case(case_dir))

    run_summary = aggregate_summary(detail_rows)
    write_csv(args.out_dir / "pw_comm_timers_detail.csv", detail_rows)
    write_csv(args.out_dir / "pw_comm_run_summary.csv", run_summary)
    write_csv(args.out_dir / "pw_comm_benchmark_summary.csv", aggregate_repeats(run_summary))
    write_csv(args.out_dir / "pw_comm_timers_summary.csv", run_summary)
    write_markdown(args.out_dir / "blocking_comm_baseline_report.md", detail_rows)


if __name__ == "__main__":
    main()

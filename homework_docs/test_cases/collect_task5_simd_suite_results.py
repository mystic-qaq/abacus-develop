#!/usr/bin/env python3
"""Collect one or more Task 5 suite outputs into suite-level CSV/Markdown summaries."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List

import collect_task5_simd_results as bench_collect


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--suite-dir",
        action="append",
        dest="suite_dirs",
        required=True,
        help="Suite output directory produced by run_task5_simd_suite.sh. Repeat to compare baseline and simd suites.",
    )
    parser.add_argument(
        "--out-dir",
        required=True,
        help="Directory for suite_summary.csv and suite_summary.md.",
    )
    return parser.parse_args()


def discover_benchmark_dirs(suite_dir: Path) -> List[Path]:
    benchmark_dirs: List[Path] = []
    if not suite_dir.exists():
        return benchmark_dirs
    for child in sorted(suite_dir.iterdir()):
        if not child.is_dir():
            continue
        if (child / "run_manifest.csv").exists():
            benchmark_dirs.append(child)
    return benchmark_dirs


def read_suite_manifest(suite_dir: Path) -> Dict[str, Dict[str, str]]:
    manifest_path = suite_dir / "suite_manifest.csv"
    mapping: Dict[str, Dict[str, str]] = {}
    if not manifest_path.exists():
        return mapping
    with manifest_path.open("r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            out_dir = row.get("out_dir", "")
            if out_dir:
                mapping[str(Path(out_dir).resolve())] = row
    return mapping


def enrich_summary_rows(summary_rows: List[Dict[str, object]], suite_dirs: List[Path]) -> List[Dict[str, object]]:
    manifest_maps = {str(suite_dir.resolve()): read_suite_manifest(suite_dir) for suite_dir in suite_dirs}

    for row in summary_rows:
        input_dir = str(Path(str(row.get("input_dir", ""))).resolve())
        suite_dir = str(Path(input_dir).parent.resolve())
        row["suite_dir"] = suite_dir
        row["benchmark_dir"] = input_dir
        row["logs_dir"] = str(Path(input_dir) / "logs")

        suite_manifest_row = manifest_maps.get(suite_dir, {}).get(input_dir, {})
        row["suite_exit_code"] = suite_manifest_row.get("exit_code", "NA")
        row["suite_start_time"] = suite_manifest_row.get("start_time", "NA")
        row["suite_end_time"] = suite_manifest_row.get("end_time", "NA")
    return summary_rows


def write_suite_summary_csv(rows: List[Dict[str, object]], out_path: Path) -> None:
    fieldnames = [
        "label",
        "case_name",
        "git_commit",
        "hostname",
        "nproc",
        "threads",
        "repeat_count",
        "successful_runs",
        "time_statistics_runs",
        "median_time_s",
        "mean_time_s",
        "min_time_s",
        "max_time_s",
        "baseline_median_time_s",
        "speedup_vs_baseline",
        "gatherp_scatters_s_median",
        "gathers_scatterp_s_median",
        "real2recip_s_median",
        "recip2real_s_median",
        "suite_exit_code",
        "suite_start_time",
        "suite_end_time",
        "benchmark_dir",
        "logs_dir",
        "notes",
    ]
    with out_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            formatted = {}
            for field in fieldnames:
                value = row.get(field)
                if isinstance(value, float):
                    formatted[field] = bench_collect.fmt_float(value)
                elif value is None or value == "":
                    formatted[field] = "NA"
                else:
                    formatted[field] = value
            writer.writerow(formatted)


def write_suite_summary_md(rows: List[Dict[str, object]], out_path: Path) -> None:
    lines = [
        "| label | case | commit | np | omp | median(s) | speedup vs baseline | gatherp(s) | gathers(s) | real2recip(s) | recip2real(s) | benchmark_dir | notes |",
        "| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |",
    ]
    for row in rows:
        lines.append(
            "| {label} | {case_name} | {commit} | {nproc} | {threads} | {median} | {speedup} | {gp} | {gs} | {r2c} | {r2r} | {benchmark_dir} | {notes} |".format(
                label=row.get("label", "NA"),
                case_name=row.get("case_name", "NA"),
                commit=str(row.get("git_commit", "NA"))[:12],
                nproc=row.get("nproc", "NA"),
                threads=row.get("threads", "NA"),
                median=bench_collect.fmt_float(row.get("median_time_s")),
                speedup=bench_collect.fmt_float(row.get("speedup_vs_baseline")),
                gp=bench_collect.fmt_float(row.get("gatherp_scatters_s_median")),
                gs=bench_collect.fmt_float(row.get("gathers_scatterp_s_median")),
                r2c=bench_collect.fmt_float(row.get("real2recip_s_median")),
                r2r=bench_collect.fmt_float(row.get("recip2real_s_median")),
                benchmark_dir=row.get("benchmark_dir", "NA"),
                notes=row.get("notes") or "NA",
            )
        )
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    args = parse_args()
    suite_dirs = [Path(item).resolve() for item in args.suite_dirs]
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    benchmark_dirs: List[Path] = []
    for suite_dir in suite_dirs:
        benchmark_dirs.extend(discover_benchmark_dirs(suite_dir))

    raw_rows: List[Dict[str, object]] = []
    for benchmark_dir in benchmark_dirs:
        raw_rows.extend(bench_collect.collect_rows(benchmark_dir))

    bench_collect.write_raw_results(raw_rows, out_dir / "suite_raw_results.csv")
    summary_rows = bench_collect.summarize_rows(raw_rows)
    for row in summary_rows:
        if "input_dir" not in row:
            row["input_dir"] = next(
                (
                    str(Path(raw_row["input_dir"]).resolve())
                    for raw_row in raw_rows
                    if raw_row.get("label") == row.get("label")
                    and raw_row.get("case_name") == row.get("case_name")
                    and raw_row.get("git_commit") == row.get("git_commit")
                    and raw_row.get("hostname") == row.get("hostname")
                    and raw_row.get("nproc") == row.get("nproc")
                    and raw_row.get("threads") == row.get("threads")
                ),
                "NA",
            )
    bench_collect.add_speedup(summary_rows)
    summary_rows = enrich_summary_rows(summary_rows, suite_dirs)

    suite_summary_csv = out_dir / "suite_summary.csv"
    suite_summary_md = out_dir / "suite_summary.md"
    write_suite_summary_csv(summary_rows, suite_summary_csv)
    write_suite_summary_md(summary_rows, suite_summary_md)

    print(f"Wrote {out_dir / 'suite_raw_results.csv'}")
    print(f"Wrote {suite_summary_csv}")
    print(f"Wrote {suite_summary_md}")


if __name__ == "__main__":
    main()

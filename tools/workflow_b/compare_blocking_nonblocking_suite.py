#!/usr/bin/env python3
"""Compare a Workflow B nonblocking suite against the 2026-05-23 blocking baseline."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


WALL_TABLE_NAMES = ("performance_wall_summary.csv",)
COMM_TABLE_NAMES = ("performance_comm_combined_summary.csv", "comm_combined_summary.csv")
CORRECTNESS_SUMMARY_NAMES = ("correctness_guard_summary.csv",)
CORRECTNESS_PAIR_NAMES = ("correctness_guard_comparison.csv",)


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


def find_table(root: Path, table_names: tuple[str, ...]) -> Path:
    for name in table_names:
        path = root / "tables" / name
        if path.exists():
            return path
    raise FileNotFoundError(f"missing tables in {root}: {', '.join(table_names)}")


def maybe_float(value: str | None) -> float | None:
    if value is None:
        return None
    text = value.strip()
    if not text:
        return None
    return float(text)


def config_sort_key(config: str) -> tuple[int, int, str]:
    if "x" not in config:
        return (10**9, 10**9, config)
    np_text, omp_text = config.split("x", 1)
    try:
        return (int(np_text), int(omp_text), config)
    except ValueError:
        return (10**9, 10**9, config)


def common_key_map(
    rows: list[dict[str, str]],
    *,
    key_fields: tuple[str, ...],
) -> dict[tuple[str, ...], dict[str, str]]:
    return {tuple(row[field] for field in key_fields): row for row in rows}


def derive_correctness_pairs(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    grouped: dict[str, dict[str, dict[str, str]]] = {}
    for row in rows:
        grouped.setdefault(row["case"], {})[row["config"]] = row

    out: list[dict[str, str]] = []
    for case, configs in sorted(grouped.items()):
        ref = configs.get("1x1")
        cmp = configs.get("8x2")
        if ref is None or cmp is None:
            continue
        ref_etot = maybe_float(ref.get("final_etot_ev"))
        cmp_etot = maybe_float(cmp.get("final_etot_ev"))
        diff = abs(ref_etot - cmp_etot) if ref_etot is not None and cmp_etot is not None else None
        status = "pass" if diff is not None and diff <= 1.0e-8 else "missing"
        out.append(
            {
                "case": case,
                "reference_config": "1x1",
                "comparison_config": "8x2",
                "reference_etot_ev": "" if ref_etot is None else f"{ref_etot:.16f}",
                "comparison_etot_ev": "" if cmp_etot is None else f"{cmp_etot:.16f}",
                "abs_etot_diff_ev": "" if diff is None else f"{diff:.16e}",
                "status": status,
            }
        )
    return out


def load_correctness_pairs(root: Path) -> list[dict[str, str]]:
    for name in CORRECTNESS_PAIR_NAMES:
        path = root / "tables" / name
        if path.exists():
            return read_csv(path)
    summary = read_csv(find_table(root, CORRECTNESS_SUMMARY_NAMES))
    return derive_correctness_pairs(summary)


def percent_delta(candidate: float | None, baseline: float | None) -> float | None:
    if candidate is None or baseline is None or baseline == 0.0:
        return None
    return (candidate - baseline) / baseline * 100.0


def compare_wall(
    baseline_rows: list[dict[str, str]],
    candidate_rows: list[dict[str, str]],
) -> list[dict[str, object]]:
    baseline = common_key_map(baseline_rows, key_fields=("case", "config"))
    candidate = common_key_map(candidate_rows, key_fields=("case", "config"))
    rows: list[dict[str, object]] = []
    for case, config in sorted(set(baseline) & set(candidate), key=lambda item: (item[0], config_sort_key(item[1]))):
        base_row = baseline[(case, config)]
        cand_row = candidate[(case, config)]
        base_wall = maybe_float(base_row.get("wall_s_mean"))
        cand_wall = maybe_float(cand_row.get("wall_s_mean"))
        delta_s = None if base_wall is None or cand_wall is None else cand_wall - base_wall
        rows.append(
            {
                "case": case,
                "config": config,
                "baseline_wall_s_mean": base_wall,
                "candidate_wall_s_mean": cand_wall,
                "wall_delta_s": delta_s,
                "wall_delta_percent": percent_delta(cand_wall, base_wall),
            }
        )
    return rows


def compare_comm(
    baseline_rows: list[dict[str, str]],
    candidate_rows: list[dict[str, str]],
) -> list[dict[str, object]]:
    baseline = common_key_map(baseline_rows, key_fields=("case", "config"))
    candidate = common_key_map(candidate_rows, key_fields=("case", "config"))
    rows: list[dict[str, object]] = []
    for case, config in sorted(set(baseline) & set(candidate), key=lambda item: (item[0], config_sort_key(item[1]))):
        base_row = baseline[(case, config)]
        cand_row = candidate[(case, config)]
        base_comm = maybe_float(base_row.get("comm_critical_sum_s"))
        cand_comm = maybe_float(cand_row.get("comm_critical_sum_s"))
        base_wait = maybe_float(base_row.get("wait_proxy_sum_s"))
        cand_wait = maybe_float(cand_row.get("wait_proxy_sum_s"))
        base_overlap = maybe_float(base_row.get("overlap_candidate_sum_s"))
        cand_overlap = maybe_float(cand_row.get("overlap_candidate_sum_s"))
        rows.append(
            {
                "case": case,
                "config": config,
                "baseline_comm_critical_sum_s": base_comm,
                "candidate_comm_critical_sum_s": cand_comm,
                "comm_delta_s": None if base_comm is None or cand_comm is None else cand_comm - base_comm,
                "comm_delta_percent": percent_delta(cand_comm, base_comm),
                "baseline_wait_proxy_sum_s": base_wait,
                "candidate_wait_proxy_sum_s": cand_wait,
                "wait_delta_s": None if base_wait is None or cand_wait is None else cand_wait - base_wait,
                "wait_delta_percent": percent_delta(cand_wait, base_wait),
                "baseline_overlap_candidate_sum_s": base_overlap,
                "candidate_overlap_candidate_sum_s": cand_overlap,
                "overlap_delta_s": None if base_overlap is None or cand_overlap is None else cand_overlap - base_overlap,
                "overlap_delta_percent": percent_delta(cand_overlap, base_overlap),
            }
        )
    return rows


def compare_correctness_summary(
    baseline_rows: list[dict[str, str]],
    candidate_rows: list[dict[str, str]],
) -> list[dict[str, object]]:
    baseline = common_key_map(baseline_rows, key_fields=("case", "config"))
    candidate = common_key_map(candidate_rows, key_fields=("case", "config"))
    rows: list[dict[str, object]] = []
    for case, config in sorted(set(baseline) & set(candidate), key=lambda item: (item[0], config_sort_key(item[1]))):
        base_row = baseline[(case, config)]
        cand_row = candidate[(case, config)]
        base_etot = maybe_float(base_row.get("final_etot_ev"))
        cand_etot = maybe_float(cand_row.get("final_etot_ev"))
        etot_delta = None if base_etot is None or cand_etot is None else abs(cand_etot - base_etot)
        rows.append(
            {
                "case": case,
                "config": config,
                "baseline_status": base_row.get("status", ""),
                "candidate_status": cand_row.get("status", ""),
                "baseline_final_etot_ev": base_etot,
                "candidate_final_etot_ev": cand_etot,
                "abs_etot_delta_vs_baseline_ev": etot_delta,
            }
        )
    return rows


def compare_correctness_pairs(
    baseline_rows: list[dict[str, str]],
    candidate_rows: list[dict[str, str]],
) -> list[dict[str, object]]:
    baseline = common_key_map(baseline_rows, key_fields=("case",))
    candidate = common_key_map(candidate_rows, key_fields=("case",))
    rows: list[dict[str, object]] = []
    for key in sorted(set(baseline) & set(candidate)):
        case = key[0]
        base_row = baseline[key]
        cand_row = candidate[key]
        base_diff = maybe_float(base_row.get("abs_etot_diff_ev"))
        cand_diff = maybe_float(cand_row.get("abs_etot_diff_ev"))
        rows.append(
            {
                "case": case,
                "reference_config": cand_row.get("reference_config", base_row.get("reference_config", "1x1")),
                "comparison_config": cand_row.get("comparison_config", base_row.get("comparison_config", "8x2")),
                "baseline_abs_etot_diff_ev": base_diff,
                "candidate_abs_etot_diff_ev": cand_diff,
                "abs_diff_delta_ev": None if base_diff is None or cand_diff is None else cand_diff - base_diff,
                "baseline_status": base_row.get("status", ""),
                "candidate_status": cand_row.get("status", ""),
            }
        )
    return rows


def markdown_table(
    rows: list[dict[str, object]],
    columns: list[str],
    float_formats: dict[str, str] | None = None,
) -> list[str]:
    float_formats = float_formats or {}
    lines = [
        "| " + " | ".join(columns) + " |",
        "| " + " | ".join("---" for _ in columns) + " |",
    ]
    for row in rows:
        cells: list[str] = []
        for column in columns:
            value = row.get(column, "")
            if value is None:
                cells.append("")
            elif column in float_formats and isinstance(value, (int, float)):
                cells.append(format(value, float_formats[column]))
            else:
                cells.append(str(value))
        lines.append("| " + " | ".join(cells) + " |")
    return lines


def layered_decision(comm_rows: list[dict[str, object]], wall_rows: list[dict[str, object]]) -> tuple[str, list[str]]:
    wall_map = {(row["case"], row["config"]): row for row in wall_rows}
    comm_map = {(row["case"], row["config"]): row for row in comm_rows}
    notes: list[str] = []

    hotspot_pass = False
    for case, config in (("NaCl USPP", "2x1"), ("Si BLPS", "2x1")):
        wall = wall_map.get((case, config))
        comm = comm_map.get((case, config))
        if wall is None or comm is None:
            continue
        wall_delta = wall.get("wall_delta_percent")
        comm_delta = comm.get("comm_delta_s")
        if isinstance(wall_delta, float) and isinstance(comm_delta, float):
            if comm_delta < 0.0 and wall_delta <= -3.0:
                hotspot_pass = True
                notes.append(f"{case} {config} met the >3% wall improvement and lower comm critical-path target.")

    nacl_8x2 = wall_map.get(("NaCl USPP", "8x2"))
    nacl_guard_pass = False
    if nacl_8x2 is not None and isinstance(nacl_8x2.get("wall_delta_percent"), float):
        nacl_guard_pass = nacl_8x2["wall_delta_percent"] <= 3.0
        if nacl_guard_pass:
            notes.append("NaCl USPP 8x2 stayed within the <=3% wall-regression guard.")

    decision = "proceed" if hotspot_pass and nacl_guard_pass else "stop"
    if not hotspot_pass:
        notes.append("No hotspot point satisfied both lower comm critical-path and >3% wall improvement.")
    if not nacl_guard_pass:
        notes.append("NaCl USPP 8x2 exceeded the <=3% wall-regression guard or was missing.")
    return decision, notes


def correctness_decision(
    summary_rows: list[dict[str, object]],
    pair_rows: list[dict[str, object]],
) -> tuple[str, list[str]]:
    notes: list[str] = []
    all_status_ok = all(row.get("candidate_status") == "converged" for row in summary_rows)
    pair_ok = all(
        isinstance(row.get("candidate_abs_etot_diff_ev"), float) and row["candidate_abs_etot_diff_ev"] <= 1.0e-8
        for row in pair_rows
    )
    baseline_ok = all(
        isinstance(row.get("abs_etot_delta_vs_baseline_ev"), float)
        and row["abs_etot_delta_vs_baseline_ev"] <= 1.0e-8
        for row in summary_rows
        if row.get("candidate_status") == "converged" and row.get("baseline_status") == "converged"
    )

    if all_status_ok:
        notes.append("All candidate correctness cases converged.")
    else:
        notes.append("At least one candidate correctness case did not converge.")
    if pair_ok:
        notes.append("All candidate 1x1 vs 8x2 ETOT differences stayed within 1e-8 eV.")
    else:
        notes.append("At least one candidate 1x1 vs 8x2 ETOT difference exceeded 1e-8 eV.")
    if baseline_ok:
        notes.append("All candidate ETOT values stayed within 1e-8 eV of the blocking baseline for the same config.")
    else:
        notes.append("At least one candidate ETOT deviated from the blocking baseline by more than 1e-8 eV.")

    decision = "pass" if all_status_ok and pair_ok and baseline_ok else "fail"
    return decision, notes


def write_report(
    report_path: Path,
    baseline_dir: Path,
    candidate_dir: Path,
    wall_rows: list[dict[str, object]],
    comm_rows: list[dict[str, object]],
    correctness_rows: list[dict[str, object]],
    pair_rows: list[dict[str, object]],
) -> None:
    layered_status, layered_notes = layered_decision(comm_rows, wall_rows)
    correctness_status, correctness_notes = correctness_decision(correctness_rows, pair_rows)

    lines = [
        "# Workflow B blocking vs nonblocking comparison",
        "",
        f"- baseline: `{baseline_dir}`",
        f"- candidate: `{candidate_dir}`",
        "",
        "## Gate Summary",
        "",
        f"- layered performance decision: `{layered_status}`",
        f"- correctness decision: `{correctness_status}`",
        "",
        "## Layered Performance Notes",
        "",
    ]
    lines.extend(f"- {note}" for note in layered_notes)
    lines.extend(["", "## Correctness Notes", ""])
    lines.extend(f"- {note}" for note in correctness_notes)
    lines.extend(["", "## Wall Delta", ""])
    lines.extend(
        markdown_table(
            wall_rows,
            ["case", "config", "baseline_wall_s_mean", "candidate_wall_s_mean", "wall_delta_s", "wall_delta_percent"],
            {
                "baseline_wall_s_mean": ".3f",
                "candidate_wall_s_mean": ".3f",
                "wall_delta_s": ".3f",
                "wall_delta_percent": ".2f",
            },
        )
    )
    lines.extend(["", "## Communication Delta", ""])
    lines.extend(
        markdown_table(
            comm_rows,
            [
                "case",
                "config",
                "baseline_comm_critical_sum_s",
                "candidate_comm_critical_sum_s",
                "comm_delta_s",
                "comm_delta_percent",
                "baseline_wait_proxy_sum_s",
                "candidate_wait_proxy_sum_s",
                "wait_delta_s",
            ],
            {
                "baseline_comm_critical_sum_s": ".6f",
                "candidate_comm_critical_sum_s": ".6f",
                "comm_delta_s": ".6f",
                "comm_delta_percent": ".2f",
                "baseline_wait_proxy_sum_s": ".6f",
                "candidate_wait_proxy_sum_s": ".6f",
                "wait_delta_s": ".6f",
            },
        )
    )
    lines.extend(["", "## Correctness Delta", ""])
    lines.extend(
        markdown_table(
            correctness_rows,
            [
                "case",
                "config",
                "baseline_status",
                "candidate_status",
                "baseline_final_etot_ev",
                "candidate_final_etot_ev",
                "abs_etot_delta_vs_baseline_ev",
            ],
            {
                "baseline_final_etot_ev": ".12f",
                "candidate_final_etot_ev": ".12f",
                "abs_etot_delta_vs_baseline_ev": ".3e",
            },
        )
    )
    lines.extend(["", "## Parallel Consistency Delta", ""])
    lines.extend(
        markdown_table(
            pair_rows,
            [
                "case",
                "reference_config",
                "comparison_config",
                "baseline_abs_etot_diff_ev",
                "candidate_abs_etot_diff_ev",
                "abs_diff_delta_ev",
                "candidate_status",
            ],
            {
                "baseline_abs_etot_diff_ev": ".3e",
                "candidate_abs_etot_diff_ev": ".3e",
                "abs_diff_delta_ev": ".3e",
            },
        )
    )
    report_path.write_text("\n".join(lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline-dir", type=Path, required=True)
    parser.add_argument("--candidate-dir", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, default=None)
    args = parser.parse_args()

    baseline_dir = args.baseline_dir.expanduser().resolve()
    candidate_dir = args.candidate_dir.expanduser().resolve()
    out_dir = args.out_dir.expanduser().resolve() if args.out_dir else candidate_dir / "tables" / "comparison_vs_blocking"
    out_dir.mkdir(parents=True, exist_ok=True)

    baseline_wall = read_csv(find_table(baseline_dir, WALL_TABLE_NAMES))
    candidate_wall = read_csv(find_table(candidate_dir, WALL_TABLE_NAMES))
    baseline_comm = read_csv(find_table(baseline_dir, COMM_TABLE_NAMES))
    candidate_comm = read_csv(find_table(candidate_dir, COMM_TABLE_NAMES))
    baseline_correctness = read_csv(find_table(baseline_dir, CORRECTNESS_SUMMARY_NAMES))
    candidate_correctness = read_csv(find_table(candidate_dir, CORRECTNESS_SUMMARY_NAMES))
    baseline_pairs = load_correctness_pairs(baseline_dir)
    candidate_pairs = load_correctness_pairs(candidate_dir)

    wall_rows = compare_wall(baseline_wall, candidate_wall)
    comm_rows = compare_comm(baseline_comm, candidate_comm)
    correctness_rows = compare_correctness_summary(baseline_correctness, candidate_correctness)
    pair_rows = compare_correctness_pairs(baseline_pairs, candidate_pairs)

    write_csv(out_dir / "wall_delta.csv", wall_rows)
    write_csv(out_dir / "comm_delta.csv", comm_rows)
    write_csv(out_dir / "correctness_delta.csv", correctness_rows)
    write_csv(out_dir / "correctness_pair_delta.csv", pair_rows)
    write_report(out_dir / "comparison_report.md", baseline_dir, candidate_dir, wall_rows, comm_rows, correctness_rows, pair_rows)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
import csv
import re
import sys
from pathlib import Path


def read_text(path: Path) -> str:
    try:
        return path.read_text(errors="replace")
    except FileNotFoundError:
        return ""


def last_float(pattern: str, text: str):
    vals = re.findall(pattern, text, flags=re.IGNORECASE)
    return vals[-1] if vals else ""


def parse_time(text: str) -> str:
    match = re.search(r"^real\s+([0-9.]+)", text, flags=re.MULTILINE)
    return match.group(1) if match else ""


def parse_input_info(text: str, key: str) -> str:
    match = re.search(rf"^{re.escape(key)}\s+(\S+)", text, flags=re.MULTILINE)
    return match.group(1) if match else ""


def parse_kpt(path: Path) -> str:
    lines = [line.strip() for line in read_text(path).splitlines() if line.strip()]
    for i, line in enumerate(lines):
        if line.lower() == "gamma" and i + 1 < len(lines):
            return "Gamma " + lines[i + 1]
        if line.lower() == "direct":
            count = lines[i - 1] if i > 0 else "?"
            first = lines[i + 1] if i + 1 < len(lines) else ""
            return f"Direct n={count} first={first}"
    return ""


def parse_ref(path: Path) -> str:
    text = read_text(path)
    match = re.search(r"^etotref\s+([-+0-9.eE]+)", text, flags=re.MULTILINE)
    return match.group(1) if match else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: summarize_gammaonly_correctness.py BENCH_DIR", file=sys.stderr)
        return 2

    bench = Path(sys.argv[1])
    rows = []
    for run_dir in sorted((bench / "runs").glob("*/*")):
        case_name = run_dir.parent.name
        cfg_name = run_dir.name
        running = read_text(run_dir / "OUT.autotest" / "running_scf.log")
        warning = read_text(run_dir / "OUT.autotest" / "warning.log")
        input_info = read_text(run_dir / "OUT.autotest" / "INPUT.info")
        stdout = read_text(run_dir / "stdout.log")
        stderr = read_text(run_dir / "stderr.log")
        time_log = stderr

        final_etot = last_float(r"FINAL_ETOT_IS\s+([-+0-9.eE]+)", running)
        if not final_etot:
            final_etot = last_float(r"ETOT\s*=\s*([-+0-9.eE]+)", running)
        drho = last_float(r"DRHO\s*=\s*([-+0-9.eE]+)", running)
        if not drho:
            drho = last_float(r"drho\s*=\s*([-+0-9.eE]+)", running)
        if not drho:
            drho = last_float(r"Electron density deviation\s+([-+0-9.eE]+)", running)
        converged = "yes" if re.search(r"charge density convergence is achieved|converged", running, re.I) else "unknown"

        ref = parse_ref(run_dir / "result.ref")
        abs_diff = ""
        if ref and final_etot:
            abs_diff = f"{abs(float(final_etot) - float(ref)):.6e}"

        rows.append({
            "case": case_name,
            "config": cfg_name,
            "kpt": parse_kpt(run_dir / "KPT"),
            "gamma_only_input_info": parse_input_info(input_info, "gamma_only"),
            "basis_type": parse_input_info(input_info, "basis_type"),
            "ecutwfc": parse_input_info(input_info, "ecutwfc"),
            "ecutrho": parse_input_info(input_info, "ecutrho"),
            "nbands": parse_input_info(input_info, "nbands"),
            "converged": converged,
            "final_etot_ev": final_etot,
            "ref_etot_ev": ref,
            "abs_diff_ev": abs_diff,
            "final_drho": drho,
            "wall_s": parse_time(time_log),
            "warning_has_gamma_only_pw_reset": "yes" if "gamma_only has not been implemented for pw yet" in warning else "no",
            "stdout_tail": " | ".join(stdout.splitlines()[-3:]),
        })

    out_dir = bench / "tables"
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / "correctness_summary.csv"
    fields = [
        "case", "config", "kpt", "gamma_only_input_info", "basis_type",
        "ecutwfc", "ecutrho", "nbands", "converged", "final_etot_ev",
        "ref_etot_ev", "abs_diff_ev", "final_drho", "wall_s",
        "warning_has_gamma_only_pw_reset", "stdout_tail",
    ]
    with out.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

    by_case = {}
    for row in rows:
        by_case.setdefault(row["case"], {})[row["config"]] = row
    cmp_rows = []
    for case_name, cfgs in sorted(by_case.items()):
        ref = cfgs.get("np1_omp1")
        other = cfgs.get("np4_omp1")
        if not ref or not other or not ref["final_etot_ev"] or not other["final_etot_ev"]:
            continue
        cmp_rows.append({
            "case": case_name,
            "reference_config": "np1_omp1",
            "compare_config": "np4_omp1",
            "reference_etot_ev": ref["final_etot_ev"],
            "compare_etot_ev": other["final_etot_ev"],
            "abs_diff_ev": f"{abs(float(ref['final_etot_ev']) - float(other['final_etot_ev'])):.6e}",
            "reference_drho": ref["final_drho"],
            "compare_drho": other["final_drho"],
            "status": "pass" if abs(float(ref["final_etot_ev"]) - float(other["final_etot_ev"])) < 1e-6 else "check",
        })
    cmp_out = out_dir / "parallel_comparison.csv"
    cmp_fields = [
        "case", "reference_config", "compare_config", "reference_etot_ev",
        "compare_etot_ev", "abs_diff_ev", "reference_drho", "compare_drho",
        "status",
    ]
    with cmp_out.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=cmp_fields)
        writer.writeheader()
        writer.writerows(cmp_rows)

    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

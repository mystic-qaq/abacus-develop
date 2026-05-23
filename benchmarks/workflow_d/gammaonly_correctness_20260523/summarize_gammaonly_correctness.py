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


def parse_grid(text: str, label: str) -> str:
    match = re.search(rf"{re.escape(label)}\s*=\s*\[\s*(\d+),\s*(\d+),\s*(\d+)\s*\]", text)
    return "x".join(match.groups()) if match else ""


def grid_points(grid: str) -> str:
    if not grid:
        return ""
    vals = [int(x) for x in grid.split("x")]
    return str(vals[0] * vals[1] * vals[2])


def parse_section_value(text: str, start_marker: str, value_pattern: str) -> str:
    idx = text.find(start_marker)
    if idx < 0:
        return ""
    section = text[idx: idx + 1800]
    match = re.search(value_pattern, section, flags=re.IGNORECASE)
    return match.group(1) if match else ""


def parse_timer(text: str, cls: str, name: str) -> tuple[str, str]:
    for line in text.splitlines():
        parts = line.split()
        if len(parts) >= 6 and parts[0] == cls and parts[1] == name:
            return parts[2], parts[3]
    return "", ""


def parse_total_timer(text: str) -> str:
    match = re.search(r"^\s*total\s+([0-9.]+)\s+", text, flags=re.MULTILINE)
    return match.group(1) if match else ""


def parse_memory_total_mb(text: str) -> str:
    match = re.search(r"^\s*total\s+([0-9.]+)\s*$", text, flags=re.MULTILINE)
    return match.group(1) if match else ""


def estimate_complex_bytes(count: str, multiplier: int = 1) -> str:
    if not count:
        return ""
    return str(int(count) * multiplier * 16)


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

    metric_rows = []
    for run_dir in sorted((bench / "runs").glob("*/*")):
        case_name = run_dir.parent.name
        cfg_name = run_dir.name
        running = read_text(run_dir / "OUT.autotest" / "running_scf.log")
        input_info = read_text(run_dir / "OUT.autotest" / "INPUT.info")
        stderr = read_text(run_dir / "stderr.log")

        charge_grid = parse_grid(running, "FFT grid for charge/potential")
        dense_grid = parse_grid(running, "fft grid for dense charge/potential")
        if not dense_grid:
            dense_grid = parse_grid(running, "FFT (big) grid for charge/potential")
        wfc_grid = parse_grid(running, "FFT grid for wave functions")

        rho_npw = parse_section_value(
            running,
            "SETUP PLANE WAVES FOR CHARGE/POTENTIAL",
            r"Number of plane waves\s*=\s*(\d+)",
        )
        rho_nst = parse_section_value(
            running,
            "SETUP PLANE WAVES FOR CHARGE/POTENTIAL",
            r"Number of sticks on FFT x-y plane\s*=\s*(\d+)",
        )
        dense_npw = parse_section_value(
            running,
            "SETUP PLANE WAVES FOR DENSE CHARGE/POTENTIAL",
            r"Number of plane waves\s*=\s*(\d+)",
        )
        dense_nst = parse_section_value(
            running,
            "SETUP PLANE WAVES FOR DENSE CHARGE/POTENTIAL",
            r"Number of sticks\s*=\s*(\d+)",
        )
        wfc_npw = parse_section_value(
            running,
            "SETUP PLANE WAVES FOR WAVE FUNCTIONS",
            r"Number of total plane waves\s*=\s*(\d+)",
        )
        wfc_nst = parse_section_value(
            running,
            "SETUP PLANE WAVES FOR WAVE FUNCTIONS",
            r"Number of sticks on FFT x-y plane\s*=\s*(\d+)",
        )

        nbands = parse_input_info(input_info, "nbands")
        nbands_int = int(nbands) if nbands and nbands.isdigit() else 0
        psi_bytes = estimate_complex_bytes(wfc_npw, max(nbands_int, 1))
        rho_bytes = estimate_complex_bytes(rho_npw)
        dense_bytes = estimate_complex_bytes(dense_npw)
        v_bytes = dense_bytes or rho_bytes

        sup_setup_s, sup_setup_calls = parse_timer(running, "PW_Basis_Sup", "setuptransform")
        sup_r2g_s, sup_r2g_calls = parse_timer(running, "PW_Basis_Sup", "real2recip")
        sup_g2r_s, sup_g2r_calls = parse_timer(running, "PW_Basis_Sup", "recip2real")
        k_r2g_s, k_r2g_calls = parse_timer(running, "PW_Basis_K", "real2recip")
        k_g2r_s, k_g2r_calls = parse_timer(running, "PW_Basis_K", "recip2real")

        metric_rows.append({
            "case": case_name,
            "config": cfg_name,
            "mode": "full_complex_baseline",
            "kpt": parse_kpt(run_dir / "KPT"),
            "gamma_only_input_info": parse_input_info(input_info, "gamma_only"),
            "nbands": nbands,
            "nkstot": last_float(r"nkstot\s*=\s*(\d+)", running),
            "nksibz": last_float(r"nkstot now\s*=\s*(\d+)", running),
            "charge_fft_grid": charge_grid,
            "charge_fft_points": grid_points(charge_grid),
            "dense_fft_grid": dense_grid,
            "dense_fft_points": grid_points(dense_grid),
            "wfc_fft_grid": wfc_grid,
            "wfc_fft_points": grid_points(wfc_grid),
            "rho_npw_full": rho_npw,
            "rho_nst_full": rho_nst,
            "dense_npw_full": dense_npw,
            "dense_nst_full": dense_nst,
            "wfc_npw_full": wfc_npw,
            "wfc_nst_full": wfc_nst,
            "rho_complex_bytes_est": rho_bytes,
            "v_complex_bytes_est": v_bytes,
            "psi_complex_bytes_est": psi_bytes,
            "pack_time_s": "N/A",
            "unpack_time_s": "N/A",
            "expanded_tmp_bytes": "N/A",
            "gather_send_bytes": "not_instrumented",
            "gather_recv_bytes": "not_instrumented",
            "wall_s": parse_time(stderr),
            "timer_total_s": parse_total_timer(running),
            "memory_total_mb_reported": parse_memory_total_mb(running),
            "PW_Basis_Sup_setuptransform_s": sup_setup_s,
            "PW_Basis_Sup_setuptransform_calls": sup_setup_calls,
            "PW_Basis_Sup_real2recip_s": sup_r2g_s,
            "PW_Basis_Sup_real2recip_calls": sup_r2g_calls,
            "PW_Basis_Sup_recip2real_s": sup_g2r_s,
            "PW_Basis_Sup_recip2real_calls": sup_g2r_calls,
            "PW_Basis_K_real2recip_s": k_r2g_s,
            "PW_Basis_K_real2recip_calls": k_r2g_calls,
            "PW_Basis_K_recip2real_s": k_g2r_s,
            "PW_Basis_K_recip2real_calls": k_g2r_calls,
        })

    metrics_out = out_dir / "full_complex_baseline_metrics.csv"
    metric_fields = [
        "case", "config", "mode", "kpt", "gamma_only_input_info", "nbands",
        "nkstot", "nksibz", "charge_fft_grid", "charge_fft_points",
        "dense_fft_grid", "dense_fft_points", "wfc_fft_grid",
        "wfc_fft_points", "rho_npw_full", "rho_nst_full",
        "dense_npw_full", "dense_nst_full", "wfc_npw_full", "wfc_nst_full",
        "rho_complex_bytes_est", "v_complex_bytes_est",
        "psi_complex_bytes_est", "pack_time_s", "unpack_time_s",
        "expanded_tmp_bytes", "gather_send_bytes", "gather_recv_bytes",
        "wall_s", "timer_total_s", "memory_total_mb_reported",
        "PW_Basis_Sup_setuptransform_s", "PW_Basis_Sup_setuptransform_calls",
        "PW_Basis_Sup_real2recip_s", "PW_Basis_Sup_real2recip_calls",
        "PW_Basis_Sup_recip2real_s", "PW_Basis_Sup_recip2real_calls",
        "PW_Basis_K_real2recip_s", "PW_Basis_K_real2recip_calls",
        "PW_Basis_K_recip2real_s", "PW_Basis_K_recip2real_calls",
    ]
    with metrics_out.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=metric_fields)
        writer.writeheader()
        writer.writerows(metric_rows)

    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

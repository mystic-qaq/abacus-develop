#!/usr/bin/env bash
set -euo pipefail

REPO="/home/yangxu/abacus-develop"
ROOT="${REPO}/benchmarks/workflow_b/blocking_comm_benchmark_20260523_large_scf10_pinned3"

mkdir -p "${ROOT}/prepared_cases" "${ROOT}/runs/performance" \
    "${ROOT}/runs/correctness" "${ROOT}/tables" "${ROOT}/reports"

python3 - <<'PY'
from pathlib import Path
import shutil

root = Path("/home/yangxu/abacus-develop/benchmarks/workflow_b/blocking_comm_benchmark_20260523_large_scf10_pinned3")
cases = {
    "nacl_uspp": Path("/home/yangxu/abacus-develop/tests/01_PW/008_PW_UPF201_USPP_NaCl"),
    "hcl_uspp": Path("/home/yangxu/abacus-develop/tests/01_PW/009_PW_UPF201_USPP"),
    "si_blps": Path("/home/yangxu/abacus-develop/tests/01_PW/016_PW_BLPS"),
}
replacements = {
    "calculation": "calculation       scf",
    "ecutwfc": "ecutwfc           50",
    "ecutrho": "ecutrho           2000",
    "nbands": "nbands            24",
    "scf_nmax": "scf_nmax          10",
    "scf_thr": "scf_thr           1e-30",
    "out_alllog": "out_alllog        1",
}
ordered_keys = ["calculation", "ecutwfc", "ecutrho", "nbands", "scf_nmax", "scf_thr", "out_alllog"]

for label, src in cases.items():
    dst = root / "prepared_cases" / label
    dst.mkdir(parents=True, exist_ok=True)
    for path in src.iterdir():
        if path.is_file() and path.name != "INPUT":
            shutil.copy2(path, dst / path.name)

    seen = set()
    out = []
    for line in (src / "INPUT").read_text().splitlines():
        stripped = line.strip()
        key = stripped.split()[0] if stripped and not stripped.startswith("#") else ""
        if key in replacements:
            out.append(replacements[key])
            seen.add(key)
        else:
            out.append(line)
    for key in ordered_keys:
        if key not in seen:
            out.append(replacements[key])
    (dst / "INPUT").write_text("\n".join(out) + "\n")
PY

cat > "${ROOT}/benchmark_plan.txt" <<'PLAN'
Workflow B blocking communication benchmark, affinity-fixed rerun

Performance matrix:
  cases: NaCl USPP, HCl/Cl USPP, Si BLPS
  scale: large only, ecutwfc=50 Ry, ecutrho=2000 Ry
  configs: 1x1 2x1 4x1 8x1 4x2 8x2
  repeats: 3
  scf_nmax: 10
  scf_thr: 1e-30
  nbands: 24
  MPI binding: mpirun --map-by slot:PE=${OMP_NUM_THREADS} --bind-to core
  OpenMP binding: OMP_PLACES=cores, OMP_PROC_BIND=spread

Correctness guard:
  cases: NaCl USPP, HCl/Cl USPP, Si BLPS
  configs: 1x1 8x2
  repeats: 1
  scf_nmax: 200
  scf_thr: 1e-13
PLAN

cd "${REPO}"
export OMP_PLACES=cores
export OMP_PROC_BIND=spread
export MPI_REPORT_BINDINGS=1

for label in nacl_uspp hcl_uspp si_blps; do
    out_dir="${ROOT}/runs/performance/${label}"
    echo "[WorkflowB] performance case=${label} out=${out_dir}"
    OUT_DIR="${out_dir}" \
    BASE_CASE="${ROOT}/prepared_cases/${label}" \
    MPIEXEC="${ROOT}/bin/mpirun_pe.sh" \
    SCALES="large:50:2000" \
    CONFIGS="1x1 2x1 4x1 8x1 4x2 8x2" \
    REPEATS=3 \
    SCF_NMAX=10 \
    NBANDS=24 \
    SCF_THR=1e-30 \
    bash tools/workflow_b/run_blocking_comm_baseline.sh
    echo "[WorkflowB] performance done case=${label}"
done

for label in nacl_uspp hcl_uspp si_blps; do
    out_dir="${ROOT}/runs/correctness_tight_e13/${label}"
    echo "[WorkflowB] correctness case=${label} out=${out_dir}"
    OUT_DIR="${out_dir}" \
    BASE_CASE="${ROOT}/prepared_cases/${label}" \
    MPIEXEC="${ROOT}/bin/mpirun_pe.sh" \
    SCALES="large:50:2000" \
    CONFIGS="1x1 8x2" \
    REPEATS=1 \
    SCF_NMAX=200 \
    NBANDS=24 \
    SCF_THR=1e-13 \
    bash tools/workflow_b/run_blocking_comm_baseline.sh
    echo "[WorkflowB] correctness done case=${label}"
done

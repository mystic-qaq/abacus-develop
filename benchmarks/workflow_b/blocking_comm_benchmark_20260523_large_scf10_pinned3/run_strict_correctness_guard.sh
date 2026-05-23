#!/usr/bin/env bash
set -euo pipefail

REPO="/home/yangxu/abacus-develop"
ROOT="${REPO}/benchmarks/workflow_b/blocking_comm_benchmark_20260523_large_scf10_pinned3"

cd "${REPO}"
export OMP_PLACES=cores
export OMP_PROC_BIND=spread
export MPI_REPORT_BINDINGS=1

for label in nacl_uspp hcl_uspp si_blps; do
    out_dir="${ROOT}/runs/correctness_strict/${label}"
    echo "[WorkflowB] strict correctness case=${label} out=${out_dir}"
    OUT_DIR="${out_dir}" \
    BASE_CASE="${ROOT}/prepared_cases/${label}" \
    MPIEXEC="${ROOT}/bin/mpirun_pe.sh" \
    SCALES="large:50:2000" \
    CONFIGS="1x1 8x2" \
    REPEATS=1 \
    SCF_NMAX=200 \
    NBANDS=24 \
    SCF_THR=1e-14 \
    bash tools/workflow_b/run_blocking_comm_baseline.sh
    echo "[WorkflowB] strict correctness done case=${label}"
done

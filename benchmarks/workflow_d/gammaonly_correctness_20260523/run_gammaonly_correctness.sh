#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BENCH_DIR="${ROOT_DIR}/benchmarks/workflow_d/gammaonly_correctness_20260523"
ABACUS_BIN="${ABACUS_BIN:-${ROOT_DIR}/build-current-abacus-mpi-local/abacus_pw_para}"
MPIRUN="${MPIRUN:-/usr/bin/mpirun.openmpi}"
PSEUDO_DIR="${ROOT_DIR}/tests/PP_ORB"

CASES=(
  "si_gamma_1x1x1"
  "si_mixed_direct_gamma_plus_k"
  "si_multik_2x2x2"
  "nacl_multik_1x1x2"
)

CONFIGS=(
  "np1_omp1:1:1"
  "np4_omp1:4:1"
)

mkdir -p "${BENCH_DIR}/runs"

for case_name in "${CASES[@]}"; do
  for cfg in "${CONFIGS[@]}"; do
    IFS=: read -r cfg_name np omp <<<"${cfg}"
    run_dir="${BENCH_DIR}/runs/${case_name}/${cfg_name}"
    rm -rf "${run_dir}"
    mkdir -p "$(dirname "${run_dir}")"
    cp -a "${BENCH_DIR}/prepared_cases/${case_name}" "${run_dir}"

    # Keep benchmark runs independent of their original tests/01_PW depth.
    perl -0pi -e "s|^pseudo_dir\s+.*$|pseudo_dir        ${PSEUDO_DIR}|m" "${run_dir}/INPUT"

    (
      cd "${run_dir}"
      export OMP_NUM_THREADS="${omp}"
      export OMP_PLACES=cores
      export OMP_PROC_BIND=spread
      /usr/bin/time -p \
        "${MPIRUN}" --map-by "slot:PE=${omp}" --bind-to core --report-bindings \
        -np "${np}" "${ABACUS_BIN}" \
        > stdout.log 2> stderr.log
    ) || {
      echo "Run failed: ${case_name}/${cfg_name}" >&2
      exit 1
    }
  done
done

python3 "${BENCH_DIR}/summarize_gammaonly_correctness.py" "${BENCH_DIR}"

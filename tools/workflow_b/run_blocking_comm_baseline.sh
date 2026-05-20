#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ABACUS_BIN="${ABACUS_BIN:-${ROOT_DIR}/build-current-abacus-mpi-local/abacus_pw_para}"
MPIEXEC="${MPIEXEC:-/usr/bin/mpirun.openmpi}"
BASE_CASE="${BASE_CASE:-${ROOT_DIR}/tests/01_PW/008_PW_UPF201_USPP_NaCl}"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/benchmarks/workflow_b/blocking_comm_baseline_$(date +%Y%m%d_%H%M%S)}"
CONFIGS="${CONFIGS:-2x1 4x1 8x1}"
SCALES="${SCALES:-single:${ECUTWFC:-8}:${ECUTRHO:-160}}"
REPEATS="${REPEATS:-1}"
SCF_NMAX="${SCF_NMAX:-4}"
SCF_THR="${SCF_THR:-1e-30}"
NBANDS="${NBANDS:-16}"
ECUTWFC="${ECUTWFC:-8}"
ECUTRHO="${ECUTRHO:-160}"

if [[ ! -x "${ABACUS_BIN}" ]]; then
    echo "ABACUS binary is not executable: ${ABACUS_BIN}" >&2
    exit 1
fi
if [[ ! -x "${MPIEXEC}" ]]; then
    echo "MPI launcher is not executable: ${MPIEXEC}" >&2
    exit 1
fi

mkdir -p "${OUT_DIR}"
{
    echo "ABACUS_BIN=${ABACUS_BIN}"
    echo "MPIEXEC=${MPIEXEC}"
    echo "BASE_CASE=${BASE_CASE}"
    echo "CONFIGS=${CONFIGS}"
    echo "SCALES=${SCALES}"
    echo "REPEATS=${REPEATS}"
    echo "SCF_NMAX=${SCF_NMAX}"
    echo "SCF_THR=${SCF_THR}"
    echo "NBANDS=${NBANDS}"
    date
} > "${OUT_DIR}/benchmark_meta.txt"

case_dirs=()
for scale in ${SCALES}; do
    IFS=':' read -r scale_name scale_ecutwfc scale_ecutrho <<< "${scale}"
    if [[ -z "${scale_name}" || -z "${scale_ecutwfc}" || -z "${scale_ecutrho}" ]]; then
        echo "Invalid SCALES entry: ${scale}. Expected name:ecutwfc:ecutrho" >&2
        exit 1
    fi
    for rep in $(seq 1 "${REPEATS}"); do
        rep_label="$(printf '%02d' "${rep}")"
        for cfg in ${CONFIGS}; do
            np="${cfg%x*}"
            omp="${cfg#*x}"
            case_dir="${OUT_DIR}/nacl_${scale_name}_rep${rep_label}_np${np}_omp${omp}"
            mkdir -p "${case_dir}"
            cp "${BASE_CASE}/STRU" "${BASE_CASE}/KPT" "${case_dir}/"

            awk \
                -v suffix="workflowB_${scale_name}_r${rep_label}_np${np}_omp${omp}" \
                -v pseudo_dir="${ROOT_DIR}/tests/PP_ORB" \
                -v scf_nmax="${SCF_NMAX}" \
                -v scf_thr="${SCF_THR}" \
                -v nbands="${NBANDS}" \
                -v ecutwfc="${scale_ecutwfc}" \
                -v ecutrho="${scale_ecutrho}" '
                BEGIN { saw_out_alllog = 0 }
                $1 == "suffix" { print "suffix            " suffix; next }
                $1 == "pseudo_dir" { print "pseudo_dir        " pseudo_dir; next }
                $1 == "scf_nmax" { print "scf_nmax          " scf_nmax; next }
                $1 == "scf_thr" { print "scf_thr           " scf_thr; next }
                $1 == "nbands" { print "nbands            " nbands; next }
                $1 == "ecutwfc" { print "ecutwfc           " ecutwfc; next }
                $1 == "ecutrho" { print "ecutrho           " ecutrho; next }
                $1 == "out_alllog" { print "out_alllog        1"; saw_out_alllog = 1; next }
                { print }
                END {
                    if (!saw_out_alllog) {
                        print "out_alllog        1"
                    }
                }
                ' "${BASE_CASE}/INPUT" > "${case_dir}/INPUT"

            echo "[WorkflowB] running ${case_dir} scale=${scale_name} rep=${rep_label} np=${np} OMP_NUM_THREADS=${omp}"
            (
                cd "${case_dir}"
                export ABACUS_TIMER_PRINT_ALL=1
                export OMP_NUM_THREADS="${omp}"
                /usr/bin/time -p "${MPIEXEC}" -np "${np}" "${ABACUS_BIN}" > stdout.log 2> stderr.log
            )
            case_dirs+=("${case_dir}")
        done
    done
done

python3 "${ROOT_DIR}/tools/workflow_b/parse_pw_comm_baseline.py" "${case_dirs[@]}" --out-dir "${OUT_DIR}"
echo "[WorkflowB] report: ${OUT_DIR}/blocking_comm_baseline_report.md"

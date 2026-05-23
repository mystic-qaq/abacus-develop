#!/usr/bin/env bash
set -euo pipefail

pe="${OMP_NUM_THREADS:-1}"
args=(--map-by "slot:PE=${pe}" --bind-to core)
if [[ "${MPI_REPORT_BINDINGS:-0}" == "1" ]]; then
    args+=(--report-bindings)
fi

exec /usr/bin/mpirun.openmpi "${args[@]}" "$@"

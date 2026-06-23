#!/usr/bin/env bash
set -euo pipefail

task7_repo=${TASK7_REPO:-$(cd "$(dirname "$0")/.." && pwd)}
baseline_repo=${BASELINE_REPO:-/root/wt-upstream-develop}
task7_exe=${TASK7_EXE:-"$task7_repo/build-task7/abacus_pw_para"}
baseline_exe=${BASELINE_EXE:-"$baseline_repo/build-upstream/abacus_pw_para"}
out_root=${ABACUS_BENCH_ROOT:-/root/abacus_validation_runs/perf_task7_multiblock_004_ecut1500_np2_blockbuf}
repeats=${ABACUS_BENCH_REPEATS:-3}
np=${ABACUS_BENCH_NP:-2}
ecut=${ABACUS_BENCH_ECUT:-1500}

rm -rf "$out_root"
mkdir -p "$out_root"
ln -sfn "$task7_repo/tests/PP_ORB" "$out_root/PP_ORB"

printf 'build,rep,rc,energy,wall,timers\n'
for build in baseline task7; do
    if [ "$build" = baseline ]; then
        exe="$baseline_exe"
    else
        exe="$task7_exe"
    fi

    for rep in $(seq 1 "$repeats"); do
        case_name="004_PW_UPF201_Si_ecut${ecut}"
        run_parent="$out_root/${build}_rep${rep}"
        mkdir -p "$run_parent"
        cp -a "$task7_repo/tests/01_PW/004_PW_UPF201_Si" "$run_parent/$case_name"
        run_dir="$run_parent/$case_name"
        cd "$run_dir"

        perl -0pi -e "s/ecutwfc\\s+\\d+/ecutwfc           ${ecut}/; s/scf_nmax\\s+\\d+/scf_nmax          1/" INPUT

        set +e
        OMP_NUM_THREADS=1 /usr/bin/time -f '%e' -o wall.txt \
            mpirun --allow-run-as-root --bind-to core --map-by core -np "$np" "$exe" > run.log 2>&1
        rc=$?
        set -e

        wall=$(tail -n1 wall.txt 2>/dev/null || printf nan)
        energy=$(awk '/FINAL_ETOT_IS/{e=$2} END{if(e=="") e="nan"; print e}' OUT.*/running_*.log 2>/dev/null || printf nan)
        timers=$(rg -o 'gatherp_overlap_comm|gathers_overlap_comm|single_block_fallback|gatherp_alltoallv|gathers_alltoallv' run.log OUT.* -g '*' 2>/dev/null \
            | sort | uniq -c | tr '\n' ';' || true)
        printf '%s,%s,%s,%s,%s,%s\n' "$build" "$rep" "$rc" "$energy" "$wall" "$timers"
    done
done

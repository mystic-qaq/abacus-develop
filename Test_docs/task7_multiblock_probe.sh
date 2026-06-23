#!/usr/bin/env bash
set -euo pipefail

repo=${ABACUS_REPO:-$(cd "$(dirname "$0")/.." && pwd)}
exe=${ABACUS_EXE:-"$repo/build-task7/abacus_pw_para"}
out_root=${ABACUS_PROBE_ROOT:-/root/abacus_validation_runs/task7_multiblock_probe}
np=${ABACUS_PROBE_NP:-8}
timeout_s=${ABACUS_PROBE_TIMEOUT:-90}
cases=${ABACUS_PROBE_CASES:-"P004_cu4_pw P005_Bi2Se2Cu2O2_pw P009_32H2O_pw P000_si16_pw"}

rm -rf "$out_root"
mkdir -p "$out_root"
ln -sfn "$repo/tests/PP_ORB" "$out_root/PP_ORB"

printf 'case,rc,energy,wall,path,timers\n'
for case_name in $cases; do
    run_parent="$out_root/$case_name"
    mkdir -p "$run_parent"
    cp -a "$repo/tests/performance/$case_name" "$run_parent/"
    run_dir="$run_parent/$case_name"
    cd "$run_dir"

    perl -0pi -e 's/scf_nmax\s+\d+/scf_nmax          1/' INPUT

    set +e
    OMP_NUM_THREADS=1 timeout "$timeout_s"s /usr/bin/time -f '%e' -o wall.txt \
        mpirun --allow-run-as-root --oversubscribe -np "$np" "$exe" > run.log 2>&1
    rc=$?
    set -e

    wall=$(tail -n1 wall.txt 2>/dev/null || printf nan)
    energy=$(awk '/FINAL_ETOT_IS/{e=$2} END{if(e=="") e="nan"; print e}' OUT.*/running_*.log 2>/dev/null || printf nan)
    timers=$(rg -o 'gatherp_overlap_comm|gathers_overlap_comm|single_block_fallback' run.log OUT.* -g '*' 2>/dev/null \
        | sort | uniq -c | tr '\n' ';' || true)
    printf '%s,%s,%s,%s,%s,%s\n' "$case_name" "$rc" "$energy" "$wall" "$run_dir" "$timers"
done

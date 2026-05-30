#!/usr/bin/env bash
# collect_env.sh
# 收集测试环境信息
# 输出: output/baseline_logs/environment.txt

set -u
ABACUS_ROOT="$(pwd)"
OUTDIR="$ABACUS_ROOT/benchmarks/workflowA"
mkdir -p "$OUTDIR"

OUTFILE="$OUTDIR/environment.txt"

cat > "$OUTFILE" <<EOF
# Environment
commit=$(git -C "$ABACUS_ROOT" rev-parse HEAD 2>/dev/null || echo "N/A")

## uname
$(uname -a)

## nproc
$(nproc)

## free -h
$(free -h)

## lscpu
$(lscpu)

## compiler
$(mpicxx --version 2>&1)

## mpi
$(mpirun --version 2>&1)

## cmake
$(cmake --version 2>&1)
EOF

echo "Environment info saved to: $OUTFILE"

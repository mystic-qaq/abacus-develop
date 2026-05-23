#!/usr/bin/env bash
# run_correctness.sh
# 运行 ABACUS PW 模块 8 个核心用例的正确性验证

set -u
ABACUS_ROOT="$(pwd)"
OUTDIR="$ABACUS_ROOT/benchmarks/workflowA/correctness_result"
mkdir -p "$OUTDIR"

# 可执行文件检测
EXEC="$ABACUS_ROOT/build_baseline/abacus_pw_para"
if [ ! -x "$EXEC" ]; then
    echo "ERROR: 可执行文件不存在: $EXEC"
    echo "请先完成编译: cmake --build build_baseline"
    exit 1
fi

# 写入 general_info（自动适配 abacus_pw_para）
cat > "$ABACUS_ROOT/tests/integrate/general_info" <<EOF
EXEC $EXEC
CHECKACCURACY 2
NUMBEROFPROCESS 4
EOF

# 修复可能的 CRLF
sed -i 's/\r$//' "$ABACUS_ROOT/tests/integrate/general_info"
sed -i 's/\r$//' "$ABACUS_ROOT/tests/integrate/Single_job.sh"
sed -i 's/\r$//' "$ABACUS_ROOT/tests/integrate/tools/run_check.sh"

TEST_DIRS=(
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/001_PW_UPF100_Al"           # fcc, Gamma, 最简单基线
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/004_PW_UPF201_Si"           # diamond, 半导体标准
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/210_PW_kspace_shift"        # k空间偏移
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/020_PW_kspace"              # 多k点
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/021_PW_kspace3"             # 更多k点
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/026_PW_KPAR"                # KPAR并行
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/801_PW_LT_sc"               # 简单立方
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/814_PW_LT_triclinic"        # 三斜, 最低对称

    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/802_PW_LT_fcc"              # fcc（与Al同晶格，验证一致性）
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/803_PW_LT_bcc"              # bcc
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/804_PW_LT_hex"              # 六角晶格
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/805_PW_LT_trigonal"         # 三方晶格
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/808_PW_LT_so"               # 正交晶格
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/810_PW_LT_fco"              # 面心正交

    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/002_PW_UPF100_RAPPE_Fe"      # Fe, nspin=2磁性, 比Al/Si重
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/057_PW_SO_IW"               # 自旋轨道耦合, 破坏时间反演
    "$ABACUS_ROOT/benchmarks/workflowA/correctness_cases/087_PW_get_pchg_kpar"       # kpar+电荷输出, FFT负载更重
)

export OMP_NUM_THREADS=2
PASS=0
FAIL=0
SUMMARY="$OUTDIR/summary.csv"
echo "case,status,exit_code" > "$SUMMARY"

for dir in "${TEST_DIRS[@]}"; do
    case_name=$(basename "$dir")
    echo "========================================"
    echo "Testing: $dir"
    cd "$ABACUS_ROOT/$dir" || { echo "$case_name,FAIL,cd_failed" >> "$SUMMARY"; continue; }

    rm -f result.out log.txt
    bash ../../integrate/Single_job.sh > log.txt 2>&1
    RET=$?

    mkdir -p "$OUTDIR/$case_name"
    cp -f log.txt "$OUTDIR/$case_name/" 2>/dev/null || true
    cp -f result.out "$OUTDIR/$case_name/" 2>/dev/null || true
    cp -f result.ref "$OUTDIR/$case_name/" 2>/dev/null || true

    if [ $RET -eq 0 ]; then
        echo "[PASS] $dir"
        echo "$case_name,PASS,$RET" >> "$SUMMARY"
        PASS=$((PASS+1))
    else
        echo "[FAIL] $dir exit=$RET"
        echo "$case_name,FAIL,$RET" >> "$SUMMARY"
        FAIL=$((FAIL+1))
    fi
done

cd "$ABACUS_ROOT"
echo "========================================"
echo "Summary: PASS=$PASS, FAIL=$FAIL"
echo "详细日志: $OUTDIR/"

#!/usr/bin/env bash
# 性能测试：保存完整 stdout + 解析 TIME STATISTICS + 变规模支持

set -u
ABACUS_ROOT="$(pwd)"
ABACUS="$ABACUS_ROOT/build_baseline/abacus_pw_para"
OUTROOT="$ABACUS_ROOT/benchmarks/workflowA/perf_result"
mkdir -p "$OUTROOT"

if [ ! -x "$ABACUS" ]; then
    echo "ERROR: 可执行文件不存在: $ABACUS"
    exit 1
fi

MPIRUN="/usr/bin/mpirun"
if [ ! -x "$MPIRUN" ]; then
    MPIRUN="mpirun"
fi

# ============================================================================
# 测试用例：4 标准 + 6 变规模
# ============================================================================
CASES=(
    # 标准用例
    # "/tests/01_PW/001_PW_UPF100_Al"
    # "/tests/01_PW/004_PW_UPF201_Si"
    # "/tests/01_PW/026_PW_KPAR"
    # "/tests/01_PW/814_PW_LT_triclinic"
    # 变规模系列（Si）
    "/benchmarks/workflowA/perf_cases/004_Si_small"
    "/benchmarks/workflowA/perf_cases/004_Si_medium"
    "/benchmarks/workflowA/perf_cases/004_Si_large"
)

CONFIGS=(
    "1:1"
    "4:1"
    "4:2"
    "2:4"
)

REPS=3
TIMEOUT=300  # 5分钟超时

# ============================================================================
# 辅助函数：标准用例改参（增大负载）
# ============================================================================
modify_standard_input() {
    local input_file="$1"
    if [ ! -f "${input_file}.bak.perf" ]; then
        cp "$input_file" "${input_file}.bak.perf"
    fi

    local old_ecut=$(grep -E "^\s*ecutwfc" "$input_file" | awk '{print $2}' | head -1)
    local new_ecut=60
    if [ -n "$old_ecut" ]; then
        local is_large=$(awk -v x="$old_ecut" 'BEGIN {print (x>30)?1:0}')
        [ "$is_large" = "1" ] && new_ecut=80
    fi
    local new_rho=$(awk -v x="$new_ecut" 'BEGIN {printf "%d", x*4}')

    local tmpfile="${input_file}.tmp"
    while IFS= read -r line || [ -n "$line" ]; do
        local stripped=$(echo "$line" | sed 's/^[[:space:]]*//')
        if echo "$stripped" | grep -qE "^ecutwfc"; then
            printf "ecutwfc\t\t%d\n" "$new_ecut" >> "$tmpfile"
        elif echo "$stripped" | grep -qE "^ecutrho"; then
            printf "ecutrho\t\t%d\n" "$new_rho" >> "$tmpfile"
        elif echo "$stripped" | grep -qE "^scf_nmax"; then
            printf "scf_nmax\t\t20\n" >> "$tmpfile"
        elif echo "$stripped" | grep -qE "^scf_thr"; then
            printf "scf_thr\t\t1e-30\n" >> "$tmpfile"
        else
            printf "%s\n" "$line" >> "$tmpfile"
        fi
    done < "$input_file"
    mv "$tmpfile" "$input_file"
    echo "    [MODIFY] ecutwfc ${old_ecut:-N/A} -> $new_ecut, scf_nmax=20"
}

restore_input() {
    local input_file="$1"
    [ -f "${input_file}.bak.perf" ] && mv "${input_file}.bak.perf" "$input_file"
}

# ============================================================================
# 主循环
# ============================================================================
SUMMARY="$OUTROOT/perf_summary.csv"
echo "case,np,nt,run,elapsed_sec,max_rss_kb,exit_code" > "$SUMMARY"

for case_dir in "${CASES[@]}"; do
    case_name=$(basename "$case_dir")
    echo "========================================"
    echo "Case: $case_name"

    cd "$ABACUS_ROOT/$case_dir" || continue

    # 判断是否需要改参（标准用例需要，变规模用例不需要）
    if [[ "$case_name" == "001_PW_UPF100_Al" || "$case_name" == "004_PW_UPF201_Si" || "$case_name" == "026_PW_KPAR" || "$case_name" == "814_PW_LT_triclinic" ]]; then
        [ -f "INPUT" ] && modify_standard_input "INPUT"
    fi

    mkdir -p "$OUTROOT/$case_name"

    for cfg in "${CONFIGS[@]}"; do
        IFS=':' read -r np nt <<< "$cfg"
        export OMP_NUM_THREADS=$nt

        for run in $(seq 1 $REPS); do
            echo "  np=$np nt=$nt run=$run"
            rm -rf OUT.* log.txt

            # 输出文件命名
            prefix="$OUTROOT/$case_name/np${np}_nt${nt}_r${run}"
            stdout_file="${prefix}.stdout"
            stderr_file="${prefix}.stderr"
            time_file="${prefix}.time"

            # 运行：保存完整 stdout/stderr，同时用 time -v 计时
            if [ "$np" -eq 1 ]; then
                timeout $TIMEOUT /usr/bin/time -v "$ABACUS" > "$stdout_file" 2> "$time_file"
                RET=$?
            else
                timeout $TIMEOUT /usr/bin/time -v "$MPIRUN" --bind-to none -np "$np" "$ABACUS" > "$stdout_file" 2> "$time_file"
                RET=$?
            fi

            # 把 time -v 的输出追加到 stderr 文件以便归档
            cat "$time_file" >> "$stderr_file" 2>/dev/null || true
            rm -f "$time_file"

            # 解析 wall time
            elapsed_raw=$(grep "Elapsed (wall clock) time" "$stderr_file" | awk -F': ' '{print $2}' | tail -1)
            elapsed=""
            if [[ "$elapsed_raw" =~ ^[0-9]+:[0-9]+:[0-9]+ ]]; then
                elapsed=$(echo "$elapsed_raw" | awk -F':' '{print ($1*3600)+($2*60)+$3}')
            elif [[ "$elapsed_raw" =~ ^[0-9]+:[0-9]+\.[0-9]+ ]]; then
                elapsed=$(echo "$elapsed_raw" | awk -F':' '{print ($1*60)+$2}')
            elif [[ "$elapsed_raw" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                elapsed="$elapsed_raw"
            fi

            # 解析内存
            rss=$(grep "Maximum resident set size" "$stderr_file" | awk '{print $6}' | tail -1)

            # 检查 TIME STATISTICS 是否存在
            has_timer="NO"
            grep -q "TIME STATISTICS" "$stdout_file" && has_timer="YES"

            echo "$case_name,$np,$nt,$run,$elapsed,$rss,$RET" >> "$SUMMARY"
            echo "    elapsed=${elapsed}s rss=${rss}KB exit=$RET timer=$has_timer"

            # 清理 OUT.ABACUS 节省空间
            rm -rf OUT.ABACUS
        done
    done

    # 恢复标准用例的原始 INPUT
    if [[ "$case_name" == "001_PW_UPF100_Al" || "$case_name" == "004_PW_UPF201_Si" || "$case_name" == "026_PW_KPAR" || "$case_name" == "814_PW_LT_triclinic" ]]; then
        restore_input "INPUT"
    fi

    cd "$ABACUS_ROOT"
done

echo "========================================"
echo "Summary: $SUMMARY"
echo "Total runs: $(tail -n +2 "$SUMMARY" | wc -l)"
echo ""
echo "Next step: parse internal timers with:"
echo "  python3 /mnt/agents/output/parse_abacus_timers.py"

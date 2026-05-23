#!/usr/bin/env bash
# setup_scaling_cases.sh
# 基于 Si 和 NaCl 创建 small/medium/large 变规模测试用例

set -e
ABACUS_ROOT="$(pwd)"

# ============================================================================
# 配置：原始用例路径
# ============================================================================
SI_ORIG="$ABACUS_ROOT/tests/01_PW/004_PW_UPF201_Si"
NACL_ORIG="$ABACUS_ROOT/tests/01_PW/008_PW_UPF201_USPP_NaCl"

# 如果路径不对，自动搜索
if [ ! -d "$SI_ORIG" ]; then
    SI_ORIG=$(find "$ABACUS_ROOT/tests/01_PW" -maxdepth 1 -type d -name "*Si*" | head -1)
fi
if [ ! -d "$NACL_ORIG" ]; then
    NACL_ORIG=$(find "$ABACUS_ROOT/tests/01_PW" -maxdepth 1 -type d -name "*NaCl*" | head -1)
fi

echo "Si source:  $SI_ORIG"
echo "NaCl source: $NACL_ORIG"

if [ -z "$SI_ORIG" ] || [ ! -d "$SI_ORIG" ]; then
    echo "ERROR: Cannot find Si test case in tests/01_PW/"
    exit 1
fi
if [ -z "$NACL_ORIG" ] || [ ! -d "$NACL_ORIG" ]; then
    echo "ERROR: Cannot find NaCl test case in tests/01_PW/"
    exit 1
fi

# ============================================================================
# 辅助函数：创建变规模用例
# ============================================================================
create_scale_case() {
    local orig_dir="$1"
    local target_dir="$2"
    local ecutwfc="$3"
    local ecutrho="$4"

    echo "Creating: $target_dir (ecutwfc=$ecutwfc, ecutrho=$ecutrho)"

    mkdir -p "$target_dir"

    # 复制必要文件
    cp "$orig_dir/STRU" "$target_dir/" 2>/dev/null || true
    cp "$orig_dir/KPT" "$target_dir/" 2>/dev/null || true
    cp "$orig_dir/README" "$target_dir/" 2>/dev/null || true

    # 复制赝势文件（支持多种命名）
    find "$orig_dir" -maxdepth 1 -name "*.upf" -o -name "*.UPF" -o -name "*.vwr" | while read -r f; do
        cp "$f" "$target_dir/"
    done

    # 读取原始 INPUT 中的其他参数（保留非截断能参数）
    local old_input="$orig_dir/INPUT"

    # 生成新的 INPUT
    cat > "$target_dir/INPUT" <<EOF
INPUT_PARAMETERS
#Parameters (1.General)
suffix            autotest
calculation       scf
EOF

    # 从原始 INPUT 复制 nbands、symmetry、latname 等通用参数
    grep -E "^\s*(nbands|symmetry|latname|pseudo_dir|pw_seed)\s" "$old_input" >> "$target_dir/INPUT" 2>/dev/null || true

    # 添加变规模参数
    cat >> "$target_dir/INPUT" <<EOF

#Parameters (2.Iteration)
ectwfc            $ecutwfc
ecutrho           $ecutrho
scf_thr           1e-30
scf_nmax          20

#Parameters (3.Basis)
basis_type        pw

EOF

    # 从原始 INPUT 复制 smearing、mixing 等参数
    grep -E "^\s*(smearing_method|smearing_sigma|mixing_type|mixing_beta|cal_force|cal_stress|pseudo_mesh|pseudo_rcut)\s" "$old_input" >> "$target_dir/INPUT" 2>/dev/null || true

    echo "  Done: $target_dir/INPUT created"
}

# ============================================================================
# 创建 Si 变规模系列
# ============================================================================
echo ""
echo "=== Si Scaling Series ==="
create_scale_case "$SI_ORIG" "$ABACUS_ROOT/benchmarks/workflowA/perf_tests/004_Si_small"  20  80
create_scale_case "$SI_ORIG" "$ABACUS_ROOT/benchmarks/workflowA/perf_tests/004_Si_medium" 60 240
create_scale_case "$SI_ORIG" "$ABACUS_ROOT/benchmarks/workflowA/perf_tests/004_Si_large"  100 400

# ============================================================================
# 创建 NaCl 变规模系列
# ============================================================================
echo ""
echo "=== NaCl Scaling Series ==="
create_scale_case "$NACL_ORIG" "$ABACUS_ROOT/benchmarks/workflowA/perf_tests/008_NaCl_small"  20  80
create_scale_case "$NACL_ORIG" "$ABACUS_ROOT/benchmarks/workflowA/perf_tests/008_NaCl_medium" 60 240
create_scale_case "$NACL_ORIG" "$ABACUS_ROOT/benchmarks/workflowA/perf_tests/008_NaCl_large"  100 400

echo ""
echo "All scaling cases created successfully!"
echo ""
echo "Created directories:"
ls -d "$ABACUS_ROOT/tests/01_PW/004_Si_*" "$ABACUS_ROOT/tests/01_PW/008_NaCl_*" 2>/dev/null
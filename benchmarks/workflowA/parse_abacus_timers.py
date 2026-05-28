#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
parse_abacus_timers.py
从 ABACUS stdout 中提取 TIME STATISTICS 计时表，生成结构化 CSV。

用法:
    python3 parse_abacus_timers.py \
        --input output/baseline_logs/perf \
        --output output/baseline_logs/perf/timers_parsed.csv
"""

import os
import re
import csv
import argparse
from pathlib import Path
from collections import defaultdict

# 需要提取的关键 timer
TARGET_TIMERS = {
    # gather/scatter 相关（题1/题3）
    "gatherp_scatters",
    "gathers_scatterp",
    "gatherp_pack",
    "gatherp_alltoallv",
    "gatherp_unpack",
    "gathers_pack",
    "gathers_alltoallv",
    "gathers_clear",
    "gathers_unpack",
    # FFT 变换
    "real2recip",
    "recip2real",
    # 对角化（Workflow D 范畴，用于占比参考）
    "diag_once",
    "diag_subspace",
    # hPsi 和 SCF 迭代
    "hPsi",
    "hamilt2rho",
    "before_scf",
    "initialize_psi",
}


def extract_timer_table(stdout_text):
    """
    从 stdout 文本中提取 TIME STATISTICS 表格。
    返回: list of dict, 每个 dict 是一条 timer 记录
    """
    records = []

    # 找 TIME STATISTICS 区块
    # 格式: 表头行 + 分隔线 + 数据行 + 空行或下一个表头
    pattern = r"TIME STATISTICS\n-+\n(.*?)\n(?:\n|Total\n|STEP OF RELAXATION)"
    match = re.search(pattern, stdout_text, re.DOTALL)
    if not match:
        return records

    table_text = match.group(1)
    lines = table_text.strip().splitlines()

    # 跳过表头行（如果有 CLASS_NAME NAME TIME...）
    data_started = False
    for line in lines:
        line = line.strip()
        if not line:
            continue
        if "CLASS_NAME" in line or "NAME" in line and "TIME" in line:
            data_started = True
            continue
        if "---" in line:
            continue

        # 解析数据行
        # 格式: CLASS_NAME  NAME  TIME(Sec)  CALLS  AVG(Sec)  PER%
        # 注意：NAME 可能包含空格？不，通常没有
        parts = line.split()
        if len(parts) < 6:
            continue

        # 最后几列是数字，前面是 class_name 和 name
        # 从后往前解析更可靠
        try:
            per = float(parts[-1].replace("%", ""))
            avg = float(parts[-2])
            calls = int(parts[-3])
            time_sec = float(parts[-4])
            # name 是倒数第5个（如果 class_name 只有一个词）
            # 但 class_name 可能有下划线，如 PW_Basis_K
            name = parts[-5]
            class_name = " ".join(parts[:-5]) if len(parts) > 5 else ""

            records.append({
                "class_name": class_name,
                "name": name,
                "time_sec": time_sec,
                "calls": calls,
                "avg_sec": avg,
                "percentage": per,
            })
        except (ValueError, IndexError):
            continue

    return records


def parse_all_runs(perf_dir):
    """
    遍历 perf 目录下的所有 .stdout 文件，解析 timer。
    返回: list of dict，每条记录包含 case/np/nt/run + timer 信息
    """
    perf_path = Path(perf_dir)
    results = []

    for stdout_file in perf_path.rglob("*.stdout"):
        # 从路径解析 case/np/nt/run
        # 路径格式: .../perf/<case>/np<X>_nt<Y>_r<Z>.stdout
        rel = stdout_file.relative_to(perf_path)
        parts = rel.parts
        if len(parts) < 2:
            continue

        case_name = parts[0]
        filename = parts[1] if len(parts) > 1 else str(rel.name)

        # 从文件名解析 np, nt, run
        # 格式: np4_nt2_r1.stdout
        m = re.search(r"np(\d+)_nt(\d+)_r(\d+)", filename)
        if not m:
            continue
        np, nt, run = m.group(1), m.group(2), m.group(3)

        # 读取 stdout
        try:
            text = stdout_file.read_text(encoding="utf-8", errors="ignore")
        except Exception as e:
            print(f"  [WARN] Cannot read {stdout_file}: {e}")
            continue

        # 提取 timer
        timers = extract_timer_table(text)

        # 过滤目标 timer
        for t in timers:
            if t["name"] in TARGET_TIMERS:
                results.append({
                    "case": case_name,
                    "np": np,
                    "nt": nt,
                    "run": run,
                    "class_name": t["class_name"],
                    "timer_name": t["name"],
                    "time_sec": t["time_sec"],
                    "calls": t["calls"],
                    "avg_sec": t["avg_sec"],
                    "percentage": t["percentage"],
                })

    return results


def aggregate_timers(records):
    """
    按 case + np + nt + timer_name 聚合，计算 mean/stdev。
    """
    groups = defaultdict(list)
    for r in records:
        key = (r["case"], r["np"], r["nt"], r["timer_name"])
        groups[key].append(r["time_sec"])

    aggregated = []
    for (case, np, nt, timer), times in groups.items():
        n = len(times)
        mean = sum(times) / n if n > 0 else 0
        # 样本标准差
        if n > 1:
            variance = sum((x - mean) ** 2 for x in times) / (n - 1)
            stdev = variance ** 0.5
        else:
            stdev = 0.0

        aggregated.append({
            "case": case,
            "np": np,
            "nt": nt,
            "timer_name": timer,
            "runs": n,
            "mean_sec": round(mean, 6),
            "stdev_sec": round(stdev, 6),
            "min_sec": round(min(times), 6) if times else 0,
            "max_sec": round(max(times), 6) if times else 0,
        })

    return aggregated


def main():
    parser = argparse.ArgumentParser(
        description="Parse ABACUS TIME STATISTICS from perf stdout files")
    parser.add_argument("--input", default="benchmarks/workflowA/perf_result",
                        help="perf output directory")
    parser.add_argument(
        "--output", default="benchmarks/workflowA/perf_result/timers_parsed.csv", help="output CSV")
    args = parser.parse_args()

    print(f"[INFO] Scanning: {args.input}")
    records = parse_all_runs(args.input)
    print(f"[INFO] Found {len(records)} target timer records")

    if not records:
        print("[WARN] No timer data found. Check if stdout files contain 'TIME STATISTICS'")
        return

    # 保存原始明细
    detail_path = Path(args.output).with_suffix(".detail.csv")
    with open(detail_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "case", "np", "nt", "run", "class_name", "timer_name",
            "time_sec", "calls", "avg_sec", "percentage"
        ])
        writer.writeheader()
        writer.writerows(records)
    print(f"[OK] Detail saved: {detail_path}")

    # 保存聚合统计
    aggregated = aggregate_timers(records)
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "case", "np", "nt", "timer_name", "runs",
            "mean_sec", "stdev_sec", "min_sec", "max_sec"
        ])
        writer.writeheader()
        writer.writerows(aggregated)
    print(f"[OK] Aggregated summary saved: {args.output}")

    # 打印关键发现
    print("\n[KEY FINDINGS]")
    for case in sorted(set(r["case"] for r in records)):
        # 找 np1_omp1 的 gatherp_scatters 和 real2recip
        g_times = [r["time_sec"] for r in records
                   if r["case"] == case and r["np"] == "1" and r["nt"] == "1"
                   and r["timer_name"] == "gatherp_scatters"]
        r_times = [r["time_sec"] for r in records
                   if r["case"] == case and r["np"] == "1" and r["nt"] == "1"
                   and r["timer_name"] == "real2recip"]
        if g_times and r_times:
            ratio = sum(g_times) / sum(r_times) * 100
            print(f"  {case} (serial): gather/scatter = {ratio:.1f}% of real2recip")


if __name__ == "__main__":
    main()

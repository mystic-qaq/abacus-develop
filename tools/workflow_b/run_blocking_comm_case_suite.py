#!/usr/bin/env python3
"""Run Workflow B blocking-communication benchmarks for multiple ABACUS cases."""

from __future__ import annotations

import argparse
import csv
import os
import shlex
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
SECTION_HEADERS = {
    "ATOMIC_SPECIES",
    "NUMERICAL_ORBITAL",
    "LATTICE_CONSTANT",
    "LATTICE_VECTORS",
    "ATOMIC_POSITIONS",
    "NUMERICAL_DESCRIPTOR",
}


def parse_case(value: str) -> tuple[str, Path]:
    if ":" not in value:
        raise argparse.ArgumentTypeError("case must be label:/path/to/case")
    label, case_path = value.split(":", 1)
    if not label:
        raise argparse.ArgumentTypeError("case label must not be empty")
    path = Path(case_path).expanduser().resolve()
    if not (path / "INPUT").exists() or not (path / "STRU").exists() or not (path / "KPT").exists():
        raise argparse.ArgumentTypeError(f"case is missing INPUT/STRU/KPT: {path}")
    return label, path


def parse_config(value: str) -> tuple[int, int]:
    if "x" not in value:
        raise argparse.ArgumentTypeError("config must be NPxOMP, for example 16x1")
    np_text, omp_text = value.split("x", 1)
    try:
        nproc = int(np_text)
        omp = int(omp_text)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid config: {value}") from exc
    if nproc < 1 or omp < 1:
        raise argparse.ArgumentTypeError(f"invalid config: {value}")
    return nproc, omp


def copy_case_files(src_dir: Path, dst_dir: Path) -> None:
    dst_dir.mkdir(parents=True, exist_ok=True)
    for src in src_dir.iterdir():
        if not src.is_file():
            continue
        dst = dst_dir / src.name
        if src.name in {"INPUT", "STRU"}:
            continue
        shutil.copy2(src, dst)


def replace_or_append_input(
    src: Path,
    dst: Path,
    *,
    suffix: str,
    pseudo_dir: Path,
    scf_nmax: int,
    scf_thr: str | None,
) -> None:
    replacements = {
        "suffix": f"suffix            {suffix}",
        "pseudo_dir": f"pseudo_dir        {pseudo_dir}",
        "scf_nmax": f"scf_nmax          {scf_nmax}",
        "out_alllog": "out_alllog        1",
    }
    if scf_thr is not None:
        replacements["scf_thr"] = f"scf_thr           {scf_thr}"

    seen: set[str] = set()
    out_lines: list[str] = []
    for line in src.read_text().splitlines():
        stripped = line.strip()
        key = stripped.split()[0] if stripped and not stripped.startswith("#") else ""
        if key in replacements:
            out_lines.append(replacements[key])
            seen.add(key)
        else:
            out_lines.append(line)

    for key, replacement in replacements.items():
        if key not in seen:
            out_lines.append(replacement)

    dst.write_text("\n".join(out_lines) + "\n")


def rewrite_stru_paths(src: Path, dst: Path) -> None:
    section = ""
    out_lines: list[str] = []
    for line in src.read_text().splitlines():
        stripped = line.strip()
        first = stripped.split()[0] if stripped else ""
        if first in SECTION_HEADERS:
            section = first
            out_lines.append(line)
            continue

        if section == "ATOMIC_SPECIES" and stripped and not stripped.startswith("#"):
            parts = line.split()
            if len(parts) >= 3:
                parts[2] = Path(parts[2]).name
                out_lines.append(" ".join(parts))
                continue

        if section == "NUMERICAL_ORBITAL" and stripped and not stripped.startswith("#"):
            parts = line.split()
            if parts:
                parts[0] = Path(parts[0]).name
                out_lines.append(" ".join(parts))
                continue

        out_lines.append(line)

    dst.write_text("\n".join(out_lines) + "\n")


def atom_count(stru: Path) -> int:
    total = 0
    lines = stru.read_text().splitlines()
    position_index = None
    for index, line in enumerate(lines):
        tokens = line.strip().split()
        if tokens and tokens[0] == "ATOMIC_POSITIONS":
            position_index = index
            break
    if position_index is None:
        return 0
    i = position_index + 1

    while i < len(lines) and not lines[i].strip():
        i += 1
    if i < len(lines):
        i += 1  # coordinate type line, e.g. Direct or Cartesian

    while i < len(lines):
        tokens = lines[i].strip().split()
        if not tokens:
            i += 1
            continue
        if tokens[0] in SECTION_HEADERS:
            break
        if tokens[0].isalpha():
            nonempty: list[str] = []
            j = i + 1
            while j < len(lines) and len(nonempty) < 2:
                if lines[j].strip():
                    nonempty.append(lines[j].strip())
                j += 1
            if len(nonempty) == 2:
                try:
                    count = int(nonempty[1].split()[0])
                except (ValueError, IndexError):
                    i += 1
                    continue
                total += count
                i = j + count
                continue
        i += 1
    return total


def run_case(
    *,
    case_dir: Path,
    abacus_bin: Path,
    mpiexec: Path,
    nproc: int,
    omp: int,
    mpi_args: list[str],
) -> int:
    env = os.environ.copy()
    env["ABACUS_TIMER_PRINT_ALL"] = "1"
    env["OMP_NUM_THREADS"] = str(omp)
    env.setdefault("OMP_PROC_BIND", "spread")
    env.setdefault("OMP_PLACES", "cores")

    cmd = ["/usr/bin/time", "-p", str(mpiexec), "-np", str(nproc), *mpi_args, str(abacus_bin)]
    with (case_dir / "stdout.log").open("w") as stdout, (case_dir / "stderr.log").open("w") as stderr:
        completed = subprocess.run(cmd, cwd=case_dir, env=env, stdout=stdout, stderr=stderr, check=False)
    return completed.returncode


def write_meta(path: Path, args: argparse.Namespace, cases: list[tuple[str, Path]]) -> None:
    lines = [
        f"timestamp={datetime.now().isoformat(timespec='seconds')}",
        f"root_dir={ROOT_DIR}",
        f"abacus_bin={args.abacus_bin}",
        f"mpiexec={args.mpiexec}",
        f"kind={args.kind}",
        f"scf_nmax={args.scf_nmax}",
        f"scf_thr={args.scf_thr if args.scf_thr is not None else '<preserve input>'}",
        f"configs={' '.join(args.configs)}",
        f"repeats={args.repeats}",
        f"mpi_args={' '.join(args.mpi_args)}",
        "cases=" + " ".join(f"{label}:{case_path}" for label, case_path in cases),
    ]
    path.write_text("\n".join(lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--kind", choices=["performance", "correctness"], default="performance")
    parser.add_argument("--case", action="append", type=parse_case, required=True)
    parser.add_argument("--configs", nargs="+", required=True)
    parser.add_argument("--repeats", type=int, default=1)
    parser.add_argument("--scf-nmax", type=int, required=True)
    parser.add_argument("--scf-thr", default=None, help="Override scf_thr. Omit to preserve the input value.")
    parser.add_argument("--abacus-bin", type=Path, default=ROOT_DIR / "build-current-abacus-mpi-local/abacus_pw_para")
    parser.add_argument("--mpiexec", type=Path, default=Path("/usr/bin/mpirun.openmpi"))
    parser.add_argument("--pseudo-dir", type=Path, default=ROOT_DIR / "tests/PP_ORB")
    parser.add_argument("--mpi-args", nargs="*", default=[])
    parser.add_argument("--mpi-args-string", default="", help="Extra MPI launcher arguments parsed with shlex.")
    parser.add_argument("--keep-going", action="store_true")
    args = parser.parse_args()

    args.abacus_bin = args.abacus_bin.expanduser().resolve()
    args.mpiexec = args.mpiexec.expanduser().resolve()
    args.pseudo_dir = args.pseudo_dir.expanduser().resolve()
    if not args.abacus_bin.exists():
        raise SystemExit(f"ABACUS binary not found: {args.abacus_bin}")
    if not args.mpiexec.exists():
        raise SystemExit(f"MPI launcher not found: {args.mpiexec}")
    if args.repeats < 1:
        raise SystemExit("repeats must be >= 1")

    configs = [parse_config(config) for config in args.configs]
    if args.mpi_args_string:
        args.mpi_args.extend(shlex.split(args.mpi_args_string))
    suite_dir = args.out_dir.expanduser().resolve()
    run_root = suite_dir / "runs" / args.kind
    run_root.mkdir(parents=True, exist_ok=True)
    (suite_dir / "meta").mkdir(parents=True, exist_ok=True)
    write_meta(suite_dir / "meta" / f"{args.kind}_meta.txt", args, args.case)

    manifest_path = suite_dir / "meta" / f"{args.kind}_manifest.csv"
    case_list_path = suite_dir / "meta" / f"{args.kind}_case_dirs.txt"
    manifest_fields = [
        "kind",
        "label",
        "base_case",
        "atoms",
        "repeat",
        "nproc",
        "omp",
        "scf_nmax",
        "scf_thr",
        "case_dir",
        "returncode",
        "elapsed_s",
    ]
    case_dirs: list[Path] = []
    with manifest_path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=manifest_fields)
        writer.writeheader()
        for label, base_case in args.case:
            atoms = atom_count(base_case / "STRU")
            for repeat in range(1, args.repeats + 1):
                rep_label = f"{repeat:02d}"
                for nproc, omp in configs:
                    case_name = f"{label}_{args.kind}_scf{args.scf_nmax}_rep{rep_label}_np{nproc}_omp{omp}"
                    case_dir = run_root / case_name
                    copy_case_files(base_case, case_dir)
                    replace_or_append_input(
                        base_case / "INPUT",
                        case_dir / "INPUT",
                        suffix=case_name,
                        pseudo_dir=args.pseudo_dir,
                        scf_nmax=args.scf_nmax,
                        scf_thr=args.scf_thr,
                    )
                    rewrite_stru_paths(base_case / "STRU", case_dir / "STRU")

                    print(
                        f"[WorkflowB] {args.kind} label={label} rep={rep_label} "
                        f"np={nproc} omp={omp} -> {case_dir}",
                        flush=True,
                    )
                    start = time.monotonic()
                    returncode = run_case(
                        case_dir=case_dir,
                        abacus_bin=args.abacus_bin,
                        mpiexec=args.mpiexec,
                        nproc=nproc,
                        omp=omp,
                        mpi_args=args.mpi_args,
                    )
                    elapsed = time.monotonic() - start
                    writer.writerow(
                        {
                            "kind": args.kind,
                            "label": label,
                            "base_case": base_case,
                            "atoms": atoms,
                            "repeat": rep_label,
                            "nproc": nproc,
                            "omp": omp,
                            "scf_nmax": args.scf_nmax,
                            "scf_thr": args.scf_thr if args.scf_thr is not None else "",
                            "case_dir": case_dir,
                            "returncode": returncode,
                            "elapsed_s": f"{elapsed:.3f}",
                        }
                    )
                    handle.flush()
                    case_dirs.append(case_dir)
                    if returncode != 0 and not args.keep_going:
                        case_list_path.write_text("\n".join(str(path) for path in case_dirs) + "\n")
                        return returncode

    case_list_path.write_text("\n".join(str(path) for path in case_dirs) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())

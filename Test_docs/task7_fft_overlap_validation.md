# Task 7 FFT Transform Overlap Validation

Date: 2026-06-23

## Scope

Branch: `pr/fft-transform-overlap`

Baseline: `upstream/develop` at `777f50c9c`

Task branch head before local edits: `aeaf3a4d0` plus working-tree changes.

Verified merge-base with `upstream/develop`: `777f50c9c7650f9cd17b36ace9534a7f552fc8ae`.

This validation keeps `final` untouched and checks the task-7 overlap implementation in its own worktree.

## Code Changes

- Removed the hidden `PW_Basis::acquire_comm_workbuf` work-buffer path.
- `gatherp_scatters` and `gathers_scatterp` use local double buffers for communication.
- The multi-rank path packs a block, starts nonblocking communication, prepares the next block while communication is in flight, then waits/unpacks.
- MPI-3 builds use block-level `MPI_Ialltoallv`; older MPI keeps the explicit `MPI_Irecv`/`MPI_Isend` fallback.
- Removed the per-transform `MPI_Allreduce` for choosing block size by deriving a globally consistent block size from `nstot`.
- Adds an explicit `*_single_block_fallback` for cases with only one communication block. There is no next block to overlap in that situation, so the fallback uses the upstream-style blocking `MPI_Alltoallv` data path.
- Delays double-buffer allocation until after the one-block fallback check, avoiding large unused overlap workspaces in single-block cases.
- Multi-block overlap now allocates only block-sized double buffers instead of full-exchange double buffers, reducing the overhead of the overlap path while preserving the block displacements/counts semantics.
- MPI calls are checked through a shared `detail::check_mpi` helper.
- Timer names were clarified from `*_alltoallv` to `*_overlap_comm` so logs reflect the nonblocking overlap path.

## Build

```bash
cmake -S . -B build-task7 \
  -DENABLE_MPI=ON -DBUILD_TESTING=ON \
  -DENABLE_LCAO=OFF -DUSE_ELPA=OFF -DENABLE_LIBXC=OFF \
  -DENABLE_LIBRI=OFF -DENABLE_PEXSI=OFF \
  -DENABLE_CNPY=OFF -DENABLE_RAPIDJSON=OFF \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-task7 --target \
  MODULE_PW_basis_pw_serial \
  MODULE_PW_basis_pw_k_serial \
  MODULE_PW_pw_test \
  abacus_pw_para -j 8
```

Result: build passed after the timer-name cleanup.

## Unit Tests

```bash
./build-task7/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_serial
./build-task7/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_k_serial
OMP_NUM_THREADS=4 ./build-task7/source/source_basis/module_pw/test/MODULE_PW_pw_test --gtest_filter='*transform_omp*'
```

Results:

| Test | Result |
| --- | --- |
| `MODULE_PW_basis_pw_serial` | 13 passed |
| `MODULE_PW_basis_pw_k_serial` | 7 passed, 1 benchmark skipped |
| `MODULE_PW_pw_test --gtest_filter='*transform_omp*'` | 2 passed, 1 disabled |
| `mpirun -np 3 ./MODULE_PW_pw_test --gtest_filter='PWTEST.*'` | 47 passed, 1 disabled |
| `mpirun -np 4 ./MODULE_PW_pw_test --gtest_filter='PWTEST.*'` | 47 passed, 1 disabled |

## Correctness Against Baseline

Run root: `/root/abacus_validation_runs/branch_cases_rerun`

The current task7 code was also rechecked after adding the single-block fallback and delaying double-buffer allocation:

- `/root/abacus_validation_runs/task7_deferbuf_022`
- `/root/abacus_validation_runs/task7_deferbuf_036`
- `/root/abacus_validation_runs/task7_deferbuf_089`

Environment: same machine, `OMP_NUM_THREADS=1`, OpenMPI, CPU build.

| Case | MPI ranks | Baseline energy (eV) | Task7 energy (eV) | Abs diff (eV) |
| --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/022_PW_CG` | 4 | `-198.2238296277166` | `-198.2238296277166` | `0.0` |
| `tests/01_PW/036_PW_AF` | 4 | `-5866.197297493522` | `-5866.197297493522` | `0.0` |
| `tests/01_PW/089_PW_get_wf_kpar` | 3 | `-211.8789253814144` | `-211.8789253814144` | `0.0` |

Correctness target `<= 1e-14` passed at printed precision.

## Overlap Evidence

Additional smoke run:

```bash
OMP_NUM_THREADS=1 mpirun --allow-run-as-root -np 4 /root/wt-task7/build-task7/abacus_pw_para
```

Case: `tests/01_PW/022_PW_CG`

Observed:

```text
!FINAL_ETOT_IS -198.2238296277166 eV
PW_Basis_K      gathers_single_block_fallback 0.00
PW_Basis_K      gatherp_single_block_fallback 0.00
```

This case has only one communication block, so the explicit fallback is active. The three fixed correctness cases and the P010 probe also use the one-block fallback in the current code. Multi-block cases remain routed to the overlap path by construction, but this local validation did not find a stable end-to-end PW case that both triggers multi-block overlap and demonstrates a reproducible speedup.

## Performance Probe

Case: `tests/01_PW/022_PW_CG`

Each row is 3 repeats from the current delayed-buffer implementation. Raw logs are in `/root/abacus_validation_runs/perf_022_task7_deferbuf`.

| MPI ranks | Baseline avg wall (s) | Task7 avg wall (s) | Max energy diff (eV) |
| ---: | ---: | ---: | ---: |
| 2 | `0.4600` | `0.4567` | `0.0` |
| 4 | `0.4600` | `0.4700` | `0.0` |
| 8 | `0.5233` | `0.5300` | `0.0` |
| 12 | `0.6567` | `0.6400` | `0.0` |
| 16 | `0.7033` | `0.7300` | `0.0` |

## Larger Communication Probe

Case: `tests/performance/P010_si2_pw`

Command:

```bash
OMP_NUM_THREADS=1 timeout 120s mpirun --allow-run-as-root --oversubscribe -np 8 ./abacus_pw_para
```

Run roots:

- `/root/abacus_validation_runs/perf_P010_probe`
- `/root/abacus_validation_runs/perf_P010_task7_ialltoallv`
- `/root/abacus_validation_runs/perf_P010_task7_singleblock`
- `/root/abacus_validation_runs/perf_P010_task7_deferbuf`

| Build | Final energy (eV) | Wall (s) | Notes |
| --- | ---: | ---: | --- |
| baseline | `-214.4687637772287871` | `73.19` | blocking MPI collectives, rerun with current task7 |
| task7 p2p overlap | `-214.4687637772287871` | `80.02` | earlier p2p overlap path |
| task7 block `MPI_Ialltoallv` overlap | `-214.4687637772287871` | `80.38` | intermediate path |
| task7 single-block fallback before delayed allocation | `-214.4687637772287871` | `76.87` | allocated unused double buffers before fallback in rerun |
| task7 delayed-buffer single-block fallback | `-214.4687637772287871` | `73.22` | current path for one-block cases |

Delaying the double-buffer allocation removes the remaining visible one-block fallback overhead on this OpenMPI CPU/oversubscribed run. The current P010 result is effectively neutral against upstream (`+0.03s`) with identical energy.

## Multi-block Search Probe

Helper script:

```bash
bash Test_docs/task7_multiblock_probe.sh
```

The script copies selected `tests/performance` PW cases to `/root/abacus_validation_runs/task7_multiblock_probe`, sets `scf_nmax` to `1` in the temporary copy only, and checks whether logs contain `gatherp_overlap_comm` or `gathers_overlap_comm`.

Current local probe, `np=8`, `OMP_NUM_THREADS=1`, `timeout=90s`:

| Case | Result | Wall (s) | Communication timer found |
| --- | --- | ---: | --- |
| `P004_cu4_pw` | completed | `75.03` | `single_block_fallback` |
| `P005_Bi2Se2Cu2O2_pw` | completed | `37.01` | `single_block_fallback` |
| `P000_si16_pw` | completed | `20.71` | `single_block_fallback` |
| `P009_32H2O_pw` | timeout | n/a | none before timeout |

No local probe produced a completed end-to-end PW run with the multi-block overlap timers active. This supports keeping the P010 result framed as a one-block fallback regression check, not as proof of multi-block overlap speedup.

After switching the probe to non-oversubscribed core binding and reducing MPI ranks, `P009_32H2O_pw` showed why a large case is needed:

| Case | MPI ranks | Columns per rank | Status |
| --- | ---: | ---: | --- |
| `P009_32H2O_pw`, `scf_nmax=1` | 16 | about `184` | below block size, fallback expected |
| `P009_32H2O_pw`, `scf_nmax=1` | 2 | about `1470` | multi-block expected, but first electronic iteration was too expensive for practical local benchmarking |

## Multi-block Performance Probe

To get a completed local multi-block benchmark without the large P009 diagonalization cost, this validation uses a temporary high-cutoff copy of `tests/01_PW/004_PW_UPF201_Si`:

- `ecutwfc = 1500`
- `scf_nmax = 1`
- `np = 2`
- `OMP_NUM_THREADS = 1`
- `mpirun --allow-run-as-root --bind-to core --map-by core`
- Run root: `/root/abacus_validation_runs/perf_task7_multiblock_004_ecut1500_np2_blockbuf`

Reproducer:

```bash
bash Test_docs/task7_multiblock_highcut_benchmark.sh
```

This temporary case is not committed; it is only a validation workload that raises the FFT grid/stick count while keeping the atom and band count small enough for repeated local runs.

| Build | Repeats | Final energy (eV) | Avg wall (s) | Timers |
| --- | ---: | ---: | ---: | --- |
| baseline | 3 | `-261.1116847686548` | `14.77` | blocking collectives |
| task7 block-buffer overlap | 3 | `-261.1116847686548` | `14.47` | `gatherp_overlap_comm` observed |

The observed speedup is about `2.0%` with zero printed energy difference. A lighter `ecutwfc = 500` variant also triggered `gatherp_overlap_comm`, but remained slightly slower than baseline; the benefit appears only once the transform communication/work size is high enough to amortize the block pipeline overhead.

## Conclusion

Build, unit tests, and end-to-end correctness pass. One-block cases now use an explicit fallback and avoid allocating unused overlap buffers; multi-block cases keep the overlap implementation with block-sized double buffers. On the small `022_PW_CG` benchmark, wall time remains neutral-to-mixed with wins at `np=2` and `np=12`. The heavier P010 probe is effectively baseline-neutral after the delayed-buffer fix. The temporary high-cutoff `004_PW_UPF201_Si` multi-block probe shows a small speedup once the communication workload is large enough.

Remaining risk: the demonstrated multi-block speedup is modest and measured on a synthetic high-cutoff validation copy, not an existing production benchmark case. Performance should still be rechecked on the intended cluster/MPI stack before merging as a broad speed optimization.

# Task 2 Nonblocking MPI Validation

Date: 2026-06-23

## Scope

Branch: `pr/nonblocking-mpi`

Baseline: `upstream/develop` at `777f50c9c`

Task branch head before local edits: `0a7ca1759` plus working-tree changes.

Verified merge-base with `upstream/develop`: `777f50c9c7650f9cd17b36ace9534a7f552fc8ae`.

This validation keeps `final` untouched and checks the task-2 gather/scatter communication update in its own worktree.

## Code Changes

- Removed the hidden `PW_Basis::acquire_comm_workbuf` mutable/thread-local work-buffer path.
- Reworked `PW_Basis::gatherp_scatters` and `PW_Basis::gathers_scatterp` to use the upstream caller-buffer data flow: pack into `out`, receive the compact exchange into `in`, then unpack back to `out`.
- Uses `MPI_Ialltoallv` when MPI-3 is available.
- Keeps an explicit `MPI_Irecv`/`MPI_Isend` fallback for older MPI.
- MPI return codes are checked and reported through `WARNING_QUIT`.
- Avoids hidden persistent work buffers and per-call large communication workspaces; buffer semantics match upstream, where `in` is temporary exchange storage in the MPI path.
- An unsafe self-skip/caller-buffer experiment was not retained: it changed `022_PW_CG` from `-198.2238296277166` eV to `-165.7132703584108` eV. The current version keeps the full all-to-all data flow and passes the regression case.

## Build

```bash
cmake -S . -B build-task2 \
  -DENABLE_MPI=ON -DBUILD_TESTING=ON \
  -DENABLE_LCAO=OFF -DUSE_ELPA=OFF -DENABLE_LIBXC=OFF \
  -DENABLE_LIBRI=OFF -DENABLE_PEXSI=OFF \
  -DENABLE_CNPY=OFF -DENABLE_RAPIDJSON=OFF \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-task2 --target \
  MODULE_PW_basis_pw_serial \
  MODULE_PW_basis_pw_k_serial \
  MODULE_PW_pw_test \
  abacus_pw_para -j 8
```

Result: build passed.

## Unit Tests

```bash
./build-task2/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_serial
./build-task2/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_k_serial
OMP_NUM_THREADS=4 ./build-task2/source/source_basis/module_pw/test/MODULE_PW_pw_test --gtest_filter='*transform_omp*'
```

Results:

| Test | Result |
| --- | --- |
| `MODULE_PW_basis_pw_serial` | 13 passed |
| `MODULE_PW_basis_pw_k_serial` | 7 passed, 1 benchmark skipped |
| `MODULE_PW_pw_test --gtest_filter='*transform_omp*'` | 0 tests on this branch |

## Correctness Against Baseline

Run roots:

- `/root/abacus_validation_runs/task2_callerbuf_022`
- `/root/abacus_validation_runs/task2_callerbuf_036`
- `/root/abacus_validation_runs/task2_callerbuf_089`

Environment: same machine, `OMP_NUM_THREADS=1`, OpenMPI, CPU build.

| Case | MPI ranks | Baseline energy (eV) | Task2 energy (eV) | Abs diff (eV) |
| --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/022_PW_CG` | 4 | `-198.2238296277166` | `-198.2238296277166` | `0.0` |
| `tests/01_PW/036_PW_AF` | 4 | `-5866.197297493522` | `-5866.197297493522` | `0.0` |
| `tests/01_PW/089_PW_get_wf_kpar` | 3 | `-211.8789253814144` | `-211.8789253814144` | `0.0` |

Correctness target `<= 1e-14` passed at printed precision.

## Performance Probe

Case: `tests/01_PW/022_PW_CG`

Command shape:

```bash
OMP_NUM_THREADS=1 mpirun --allow-run-as-root --oversubscribe -np <N> ./abacus_pw_para
```

Each row is 3 repeats from the current caller-buffer implementation. Raw logs are in `/root/abacus_validation_runs/perf_022_task2_callerbuf_v2`.

| MPI ranks | Baseline avg wall (s) | Task2 avg wall (s) | Max energy diff (eV) |
| ---: | ---: | ---: | ---: |
| 2 | `0.4500` | `0.4567` | `0.0` |
| 4 | `0.4533` | `0.4533` | `0.0` |
| 8 | `0.5167` | `0.5433` | `0.0` |
| 12 | `0.6400` | `0.6400` | `0.0` |
| 16 | `0.7000` | `0.7033` | `0.0` |

Timer evidence from task2 logs includes `gathers_ialltoallv` and `gatherp_ialltoallv`, confirming that the intended nonblocking collective path is active.

## Larger Communication Probe

Case: `tests/performance/P010_si2_pw`

Command:

```bash
OMP_NUM_THREADS=1 timeout 120s mpirun --allow-run-as-root --oversubscribe -np 8 ./abacus_pw_para
```

Run root: `/root/abacus_validation_runs/perf_P010_task2_callerbuf`

| Build | Final energy (eV) | Wall (s) | Notes |
| --- | ---: | ---: | --- |
| baseline | `-214.4687637772287871` | `72.71` | blocking MPI collectives |
| task2 | `-214.4687637772287871` | `72.49` | `gathers_ialltoallv`, `gatherp_ialltoallv` active |

This probe confirms correctness on a heavier k-point workload and shows a small speedup in this run after removing the extra communication workspaces.

## Conclusion

Correctness and build acceptance pass. The nonblocking collective path is active and robustly checked. The caller-buffer data flow removes the extra large communication workspaces from the previous task2 revision while preserving upstream buffer semantics.

Performance result: the small `022_PW_CG` matrix is neutral-to-slightly-slower, while the heavier P010 probe is slightly faster than upstream in the same rerun. Treat task2 as correctness/robustness passing with a modest heavy-case speed win, not as a universal speedup.

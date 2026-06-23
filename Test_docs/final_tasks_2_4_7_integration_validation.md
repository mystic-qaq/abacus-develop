# Final integration validation for tasks 2, 4, and 7

Date: 2026-06-23
Branch: final
Build directory: build-pw-final-clean

## Scope

Integrated the validated task branches without merging their full histories:

- Task 2 nonblocking MPI communication report: Test_docs/task2_nonblocking_mpi_validation.md
- Task 4 GammaOnly report: Test_docs/task4_gammaonly_validation.md
- Task 7 FFT transform overlap report: Test_docs/task7_fft_overlap_validation.md

Task 4 precision note: tighter SCF settings reduced the full-complex vs half-spectrum repeated-Gamma energy gap to about 2.1e-10 eV. This passes the 1e-9 acceptance level but did not reliably reach 1e-14 in the repeated-Gamma case.

## Build

Configure:

```bash
cmake -S . -B build-pw-final-clean -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DENABLE_MPI=ON \
  -DENABLE_LCAO=OFF \
  -DENABLE_MLALGO=OFF \
  -DENABLE_LIBRI=OFF \
  -DUSE_OPENMP=ON \
  -DUSE_ELPA=OFF \
  -DUSE_CUDA=OFF \
  -DUSE_ROCM=OFF \
  -DUSE_DSP=OFF
```

Build:

```bash
cmake --build build-pw-final-clean --target \
  abacus_pw_para MODULE_PW_basis_pw_serial MODULE_PW_basis_pw_k_serial -j 16
```

Result: passed.

## Unit Tests

```bash
./build-pw-final-clean/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_serial
./build-pw-final-clean/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_k_serial
```

Results:

- MODULE_PW_basis_pw_serial: 13 passed.
- MODULE_PW_basis_pw_k_serial: 12 passed, 1 skipped timer benchmark.

Note: the older build-pw-final directory produced stale-object failures after interface replacement. A clean build directory was used for validation.

## Application Smoke Tests

All runs used OMP_NUM_THREADS=1 and a temporary copy of the input case with pseudo_dir set to /root/abacus-develop/tests/PP_ORB.

| Case | MPI ranks | Result | Final energy (eV) |
| --- | ---: | --- | ---: |
| tests/01_PW/022_PW_CG | 2 | passed | -198.2238296277165 |
| tests/01_PW/036_PW_AF | 2 | passed | -5866.197297502572 |
| tests/01_PW/089_PW_get_wf_kpar | 3 | passed | -211.8789253814144 |

## Conclusion

The final branch working tree builds and runs with the integrated task 2/4/7 changes. Existing branch-level reports contain the detailed correctness and performance comparisons; this file records the final integration smoke validation.

## Task 8 compatibility repair

Date: 2026-06-23

After commit `7046c8dfa` integrated tasks 2/4/7, `MODULE_PW_pw_test` no longer
compiled because the integration replaced the task-8 `PW_Basis` cache interface
used by `source/source_basis/module_pw/test/test1-1-1.cpp`:

- `PW_Basis::reset_cache_stats()`
- `PW_Basis::get_cache_stats()`

The repair restores the task-8 cache API in `PW_Basis` while preserving the
task-2/4/7 integration. The cache storage now uses explicit ownership through
RAII containers and normal mutex-protected invalidation; it does not use
`mutable`.

Additional invalidation points were restored for MPI/grid/parameter changes and
for redistributed G-vector maps. `MODULE_PW_cache_bench` now initializes MPI on
its benchmark `PW_Basis` objects before calling MPI-dependent setup code.

### Verification after repair

Build:

```bash
cmake --build build-pw-final-clean --target \
  MODULE_PW_cache_bench MODULE_PW_cache_bench_serial MODULE_PW_pw_test \
  MODULE_PW_basis_pw_serial MODULE_PW_basis_pw_k_serial abacus_pw_para -j 16
```

Result: passed.

Unit and benchmark checks:

```bash
OMP_NUM_THREADS=1 mpirun --allow-run-as-root -np 4 \
  ./build-pw-final-clean/source/source_basis/module_pw/test/MODULE_PW_pw_test \
  --gtest_filter='PWTEST.*'

./build-pw-final-clean/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_serial
./build-pw-final-clean/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_k_serial

OMP_NUM_THREADS=1 mpirun --allow-run-as-root -np 2 \
  ./build-pw-final-clean/source/source_basis/module_pw/MODULE_PW_cache_bench
```

Results:

- `MODULE_PW_pw_test --gtest_filter='PWTEST.*'`: 47 passed, 1 disabled.
- `MODULE_PW_basis_pw_serial`: 13 passed.
- `MODULE_PW_basis_pw_k_serial`: 12 passed, 1 skipped timer benchmark.
- `MODULE_PW_cache_bench`: ran successfully and reported cache hits for both
  `collect_local_pw` and `collect_uniqgg`.

Application smoke checks after rebuilding `abacus_pw_para`:

| Case | MPI ranks | Final energy (eV) | Reference (eV) | Absolute diff (eV) |
| --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/022_PW_CG` | 2 | `-198.2238296277165` | `-198.2238296207179` | `6.998590e-09` |
| `tests/01_PW/036_PW_AF` | 2 | `-5866.197297502572` | `-5866.197297502891` | `3.192326e-10` |
| `tests/01_PW/089_PW_get_wf_kpar` | 3 | `-211.8789253814144` | `-211.8789253813293` | `8.509460e-11` |

Note: `022_PW_CG` uses `scf_thr 1e-8` in the stock input, so its reference-file
energy comparison is expected to be looser than the tighter SCF cases.

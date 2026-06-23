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

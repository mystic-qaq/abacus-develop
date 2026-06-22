# Final integration validation report

Date: 2026-06-22
Base branch: `origin/final`
Integrated branches:

- `GammaOnly` for task 7
- `pr/nonblocking-mpi` for task 2
- `pr/fft-transform-overlap` for task 4

## Integration Notes

- Resolved `pw_basis.h` conflicts by preserving both compact-gamma APIs already present on `final` and the `GammaCompact` helper from `GammaOnly`.
- Resolved `pw_gatherscatter.h` conflicts by keeping the non-blocking MPI gather/scatter implementation and its timer scopes.
- Fixed `GammaOnly` serial-test linkage by adding `gamma_compact.cpp` to `planewave_serial`.
- Rewrote one C++17-style init-statement in `pw_transform.cpp` to C++14-compatible code.

## Build

Configuration:

```bash
cmake -S . -B build-pw-final -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DENABLE_MPI=ON -DUSE_OPENMP=ON \
  -DENABLE_LCAO=OFF -DENABLE_LIBXC=OFF -DUSE_ELPA=OFF \
  -DUSE_CUDA=OFF -DUSE_ROCM=OFF
```

Focused build targets all passed:

- `MODULE_PW_pw_test`
- `MODULE_PW_basis_pw_serial`
- `MODULE_PW_basis_pw_k_serial`
- `MODULE_PW_PW_Kernels_UTs`

## Correctness

- `MODULE_PW_basis_pw_serial`: 16 passed.
- `MODULE_PW_basis_pw_k_serial`: 8 passed, 1 benchmark skipped by default.
- `MODULE_PW_PW_Kernels_UTs`: 3 passed.
- `MODULE_PW_pw_test`: 54 passed, 1 skipped in single-rank mode.
- `MODULE_PW_pw_test_parallel`: 55 passed under `mpirun -np 3`.

The final run covered gamma-only, compact-gamma, cache reuse, non-blocking gather/scatter, serial and MPI FFT paths, and transform OpenMP consistency tests.

## Performance Smoke Tests

`PWBasisKTEST.CopyComplexBufferTimerBenchmark`:

- copy helper: 0.247263 s, 4.04427 GiB/s
- scalar loop: 0.297485 s, 3.36152 GiB/s
- speedup: 1.20311x

`PWTEST.DISABLED_transform_omp_speedup_report`:

- 1 thread: 0.00191387 s, speedup 1.0x
- 2 threads: 0.00179712 s, speedup 1.06496x
- 4 threads: 0.00216891 s, speedup 0.882414x

The copy helper shows a local improvement in this run. The transform benchmark is too small for a stable speedup claim; 4 threads are slower due to overhead.

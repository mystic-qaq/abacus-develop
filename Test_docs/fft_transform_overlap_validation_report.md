# FFT transform overlap validation report

Date: 2026-06-22
Upstream base: `upstream/develop` at `59ebed572db848153064143ba597f8342aefeb4e`
Branch head tested: `dad4279b3`

## Scope

- Synced `pr/fft-transform-overlap` with upstream `develop`.
- Resolved conflicts in PW gather/scatter and transform code.
- Kept upstream `pw_transform.cpp` after merge because upstream already contains the cache-blocked transform copy path and the branch history had reverted an earlier SIMD loop variant for gamma-only correctness.
- Preserved the branch's transform OpenMP consistency tests and copy helper benchmark.

## Build

Configuration:

```bash
cmake -S . -B build-pw-explicit -G Ninja -DCMAKE_BUILD_TYPE=Release \
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

- `MODULE_PW_basis_pw_serial`: 13 passed.
- `MODULE_PW_basis_pw_k_serial`: 7 passed, 1 benchmark skipped by default.
- `MODULE_PW_PW_Kernels_UTs`: 3 passed.
- `MODULE_PW_pw_test`: 54 passed, 1 skipped in single-rank mode.
- `MODULE_PW_pw_test_parallel`: 55 passed under `mpirun -np 3`.

The added OpenMP transform consistency tests passed for complex round trip and real gamma-only add/non-add paths.

## Performance Smoke Tests

`PWBasisKTEST.CopyComplexBufferTimerBenchmark`:

- copy helper: 0.0783997 s, 12.7551 GiB/s
- scalar loop: 0.083701 s, 11.9473 GiB/s
- speedup: 1.06762x

`PWTEST.DISABLED_transform_omp_speedup_report`:

- 1 thread: 0.00195429 s, speedup 1.0x
- 2 threads: 0.00192161 s, speedup 1.01701x
- 4 threads: 0.00202155 s, speedup 0.966727x

The transform helper is too small to prove a broad performance gain. It confirms the performance path runs, but a larger benchmark is needed before claiming scalable OpenMP speedup.

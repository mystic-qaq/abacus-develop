# GammaOnly validation report

Date: 2026-06-22
Upstream base: `upstream/develop` at `59ebed572db848153064143ba597f8342aefeb4e`
Branch head tested: `2422af036` plus local CMake test-link fix

## Scope

- Synced `GammaOnly` with upstream `develop`.
- Resolved conflicts in PW distribution/transform code.
- Fixed `source/source_basis/module_pw/test_serial/CMakeLists.txt` so `planewave_serial` also links `gamma_compact.cpp`.

## Build

Configuration:

```bash
cmake -S . -B build-pw-gamma -G Ninja -DCMAKE_BUILD_TYPE=Release \
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
- `MODULE_PW_pw_test`: 52 passed, 1 skipped in single-rank mode.
- `MODULE_PW_pw_test_parallel`: 53 passed under `mpirun -np 3`.

The PW tests cover gamma-only, xprime, full_pw, r2c/c2r, c2c FFT round trips, and MPI-distributed FFT paths.

## Performance Smoke Test

`PWBasisKTEST.CopyComplexBufferTimerBenchmark`:

- copy helper: 0.0761594 s, 13.1304 GiB/s
- scalar loop: 0.077683 s, 12.8728 GiB/s
- speedup: 1.02001x

This is a small local microbenchmark; treat it as a smoke test rather than a full performance claim.

# Nonblocking MPI validation report

Date: 2026-06-22
Upstream base: `upstream/develop` at `59ebed572db848153064143ba597f8342aefeb4e`
Branch head tested: `abf202874`

## Scope

- Synced `pr/nonblocking-mpi` with upstream `develop`.
- Resolved the conflict in `source/source_basis/module_pw/pw_gatherscatter.h`.
- Preserved the branch's non-blocking MPI gather/scatter implementation.
- Kept upstream helper copy functions used by the newer transform code.

## Build

Configuration:

```bash
cmake -S . -B build-pw-nonblocking -G Ninja -DCMAKE_BUILD_TYPE=Release \
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

The parallel PW test directly exercises distributed gather/scatter and FFT paths with the non-blocking MPI implementation.

## Performance Smoke Test

`PWBasisKTEST.CopyComplexBufferTimerBenchmark`:

- copy helper: 0.0789152 s, 12.6718 GiB/s
- scalar loop: 0.0762162 s, 13.1206 GiB/s
- speedup: 0.965799x

This branch did not include a stable before/after communication benchmark. Correctness is verified; performance should be measured with a dedicated MPI benchmark or a representative ABACUS input before making a strong speedup claim.

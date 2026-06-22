# Final integration validation report

Date: 2026-06-22

Branch: `final`

Integrated local merge commits:

- `e7fcf4cff` merge `GammaOnly`
- `98d75153c` merge `pr/nonblocking-mpi`
- `6c4587ea8` merge `pr/fft-transform-overlap`

## Scope

This report validates the merged `final` branch itself, not just the three feature branches in isolation.

The integrated content covered here is:

- task 7: `GammaOnly`
- task 2: `pr/nonblocking-mpi`
- task 4: `pr/fft-transform-overlap`

## Build

Validated configuration:

```bash
cmake -S . -B build-rel -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/mpicxx \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DENABLE_MPI=ON -DUSE_OPENMP=ON \
  -DENABLE_LCAO=OFF -DENABLE_LIBXC=OFF -DUSE_ELPA=OFF \
  -DUSE_CUDA=OFF -DUSE_ROCM=OFF
```

Validated build targets:

- `abacus_pw_para`
- `source/source_basis/module_pw/test/MODULE_PW_pw_test`

## Final-branch correctness evidence

All MPI end-to-end runs below used:

- the merged `final` executable `build-rel/abacus_pw_para`
- the same local pseudopotential directory

### 1. GammaOnly supported path still works after integration

Case: `022_PW_CG` modified to:

- `ecutwfc = 200`
- `scf_thr = 1e-9`
- `init_wfc = random`
- `pw_seed = 1`

Run setup:

- `mpirun -np 4`
- one run with full-complex PW
- one run with `gamma_only = 1`

Observed final total energies:

| Mode | Final total energy (eV) |
| --- | ---: |
| full-complex | `-198.3550507424494` |
| GammaOnly | `-198.3550507394055` |

Difference:

- about `3.0e-9 eV`

Conclusion:

- The integrated `final` branch preserves the validated GammaOnly correctness on the supported path.

### 2. `089_PW_get_wf_kpar` still produces correct `.cube` outputs

Case: stock `tests/01_PW/089_PW_get_wf_kpar`

Run setup:

- `mpirun -np 6`

Observed final total energy:

- `-211.878925381389 eV`

Observed `.cube` integrals:

- `wfi1s1k1.cube = 21.94720511`
- `wfi1s1k2.cube = 19.65846447`
- `wfi1s1k3.cube = 19.34110139`

These values match the audited `develop` baseline exactly.

### 3. `007_PW_UPF201_USPP_Fe` still agrees with the audited baseline

Case: stock `tests/01_PW/007_PW_UPF201_USPP_Fe`

Run setup:

- `mpirun -np 6`

Observed final total energy:

- `-673.8349347004925676 eV`

This matches the audited `develop` baseline exactly.

### 4. Branch-specific OpenMP transform tests still pass after integration

Executed:

```bash
mpirun -np 1 ./source/source_basis/module_pw/test/MODULE_PW_pw_test \
  --gtest_filter='PWTEST.transform_omp_threads_*'
```

Passed:

- `PWTEST.transform_omp_threads_complex_roundtrip_consistency`
- `PWTEST.transform_omp_threads_real_gamma_and_add_consistency`

Conclusion:

- The extra transform-thread-consistency coverage from task 4 remains intact on the merged `final` branch.

## Final-branch performance evidence

### Benchmark method

- comparison target: audited `develop` baseline from the same machine and same harness
- launcher: `mpirun -np 6`
- threads: `OMP_NUM_THREADS=1`
- repeats: 5 runs per case
- metric: external wall time from `/usr/bin/time`
- important note: all measurements cited below were run sequentially, not concurrently

### Case A: heavy `kpar > 1` benchmark

Base case: `089_PW_get_wf_kpar`

Modification:

- `ecutwfc = 300`

Per-run wall times (seconds):

| Branch | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Average |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `develop` | 5.58 | 5.64 | 5.43 | 6.03 | 5.52 | 5.640 |
| `final` | 5.47 | 5.40 | 5.39 | 5.40 | 5.49 | 5.430 |

Observed average change:

- wall time: `5.640 s -> 5.430 s`
- relative improvement: about `3.7%`

Same-case correctness check:

- final energy matched the audited baseline exactly in every run: `-211.878935664417 eV`

Selected average timers:

| Branch | `PW_Basis_K recip2real` | `PW_Basis_K real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | 1.270 s | 0.792 s | 2.402 s | 2.854 s |
| `final` | 1.272 s | 0.746 s | 2.324 s | 2.738 s |

Conclusion for Case A:

- The merged `final` branch retains a real end-to-end speedup on the most relevant audited `kpar > 1` heavy benchmark.

### Case B: heavier AF Fe SCF benchmark

Base case: `036_PW_AF`

Modification:

- `ecutwfc = 60`

Per-run wall times (seconds):

| Branch | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Average |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `develop` | 2.07 | 2.04 | 2.12 | 2.03 | 2.03 | 2.058 |
| `final` | 2.07 | 2.07 | 2.11 | 2.07 | 2.08 | 2.080 |

Observed average change:

- wall time: `2.058 s -> 2.080 s`
- relative change: about `+1.1%`

Same-case correctness check:

- final energy matched the audited baseline exactly in every run: `-6445.671596031703 eV`

Conclusion for Case B:

- The merged `final` branch is essentially neutral on this second heavier SCF benchmark; no meaningful speedup is claimed here.

## Overall conclusion

What is now supported by evidence on the merged `final` branch:

- GammaOnly supported-path correctness is preserved after integration
- `kpar > 1` wavefunction-output correctness is preserved after integration
- USPP + `kpar=3` correctness is preserved after integration
- the added OpenMP transform consistency tests still pass after integration
- the merged branch retains a modest but real end-to-end speedup on the audited heavy `kpar > 1` benchmark

What is not claimed:

- that every integrated task speeds up every workload
- that GammaOnly provides end-to-end wall-time speedup
- that the second heavier AF Fe benchmark shows a meaningful wall-time win

## Acceptance status

- Final integration correctness: pass
- Final integration robustness on audited cases: pass
- Final integration performance: pass, with a real case-specific end-to-end gain retained on the heavy `kpar > 1` benchmark and neutral behavior on the second benchmark

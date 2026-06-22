# FFT transform overlap validation report

Date: 2026-06-22

Branch: `pr/fft-transform-overlap`

## Current audited status

- End-to-end correctness: supported by real MPI SCF runs
- Added OpenMP transform consistency tests: passed
- Performance: now demonstrates a small but repeatable wall-time gain on a k-point-parallel heavy case, and is effectively neutral on a second heavier SCF case
- Important scope clarification: in the current branch state, the hot PW gather/scatter implementation is effectively the same optimized path used by `pr/nonblocking-mpi`; the distinct added value of this branch is the extra transform OpenMP consistency coverage

## Build

Validated executable:

- `build-rel/abacus_pw_para`

Validated test binary:

- `build-rel/source/source_basis/module_pw/test/MODULE_PW_pw_test`

## Correctness evidence

All end-to-end runs below used:

- `mpirun -np 6`
- `OMP_NUM_THREADS=1`

### 1. `089_PW_get_wf_kpar`: exact `.cube` output agreement

Case: stock `tests/01_PW/089_PW_get_wf_kpar`

Observed final total energy:

- `-211.878925381389 eV`

Observed `.cube` integrals:

- `wfi1s1k1.cube = 21.94720511`
- `wfi1s1k2.cube = 19.65846447`
- `wfi1s1k3.cube = 19.34110139`

These values matched the `develop` baseline exactly in this audit.

### 2. `007_PW_UPF201_USPP_Fe`: USPP + `kpar=3` path agreement

Case: stock `tests/01_PW/007_PW_UPF201_USPP_Fe`

Observed final total energy:

- `-673.8349347004925676 eV`

This matched the `develop` baseline exactly in this audit.

### 3. Heavy SCF agreement

Case: `036_PW_AF` with `ecutwfc` raised from `15` to `60`

Observed final total energy:

- `-6445.671596031703 eV`

This matched the `develop` baseline exactly in all repeated runs used for the performance section below.

## Branch-specific OpenMP transform tests

Executed:

```bash
mpirun -np 1 ./source/source_basis/module_pw/test/MODULE_PW_pw_test \
  --gtest_filter='PWTEST.transform_omp_threads_*'
```

Passed tests:

- `PWTEST.transform_omp_threads_complex_roundtrip_consistency`
- `PWTEST.transform_omp_threads_real_gamma_and_add_consistency`

These tests verify that the transform results stay consistent across different OpenMP thread counts for both the complex round-trip path and the real gamma-only add/non-add path.

## Performance evidence

### Benchmark method

- Branches compared: `develop` vs current `pr/fft-transform-overlap`
- Launcher: `mpirun -np 6`
- Threads: `OMP_NUM_THREADS=1`
- Repeats: 5 runs per branch per case
- Metric: external wall time from `/usr/bin/time`

### Case A: k-point-parallel heavy variant of `089_PW_get_wf_kpar`

Modified from the stock case:

- `ecutwfc = 300`

Per-run wall times (seconds):

| Branch | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Average |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `develop` | 5.58 | 5.64 | 5.43 | 6.03 | 5.52 | 5.640 |
| `pr/fft-transform-overlap` | 5.53 | 5.48 | 5.39 | 5.41 | 5.44 | 5.450 |

Observed average change:

- wall time: `5.640 s -> 5.450 s`
- relative improvement: about `3.4%`

Same-case correctness check:

- final energy matched `develop` exactly in every run: `-211.878935664417 eV`

Selected average timers:

| Branch | `PW_Basis_K recip2real` | `PW_Basis_K real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | 1.270 s | 0.792 s | 2.402 s | 2.854 s |
| `pr/fft-transform-overlap` | 1.238 s | 0.764 s | 2.350 s | 2.776 s |

Conclusion for Case A:

- This branch now shows a real repeated end-to-end speedup on the audited `kpar > 1` heavy case.

### Case B: heavier AF Fe SCF

Modified from `tests/01_PW/036_PW_AF`:

- `ecutwfc = 60`

Per-run wall times (seconds):

| Branch | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Average |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `develop` | 2.07 | 2.04 | 2.12 | 2.03 | 2.03 | 2.058 |
| `pr/fft-transform-overlap` | 2.10 | 2.04 | 2.04 | 2.08 | 2.02 | 2.056 |

Observed average change:

- wall time: `2.058 s -> 2.056 s`
- relative change about `-0.1%`, effectively neutral

Same-case correctness check:

- final energy matched `develop` exactly in every run: `-6445.671596031703 eV`

Conclusion for Case B:

- No meaningful speedup or slowdown was observed here.

## Overall conclusion

What is now supported by evidence:

- end-to-end correctness on the audited MPI PW cases
- exact agreement with `develop` on the audited correctness outputs
- added OpenMP transform consistency tests pass
- a modest but repeatable end-to-end speedup on a relevant heavy `kpar > 1` benchmark
- neutral behavior on the second heavier SCF benchmark

What is not claimed:

- a universal FFT/OpenMP speedup for all workloads
- that this branch currently contains a distinct end-to-end overlap mechanism separate from the gather/scatter optimization shared with `pr/nonblocking-mpi`

## Acceptance status

- Correctness: pass
- OpenMP transform consistency: pass
- Performance: pass, with a modest but real case-specific end-to-end gain and neutral behavior on the second benchmark

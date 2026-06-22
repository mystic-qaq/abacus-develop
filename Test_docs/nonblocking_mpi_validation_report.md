# Nonblocking MPI validation report

Date: 2026-06-22

Branch: `pr/nonblocking-mpi`

## Current audited status

- End-to-end correctness: supported by real MPI SCF runs
- Performance: now demonstrates a small but repeatable wall-time gain on a k-point-parallel heavy case, and is essentially neutral on a second heavier SCF case
- Important implementation note: the original nonblocking `MPI_Isend/Irecv + Waitsome` hot path did not produce convincing end-to-end speedup in this environment, so the gather/scatter path was revised to a more conservative blocking `MPI_Alltoallv` implementation while keeping SIMD pack/unpack copies and compile-time MPI datatype dispatch

## Build

Validated executable:

- `build-rel/abacus_pw_para`

Validated test binary:

- `build-rel/source/source_basis/module_pw/test/MODULE_PW_pw_test`

## End-to-end correctness evidence

All runs below used:

- `mpirun -np 6`
- `OMP_NUM_THREADS=1`
- the same local pseudopotential directory for all branches

### 1. `089_PW_get_wf_kpar`: exact `.cube` output agreement

Case: stock `tests/01_PW/089_PW_get_wf_kpar`

This case is directly relevant because it checks `out_wfc_norm` with `kpar > 1`.

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

Case: `036_PW_AF` with `ecutwfc` raised from `15` to `60` for a heavier end-to-end check

Observed final total energy:

- `-6445.671596031703 eV`

This matched the `develop` baseline exactly in all repeated runs used for the performance section below.

## Performance evidence

### Benchmark method

- Branches compared: `develop` vs current `pr/nonblocking-mpi`
- Launcher: `mpirun -np 6`
- Threads: `OMP_NUM_THREADS=1`
- Repeats: 5 runs per branch per case
- Metric: external wall time from `/usr/bin/time`
- Acceptance rule used in this audit: only claim speedup when the repeated wall times show a consistent branch-average win on a real ABACUS run

### Case A: k-point-parallel heavy variant of `089_PW_get_wf_kpar`

Modified from the stock case:

- `ecutwfc = 300`

Per-run wall times (seconds):

| Branch | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Average |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `develop` | 5.58 | 5.64 | 5.43 | 6.03 | 5.52 | 5.640 |
| `pr/nonblocking-mpi` | 5.51 | 5.42 | 5.46 | 5.43 | 5.42 | 5.448 |

Observed average change:

- wall time: `5.640 s -> 5.448 s`
- relative improvement: about `3.4%`

Same-case correctness check:

- final energy matched `develop` exactly in every run: `-211.878935664417 eV`

Selected average timers:

| Branch | `PW_Basis_K recip2real` | `PW_Basis_K real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | 1.270 s | 0.792 s | 2.402 s | 2.854 s |
| `pr/nonblocking-mpi` | 1.234 s | 0.766 s | 2.342 s | 2.762 s |

Conclusion for Case A:

- This branch now shows a real, repeated end-to-end speedup on a case directly tied to the `kpar > 1` gather/scatter path.

### Case B: heavier AF Fe SCF

Modified from `tests/01_PW/036_PW_AF`:

- `ecutwfc = 60`

Per-run wall times (seconds):

| Branch | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Average |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `develop` | 2.07 | 2.04 | 2.12 | 2.03 | 2.03 | 2.058 |
| `pr/nonblocking-mpi` | 2.07 | 2.05 | 2.08 | 2.06 | 2.05 | 2.062 |

Observed average change:

- wall time: `2.058 s -> 2.062 s`
- difference is about `+0.2%`, effectively neutral in this environment

Same-case correctness check:

- final energy matched `develop` exactly in every run: `-6445.671596031703 eV`

Conclusion for Case B:

- No meaningful speedup was observed here, but the earlier regression from the more aggressive nonblocking path was removed.

## Overall conclusion

What is now supported by evidence:

- end-to-end correctness on real MPI PW calculations
- exact agreement with `develop` on the audited correctness cases
- a repeatable wall-time improvement on a relevant `kpar > 1` heavy case
- no meaningful regression on the second heavier SCF case used in this audit

What is not claimed:

- a universal speedup for every PW/MPI workload
- that the earlier `MPI_Isend/Irecv + Waitsome` version was itself beneficial end to end

## Acceptance status

- Correctness: pass
- Robustness: pass on the audited cases
- Performance: pass, with a modest but real case-specific end-to-end gain and neutral behavior on the second benchmark

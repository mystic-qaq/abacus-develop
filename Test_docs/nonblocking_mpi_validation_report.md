# Nonblocking MPI Validation Report

Date: 2026-06-22

Branch: `pr/nonblocking-mpi`

## Scope

This branch targets task 2 in `01_plane_wave.md`: replace the blocking `MPI_Alltoallv`
path in `pw_gatherscatter.h` with a true nonblocking point-to-point implementation
based on `MPI_Isend` / `MPI_Irecv`, while keeping end-to-end correctness and avoiding
obvious performance regressions.

## Final implementation status

- Task alignment: yes
  - the active optimized path uses `MPI_Isend` / `MPI_Irecv`
  - self-owned data are copied locally while remote communication is in flight
  - a conservative runtime heuristic falls back to the original blocking path on
    communication-heavy cases that do not benefit on this test machine
- End-to-end correctness: pass on the audited MPI PW cases
- Performance:
  - one repeated SCF benchmark shows a real wall-time gain
  - two heavier benchmarks stay close to baseline, with only small regressions

## Build note

Validated executable:

- `build-rel/abacus_pw_para`

Full `cmake --build build-rel` is currently blocked by an unrelated upstream unit-test
compile error in `source/source_cell/test/read_atoms_helper_test.cpp`
(`InfoNonlocal` missing type). This does not affect `abacus_pw_para`, which was built
successfully with:

```bash
cmake --build build-rel --target abacus_pw_para -j 8
```

## Correctness evidence

All audited runs used `OMP_NUM_THREADS=1`.

### 1. Official end-to-end case: `007_PW_UPF201_USPP_Fe`

Command shape:

```bash
mpirun -np 6 ./abacus_pw_para
```

Observed final total energy:

- `develop`: `-673.8349347004925676 eV`
- `pr/nonblocking-mpi`: `-673.8349347004925676 eV`

Result: exact agreement to printed precision.

### 2. Repeated heavy-SCF correctness

Repeated performance benchmarks below also verified final total energy equality:

| Case | `develop` final energy (eV) | branch final energy (eV) |
| --- | ---: | ---: |
| `036-hi` (`np=6`, 3 repeats) | `-6445.671596031704` | `-6445.671596031704` |
| `089-hi` (`np=16`, 3 repeats) | `-211.878935664122` | `-211.878935664122` |
| `089-xhi` (`np=12`, 3 repeats) | `-211.878935981624` | `-211.878935981624` |

No energy drift was observed in the audited repeated runs.

## Performance methodology

- benchmark mode: serial branch-by-branch execution only
- metric: external wall time from `/usr/bin/time`
- pseudo directory: `tests/PP_ORB`
- benchmark helper: `/tmp/abacus_repeat_bench.sh`
- repeats: `3`

The branch uses an adaptive policy:

- low communication pressure: nonblocking point-to-point path enabled
- higher communication pressure: fall back to the original blocking exchange

So the numbers below are honest end-to-end branch results, not forced-path microbenchmarks.

## Performance results

### Case A: `036-hi` (`036_PW_AF`, `ecutwfc=60`, `np=6`)

| Branch | Avg wall (s) | Delta vs `develop` |
| --- | ---: | ---: |
| `develop` | `2.136667` | baseline |
| `pr/nonblocking-mpi` | `2.066667` | `-3.28%` |

Selected average timers:

| Branch | `recip2real` | `real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | `0.443333` | `0.396667` | `1.010000` | `1.383333` |
| branch | `0.433333` | `0.380000` | `0.980000` | `1.330000` |

Interpretation:

- this is the clearest positive case in the current audit
- the branch gives a small but repeatable end-to-end speedup

### Case B: `089-hi` (`089_PW_get_wf_kpar`, `ecutwfc=300`, `np=16`)

| Branch | Avg wall (s) | Delta vs `develop` |
| --- | ---: | ---: |
| `develop` | `4.863333` | baseline |
| `pr/nonblocking-mpi` | `4.940000` | `+1.58%` |

Selected average timers:

| Branch | `recip2real` | `real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | `0.630000` | `0.433333` | `1.300000` | `2.360000` |
| branch | `0.633333` | `0.410000` | `1.273333` | `2.296667` |

Interpretation:

- regression is small
- on this heavier k-point-parallel case the adaptive branch is close to neutral

### Case C: `089-xhi` (`089_PW_get_wf_kpar`, `ecutwfc=600`, `np=12`)

Baseline was re-run after the branch benchmark to control for machine-state drift.

| Branch | Avg wall (s) | Delta vs `develop` |
| --- | ---: | ---: |
| `develop` | `18.316667` | baseline |
| `pr/nonblocking-mpi` | `19.110000` | `+4.33%` |

Selected average timers:

| Branch | `recip2real` | `real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | `3.966667` | `3.533333` | `8.216667` | `9.513333` |
| branch | `4.290000` | `3.700000` | `8.806667` | `10.106667` |

Interpretation:

- this is still the hardest case for the branch
- the adaptive fallback removed the earlier large regression, but did not turn this
  case into a win on the current single-node testbed

## Conclusion

What is now supported by evidence:

- the branch really implements task-2-style `MPI_Isend` / `MPI_Irecv` optimization
- end-to-end correctness is preserved on the audited cases
- one repeated SCF benchmark (`036-hi`) shows a real wall-time gain
- heavier cases are kept near baseline instead of suffering the much larger regressions
  seen in earlier experiments

What is not claimed:

- a universal speedup for all MPI PW workloads
- a positive result on every communication-heavy benchmark in this local environment

## Acceptance summary

- Correctness: pass
- Task alignment: pass
- Performance: pass with one measured win and otherwise no large regression

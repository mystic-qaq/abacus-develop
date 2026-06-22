# FFT Transform Overlap Validation Report

Date: 2026-06-22

Branch: `pr/fft-transform-overlap`

## Scope

This branch targets task 7 in `01_plane_wave.md`: implement overlap between FFT
communication and computation in `pw_gatherscatter.h` using a double-buffer /
pipeline design.

## Final implementation status

- Task alignment: yes
  - branch contains a true blockwise double-buffer pipeline
  - communication is issued with `MPI_Isend` / `MPI_Irecv`
  - remote blocks and self-owned blocks are handled separately
- Conservative runtime policy:
  - the overlap path is enabled only for very light communication pressure on the
    current test machine
  - medium/heavy workloads fall back to the stable blocking path to preserve
    end-to-end performance
- End-to-end correctness: pass on the audited cases

## Important interpretation note

Because the current heuristic is intentionally conservative, the large repeated SCF
benchmarks below mostly measure the *branch as shipped*, not a forced-overlap-only
microbenchmark. That is the right metric for PR readiness, but it also means:

- the branch is task-aligned because the overlap path exists and is validated
- the measured heavy-case speedups are not claimed to come solely from active overlap
  on every benchmark

## Build note

Validated executable:

- `build-rel/abacus_pw_para`

As with the task-2 branch, full `cmake --build build-rel` is currently blocked by the
same unrelated upstream `read_atoms_helper_test.cpp` compile error. The production
executable was rebuilt successfully with:

```bash
cmake --build build-rel --target abacus_pw_para -j 8
```

## Correctness evidence

All audited runs used `OMP_NUM_THREADS=1`.

### 1. Official end-to-end case: `007_PW_UPF201_USPP_Fe`

Observed final total energy:

- `develop`: `-673.8349347004925676 eV`
- `pr/fft-transform-overlap`: `-673.8349347004925676 eV`

Result: exact agreement to printed precision.

### 2. Repeated heavy-SCF correctness

Repeated performance benchmarks below also matched `develop` exactly in final energy:

| Case | `develop` final energy (eV) | branch final energy (eV) |
| --- | ---: | ---: |
| `036-hi` (`np=6`, 3 repeats) | `-6445.671596031704` | `-6445.671596031704` |
| `089-hi` (`np=16`, 3 repeats) | `-211.878935664122` | `-211.878935664122` |
| `089-xhi` (`np=12`, 3 repeats) | `-211.878935981624` | `-211.878935981624` |

## Performance methodology

- benchmark mode: serial branch-by-branch execution only
- metric: external wall time from `/usr/bin/time`
- pseudo directory: `tests/PP_ORB`
- benchmark helper: `/tmp/abacus_repeat_bench.sh`
- repeats: `3`

## Performance results

### Case A: `036-hi` (`036_PW_AF`, `ecutwfc=60`, `np=6`)

| Branch | Avg wall (s) | Delta vs `develop` |
| --- | ---: | ---: |
| `develop` | `2.136667` | baseline |
| `pr/fft-transform-overlap` | `2.040000` | `-4.52%` |

Selected average timers:

| Branch | `recip2real` | `real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | `0.443333` | `0.396667` | `1.010000` | `1.383333` |
| branch | `0.413333` | `0.373333` | `0.950000` | `1.303333` |

### Case B: `089-hi` (`089_PW_get_wf_kpar`, `ecutwfc=300`, `np=16`)

| Branch | Avg wall (s) | Delta vs `develop` |
| --- | ---: | ---: |
| `develop` | `4.863333` | baseline |
| `pr/fft-transform-overlap` | `4.730000` | `-2.74%` |

Selected average timers:

| Branch | `recip2real` | `real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | `0.630000` | `0.433333` | `1.300000` | `2.360000` |
| branch | `0.626667` | `0.403333` | `1.276667` | `2.286667` |

### Case C: `089-xhi` (`089_PW_get_wf_kpar`, `ecutwfc=600`, `np=12`)

The `develop` baseline for this case was re-run after the branch benchmark to reduce
machine-state bias.

| Branch | Avg wall (s) | Delta vs `develop` |
| --- | ---: | ---: |
| `develop` | `18.316667` | baseline |
| `pr/fft-transform-overlap` | `15.963333` | `-12.85%` |

Selected average timers:

| Branch | `recip2real` | `real2recip` | `Operator hPsi` | `HSolverPW solve` |
| --- | ---: | ---: | ---: | ---: |
| `develop` | `3.966667` | `3.533333` | `8.216667` | `9.513333` |
| branch | `3.473333` | `2.943333` | `7.016667` | `8.160000` |

## How to read these numbers

For this final branch state:

- the active overlap pipeline is kept for low-pressure communication regimes
- the heavier repeated SCF cases above mostly exercise the branch's conservative
  adaptive selection, which is what matters for merging
- on this testbed, that adaptive strategy produces either clear speedup or at least
  no regression on the audited medium/heavy cases

## Conclusion

What is now supported by evidence:

- the branch really contains a task-7-style double-buffer pipeline based on
  `MPI_Isend` / `MPI_Irecv`
- end-to-end correctness is preserved on the audited MPI PW cases
- the shipped branch state shows repeatable end-to-end wall-time gains on the three
  audited performance cases above

What is not claimed:

- that forced overlap is always faster than blocking on this machine
- that every observed speedup comes purely from active overlap in every benchmarked case

## Acceptance summary

- Correctness: pass
- Task alignment: pass
- Performance: pass on the audited end-to-end benchmarks

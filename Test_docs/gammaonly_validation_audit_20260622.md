# GammaOnly Validation Audit - 2026-06-22

Branch: `GammaOnly`

## Scope

This audit focuses on task 4 in `01_plane_wave.md`:

- in PW mode, when all k-points are Gamma points,
- `PW_Basis_K` should keep the GammaOnly half-spectrum path even for multi-k runs,
- so wavefunction storage is reduced roughly by half.

The key question is not "does a low-level FFT unit test pass", but:

- does a real SCF run with multiple Gamma k-points converge correctly,
- and does the log show that the half-spectrum PW path is truly active?

## Validated executable

- `build-pw-gamma/abacus_pw_para`

## Test case

Custom SCF case derived from the Si PW Gamma example:

- `2` k-points
- both k-points are exactly Gamma
- `symmetry 0`
- `ecutwfc 200`
- `init_wfc random`
- `pw_seed 1`

Compared pairs:

- full-complex PW (`gamma_only 0`)
- GammaOnly PW (`gamma_only 1`)

## End-to-end correctness evidence

### 1. `np = 1`

Observed final total energies:

| Mode | Final total energy (eV) |
| --- | ---: |
| full-complex | `-198.355050734022` |
| GammaOnly | `-198.3550507115897` |

Difference:

- `+2.2432288915e-08 eV`

This is comfortably within roundoff-level agreement for an end-to-end SCF comparison.

### 2. `np = 4`

Observed final total energies:

| Mode | Final total energy (eV) |
| --- | ---: |
| full-complex | `-198.3550507340123` |
| GammaOnly | `-198.3550507362148` |

Difference:

- `-2.2024835289e-09 eV`

MPI execution therefore remains consistent with the serial correctness result.

## Direct evidence that multi-k GammaOnly is really active

From the GammaOnly run log:

- `GammaOnly PW: 2 of 2 k-points are Gamma points. Full GammaOnly mode active (half-spectrum FFT for all k-points).`

This matters because it shows the branch is not merely forcing a single-k Gamma case;
the multi-k all-Gamma path is actually engaged.

## Storage / plane-wave reduction evidence

### `np = 1`

| Mode | total plane waves | `npwx` |
| --- | ---: | ---: |
| full-complex | `12627` | `12627` |
| GammaOnly | `6603` | `6603` |

### `np = 4`

| Mode | total plane waves | `npwx` |
| --- | ---: | ---: |
| full-complex | `12627` | `3157` |
| GammaOnly | `6603` | `1652` |

Observed reduction:

- total plane waves: `47.71%`
- `npwx` at `np=4`: `47.67%`

This is the core performance value currently demonstrated by the branch.

## Wall-time observation

### `np = 1`

| Mode | wall time (s) | `recip2real` | `real2recip` | `Operator hPsi` |
| --- | ---: | ---: | ---: | ---: |
| full-complex | `5.73` | `2.18` | `1.41` | `3.82` |
| GammaOnly | `9.11` | `3.80` | `3.03` | `7.56` |

### `np = 4`

| Mode | wall time (s) | `recip2real` | `real2recip` | `Operator hPsi` |
| --- | ---: | ---: | ---: | ---: |
| full-complex | `1.62` | `0.46` | `0.36` | `0.83` |
| GammaOnly | `2.96` | `0.98` | `0.98` | `2.13` |

Interpretation:

- the branch clearly wins on storage
- on the current test machine it does **not** yet win on wall time
- the extra solver / transform work still outweighs the compact-storage benefit in this case

## What is and is not claimed

Claimed:

- multi-k all-Gamma PW correctness
- real half-spectrum storage reduction in end-to-end runs
- MPI consistency (`np=1` and `np=4`)

Not claimed:

- mixed gamma/non-gamma PW wavefunction support in one run
- end-to-end wall-time speedup
- support for every solver/backend combination without fallback

## Acceptance summary

- Task alignment: pass for the all-Gamma multi-k target
- End-to-end correctness: pass
- MPI correctness: pass
- Storage reduction: pass
- Wall-time speedup: not demonstrated

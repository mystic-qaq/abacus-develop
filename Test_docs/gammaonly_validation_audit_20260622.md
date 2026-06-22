# GammaOnly validation audit - 2026-06-22

## Scope

Branch: `GammaOnly`

Goal: validate the PW `gamma_only=1` path with end-to-end SCF runs, not only low-level FFT tests, and record both what is supported and what must fall back.

## What changed during this audit

- Kept the compact-Gamma fixes that were required for correctness:
  - global Gamma compact weights for MPI
  - weighted `<beta|psi>` accumulation in `op_pw_nl.cpp`
  - weighted CG inner products for the compact Gamma metric
- Re-tested an experimental CG "conjugate-subspace projection" idea on a real Si/Gamma SCF case and found that it breaks convergence badly. That experiment was removed from the active code path and then deleted.
- Added conservative guards so that PW GammaOnly is only enabled for the currently validated path:
  - KPT must be a single Gamma point
  - `ks_solver` must be `cg`
  - USPP / double-grid calculations fall back to full-complex PW grids

## Low-level tests

Serial transform tests:

```text
PWBasisKTEST.SetupTransform
PWBasisKTEST.ComplexTransformRoundTrip
PWBasisKTEST.GammaRealForwardMatchesFullComplex
PWBasisKTEST.GammaProjectedInverseMatchesFullComplex
PWBasisKTEST.GammaComplexRealSpaceInputIsNotFullComplexEquivalent
```

All 5 tests passed on the current worktree.

## End-to-end correctness evidence

### 1. Supported path: single-Gamma PW + `ks_solver=cg`

Case: `tests/01_PW/022_PW_CG` (Si, Gamma 1x1x1), modified to `ecutwfc=200`, `init_wfc=random`, `pw_seed=1`.

Final total energies from `OUT.autotest/running_scf.log`:

| Case | np | gamma_only | Final total energy (eV) |
| --- | ---: | ---: | ---: |
| Si Gamma 1x1x1, CG | 1 | 0 | -198.3550507385 |
| Si Gamma 1x1x1, CG | 1 | 1 | -198.3550507185 |
| Si Gamma 1x1x1, CG | 4 | 0 | -198.3550507233 |
| Si Gamma 1x1x1, CG | 4 | 1 | -198.3550507233 |

Observations:

- `np=1` full vs GammaOnly differ by about `2.0e-8 eV`
- `np=4` full vs GammaOnly match to the printed precision
- MPI correctness is therefore supported for this validated path

### 2. Mixed/Non-Gamma k-points: safe fallback

Case: `tests/01_PW/022_PW_CG` with KPT changed to `2 2 2 0 0 0`, `gamma_only=1` requested.

| Case | gamma_only request | Final total energy (eV) |
| --- | ---: | ---: |
| Si 2x2x2 k-mesh | 0 | -212.9934339688 |
| Si 2x2x2 k-mesh | 1, fallback | -212.9934339688 |

Conclusion: mixed-k input now safely falls back to full-complex and matches the non-GammaOnly result exactly.

### 3. USPP / double-grid: safe fallback

Case: `tests/01_PW/003_PW_UPF100_USPP_Fe` (Fe USPP), modified to Gamma 1x1x1 and `ks_solver=cg`.

| Case | gamma_only request | Final total energy (eV) |
| --- | ---: | ---: |
| Fe USPP Gamma 1x1x1 | 0 | -2979.0180788015 |
| Fe USPP Gamma 1x1x1 | 1, fallback | -2979.0180788020 |

Conclusion: USPP / double-grid cases now fall back safely; the residual difference is only roundoff-level.

### 4. Davidson solver: safe fallback

Case: `tests/01_PW/023_PW_DA` (Si, Gamma 1x1x1), modified to `ecutwfc=200`, `init_wfc=random`, `pw_seed=1`.

| Case | ks_solver | gamma_only request | Final total energy (eV) |
| --- | --- | ---: | ---: |
| Si Gamma 1x1x1 | dav | 0 | -198.3550507383 |
| Si Gamma 1x1x1 | dav | 1, fallback | -198.3550507383 |

Important note:

- Before adding the `ks_solver=cg` guard, the GammaOnly + DAV path produced NaNs and non-convergence in real SCF tests.
- The current branch does not claim DAV support for GammaOnly; it now falls back instead of silently running an incorrect path.

## Performance evidence

### Real gain: memory / plane-wave-count reduction

Same supported Si/Gamma/CG case as above.

| np | gamma_only | total plane waves | npwx | MEMORY FOR PSI |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0 | 12627 | 12627 | 1.15604 MB |
| 1 | 1 | 6603 | 6603 | 0.604523 MB |
| 4 | 0 | 12627 | 3157 | 0.289032 MB |
| 4 | 1 | 6603 | 1652 | 0.151245 MB |

This is a real and reproducible memory benefit of about 48%.

### No demonstrated wall-time speedup yet

For the same validated Si/Gamma/CG case:

| np | gamma_only | wall time | `Operator hPsi` | `PW_Basis_K recip2real` | `PW_Basis_K real2recip` |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0 | 1.66 s | 0.84 s / 254 calls | 0.47 s | 0.31 s |
| 1 | 1 | 3.10 s | 2.31 s / 1022 calls | 1.15 s | 0.86 s |
| 4 | 0 | 1.33 s | 0.57 s / 254 calls | 0.28 s | 0.25 s |
| 4 | 1 | 1.89 s | 1.33 s / 934 calls | 0.55 s | 0.65 s |

Interpretation:

- GammaOnly clearly reduces storage and local `npwx`
- But the current compact-metric CG path still needs more iterative diagonalization work than the full-complex baseline
- Therefore this branch currently demonstrates a genuine memory benefit, but not an end-to-end wall-time speedup

## Rejected experiment

During this audit, a CG-side projection onto the explicit Gamma conjugate manifold was tested. On the real case above it caused severe correctness regressions:

- SCF failed to converge within 100 steps
- energies dropped to obviously unphysical values (down to about `-237 eV` on the Si test)
- `hPsi` calls exploded from hundreds to tens of thousands

That experiment was removed and is not part of the current result.

## Acceptance status for the current branch state

- Supported path `single Gamma + PW + ks_solver=cg`: pass
- MPI correctness on the supported path: pass (`np=1` and `np=4` re-verified in this audit)
- Mixed-k path: safe fallback, pass
- USPP / double-grid path: safe fallback, pass
- DAV path: not supported directly; safe fallback, pass
- End-to-end wall-time speedup: not demonstrated
- End-to-end memory reduction: demonstrated

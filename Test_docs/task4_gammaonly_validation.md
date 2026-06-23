# Task 4 GammaOnly Validation

Date: 2026-06-23

## Scope

Branch: `GammaOnly`

Baseline: `upstream/develop` at `777f50c9c`

Task branch head before local edits: `e73c62701` plus working-tree changes.

Verified merge-base with `upstream/develop`: `777f50c9c7650f9cd17b36ace9534a7f552fc8ae`.

This validation keeps `final` untouched and checks the all-Gamma multi-k GammaOnly path in its own worktree.

## Code Changes

- `PW_Basis_K::initparameters` now uses one tolerance-based all-Gamma decision for `gamma_only`.
- All-Gamma multi-k inputs keep the half-spectrum `r2c/c2r` dimensions.
- Mixed Gamma plus non-Gamma k-point inputs safely fall back to full-complex mode.
- Complex real-space input in GammaOnly mode is accepted only when its imaginary part is negligible; non-real input exits explicitly instead of silently dropping the imaginary component.
- `ElecStatePW::cal_becsum` now applies the same compact-Gamma inner-product weights used by the nonlocal Hamiltonian path before forming USPP augmentation charge projectors. This keeps `<beta|psi>` semantics consistent for half-spectrum wavefunctions.
- `Charge_Mixing` reciprocal-space residual and Hartree-like mixing inner products now use per-G compact conjugate weights instead of coarse Gamma-only `*2.0` factors. This keeps `G=0` and other self-conjugate components from being over-counted and makes the SCF residual comparable to the full-complex norm.
- `DiagoIterAssist::diag_subspace` accepts optional inner-product weights. `HSolverPW` passes the same Gamma compact weights used by CG so subspace Hamiltonian/overlap matrices are built with `<psi|W|Hpsi>` and `<psi|W|Spsi>` in half-spectrum mode.
- Updated the stale `nmaxgr` serial unit-test expectation from `4000` to the current full real-space work-buffer bound `8000`.

## Build

```bash
cmake -S . -B build-task4 \
  -DENABLE_MPI=ON -DBUILD_TESTING=ON \
  -DENABLE_LCAO=OFF -DUSE_ELPA=OFF -DENABLE_LIBXC=OFF \
  -DENABLE_LIBRI=OFF -DENABLE_PEXSI=OFF \
  -DENABLE_CNPY=OFF -DENABLE_RAPIDJSON=OFF \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-task4 --target \
  MODULE_PW_basis_pw_serial \
  MODULE_PW_basis_pw_k_serial \
  MODULE_PW_pw_test \
  abacus_pw_para -j 8
```

Result: build passed.

## Unit Tests

```bash
./build-task4/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_serial
./build-task4/source/source_basis/module_pw/test_serial/MODULE_PW_basis_pw_k_serial
OMP_NUM_THREADS=4 ./build-task4/source/source_basis/module_pw/test/MODULE_PW_pw_test --gtest_filter='*transform_omp*'
```

Results:

| Test | Result |
| --- | --- |
| `MODULE_PW_basis_pw_serial` | 13 passed |
| `MODULE_PW_basis_pw_k_serial --gtest_filter='*Gamma*:*RoundTrip*'` | 6 passed |
| `MODULE_PW_pw_test --gtest_filter='*transform_omp*'` | 0 tests on this branch |

New task-4 unit coverage:

| Test | Purpose |
| --- | --- |
| `AllGammaMultiKUsesHalfSpectrum` | all k-points Gamma, `gamma_only` remains active |
| `MixedGammaAndNonGammaFallsBackToFullComplex` | mixed k-point input disables half-spectrum safely |
| `GammaComplexRealSpaceInputRejectsNonRealData` | non-real complex input is explicitly rejected |

## Correctness Against Baseline

Run root: `/root/abacus_validation_runs/branch_cases`

Environment: same machine, `OMP_NUM_THREADS=1`, OpenMPI, CPU build.

| Case | MPI ranks | Baseline energy (eV) | Task4 energy (eV) | Abs diff (eV) |
| --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/022_PW_CG` | 4 | `-198.2238296277166` | `-198.2238296277166` | `0.0` |
| `tests/01_PW/036_PW_AF` | 4 | `-5866.197297493522` | `-5866.197297493522` | `0.0` |
| `tests/01_PW/089_PW_get_wf_kpar` | 3 | `-211.8789253814144` | `-211.8789253814144` | `0.0` |

Correctness target `<= 1e-14` passed at printed precision.

## Follow-up Numerical Audit

Run root: `/root/task4_gamma_check`

Environment: same machine, `OMP_NUM_THREADS=1`, OpenMPI, CPU build.

These cases were added after a report of slightly larger task-4 error. They compare the current task4 worktree against `upstream/develop` using temporary inputs outside the repository.

Important caveat: the first three full-program checks below have `gamma_only=0`, so they exercise the ordinary full-complex PW path even when the k-points are Gamma. They are useful as fallback/regression checks, but they do not validate the task-4 half-spectrum path.

| Case | Modification | MPI ranks | Baseline energy (eV) | Task4 energy (eV) | Abs diff (eV) |
| --- | --- | ---: | ---: | ---: | ---: |
| `tests/01_PW/009_PW_UPF201_USPP` | original mixed/non-Gamma `2x2x2` k mesh | 4 | `-426.8936030034611804` | `-426.8936030034611804` | `0.0` |
| `tests/01_PW/022_PW_CG` | temporary two repeated Gamma k-points, weights `0.5/0.5`, `symmetry=0` | 4 | `-198.2238296277164` | `-198.2238296277164` | `0.0` |
| `tests/01_PW/009_PW_UPF201_USPP` | temporary two repeated Gamma k-points, weights `0.5/0.5` | 4 | `-401.6356858821099536` | `-401.6356858821099536` | `0.0` |

With `gamma_only=1`, the task4 path preserves the repeated all-Gamma k-points and activates half-spectrum wavefunction storage. Upstream `develop` collapses the same repeated-Gamma input to one Gamma k-point/full-complex behavior, so this comparison is physical-equivalence rather than identical execution-path equivalence.

| Case | Modification | MPI ranks | Reference energy (eV) | Task4 energy (eV) | Abs diff (eV) | Notes |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `tests/01_PW/022_PW_CG` | two repeated Gamma k-points, `gamma_only=1`, `scf_thr=1e-8` | 4 | `-198.2238296277166` | `-198.2238239787911` | `5.65e-6` | Half-spectrum active, `npwx=59`; converged by `DRHO`, but energy still drifting. |
| `tests/01_PW/022_PW_CG` | same, `scf_thr=1e-12` | 4 | `-198.2238296277166` | `-198.2238296307621` | `3.05e-9` | Difference shrinks with tighter SCF convergence. |
| `tests/01_PW/009_PW_UPF201_USPP` | two repeated Gamma k-points, `gamma_only=1`, `scf_thr=1e-9` | 4 | `-401.6356858800573946` | `-401.6356858821099536` | `2.05e-9` | USPP/double-grid wavefunction path safely falls back full-complex. |

The table above used the default full-complex energy as a historical reference. A stricter and fairer high-precision comparison must run both full-complex and GammaOnly with the same tightened thresholds. After the compact-weight fixes, the following same-branch same-threshold checks were run with `OMP_NUM_THREADS=1` and the temporary two-Gamma 022 input:

| MPI ranks | Mode | `gamma_only` | `scf_thr` | `pw_diag_thr` | Mixing | `npwx` | Iterations | Final energy (eV) |
| ---: | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| 1 | full-complex | 0 | `1e-8` | default | plain, `beta=0.5` | 411 | 15 | `-198.2238296277240` |
| 1 | full-complex tight | 0 | `1e-12` | `1e-12` | plain, `beta=0.5` | 411 | 23 | `-198.2238296313677` |
| 1 | GammaOnly tight | 1 | `1e-12` | `1e-12` | plain, `beta=0.5` | 237 | 92 | `-198.2238296311491` |
| 4 | full-complex tight | 0 | `1e-12` | `1e-12` | plain, `beta=0.5` | 104 | 23 | `-198.2238296313608` |
| 4 | GammaOnly tight | 1 | `1e-12` | `1e-12` | plain, `beta=0.5` | 59 | 74 | `-198.2238296311486` |

Tight full-complex vs tight GammaOnly absolute energy difference: `2.186e-10 eV` at one MPI rank and `2.122e-10 eV` at four MPI ranks.

Conclusion of the follow-up audit: the originally reported larger error was real for the default `scf_thr=1e-8`, but the default full-complex result was also not a strict high-precision reference. With matched tighter thresholds, the half-spectrum path agrees with the full-complex path within `1e-9 eV`. This passes the stated minimum acceptance target. It does not yet demonstrate a `1e-14` PR-grade target; attempting `scf_thr=1e-14` on this repeated-Gamma case was unstable with plain mixing, so any future `1e-14` claim needs a more robust convergence strategy or a less degenerate validation case.

## GammaOnly Memory Evidence

Existing branch audit data remains relevant for the all-Gamma multi-k target:

| Mode | MPI ranks | Total plane waves | `npwx` |
| --- | ---: | ---: | ---: |
| full-complex | 1 | `12627` | `12627` |
| GammaOnly | 1 | `6603` | `6603` |
| full-complex | 4 | `12627` | `3157` |
| GammaOnly | 4 | `6603` | `1652` |

Observed reduction:

- total plane waves: `47.71%`
- `npwx` at `np=4`: `47.67%`

The new unit test also verifies the structural half-spectrum condition directly: all-Gamma multi-k keeps `fftnx = nx / 2 + 1` with `xprime=true`, while mixed k-points keep `fftnx = nx`.

## Conclusion

Build, unit tests, mixed-k fallback, non-real complex-input handling, and the structural all-Gamma half-spectrum activation pass.

The full-program numerical audit found that default `scf_thr=1e-8` is too loose for high-precision GammaOnly validation. With matched `scf_thr=1e-12` and `pw_diag_thr=1e-12`, the all-Gamma half-spectrum path agrees with the full-complex path to `2.186e-10 eV` at one MPI rank and `2.122e-10 eV` at four MPI ranks, passing the `1e-9` minimum target.

Performance claim remains memory-focused. Existing end-to-end branch audit shows strong storage reduction but no stable wall-time speedup on the measured case.

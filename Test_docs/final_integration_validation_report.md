# Final Integration Validation Report

Date: 2026-06-22

Branch: `final`

## Scope

This report validates the current integrated `final` branch after manually folding in
the latest audited task-2 / task-4 / task-7 work:

- task 2: nonblocking point-to-point gather/scatter path
- task 4: multi-k all-Gamma `GammaOnly` path
- task 7: double-buffer overlap pipeline

The focus here is the merged branch itself, not the feature branches in isolation.

## Build note

Validated executable:

- `build-rel/abacus_pw_para`

Build command:

```bash
cmake --build build-rel --target abacus_pw_para -j 8
```

As in earlier audits, full `cmake --build build-rel` is still blocked by the unrelated
upstream unit-test compile error in
`source/source_cell/test/read_atoms_helper_test.cpp`.

## Correctness validation

All audited runs used `OMP_NUM_THREADS=1`.

### 1. `007_PW_UPF201_USPP_Fe` (`np=6`)

Raw output:

- `/tmp/final-validation-correctness/007_np6.txt`

Observed final total energy:

- `-673.8349347004925676 eV`

Result:

- matches the audited `develop` baseline exactly to printed precision

### 2. `089_PW_get_wf_kpar` stock case (`np=6`)

Raw output:

- `/tmp/final-validation-correctness/089_base_np6.txt`

Observed final total energy:

- `-211.878925381389 eV`

Observed `.cube` integrals:

- `wfi1s1k1.cube = 21.94720511`
- `wfi1s1k2.cube = 19.65846447`
- `wfi1s1k3.cube = 19.34110139`

Result:

- matches the earlier audited baseline exactly

### 3. GammaOnly multi-k all-Gamma case

Case source:

- `022_PW_CG` modified to `2` Gamma k-points, `symmetry 0`, `ecutwfc 200`,
  `init_wfc random`, `pw_seed 1`

Raw outputs:

- `/tmp/final-validation-correctness/gamma_full_np1.txt`
- `/tmp/final-validation-correctness/gamma_gamma_np1.txt`
- `/tmp/final-validation-correctness/gamma_full_np4.txt`
- `/tmp/final-validation-correctness/gamma_gamma_np4.txt`

Observed final energies:

| Mode | `np=1` (eV) | `np=4` (eV) |
| --- | ---: | ---: |
| full-complex | `-198.355050734022` | `-198.3550507340123` |
| GammaOnly | `-198.3550507115897` | `-198.3550507362148` |

Differences:

- `np=1`: `+2.2432288915e-08 eV`
- `np=4`: `-2.2024835289e-09 eV`

Direct log evidence of real GammaOnly activation:

- `GammaOnly PW: 2 of 2 k-points are Gamma points. Full GammaOnly mode active (half-spectrum FFT for all k-points).`

Storage reduction from the same logs:

- total plane waves: `12627 -> 6603`
- `npwx` at `np=4`: `3157 -> 1652`

Result:

- integrated `final` preserves the audited task-4 correctness path
- GammaOnly is really active in the merged branch, not silently falling back

## Performance validation

### Method

- sequential branch-by-branch execution only
- metric: external wall time from `/usr/bin/time`
- repeats: `3`
- pseudo directory: `tests/PP_ORB`
- helper script: `/tmp/abacus_repeat_bench.sh`

Raw directories:

- `/tmp/final-bench-results/develop-036-hi`
- `/tmp/final-bench-results/final-036-hi`
- `/tmp/final-bench-results/develop-089-hi`
- `/tmp/final-bench-results/final-089-hi`
- `/tmp/final-bench-results/develop-089-xhi`
- `/tmp/final-bench-results/final-089-xhi`
- rerun for stability check: `/tmp/final-bench-results/final-089-xhi-rerun`

### Results

| Case | `develop` avg wall (s) | `final` avg wall (s) | Delta vs `develop` |
| --- | ---: | ---: | ---: |
| `036-hi` (`036_PW_AF`, `ecutwfc=60`, `np=6`) | `2.033333` | `2.040000` | `+0.33%` |
| `089-hi` (`089_PW_get_wf_kpar`, `ecutwfc=300`, `np=16`) | `4.646667` | `4.706667` | `+1.29%` |
| `089-xhi` first run (`ecutwfc=600`, `np=12`) | `15.990000` | `17.983333` | `+12.47%` |
| `089-xhi` rerun (`ecutwfc=600`, `np=12`) | `15.990000` | `16.676667` | `+4.29%` |

Energy consistency in all repeated benchmarks:

- `036-hi`: exact match
- `089-hi`: exact match
- `089-xhi`: exact match

### Interpretation

- correctness is stable in all repeated runs
- `036-hi` and `089-hi` are close to neutral, but do not show a speedup on the
  merged branch in this final audit
- `089-xhi` shows visible regression relative to current `develop`
- one of the first `089-xhi` repeats was an outlier (`20.19 s`), so a second
  `3`-repeat rerun was added; the rerun reduced the regression, but it still did
  not turn into a win

## Conclusion

What is supported by this final integrated audit:

- task-2 / task-4 / task-7 code can coexist in `final` without breaking audited
  end-to-end correctness
- GammaOnly multi-k all-Gamma mode remains genuinely active after integration
- all audited final energies match baseline expectations

What is **not** supported by this final integrated audit:

- a convincing end-to-end performance win for the merged `final` branch
- a PR-ready claim that integration preserved the branch-level speedups seen in
  earlier isolated audits

## Acceptance status

- Integrated correctness: pass
- Integrated GammaOnly activation: pass
- Integrated performance: not yet accepted


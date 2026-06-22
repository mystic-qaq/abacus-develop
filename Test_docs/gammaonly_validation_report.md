# GammaOnly validation report

Date: 2026-06-22

This file is a short front page. The detailed end-to-end audit is:

- `Test_docs/gammaonly_validation_audit_20260622.md`

## Current validated status

- Supported path: `PW + single Gamma KPT + ks_solver=cg`
- Re-verified end-to-end on real SCF cases for `np=1` and `np=4`
- Mixed-k inputs: safe fallback to full-complex
- USPP / double-grid inputs: safe fallback to full-complex
- `ks_solver=dav`: safe fallback to full-complex

## What is and is not claimed

- Claimed:
  - end-to-end correctness for the supported CG path
  - safe fallback for unsupported paths
  - real memory reduction for the supported path
- Not claimed:
  - end-to-end wall-time speedup
  - direct GammaOnly support for DAV

## Important audit outcome

An experimental CG-side conjugate-subspace projection was tested during this audit and rejected because it caused severe end-to-end correctness regressions on a real Si/Gamma SCF case. The current branch state does not use that projection.

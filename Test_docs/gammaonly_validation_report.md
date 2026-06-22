# GammaOnly Validation Report

Date: 2026-06-22

Branch: `GammaOnly`

## Front-page conclusion

This branch now supports the task-4 target that matters most:

- PW mode
- multiple k-points
- all k-points are Gamma points
- `PW_Basis_K` stays on the half-spectrum storage path

What is demonstrated:

- end-to-end correctness on a real SCF case with `2` Gamma k-points
- real storage reduction of about `47.7%`
- exact log evidence that the multi-k GammaOnly path is active

What is not claimed:

- wall-time speedup on the current test machine
- mixed gamma/non-gamma k-point support inside the same PW wavefunction run

Detailed evidence is recorded in:

- `Test_docs/gammaonly_validation_audit_20260622.md`

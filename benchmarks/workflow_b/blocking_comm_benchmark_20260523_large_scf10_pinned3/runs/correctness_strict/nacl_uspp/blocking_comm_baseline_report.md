# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 1 | 86.770 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 1 | 86.770 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_K | 1 | 11.620 +/- 0.000 | 0.368170 +/- 0.000000 | 0.162057 +/- 0.000000 | 0.040707 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_Sup | 1 | 11.620 +/- 0.000 | 0.450187 +/- 0.000000 | 0.174807 +/- 0.000000 | 0.171125 +/- 0.000000 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 86.770 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 86.770 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 11.620 | 0.250984 | 0.368170 | 0.162057 | 0.040707 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 11.620 | 0.372298 | 0.450187 | 0.174807 | 0.171125 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 5930 | 0.030980 | 0.030980 | 0.030980 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 6986 | 0.085263 | 0.085263 | 0.085263 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 294 | 0.402072 | 0.402072 | 0.402072 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 219 | 0.557299 | 0.557299 | 0.557299 | 0.000000 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 5822 | 0.139677 | 0.100177 | 0.247654 | 0.147477 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 5822 | 0.007459 | 0.006642 | 0.008196 | 0.001554 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 5822 | 0.158037 | 0.118574 | 0.265119 | 0.146545 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 5822 | 0.007297 | 0.007064 | 0.007542 | 0.000478 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 6782 | 0.111308 | 0.105936 | 0.120516 | 0.014580 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 6782 | 0.009648 | 0.007804 | 0.010707 | 0.002903 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 6782 | 0.008223 | 0.007897 | 0.008724 | 0.000827 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 6782 | 0.141972 | 0.139190 | 0.148557 | 0.009367 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 6782 | 0.008080 | 0.007259 | 0.008778 | 0.001519 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 270 | 0.163536 | 0.156562 | 0.170529 | 0.013967 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 270 | 0.024217 | 0.023312 | 0.025046 | 0.001734 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 270 | 0.267502 | 0.263054 | 0.273412 | 0.010358 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 270 | 0.079428 | 0.075833 | 0.082527 | 0.006694 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 201 | 0.208763 | 0.118818 | 0.279658 | 0.160840 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 201 | 0.035227 | 0.033856 | 0.036061 | 0.002205 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 201 | 0.017969 | 0.017444 | 0.018675 | 0.001231 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 201 | 0.276559 | 0.186171 | 0.347090 | 0.160919 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 201 | 0.014283 | 0.013639 | 0.015253 | 0.001614 |

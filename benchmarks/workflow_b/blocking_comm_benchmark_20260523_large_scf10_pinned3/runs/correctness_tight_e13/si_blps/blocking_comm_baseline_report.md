# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 1 | 10.530 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 1 | 10.530 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_K | 1 | 3.040 +/- 0.000 | 0.317642 +/- 0.000000 | 0.126794 +/- 0.000000 | 0.035688 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_Sup | 1 | 3.040 +/- 0.000 | 0.138464 +/- 0.000000 | 0.041737 +/- 0.000000 | 0.056554 +/- 0.000000 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 10.530 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 10.530 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 3.040 | 0.253437 | 0.317642 | 0.126794 | 0.035688 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 3.040 | 0.117399 | 0.138464 | 0.041737 | 0.056554 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 3366 | 0.026389 | 0.026389 | 0.026389 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 4086 | 0.082096 | 0.082096 | 0.082096 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.125200 | 0.125200 | 0.125200 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 55 | 0.204334 | 0.204334 | 0.204334 | 0.000000 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.149690 | 0.096041 | 0.206119 | 0.110078 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 3362 | 0.006372 | 0.005002 | 0.007452 | 0.002450 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 3362 | 0.164631 | 0.112524 | 0.219015 | 0.106491 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 3362 | 0.005893 | 0.005516 | 0.006380 | 0.000864 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 4082 | 0.103747 | 0.094807 | 0.111523 | 0.016716 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 4082 | 0.008850 | 0.007522 | 0.010098 | 0.002576 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 4082 | 0.007143 | 0.005477 | 0.008680 | 0.003203 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 4082 | 0.130809 | 0.126477 | 0.134538 | 0.008061 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 4082 | 0.007431 | 0.005905 | 0.008778 | 0.002873 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.062741 | 0.051037 | 0.073808 | 0.022771 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 64 | 0.009616 | 0.008815 | 0.010268 | 0.001453 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 64 | 0.090553 | 0.079615 | 0.101069 | 0.021454 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 64 | 0.018100 | 0.017686 | 0.018425 | 0.000739 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 55 | 0.054658 | 0.045690 | 0.064656 | 0.018966 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 55 | 0.013808 | 0.012816 | 0.015154 | 0.002338 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 55 | 0.008824 | 0.008466 | 0.009064 | 0.000598 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 55 | 0.083598 | 0.074562 | 0.095148 | 0.020586 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 55 | 0.006206 | 0.005353 | 0.006743 | 0.001390 |

# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 1 | 18.840 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 1 | 18.840 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_K | 1 | 4.440 +/- 0.000 | 0.434744 +/- 0.000000 | 0.022790 +/- 0.000000 | 0.065947 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_Sup | 1 | 4.440 +/- 0.000 | 0.140013 +/- 0.000000 | 0.039733 +/- 0.000000 | 0.056423 +/- 0.000000 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 18.840 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 18.840 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 4.440 | 0.423092 | 0.434744 | 0.022790 | 0.065947 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 4.440 | 0.119613 | 0.140013 | 0.039733 | 0.056423 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 12278 | 0.038165 | 0.038165 | 0.038165 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 15542 | 0.089841 | 0.089841 | 0.089841 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 208 | 0.100131 | 0.100131 | 0.100131 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 149 | 0.102598 | 0.102598 | 0.102598 | 0.000000 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 12161 | 0.193115 | 0.184385 | 0.201252 | 0.016867 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 12161 | 0.011438 | 0.010890 | 0.012183 | 0.001293 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 12161 | 0.223316 | 0.216259 | 0.230877 | 0.014618 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 12161 | 0.011743 | 0.011275 | 0.012099 | 0.000824 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 15425 | 0.229976 | 0.227569 | 0.233492 | 0.005923 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 15425 | 0.014659 | 0.014233 | 0.015469 | 0.001236 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 15425 | 0.014337 | 0.013651 | 0.015336 | 0.001685 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 15425 | 0.282689 | 0.280763 | 0.284575 | 0.003812 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 15425 | 0.013770 | 0.013099 | 0.014654 | 0.001555 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 208 | 0.050890 | 0.049901 | 0.051679 | 0.001778 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 208 | 0.008008 | 0.007950 | 0.008082 | 0.000132 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 208 | 0.085899 | 0.084476 | 0.087251 | 0.002775 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 208 | 0.026807 | 0.025516 | 0.028106 | 0.002590 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 149 | 0.068722 | 0.050379 | 0.088334 | 0.037955 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 149 | 0.012031 | 0.011619 | 0.012550 | 0.000931 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 149 | 0.004655 | 0.004573 | 0.004713 | 0.000140 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 149 | 0.090489 | 0.072533 | 0.109839 | 0.037306 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 149 | 0.004922 | 0.004821 | 0.005009 | 0.000188 |

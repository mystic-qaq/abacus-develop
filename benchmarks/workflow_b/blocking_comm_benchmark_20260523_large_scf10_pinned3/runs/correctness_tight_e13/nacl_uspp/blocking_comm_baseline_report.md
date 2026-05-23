# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 1 | 83.620 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 1 | 83.620 +/- 0.000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_K | 1 | 10.720 +/- 0.000 | 0.324002 +/- 0.000000 | 0.149766 +/- 0.000000 | 0.035594 +/- 0.000000 |
| large | 8 | 2 | PW_Basis_Sup | 1 | 10.720 +/- 0.000 | 0.362757 +/- 0.000000 | 0.085318 +/- 0.000000 | 0.156584 +/- 0.000000 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 83.620 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 83.620 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 10.720 | 0.218003 | 0.324002 | 0.149766 | 0.035594 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 10.720 | 0.303453 | 0.362757 | 0.085318 | 0.156584 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 5558 | 0.028753 | 0.028753 | 0.028753 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 6566 | 0.080439 | 0.080439 | 0.080439 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 282 | 0.381485 | 0.381485 | 0.381485 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 210 | 0.534731 | 0.534731 | 0.534731 | 0.000000 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 5126 | 0.125602 | 0.085612 | 0.222999 | 0.137387 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 5126 | 0.006436 | 0.005758 | 0.006878 | 0.001120 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 5126 | 0.141684 | 0.102166 | 0.238409 | 0.136243 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 5126 | 0.006472 | 0.006300 | 0.006761 | 0.000461 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 5990 | 0.092401 | 0.088624 | 0.101003 | 0.012379 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 5990 | 0.008555 | 0.007145 | 0.009168 | 0.002023 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 5990 | 0.007116 | 0.006798 | 0.007464 | 0.000666 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 5990 | 0.119303 | 0.116405 | 0.125632 | 0.009227 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 5990 | 0.007015 | 0.006505 | 0.007661 | 0.001156 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 246 | 0.150381 | 0.146893 | 0.155887 | 0.008994 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 246 | 0.022890 | 0.022209 | 0.025096 | 0.002887 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 246 | 0.244394 | 0.240477 | 0.248676 | 0.008199 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 246 | 0.070817 | 0.069856 | 0.072737 | 0.002881 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 183 | 0.153071 | 0.130546 | 0.206870 | 0.076324 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 183 | 0.033058 | 0.032121 | 0.034670 | 0.002549 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 183 | 0.016370 | 0.016050 | 0.016750 | 0.000700 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 183 | 0.216238 | 0.193718 | 0.269784 | 0.076066 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 183 | 0.013448 | 0.012826 | 0.014049 | 0.001223 |

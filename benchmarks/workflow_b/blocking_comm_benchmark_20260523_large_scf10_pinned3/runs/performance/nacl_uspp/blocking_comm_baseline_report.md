# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 3 | 47.017 +/- 0.091 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 3 | 47.017 +/- 0.091 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 2 | 1 | PW_Basis_K | 3 | 26.287 +/- 0.055 | 0.100515 +/- 0.023375 | 0.045664 +/- 0.038354 | 0.059000 +/- 0.000504 |
| large | 2 | 1 | PW_Basis_Sup | 3 | 26.287 +/- 0.055 | 0.614784 +/- 0.089439 | 0.337974 +/- 0.078311 | 0.446241 +/- 0.004270 |
| large | 4 | 1 | PW_Basis_K | 3 | 15.227 +/- 0.025 | 0.217469 +/- 0.003071 | 0.128588 +/- 0.004529 | 0.040572 +/- 0.000698 |
| large | 4 | 1 | PW_Basis_Sup | 3 | 15.227 +/- 0.025 | 0.474322 +/- 0.037891 | 0.249268 +/- 0.056375 | 0.273498 +/- 0.001051 |
| large | 4 | 2 | PW_Basis_K | 3 | 12.670 +/- 0.125 | 0.199895 +/- 0.005758 | 0.078461 +/- 0.003738 | 0.047423 +/- 0.003546 |
| large | 4 | 2 | PW_Basis_Sup | 3 | 12.670 +/- 0.125 | 0.409607 +/- 0.051856 | 0.164641 +/- 0.079482 | 0.150399 +/- 0.001975 |
| large | 8 | 1 | PW_Basis_K | 3 | 8.940 +/- 0.020 | 0.226085 +/- 0.011378 | 0.116573 +/- 0.007234 | 0.016785 +/- 0.000072 |
| large | 8 | 1 | PW_Basis_Sup | 3 | 8.940 +/- 0.020 | 0.309243 +/- 0.018220 | 0.100644 +/- 0.016666 | 0.151305 +/- 0.000040 |
| large | 8 | 2 | PW_Basis_K | 3 | 7.360 +/- 0.050 | 0.179755 +/- 0.003594 | 0.080408 +/- 0.001316 | 0.020411 +/- 0.000249 |
| large | 8 | 2 | PW_Basis_Sup | 3 | 7.360 +/- 0.050 | 0.248605 +/- 0.008707 | 0.078755 +/- 0.008195 | 0.094515 +/- 0.001234 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 47.120 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 47.120 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_K | 26.260 | 0.083448 | 0.120338 | 0.073779 | 0.059526 |
| large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_Sup | 26.260 | 0.386627 | 0.518494 | 0.263734 | 0.446936 |
| large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_K | 15.200 | 0.151835 | 0.218991 | 0.127351 | 0.041322 |
| large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_Sup | 15.200 | 0.363719 | 0.431638 | 0.184287 | 0.274379 |
| large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_K | 12.570 | 0.157767 | 0.196306 | 0.076112 | 0.051494 |
| large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_Sup | 12.570 | 0.328425 | 0.394546 | 0.151319 | 0.151340 |
| large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_K | 8.960 | 0.139207 | 0.218675 | 0.111627 | 0.016703 |
| large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_Sup | 8.960 | 0.283729 | 0.328694 | 0.103872 | 0.151293 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 7.360 | 0.118904 | 0.178659 | 0.081266 | 0.020542 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 7.360 | 0.211852 | 0.247438 | 0.069295 | 0.095939 |
| large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_K | 46.950 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_Sup | 46.950 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_K | 26.350 | 0.075846 | 0.106467 | 0.061242 | 0.058521 |
| large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_Sup | 26.350 | 0.485362 | 0.695264 | 0.419804 | 0.441666 |
| large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_K | 15.230 | 0.159277 | 0.219482 | 0.133607 | 0.039941 |
| large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_Sup | 15.230 | 0.391348 | 0.503989 | 0.285109 | 0.272335 |
| large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_K | 12.810 | 0.165801 | 0.206537 | 0.082771 | 0.045764 |
| large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_Sup | 12.810 | 0.388564 | 0.467327 | 0.249942 | 0.151729 |
| large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_K | 8.940 | 0.137813 | 0.220394 | 0.113216 | 0.016823 |
| large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_Sup | 8.940 | 0.260444 | 0.292575 | 0.082600 | 0.151349 |
| large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_K | 7.410 | 0.124837 | 0.183769 | 0.081066 | 0.020567 |
| large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_Sup | 7.410 | 0.205611 | 0.240540 | 0.083685 | 0.093756 |
| large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_K | 46.980 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_Sup | 46.980 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_K | 26.250 | 0.073753 | 0.074739 | 0.001972 | 0.058953 |
| large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_Sup | 26.250 | 0.465400 | 0.630593 | 0.330385 | 0.450122 |
| large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_K | 15.250 | 0.155272 | 0.213935 | 0.124806 | 0.040452 |
| large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_Sup | 15.250 | 0.373337 | 0.487338 | 0.278407 | 0.273779 |
| large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_K | 12.630 | 0.159255 | 0.196842 | 0.076499 | 0.045011 |
| large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_Sup | 12.630 | 0.324501 | 0.366949 | 0.092661 | 0.148129 |
| large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_K | 8.920 | 0.158440 | 0.239185 | 0.124875 | 0.016831 |
| large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_Sup | 8.920 | 0.259210 | 0.306461 | 0.115459 | 0.151273 |
| large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_K | 7.310 | 0.119022 | 0.176836 | 0.078893 | 0.020124 |
| large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_Sup | 7.310 | 0.218301 | 0.257836 | 0.083285 | 0.093849 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 2908 | 0.015245 | 0.015245 | 0.015245 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 3388 | 0.041872 | 0.041872 | 0.041872 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.205314 | 0.205314 | 0.205314 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.291781 | 0.291781 | 0.291781 | 0.000000 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 2914 | 0.059249 | 0.025874 | 0.092624 | 0.066750 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_pack | 2914 | 0.007526 | 0.007353 | 0.007698 | 0.000345 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_scatters | 2914 | 0.081045 | 0.047570 | 0.114520 | 0.066950 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_unpack | 2914 | 0.012296 | 0.011945 | 0.012647 | 0.000702 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_alltoallv | 3394 | 0.024199 | 0.020685 | 0.027714 | 0.007029 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_clear | 3394 | 0.023324 | 0.022811 | 0.023837 | 0.001026 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_pack | 3394 | 0.008472 | 0.008348 | 0.008596 | 0.000248 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_scatterp | 3394 | 0.066424 | 0.063740 | 0.069108 | 0.005368 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_unpack | 3394 | 0.007908 | 0.007754 | 0.008062 | 0.000308 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.195734 | 0.152682 | 0.238786 | 0.086104 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.109052 | 0.106683 | 0.111421 | 0.004738 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.421304 | 0.365712 | 0.476897 | 0.111185 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.116158 | 0.101270 | 0.131046 | 0.029776 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.190893 | 0.102078 | 0.279708 | 0.177630 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.065187 | 0.062050 | 0.068324 | 0.006274 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.082477 | 0.082403 | 0.082550 | 0.000147 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.413022 | 0.329923 | 0.496121 | 0.166198 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.074062 | 0.071552 | 0.076572 | 0.005020 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 2910 | 0.104383 | 0.049118 | 0.165517 | 0.116399 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_pack | 2910 | 0.004451 | 0.004193 | 0.004789 | 0.000596 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_scatters | 2910 | 0.118337 | 0.063213 | 0.179487 | 0.116274 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_unpack | 2910 | 0.007609 | 0.006995 | 0.008151 | 0.001156 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_alltoallv | 3390 | 0.047452 | 0.042522 | 0.053474 | 0.010952 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_clear | 3390 | 0.019291 | 0.018614 | 0.019963 | 0.001349 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_pack | 3390 | 0.004924 | 0.004879 | 0.004974 | 0.000095 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_scatterp | 3390 | 0.079206 | 0.075180 | 0.084314 | 0.009134 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_unpack | 3390 | 0.005046 | 0.004743 | 0.005243 | 0.000500 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.160027 | 0.111454 | 0.185287 | 0.073833 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.059319 | 0.058238 | 0.060808 | 0.002570 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.296923 | 0.246761 | 0.324673 | 0.077912 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.077252 | 0.074186 | 0.080777 | 0.006591 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.203691 | 0.135897 | 0.246351 | 0.110454 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.047173 | 0.046260 | 0.048032 | 0.001772 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.042091 | 0.041664 | 0.043098 | 0.001434 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.341851 | 0.274897 | 0.385232 | 0.110335 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.048545 | 0.047387 | 0.049056 | 0.001669 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 2896 | 0.085322 | 0.057615 | 0.158280 | 0.100665 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_pack | 2896 | 0.002849 | 0.002466 | 0.003212 | 0.000746 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_scatters | 2896 | 0.093084 | 0.065710 | 0.165549 | 0.099839 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_unpack | 2896 | 0.003063 | 0.002989 | 0.003154 | 0.000165 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_alltoallv | 3376 | 0.053885 | 0.049433 | 0.060395 | 0.010962 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_clear | 3376 | 0.004164 | 0.003498 | 0.004545 | 0.001047 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_pack | 3376 | 0.003508 | 0.003392 | 0.003584 | 0.000192 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_scatterp | 3376 | 0.067104 | 0.063342 | 0.072684 | 0.009342 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_unpack | 3376 | 0.003118 | 0.002979 | 0.003326 | 0.000347 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.146281 | 0.138553 | 0.165829 | 0.027276 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.027362 | 0.025543 | 0.029493 | 0.003950 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.224395 | 0.215416 | 0.243714 | 0.028298 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.050509 | 0.049132 | 0.054263 | 0.005131 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.137448 | 0.086269 | 0.162865 | 0.076596 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.039077 | 0.037831 | 0.040015 | 0.002184 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.020308 | 0.019637 | 0.021241 | 0.001604 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.211151 | 0.161128 | 0.237030 | 0.075902 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.014037 | 0.012186 | 0.015111 | 0.002925 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 2899 | 0.090724 | 0.056788 | 0.126112 | 0.069324 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_pack | 2899 | 0.005598 | 0.005259 | 0.005935 | 0.000676 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_scatters | 2899 | 0.117809 | 0.077815 | 0.146161 | 0.068346 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_unpack | 2899 | 0.019660 | 0.013009 | 0.038428 | 0.025419 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_alltoallv | 3379 | 0.067043 | 0.063406 | 0.070194 | 0.006788 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_clear | 3379 | 0.013063 | 0.012510 | 0.013689 | 0.001179 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_pack | 3379 | 0.006458 | 0.006347 | 0.006644 | 0.000297 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_scatterp | 3379 | 0.095622 | 0.092857 | 0.097829 | 0.004972 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_unpack | 3379 | 0.006715 | 0.006359 | 0.007014 | 0.000655 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.144006 | 0.116311 | 0.163353 | 0.047042 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_pack | 149 | 0.030898 | 0.028547 | 0.032808 | 0.004261 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 149 | 0.219250 | 0.189418 | 0.239515 | 0.050097 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 149 | 0.044118 | 0.043677 | 0.044472 | 0.000795 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.184419 | 0.126916 | 0.231193 | 0.104277 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_clear | 112 | 0.036722 | 0.034927 | 0.038320 | 0.003393 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_pack | 112 | 0.023291 | 0.022701 | 0.023631 | 0.000930 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 112 | 0.260970 | 0.202547 | 0.309133 | 0.106586 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_unpack | 112 | 0.016311 | 0.015972 | 0.016702 | 0.000730 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 2898 | 0.065759 | 0.046504 | 0.120969 | 0.074465 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 2898 | 0.003731 | 0.003282 | 0.003975 | 0.000693 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 2898 | 0.074970 | 0.056109 | 0.129598 | 0.073489 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 2898 | 0.003721 | 0.003564 | 0.003886 | 0.000322 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 3378 | 0.053145 | 0.050889 | 0.057690 | 0.006801 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 3378 | 0.004935 | 0.004107 | 0.005337 | 0.001230 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 3378 | 0.004088 | 0.003790 | 0.004302 | 0.000512 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 3378 | 0.068568 | 0.067140 | 0.071584 | 0.004444 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 3378 | 0.004065 | 0.003622 | 0.004324 | 0.000702 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.093968 | 0.088195 | 0.097389 | 0.009194 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 149 | 0.013290 | 0.012855 | 0.013621 | 0.000766 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 149 | 0.151801 | 0.143543 | 0.169211 | 0.025668 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 149 | 0.044366 | 0.041430 | 0.061828 | 0.020398 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.117885 | 0.089948 | 0.150049 | 0.060101 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 112 | 0.019642 | 0.019261 | 0.019869 | 0.000608 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 112 | 0.010261 | 0.009747 | 0.010762 | 0.001015 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 112 | 0.156349 | 0.128510 | 0.188524 | 0.060014 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 112 | 0.008381 | 0.008145 | 0.008774 | 0.000629 |
| large_rep02_np1_omp1 | PW_Basis_K | gatherp_scatters | 2908 | 0.015203 | 0.015203 | 0.015203 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_K | gathers_scatterp | 3388 | 0.041622 | 0.041622 | 0.041622 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.208553 | 0.208553 | 0.208553 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.293135 | 0.293135 | 0.293135 | 0.000000 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 2914 | 0.051935 | 0.024771 | 0.079098 | 0.054327 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_pack | 2914 | 0.007586 | 0.007374 | 0.007798 | 0.000424 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_scatters | 2914 | 0.073331 | 0.046477 | 0.100186 | 0.053709 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_unpack | 2914 | 0.011883 | 0.011874 | 0.011893 | 0.000019 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_alltoallv | 3394 | 0.023912 | 0.020454 | 0.027369 | 0.006915 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_clear | 3394 | 0.022762 | 0.022332 | 0.023192 | 0.000860 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_pack | 3394 | 0.008427 | 0.008268 | 0.008586 | 0.000318 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_scatterp | 3394 | 0.065524 | 0.062916 | 0.068132 | 0.005216 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_unpack | 3394 | 0.007862 | 0.007725 | 0.007999 | 0.000274 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.264062 | 0.143297 | 0.384827 | 0.241530 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.105448 | 0.102795 | 0.108101 | 0.005306 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.485642 | 0.349086 | 0.622197 | 0.273111 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.115786 | 0.097361 | 0.134211 | 0.036850 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.221300 | 0.132163 | 0.310437 | 0.178274 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.065380 | 0.061460 | 0.069300 | 0.007840 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.080362 | 0.079913 | 0.080810 | 0.000897 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.442116 | 0.360822 | 0.523411 | 0.162589 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.074690 | 0.071210 | 0.078171 | 0.006961 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 2910 | 0.109510 | 0.043149 | 0.164207 | 0.121058 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_pack | 2910 | 0.004403 | 0.004002 | 0.004838 | 0.000836 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_scatters | 2910 | 0.123320 | 0.057491 | 0.177212 | 0.119721 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_unpack | 2910 | 0.007450 | 0.006971 | 0.008178 | 0.001207 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_alltoallv | 3390 | 0.049767 | 0.042726 | 0.055275 | 0.012549 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_clear | 3390 | 0.018251 | 0.016992 | 0.019129 | 0.002137 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_pack | 3390 | 0.004873 | 0.004744 | 0.005121 | 0.000377 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_scatterp | 3390 | 0.080351 | 0.074917 | 0.084446 | 0.009529 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_unpack | 3390 | 0.004964 | 0.004632 | 0.005393 | 0.000761 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.177583 | 0.104291 | 0.246536 | 0.142245 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.055758 | 0.053184 | 0.057671 | 0.004487 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.310619 | 0.234863 | 0.383850 | 0.148987 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.076985 | 0.072607 | 0.083837 | 0.011230 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.213766 | 0.114589 | 0.257453 | 0.142864 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.047507 | 0.046774 | 0.049435 | 0.002661 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.042646 | 0.042148 | 0.042879 | 0.000731 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.353723 | 0.257749 | 0.396513 | 0.138764 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.049438 | 0.046375 | 0.051694 | 0.005319 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 2896 | 0.082382 | 0.054734 | 0.159178 | 0.104444 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_pack | 2896 | 0.002808 | 0.002421 | 0.003065 | 0.000644 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_scatters | 2896 | 0.090125 | 0.062509 | 0.166395 | 0.103886 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_unpack | 2896 | 0.003062 | 0.003000 | 0.003139 | 0.000139 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_alltoallv | 3376 | 0.055430 | 0.052444 | 0.061216 | 0.008772 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_clear | 3376 | 0.004357 | 0.003475 | 0.004922 | 0.001447 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_pack | 3376 | 0.003478 | 0.003391 | 0.003544 | 0.000153 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_scatterp | 3376 | 0.068882 | 0.066492 | 0.073715 | 0.007223 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_unpack | 3376 | 0.003117 | 0.002902 | 0.003293 | 0.000391 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.124784 | 0.117678 | 0.135376 | 0.017698 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.027751 | 0.026787 | 0.029020 | 0.002233 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.203378 | 0.196694 | 0.215293 | 0.018599 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.050607 | 0.049041 | 0.053925 | 0.004884 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.135660 | 0.092297 | 0.157199 | 0.064902 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.038922 | 0.037912 | 0.039902 | 0.001990 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.020662 | 0.019795 | 0.021433 | 0.001638 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.208915 | 0.165342 | 0.230893 | 0.065551 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.013406 | 0.011720 | 0.014631 | 0.002911 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 2899 | 0.097841 | 0.060317 | 0.133969 | 0.073652 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_pack | 2899 | 0.005694 | 0.005339 | 0.006289 | 0.000950 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_scatters | 2899 | 0.118932 | 0.082419 | 0.154380 | 0.071961 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_unpack | 2899 | 0.013580 | 0.013215 | 0.013967 | 0.000752 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_alltoallv | 3379 | 0.067961 | 0.063449 | 0.072568 | 0.009119 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_clear | 3379 | 0.013286 | 0.012435 | 0.014776 | 0.002341 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_pack | 3379 | 0.006484 | 0.006186 | 0.006907 | 0.000721 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_scatterp | 3379 | 0.096829 | 0.094860 | 0.099859 | 0.004999 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_unpack | 3379 | 0.006720 | 0.006263 | 0.007245 | 0.000982 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.142154 | 0.127336 | 0.153567 | 0.026231 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_pack | 149 | 0.031482 | 0.030387 | 0.032805 | 0.002418 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 149 | 0.217441 | 0.202706 | 0.229109 | 0.026403 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 149 | 0.043559 | 0.043027 | 0.044448 | 0.001421 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.246410 | 0.090049 | 0.313760 | 0.223711 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_clear | 112 | 0.036509 | 0.034629 | 0.038627 | 0.003998 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_pack | 112 | 0.022984 | 0.022349 | 0.023549 | 0.001200 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 112 | 0.323336 | 0.167981 | 0.391994 | 0.224013 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_unpack | 112 | 0.017195 | 0.016707 | 0.017598 | 0.000891 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 2898 | 0.069494 | 0.049716 | 0.123748 | 0.074032 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_pack | 2898 | 0.003716 | 0.003325 | 0.004043 | 0.000718 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_scatters | 2898 | 0.078680 | 0.059351 | 0.132395 | 0.073044 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_unpack | 2898 | 0.003680 | 0.003496 | 0.003844 | 0.000348 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_alltoallv | 3378 | 0.055343 | 0.052987 | 0.060021 | 0.007034 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_clear | 3378 | 0.004941 | 0.004096 | 0.005431 | 0.001335 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_pack | 3378 | 0.004139 | 0.003849 | 0.004320 | 0.000471 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_scatterp | 3378 | 0.070861 | 0.069331 | 0.074158 | 0.004827 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_unpack | 3378 | 0.004091 | 0.003689 | 0.004416 | 0.000727 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.091474 | 0.086119 | 0.094643 | 0.008524 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_pack | 149 | 0.013409 | 0.013013 | 0.013710 | 0.000697 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 149 | 0.147078 | 0.142605 | 0.149827 | 0.007222 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 149 | 0.041985 | 0.040961 | 0.043476 | 0.002515 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.114137 | 0.070736 | 0.145897 | 0.075161 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_clear | 112 | 0.019878 | 0.019440 | 0.020419 | 0.000979 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_pack | 112 | 0.010324 | 0.010010 | 0.010719 | 0.000709 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 112 | 0.152685 | 0.109268 | 0.183819 | 0.074551 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_unpack | 112 | 0.008160 | 0.007920 | 0.008520 | 0.000600 |
| large_rep03_np1_omp1 | PW_Basis_K | gatherp_scatters | 2908 | 0.015341 | 0.015341 | 0.015341 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_K | gathers_scatterp | 3388 | 0.041456 | 0.041456 | 0.041456 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.207493 | 0.207493 | 0.207493 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.292719 | 0.292719 | 0.292719 | 0.000000 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 2914 | 0.049924 | 0.049199 | 0.050650 | 0.001451 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_pack | 2914 | 0.007673 | 0.007639 | 0.007708 | 0.000069 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_scatters | 2914 | 0.071444 | 0.070790 | 0.072099 | 0.001309 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_unpack | 2914 | 0.011870 | 0.011788 | 0.011953 | 0.000165 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_alltoallv | 3394 | 0.023828 | 0.023568 | 0.024089 | 0.000521 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_clear | 3394 | 0.023219 | 0.023010 | 0.023429 | 0.000419 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_pack | 3394 | 0.008358 | 0.008223 | 0.008493 | 0.000270 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_scatterp | 3394 | 0.065758 | 0.065549 | 0.065967 | 0.000418 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_unpack | 3394 | 0.007832 | 0.007746 | 0.007918 | 0.000172 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.252495 | 0.149876 | 0.355114 | 0.205238 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.106432 | 0.105168 | 0.107696 | 0.002528 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.478956 | 0.371834 | 0.586078 | 0.214244 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.119678 | 0.113959 | 0.125397 | 0.011438 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.212905 | 0.150332 | 0.275479 | 0.125147 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.065203 | 0.063917 | 0.066490 | 0.002573 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.085024 | 0.084778 | 0.085270 | 0.000492 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.437302 | 0.373485 | 0.501119 | 0.127634 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.073784 | 0.073561 | 0.074007 | 0.000446 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 2910 | 0.108102 | 0.049076 | 0.161120 | 0.112044 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_pack | 2910 | 0.004374 | 0.004030 | 0.004797 | 0.000767 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_scatters | 2910 | 0.122604 | 0.064626 | 0.174412 | 0.109786 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_unpack | 2910 | 0.008200 | 0.007366 | 0.008921 | 0.001555 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_alltoallv | 3390 | 0.047169 | 0.040053 | 0.052815 | 0.012762 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_clear | 3390 | 0.017964 | 0.017031 | 0.018698 | 0.001667 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_pack | 3390 | 0.004976 | 0.004860 | 0.005187 | 0.000327 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_scatterp | 3390 | 0.077545 | 0.071891 | 0.081907 | 0.010016 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_unpack | 3390 | 0.004938 | 0.004631 | 0.005346 | 0.000715 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.158356 | 0.118667 | 0.219206 | 0.100539 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.059487 | 0.055958 | 0.061877 | 0.005919 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.294210 | 0.253045 | 0.357947 | 0.104902 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.076047 | 0.072173 | 0.082437 | 0.010264 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.214980 | 0.090264 | 0.268132 | 0.177868 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.047795 | 0.046762 | 0.048633 | 0.001871 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.042333 | 0.041946 | 0.042628 | 0.000682 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.353596 | 0.231003 | 0.406016 | 0.175013 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.048117 | 0.046371 | 0.050003 | 0.003632 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 2896 | 0.102205 | 0.062629 | 0.176486 | 0.113857 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_pack | 2896 | 0.002813 | 0.002448 | 0.003008 | 0.000560 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_scatters | 2896 | 0.109882 | 0.070501 | 0.183933 | 0.113432 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_unpack | 2896 | 0.003041 | 0.002985 | 0.003119 | 0.000134 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_alltoallv | 3376 | 0.056235 | 0.051681 | 0.062699 | 0.011018 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_clear | 3376 | 0.004351 | 0.003478 | 0.005086 | 0.001608 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_pack | 3376 | 0.003530 | 0.003374 | 0.003671 | 0.000297 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_scatterp | 3376 | 0.069647 | 0.066282 | 0.074965 | 0.008683 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_unpack | 3376 | 0.003095 | 0.002841 | 0.003306 | 0.000465 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.130089 | 0.110095 | 0.144838 | 0.034743 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_pack | 149 | 0.027577 | 0.026003 | 0.029145 | 0.003142 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 149 | 0.208677 | 0.187544 | 0.227618 | 0.040074 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 149 | 0.050780 | 0.049478 | 0.056527 | 0.007049 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.129121 | 0.080907 | 0.161623 | 0.080716 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_clear | 112 | 0.038780 | 0.038243 | 0.039848 | 0.001605 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_pack | 112 | 0.020908 | 0.020079 | 0.021433 | 0.001354 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 112 | 0.202313 | 0.153459 | 0.234515 | 0.081056 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_unpack | 112 | 0.013228 | 0.011993 | 0.014225 | 0.002232 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 2899 | 0.094127 | 0.058465 | 0.128987 | 0.070522 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_pack | 2899 | 0.005581 | 0.005289 | 0.005910 | 0.000621 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_scatters | 2899 | 0.114733 | 0.079556 | 0.149028 | 0.069472 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_unpack | 2899 | 0.013200 | 0.012883 | 0.013503 | 0.000620 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_alltoallv | 3379 | 0.065128 | 0.061878 | 0.067855 | 0.005977 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_clear | 3379 | 0.013077 | 0.012761 | 0.013566 | 0.000805 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_pack | 3379 | 0.006455 | 0.006338 | 0.006596 | 0.000258 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_scatterp | 3379 | 0.093754 | 0.091263 | 0.096329 | 0.005066 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_unpack | 3379 | 0.006698 | 0.006403 | 0.007044 | 0.000641 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.167969 | 0.149962 | 0.184734 | 0.034772 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_pack | 149 | 0.030701 | 0.029800 | 0.031577 | 0.001777 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 149 | 0.242440 | 0.224604 | 0.260269 | 0.035665 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 149 | 0.043543 | 0.042647 | 0.044242 | 0.001595 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.156533 | 0.124326 | 0.182215 | 0.057889 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_clear | 112 | 0.035731 | 0.034148 | 0.037564 | 0.003416 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_pack | 112 | 0.022406 | 0.021526 | 0.022910 | 0.001384 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 112 | 0.230648 | 0.197032 | 0.257831 | 0.060799 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_unpack | 112 | 0.015747 | 0.015409 | 0.016090 | 0.000681 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 2898 | 0.065682 | 0.046521 | 0.119527 | 0.073006 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_pack | 2898 | 0.003653 | 0.003292 | 0.003904 | 0.000612 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_scatters | 2898 | 0.074775 | 0.055847 | 0.128151 | 0.072304 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_unpack | 2898 | 0.003666 | 0.003588 | 0.003787 | 0.000199 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_alltoallv | 3378 | 0.053340 | 0.051422 | 0.057309 | 0.005887 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_clear | 3378 | 0.004807 | 0.003993 | 0.005119 | 0.001126 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_pack | 3378 | 0.004053 | 0.003942 | 0.004218 | 0.000276 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_scatterp | 3378 | 0.068510 | 0.067095 | 0.071675 | 0.004580 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_unpack | 3378 | 0.003945 | 0.003593 | 0.004171 | 0.000578 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 149 | 0.097187 | 0.094837 | 0.100240 | 0.005403 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_pack | 149 | 0.013224 | 0.012943 | 0.013394 | 0.000451 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 149 | 0.152900 | 0.150097 | 0.155334 | 0.005237 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 149 | 0.042316 | 0.041701 | 0.043336 | 0.001635 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 112 | 0.121113 | 0.079714 | 0.157596 | 0.077882 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_clear | 112 | 0.020221 | 0.019869 | 0.020598 | 0.000729 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_pack | 112 | 0.009978 | 0.009728 | 0.010247 | 0.000519 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 112 | 0.159601 | 0.117988 | 0.196173 | 0.078185 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_unpack | 112 | 0.008110 | 0.007804 | 0.008311 | 0.000507 |

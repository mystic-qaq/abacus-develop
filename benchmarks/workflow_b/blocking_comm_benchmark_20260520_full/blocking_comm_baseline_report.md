# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 3 | 23.607 +/- 0.067 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 3 | 23.607 +/- 0.067 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 2 | 1 | PW_Basis_K | 3 | 13.380 +/- 0.211 | 0.019109 +/- 0.003749 | 0.004924 +/- 0.005195 | 0.017293 +/- 0.000193 |
| large | 2 | 1 | PW_Basis_Sup | 3 | 13.380 +/- 0.211 | 0.185039 +/- 0.018954 | 0.078176 +/- 0.007544 | 0.198046 +/- 0.000654 |
| large | 4 | 1 | PW_Basis_K | 3 | 7.870 +/- 0.053 | 0.090792 +/- 0.003207 | 0.032091 +/- 0.001113 | 0.020227 +/- 0.000052 |
| large | 4 | 1 | PW_Basis_Sup | 3 | 7.870 +/- 0.053 | 0.168537 +/- 0.004188 | 0.037835 +/- 0.004397 | 0.102402 +/- 0.000869 |
| large | 4 | 2 | PW_Basis_K | 3 | 7.040 +/- 0.030 | 0.075829 +/- 0.005928 | 0.013547 +/- 0.001978 | 0.029303 +/- 0.001043 |
| large | 4 | 2 | PW_Basis_Sup | 3 | 7.040 +/- 0.030 | 0.170108 +/- 0.006930 | 0.034854 +/- 0.008450 | 0.095560 +/- 0.001405 |
| large | 8 | 1 | PW_Basis_K | 3 | 4.937 +/- 0.015 | 0.075757 +/- 0.000722 | 0.032354 +/- 0.000657 | 0.004821 +/- 0.000033 |
| large | 8 | 1 | PW_Basis_Sup | 3 | 4.937 +/- 0.015 | 0.107118 +/- 0.003210 | 0.028424 +/- 0.005135 | 0.054835 +/- 0.000274 |
| large | 8 | 2 | PW_Basis_K | 3 | 4.670 +/- 0.020 | 0.073515 +/- 0.005230 | 0.022601 +/- 0.005052 | 0.016764 +/- 0.000198 |
| large | 8 | 2 | PW_Basis_Sup | 3 | 4.670 +/- 0.020 | 0.124014 +/- 0.003546 | 0.031660 +/- 0.008125 | 0.052495 +/- 0.000517 |
| medium | 1 | 1 | PW_Basis_K | 3 | 8.180 +/- 0.010 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| medium | 1 | 1 | PW_Basis_Sup | 3 | 8.180 +/- 0.010 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| medium | 2 | 1 | PW_Basis_K | 3 | 5.113 +/- 0.038 | 0.018933 +/- 0.009117 | 0.008002 +/- 0.006507 | 0.010722 +/- 0.000169 |
| medium | 2 | 1 | PW_Basis_Sup | 3 | 5.113 +/- 0.038 | 0.055611 +/- 0.013591 | 0.021770 +/- 0.007955 | 0.054337 +/- 0.000429 |
| medium | 4 | 1 | PW_Basis_K | 3 | 3.487 +/- 0.015 | 0.027825 +/- 0.000245 | 0.005106 +/- 0.000103 | 0.005236 +/- 0.000031 |
| medium | 4 | 1 | PW_Basis_Sup | 3 | 3.487 +/- 0.015 | 0.047054 +/- 0.000786 | 0.007886 +/- 0.001636 | 0.032544 +/- 0.000066 |
| medium | 4 | 2 | PW_Basis_K | 3 | 3.517 +/- 0.032 | 0.043331 +/- 0.018396 | 0.016611 +/- 0.018077 | 0.019751 +/- 0.000378 |
| medium | 4 | 2 | PW_Basis_Sup | 3 | 3.517 +/- 0.032 | 0.070196 +/- 0.004976 | 0.025479 +/- 0.003669 | 0.039202 +/- 0.000620 |
| medium | 8 | 1 | PW_Basis_K | 3 | 2.837 +/- 0.035 | 0.041405 +/- 0.000333 | 0.003548 +/- 0.000159 | 0.003844 +/- 0.000009 |
| medium | 8 | 1 | PW_Basis_Sup | 3 | 2.837 +/- 0.035 | 0.039456 +/- 0.004945 | 0.009636 +/- 0.004168 | 0.021145 +/- 0.000066 |
| medium | 8 | 2 | PW_Basis_K | 3 | 2.893 +/- 0.040 | 0.063593 +/- 0.010999 | 0.018648 +/- 0.005013 | 0.015156 +/- 0.001129 |
| medium | 8 | 2 | PW_Basis_Sup | 3 | 2.893 +/- 0.040 | 0.065051 +/- 0.007800 | 0.021610 +/- 0.004454 | 0.022310 +/- 0.000278 |
| small | 1 | 1 | PW_Basis_K | 3 | 3.633 +/- 0.012 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| small | 1 | 1 | PW_Basis_Sup | 3 | 3.633 +/- 0.012 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| small | 2 | 1 | PW_Basis_K | 3 | 2.737 +/- 0.042 | 0.011141 +/- 0.002572 | 0.003133 +/- 0.002306 | 0.009092 +/- 0.000026 |
| small | 2 | 1 | PW_Basis_Sup | 3 | 2.737 +/- 0.042 | 0.022669 +/- 0.007546 | 0.017525 +/- 0.007606 | 0.012264 +/- 0.000145 |
| small | 4 | 1 | PW_Basis_K | 3 | 2.250 +/- 0.010 | 0.021181 +/- 0.000382 | 0.000881 +/- 0.000171 | 0.004614 +/- 0.000056 |
| small | 4 | 1 | PW_Basis_Sup | 3 | 2.250 +/- 0.010 | 0.020638 +/- 0.000646 | 0.007442 +/- 0.000157 | 0.011842 +/- 0.000084 |
| small | 4 | 2 | PW_Basis_K | 3 | 2.323 +/- 0.006 | 0.027518 +/- 0.003474 | 0.004609 +/- 0.002193 | 0.016896 +/- 0.000113 |
| small | 4 | 2 | PW_Basis_Sup | 3 | 2.323 +/- 0.006 | 0.026091 +/- 0.001409 | 0.007737 +/- 0.000595 | 0.013710 +/- 0.000081 |
| small | 8 | 1 | PW_Basis_K | 3 | 2.077 +/- 0.006 | 0.037908 +/- 0.000764 | 0.001007 +/- 0.000374 | 0.003479 +/- 0.000038 |
| small | 8 | 1 | PW_Basis_Sup | 3 | 2.077 +/- 0.006 | 0.021241 +/- 0.000729 | 0.006882 +/- 0.000207 | 0.007483 +/- 0.000062 |
| small | 8 | 2 | PW_Basis_K | 3 | 2.147 +/- 0.021 | 0.056202 +/- 0.016393 | 0.014813 +/- 0.016575 | 0.012858 +/- 0.000857 |
| small | 8 | 2 | PW_Basis_Sup | 3 | 2.147 +/- 0.021 | 0.033196 +/- 0.005378 | 0.014288 +/- 0.003503 | 0.007576 +/- 0.000174 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| nacl_large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 23.650 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 23.650 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_K | 13.160 | 0.014555 | 0.016241 | 0.003371 | 0.017084 |
| nacl_large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_Sup | 13.160 | 0.129889 | 0.166134 | 0.072489 | 0.197524 |
| nacl_large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_K | 7.850 | 0.073401 | 0.088550 | 0.031243 | 0.020232 |
| nacl_large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_Sup | 7.850 | 0.148062 | 0.172432 | 0.035161 | 0.102812 |
| nacl_large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_K | 7.040 | 0.063361 | 0.069736 | 0.013084 | 0.029957 |
| nacl_large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_Sup | 7.040 | 0.149460 | 0.165679 | 0.028844 | 0.097003 |
| nacl_large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_K | 4.940 | 0.052588 | 0.076185 | 0.032982 | 0.004854 |
| nacl_large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_Sup | 4.940 | 0.091319 | 0.103917 | 0.026310 | 0.055072 |
| nacl_large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 4.670 | 0.057126 | 0.067489 | 0.016847 | 0.016878 |
| nacl_large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 4.670 | 0.107985 | 0.121764 | 0.038084 | 0.053050 |
| nacl_large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_K | 23.530 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_Sup | 23.530 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_K | 13.580 | 0.017992 | 0.023351 | 0.010718 | 0.017464 |
| nacl_large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_Sup | 13.580 | 0.160674 | 0.204041 | 0.086734 | 0.197835 |
| nacl_large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_K | 7.930 | 0.078647 | 0.094466 | 0.031680 | 0.020172 |
| nacl_large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_Sup | 7.930 | 0.148605 | 0.164108 | 0.035434 | 0.101403 |
| nacl_large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_K | 7.010 | 0.068688 | 0.076176 | 0.011842 | 0.029852 |
| nacl_large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_Sup | 7.010 | 0.150250 | 0.166551 | 0.031202 | 0.095480 |
| nacl_large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_K | 4.920 | 0.053514 | 0.076163 | 0.031672 | 0.004822 |
| nacl_large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_Sup | 4.920 | 0.091967 | 0.107101 | 0.024683 | 0.054898 |
| nacl_large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_K | 4.690 | 0.060862 | 0.076174 | 0.024646 | 0.016879 |
| nacl_large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_Sup | 4.690 | 0.111083 | 0.128101 | 0.034369 | 0.052407 |
| nacl_large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_K | 23.640 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_Sup | 23.640 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_K | 13.400 | 0.017395 | 0.017736 | 0.000683 | 0.017331 |
| nacl_large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_Sup | 13.400 | 0.147291 | 0.184943 | 0.075304 | 0.198779 |
| nacl_large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_K | 7.830 | 0.072689 | 0.089360 | 0.033351 | 0.020277 |
| nacl_large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_Sup | 7.830 | 0.144043 | 0.169072 | 0.042910 | 0.102990 |
| nacl_large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_K | 7.070 | 0.073168 | 0.081576 | 0.015715 | 0.028101 |
| nacl_large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_Sup | 7.070 | 0.158889 | 0.178095 | 0.044516 | 0.094197 |
| nacl_large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_K | 4.950 | 0.051399 | 0.074924 | 0.032407 | 0.004788 |
| nacl_large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_Sup | 4.950 | 0.095729 | 0.110336 | 0.034279 | 0.054534 |
| nacl_large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_K | 4.650 | 0.060534 | 0.076881 | 0.026309 | 0.016536 |
| nacl_large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_Sup | 4.650 | 0.110225 | 0.122176 | 0.022526 | 0.052028 |
| nacl_medium_rep01_np1_omp1 | medium | 01 | 1 | 1 | PW_Basis_K | 8.180 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_medium_rep01_np1_omp1 | medium | 01 | 1 | 1 | PW_Basis_Sup | 8.180 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_medium_rep01_np2_omp1 | medium | 01 | 2 | 1 | PW_Basis_K | 5.130 | 0.021697 | 0.029449 | 0.015505 | 0.010916 |
| nacl_medium_rep01_np2_omp1 | medium | 01 | 2 | 1 | PW_Basis_Sup | 5.130 | 0.044448 | 0.054816 | 0.020737 | 0.054562 |
| nacl_medium_rep01_np4_omp1 | medium | 01 | 4 | 1 | PW_Basis_K | 3.470 | 0.026346 | 0.027725 | 0.005007 | 0.005272 |
| nacl_medium_rep01_np4_omp1 | medium | 01 | 4 | 1 | PW_Basis_Sup | 3.470 | 0.044021 | 0.047934 | 0.009757 | 0.032479 |
| nacl_medium_rep01_np4_omp2 | medium | 01 | 4 | 2 | PW_Basis_K | 3.530 | 0.029455 | 0.031600 | 0.005029 | 0.019832 |
| nacl_medium_rep01_np4_omp2 | medium | 01 | 4 | 2 | PW_Basis_Sup | 3.530 | 0.060330 | 0.072908 | 0.023877 | 0.039078 |
| nacl_medium_rep01_np8_omp1 | medium | 01 | 8 | 1 | PW_Basis_K | 2.840 | 0.040503 | 0.041248 | 0.003388 | 0.003842 |
| nacl_medium_rep01_np8_omp1 | medium | 01 | 8 | 1 | PW_Basis_Sup | 2.840 | 0.033823 | 0.037039 | 0.007441 | 0.021201 |
| nacl_medium_rep01_np8_omp2 | medium | 01 | 8 | 2 | PW_Basis_K | 2.930 | 0.049611 | 0.056137 | 0.014750 | 0.014313 |
| nacl_medium_rep01_np8_omp2 | medium | 01 | 8 | 2 | PW_Basis_Sup | 2.930 | 0.048538 | 0.056637 | 0.016467 | 0.021996 |
| nacl_medium_rep02_np1_omp1 | medium | 02 | 1 | 1 | PW_Basis_K | 8.190 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_medium_rep02_np1_omp1 | medium | 02 | 1 | 1 | PW_Basis_Sup | 8.190 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_medium_rep02_np2_omp1 | medium | 02 | 2 | 1 | PW_Basis_K | 5.070 | 0.011296 | 0.013250 | 0.003908 | 0.010616 |
| nacl_medium_rep02_np2_omp1 | medium | 02 | 2 | 1 | PW_Basis_Sup | 5.070 | 0.035244 | 0.042435 | 0.014381 | 0.053842 |
| nacl_medium_rep02_np4_omp1 | medium | 02 | 4 | 1 | PW_Basis_K | 3.500 | 0.026617 | 0.028104 | 0.005213 | 0.005217 |
| nacl_medium_rep02_np4_omp1 | medium | 02 | 4 | 1 | PW_Basis_Sup | 3.500 | 0.043883 | 0.046804 | 0.007181 | 0.032543 |
| nacl_medium_rep02_np4_omp2 | medium | 02 | 4 | 2 | PW_Basis_K | 3.480 | 0.030363 | 0.033861 | 0.007363 | 0.019339 |
| nacl_medium_rep02_np4_omp2 | medium | 02 | 4 | 2 | PW_Basis_Sup | 3.480 | 0.060980 | 0.073226 | 0.029677 | 0.038653 |
| nacl_medium_rep02_np8_omp1 | medium | 02 | 8 | 1 | PW_Basis_K | 2.800 | 0.040843 | 0.041787 | 0.003705 | 0.003837 |
| nacl_medium_rep02_np8_omp1 | medium | 02 | 8 | 1 | PW_Basis_Sup | 2.800 | 0.032836 | 0.036184 | 0.007024 | 0.021072 |
| nacl_medium_rep02_np8_omp2 | medium | 02 | 8 | 2 | PW_Basis_K | 2.900 | 0.068956 | 0.076226 | 0.024303 | 0.016439 |
| nacl_medium_rep02_np8_omp2 | medium | 02 | 8 | 2 | PW_Basis_Sup | 2.900 | 0.055795 | 0.066475 | 0.024108 | 0.022408 |
| nacl_medium_rep03_np1_omp1 | medium | 03 | 1 | 1 | PW_Basis_K | 8.170 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_medium_rep03_np1_omp1 | medium | 03 | 1 | 1 | PW_Basis_Sup | 8.170 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_medium_rep03_np2_omp1 | medium | 03 | 2 | 1 | PW_Basis_K | 5.140 | 0.011803 | 0.014100 | 0.004594 | 0.010632 |
| nacl_medium_rep03_np2_omp1 | medium | 03 | 2 | 1 | PW_Basis_Sup | 5.140 | 0.054487 | 0.069583 | 0.030191 | 0.054606 |
| nacl_medium_rep03_np4_omp1 | medium | 03 | 4 | 1 | PW_Basis_K | 3.490 | 0.026256 | 0.027645 | 0.005098 | 0.005218 |
| nacl_medium_rep03_np4_omp1 | medium | 03 | 4 | 1 | PW_Basis_Sup | 3.490 | 0.042857 | 0.046423 | 0.006721 | 0.032610 |
| nacl_medium_rep03_np4_omp2 | medium | 03 | 4 | 2 | PW_Basis_K | 3.540 | 0.048817 | 0.064533 | 0.037441 | 0.020083 |
| nacl_medium_rep03_np4_omp2 | medium | 03 | 4 | 2 | PW_Basis_Sup | 3.540 | 0.057292 | 0.064453 | 0.022884 | 0.039874 |
| nacl_medium_rep03_np8_omp1 | medium | 03 | 8 | 1 | PW_Basis_K | 2.870 | 0.040328 | 0.041179 | 0.003552 | 0.003854 |
| nacl_medium_rep03_np8_omp1 | medium | 03 | 8 | 1 | PW_Basis_Sup | 2.870 | 0.041086 | 0.045144 | 0.014442 | 0.021163 |
| nacl_medium_rep03_np8_omp2 | medium | 03 | 8 | 2 | PW_Basis_K | 2.850 | 0.054693 | 0.058417 | 0.016892 | 0.014715 |
| nacl_medium_rep03_np8_omp2 | medium | 03 | 8 | 2 | PW_Basis_Sup | 2.850 | 0.058364 | 0.072040 | 0.024254 | 0.022527 |
| nacl_small_rep01_np1_omp1 | small | 01 | 1 | 1 | PW_Basis_K | 3.640 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_small_rep01_np1_omp1 | small | 01 | 1 | 1 | PW_Basis_Sup | 3.640 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_small_rep01_np2_omp1 | small | 01 | 2 | 1 | PW_Basis_K | 2.770 | 0.010548 | 0.012464 | 0.003831 | 0.009062 |
| nacl_small_rep01_np2_omp1 | small | 01 | 2 | 1 | PW_Basis_Sup | 2.770 | 0.016039 | 0.027113 | 0.022149 | 0.012407 |
| nacl_small_rep01_np4_omp1 | small | 01 | 4 | 1 | PW_Basis_K | 2.260 | 0.020713 | 0.021113 | 0.000923 | 0.004559 |
| nacl_small_rep01_np4_omp1 | small | 01 | 4 | 1 | PW_Basis_Sup | 2.260 | 0.017718 | 0.020080 | 0.007458 | 0.011880 |
| nacl_small_rep01_np4_omp2 | small | 01 | 4 | 2 | PW_Basis_K | 2.320 | 0.029178 | 0.031502 | 0.006973 | 0.016937 |
| nacl_small_rep01_np4_omp2 | small | 01 | 4 | 2 | PW_Basis_Sup | 2.320 | 0.023020 | 0.027298 | 0.008390 | 0.013761 |
| nacl_small_rep01_np8_omp1 | small | 01 | 8 | 1 | PW_Basis_K | 2.070 | 0.038188 | 0.038759 | 0.001174 | 0.003471 |
| nacl_small_rep01_np8_omp1 | small | 01 | 8 | 1 | PW_Basis_Sup | 2.070 | 0.017857 | 0.022027 | 0.006877 | 0.007438 |
| nacl_small_rep01_np8_omp2 | small | 01 | 8 | 2 | PW_Basis_K | 2.170 | 0.045613 | 0.047792 | 0.003961 | 0.013012 |
| nacl_small_rep01_np8_omp2 | small | 01 | 8 | 2 | PW_Basis_Sup | 2.170 | 0.023555 | 0.031634 | 0.013194 | 0.007535 |
| nacl_small_rep02_np1_omp1 | small | 02 | 1 | 1 | PW_Basis_K | 3.620 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_small_rep02_np1_omp1 | small | 02 | 1 | 1 | PW_Basis_Sup | 3.620 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_small_rep02_np2_omp1 | small | 02 | 2 | 1 | PW_Basis_K | 2.690 | 0.007897 | 0.008176 | 0.000559 | 0.009108 |
| nacl_small_rep02_np2_omp1 | small | 02 | 2 | 1 | PW_Basis_Sup | 2.690 | 0.009584 | 0.013957 | 0.008746 | 0.012117 |
| nacl_small_rep02_np4_omp1 | small | 02 | 4 | 1 | PW_Basis_K | 2.250 | 0.020487 | 0.020838 | 0.000693 | 0.004611 |
| nacl_small_rep02_np4_omp1 | small | 02 | 4 | 1 | PW_Basis_Sup | 2.250 | 0.018347 | 0.021346 | 0.007278 | 0.011900 |
| nacl_small_rep02_np4_omp2 | small | 02 | 4 | 2 | PW_Basis_K | 2.330 | 0.023731 | 0.025122 | 0.002642 | 0.016768 |
| nacl_small_rep02_np4_omp2 | small | 02 | 4 | 2 | PW_Basis_Sup | 2.330 | 0.023847 | 0.026431 | 0.007226 | 0.013752 |
| nacl_small_rep02_np8_omp1 | small | 02 | 8 | 1 | PW_Basis_K | 2.080 | 0.036980 | 0.037283 | 0.000579 | 0.003446 |
| nacl_small_rep02_np8_omp1 | small | 02 | 8 | 1 | PW_Basis_Sup | 2.080 | 0.017067 | 0.021110 | 0.007091 | 0.007554 |
| nacl_small_rep02_np8_omp2 | small | 02 | 8 | 2 | PW_Basis_K | 2.130 | 0.047500 | 0.075093 | 0.033892 | 0.011935 |
| nacl_small_rep02_np8_omp2 | small | 02 | 8 | 2 | PW_Basis_Sup | 2.130 | 0.027662 | 0.039182 | 0.018207 | 0.007425 |
| nacl_small_rep03_np1_omp1 | small | 03 | 1 | 1 | PW_Basis_K | 3.640 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_small_rep03_np1_omp1 | small | 03 | 1 | 1 | PW_Basis_Sup | 3.640 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| nacl_small_rep03_np2_omp1 | small | 03 | 2 | 1 | PW_Basis_K | 2.750 | 0.010277 | 0.012782 | 0.005010 | 0.009105 |
| nacl_small_rep03_np2_omp1 | small | 03 | 2 | 1 | PW_Basis_Sup | 2.750 | 0.016098 | 0.026938 | 0.021680 | 0.012268 |
| nacl_small_rep03_np4_omp1 | small | 03 | 4 | 1 | PW_Basis_K | 2.240 | 0.021039 | 0.021593 | 0.001027 | 0.004672 |
| nacl_small_rep03_np4_omp1 | small | 03 | 4 | 1 | PW_Basis_Sup | 2.240 | 0.017509 | 0.020488 | 0.007591 | 0.011745 |
| nacl_small_rep03_np4_omp2 | small | 03 | 4 | 2 | PW_Basis_K | 2.320 | 0.023969 | 0.025929 | 0.004212 | 0.016983 |
| nacl_small_rep03_np4_omp2 | small | 03 | 4 | 2 | PW_Basis_Sup | 2.320 | 0.021016 | 0.024543 | 0.007595 | 0.013616 |
| nacl_small_rep03_np8_omp1 | small | 03 | 8 | 1 | PW_Basis_K | 2.080 | 0.036944 | 0.037681 | 0.001268 | 0.003521 |
| nacl_small_rep03_np8_omp1 | small | 03 | 8 | 1 | PW_Basis_Sup | 2.080 | 0.016862 | 0.020587 | 0.006677 | 0.007458 |
| nacl_small_rep03_np8_omp2 | small | 03 | 8 | 2 | PW_Basis_K | 2.140 | 0.042737 | 0.045722 | 0.006585 | 0.013628 |
| nacl_small_rep03_np8_omp2 | small | 03 | 8 | 2 | PW_Basis_Sup | 2.140 | 0.022670 | 0.028771 | 0.011462 | 0.007767 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| nacl_small_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.002642 | 0.002642 | 0.002642 | 0.000000 |
| nacl_small_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.006140 | 0.006140 | 0.006140 | 0.000000 |
| nacl_small_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.004147 | 0.004147 | 0.004147 | 0.000000 |
| nacl_small_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.004907 | 0.004907 | 0.004907 | 0.000000 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.005815 | 0.004324 | 0.007307 | 0.002983 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.001313 | 0.001286 | 0.001341 | 0.000055 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.009735 | 0.008257 | 0.011213 | 0.002956 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.002084 | 0.002062 | 0.002106 | 0.000044 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.004733 | 0.004309 | 0.005157 | 0.000848 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.002842 | 0.002716 | 0.002968 | 0.000252 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.001409 | 0.001377 | 0.001440 | 0.000063 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.011075 | 0.010831 | 0.011319 | 0.000488 |
| nacl_small_rep01_np2_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.001414 | 0.001400 | 0.001427 | 0.000027 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.007506 | 0.002581 | 0.012431 | 0.009850 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.002412 | 0.002285 | 0.002539 | 0.000254 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.014482 | 0.009827 | 0.019137 | 0.009310 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.004509 | 0.004370 | 0.004649 | 0.000279 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.008533 | 0.002383 | 0.014682 | 0.012299 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.001772 | 0.001727 | 0.001816 | 0.000089 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.002039 | 0.002023 | 0.002055 | 0.000032 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.014068 | 0.008051 | 0.020085 | 0.012034 |
| nacl_small_rep01_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.001675 | 0.001607 | 0.001743 | 0.000136 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.009773 | 0.009283 | 0.010144 | 0.000861 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.000786 | 0.000775 | 0.000806 | 0.000031 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.011873 | 0.011366 | 0.012209 | 0.000843 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.000817 | 0.000796 | 0.000866 | 0.000070 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.010939 | 0.010907 | 0.010969 | 0.000062 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.001101 | 0.001051 | 0.001165 | 0.000114 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.000919 | 0.000898 | 0.000936 | 0.000038 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.014565 | 0.014468 | 0.014710 | 0.000242 |
| nacl_small_rep01_np4_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.000936 | 0.000914 | 0.000978 | 0.000064 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.010813 | 0.007315 | 0.012330 | 0.005015 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.001012 | 0.000933 | 0.001204 | 0.000271 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.018943 | 0.015533 | 0.020347 | 0.004814 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.007074 | 0.006967 | 0.007209 | 0.000242 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.006905 | 0.005307 | 0.007750 | 0.002443 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.002371 | 0.002300 | 0.002413 | 0.000113 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.000812 | 0.000768 | 0.000906 | 0.000138 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.010738 | 0.009318 | 0.011553 | 0.002235 |
| nacl_small_rep01_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.000611 | 0.000596 | 0.000651 | 0.000055 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.018278 | 0.017831 | 0.018661 | 0.000830 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.000574 | 0.000560 | 0.000588 | 0.000028 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.020045 | 0.019618 | 0.020417 | 0.000799 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.000694 | 0.000675 | 0.000711 | 0.000036 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.019910 | 0.019754 | 0.020098 | 0.000344 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.000772 | 0.000757 | 0.000793 | 0.000036 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.000742 | 0.000724 | 0.000756 | 0.000032 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.022784 | 0.022666 | 0.022899 | 0.000233 |
| nacl_small_rep01_np8_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.000690 | 0.000670 | 0.000726 | 0.000056 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.010031 | 0.008009 | 0.013742 | 0.005733 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.000509 | 0.000414 | 0.000579 | 0.000165 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.015308 | 0.013503 | 0.018619 | 0.005116 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.004726 | 0.004395 | 0.005093 | 0.000698 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.007827 | 0.007141 | 0.008285 | 0.001144 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.001473 | 0.001441 | 0.001523 | 0.000082 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.000399 | 0.000377 | 0.000434 | 0.000057 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.010068 | 0.009418 | 0.010457 | 0.001039 |
| nacl_small_rep01_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.000331 | 0.000283 | 0.000384 | 0.000101 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 858 | 0.014720 | 0.011607 | 0.016509 | 0.004902 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gatherp_pack | 858 | 0.002891 | 0.002733 | 0.003184 | 0.000451 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gatherp_scatters | 858 | 0.021684 | 0.019297 | 0.023373 | 0.004076 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gatherp_unpack | 858 | 0.003450 | 0.003159 | 0.003779 | 0.000620 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1002 | 0.014458 | 0.012922 | 0.014993 | 0.002071 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gathers_clear | 1002 | 0.004301 | 0.004093 | 0.004619 | 0.000526 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gathers_pack | 1002 | 0.002984 | 0.002718 | 0.003332 | 0.000614 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gathers_scatterp | 1002 | 0.026000 | 0.025454 | 0.026301 | 0.000847 |
| nacl_small_rep01_np4_omp2 | PW_Basis_K | gathers_unpack | 1002 | 0.003312 | 0.003061 | 0.003608 | 0.000547 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.011841 | 0.010100 | 0.013125 | 0.003025 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.002478 | 0.002388 | 0.002656 | 0.000268 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.019631 | 0.018232 | 0.020960 | 0.002728 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.005263 | 0.005073 | 0.005425 | 0.000352 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.011179 | 0.008808 | 0.014173 | 0.005365 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.001918 | 0.001705 | 0.002169 | 0.000464 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.002047 | 0.001950 | 0.002191 | 0.000241 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.017247 | 0.014669 | 0.020132 | 0.005463 |
| nacl_small_rep01_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.002054 | 0.001985 | 0.002187 | 0.000202 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 858 | 0.022656 | 0.021715 | 0.023616 | 0.001901 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 858 | 0.002298 | 0.002194 | 0.002456 | 0.000262 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 858 | 0.027997 | 0.027230 | 0.028786 | 0.001556 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 858 | 0.002340 | 0.002052 | 0.002449 | 0.000397 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1002 | 0.022956 | 0.022116 | 0.024176 | 0.002060 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 1002 | 0.003126 | 0.002997 | 0.003311 | 0.000314 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 1002 | 0.002530 | 0.002285 | 0.002677 | 0.000392 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 1002 | 0.032210 | 0.031637 | 0.033042 | 0.001405 |
| nacl_small_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 1002 | 0.002718 | 0.002598 | 0.002839 | 0.000241 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.010779 | 0.008690 | 0.013497 | 0.004807 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.001475 | 0.001303 | 0.001578 | 0.000275 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.015071 | 0.013239 | 0.017629 | 0.004390 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.002765 | 0.002663 | 0.002966 | 0.000303 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.012775 | 0.009750 | 0.018137 | 0.008387 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.001249 | 0.001161 | 0.001304 | 0.000143 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.000956 | 0.000919 | 0.001024 | 0.000105 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.016116 | 0.013135 | 0.021509 | 0.008374 |
| nacl_small_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.001091 | 0.000945 | 0.001204 | 0.000259 |
| nacl_small_rep02_np1_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.002675 | 0.002675 | 0.002675 | 0.000000 |
| nacl_small_rep02_np1_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.006227 | 0.006227 | 0.006227 | 0.000000 |
| nacl_small_rep02_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.004232 | 0.004232 | 0.004232 | 0.000000 |
| nacl_small_rep02_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.005114 | 0.005114 | 0.005114 | 0.000000 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.003585 | 0.003472 | 0.003697 | 0.000225 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.001307 | 0.001289 | 0.001325 | 0.000036 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.007449 | 0.007362 | 0.007536 | 0.000174 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.002040 | 0.002039 | 0.002040 | 0.000001 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.004312 | 0.004145 | 0.004479 | 0.000334 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.002903 | 0.002885 | 0.002922 | 0.000037 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.001469 | 0.001384 | 0.001554 | 0.000170 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.010744 | 0.010685 | 0.010803 | 0.000118 |
| nacl_small_rep02_np2_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.001388 | 0.001386 | 0.001391 | 0.000005 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.005422 | 0.002710 | 0.008133 | 0.005423 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.002418 | 0.002348 | 0.002488 | 0.000140 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.012195 | 0.009551 | 0.014838 | 0.005287 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.004300 | 0.004298 | 0.004302 | 0.000004 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.004163 | 0.002501 | 0.005824 | 0.003323 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.001730 | 0.001721 | 0.001738 | 0.000017 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.002034 | 0.002023 | 0.002045 | 0.000022 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.009612 | 0.007991 | 0.011233 | 0.003242 |
| nacl_small_rep02_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.001636 | 0.001596 | 0.001676 | 0.000080 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.009722 | 0.009507 | 0.009959 | 0.000452 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.000802 | 0.000791 | 0.000812 | 0.000021 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.011836 | 0.011636 | 0.012035 | 0.000399 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.000813 | 0.000792 | 0.000828 | 0.000036 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.010765 | 0.010638 | 0.010879 | 0.000241 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.001099 | 0.001082 | 0.001110 | 0.000028 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.000951 | 0.000914 | 0.001001 | 0.000087 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.014432 | 0.014311 | 0.014535 | 0.000224 |
| nacl_small_rep02_np4_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.000946 | 0.000924 | 0.000978 | 0.000054 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.011080 | 0.007411 | 0.013081 | 0.005670 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.001014 | 0.000907 | 0.001252 | 0.000345 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.019164 | 0.015738 | 0.021032 | 0.005294 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.007025 | 0.006973 | 0.007098 | 0.000125 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.007267 | 0.006657 | 0.008265 | 0.001608 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.002457 | 0.002325 | 0.002626 | 0.000301 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.000791 | 0.000781 | 0.000802 | 0.000021 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.011169 | 0.010771 | 0.012026 | 0.001255 |
| nacl_small_rep02_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.000613 | 0.000588 | 0.000645 | 0.000057 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.016997 | 0.016871 | 0.017175 | 0.000304 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.000572 | 0.000564 | 0.000584 | 0.000020 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.018757 | 0.018650 | 0.018902 | 0.000252 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.000687 | 0.000659 | 0.000709 | 0.000050 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.019983 | 0.019833 | 0.020108 | 0.000275 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.000767 | 0.000758 | 0.000779 | 0.000021 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.000730 | 0.000711 | 0.000749 | 0.000038 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.022834 | 0.022678 | 0.022958 | 0.000280 |
| nacl_small_rep02_np8_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.000690 | 0.000672 | 0.000711 | 0.000039 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.009803 | 0.007253 | 0.013275 | 0.006022 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.000511 | 0.000410 | 0.000588 | 0.000178 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.015117 | 0.012724 | 0.018238 | 0.005514 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.004760 | 0.004505 | 0.005060 | 0.000555 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.007264 | 0.006766 | 0.007835 | 0.001069 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.001546 | 0.001501 | 0.001592 | 0.000091 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.000404 | 0.000366 | 0.000450 | 0.000084 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.009584 | 0.009144 | 0.010137 | 0.000993 |
| nacl_small_rep02_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.000332 | 0.000301 | 0.000360 | 0.000059 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 858 | 0.011791 | 0.010977 | 0.012720 | 0.001743 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gatherp_pack | 858 | 0.003027 | 0.002753 | 0.003555 | 0.000802 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gatherp_scatters | 858 | 0.018855 | 0.017773 | 0.020192 | 0.002419 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gatherp_unpack | 858 | 0.003341 | 0.003245 | 0.003541 | 0.000296 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1002 | 0.011940 | 0.011503 | 0.012402 | 0.000899 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gathers_clear | 1002 | 0.004298 | 0.004107 | 0.004505 | 0.000398 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gathers_pack | 1002 | 0.002818 | 0.002730 | 0.002870 | 0.000140 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gathers_scatterp | 1002 | 0.023276 | 0.023166 | 0.023490 | 0.000324 |
| nacl_small_rep02_np4_omp2 | PW_Basis_K | gathers_unpack | 1002 | 0.003283 | 0.003009 | 0.003686 | 0.000677 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.013532 | 0.009952 | 0.015203 | 0.005251 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.002523 | 0.002353 | 0.002909 | 0.000556 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.021222 | 0.017764 | 0.022991 | 0.005227 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.005118 | 0.004849 | 0.005389 | 0.000540 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.010315 | 0.009253 | 0.011228 | 0.001975 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.002023 | 0.001881 | 0.002272 | 0.000391 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.002005 | 0.001939 | 0.002095 | 0.000156 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.016472 | 0.015798 | 0.017447 | 0.001649 |
| nacl_small_rep02_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.002084 | 0.001961 | 0.002374 | 0.000413 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 858 | 0.024822 | 0.020545 | 0.043961 | 0.023416 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gatherp_pack | 858 | 0.002145 | 0.000752 | 0.002484 | 0.001732 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gatherp_scatters | 858 | 0.029778 | 0.026018 | 0.045980 | 0.019962 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gatherp_unpack | 858 | 0.002148 | 0.000772 | 0.002463 | 0.001691 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1002 | 0.022678 | 0.020656 | 0.031132 | 0.010476 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gathers_clear | 1002 | 0.002742 | 0.000933 | 0.003340 | 0.002407 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gathers_pack | 1002 | 0.002357 | 0.000893 | 0.002753 | 0.001860 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gathers_scatterp | 1002 | 0.031199 | 0.030294 | 0.034450 | 0.004156 |
| nacl_small_rep02_np8_omp2 | PW_Basis_K | gathers_unpack | 1002 | 0.002543 | 0.000847 | 0.003171 | 0.002324 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.011882 | 0.009518 | 0.018026 | 0.008508 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.001396 | 0.000529 | 0.001580 | 0.001051 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.016147 | 0.014010 | 0.021214 | 0.007204 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.002818 | 0.002618 | 0.002954 | 0.000336 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.015779 | 0.011457 | 0.021156 | 0.009699 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.001334 | 0.001287 | 0.001397 | 0.000110 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.000882 | 0.000331 | 0.001020 | 0.000689 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.019037 | 0.014939 | 0.023161 | 0.008222 |
| nacl_small_rep02_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.000996 | 0.000305 | 0.001165 | 0.000860 |
| nacl_small_rep03_np1_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.002599 | 0.002599 | 0.002599 | 0.000000 |
| nacl_small_rep03_np1_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.006044 | 0.006044 | 0.006044 | 0.000000 |
| nacl_small_rep03_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.004265 | 0.004265 | 0.004265 | 0.000000 |
| nacl_small_rep03_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.005106 | 0.005106 | 0.005106 | 0.000000 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.005649 | 0.003565 | 0.007733 | 0.004168 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.001299 | 0.001271 | 0.001328 | 0.000057 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.009619 | 0.007531 | 0.011706 | 0.004175 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.002157 | 0.002098 | 0.002217 | 0.000119 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.004628 | 0.004207 | 0.005049 | 0.000842 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.002819 | 0.002763 | 0.002876 | 0.000113 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.001421 | 0.001367 | 0.001475 | 0.000108 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.010956 | 0.010692 | 0.011220 | 0.000528 |
| nacl_small_rep03_np2_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.001408 | 0.001388 | 0.001428 | 0.000040 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.007659 | 0.002709 | 0.012609 | 0.009900 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.002414 | 0.002332 | 0.002497 | 0.000165 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.014555 | 0.009728 | 0.019382 | 0.009654 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.004424 | 0.004386 | 0.004462 | 0.000076 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.008439 | 0.002549 | 0.014329 | 0.011780 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.001741 | 0.001703 | 0.001779 | 0.000076 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.002038 | 0.002009 | 0.002066 | 0.000057 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.013919 | 0.008153 | 0.019684 | 0.011531 |
| nacl_small_rep03_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.001651 | 0.001597 | 0.001705 | 0.000108 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.010046 | 0.009850 | 0.010387 | 0.000537 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.000807 | 0.000783 | 0.000844 | 0.000061 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.012188 | 0.012055 | 0.012487 | 0.000432 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.000832 | 0.000811 | 0.000869 | 0.000058 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.010993 | 0.010716 | 0.011206 | 0.000490 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.001114 | 0.001084 | 0.001173 | 0.000089 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.000940 | 0.000905 | 0.000980 | 0.000075 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.014699 | 0.014540 | 0.014869 | 0.000329 |
| nacl_small_rep03_np4_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.000980 | 0.000916 | 0.001030 | 0.000114 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.010711 | 0.006580 | 0.012674 | 0.006094 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.001009 | 0.000915 | 0.001221 | 0.000306 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.018747 | 0.014910 | 0.020456 | 0.005546 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.006982 | 0.006797 | 0.007064 | 0.000267 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.006798 | 0.006317 | 0.007814 | 0.001497 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.002340 | 0.002316 | 0.002354 | 0.000038 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.000804 | 0.000782 | 0.000837 | 0.000055 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.010592 | 0.010181 | 0.011559 | 0.001378 |
| nacl_small_rep03_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.000610 | 0.000590 | 0.000647 | 0.000057 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 858 | 0.017788 | 0.017497 | 0.018300 | 0.000803 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gatherp_pack | 858 | 0.000631 | 0.000566 | 0.000969 | 0.000403 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gatherp_scatters | 858 | 0.019608 | 0.019279 | 0.020025 | 0.000746 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gatherp_unpack | 858 | 0.000687 | 0.000669 | 0.000708 | 0.000039 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1002 | 0.019156 | 0.018916 | 0.019381 | 0.000465 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gathers_clear | 1002 | 0.000762 | 0.000746 | 0.000776 | 0.000030 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gathers_pack | 1002 | 0.000744 | 0.000718 | 0.000769 | 0.000051 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gathers_scatterp | 1002 | 0.022025 | 0.021793 | 0.022222 | 0.000429 |
| nacl_small_rep03_np8_omp1 | PW_Basis_K | gathers_unpack | 1002 | 0.000697 | 0.000667 | 0.000743 | 0.000076 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.009723 | 0.007231 | 0.013193 | 0.005962 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.000509 | 0.000410 | 0.000582 | 0.000172 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.014936 | 0.012797 | 0.017998 | 0.005201 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.004662 | 0.004352 | 0.005005 | 0.000653 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.007139 | 0.006679 | 0.007394 | 0.000715 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.001565 | 0.001472 | 0.001633 | 0.000161 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.000392 | 0.000373 | 0.000432 | 0.000059 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.009465 | 0.009133 | 0.009672 | 0.000539 |
| nacl_small_rep03_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.000329 | 0.000292 | 0.000357 | 0.000065 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 858 | 0.011116 | 0.010188 | 0.011969 | 0.001781 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gatherp_pack | 858 | 0.002874 | 0.002762 | 0.002979 | 0.000217 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gatherp_scatters | 858 | 0.017997 | 0.017083 | 0.018813 | 0.001730 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gatherp_unpack | 858 | 0.003331 | 0.003184 | 0.003415 | 0.000231 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1002 | 0.012853 | 0.011529 | 0.013960 | 0.002431 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gathers_clear | 1002 | 0.004177 | 0.004107 | 0.004250 | 0.000143 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gathers_pack | 1002 | 0.003082 | 0.002817 | 0.003559 | 0.000742 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gathers_scatterp | 1002 | 0.024564 | 0.024113 | 0.025142 | 0.001029 |
| nacl_small_rep03_np4_omp2 | PW_Basis_K | gathers_unpack | 1002 | 0.003521 | 0.003326 | 0.003872 | 0.000546 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.009546 | 0.009186 | 0.009810 | 0.000624 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.002466 | 0.002378 | 0.002680 | 0.000302 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.017163 | 0.016677 | 0.017400 | 0.000723 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.005100 | 0.005017 | 0.005154 | 0.000137 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.011470 | 0.007762 | 0.014733 | 0.006971 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.001935 | 0.001831 | 0.002142 | 0.000311 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.002044 | 0.001985 | 0.002208 | 0.000223 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.017568 | 0.013849 | 0.020632 | 0.006783 |
| nacl_small_rep03_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.002070 | 0.001988 | 0.002265 | 0.000277 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 858 | 0.021035 | 0.018466 | 0.023032 | 0.004566 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gatherp_pack | 858 | 0.002397 | 0.002205 | 0.002661 | 0.000456 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gatherp_scatters | 858 | 0.026558 | 0.024356 | 0.028316 | 0.003960 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gatherp_unpack | 858 | 0.002465 | 0.002273 | 0.002619 | 0.000346 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1002 | 0.021702 | 0.020671 | 0.022690 | 0.002019 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gathers_clear | 1002 | 0.003221 | 0.002952 | 0.003457 | 0.000505 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gathers_pack | 1002 | 0.002711 | 0.002475 | 0.003014 | 0.000539 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gathers_scatterp | 1002 | 0.031401 | 0.030672 | 0.031926 | 0.001254 |
| nacl_small_rep03_np8_omp2 | PW_Basis_K | gathers_unpack | 1002 | 0.002833 | 0.002628 | 0.003041 | 0.000413 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.011219 | 0.008996 | 0.013745 | 0.004749 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.001520 | 0.001363 | 0.001702 | 0.000339 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.015623 | 0.013628 | 0.017948 | 0.004320 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.002832 | 0.002730 | 0.002979 | 0.000249 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.011451 | 0.008313 | 0.015026 | 0.006713 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.001335 | 0.001248 | 0.001382 | 0.000134 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.000959 | 0.000912 | 0.001039 | 0.000127 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.014911 | 0.011630 | 0.018575 | 0.006945 |
| nacl_small_rep03_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.001121 | 0.000992 | 0.001273 | 0.000281 |
| nacl_medium_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.003137 | 0.003137 | 0.003137 | 0.000000 |
| nacl_medium_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.007252 | 0.007252 | 0.007252 | 0.000000 |
| nacl_medium_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.026033 | 0.026033 | 0.026033 | 0.000000 |
| nacl_medium_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.033710 | 0.033710 | 0.033710 | 0.000000 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.016484 | 0.008766 | 0.024203 | 0.015437 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.001593 | 0.001585 | 0.001601 | 0.000016 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.020966 | 0.013238 | 0.028694 | 0.015456 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.002347 | 0.002330 | 0.002365 | 0.000035 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.005212 | 0.005178 | 0.005246 | 0.000068 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.003547 | 0.003518 | 0.003577 | 0.000059 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.001739 | 0.001697 | 0.001782 | 0.000085 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.012898 | 0.012842 | 0.012954 | 0.000112 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.001689 | 0.001684 | 0.001694 | 0.000010 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.023885 | 0.021267 | 0.026503 | 0.005236 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.014253 | 0.014235 | 0.014272 | 0.000037 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.051844 | 0.048637 | 0.055051 | 0.006414 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.013607 | 0.013034 | 0.014181 | 0.001147 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.020563 | 0.012812 | 0.028313 | 0.015501 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.010045 | 0.009827 | 0.010263 | 0.000436 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.011380 | 0.011295 | 0.011464 | 0.000169 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.047362 | 0.039992 | 0.054732 | 0.014740 |
| nacl_medium_rep01_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.005277 | 0.005049 | 0.005505 | 0.000456 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.014286 | 0.010841 | 0.015510 | 0.004669 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.000941 | 0.000904 | 0.000974 | 0.000070 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.016677 | 0.013266 | 0.017914 | 0.004648 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.000927 | 0.000918 | 0.000932 | 0.000014 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.012060 | 0.011877 | 0.012215 | 0.000338 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.001254 | 0.001212 | 0.001331 | 0.000119 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.001073 | 0.001062 | 0.001083 | 0.000021 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.016155 | 0.016064 | 0.016286 | 0.000222 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.001077 | 0.001042 | 0.001144 | 0.000102 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.024008 | 0.020430 | 0.025950 | 0.005520 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.004131 | 0.004056 | 0.004196 | 0.000140 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.043795 | 0.040065 | 0.045879 | 0.005814 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.015599 | 0.015454 | 0.015818 | 0.000364 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.020012 | 0.017747 | 0.021984 | 0.004237 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.006684 | 0.006611 | 0.006761 | 0.000150 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.003459 | 0.003308 | 0.003746 | 0.000438 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.032811 | 0.030564 | 0.034927 | 0.004363 |
| nacl_medium_rep01_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.002605 | 0.002561 | 0.002628 | 0.000067 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.020493 | 0.018062 | 0.021037 | 0.002975 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.000639 | 0.000617 | 0.000717 | 0.000100 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.022418 | 0.020117 | 0.022946 | 0.002829 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.000761 | 0.000735 | 0.000800 | 0.000065 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.020010 | 0.019798 | 0.020211 | 0.000413 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.000835 | 0.000808 | 0.000952 | 0.000144 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.000830 | 0.000802 | 0.000857 | 0.000055 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.023139 | 0.023028 | 0.023270 | 0.000242 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.000777 | 0.000734 | 0.000832 | 0.000098 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.017855 | 0.015935 | 0.018656 | 0.002721 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.001816 | 0.001788 | 0.001855 | 0.000067 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.032196 | 0.030146 | 0.032991 | 0.002845 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.012478 | 0.012316 | 0.012757 | 0.000441 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.015969 | 0.013663 | 0.018383 | 0.004720 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.004255 | 0.004137 | 0.004367 | 0.000230 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.001567 | 0.001445 | 0.001666 | 0.000221 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.022922 | 0.020613 | 0.025073 | 0.004460 |
| nacl_medium_rep01_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.001085 | 0.001066 | 0.001101 | 0.000035 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 873 | 0.015009 | 0.013498 | 0.015920 | 0.002422 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gatherp_pack | 873 | 0.003249 | 0.003132 | 0.003324 | 0.000192 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gatherp_scatters | 873 | 0.022703 | 0.021499 | 0.023674 | 0.002175 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gatherp_unpack | 873 | 0.003719 | 0.003497 | 0.003991 | 0.000494 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1017 | 0.014446 | 0.013073 | 0.015680 | 0.002607 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gathers_clear | 1017 | 0.005087 | 0.004902 | 0.005176 | 0.000274 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gathers_pack | 1017 | 0.003790 | 0.003546 | 0.004137 | 0.000591 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gathers_scatterp | 1017 | 0.028246 | 0.027390 | 0.029463 | 0.002073 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_K | gathers_unpack | 1017 | 0.003987 | 0.003707 | 0.004137 | 0.000430 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.033609 | 0.025790 | 0.042185 | 0.016395 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.006785 | 0.006460 | 0.007145 | 0.000685 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.053857 | 0.046999 | 0.061766 | 0.014767 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.013404 | 0.012253 | 0.014298 | 0.002045 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.026721 | 0.023241 | 0.030723 | 0.007482 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.005521 | 0.005367 | 0.005929 | 0.000562 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.005740 | 0.005327 | 0.006112 | 0.000785 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.045663 | 0.042519 | 0.048713 | 0.006194 |
| nacl_medium_rep01_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.007629 | 0.007244 | 0.008004 | 0.000760 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 873 | 0.026573 | 0.019401 | 0.032021 | 0.012620 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 873 | 0.002470 | 0.002164 | 0.002617 | 0.000453 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 873 | 0.032151 | 0.025238 | 0.037047 | 0.011809 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 873 | 0.002470 | 0.002261 | 0.002782 | 0.000521 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1017 | 0.023037 | 0.021986 | 0.024116 | 0.002130 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 1017 | 0.003501 | 0.003089 | 0.003884 | 0.000795 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 1017 | 0.002852 | 0.002557 | 0.003076 | 0.000519 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 1017 | 0.033339 | 0.032822 | 0.034025 | 0.001203 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 1017 | 0.003020 | 0.002552 | 0.003323 | 0.000771 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.024459 | 0.020511 | 0.029148 | 0.008637 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.004123 | 0.003919 | 0.004471 | 0.000552 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.036127 | 0.032565 | 0.040448 | 0.007883 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.007492 | 0.007202 | 0.007702 | 0.000500 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.024079 | 0.019659 | 0.027489 | 0.007830 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.004059 | 0.003905 | 0.004262 | 0.000357 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.002908 | 0.002726 | 0.003115 | 0.000389 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.034509 | 0.030502 | 0.037424 | 0.006922 |
| nacl_medium_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.003414 | 0.003225 | 0.003664 | 0.000439 |
| nacl_medium_rep02_np1_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.003127 | 0.003127 | 0.003127 | 0.000000 |
| nacl_medium_rep02_np1_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.007253 | 0.007253 | 0.007253 | 0.000000 |
| nacl_medium_rep02_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.026488 | 0.026488 | 0.026488 | 0.000000 |
| nacl_medium_rep02_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.034462 | 0.034462 | 0.034462 | 0.000000 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.005366 | 0.003693 | 0.007038 | 0.003345 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.001556 | 0.001510 | 0.001602 | 0.000092 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.009901 | 0.008205 | 0.011597 | 0.003392 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.002457 | 0.002384 | 0.002530 | 0.000146 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.005931 | 0.005649 | 0.006212 | 0.000563 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.003271 | 0.003179 | 0.003363 | 0.000184 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.001701 | 0.001697 | 0.001705 | 0.000008 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.013215 | 0.012871 | 0.013558 | 0.000687 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.001631 | 0.001597 | 0.001666 | 0.000069 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.021031 | 0.015281 | 0.026780 | 0.011499 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.014429 | 0.014367 | 0.014492 | 0.000125 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.048643 | 0.043006 | 0.054279 | 0.011273 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.013094 | 0.013035 | 0.013152 | 0.000117 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.014214 | 0.012773 | 0.015655 | 0.002882 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.009761 | 0.009741 | 0.009781 | 0.000040 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.011393 | 0.011309 | 0.011477 | 0.000168 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.040609 | 0.039226 | 0.041992 | 0.002766 |
| nacl_medium_rep02_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.005165 | 0.005118 | 0.005211 | 0.000093 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.014779 | 0.011255 | 0.016132 | 0.004877 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.000918 | 0.000895 | 0.000978 | 0.000083 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.017142 | 0.013677 | 0.018497 | 0.004820 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.000925 | 0.000906 | 0.000950 | 0.000044 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.011838 | 0.011636 | 0.011972 | 0.000336 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.001236 | 0.001198 | 0.001321 | 0.000123 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.001071 | 0.001044 | 0.001120 | 0.000076 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.015908 | 0.015874 | 0.015931 | 0.000057 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.001067 | 0.001023 | 0.001152 | 0.000129 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.023555 | 0.021367 | 0.025249 | 0.003882 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.004127 | 0.004115 | 0.004137 | 0.000022 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.043210 | 0.041072 | 0.045307 | 0.004235 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.015477 | 0.014954 | 0.015878 | 0.000924 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.020328 | 0.018256 | 0.021555 | 0.003299 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.006745 | 0.006547 | 0.007102 | 0.000555 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.003566 | 0.003324 | 0.004057 | 0.000733 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.033320 | 0.031027 | 0.034292 | 0.003265 |
| nacl_medium_rep02_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.002628 | 0.002605 | 0.002641 | 0.000036 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.020852 | 0.018265 | 0.021576 | 0.003311 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.000637 | 0.000618 | 0.000706 | 0.000088 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.022759 | 0.020227 | 0.023454 | 0.003227 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.000754 | 0.000738 | 0.000767 | 0.000029 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.019991 | 0.019817 | 0.020211 | 0.000394 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.000835 | 0.000815 | 0.000935 | 0.000120 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.000831 | 0.000795 | 0.000870 | 0.000075 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.023122 | 0.022953 | 0.023240 | 0.000287 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.000780 | 0.000738 | 0.000829 | 0.000091 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.017163 | 0.016005 | 0.018665 | 0.002660 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.001723 | 0.001662 | 0.001777 | 0.000115 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.031370 | 0.030176 | 0.032972 | 0.002796 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.012437 | 0.012236 | 0.012680 | 0.000444 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.015672 | 0.013155 | 0.017519 | 0.004364 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.004293 | 0.004206 | 0.004346 | 0.000140 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.001534 | 0.001401 | 0.001644 | 0.000243 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.022630 | 0.020233 | 0.024357 | 0.004124 |
| nacl_medium_rep02_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.001085 | 0.001076 | 0.001092 | 0.000016 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 873 | 0.016040 | 0.013282 | 0.018825 | 0.005543 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gatherp_pack | 873 | 0.003167 | 0.002963 | 0.003367 | 0.000404 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gatherp_scatters | 873 | 0.023582 | 0.021235 | 0.025773 | 0.004538 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gatherp_unpack | 873 | 0.003711 | 0.003440 | 0.003906 | 0.000466 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1017 | 0.014323 | 0.013216 | 0.015036 | 0.001820 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gathers_clear | 1017 | 0.004925 | 0.004336 | 0.005266 | 0.000930 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gathers_pack | 1017 | 0.003706 | 0.003531 | 0.003911 | 0.000380 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gathers_scatterp | 1017 | 0.027741 | 0.026936 | 0.029098 | 0.002162 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_K | gathers_unpack | 1017 | 0.003830 | 0.003408 | 0.004253 | 0.000845 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.028791 | 0.024418 | 0.032153 | 0.007735 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.006676 | 0.006529 | 0.006806 | 0.000277 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.048829 | 0.045204 | 0.051967 | 0.006763 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.013302 | 0.012630 | 0.014016 | 0.001386 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.032189 | 0.019131 | 0.041073 | 0.021942 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.005622 | 0.005411 | 0.005917 | 0.000506 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.005494 | 0.005414 | 0.005550 | 0.000136 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.050922 | 0.038040 | 0.059698 | 0.021658 |
| nacl_medium_rep02_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.007560 | 0.007355 | 0.007754 | 0.000399 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 873 | 0.044329 | 0.030296 | 0.049756 | 0.019460 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gatherp_pack | 873 | 0.002658 | 0.002362 | 0.003257 | 0.000895 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gatherp_scatters | 873 | 0.050354 | 0.037471 | 0.055289 | 0.017818 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gatherp_unpack | 873 | 0.002666 | 0.002288 | 0.003114 | 0.000826 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1017 | 0.024627 | 0.021627 | 0.026470 | 0.004843 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gathers_clear | 1017 | 0.004994 | 0.003258 | 0.014071 | 0.010813 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gathers_pack | 1017 | 0.002996 | 0.002625 | 0.003553 | 0.000928 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gathers_scatterp | 1017 | 0.036633 | 0.034451 | 0.044826 | 0.010375 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_K | gathers_unpack | 1017 | 0.003124 | 0.002704 | 0.004014 | 0.001310 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.029396 | 0.024605 | 0.033882 | 0.009277 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.004227 | 0.003953 | 0.004440 | 0.000487 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.041274 | 0.036811 | 0.045238 | 0.008427 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.007598 | 0.007280 | 0.007836 | 0.000556 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.026399 | 0.017762 | 0.032593 | 0.014831 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.004024 | 0.003900 | 0.004142 | 0.000242 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.003059 | 0.002800 | 0.003288 | 0.000488 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.037035 | 0.028500 | 0.042849 | 0.014349 |
| nacl_medium_rep02_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.003500 | 0.003271 | 0.003643 | 0.000372 |
| nacl_medium_rep03_np1_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.003132 | 0.003132 | 0.003132 | 0.000000 |
| nacl_medium_rep03_np1_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.007181 | 0.007181 | 0.007181 | 0.000000 |
| nacl_medium_rep03_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.025969 | 0.025969 | 0.025969 | 0.000000 |
| nacl_medium_rep03_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.034165 | 0.034165 | 0.034165 | 0.000000 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.007114 | 0.005077 | 0.009151 | 0.004074 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.001548 | 0.001487 | 0.001608 | 0.000121 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.011536 | 0.009707 | 0.013364 | 0.003657 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.002339 | 0.002190 | 0.002488 | 0.000298 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.004689 | 0.004429 | 0.004949 | 0.000520 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.003419 | 0.003295 | 0.003542 | 0.000247 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.001694 | 0.001691 | 0.001697 | 0.000006 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.012131 | 0.012037 | 0.012225 | 0.000188 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.001633 | 0.001585 | 0.001680 | 0.000095 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.024331 | 0.022962 | 0.025700 | 0.002738 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.014377 | 0.014353 | 0.014401 | 0.000048 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.052306 | 0.051130 | 0.053481 | 0.002351 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.013503 | 0.013284 | 0.013722 | 0.000438 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.030156 | 0.016430 | 0.043883 | 0.027453 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.010046 | 0.009893 | 0.010198 | 0.000305 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.011439 | 0.011384 | 0.011494 | 0.000110 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.056965 | 0.043450 | 0.070480 | 0.027030 |
| nacl_medium_rep03_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.005241 | 0.005124 | 0.005358 | 0.000234 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.014270 | 0.010795 | 0.015487 | 0.004692 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.000916 | 0.000890 | 0.000980 | 0.000090 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.016642 | 0.013242 | 0.017853 | 0.004611 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.000944 | 0.000919 | 0.000974 | 0.000055 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.011987 | 0.011752 | 0.012158 | 0.000406 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.001227 | 0.001186 | 0.001323 | 0.000137 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.001053 | 0.001041 | 0.001066 | 0.000025 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.016023 | 0.015975 | 0.016128 | 0.000153 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.001077 | 0.001020 | 0.001164 | 0.000144 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.023242 | 0.021007 | 0.025731 | 0.004724 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.004147 | 0.004096 | 0.004171 | 0.000075 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.043039 | 0.040995 | 0.045627 | 0.004632 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.015596 | 0.015469 | 0.015763 | 0.000294 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.019615 | 0.018695 | 0.020692 | 0.001997 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.006621 | 0.006411 | 0.006764 | 0.000353 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.003603 | 0.003377 | 0.003855 | 0.000478 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.032533 | 0.031871 | 0.033195 | 0.001324 |
| nacl_medium_rep03_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.002644 | 0.002624 | 0.002663 | 0.000039 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 873 | 0.020659 | 0.018170 | 0.021286 | 0.003116 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gatherp_pack | 873 | 0.000635 | 0.000611 | 0.000706 | 0.000095 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gatherp_scatters | 873 | 0.022569 | 0.020167 | 0.023148 | 0.002981 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gatherp_unpack | 873 | 0.000758 | 0.000736 | 0.000775 | 0.000039 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1017 | 0.019669 | 0.019457 | 0.019893 | 0.000436 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gathers_clear | 1017 | 0.000847 | 0.000808 | 0.000948 | 0.000140 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gathers_pack | 1017 | 0.000834 | 0.000801 | 0.000864 | 0.000063 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gathers_scatterp | 1017 | 0.022823 | 0.022650 | 0.023103 | 0.000453 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_K | gathers_unpack | 1017 | 0.000780 | 0.000736 | 0.000807 | 0.000071 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.017268 | 0.016472 | 0.018309 | 0.001837 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.001695 | 0.001667 | 0.001723 | 0.000056 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.031511 | 0.030717 | 0.032819 | 0.002102 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.012500 | 0.012081 | 0.012796 | 0.000715 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.023818 | 0.014230 | 0.026835 | 0.012605 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.004310 | 0.004211 | 0.004395 | 0.000184 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.001569 | 0.001394 | 0.001729 | 0.000335 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.030832 | 0.021346 | 0.033655 | 0.012309 |
| nacl_medium_rep03_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.001089 | 0.001077 | 0.001098 | 0.000021 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 873 | 0.021794 | 0.012697 | 0.032509 | 0.019812 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gatherp_pack | 873 | 0.003247 | 0.003091 | 0.003455 | 0.000364 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gatherp_scatters | 873 | 0.029583 | 0.020750 | 0.040532 | 0.019782 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gatherp_unpack | 873 | 0.003853 | 0.003802 | 0.003999 | 0.000197 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1017 | 0.027023 | 0.014395 | 0.032024 | 0.017629 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gathers_clear | 1017 | 0.005039 | 0.004533 | 0.005441 | 0.000908 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gathers_pack | 1017 | 0.003924 | 0.003813 | 0.004032 | 0.000219 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gathers_scatterp | 1017 | 0.040825 | 0.028931 | 0.045398 | 0.016467 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_K | gathers_unpack | 1017 | 0.004019 | 0.003584 | 0.004452 | 0.000868 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.034144 | 0.024172 | 0.038350 | 0.014178 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.006957 | 0.006679 | 0.007357 | 0.000678 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.054707 | 0.046059 | 0.058334 | 0.012275 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.013544 | 0.012797 | 0.014472 | 0.001675 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.023147 | 0.017397 | 0.026103 | 0.008706 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.005635 | 0.005363 | 0.006012 | 0.000649 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.005782 | 0.005499 | 0.006045 | 0.000546 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.042578 | 0.037886 | 0.044654 | 0.006768 |
| nacl_medium_rep03_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.007956 | 0.007596 | 0.008385 | 0.000789 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 873 | 0.031786 | 0.020037 | 0.034700 | 0.014663 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gatherp_pack | 873 | 0.002518 | 0.002371 | 0.002955 | 0.000584 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gatherp_scatters | 873 | 0.037508 | 0.026479 | 0.040467 | 0.013988 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gatherp_unpack | 873 | 0.002520 | 0.002273 | 0.002746 | 0.000473 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1017 | 0.022907 | 0.021488 | 0.023717 | 0.002229 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gathers_clear | 1017 | 0.003625 | 0.003341 | 0.004495 | 0.001154 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gathers_pack | 1017 | 0.002980 | 0.002868 | 0.003134 | 0.000266 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gathers_scatterp | 1017 | 0.033399 | 0.032849 | 0.033850 | 0.001001 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_K | gathers_unpack | 1017 | 0.003072 | 0.002809 | 0.003608 | 0.000799 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.027951 | 0.025400 | 0.030682 | 0.005282 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.004163 | 0.003987 | 0.004431 | 0.000444 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.039809 | 0.037259 | 0.042451 | 0.005192 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.007642 | 0.007336 | 0.007757 | 0.000421 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.030414 | 0.022386 | 0.041358 | 0.018972 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.004141 | 0.003889 | 0.004344 | 0.000455 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.003099 | 0.002971 | 0.003306 | 0.000335 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.041188 | 0.033616 | 0.051927 | 0.018311 |
| nacl_medium_rep03_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.003481 | 0.003362 | 0.003686 | 0.000324 |
| nacl_large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.004573 | 0.004573 | 0.004573 | 0.000000 |
| nacl_large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.012446 | 0.012446 | 0.012446 | 0.000000 |
| nacl_large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.092686 | 0.092686 | 0.092686 | 0.000000 |
| nacl_large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.132378 | 0.132378 | 0.132378 | 0.000000 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.008667 | 0.007195 | 0.010140 | 0.002945 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.002178 | 0.002162 | 0.002194 | 0.000032 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.014742 | 0.013362 | 0.016122 | 0.002760 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.003352 | 0.003248 | 0.003456 | 0.000208 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.005888 | 0.005675 | 0.006101 | 0.000426 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.006851 | 0.006795 | 0.006908 | 0.000113 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.002452 | 0.002436 | 0.002469 | 0.000033 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.018146 | 0.018006 | 0.018286 | 0.000280 |
| nacl_large_rep01_np2_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.002249 | 0.002249 | 0.002250 | 0.000001 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.088759 | 0.052545 | 0.124973 | 0.072428 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.048862 | 0.047010 | 0.050714 | 0.003704 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.188739 | 0.151181 | 0.226296 | 0.075115 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.050963 | 0.047782 | 0.054144 | 0.006362 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.041131 | 0.041100 | 0.041161 | 0.000061 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.028317 | 0.027947 | 0.028687 | 0.000740 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.037023 | 0.036906 | 0.037140 | 0.000234 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.139009 | 0.138471 | 0.139546 | 0.001075 |
| nacl_large_rep01_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.032359 | 0.032266 | 0.032452 | 0.000186 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.042354 | 0.027759 | 0.056333 | 0.028574 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.001272 | 0.001180 | 0.001358 | 0.000178 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.053812 | 0.039865 | 0.067494 | 0.027629 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.009654 | 0.009185 | 0.010228 | 0.001043 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.031047 | 0.029548 | 0.032217 | 0.002669 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.006452 | 0.005944 | 0.006658 | 0.000714 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.001420 | 0.001416 | 0.001422 | 0.000006 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.041055 | 0.039767 | 0.042104 | 0.002337 |
| nacl_large_rep01_np4_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.001435 | 0.001355 | 0.001512 | 0.000157 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.084580 | 0.077584 | 0.098621 | 0.021037 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.018275 | 0.018187 | 0.018361 | 0.000174 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.148807 | 0.142485 | 0.161247 | 0.018762 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.045870 | 0.044350 | 0.047271 | 0.002921 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.063483 | 0.059687 | 0.073811 | 0.014124 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.018328 | 0.017774 | 0.018800 | 0.001026 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.013654 | 0.013301 | 0.014319 | 0.001018 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.102222 | 0.097925 | 0.111917 | 0.013992 |
| nacl_large_rep01_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.006685 | 0.006560 | 0.006793 | 0.000233 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.030275 | 0.021708 | 0.052186 | 0.030478 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.000817 | 0.000719 | 0.000871 | 0.000152 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.032525 | 0.023952 | 0.054291 | 0.030339 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.000903 | 0.000875 | 0.000943 | 0.000068 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.022314 | 0.021495 | 0.023999 | 0.002504 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.001144 | 0.001012 | 0.001203 | 0.000191 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.001053 | 0.001017 | 0.001100 | 0.000083 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.026140 | 0.025443 | 0.027593 | 0.002150 |
| nacl_large_rep01_np8_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.000937 | 0.000863 | 0.000984 | 0.000121 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.050395 | 0.043403 | 0.055567 | 0.012164 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.005678 | 0.005520 | 0.006025 | 0.000505 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.088159 | 0.081869 | 0.093564 | 0.011695 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.032028 | 0.031294 | 0.033070 | 0.001776 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.040925 | 0.034204 | 0.048350 | 0.014146 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.009805 | 0.009522 | 0.010154 | 0.000632 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.004707 | 0.004530 | 0.004888 | 0.000358 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.058347 | 0.051591 | 0.065670 | 0.014079 |
| nacl_large_rep01_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.002853 | 0.002821 | 0.002892 | 0.000071 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 869 | 0.033636 | 0.027813 | 0.039133 | 0.011320 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gatherp_pack | 869 | 0.004096 | 0.003934 | 0.004348 | 0.000414 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gatherp_scatters | 869 | 0.045308 | 0.039620 | 0.050432 | 0.010812 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gatherp_unpack | 869 | 0.006912 | 0.006739 | 0.007102 | 0.000363 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1013 | 0.029724 | 0.028839 | 0.030603 | 0.001764 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gathers_clear | 1013 | 0.009034 | 0.008421 | 0.009667 | 0.001246 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gathers_pack | 1013 | 0.004511 | 0.004385 | 0.004627 | 0.000242 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gathers_scatterp | 1013 | 0.049689 | 0.048770 | 0.051187 | 0.002417 |
| nacl_large_rep01_np4_omp2 | PW_Basis_K | gathers_unpack | 1013 | 0.005404 | 0.005237 | 0.005639 | 0.000402 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.080682 | 0.075390 | 0.087540 | 0.012150 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.017610 | 0.017396 | 0.017718 | 0.000322 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.129984 | 0.124035 | 0.137721 | 0.013686 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.031619 | 0.031176 | 0.032404 | 0.001228 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.068778 | 0.061445 | 0.078139 | 0.016694 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.012564 | 0.012010 | 0.013606 | 0.001596 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.015255 | 0.014880 | 0.015687 | 0.000807 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.116663 | 0.109357 | 0.125788 | 0.016431 |
| nacl_large_rep01_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.019956 | 0.019361 | 0.020947 | 0.001586 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 869 | 0.031936 | 0.026819 | 0.041386 | 0.014567 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 869 | 0.002895 | 0.002583 | 0.003223 | 0.000640 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 869 | 0.038641 | 0.033567 | 0.047827 | 0.014260 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 869 | 0.003082 | 0.002920 | 0.003362 | 0.000442 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1013 | 0.025190 | 0.023823 | 0.026103 | 0.002280 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 1013 | 0.004468 | 0.003829 | 0.004749 | 0.000920 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 1013 | 0.003209 | 0.002990 | 0.003421 | 0.000431 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 1013 | 0.037018 | 0.036528 | 0.037633 | 0.001105 |
| nacl_large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 1013 | 0.003224 | 0.002922 | 0.003679 | 0.000757 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.055658 | 0.046837 | 0.061538 | 0.014701 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.009204 | 0.008642 | 0.009536 | 0.000894 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.084040 | 0.075602 | 0.089555 | 0.013953 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.019117 | 0.018368 | 0.019521 | 0.001153 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.052328 | 0.036843 | 0.060226 | 0.023383 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.008535 | 0.008168 | 0.008926 | 0.000758 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.007467 | 0.007233 | 0.007708 | 0.000475 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.077112 | 0.061172 | 0.085796 | 0.024624 |
| nacl_large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.008727 | 0.008367 | 0.009150 | 0.000783 |
| nacl_large_rep02_np1_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.004393 | 0.004393 | 0.004393 | 0.000000 |
| nacl_large_rep02_np1_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.012480 | 0.012480 | 0.012480 | 0.000000 |
| nacl_large_rep02_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.091673 | 0.091673 | 0.091673 | 0.000000 |
| nacl_large_rep02_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.133415 | 0.133415 | 0.133415 | 0.000000 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.011513 | 0.006683 | 0.016343 | 0.009660 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.002186 | 0.002169 | 0.002203 | 0.000034 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.017839 | 0.012959 | 0.022718 | 0.009759 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.003593 | 0.003534 | 0.003651 | 0.000117 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.006479 | 0.005950 | 0.007008 | 0.001058 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.006925 | 0.006909 | 0.006941 | 0.000032 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.002478 | 0.002470 | 0.002487 | 0.000017 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.018867 | 0.018351 | 0.019384 | 0.001033 |
| nacl_large_rep02_np2_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.002282 | 0.002252 | 0.002312 | 0.000060 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.088312 | 0.071728 | 0.104895 | 0.033167 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.048252 | 0.047164 | 0.049340 | 0.002176 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.187452 | 0.165873 | 0.209031 | 0.043158 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.050726 | 0.044645 | 0.056807 | 0.012162 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.072362 | 0.045579 | 0.099146 | 0.053567 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.028683 | 0.027516 | 0.029850 | 0.002334 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.037019 | 0.036936 | 0.037103 | 0.000167 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.171400 | 0.146323 | 0.196476 | 0.050153 |
| nacl_large_rep02_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.033154 | 0.032524 | 0.033784 | 0.001260 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.048245 | 0.033907 | 0.062509 | 0.028602 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.001300 | 0.001197 | 0.001433 | 0.000236 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.059776 | 0.045962 | 0.073799 | 0.027837 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.009692 | 0.009269 | 0.010081 | 0.000812 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.030402 | 0.028879 | 0.031957 | 0.003078 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.006291 | 0.005922 | 0.006519 | 0.000597 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.001434 | 0.001419 | 0.001444 | 0.000025 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.040295 | 0.039075 | 0.041896 | 0.002821 |
| nacl_large_rep02_np4_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.001455 | 0.001369 | 0.001544 | 0.000175 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.083677 | 0.072608 | 0.090219 | 0.017611 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.017815 | 0.017627 | 0.017928 | 0.000301 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.147347 | 0.136401 | 0.156682 | 0.020281 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.045773 | 0.044252 | 0.048442 | 0.004190 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.064928 | 0.056066 | 0.073889 | 0.017823 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.017777 | 0.017357 | 0.018557 | 0.001200 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.013463 | 0.013177 | 0.014000 | 0.000823 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.102818 | 0.095485 | 0.111244 | 0.015759 |
| nacl_large_rep02_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.006575 | 0.006493 | 0.006786 | 0.000293 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.030748 | 0.022423 | 0.051657 | 0.029234 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.000815 | 0.000731 | 0.000854 | 0.000123 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.032985 | 0.024668 | 0.053770 | 0.029102 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.000902 | 0.000878 | 0.000945 | 0.000067 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.022766 | 0.022068 | 0.024506 | 0.002438 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.001133 | 0.000983 | 0.001215 | 0.000232 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.001038 | 0.001010 | 0.001075 | 0.000065 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.026564 | 0.025920 | 0.028072 | 0.002152 |
| nacl_large_rep02_np8_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.000934 | 0.000904 | 0.000969 | 0.000065 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.045169 | 0.040542 | 0.052110 | 0.011568 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.005686 | 0.005552 | 0.005985 | 0.000433 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.082942 | 0.079188 | 0.088947 | 0.009759 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.032030 | 0.031231 | 0.032891 | 0.001660 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.046798 | 0.041876 | 0.054991 | 0.013115 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.009652 | 0.009330 | 0.009951 | 0.000621 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.004676 | 0.004490 | 0.004953 | 0.000463 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.064037 | 0.058806 | 0.072056 | 0.013250 |
| nacl_large_rep02_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.002853 | 0.002813 | 0.002918 | 0.000105 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 869 | 0.037279 | 0.034040 | 0.043431 | 0.009391 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gatherp_pack | 869 | 0.004036 | 0.003675 | 0.004343 | 0.000668 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gatherp_scatters | 869 | 0.048925 | 0.045915 | 0.054536 | 0.008621 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gatherp_unpack | 869 | 0.006879 | 0.006586 | 0.007244 | 0.000658 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1013 | 0.031409 | 0.030294 | 0.032745 | 0.002451 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gathers_clear | 1013 | 0.008992 | 0.008473 | 0.009382 | 0.000909 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gathers_pack | 1013 | 0.004557 | 0.004383 | 0.004750 | 0.000367 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gathers_scatterp | 1013 | 0.051250 | 0.050281 | 0.052946 | 0.002665 |
| nacl_large_rep02_np4_omp2 | PW_Basis_K | gathers_unpack | 1013 | 0.005387 | 0.004945 | 0.005853 | 0.000908 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.085014 | 0.075143 | 0.092126 | 0.016983 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.017311 | 0.016986 | 0.017675 | 0.000689 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.133815 | 0.123972 | 0.140048 | 0.016076 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.031418 | 0.030867 | 0.032313 | 0.001446 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.065236 | 0.060206 | 0.074425 | 0.014219 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.012382 | 0.011729 | 0.013888 | 0.002159 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.015098 | 0.014782 | 0.015317 | 0.000535 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.112057 | 0.106413 | 0.121505 | 0.015092 |
| nacl_large_rep02_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.019271 | 0.019034 | 0.019645 | 0.000611 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 869 | 0.034494 | 0.027054 | 0.047957 | 0.020903 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gatherp_pack | 869 | 0.002902 | 0.002584 | 0.003280 | 0.000696 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gatherp_scatters | 869 | 0.041137 | 0.033737 | 0.054009 | 0.020272 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gatherp_unpack | 869 | 0.003024 | 0.002772 | 0.003280 | 0.000508 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1013 | 0.026368 | 0.024474 | 0.028217 | 0.003743 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gathers_clear | 1013 | 0.004453 | 0.003849 | 0.004825 | 0.000976 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gathers_pack | 1013 | 0.003146 | 0.002949 | 0.003535 | 0.000586 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gathers_scatterp | 1013 | 0.038262 | 0.036869 | 0.039433 | 0.002564 |
| nacl_large_rep02_np8_omp2 | PW_Basis_K | gathers_unpack | 1013 | 0.003354 | 0.002836 | 0.003911 | 0.001075 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.054071 | 0.045255 | 0.063362 | 0.018107 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.009170 | 0.008679 | 0.009844 | 0.001165 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.082341 | 0.074399 | 0.090356 | 0.015957 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.019038 | 0.018212 | 0.019371 | 0.001159 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.057012 | 0.048477 | 0.064739 | 0.016262 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.008208 | 0.007879 | 0.008920 | 0.001041 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.007389 | 0.006978 | 0.007895 | 0.000917 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.081270 | 0.074490 | 0.088194 | 0.013704 |
| nacl_large_rep02_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.008602 | 0.008211 | 0.009141 | 0.000930 |
| nacl_large_rep03_np1_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.004452 | 0.004452 | 0.004452 | 0.000000 |
| nacl_large_rep03_np1_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.012472 | 0.012472 | 0.012472 | 0.000000 |
| nacl_large_rep03_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.090867 | 0.090867 | 0.090867 | 0.000000 |
| nacl_large_rep03_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.132355 | 0.132355 | 0.132355 | 0.000000 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.008868 | 0.008725 | 0.009011 | 0.000286 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.002225 | 0.002207 | 0.002243 | 0.000036 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.015116 | 0.015016 | 0.015217 | 0.000201 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.003470 | 0.003443 | 0.003498 | 0.000055 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.008526 | 0.008328 | 0.008725 | 0.000397 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.006896 | 0.006874 | 0.006919 | 0.000045 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.002482 | 0.002477 | 0.002487 | 0.000010 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.020878 | 0.020698 | 0.021057 | 0.000359 |
| nacl_large_rep03_np2_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.002257 | 0.002255 | 0.002260 | 0.000005 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.085923 | 0.062852 | 0.108993 | 0.046141 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.050516 | 0.048554 | 0.052477 | 0.003923 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.186787 | 0.160148 | 0.213426 | 0.053278 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.050180 | 0.044656 | 0.055703 | 0.011047 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.061369 | 0.046787 | 0.075950 | 0.029163 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.028782 | 0.027457 | 0.030106 | 0.002649 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.036844 | 0.036665 | 0.037022 | 0.000357 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.159614 | 0.147807 | 0.171421 | 0.023614 |
| nacl_large_rep03_np2_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.032459 | 0.031185 | 0.033733 | 0.002548 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.042922 | 0.027625 | 0.058230 | 0.030605 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.001282 | 0.001190 | 0.001370 | 0.000180 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.054420 | 0.039612 | 0.069762 | 0.030150 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.009671 | 0.009413 | 0.010057 | 0.000644 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.029767 | 0.028384 | 0.031130 | 0.002746 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.006455 | 0.006169 | 0.006795 | 0.000626 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.001433 | 0.001418 | 0.001449 | 0.000031 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.039800 | 0.038408 | 0.041102 | 0.002694 |
| nacl_large_rep03_np4_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.001436 | 0.001347 | 0.001521 | 0.000174 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.084304 | 0.072576 | 0.098532 | 0.025956 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.018441 | 0.018262 | 0.018669 | 0.000407 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.148895 | 0.136452 | 0.163240 | 0.026788 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.046069 | 0.045522 | 0.047268 | 0.001746 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.059738 | 0.053586 | 0.070540 | 0.016954 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.018234 | 0.017870 | 0.018583 | 0.000713 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.013607 | 0.013232 | 0.014196 | 0.000964 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.098291 | 0.091984 | 0.108907 | 0.016923 |
| nacl_large_rep03_np4_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.006640 | 0.006437 | 0.006789 | 0.000352 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 869 | 0.029137 | 0.020983 | 0.051194 | 0.030211 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gatherp_pack | 869 | 0.000808 | 0.000728 | 0.000848 | 0.000120 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gatherp_scatters | 869 | 0.031349 | 0.023230 | 0.053336 | 0.030106 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gatherp_unpack | 869 | 0.000887 | 0.000863 | 0.000910 | 0.000047 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gathers_alltoallv | 1013 | 0.022262 | 0.021534 | 0.023730 | 0.002196 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gathers_clear | 1013 | 0.001113 | 0.000986 | 0.001166 | 0.000180 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gathers_pack | 1013 | 0.001047 | 0.001013 | 0.001124 | 0.000111 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gathers_scatterp | 1013 | 0.026042 | 0.025422 | 0.027386 | 0.001964 |
| nacl_large_rep03_np8_omp1 | PW_Basis_K | gathers_unpack | 1013 | 0.000933 | 0.000899 | 0.000953 | 0.000054 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.053494 | 0.043013 | 0.059378 | 0.016365 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_pack | 65 | 0.005675 | 0.005505 | 0.005890 | 0.000385 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 65 | 0.090943 | 0.080017 | 0.096396 | 0.016379 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 65 | 0.031716 | 0.030912 | 0.033125 | 0.002213 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.042235 | 0.033044 | 0.050958 | 0.017914 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gathers_clear | 49 | 0.009589 | 0.009323 | 0.010080 | 0.000757 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gathers_pack | 49 | 0.004709 | 0.004598 | 0.004992 | 0.000394 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 49 | 0.059436 | 0.050137 | 0.067765 | 0.017628 |
| nacl_large_rep03_np8_omp1 | PW_Basis_Sup | gathers_unpack | 49 | 0.002845 | 0.002806 | 0.002875 | 0.000069 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 869 | 0.041225 | 0.035808 | 0.045072 | 0.009264 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gatherp_pack | 869 | 0.003695 | 0.003108 | 0.004027 | 0.000919 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gatherp_scatters | 869 | 0.052347 | 0.047367 | 0.056290 | 0.008923 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gatherp_unpack | 869 | 0.006652 | 0.006227 | 0.006844 | 0.000617 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gathers_alltoallv | 1013 | 0.031943 | 0.030053 | 0.036504 | 0.006451 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gathers_clear | 1013 | 0.008710 | 0.008007 | 0.009276 | 0.001269 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gathers_pack | 1013 | 0.004244 | 0.003320 | 0.004608 | 0.001288 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gathers_scatterp | 1013 | 0.050503 | 0.049219 | 0.052775 | 0.003556 |
| nacl_large_rep03_np4_omp2 | PW_Basis_K | gathers_unpack | 1013 | 0.004800 | 0.004019 | 0.005191 | 0.001172 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.083238 | 0.072153 | 0.090365 | 0.018212 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.017268 | 0.016465 | 0.017857 | 0.001392 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.131702 | 0.122003 | 0.137902 | 0.015899 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.031123 | 0.030536 | 0.031924 | 0.001388 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.075650 | 0.061426 | 0.087730 | 0.026304 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.012441 | 0.011737 | 0.014188 | 0.002451 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.014680 | 0.013850 | 0.015294 | 0.001444 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.121525 | 0.107389 | 0.133772 | 0.026383 |
| nacl_large_rep03_np4_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.018685 | 0.016868 | 0.019577 | 0.002709 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 869 | 0.034085 | 0.026429 | 0.044452 | 0.018023 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gatherp_pack | 869 | 0.002869 | 0.001902 | 0.003334 | 0.001432 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gatherp_scatters | 869 | 0.040681 | 0.033117 | 0.051090 | 0.017973 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gatherp_unpack | 869 | 0.003051 | 0.002145 | 0.003305 | 0.001160 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gathers_alltoallv | 1013 | 0.026448 | 0.024143 | 0.032429 | 0.008286 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gathers_clear | 1013 | 0.004311 | 0.002986 | 0.004838 | 0.001852 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gathers_pack | 1013 | 0.003123 | 0.002277 | 0.003468 | 0.001191 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gathers_scatterp | 1013 | 0.037968 | 0.037034 | 0.040629 | 0.003595 |
| nacl_large_rep03_np8_omp2 | PW_Basis_K | gathers_unpack | 1013 | 0.003182 | 0.002133 | 0.003793 | 0.001660 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 65 | 0.059766 | 0.054436 | 0.066601 | 0.012165 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_pack | 65 | 0.009088 | 0.008284 | 0.009643 | 0.001359 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 65 | 0.087894 | 0.083411 | 0.094427 | 0.011016 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 65 | 0.018977 | 0.018388 | 0.019318 | 0.000930 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 49 | 0.050459 | 0.045214 | 0.055575 | 0.010361 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gathers_clear | 49 | 0.008180 | 0.007893 | 0.008682 | 0.000789 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gathers_pack | 49 | 0.007511 | 0.006581 | 0.007912 | 0.001331 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 49 | 0.074481 | 0.070404 | 0.080433 | 0.010029 |
| nacl_large_rep03_np8_omp2 | PW_Basis_Sup | gathers_unpack | 49 | 0.008272 | 0.007177 | 0.008856 | 0.001679 |

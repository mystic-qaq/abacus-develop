# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 3 | 10.577 +/- 0.012 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 3 | 10.577 +/- 0.012 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 2 | 1 | PW_Basis_K | 3 | 6.947 +/- 0.006 | 0.136827 +/- 0.008390 | 0.072779 +/- 0.015654 | 0.111546 +/- 0.001177 |
| large | 2 | 1 | PW_Basis_Sup | 3 | 6.947 +/- 0.006 | 0.248873 +/- 0.014619 | 0.110603 +/- 0.015057 | 0.333428 +/- 0.004457 |
| large | 4 | 1 | PW_Basis_K | 3 | 4.583 +/- 0.083 | 0.135522 +/- 0.024232 | 0.033599 +/- 0.022489 | 0.066453 +/- 0.000871 |
| large | 4 | 1 | PW_Basis_Sup | 3 | 4.583 +/- 0.083 | 0.178625 +/- 0.021820 | 0.048239 +/- 0.024508 | 0.210668 +/- 0.001786 |
| large | 4 | 2 | PW_Basis_K | 3 | 3.883 +/- 0.095 | 0.190916 +/- 0.019008 | 0.016624 +/- 0.012354 | 0.085781 +/- 0.006159 |
| large | 4 | 2 | PW_Basis_Sup | 3 | 3.883 +/- 0.095 | 0.159167 +/- 0.010691 | 0.018041 +/- 0.001222 | 0.113788 +/- 0.001624 |
| large | 8 | 1 | PW_Basis_K | 3 | 3.410 +/- 0.061 | 0.247435 +/- 0.015438 | 0.114980 +/- 0.012990 | 0.026947 +/- 0.000125 |
| large | 8 | 1 | PW_Basis_Sup | 3 | 3.410 +/- 0.061 | 0.161494 +/- 0.011496 | 0.064107 +/- 0.010864 | 0.101593 +/- 0.000184 |
| large | 8 | 2 | PW_Basis_K | 3 | 3.040 +/- 0.044 | 0.267720 +/- 0.040828 | 0.089422 +/- 0.035504 | 0.036676 +/- 0.001496 |
| large | 8 | 2 | PW_Basis_Sup | 3 | 3.040 +/- 0.044 | 0.149780 +/- 0.012773 | 0.050783 +/- 0.008430 | 0.057809 +/- 0.000320 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 10.570 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 10.570 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_K | 6.950 | 0.102110 | 0.129773 | 0.055326 | 0.111395 |
| large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_Sup | 6.950 | 0.199289 | 0.263268 | 0.127959 | 0.328386 |
| large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_K | 4.490 | 0.104203 | 0.107565 | 0.007740 | 0.065612 |
| large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_Sup | 4.490 | 0.145119 | 0.156678 | 0.024034 | 0.212609 |
| large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_K | 3.980 | 0.195610 | 0.198522 | 0.005283 | 0.092850 |
| large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_Sup | 3.980 | 0.162617 | 0.171500 | 0.017887 | 0.112635 |
| large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_K | 3.440 | 0.201105 | 0.254505 | 0.122323 | 0.026995 |
| large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_Sup | 3.440 | 0.138553 | 0.169145 | 0.070939 | 0.101380 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 3.070 | 0.232319 | 0.266786 | 0.087856 | 0.038347 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 3.070 | 0.136847 | 0.159978 | 0.056133 | 0.058179 |
| large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_K | 10.590 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_Sup | 10.590 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_K | 6.940 | 0.095886 | 0.134602 | 0.077432 | 0.110453 |
| large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_Sup | 6.940 | 0.198789 | 0.249312 | 0.101046 | 0.335056 |
| large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_K | 4.610 | 0.135314 | 0.148522 | 0.044464 | 0.066396 |
| large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_Sup | 4.610 | 0.165278 | 0.178882 | 0.047644 | 0.209093 |
| large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_K | 3.790 | 0.162566 | 0.169283 | 0.014799 | 0.081564 |
| large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_Sup | 3.790 | 0.140881 | 0.152530 | 0.019333 | 0.115645 |
| large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_K | 3.340 | 0.180147 | 0.229728 | 0.099982 | 0.026805 |
| large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_Sup | 3.340 | 0.121690 | 0.148274 | 0.051580 | 0.101692 |
| large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_K | 2.990 | 0.203398 | 0.227367 | 0.054727 | 0.035461 |
| large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_Sup | 2.990 | 0.115440 | 0.135453 | 0.041066 | 0.057629 |
| large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_K | 10.570 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_Sup | 10.570 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_K | 6.950 | 0.103316 | 0.146105 | 0.085578 | 0.112791 |
| large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_Sup | 6.950 | 0.182638 | 0.234040 | 0.102803 | 0.336842 |
| large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_K | 4.650 | 0.134452 | 0.150480 | 0.048592 | 0.067351 |
| large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_Sup | 4.650 | 0.166004 | 0.200315 | 0.073040 | 0.210302 |
| large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_K | 3.880 | 0.192720 | 0.204943 | 0.029789 | 0.082930 |
| large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_Sup | 3.880 | 0.144034 | 0.153470 | 0.016903 | 0.113085 |
| large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_K | 3.450 | 0.202953 | 0.258073 | 0.122636 | 0.027041 |
| large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_Sup | 3.450 | 0.137354 | 0.167063 | 0.069802 | 0.101706 |
| large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_K | 3.060 | 0.239997 | 0.309007 | 0.125684 | 0.036221 |
| large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_Sup | 3.060 | 0.132648 | 0.153910 | 0.055150 | 0.057619 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 3366 | 0.026436 | 0.026436 | 0.026436 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 4086 | 0.085946 | 0.085946 | 0.085946 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.122901 | 0.122901 | 0.122901 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.208835 | 0.208835 | 0.208835 | 0.000000 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.070469 | 0.047946 | 0.092992 | 0.045046 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_pack | 3362 | 0.014484 | 0.013873 | 0.015095 | 0.001222 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_scatters | 3362 | 0.107230 | 0.086555 | 0.127905 | 0.041350 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_unpack | 3362 | 0.020034 | 0.018842 | 0.021225 | 0.002383 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_alltoallv | 4082 | 0.031641 | 0.026501 | 0.036781 | 0.010280 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_clear | 4082 | 0.044138 | 0.044035 | 0.044241 | 0.000206 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_pack | 4082 | 0.014146 | 0.013987 | 0.014305 | 0.000318 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_scatterp | 4082 | 0.111570 | 0.107055 | 0.116086 | 0.009031 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_unpack | 4082 | 0.018593 | 0.018328 | 0.018858 | 0.000530 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.117234 | 0.075531 | 0.158937 | 0.083406 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.070265 | 0.065757 | 0.074772 | 0.009015 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.271355 | 0.230937 | 0.311774 | 0.080837 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.083676 | 0.080444 | 0.086908 | 0.006464 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.082055 | 0.059778 | 0.104331 | 0.044553 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.048785 | 0.048263 | 0.049306 | 0.001043 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.063210 | 0.059981 | 0.066439 | 0.006458 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.256717 | 0.238306 | 0.275129 | 0.036823 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.062450 | 0.061294 | 0.063607 | 0.002313 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 3363 | 0.057570 | 0.053445 | 0.060526 | 0.007081 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_pack | 3363 | 0.007443 | 0.007302 | 0.007628 | 0.000326 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_scatters | 3363 | 0.080547 | 0.076495 | 0.083121 | 0.006626 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_unpack | 3363 | 0.013352 | 0.012890 | 0.013832 | 0.000942 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_alltoallv | 4083 | 0.046633 | 0.046380 | 0.047039 | 0.000659 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_clear | 4083 | 0.028927 | 0.028432 | 0.029335 | 0.000903 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_pack | 4083 | 0.007772 | 0.007702 | 0.007825 | 0.000123 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_scatterp | 4083 | 0.094360 | 0.094101 | 0.094635 | 0.000534 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_unpack | 4083 | 0.008119 | 0.008057 | 0.008144 | 0.000087 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.084957 | 0.081024 | 0.088633 | 0.007609 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.048873 | 0.046782 | 0.049890 | 0.003108 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.183988 | 0.180814 | 0.187772 | 0.006958 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.050001 | 0.049678 | 0.050591 | 0.000913 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.060162 | 0.051620 | 0.068045 | 0.016425 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.036146 | 0.036087 | 0.036245 | 0.000158 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.035231 | 0.033959 | 0.037008 | 0.003049 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.174108 | 0.167728 | 0.181051 | 0.013323 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.042359 | 0.041764 | 0.043012 | 0.001248 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.123476 | 0.061865 | 0.170223 | 0.108358 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_pack | 3362 | 0.004318 | 0.003773 | 0.005188 | 0.001415 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_scatters | 3362 | 0.134543 | 0.074032 | 0.180603 | 0.106571 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_unpack | 3362 | 0.004608 | 0.004446 | 0.004728 | 0.000282 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_alltoallv | 4082 | 0.077629 | 0.070317 | 0.084282 | 0.013965 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_clear | 4082 | 0.007905 | 0.006341 | 0.009722 | 0.003381 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_pack | 4082 | 0.005240 | 0.005042 | 0.005557 | 0.000515 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_scatterp | 4082 | 0.098565 | 0.094311 | 0.103653 | 0.009342 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_unpack | 4082 | 0.004925 | 0.004365 | 0.005921 | 0.001556 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.079589 | 0.050574 | 0.102397 | 0.051823 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.017208 | 0.016508 | 0.017739 | 0.001231 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.121747 | 0.093680 | 0.143364 | 0.049684 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.024836 | 0.024351 | 0.025514 | 0.001163 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.058963 | 0.047632 | 0.066748 | 0.019116 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.027942 | 0.026384 | 0.030313 | 0.003929 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.017446 | 0.016946 | 0.018655 | 0.001709 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.118463 | 0.108435 | 0.127825 | 0.019390 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.013949 | 0.011566 | 0.015160 | 0.003594 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 3364 | 0.095635 | 0.094023 | 0.097031 | 0.003008 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_pack | 3364 | 0.010110 | 0.009867 | 0.010292 | 0.000425 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_scatters | 3364 | 0.136385 | 0.133464 | 0.140201 | 0.006737 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_unpack | 3364 | 0.027295 | 0.025463 | 0.029854 | 0.004391 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_alltoallv | 4084 | 0.099975 | 0.099216 | 0.101491 | 0.002275 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_clear | 4084 | 0.031559 | 0.030876 | 0.032133 | 0.001257 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_pack | 4084 | 0.011864 | 0.011788 | 0.011985 | 0.000197 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_scatterp | 4084 | 0.159867 | 0.158587 | 0.161680 | 0.003093 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_unpack | 4084 | 0.012023 | 0.011922 | 0.012100 | 0.000178 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.088721 | 0.085890 | 0.092259 | 0.006369 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_pack | 64 | 0.022576 | 0.021783 | 0.023512 | 0.001729 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 64 | 0.138073 | 0.133918 | 0.142488 | 0.008570 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 64 | 0.026619 | 0.026081 | 0.026891 | 0.000810 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.073896 | 0.067723 | 0.079241 | 0.011518 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_clear | 56 | 0.027338 | 0.026936 | 0.028330 | 0.001394 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_pack | 56 | 0.018624 | 0.018125 | 0.019176 | 0.001051 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 56 | 0.137518 | 0.131928 | 0.143120 | 0.011192 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_unpack | 56 | 0.017478 | 0.016598 | 0.018844 | 0.002246 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.128233 | 0.079945 | 0.158077 | 0.078132 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 3362 | 0.006644 | 0.006088 | 0.007498 | 0.001410 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 3362 | 0.143701 | 0.095860 | 0.173766 | 0.077906 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 3362 | 0.005839 | 0.005235 | 0.006426 | 0.001191 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 4082 | 0.104087 | 0.098985 | 0.108709 | 0.009724 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 4082 | 0.010391 | 0.007990 | 0.019234 | 0.011244 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 4082 | 0.007661 | 0.005608 | 0.008699 | 0.003091 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 4082 | 0.134024 | 0.130986 | 0.136926 | 0.005940 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 4082 | 0.007813 | 0.007124 | 0.008830 | 0.001706 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.071646 | 0.054250 | 0.082338 | 0.028088 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 64 | 0.009637 | 0.009188 | 0.010139 | 0.000951 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 64 | 0.099813 | 0.082821 | 0.110224 | 0.027403 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 64 | 0.018422 | 0.018181 | 0.018846 | 0.000665 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.065200 | 0.049595 | 0.077640 | 0.028045 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 56 | 0.014478 | 0.013065 | 0.015925 | 0.002860 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 56 | 0.009347 | 0.009191 | 0.009639 | 0.000448 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 56 | 0.095434 | 0.081462 | 0.109673 | 0.028211 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 56 | 0.006294 | 0.005742 | 0.006811 | 0.001069 |
| large_rep02_np1_omp1 | PW_Basis_K | gatherp_scatters | 3366 | 0.024854 | 0.024854 | 0.024854 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_K | gathers_scatterp | 4086 | 0.082579 | 0.082579 | 0.082579 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.125911 | 0.125911 | 0.125911 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.207711 | 0.207711 | 0.207711 | 0.000000 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.063833 | 0.031487 | 0.096179 | 0.064692 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_pack | 3362 | 0.014177 | 0.013979 | 0.014375 | 0.000396 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_scatters | 3362 | 0.100964 | 0.069234 | 0.132694 | 0.063460 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_unpack | 3362 | 0.020687 | 0.020274 | 0.021100 | 0.000826 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_alltoallv | 4082 | 0.032053 | 0.025683 | 0.038423 | 0.012740 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_clear | 4082 | 0.043187 | 0.042246 | 0.044128 | 0.001882 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_pack | 4082 | 0.014000 | 0.013874 | 0.014127 | 0.000253 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_scatterp | 4082 | 0.110659 | 0.105662 | 0.115656 | 0.009994 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_unpack | 4082 | 0.018402 | 0.018149 | 0.018654 | 0.000505 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.120794 | 0.085533 | 0.156055 | 0.070522 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.071978 | 0.066508 | 0.077448 | 0.010940 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.278278 | 0.247284 | 0.309271 | 0.061987 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.085325 | 0.084116 | 0.086535 | 0.002419 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.077995 | 0.062733 | 0.093257 | 0.030524 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.047814 | 0.047287 | 0.048340 | 0.001053 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.063989 | 0.061179 | 0.066799 | 0.005620 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.255957 | 0.241215 | 0.270699 | 0.029484 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.065950 | 0.064197 | 0.067703 | 0.003506 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 3363 | 0.079145 | 0.052405 | 0.090155 | 0.037750 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_pack | 3363 | 0.007485 | 0.007399 | 0.007607 | 0.000208 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_scatters | 3363 | 0.102001 | 0.076822 | 0.112281 | 0.035459 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_unpack | 3363 | 0.013117 | 0.012203 | 0.014481 | 0.002278 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_alltoallv | 4083 | 0.056169 | 0.051653 | 0.058367 | 0.006714 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_clear | 4083 | 0.029817 | 0.029442 | 0.029990 | 0.000548 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_pack | 4083 | 0.007840 | 0.007734 | 0.008101 | 0.000367 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_scatterp | 4083 | 0.104946 | 0.101124 | 0.106945 | 0.005821 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_unpack | 4083 | 0.008136 | 0.008034 | 0.008359 | 0.000325 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.099810 | 0.068477 | 0.110880 | 0.042403 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.044996 | 0.044002 | 0.046572 | 0.002570 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.196124 | 0.165730 | 0.206683 | 0.040953 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.051153 | 0.050507 | 0.051839 | 0.001332 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.065468 | 0.062761 | 0.068002 | 0.005241 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.035961 | 0.034704 | 0.038751 | 0.004047 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.034733 | 0.034551 | 0.035068 | 0.000517 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.178626 | 0.174179 | 0.184596 | 0.010417 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.042251 | 0.041349 | 0.043135 | 0.001786 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.107225 | 0.061964 | 0.150850 | 0.088886 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_pack | 3362 | 0.004245 | 0.003823 | 0.004780 | 0.000957 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_scatters | 3362 | 0.118275 | 0.073434 | 0.161949 | 0.088515 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_unpack | 3362 | 0.004691 | 0.004479 | 0.005535 | 0.001056 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_alltoallv | 4082 | 0.072922 | 0.067782 | 0.078878 | 0.011096 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_clear | 4082 | 0.007820 | 0.006620 | 0.009238 | 0.002618 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_pack | 4082 | 0.005193 | 0.005061 | 0.005351 | 0.000290 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_scatterp | 4082 | 0.093600 | 0.089694 | 0.097881 | 0.008187 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_unpack | 4082 | 0.004857 | 0.004344 | 0.005516 | 0.001172 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.069540 | 0.051144 | 0.087306 | 0.036162 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.017954 | 0.016511 | 0.019489 | 0.002978 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.112388 | 0.095367 | 0.129203 | 0.033836 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.024778 | 0.024567 | 0.025080 | 0.000513 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.052150 | 0.045550 | 0.060968 | 0.015418 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.027780 | 0.025795 | 0.029383 | 0.003588 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.017434 | 0.016602 | 0.018135 | 0.001533 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.111267 | 0.103548 | 0.121282 | 0.017734 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.013747 | 0.012188 | 0.014960 | 0.002772 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 3364 | 0.078062 | 0.071785 | 0.082545 | 0.010760 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_pack | 3364 | 0.009215 | 0.008910 | 0.009350 | 0.000440 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_scatters | 3364 | 0.115528 | 0.109649 | 0.117953 | 0.008304 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_unpack | 3364 | 0.026080 | 0.023089 | 0.028688 | 0.005599 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_alltoallv | 4084 | 0.084503 | 0.082699 | 0.086738 | 0.004039 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_clear | 4084 | 0.024231 | 0.023228 | 0.025473 | 0.002245 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_pack | 4084 | 0.011387 | 0.010987 | 0.012394 | 0.001407 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_scatterp | 4084 | 0.133640 | 0.132774 | 0.134780 | 0.002006 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_unpack | 4084 | 0.010652 | 0.010489 | 0.010858 | 0.000369 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.072794 | 0.070434 | 0.078030 | 0.007596 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_pack | 64 | 0.024126 | 0.023021 | 0.025976 | 0.002955 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 64 | 0.123766 | 0.121231 | 0.128031 | 0.006800 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 64 | 0.026717 | 0.026301 | 0.026861 | 0.000560 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.068087 | 0.062763 | 0.074500 | 0.011737 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_clear | 56 | 0.029444 | 0.026423 | 0.035646 | 0.009223 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_pack | 56 | 0.019189 | 0.018667 | 0.019776 | 0.001109 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 56 | 0.133045 | 0.128990 | 0.138100 | 0.009110 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_unpack | 56 | 0.016170 | 0.015820 | 0.016802 | 0.000982 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.099100 | 0.076338 | 0.119403 | 0.043065 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_pack | 3362 | 0.006247 | 0.005903 | 0.006638 | 0.000735 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_scatters | 3362 | 0.113804 | 0.090386 | 0.134553 | 0.044167 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_unpack | 3362 | 0.005785 | 0.005367 | 0.006362 | 0.000995 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_alltoallv | 4082 | 0.104299 | 0.096302 | 0.107964 | 0.011662 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_clear | 4082 | 0.008852 | 0.008262 | 0.009365 | 0.001103 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_pack | 4082 | 0.007247 | 0.005905 | 0.008662 | 0.002757 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_scatterp | 4082 | 0.131378 | 0.121866 | 0.133644 | 0.011778 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_unpack | 4082 | 0.007330 | 0.006984 | 0.007872 | 0.000888 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.058642 | 0.049193 | 0.067855 | 0.018662 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_pack | 64 | 0.009859 | 0.009111 | 0.010626 | 0.001515 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 64 | 0.086710 | 0.078102 | 0.095294 | 0.017192 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 64 | 0.018108 | 0.017781 | 0.018476 | 0.000695 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.056798 | 0.045194 | 0.067598 | 0.022404 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_clear | 56 | 0.014181 | 0.012903 | 0.015200 | 0.002297 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_pack | 56 | 0.009176 | 0.008824 | 0.009551 | 0.000727 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 56 | 0.086565 | 0.076211 | 0.098615 | 0.022404 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_unpack | 56 | 0.006305 | 0.006019 | 0.006663 | 0.000644 |
| large_rep03_np1_omp1 | PW_Basis_K | gatherp_scatters | 3366 | 0.026439 | 0.026439 | 0.026439 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_K | gathers_scatterp | 4086 | 0.081234 | 0.081234 | 0.081234 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.124072 | 0.124072 | 0.124072 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.206612 | 0.206612 | 0.206612 | 0.000000 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.068833 | 0.031024 | 0.106641 | 0.075617 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_pack | 3362 | 0.014430 | 0.013700 | 0.015159 | 0.001459 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_scatters | 3362 | 0.106340 | 0.070274 | 0.142406 | 0.072132 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_unpack | 3362 | 0.020828 | 0.019914 | 0.021742 | 0.001828 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_alltoallv | 4082 | 0.034484 | 0.029503 | 0.039464 | 0.009961 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_clear | 4082 | 0.044761 | 0.044378 | 0.045145 | 0.000767 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_pack | 4082 | 0.014214 | 0.013820 | 0.014607 | 0.000787 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_scatterp | 4082 | 0.115275 | 0.111482 | 0.119067 | 0.007585 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_unpack | 4082 | 0.018559 | 0.018269 | 0.018849 | 0.000580 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.108375 | 0.068619 | 0.148131 | 0.079512 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.073154 | 0.068027 | 0.078280 | 0.010253 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.269576 | 0.236272 | 0.302879 | 0.066607 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.087859 | 0.086537 | 0.089181 | 0.002644 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.074263 | 0.062618 | 0.085909 | 0.023291 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.048272 | 0.046821 | 0.049722 | 0.002901 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.065129 | 0.061381 | 0.068876 | 0.007495 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.250296 | 0.246503 | 0.254090 | 0.007587 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.062429 | 0.059774 | 0.065085 | 0.005311 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 3363 | 0.079708 | 0.051512 | 0.093683 | 0.042171 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_pack | 3363 | 0.007494 | 0.007338 | 0.007650 | 0.000312 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_scatters | 3363 | 0.102786 | 0.074501 | 0.116950 | 0.042449 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_unpack | 3363 | 0.013362 | 0.013036 | 0.013713 | 0.000677 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_alltoallv | 4083 | 0.054743 | 0.050376 | 0.056797 | 0.006421 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_clear | 4083 | 0.030486 | 0.029627 | 0.031068 | 0.001441 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_pack | 4083 | 0.007830 | 0.007723 | 0.008081 | 0.000358 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_scatterp | 4083 | 0.104204 | 0.100448 | 0.106595 | 0.006147 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_unpack | 4083 | 0.008178 | 0.008067 | 0.008413 | 0.000346 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.098288 | 0.070145 | 0.115559 | 0.045414 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.045256 | 0.042799 | 0.047146 | 0.004347 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.195473 | 0.168424 | 0.210801 | 0.042377 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.051760 | 0.051322 | 0.052271 | 0.000949 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.067716 | 0.057130 | 0.084756 | 0.027626 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.036168 | 0.035168 | 0.037435 | 0.002267 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.034562 | 0.031785 | 0.036538 | 0.004753 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.181215 | 0.174874 | 0.194515 | 0.019641 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.042554 | 0.041054 | 0.043560 | 0.002506 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.125925 | 0.064830 | 0.174119 | 0.109289 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_pack | 3362 | 0.004286 | 0.003798 | 0.004889 | 0.001091 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_scatters | 3362 | 0.136924 | 0.076635 | 0.184791 | 0.108156 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_unpack | 3362 | 0.004580 | 0.004406 | 0.004692 | 0.000286 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_alltoallv | 4082 | 0.077028 | 0.070607 | 0.083954 | 0.013347 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_clear | 4082 | 0.008031 | 0.006794 | 0.009690 | 0.002896 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_pack | 4082 | 0.005295 | 0.005152 | 0.005560 | 0.000408 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_scatterp | 4082 | 0.098062 | 0.094052 | 0.103138 | 0.009086 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_unpack | 4082 | 0.004849 | 0.004366 | 0.005505 | 0.001139 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.079248 | 0.050818 | 0.100018 | 0.049200 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_pack | 64 | 0.017078 | 0.015957 | 0.017720 | 0.001763 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 64 | 0.121469 | 0.095162 | 0.141509 | 0.046347 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 64 | 0.025027 | 0.024411 | 0.026658 | 0.002247 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.058106 | 0.046443 | 0.067045 | 0.020602 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_clear | 56 | 0.027895 | 0.026111 | 0.030160 | 0.004049 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_pack | 56 | 0.017726 | 0.016752 | 0.018219 | 0.001467 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 56 | 0.117867 | 0.107691 | 0.128968 | 0.021277 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_unpack | 56 | 0.013980 | 0.012125 | 0.015226 | 0.003101 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 3364 | 0.098437 | 0.083555 | 0.107973 | 0.024418 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_pack | 3364 | 0.009451 | 0.009218 | 0.009590 | 0.000372 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_scatters | 3364 | 0.136403 | 0.122927 | 0.149121 | 0.026194 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_unpack | 3364 | 0.026151 | 0.022935 | 0.029758 | 0.006823 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_alltoallv | 4084 | 0.094284 | 0.091599 | 0.096970 | 0.005371 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_clear | 4084 | 0.025129 | 0.023451 | 0.027241 | 0.003790 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_pack | 4084 | 0.011223 | 0.011033 | 0.011412 | 0.000379 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_scatterp | 4084 | 0.144812 | 0.142821 | 0.147308 | 0.004487 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_unpack | 4084 | 0.010975 | 0.010643 | 0.011306 | 0.000663 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.077030 | 0.072660 | 0.081060 | 0.008400 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_pack | 64 | 0.024013 | 0.023418 | 0.025516 | 0.002098 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 64 | 0.128065 | 0.122989 | 0.131638 | 0.008649 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 64 | 0.026891 | 0.026539 | 0.027252 | 0.000713 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.067004 | 0.063907 | 0.072410 | 0.008503 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_clear | 56 | 0.027149 | 0.026699 | 0.027561 | 0.000862 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_pack | 56 | 0.019000 | 0.018234 | 0.019638 | 0.001404 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 56 | 0.129332 | 0.125777 | 0.135157 | 0.009380 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_unpack | 56 | 0.016032 | 0.015541 | 0.016476 | 0.000935 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 3362 | 0.134235 | 0.084019 | 0.192929 | 0.108910 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_pack | 3362 | 0.006550 | 0.005137 | 0.007576 | 0.002439 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_scatters | 3362 | 0.149351 | 0.100058 | 0.206049 | 0.105991 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_unpack | 3362 | 0.005754 | 0.005257 | 0.006192 | 0.000935 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_alltoallv | 4082 | 0.105762 | 0.099304 | 0.116078 | 0.016774 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_clear | 4082 | 0.008911 | 0.007594 | 0.010524 | 0.002930 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_pack | 4082 | 0.007333 | 0.005643 | 0.008624 | 0.002981 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_scatterp | 4082 | 0.133537 | 0.130474 | 0.138575 | 0.008101 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_unpack | 4082 | 0.007673 | 0.006131 | 0.008853 | 0.002722 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 64 | 0.077226 | 0.051300 | 0.091255 | 0.039955 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_pack | 64 | 0.009301 | 0.008732 | 0.010048 | 0.001316 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 64 | 0.105088 | 0.079115 | 0.118873 | 0.039758 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 64 | 0.018457 | 0.018245 | 0.018880 | 0.000635 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 56 | 0.055423 | 0.047460 | 0.062655 | 0.015195 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_clear | 56 | 0.014206 | 0.012953 | 0.015800 | 0.002847 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_pack | 56 | 0.009225 | 0.008809 | 0.009387 | 0.000578 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 56 | 0.085394 | 0.077112 | 0.093553 | 0.016441 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_unpack | 56 | 0.006429 | 0.005825 | 0.007083 | 0.001258 |

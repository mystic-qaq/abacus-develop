# Workflow B blocking communication benchmark

Interpretation:

- `comm_critical_s`: rank-critical blocking `MPI_Alltoallv` time, using max time across ranks.
- `wait_proxy_max_s`: rank skew proxy, computed as max(rank time) - min(rank time).
- `overlap_candidate_rank_avg_s`: local pack/unpack/clear work around the collective, averaged across ranks.
- `*_stdev`: sample standard deviation across repeated runs with the same scale and parallel configuration.

## Repeated Summary

| scale | np | omp | class | repeats | wall mean/s | comm critical mean/s | wait proxy mean/s | overlap candidate mean/s |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large | 1 | 1 | PW_Basis_K | 3 | 13.427 +/- 0.006 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 1 | 1 | PW_Basis_Sup | 3 | 13.427 +/- 0.006 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 | 0.000000 +/- 0.000000 |
| large | 2 | 1 | PW_Basis_K | 3 | 8.090 +/- 0.040 | 0.125598 +/- 0.004871 | 0.045897 +/- 0.005752 | 0.087775 +/- 0.001695 |
| large | 2 | 1 | PW_Basis_Sup | 3 | 8.090 +/- 0.040 | 0.170253 +/- 0.024118 | 0.080158 +/- 0.024481 | 0.131820 +/- 0.001237 |
| large | 4 | 1 | PW_Basis_K | 3 | 4.883 +/- 0.055 | 0.096835 +/- 0.017745 | 0.017171 +/- 0.012132 | 0.047420 +/- 0.000877 |
| large | 4 | 1 | PW_Basis_Sup | 3 | 4.883 +/- 0.055 | 0.124107 +/- 0.015040 | 0.043126 +/- 0.021157 | 0.078730 +/- 0.000190 |
| large | 4 | 2 | PW_Basis_K | 3 | 4.503 +/- 0.042 | 0.152594 +/- 0.011780 | 0.011421 +/- 0.004056 | 0.059066 +/- 0.000286 |
| large | 4 | 2 | PW_Basis_Sup | 3 | 4.503 +/- 0.042 | 0.131954 +/- 0.033556 | 0.042289 +/- 0.027221 | 0.057555 +/- 0.000139 |
| large | 8 | 1 | PW_Basis_K | 3 | 3.657 +/- 0.023 | 0.274241 +/- 0.010114 | 0.016369 +/- 0.002312 | 0.034663 +/- 0.000188 |
| large | 8 | 1 | PW_Basis_Sup | 3 | 3.657 +/- 0.023 | 0.117739 +/- 0.009424 | 0.030988 +/- 0.006128 | 0.050675 +/- 0.000421 |
| large | 8 | 2 | PW_Basis_K | 3 | 3.437 +/- 0.072 | 0.305513 +/- 0.017703 | 0.024140 +/- 0.009621 | 0.043633 +/- 0.000504 |
| large | 8 | 2 | PW_Basis_Sup | 3 | 3.437 +/- 0.072 | 0.121707 +/- 0.022635 | 0.054004 +/- 0.027836 | 0.038016 +/- 0.000065 |

## Per-Run Summary

| case | scale | rep | np | omp | class | wall/s | comm avg/s | comm critical/s | wait proxy/s | overlap candidate/s |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_K | 13.430 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np1_omp1 | large | 01 | 1 | 1 | PW_Basis_Sup | 13.430 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_K | 8.090 | 0.104210 | 0.128144 | 0.047867 | 0.089342 |
| large_rep01_np2_omp1 | large | 01 | 2 | 1 | PW_Basis_Sup | 8.090 | 0.121298 | 0.152518 | 0.062439 | 0.130479 |
| large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_K | 4.920 | 0.094841 | 0.101451 | 0.023586 | 0.047331 |
| large_rep01_np4_omp1 | large | 01 | 4 | 1 | PW_Basis_Sup | 4.920 | 0.110009 | 0.126865 | 0.048307 | 0.078948 |
| large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_K | 4.490 | 0.149257 | 0.154663 | 0.011777 | 0.059078 |
| large_rep01_np4_omp2 | large | 01 | 4 | 2 | PW_Basis_Sup | 4.490 | 0.133710 | 0.159253 | 0.061227 | 0.057576 |
| large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_K | 3.670 | 0.279584 | 0.285140 | 0.018301 | 0.034612 |
| large_rep01_np8_omp1 | large | 01 | 8 | 1 | PW_Basis_Sup | 3.670 | 0.113339 | 0.125043 | 0.037115 | 0.050639 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_K | 3.390 | 0.294485 | 0.302375 | 0.024950 | 0.043161 |
| large_rep01_np8_omp2 | large | 01 | 8 | 2 | PW_Basis_Sup | 3.390 | 0.098345 | 0.110595 | 0.046304 | 0.037961 |
| large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_K | 13.430 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep02_np1_omp1 | large | 02 | 1 | 1 | PW_Basis_Sup | 13.430 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_K | 8.050 | 0.100272 | 0.119982 | 0.039419 | 0.085976 |
| large_rep02_np2_omp1 | large | 02 | 2 | 1 | PW_Basis_Sup | 8.050 | 0.125555 | 0.160526 | 0.069943 | 0.132915 |
| large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_K | 4.820 | 0.075598 | 0.077238 | 0.003178 | 0.046592 |
| large_rep02_np4_omp1 | large | 02 | 4 | 1 | PW_Basis_Sup | 4.820 | 0.099185 | 0.107879 | 0.019860 | 0.078600 |
| large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_K | 4.470 | 0.136351 | 0.139916 | 0.007199 | 0.058775 |
| large_rep02_np4_omp2 | large | 02 | 4 | 2 | PW_Basis_Sup | 4.470 | 0.086934 | 0.094492 | 0.011095 | 0.057683 |
| large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_K | 3.670 | 0.268119 | 0.272426 | 0.016998 | 0.034871 |
| large_rep02_np8_omp1 | large | 02 | 8 | 1 | PW_Basis_Sup | 3.670 | 0.108626 | 0.121072 | 0.030990 | 0.051112 |
| large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_K | 3.400 | 0.282711 | 0.289589 | 0.014140 | 0.043575 |
| large_rep02_np8_omp2 | large | 02 | 8 | 2 | PW_Basis_Sup | 3.400 | 0.091007 | 0.106775 | 0.030829 | 0.038087 |
| large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_K | 13.420 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep03_np1_omp1 | large | 03 | 1 | 1 | PW_Basis_Sup | 13.420 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_K | 8.130 | 0.103466 | 0.128669 | 0.050405 | 0.088006 |
| large_rep03_np2_omp1 | large | 03 | 2 | 1 | PW_Basis_Sup | 8.130 | 0.143670 | 0.197716 | 0.108092 | 0.132065 |
| large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_K | 4.910 | 0.102212 | 0.111815 | 0.024748 | 0.048338 |
| large_rep03_np4_omp1 | large | 03 | 4 | 1 | PW_Basis_Sup | 4.910 | 0.115113 | 0.137578 | 0.061211 | 0.078642 |
| large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_K | 4.550 | 0.155253 | 0.163202 | 0.015287 | 0.059347 |
| large_rep03_np4_omp2 | large | 03 | 4 | 2 | PW_Basis_Sup | 4.550 | 0.127350 | 0.142118 | 0.054546 | 0.057406 |
| large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_K | 3.630 | 0.260335 | 0.265158 | 0.013807 | 0.034506 |
| large_rep03_np8_omp1 | large | 03 | 8 | 1 | PW_Basis_Sup | 3.630 | 0.095242 | 0.107101 | 0.024859 | 0.050273 |
| large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_K | 3.520 | 0.313592 | 0.324575 | 0.033330 | 0.044164 |
| large_rep03_np8_omp2 | large | 03 | 8 | 2 | PW_Basis_Sup | 3.520 | 0.121412 | 0.147751 | 0.084880 | 0.038000 |

## Detail

| case | class | timer | calls | avg_s | min_s | max_s | wait_proxy_max_s |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| large_rep01_np1_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.024853 | 0.024853 | 0.024853 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.058924 | 0.058924 | 0.058924 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.068127 | 0.068127 | 0.068127 | 0.000000 |
| large_rep01_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.071831 | 0.071831 | 0.071831 | 0.000000 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.056258 | 0.036554 | 0.075961 | 0.039407 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.012272 | 0.011983 | 0.012561 | 0.000578 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.093738 | 0.073396 | 0.114079 | 0.040683 |
| large_rep01_np2_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.020310 | 0.019323 | 0.021296 | 0.001973 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.047953 | 0.043723 | 0.052183 | 0.008460 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.027561 | 0.026368 | 0.028755 | 0.002387 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.014479 | 0.014161 | 0.014797 | 0.000636 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.111571 | 0.109262 | 0.113881 | 0.004619 |
| large_rep01_np2_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.014721 | 0.014324 | 0.015117 | 0.000793 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.062492 | 0.058148 | 0.066837 | 0.008689 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.036167 | 0.035789 | 0.036544 | 0.000755 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.133029 | 0.130651 | 0.135407 | 0.004756 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.034187 | 0.032602 | 0.035773 | 0.003171 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.058806 | 0.031931 | 0.085681 | 0.053750 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.018939 | 0.018410 | 0.019468 | 0.001058 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.028271 | 0.027640 | 0.028901 | 0.001261 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.119127 | 0.093746 | 0.144508 | 0.050762 |
| large_rep01_np2_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.012915 | 0.012580 | 0.013251 | 0.000671 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.048448 | 0.034264 | 0.053590 | 0.019326 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.007705 | 0.007564 | 0.007980 | 0.000416 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.068891 | 0.055739 | 0.073602 | 0.017863 |
| large_rep01_np4_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.007957 | 0.007720 | 0.008464 | 0.000744 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.046393 | 0.043601 | 0.047861 | 0.004260 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.012340 | 0.012138 | 0.012671 | 0.000533 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.009748 | 0.009354 | 0.010215 | 0.000861 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.084817 | 0.082821 | 0.086063 | 0.003242 |
| large_rep01_np4_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.009581 | 0.009360 | 0.009961 | 0.000601 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.055251 | 0.050264 | 0.058609 | 0.008345 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.018676 | 0.017862 | 0.019431 | 0.001569 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.096744 | 0.093761 | 0.100239 | 0.006478 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.022619 | 0.022011 | 0.024216 | 0.002205 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.054757 | 0.028294 | 0.068256 | 0.039962 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.013982 | 0.013669 | 0.014473 | 0.000804 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.014786 | 0.014554 | 0.015082 | 0.000528 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.092583 | 0.067284 | 0.105275 | 0.037991 |
| large_rep01_np4_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.008886 | 0.008628 | 0.009248 | 0.000620 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.135474 | 0.124603 | 0.138557 | 0.013954 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.005594 | 0.005390 | 0.006021 | 0.000631 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.152544 | 0.142534 | 0.155189 | 0.012655 |
| large_rep01_np8_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.006644 | 0.006424 | 0.006830 | 0.000406 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.144110 | 0.142236 | 0.146583 | 0.004347 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.007803 | 0.007644 | 0.008145 | 0.000501 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.007565 | 0.007303 | 0.007764 | 0.000461 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.173163 | 0.171088 | 0.175559 | 0.004471 |
| large_rep01_np8_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.007006 | 0.006820 | 0.007267 | 0.000447 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.064577 | 0.056823 | 0.071884 | 0.015061 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.007972 | 0.007807 | 0.008188 | 0.000381 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.093467 | 0.085530 | 0.100319 | 0.014789 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.020742 | 0.019807 | 0.022569 | 0.002762 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.048762 | 0.031105 | 0.053159 | 0.022054 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.012002 | 0.011317 | 0.012909 | 0.001592 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.005838 | 0.005593 | 0.006056 | 0.000463 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.070847 | 0.052579 | 0.075643 | 0.023064 |
| large_rep01_np8_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.004085 | 0.003813 | 0.004333 | 0.000520 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.071563 | 0.066431 | 0.075453 | 0.009022 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_pack | 8055 | 0.010349 | 0.010164 | 0.010568 | 0.000404 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_scatters | 8055 | 0.098078 | 0.092925 | 0.102279 | 0.009354 |
| large_rep01_np4_omp2 | PW_Basis_K | gatherp_unpack | 8055 | 0.011191 | 0.010957 | 0.011339 | 0.000382 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_alltoallv | 10167 | 0.077694 | 0.076455 | 0.079210 | 0.002755 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_clear | 10167 | 0.013926 | 0.013726 | 0.014131 | 0.000405 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_pack | 10167 | 0.011957 | 0.011577 | 0.012185 | 0.000608 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_scatterp | 10167 | 0.121993 | 0.120481 | 0.122707 | 0.002226 |
| large_rep01_np4_omp2 | PW_Basis_K | gathers_unpack | 10167 | 0.011655 | 0.011240 | 0.012047 | 0.000807 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.047510 | 0.046129 | 0.048916 | 0.002787 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_pack | 141 | 0.011324 | 0.011215 | 0.011451 | 0.000236 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 141 | 0.080237 | 0.079055 | 0.081544 | 0.002489 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 141 | 0.021240 | 0.020986 | 0.021504 | 0.000518 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.086201 | 0.051897 | 0.110337 | 0.058440 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_clear | 102 | 0.010541 | 0.009761 | 0.011224 | 0.001463 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_pack | 102 | 0.007505 | 0.007417 | 0.007593 | 0.000176 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 102 | 0.111352 | 0.076158 | 0.135898 | 0.059740 |
| large_rep01_np4_omp2 | PW_Basis_Sup | gathers_unpack | 102 | 0.006966 | 0.006847 | 0.007056 | 0.000209 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.140208 | 0.127211 | 0.146512 | 0.019301 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_pack | 8055 | 0.007433 | 0.007238 | 0.007840 | 0.000602 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_scatters | 8055 | 0.159932 | 0.146377 | 0.165793 | 0.019416 |
| large_rep01_np8_omp2 | PW_Basis_K | gatherp_unpack | 8055 | 0.007700 | 0.007527 | 0.008024 | 0.000497 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_alltoallv | 10167 | 0.154278 | 0.150214 | 0.155863 | 0.005649 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_clear | 10167 | 0.009634 | 0.009369 | 0.009967 | 0.000598 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_pack | 10167 | 0.009383 | 0.008942 | 0.009770 | 0.000828 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_scatterp | 10167 | 0.188760 | 0.183523 | 0.191054 | 0.007531 |
| large_rep01_np8_omp2 | PW_Basis_K | gathers_unpack | 10167 | 0.009011 | 0.008718 | 0.009382 | 0.000664 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.040487 | 0.031611 | 0.042567 | 0.010956 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_pack | 141 | 0.005258 | 0.004842 | 0.005469 | 0.000627 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 141 | 0.063760 | 0.053801 | 0.065729 | 0.011928 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 141 | 0.017888 | 0.017202 | 0.018726 | 0.001524 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.057858 | 0.032680 | 0.068028 | 0.035348 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_clear | 102 | 0.008185 | 0.007883 | 0.008467 | 0.000584 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_pack | 102 | 0.003221 | 0.003107 | 0.003265 | 0.000158 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 102 | 0.072784 | 0.047877 | 0.082823 | 0.034946 |
| large_rep01_np8_omp2 | PW_Basis_Sup | gathers_unpack | 102 | 0.003409 | 0.003295 | 0.003474 | 0.000179 |
| large_rep02_np1_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.025041 | 0.025041 | 0.025041 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.058596 | 0.058596 | 0.058596 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.067799 | 0.067799 | 0.067799 | 0.000000 |
| large_rep02_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.072032 | 0.072032 | 0.072032 | 0.000000 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.053331 | 0.037147 | 0.069516 | 0.032369 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.011963 | 0.011788 | 0.012137 | 0.000349 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.089457 | 0.073596 | 0.105319 | 0.031723 |
| large_rep02_np2_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.019446 | 0.019334 | 0.019559 | 0.000225 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.046941 | 0.043416 | 0.050466 | 0.007050 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.025812 | 0.025338 | 0.026285 | 0.000947 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.014278 | 0.014016 | 0.014539 | 0.000523 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.108230 | 0.105711 | 0.110748 | 0.005037 |
| large_rep02_np2_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.014478 | 0.014230 | 0.014726 | 0.000496 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.065424 | 0.057495 | 0.073353 | 0.015858 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.037081 | 0.036874 | 0.037288 | 0.000414 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.137904 | 0.128024 | 0.147783 | 0.019759 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.035211 | 0.033481 | 0.036941 | 0.003460 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.060131 | 0.033088 | 0.087173 | 0.054085 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.019293 | 0.018974 | 0.019612 | 0.000638 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.028234 | 0.028175 | 0.028294 | 0.000119 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.120944 | 0.094562 | 0.147325 | 0.052763 |
| large_rep02_np2_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.013096 | 0.012827 | 0.013365 | 0.000538 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.035344 | 0.034183 | 0.036622 | 0.002439 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.007539 | 0.007460 | 0.007665 | 0.000205 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.055398 | 0.054109 | 0.056319 | 0.002210 |
| large_rep02_np4_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.007817 | 0.007538 | 0.008095 | 0.000557 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.040253 | 0.039877 | 0.040616 | 0.000739 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.012192 | 0.011938 | 0.012670 | 0.000732 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.009552 | 0.009192 | 0.010109 | 0.000917 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.078113 | 0.077307 | 0.078882 | 0.001575 |
| large_rep02_np4_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.009491 | 0.009236 | 0.009718 | 0.000482 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.056371 | 0.053690 | 0.060437 | 0.006747 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.019021 | 0.017919 | 0.019870 | 0.001951 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.097516 | 0.093923 | 0.101011 | 0.007088 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.021942 | 0.021639 | 0.022132 | 0.000493 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.042814 | 0.034329 | 0.047442 | 0.013113 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.014022 | 0.013867 | 0.014286 | 0.000419 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.014898 | 0.014537 | 0.015132 | 0.000595 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.080623 | 0.072301 | 0.085618 | 0.013317 |
| large_rep02_np4_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.008717 | 0.008563 | 0.008867 | 0.000304 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.126361 | 0.115417 | 0.128758 | 0.013341 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.005506 | 0.005382 | 0.005941 | 0.000559 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.143319 | 0.133218 | 0.145281 | 0.012063 |
| large_rep02_np8_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.006624 | 0.006399 | 0.006742 | 0.000343 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.141758 | 0.140011 | 0.143668 | 0.003657 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.007867 | 0.007805 | 0.008062 | 0.000257 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.007688 | 0.007323 | 0.008031 | 0.000708 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.171222 | 0.169451 | 0.173022 | 0.003571 |
| large_rep02_np8_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.007186 | 0.006952 | 0.007452 | 0.000500 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.055636 | 0.051814 | 0.059693 | 0.007879 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.007929 | 0.007740 | 0.008448 | 0.000708 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.085347 | 0.081176 | 0.093626 | 0.012450 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.021613 | 0.019695 | 0.027953 | 0.008258 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.052990 | 0.038268 | 0.061379 | 0.023111 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.012051 | 0.011243 | 0.012904 | 0.001661 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.005350 | 0.005273 | 0.005430 | 0.000157 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.074715 | 0.059737 | 0.083026 | 0.023289 |
| large_rep02_np8_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.004169 | 0.003764 | 0.004404 | 0.000640 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.062649 | 0.060375 | 0.065051 | 0.004676 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_pack | 8055 | 0.010163 | 0.009991 | 0.010287 | 0.000296 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_scatters | 8055 | 0.088507 | 0.086193 | 0.090887 | 0.004694 |
| large_rep02_np4_omp2 | PW_Basis_K | gatherp_unpack | 8055 | 0.011147 | 0.010817 | 0.011378 | 0.000561 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_alltoallv | 10167 | 0.073702 | 0.072342 | 0.074865 | 0.002523 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_clear | 10167 | 0.013879 | 0.013783 | 0.014048 | 0.000265 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_pack | 10167 | 0.012002 | 0.011444 | 0.012684 | 0.001240 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_scatterp | 10167 | 0.117697 | 0.116302 | 0.118400 | 0.002098 |
| large_rep02_np4_omp2 | PW_Basis_K | gathers_unpack | 10167 | 0.011584 | 0.011238 | 0.011868 | 0.000630 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.046111 | 0.045284 | 0.046910 | 0.001626 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_pack | 141 | 0.011184 | 0.011029 | 0.011372 | 0.000343 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 141 | 0.078606 | 0.077750 | 0.079416 | 0.001666 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 141 | 0.021154 | 0.020792 | 0.021723 | 0.000931 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.040822 | 0.038113 | 0.047582 | 0.009469 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_clear | 102 | 0.010643 | 0.009906 | 0.011283 | 0.001377 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_pack | 102 | 0.007849 | 0.007669 | 0.008104 | 0.000435 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 102 | 0.066310 | 0.063650 | 0.073198 | 0.009548 |
| large_rep02_np4_omp2 | PW_Basis_Sup | gathers_unpack | 102 | 0.006853 | 0.006763 | 0.006926 | 0.000163 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.130187 | 0.124626 | 0.134914 | 0.010288 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_pack | 8055 | 0.007558 | 0.007212 | 0.007993 | 0.000781 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_scatters | 8055 | 0.150230 | 0.145361 | 0.154459 | 0.009098 |
| large_rep02_np8_omp2 | PW_Basis_K | gatherp_unpack | 8055 | 0.007776 | 0.007389 | 0.008055 | 0.000666 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_alltoallv | 10167 | 0.152524 | 0.150823 | 0.154675 | 0.003852 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_clear | 10167 | 0.009666 | 0.009157 | 0.010090 | 0.000933 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_pack | 10167 | 0.009447 | 0.009140 | 0.009736 | 0.000596 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_scatterp | 10167 | 0.187425 | 0.186087 | 0.188451 | 0.002364 |
| large_rep02_np8_omp2 | PW_Basis_K | gathers_unpack | 10167 | 0.009128 | 0.008735 | 0.009481 | 0.000746 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.034852 | 0.033840 | 0.036103 | 0.002263 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_pack | 141 | 0.005444 | 0.005375 | 0.005491 | 0.000116 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 141 | 0.058381 | 0.056336 | 0.059837 | 0.003501 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 141 | 0.017955 | 0.016868 | 0.018892 | 0.002024 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.056155 | 0.042106 | 0.070672 | 0.028566 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_clear | 102 | 0.008125 | 0.007852 | 0.008613 | 0.000761 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_pack | 102 | 0.003223 | 0.003164 | 0.003269 | 0.000105 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 102 | 0.070954 | 0.057372 | 0.085439 | 0.028067 |
| large_rep02_np8_omp2 | PW_Basis_Sup | gathers_unpack | 102 | 0.003341 | 0.003272 | 0.003401 | 0.000129 |
| large_rep03_np1_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.024555 | 0.024555 | 0.024555 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.057944 | 0.057944 | 0.057944 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.068235 | 0.068235 | 0.068235 | 0.000000 |
| large_rep03_np1_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.072652 | 0.072652 | 0.072652 | 0.000000 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.054639 | 0.034960 | 0.074318 | 0.039358 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.012141 | 0.011814 | 0.012467 | 0.000653 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.091250 | 0.072939 | 0.109560 | 0.036621 |
| large_rep03_np2_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.019708 | 0.018847 | 0.020569 | 0.001722 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.048827 | 0.043304 | 0.054351 | 0.011047 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.027469 | 0.026483 | 0.028455 | 0.001972 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.014180 | 0.013819 | 0.014541 | 0.000722 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.111658 | 0.107972 | 0.115344 | 0.007372 |
| large_rep03_np2_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.014509 | 0.014247 | 0.014771 | 0.000524 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.073767 | 0.057383 | 0.090152 | 0.032769 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.036719 | 0.036391 | 0.037047 | 0.000656 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.145387 | 0.130237 | 0.160537 | 0.030300 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.034717 | 0.033153 | 0.036282 | 0.003129 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.069903 | 0.032241 | 0.107564 | 0.075323 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.019303 | 0.019007 | 0.019598 | 0.000591 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.028145 | 0.028085 | 0.028206 | 0.000121 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.130714 | 0.093422 | 0.168006 | 0.074584 |
| large_rep03_np2_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.013181 | 0.013178 | 0.013183 | 0.000005 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.055865 | 0.043619 | 0.064154 | 0.020535 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.007680 | 0.007509 | 0.007811 | 0.000302 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.076338 | 0.064875 | 0.084314 | 0.019439 |
| large_rep03_np4_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.007986 | 0.007773 | 0.008408 | 0.000635 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.046348 | 0.043448 | 0.047661 | 0.004213 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.012984 | 0.012716 | 0.013178 | 0.000462 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.010181 | 0.009634 | 0.010641 | 0.001007 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.085740 | 0.083361 | 0.086957 | 0.003596 |
| large_rep03_np4_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.009507 | 0.009247 | 0.009957 | 0.000710 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.059942 | 0.047674 | 0.070059 | 0.022385 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.018210 | 0.018003 | 0.018324 | 0.000321 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.101391 | 0.090833 | 0.110699 | 0.019866 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.023040 | 0.022193 | 0.024952 | 0.002759 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.055171 | 0.028693 | 0.067519 | 0.038826 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.013758 | 0.013425 | 0.014484 | 0.001059 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.014841 | 0.014664 | 0.015150 | 0.000486 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.092748 | 0.067832 | 0.104510 | 0.036678 |
| large_rep03_np4_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.008793 | 0.008610 | 0.009291 | 0.000681 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.119014 | 0.112979 | 0.120582 | 0.007603 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_pack | 8055 | 0.005448 | 0.005342 | 0.005526 | 0.000184 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_scatters | 8055 | 0.135855 | 0.130034 | 0.136990 | 0.006956 |
| large_rep03_np8_omp1 | PW_Basis_K | gatherp_unpack | 8055 | 0.006594 | 0.006383 | 0.006727 | 0.000344 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_alltoallv | 10167 | 0.141320 | 0.138372 | 0.144576 | 0.006204 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_clear | 10167 | 0.007829 | 0.007746 | 0.008180 | 0.000434 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_pack | 10167 | 0.007579 | 0.007317 | 0.007784 | 0.000467 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_scatterp | 10167 | 0.170426 | 0.168047 | 0.173487 | 0.005440 |
| large_rep03_np8_omp1 | PW_Basis_K | gathers_unpack | 10167 | 0.007055 | 0.006807 | 0.007380 | 0.000573 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.054846 | 0.050210 | 0.058791 | 0.008581 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_pack | 141 | 0.008014 | 0.007736 | 0.008411 | 0.000675 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_scatters | 141 | 0.083462 | 0.079165 | 0.087066 | 0.007901 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gatherp_unpack | 141 | 0.020432 | 0.019694 | 0.021053 | 0.001359 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.040396 | 0.032032 | 0.048310 | 0.016278 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_clear | 102 | 0.012184 | 0.011475 | 0.012898 | 0.001423 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_pack | 102 | 0.005568 | 0.005318 | 0.005717 | 0.000399 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_scatterp | 102 | 0.062387 | 0.053926 | 0.070091 | 0.016165 |
| large_rep03_np8_omp1 | PW_Basis_Sup | gathers_unpack | 102 | 0.004075 | 0.003864 | 0.004188 | 0.000324 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.074956 | 0.068913 | 0.081759 | 0.012846 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_pack | 8055 | 0.010145 | 0.009787 | 0.010445 | 0.000658 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_scatters | 8055 | 0.101230 | 0.095164 | 0.107398 | 0.012234 |
| large_rep03_np4_omp2 | PW_Basis_K | gatherp_unpack | 8055 | 0.011442 | 0.011013 | 0.012324 | 0.001311 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_alltoallv | 10167 | 0.080296 | 0.079002 | 0.081443 | 0.002441 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_clear | 10167 | 0.013851 | 0.013560 | 0.014218 | 0.000658 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_pack | 10167 | 0.012171 | 0.011642 | 0.012514 | 0.000872 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_scatterp | 10167 | 0.124661 | 0.123040 | 0.125812 | 0.002772 |
| large_rep03_np4_omp2 | PW_Basis_K | gathers_unpack | 10167 | 0.011738 | 0.011472 | 0.011933 | 0.000461 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.050824 | 0.048772 | 0.051969 | 0.003197 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_pack | 141 | 0.011082 | 0.010822 | 0.011301 | 0.000479 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_scatters | 141 | 0.083309 | 0.081680 | 0.084240 | 0.002560 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gatherp_unpack | 141 | 0.021245 | 0.020814 | 0.021688 | 0.000874 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.076526 | 0.038800 | 0.090149 | 0.051349 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_clear | 102 | 0.010627 | 0.009820 | 0.011348 | 0.001528 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_pack | 102 | 0.007587 | 0.007310 | 0.007839 | 0.000529 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_scatterp | 102 | 0.101750 | 0.062814 | 0.115312 | 0.052498 |
| large_rep03_np4_omp2 | PW_Basis_Sup | gathers_unpack | 102 | 0.006866 | 0.006746 | 0.006972 | 0.000226 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_alltoallv | 8055 | 0.145513 | 0.129923 | 0.150951 | 0.021028 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_pack | 8055 | 0.007692 | 0.007222 | 0.008623 | 0.001401 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_scatters | 8055 | 0.165817 | 0.152514 | 0.170375 | 0.017861 |
| large_rep03_np8_omp2 | PW_Basis_K | gatherp_unpack | 8055 | 0.007772 | 0.007429 | 0.008323 | 0.000894 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_alltoallv | 10167 | 0.168079 | 0.161322 | 0.173624 | 0.012302 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_clear | 10167 | 0.009737 | 0.009246 | 0.010454 | 0.001208 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_pack | 10167 | 0.009704 | 0.009135 | 0.011103 | 0.001968 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_scatterp | 10167 | 0.203547 | 0.201403 | 0.207566 | 0.006163 |
| large_rep03_np8_omp2 | PW_Basis_K | gathers_unpack | 10167 | 0.009258 | 0.008722 | 0.010333 | 0.001611 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_alltoallv | 141 | 0.036694 | 0.034863 | 0.037409 | 0.002546 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_pack | 141 | 0.005287 | 0.005225 | 0.005369 | 0.000144 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_scatters | 141 | 0.060012 | 0.058355 | 0.060773 | 0.002418 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gatherp_unpack | 141 | 0.017902 | 0.016988 | 0.018717 | 0.001729 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_alltoallv | 102 | 0.084717 | 0.028008 | 0.110342 | 0.082334 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_clear | 102 | 0.008197 | 0.007676 | 0.008684 | 0.001008 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_pack | 102 | 0.003244 | 0.003139 | 0.003322 | 0.000183 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_scatterp | 102 | 0.099640 | 0.043214 | 0.125323 | 0.082109 |
| large_rep03_np8_omp2 | PW_Basis_Sup | gathers_unpack | 102 | 0.003370 | 0.003312 | 0.003465 | 0.000153 |

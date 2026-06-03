/**
 * @file bench_simd_copy.cpp
 * @brief Standalone micro-benchmark: SIMD intrinsics vs auto-vectorized copy.
 *
 * Build:
 *   cd source
 *   g++ -std=c++14 -O2 -fopenmp -mavx2 -I. \
 *       source_basis/module_pw/test/bench_simd_copy.cpp -o /tmp/bench_simd_copy
 *
 * Run:
 *   OMP_NUM_THREADS=4 /tmp/bench_simd_copy
 */

#include <complex>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <omp.h>

#include "source_basis/module_pw/pw_simd_copy.h"

// -----------------------------------------------------------------------
// Baseline: the OLD auto-vectorized copy
// -----------------------------------------------------------------------
template <typename T>
static void old_copy(T* __restrict__ d, const T* __restrict__ s, int n)
{
#ifdef __GNUC__
#pragma GCC ivdep
#endif
    for (int i = 0; i < n; ++i) d[i] = s[i];
}

// -----------------------------------------------------------------------
// Benchmark: each thread copies many independent slices so that total
// work dominates OpenMP fork/join overhead.
// -----------------------------------------------------------------------
template <typename T>
struct BenchResult
{
    double old_ns_per_elem;
    double new_ns_per_elem;
    double speedup;
    bool   correct;
};

template <typename T>
BenchResult<T> bench(int n_complex, int n_repeats, int n_threads)
{
    const int n_scalars = 2 * n_complex;
    const int n_slices  = 256; // independent slices per thread to fill cache

    // One big contiguous buffer per method so we can batch-copy
    std::vector<T> src(n_scalars * n_slices);
    std::vector<T> dst_old(n_scalars * n_slices);
    std::vector<T> dst_new(n_scalars * n_slices);

    for (int i = 0; i < n_scalars * n_slices; ++i)
        src[i] = static_cast<T>(i & 0xFF);

    // Warm up
    for (int s = 0; s < n_slices; ++s)
    {
        T* d = dst_old.data() + s * n_scalars;
        const T* sr = src.data() + s * n_scalars;
        old_copy(d, sr, n_scalars);
        ModulePW::simd_copy_n(dst_new.data() + s * n_scalars, sr, n_scalars);
    }

    const double total_elems = static_cast<double>(n_scalars) * n_slices * n_repeats;

    omp_set_num_threads(n_threads);

    // --- OLD ---
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < n_repeats; ++r)
    {
        #pragma omp parallel for schedule(static)
        for (int s = 0; s < n_slices; ++s)
        {
            T* d = dst_old.data() + s * n_scalars;
            const T* sr = src.data() + s * n_scalars;
            old_copy(d, sr, n_scalars);
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    // --- NEW ---
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < n_repeats; ++r)
    {
        #pragma omp parallel for schedule(static)
        for (int s = 0; s < n_slices; ++s)
        {
            T* d = dst_new.data() + s * n_scalars;
            const T* sr = src.data() + s * n_scalars;
            ModulePW::simd_copy_n(d, sr, n_scalars);
        }
    }
    auto t3 = std::chrono::high_resolution_clock::now();

    double old_ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
    double new_ns = std::chrono::duration<double, std::nano>(t3 - t2).count();

    // Verify
    bool ok = true;
    for (int i = 0; i < n_scalars * n_slices && ok; ++i)
        ok = (dst_old[i] == dst_new[i]);

    return {
        old_ns / total_elems,
        new_ns / total_elems,
        old_ns / new_ns,
        ok
    };
}

int main()
{
    int n_repeats = 500;
    int n_threads = omp_get_max_threads();
    if (n_threads > 8) n_threads = 8; // cap for meaningful comparison

    // Representative FFT inner dimensions
    int sizes[] = {32, 64, 96, 128, 192, 256, 384, 512, 768};
    constexpr int n_sizes = sizeof(sizes) / sizeof(sizes[0]);

    std::cout << "SIMD copy benchmark | repeats=" << n_repeats
              << " | slices=" << 256 << " | threads=" << n_threads << "\n\n";

    auto run = [&](const char* label) {
        std::cout << "--- " << label << " (" << n_threads << " threads) ---\n";
        std::cout << std::setw(10) << "n_cplx"
                  << std::setw(14) << "old ns/elem"
                  << std::setw(14) << "new ns/elem"
                  << std::setw(10) << "speedup"
                  << "\n";
        double geo_speedup = 1.0;
        int    n_valid = 0;
        for (int i = 0; i < n_sizes; ++i)
        {
            auto r = bench<float>(sizes[i], n_repeats, n_threads);
            if (!r.correct) std::cerr << "  [CORRECTNESS FAIL]\n";
            std::cout << std::setw(10) << sizes[i]
                      << std::setw(14) << std::fixed << std::setprecision(3) << r.old_ns_per_elem
                      << std::setw(14) << std::fixed << std::setprecision(3) << r.new_ns_per_elem
                      << std::setw(9)  << std::fixed << std::setprecision(2) << r.speedup << "x"
                      << "\n";
            if (r.correct && r.speedup > 0) { geo_speedup *= r.speedup; ++n_valid; }
        }
        if (n_valid > 0)
            std::cout << "  => geometric-mean speedup: "
                      << std::fixed << std::setprecision(2)
                      << std::pow(geo_speedup, 1.0 / n_valid) << "x\n\n";
    };

    // Float first
    run("std::complex<float>");

    // Then double
    auto run_dbl = [&]() {
        std::cout << "--- std::complex<double> (" << n_threads << " threads) ---\n";
        std::cout << std::setw(10) << "n_cplx"
                  << std::setw(14) << "old ns/elem"
                  << std::setw(14) << "new ns/elem"
                  << std::setw(10) << "speedup"
                  << "\n";
        double geo = 1.0;
        int nv = 0;
        for (int i = 0; i < n_sizes; ++i)
        {
            auto r = bench<double>(sizes[i], n_repeats, n_threads);
            if (!r.correct) std::cerr << "  [CORRECTNESS FAIL]\n";
            std::cout << std::setw(10) << sizes[i]
                      << std::setw(14) << std::fixed << std::setprecision(3) << r.old_ns_per_elem
                      << std::setw(14) << std::fixed << std::setprecision(3) << r.new_ns_per_elem
                      << std::setw(9)  << std::fixed << std::setprecision(2) << r.speedup << "x"
                      << "\n";
            if (r.correct && r.speedup > 0) { geo *= r.speedup; ++nv; }
        }
        if (nv > 0)
            std::cout << "  => geometric-mean speedup: "
                      << std::fixed << std::setprecision(2)
                      << std::pow(geo, 1.0 / nv) << "x\n";
    };
    run_dbl();

    return 0;
}

#include "../pw_basis.h"
#ifdef __MPI
#include "mpi.h"
#include "source_base/parallel_global.h"
#include "test_tool.h"
#endif
#include "pw_test.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <iostream>
#include <vector>

namespace
{
std::vector<int> thread_counts_to_check()
{
    std::vector<int> counts(1, 1);
#ifdef _OPENMP
    const int max_threads = omp_get_max_threads();
    if (max_threads >= 2)
    {
        counts.push_back(2);
    }
    if (max_threads > 2)
    {
        counts.push_back(max_threads);
    }
#endif
    return counts;
}

void set_omp_threads(const int nthread)
{
#ifdef _OPENMP
    omp_set_num_threads(nthread);
#else
    (void)nthread;
#endif
}

template <typename T>
T deterministic_real_value(const int i)
{
    return static_cast<T>(0.13 * std::sin(0.17 * i) + 0.07 * std::cos(0.11 * i));
}

template <typename T>
std::complex<T> deterministic_complex_value(const int i)
{
    return std::complex<T>(deterministic_real_value<T>(i), static_cast<T>(0.19 * std::cos(0.23 * i)));
}

template <typename T>
void expect_complex_vectors_near(const std::vector<std::complex<T>>& ref,
                                 const std::vector<std::complex<T>>& val,
                                 const T tol)
{
    ASSERT_EQ(ref.size(), val.size());
    for (std::size_t i = 0; i < ref.size(); ++i)
    {
        EXPECT_NEAR(ref[i].real(), val[i].real(), tol) << "real mismatch at " << i;
        EXPECT_NEAR(ref[i].imag(), val[i].imag(), tol) << "imag mismatch at " << i;
    }
}

template <typename T>
void expect_real_vectors_near(const std::vector<T>& ref, const std::vector<T>& val, const T tol)
{
    ASSERT_EQ(ref.size(), val.size());
    for (std::size_t i = 0; i < ref.size(); ++i)
    {
        EXPECT_NEAR(ref[i], val[i], tol) << "mismatch at " << i;
    }
}

void init_pw_basis(ModulePW::PW_Basis& pwtest, const bool gamma_only)
{
#ifdef __MPI
    pwtest.initmpi(nproc_in_pool, rank_in_pool, POOL_WORLD);
#endif
    ModuleBase::Matrix3 latvec(1.0, 0.1, 0.0, 0.0, 1.0, 0.2, 0.0, 0.0, 1.0);
    pwtest.initgrids(7.0, latvec, 18, 20, 22);
    pwtest.initparameters(gamma_only, 12.0, 1, true);
    pwtest.setuptransform();
}
} // namespace

TEST_F(PWTEST, transform_omp_threads_complex_roundtrip_consistency)
{
    std::cout << "FFT transform OpenMP consistency test for complex real2recip/recip2real" << std::endl;
    ModulePW::PW_Basis pwtest(device_flag, precision_flag);
    init_pw_basis(pwtest, false);

    std::vector<std::complex<double>> real_in(pwtest.nrxx);
    for (int i = 0; i < pwtest.nrxx; ++i)
    {
        real_in[i] = deterministic_complex_value<double>(i);
    }

    std::vector<std::complex<double>> recip_ref(pwtest.npw);
    std::vector<std::complex<double>> real_ref(pwtest.nrxx);

    set_omp_threads(1);
    pwtest.real2recip(real_in.data(), recip_ref.data());
    pwtest.recip2real(recip_ref.data(), real_ref.data());

    for (const int nthread : thread_counts_to_check())
    {
        set_omp_threads(nthread);
        std::vector<std::complex<double>> recip_val(pwtest.npw);
        std::vector<std::complex<double>> real_val(pwtest.nrxx);
        pwtest.real2recip(real_in.data(), recip_val.data());
        pwtest.recip2real(recip_val.data(), real_val.data());

        expect_complex_vectors_near(recip_ref, recip_val, 1.0e-10);
        expect_complex_vectors_near(real_ref, real_val, 1.0e-10);
    }
}

TEST_F(PWTEST, transform_omp_threads_real_gamma_and_add_consistency)
{
    std::cout << "FFT transform OpenMP consistency test for real gamma-only add/non-add paths" << std::endl;
    ModulePW::PW_Basis pwtest(device_flag, precision_flag);
    init_pw_basis(pwtest, true);

    std::vector<double> real_in(pwtest.nrxx);
    for (int i = 0; i < pwtest.nrxx; ++i)
    {
        real_in[i] = deterministic_real_value<double>(i);
    }

    std::vector<std::complex<double>> recip_ref(pwtest.npw);
    std::vector<std::complex<double>> recip_add_ref(pwtest.npw, std::complex<double>(0.25, -0.125));
    std::vector<double> real_ref(pwtest.nrxx);
    std::vector<double> real_add_ref(pwtest.nrxx, 0.5);

    set_omp_threads(1);
    pwtest.real2recip(real_in.data(), recip_ref.data());
    pwtest.real2recip(real_in.data(), recip_add_ref.data(), true, 0.75);
    pwtest.recip2real(recip_ref.data(), real_ref.data());
    pwtest.recip2real(recip_ref.data(), real_add_ref.data(), true, 0.5);

    for (const int nthread : thread_counts_to_check())
    {
        set_omp_threads(nthread);
        std::vector<std::complex<double>> recip_val(pwtest.npw);
        std::vector<std::complex<double>> recip_add_val(pwtest.npw, std::complex<double>(0.25, -0.125));
        std::vector<double> real_val(pwtest.nrxx);
        std::vector<double> real_add_val(pwtest.nrxx, 0.5);

        pwtest.real2recip(real_in.data(), recip_val.data());
        pwtest.real2recip(real_in.data(), recip_add_val.data(), true, 0.75);
        pwtest.recip2real(recip_val.data(), real_val.data());
        pwtest.recip2real(recip_val.data(), real_add_val.data(), true, 0.5);

        expect_complex_vectors_near(recip_ref, recip_val, 1.0e-10);
        expect_complex_vectors_near(recip_add_ref, recip_add_val, 1.0e-10);
        expect_real_vectors_near(real_ref, real_val, 1.0e-10);
        expect_real_vectors_near(real_add_ref, real_add_val, 1.0e-10);
    }
}

TEST_F(PWTEST, DISABLED_transform_omp_speedup_report)
{
    std::cout << "Disabled performance helper: run with --gtest_also_run_disabled_tests to compare OpenMP speedup"
              << std::endl;
    ModulePW::PW_Basis pwtest(device_flag, precision_flag);
    init_pw_basis(pwtest, false);
    std::vector<std::complex<double>> real_in(pwtest.nrxx);
    std::vector<std::complex<double>> recip_out(pwtest.npw);
    std::vector<std::complex<double>> real_out(pwtest.nrxx);
    for (int i = 0; i < pwtest.nrxx; ++i)
    {
        real_in[i] = deterministic_complex_value<double>(i);
    }

    constexpr int nrepeat = 20;
    double t1 = 0.0;
    for (const int nthread : thread_counts_to_check())
    {
        set_omp_threads(nthread);
        const auto tstart = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < nrepeat; ++i)
        {
            pwtest.real2recip(real_in.data(), recip_out.data());
            pwtest.recip2real(recip_out.data(), real_out.data());
        }
        const auto tend = std::chrono::high_resolution_clock::now();
        const double elapsed = std::chrono::duration<double>(tend - tstart).count();
        if (nthread == 1)
        {
            t1 = elapsed;
        }
        const double speedup = t1 > 0.0 ? t1 / elapsed : 1.0;
        const double efficiency = speedup / static_cast<double>(std::max(1, nthread));
        std::cout << "threads=" << nthread << " time=" << elapsed << "s speedup=" << speedup
                  << " efficiency=" << efficiency << std::endl;
    }
}

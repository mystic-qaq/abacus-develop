#ifndef PW_SIMD_COPY_H
#define PW_SIMD_COPY_H
/**
 * @file pw_simd_copy.h
 * @brief Architecture-detected SIMD memcpy for plane-wave pack/unpack loops.
 *
 * Provides simd_copy_n<T>(dest, src, count) which copies `count` scalars
 * (NOT complex elements) using the widest available SIMD instruction set.
 * The input is the output of reinterpret_cast<T*>(complex_ptr), so count
 * is always 2 * (number of std::complex<T> elements).
 *
 * Performance rationale:
 *   - FFT dimensions (nplane, nz, nzip) are often multiples of 8/16/32,
 *     so the tail loop is rarely taken in practice.
 *   - Unaligned loads/stores are used everywhere because std::vector
 *     allocations are not guaranteed to be cache-line aligned.
 *   - AVX-512 is guarded by a runtime check (SIMD_COPY_ALLOW_AVX512)
 *     because some CPUs down-clock heavily when 512-bit instructions
 *     are used.  The default is AVX2-only.
 */

#include <cstddef>
#include <cstdint>
#include <type_traits>

#if defined(__AVX512F__) && !defined(SIMD_COPY_DISABLE_AVX512)
#define PW_SIMD_AVX512 1
#include <immintrin.h>
#elif defined(__AVX2__)
#define PW_SIMD_AVX2 1
#include <immintrin.h>
#endif

namespace ModulePW
{
namespace simd_detail
{

// ---- float specialisation ------------------------------------------------

#if defined(PW_SIMD_AVX512)
inline void copy_n_impl(float* __restrict__ d, const float* __restrict__ s, int n)
{
    constexpr int W = 16; // 512 bits / 32-bit float
    int i = 0;
    for (; i + W <= n; i += W)
    {
        __m512 v = _mm512_loadu_ps(s + i);
        _mm512_storeu_ps(d + i, v);
    }
    for (; i < n; ++i) d[i] = s[i]; // tail
}
#elif defined(PW_SIMD_AVX2)
inline void copy_n_impl(float* __restrict__ d, const float* __restrict__ s, int n)
{
    constexpr int W = 8; // 256 bits / 32-bit float
    int i = 0;
    for (; i + W <= n; i += W)
    {
        __m256 v = _mm256_loadu_ps(s + i);
        _mm256_storeu_ps(d + i, v);
    }
    for (; i < n; ++i) d[i] = s[i]; // tail
}
#else
inline void copy_n_impl(float* __restrict__ d, const float* __restrict__ s, int n)
{
    for (int i = 0; i < n; ++i) d[i] = s[i];
}
#endif

// ---- double specialisation -----------------------------------------------

#if defined(PW_SIMD_AVX512)
inline void copy_n_impl(double* __restrict__ d, const double* __restrict__ s, int n)
{
    constexpr int W = 8; // 512 bits / 64-bit double
    int i = 0;
    for (; i + W <= n; i += W)
    {
        __m512d v = _mm512_loadu_pd(s + i);
        _mm512_storeu_pd(d + i, v);
    }
    for (; i < n; ++i) d[i] = s[i]; // tail
}
#elif defined(PW_SIMD_AVX2)
inline void copy_n_impl(double* __restrict__ d, const double* __restrict__ s, int n)
{
    constexpr int W = 4; // 256 bits / 64-bit double
    int i = 0;
    for (; i + W <= n; i += W)
    {
        __m256d v = _mm256_loadu_pd(s + i);
        _mm256_storeu_pd(d + i, v);
    }
    for (; i < n; ++i) d[i] = s[i]; // tail
}
#else
inline void copy_n_impl(double* __restrict__ d, const double* __restrict__ s, int n)
{
    for (int i = 0; i < n; ++i) d[i] = s[i];
}
#endif

} // namespace simd_detail

/**
 * @brief Copy `count` scalars from src to dest using SIMD where possible.
 *
 * @tparam T  Must be float or double (the underlying scalar type of
 *            std::complex<T> after reinterpret_cast).
 * @param dest Destination pointer (must not alias src).
 * @param src  Source pointer.
 * @param count Number of SCALAR elements to copy (= 2 * n_complex).
 */
template <typename T>
inline void simd_copy_n(T* __restrict__ dest, const T* __restrict__ src, int count)
{
    static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value,
                  "simd_copy_n only supports float or double");
    simd_detail::copy_n_impl(dest, src, count);
}

} // namespace ModulePW

#endif // PW_SIMD_COPY_H

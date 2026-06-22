#include "pw_basis.h"
#include "pw_simd_copy.h"
#include "source_base/global_function.h"
#include "source_base/timer.h"
#include <algorithm>
#include <type_traits>

namespace ModulePW
{
namespace detail
{
template <typename T>
inline void copy_complex_buffer(const std::complex<T>* in, std::complex<T>* out, const int count)
{
    if (count <= 0)
    {
        return;
    }

    std::copy_n(in, count, out);
}

template <typename T>
inline void copy_complex_buffer_parallel(const std::complex<T>* in, std::complex<T>* out, const int count)
{
    constexpr int chunk_size = 1024;
    if (count <= chunk_size)
    {
        copy_complex_buffer(in, out, count);
        return;
    }

#ifdef _OPENMP
#pragma omp parallel for schedule(static)
    for (int offset = 0; offset < count; offset += chunk_size)
    {
        const int chunk_count = std::min(chunk_size, count - offset);
        std::copy_n(in + offset, chunk_count, out + offset);
    }
#else
    copy_complex_buffer(in, out, count);
#endif
}

#ifdef __MPI
template <typename T>
inline MPI_Datatype mpi_complex_dtype()
{
    return MPI_DATATYPE_NULL;
}
template <>
inline MPI_Datatype mpi_complex_dtype<double>()
{
    return MPI_DOUBLE_COMPLEX;
}
template <>
inline MPI_Datatype mpi_complex_dtype<float>()
{
    return MPI_COMPLEX;
}
#endif
} // namespace detail

/**
 * @brief gather planes and scatter sticks
 * @param in: (nplane,fftny,fftnx)
 * @param out: (nz,nst)
 * @note in and out should be in different places
 * @note in[] will be changed
 */
template <typename T>
void PW_Basis::gatherp_scatters(std::complex<T>* in, std::complex<T>* out) const
{
    ModuleBase::timer::start(this->classname, "gatherp_scatters");

    if (this->poolnproc == 1)
    {
        const int nst_ = this->nst;
        const int nz_ = this->nz;
        const int* istot2ixy_ = this->istot2ixy;
        ModuleBase::timer::start(this->classname, "gatherp_copy_serial");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < nst_; ++is)
        {
            const int ixy = istot2ixy_[is];
            std::complex<T>* outp = &out[is * nz_];
            const std::complex<T>* inp = &in[ixy * nz_];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nz_);
        }
        ModuleBase::timer::end(this->classname, "gatherp_copy_serial");
        ModuleBase::timer::end(this->classname, "gatherp_scatters");
        return;
    }

#ifdef __MPI
    const int nstot_gps = this->nstot;
    const int nplane_gps = this->nplane;
    const int* istot2ixy_gps = this->istot2ixy;

    ModuleBase::timer::start(this->classname, "gatherp_pack");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int istot = 0; istot < nstot_gps; ++istot)
    {
        const int ixy = istot2ixy_gps[istot];
        std::complex<T>* outp = &out[istot * nplane_gps];
        const std::complex<T>* inp = &in[ixy * nplane_gps];
        ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                              reinterpret_cast<const T*>(inp),
                              2 * nplane_gps);
    }
    ModuleBase::timer::end(this->classname, "gatherp_pack");

    const MPI_Datatype mpi_type = detail::mpi_complex_dtype<T>();
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value,
                  "PW_Basis::gatherp_scatters only supports float or double");

    ModuleBase::timer::start(this->classname, "gatherp_alltoallv");
    MPI_Alltoallv(out, numr, startr, mpi_type, in, numg, startg, mpi_type, this->pool_world);
    ModuleBase::timer::end(this->classname, "gatherp_alltoallv");

    const int poolnproc_gps = this->poolnproc;
    const int nst_gps = this->nst;
    const int nz_gps = this->nz;
    const int* numz_gps = this->numz;
    const int* startg_gps = this->startg;
    const int* startz_gps = this->startz;

    ModuleBase::timer::start(this->classname, "gatherp_unpack");
#ifdef _OPENMP
#pragma omp parallel for collapse(2) schedule(static)
#endif
    for (int ip = 0; ip < poolnproc_gps; ++ip)
    {
        for (int is = 0; is < nst_gps; ++is)
        {
            const int nzip = numz_gps[ip];
            std::complex<T>* outp = &out[startz_gps[ip] + is * nz_gps];
            const std::complex<T>* inp = &in[startg_gps[ip] + is * nzip];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nzip);
        }
    }
    ModuleBase::timer::end(this->classname, "gatherp_unpack");
#endif

    ModuleBase::timer::end(this->classname, "gatherp_scatters");
}

/**
 * @brief gather sticks and scatter planes
 * @param in: (nz,nst)
 * @param out: (nplane,fftny,fftnx)
 * @note in and out should be in different places
 * @note in[] will be changed
 */
template <typename T>
void PW_Basis::gathers_scatterp(std::complex<T>* in, std::complex<T>* out) const
{
    ModuleBase::timer::start(this->classname, "gathers_scatterp");

    if (this->poolnproc == 1)
    {
        const int nrxx_ = this->nrxx;
        const int nst_ = this->nst;
        const int nz_ = this->nz;
        const int* istot2ixy_ = this->istot2ixy;
        ModuleBase::timer::start(this->classname, "gathers_zero_serial");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int i = 0; i < nrxx_; ++i)
        {
            out[i] = std::complex<T>(0, 0);
        }
        ModuleBase::timer::end(this->classname, "gathers_zero_serial");

        ModuleBase::timer::start(this->classname, "gathers_copy_serial");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < nst_; ++is)
        {
            const int ixy = istot2ixy_[is];
            std::complex<T>* outp = &out[ixy * nz_];
            const std::complex<T>* inp = &in[is * nz_];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nz_);
        }
        ModuleBase::timer::end(this->classname, "gathers_copy_serial");
        ModuleBase::timer::end(this->classname, "gathers_scatterp");
        return;
    }

#ifdef __MPI
    const int poolnproc_ = this->poolnproc;
    const int nst_ = this->nst;
    const int nz_ = this->nz;
    const int* numz_ = this->numz;
    const int* startg_ = this->startg;
    const int* startz_ = this->startz;

    ModuleBase::timer::start(this->classname, "gathers_pack");
#ifdef _OPENMP
#pragma omp parallel for collapse(2) schedule(static)
#endif
    for (int ip = 0; ip < poolnproc_; ++ip)
    {
        for (int is = 0; is < nst_; ++is)
        {
            const int nzip = numz_[ip];
            std::complex<T>* outp = &out[startg_[ip] + is * nzip];
            const std::complex<T>* inp = &in[startz_[ip] + is * nz_];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nzip);
        }
    }
    ModuleBase::timer::end(this->classname, "gathers_pack");

    const MPI_Datatype mpi_type = detail::mpi_complex_dtype<T>();
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value,
                  "PW_Basis::gathers_scatterp only supports float or double");

    ModuleBase::timer::start(this->classname, "gathers_alltoallv");
    MPI_Alltoallv(out, numg, startg, mpi_type, in, numr, startr, mpi_type, this->pool_world);
    ModuleBase::timer::end(this->classname, "gathers_alltoallv");

    const int nrxx_gsp = this->nrxx;
    ModuleBase::timer::start(this->classname, "gathers_zero_mpi");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < nrxx_gsp; ++i)
    {
        out[i] = std::complex<T>(0, 0);
    }
    ModuleBase::timer::end(this->classname, "gathers_zero_mpi");

    const int nstot = this->nstot;
    const int nplane = this->nplane;
    const int* istot2ixy = this->istot2ixy;
    ModuleBase::timer::start(this->classname, "gathers_unpack");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int istot = 0; istot < nstot; ++istot)
    {
        const int ixy = istot2ixy[istot];
        std::complex<T>* outp = &out[ixy * nplane];
        const std::complex<T>* inp = &in[istot * nplane];
        ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                              reinterpret_cast<const T*>(inp),
                              2 * nplane);
    }
    ModuleBase::timer::end(this->classname, "gathers_unpack");
#endif

    ModuleBase::timer::end(this->classname, "gathers_scatterp");
}

} // namespace ModulePW

#include "pw_basis.h"
#include "pw_simd_copy.h"
#include "source_base/global_function.h"
#include "source_base/timer.h"
#include <algorithm>
#include <type_traits>
#include <vector>

namespace ModulePW
{

// ---------------------------------------------------------------------------
// Lightweight copy helpers — used by pw_transform.cpp, pw_transform_k.cpp
// and inside gather/scatter parallel regions below.
// ---------------------------------------------------------------------------
namespace copy_detail
{
template <typename T>
inline void copy_complex_buffer(const std::complex<T>* in, std::complex<T>* out, const int count)
{
    if (count <= 0)
    {
        return;
    }

    ModulePW::simd_copy_n(reinterpret_cast<T*>(out), reinterpret_cast<const T*>(in), 2 * count);
}

// Top-level transform copies own the OpenMP parallel region; gather/scatter
// loops call the non-parallel helper inside their existing parallel regions.
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
        copy_complex_buffer(in + offset, out + offset, chunk_count);
    }
#else
    copy_complex_buffer(in, out, count);
#endif
}
} // namespace copy_detail

// ---------------------------------------------------------------------------
// Compile-time MPI complex type dispatch — avoids typeid() runtime overhead.
// Only available under __MPI; call-sites are already guarded by #ifdef __MPI.
// ---------------------------------------------------------------------------
#ifdef __MPI
namespace detail
{
template <typename T>
inline MPI_Datatype mpi_complex_dtype()
{
    // Unsupported types fail at compile time via the static_assert in the
    // only call-sites (gatherp_scatters / gathers_scatterp).
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
} // namespace detail
#endif // __MPI

/**
 * @brief gather planes and scatter sticks
 * @param in: (nplane,fftny,fftnx)
 * @param out: (nz,nst)
 * @note in and out should be in different places
 * @note in[] is read-only in the non-blocking path (dedicated sendbuf/recvbuf)
 */
template <typename T>
void PW_Basis::gatherp_scatters(std::complex<T>* in, std::complex<T>* out) const
{
    ModuleBase::timer::start(this->classname, "gatherp_scatters");

    if (this->poolnproc == 1) // In this case nst=nstot, nz = nplane
    {
        const int nst_ = this->nst;
        const int nz_ = this->nz;
        const int* istot2ixy_ = this->istot2ixy;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < nst_; ++is)
        {
            int ixy = istot2ixy_[is];
            std::complex<T>* outp = &out[is * nz_];
            std::complex<T>* inp = &in[ixy * nz_];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nz_);
        }
        ModuleBase::timer::end(this->classname, "gatherp_scatters");
        return;
    }

#ifdef __MPI
    // ---- capture member pointers for const-correctness & register pressure ----
    const int nstot_gps = this->nstot;
    const int nplane_gps = this->nplane;
    const int* istot2ixy_gps = this->istot2ixy;
    const int* numg_gps = this->numg;
    const int* numr_gps = this->numr;
    const int* startg_gps = this->startg;
    const int* startr_gps = this->startr;
    const int poolrank_gps = this->poolrank;
    const int poolnproc_gps = this->poolnproc;

    // ---- allocate work buffers ----
    const int send_count_gps = startr_gps[poolnproc_gps - 1] + numr_gps[poolnproc_gps - 1];
    const int recv_count_gps = startg_gps[poolnproc_gps - 1] + numg_gps[poolnproc_gps - 1];
    std::complex<T>* commbuf = this->acquire_comm_workbuf<T>(send_count_gps + recv_count_gps);
    std::complex<T>* sendbuf = commbuf;
    std::complex<T>* recvbuf = commbuf + send_count_gps;

    // ---- self-stick range (global stick indices owned by this rank) ----
    // startr[poolrank] = sum_{ip < poolrank} nst_per[ip] * nplane
    // => self_istot_beg = startr[poolrank] / nplane
    const int nst_gps = this->nst;
    const int self_istot_beg
        = (nplane_gps > 0) ? (startr_gps[poolrank_gps] / nplane_gps) : 0;
    const int self_istot_end = self_istot_beg + nst_gps;

    // ---- pack: in(ixy, nplane) -> sendbuf(istot, nplane) ----
    // Self-owned sticks are skipped here; they are written directly in->out below.
    ModuleBase::timer::start(this->classname, "gatherp_pack");
    if (nplane_gps > 0)
    {
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int istot = 0; istot < nstot_gps; ++istot)
        {
            // Self-owned sticks: bypass sendbuf, handled via direct in->out copy
            if (istot >= self_istot_beg && istot < self_istot_end)
            {
                continue;
            }
            int ixy = istot2ixy_gps[istot];
            std::complex<T>* outp = &sendbuf[istot * nplane_gps];
            std::complex<T>* inp = &in[ixy * nplane_gps];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nplane_gps);
        }
    }
    ModuleBase::timer::end(this->classname, "gatherp_pack");

    // ---- MPI type (compile-time dispatch) ----
    const MPI_Datatype mpi_type = detail::mpi_complex_dtype<T>();
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value,
                  "PW_Basis::gatherp_scatters only supports float or double");

    // ---- thread-local MPI bookkeeping ----
    static thread_local std::vector<MPI_Request> recv_requests;
    static thread_local std::vector<MPI_Request> send_requests;
    static thread_local std::vector<MPI_Status> recv_status;
    static thread_local std::vector<int> recv_indices;
    recv_requests.assign(poolnproc_gps, MPI_REQUEST_NULL);
    send_requests.assign(poolnproc_gps, MPI_REQUEST_NULL);
    recv_status.resize(poolnproc_gps);
    recv_indices.assign(poolnproc_gps, MPI_UNDEFINED);
    int active_recvs = 0;
    int active_sends = 0;

    // ---- post non-blocking receives & sends (skip self and zero-size) ----
    ModuleBase::timer::start(this->classname, "gatherp_alltoallv");
    for (int ip = 0; ip < poolnproc_gps; ++ip)
    {
        if (ip == poolrank_gps || numg_gps[ip] == 0)
        {
            continue;
        }
        MPI_Irecv(&recvbuf[startg_gps[ip]], numg_gps[ip], mpi_type, ip, 0,
                  this->pool_world, &recv_requests[ip]);
        ++active_recvs;
    }
    for (int ip = 0; ip < poolnproc_gps; ++ip)
    {
        if (ip == poolrank_gps || numr_gps[ip] == 0)
        {
            continue;
        }
        MPI_Isend(&sendbuf[startr_gps[ip]], numr_gps[ip], mpi_type, ip, 0,
                  this->pool_world, &send_requests[ip]);
        ++active_sends;
    }
    ModuleBase::timer::end(this->classname, "gatherp_alltoallv");

    // ---- unpack lambda: recvbuf[ip] -> out (stick-major) ----
    const int nz_gps = this->nz;
    const int* numz_gps = this->numz;
    const int* startz_gps = this->startz;
    auto unpack_peer = [&](const int ip)
    {
        const int nzip = numz_gps[ip];
        if (nzip == 0)
        {
            return;
        }
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < nst_gps; ++is)
        {
            std::complex<T>* outp = &out[is * nz_gps + startz_gps[ip]];
            std::complex<T>* inp = &recvbuf[startg_gps[ip] + is * nzip];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nzip);
        }
    };

    // ---- self-data: direct in -> out (bypasses sendbuf & recvbuf) ----
    ModuleBase::timer::start(this->classname, "gatherp_unpack");
    if (nplane_gps > 0)
    {
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < nst_gps; ++is)
        {
            const int istot = self_istot_beg + is;
            const int ixy = istot2ixy_gps[istot];
            std::complex<T>* outp = &out[is * nz_gps + startz_gps[poolrank_gps]];
            std::complex<T>* inp = &in[ixy * nplane_gps];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nplane_gps);
        }
    }
    ModuleBase::timer::end(this->classname, "gatherp_unpack");

    // ---- progress loop: overlap communication with unpack ----
    while (active_recvs > 0)
    {
        int outcount = 0;
        ModuleBase::timer::start(this->classname, "gatherp_alltoallv");
        MPI_Waitsome(poolnproc_gps,
                     recv_requests.data(),
                     &outcount,
                     recv_indices.data(),
                     recv_status.data());
        ModuleBase::timer::end(this->classname, "gatherp_alltoallv");
        if (outcount == MPI_UNDEFINED)
        {
            break;
        }
        for (int idx = 0; idx < outcount; ++idx)
        {
            ModuleBase::timer::start(this->classname, "gatherp_unpack");
            unpack_peer(recv_indices[idx]);
            ModuleBase::timer::end(this->classname, "gatherp_unpack");
        }
        active_recvs -= outcount;
    }

    // ---- drain sends ----
    if (active_sends > 0)
    {
        ModuleBase::timer::start(this->classname, "gatherp_alltoallv");
        MPI_Waitall(poolnproc_gps, send_requests.data(), MPI_STATUSES_IGNORE);
        ModuleBase::timer::end(this->classname, "gatherp_alltoallv");
    }
#endif
    ModuleBase::timer::end(this->classname, "gatherp_scatters");
    return;
}

/**
 * @brief gather sticks and scatter planes
 * @param in: (nz,nst)
 * @param out: (nplane,fftny,fftnx)
 * @note in and out should be in different places
 * @note in[] is read-only in the non-blocking path (dedicated sendbuf/recvbuf)
 */
template <typename T>
void PW_Basis::gathers_scatterp(std::complex<T>* in, std::complex<T>* out) const
{
    ModuleBase::timer::start(this->classname, "gathers_scatterp");
    if (this->poolnproc == 1) // In this case nrxx=fftnx*fftny*nz, nst = nstot
    {
        const int nrxx_ = this->nrxx;
        const int nst_ = this->nst;
        const int nz_ = this->nz;
        const int* istot2ixy_ = this->istot2ixy;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int i = 0; i < nrxx_; ++i)
        {
            out[i] = std::complex<T>(0, 0);
        }

#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < nst_; ++is)
        {
            int ixy = istot2ixy_[is];
            std::complex<T>* outp = &out[ixy * nz_];
            std::complex<T>* inp = &in[is * nz_];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nz_);
        }
        ModuleBase::timer::end(this->classname, "gathers_scatterp");
        return;
    }

#ifdef __MPI
    // ---- capture member pointers ----
    const int poolnproc_ = this->poolnproc;
    const int nst_ = this->nst;
    const int nz_ = this->nz;
    const int* numz_ = this->numz;
    const int* startg_ = this->startg;
    const int* startz_ = this->startz;
    const int* nst_per_ = this->nst_per;
    const int* startr_ = this->startr;
    const int poolrank_ = this->poolrank;

    // ---- allocate work buffers ----
    const int send_count_ = startg_[poolnproc_ - 1] + this->numg[poolnproc_ - 1];
    const int recv_count_ = startr_[poolnproc_ - 1] + this->numr[poolnproc_ - 1];
    std::complex<T>* commbuf = this->acquire_comm_workbuf<T>(send_count_ + recv_count_);
    std::complex<T>* sendbuf = commbuf;
    std::complex<T>* recvbuf = commbuf + send_count_;

    // ---- pack: in(is, nz) -> sendbuf(startg[ip], is*nzip) ----
    // Use dynamic scheduling because numz[ip] (inner-loop trip count) varies
    // across peers.  Self (ip == poolrank_) is skipped — its data goes
    // directly in->out below.
    ModuleBase::timer::start(this->classname, "gathers_pack");
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1)
#endif
    for (int ip = 0; ip < poolnproc_; ++ip)
    {
        if (ip == poolrank_ || numz_[ip] == 0)
        {
            continue;
        }
        const int nzip = numz_[ip];
        const std::complex<T>* __restrict__ inp_base = &in[startz_[ip]];
        std::complex<T>* __restrict__ outp_base = &sendbuf[startg_[ip]];
        for (int is = 0; is < nst_; ++is)
        {
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp_base + is * nzip),
                                  reinterpret_cast<const T*>(inp_base + is * nz_),
                                  2 * nzip);
        }
    }
    ModuleBase::timer::end(this->classname, "gathers_pack");

    // ---- MPI type (compile-time dispatch) ----
    const MPI_Datatype mpi_type = detail::mpi_complex_dtype<T>();
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value,
                  "PW_Basis::gathers_scatterp only supports float or double");

    // ---- thread-local MPI bookkeeping ----
    static thread_local std::vector<MPI_Request> recv_requests;
    static thread_local std::vector<MPI_Request> send_requests;
    static thread_local std::vector<MPI_Status> recv_status;
    static thread_local std::vector<int> recv_indices;
    recv_requests.assign(poolnproc_, MPI_REQUEST_NULL);
    send_requests.assign(poolnproc_, MPI_REQUEST_NULL);
    recv_status.resize(poolnproc_);
    recv_indices.assign(poolnproc_, MPI_UNDEFINED);
    int active_recvs = 0;
    int active_sends = 0;

    // ---- post non-blocking receives & sends (skip self and zero-size) ----
    ModuleBase::timer::start(this->classname, "gathers_alltoallv");
    for (int ip = 0; ip < poolnproc_; ++ip)
    {
        if (ip == poolrank_ || this->numr[ip] == 0)
        {
            continue;
        }
        MPI_Irecv(&recvbuf[startr_[ip]], this->numr[ip], mpi_type, ip, 0,
                  this->pool_world, &recv_requests[ip]);
        ++active_recvs;
    }
    for (int ip = 0; ip < poolnproc_; ++ip)
    {
        if (ip == poolrank_ || this->numg[ip] == 0)
        {
            continue;
        }
        MPI_Isend(&sendbuf[startg_[ip]], this->numg[ip], mpi_type, ip, 0,
                  this->pool_world, &send_requests[ip]);
        ++active_sends;
    }
    ModuleBase::timer::end(this->classname, "gathers_alltoallv");

    // ---- clear output (overlaps with MPI wire-up) ----
    ModuleBase::timer::start(this->classname, "gathers_clear");
    const int nrxx_gsp = this->nrxx;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < nrxx_gsp; ++i)
    {
        out[i] = std::complex<T>(0, 0);
    }
    ModuleBase::timer::end(this->classname, "gathers_clear");

    // ---- unpack lambda: recvbuf[ip] -> out(ixy, nplane) ----
    const int nplane = this->nplane;
    const int* istot2ixy = this->istot2ixy;
    auto unpack_peer = [&](const int ip)
    {
        const int peer_nst = nst_per_[ip];
        if (peer_nst == 0 || nplane == 0)
        {
            return;
        }
        // Derive istot offset from startr: startr[ip] = istot_offset[ip] * nplane
        const int istot0 = startr_[ip] / nplane;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < peer_nst; ++is)
        {
            const int istot = istot0 + is;
            const int ixy = istot2ixy[istot];
            std::complex<T>* outp = &out[ixy * nplane];
            std::complex<T>* inp = &recvbuf[startr_[ip] + is * nplane];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nplane);
        }
    };

    // ---- self-data: direct in -> out (bypasses sendbuf & recvbuf) ----
    ModuleBase::timer::start(this->classname, "gathers_unpack");
    if (nplane > 0)
    {
        // istot_offset[self] = startr[poolrank] / nplane
        const int self_istot0 = startr_[poolrank_] / nplane;
        const int self_nst = nst_per_[poolrank_];
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < self_nst; ++is)
        {
            const int istot = self_istot0 + is;
            const int ixy = istot2ixy[istot];
            std::complex<T>* outp = &out[ixy * nplane];
            std::complex<T>* inp = &in[is * nz_ + startz_[poolrank_]];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nplane);
        }
    }
    ModuleBase::timer::end(this->classname, "gathers_unpack");

    // ---- progress loop ----
    while (active_recvs > 0)
    {
        int outcount = 0;
        ModuleBase::timer::start(this->classname, "gathers_alltoallv");
        MPI_Waitsome(poolnproc_,
                     recv_requests.data(),
                     &outcount,
                     recv_indices.data(),
                     recv_status.data());
        ModuleBase::timer::end(this->classname, "gathers_alltoallv");
        if (outcount == MPI_UNDEFINED)
        {
            break;
        }
        for (int idx = 0; idx < outcount; ++idx)
        {
            ModuleBase::timer::start(this->classname, "gathers_unpack");
            unpack_peer(recv_indices[idx]);
            ModuleBase::timer::end(this->classname, "gathers_unpack");
        }
        active_recvs -= outcount;
    }

    // ---- drain sends ----
    if (active_sends > 0)
    {
        ModuleBase::timer::start(this->classname, "gathers_alltoallv");
        MPI_Waitall(poolnproc_, send_requests.data(), MPI_STATUSES_IGNORE);
        ModuleBase::timer::end(this->classname, "gathers_alltoallv");
    }
#endif
    ModuleBase::timer::end(this->classname, "gathers_scatterp");
    return;
}

} // namespace ModulePW

#include "pw_basis.h"
#include "pw_simd_copy.h"
#include "source_base/global_function.h"
#include "source_base/timer.h"
#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

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

// ---------------------------------------------------------------------------
// Compile-time MPI complex type dispatch — avoids typeid() runtime overhead.
// Only available under __MPI; call-sites are already guarded by #ifdef __MPI.
// ---------------------------------------------------------------------------
#ifdef __MPI
inline int overlap_block_sticks(const int nst, const int span)
{
    if (nst <= 64)
    {
        return std::max(1, nst);
    }

    const int safe_span = std::max(1, span);
    const int target_complex = 262144;
    int block = target_complex / safe_span;
    if (block < 16)
    {
        block = 16;
    }
    if (block > 512)
    {
        block = 512;
    }
    return std::max(1, std::min(nst, block));
}

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

inline void check_mpi(const int ierr, const char* where)
{
    if (ierr == MPI_SUCCESS)
    {
        return;
    }

    char error_string[MPI_MAX_ERROR_STRING] = {0};
    int error_length = 0;
    MPI_Error_string(ierr, error_string, &error_length);
    ModuleBase::WARNING_QUIT(where,
                             std::string("MPI communication failed: ")
                                 + std::string(error_string, error_length));
}

inline bool prefer_overlap_pipeline(const int npwtot, const int poolnproc)
{
    (void)npwtot;
    return poolnproc > 1;
}
#endif // __MPI
} // namespace detail

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
    if (this->poolnproc == 1) // In this case nst=nstot, nz = nplane
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
            int ixy = istot2ixy_[is];
            std::complex<T>* outp = &out[is * nz_];
            std::complex<T>* inp = &in[ixy * nz_];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nz_);
        }
        ModuleBase::timer::end(this->classname, "gatherp_copy_serial");
        return;
    }

#ifdef __MPI
    const int nstot_gps = this->nstot;
    const int nst_gps = this->nst;
    const int nplane_gps = this->nplane;
    const int nz_gps = this->nz;
    const int* istot2ixy_gps = this->istot2ixy;
    const int* numg_gps = this->numg;
    const int* numr_gps = this->numr;
    const int* numz_gps = this->numz;
    const int* startg_gps = this->startg;
    const int* startr_gps = this->startr;
    const int* startz_gps = this->startz;
    const int* nst_per_gps = this->nst_per;
    const int poolrank_gps = this->poolrank;
    const int poolnproc_gps = this->poolnproc;
    const MPI_Datatype mpi_type = detail::mpi_complex_dtype<T>();
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value,
                  "PW_Basis::gatherp_scatters only supports float or double");

    if (!detail::prefer_overlap_pipeline(this->npwtot, poolnproc_gps))
    {
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int istot = 0; istot < nstot_gps; ++istot)
        {
            const int ixy = istot2ixy_gps[istot];
            std::complex<T>* outp = &out[istot * nplane_gps];
            const std::complex<T>* inp = &in[ixy * nplane_gps];
            detail::copy_complex_buffer(inp, outp, nplane_gps);
        }
        MPI_Alltoallv(out, numr_gps, startr_gps, mpi_type, in, numg_gps, startg_gps, mpi_type, this->pool_world);
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
                detail::copy_complex_buffer(inp, outp, nzip);
            }
        }
        return;
    }

    const int self_istot_beg = (nplane_gps > 0) ? (startr_gps[poolrank_gps] / nplane_gps) : 0;
    const int block_sticks = detail::overlap_block_sticks(nstot_gps, std::max(nplane_gps, nz_gps));
    int nst_max = 0;
    for (int ip = 0; ip < poolnproc_gps; ++ip)
    {
        nst_max = std::max(nst_max, nst_per_gps[ip]);
    }
    const int num_blocks = std::max(1, (nst_max + block_sticks - 1) / block_sticks);

    if (num_blocks == 1)
    {
        ModuleBase::timer::start(this->classname, "gatherp_single_block_fallback");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int istot = 0; istot < nstot_gps; ++istot)
        {
            const int ixy = istot2ixy_gps[istot];
            std::complex<T>* outp = &out[istot * nplane_gps];
            const std::complex<T>* inp = &in[ixy * nplane_gps];
            detail::copy_complex_buffer(inp, outp, nplane_gps);
        }
        detail::check_mpi(MPI_Alltoallv(out,
                                        numr_gps,
                                        startr_gps,
                                        mpi_type,
                                        in,
                                        numg_gps,
                                        startg_gps,
                                        mpi_type,
                                        this->pool_world),
                          "PW_Basis::gatherp_scatters");
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
                detail::copy_complex_buffer(inp, outp, nzip);
            }
        }
        ModuleBase::timer::end(this->classname, "gatherp_single_block_fallback");
        return;
    }

    const int send_count_gps = block_sticks * nplane_gps * poolnproc_gps;
    const int recv_count_gps = block_sticks * nz_gps;
    const int buf_span_gps = std::max(1, send_count_gps + recv_count_gps);
    std::vector<std::complex<T>> commbuf(std::max(1, 2 * buf_span_gps));
    std::complex<T>* sendbufs[2] = {commbuf.data(), commbuf.data() + buf_span_gps};
    std::complex<T>* recvbufs[2] = {sendbufs[0] + send_count_gps, sendbufs[1] + send_count_gps};

    struct BlockState
    {
        int is0 = 0;
        int local_bsticks = 0;
        std::vector<int> sendcounts;
        std::vector<int> recvcounts;
        std::vector<int> sdispls;
        std::vector<int> rdispls;
        std::vector<MPI_Request> recv_requests;
        std::vector<MPI_Request> send_requests;
        MPI_Request alltoall_request = MPI_REQUEST_NULL;
    };

    BlockState blocks[2];
    for (int ib = 0; ib < 2; ++ib)
    {
        blocks[ib].sendcounts.assign(poolnproc_gps, 0);
        blocks[ib].recvcounts.assign(poolnproc_gps, 0);
        blocks[ib].sdispls.assign(poolnproc_gps, 0);
        blocks[ib].rdispls.assign(poolnproc_gps, 0);
        blocks[ib].recv_requests.assign(poolnproc_gps, MPI_REQUEST_NULL);
        blocks[ib].send_requests.assign(poolnproc_gps, MPI_REQUEST_NULL);
    }

    auto prepare_block = [&](BlockState& blk, const int buf_id, const int is0)
    {
        blk.is0 = is0;
        blk.local_bsticks = std::max(0, std::min(block_sticks, nst_gps - is0));
        std::fill(blk.sendcounts.begin(), blk.sendcounts.end(), 0);
        std::fill(blk.recvcounts.begin(), blk.recvcounts.end(), 0);
        std::fill(blk.sdispls.begin(), blk.sdispls.end(), 0);
        std::fill(blk.rdispls.begin(), blk.rdispls.end(), 0);
        std::fill(blk.recv_requests.begin(), blk.recv_requests.end(), MPI_REQUEST_NULL);
        std::fill(blk.send_requests.begin(), blk.send_requests.end(), MPI_REQUEST_NULL);
        blk.alltoall_request = MPI_REQUEST_NULL;

        int send_disp = 0;
        int recv_disp = 0;
        for (int ip = 0; ip < poolnproc_gps; ++ip)
        {
            blk.sdispls[ip] = send_disp;
            if (ip != poolrank_gps && nplane_gps > 0)
            {
                const int peer_nst = nst_per_gps[ip];
                const int peer_bsticks = std::max(0, std::min(block_sticks, peer_nst - is0));
                blk.sendcounts[ip] = peer_bsticks * nplane_gps;
                send_disp += blk.sendcounts[ip];
            }

            blk.rdispls[ip] = recv_disp;
            if (ip != poolrank_gps)
            {
                blk.recvcounts[ip] = blk.local_bsticks * numz_gps[ip];
                recv_disp += blk.recvcounts[ip];
            }
        }

        ModuleBase::timer::start(this->classname, "gatherp_pack");
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1)
#endif
        for (int ip = 0; ip < poolnproc_gps; ++ip)
        {
            if (ip == poolrank_gps || blk.sendcounts[ip] == 0)
            {
                continue;
            }
            const int istot0 = startr_gps[ip] / nplane_gps + is0;
            std::complex<T>* outp = sendbufs[buf_id] + blk.sdispls[ip];
            for (int is = 0; is < blk.sendcounts[ip] / nplane_gps; ++is)
            {
                const int ixy = istot2ixy_gps[istot0 + is];
                const std::complex<T>* inp = &in[ixy * nplane_gps];
                ModulePW::simd_copy_n(reinterpret_cast<T*>(outp + is * nplane_gps),
                                      reinterpret_cast<const T*>(inp),
                                      2 * nplane_gps);
            }
        }
        ModuleBase::timer::end(this->classname, "gatherp_pack");

        ModuleBase::timer::start(this->classname, "gatherp_unpack");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < blk.local_bsticks; ++is)
        {
            const int istot = self_istot_beg + is0 + is;
            const int ixy = istot2ixy_gps[istot];
            std::complex<T>* outp = &out[(is0 + is) * nz_gps + startz_gps[poolrank_gps]];
            const std::complex<T>* inp = &in[ixy * nplane_gps];
            detail::copy_complex_buffer(inp, outp, nplane_gps);
        }
        ModuleBase::timer::end(this->classname, "gatherp_unpack");

        ModuleBase::timer::start(this->classname, "gatherp_overlap_comm");
#if MPI_VERSION >= 3
        detail::check_mpi(MPI_Ialltoallv(sendbufs[buf_id],
                                         blk.sendcounts.data(),
                                         blk.sdispls.data(),
                                         mpi_type,
                                         recvbufs[buf_id],
                                         blk.recvcounts.data(),
                                         blk.rdispls.data(),
                                         mpi_type,
                                         this->pool_world,
                                         &blk.alltoall_request),
                          "PW_Basis::gatherp_scatters");
#else
        for (int ip = 0; ip < poolnproc_gps; ++ip)
        {
            if (ip == poolrank_gps || blk.recvcounts[ip] == 0)
            {
                continue;
            }
            detail::check_mpi(MPI_Irecv(recvbufs[buf_id] + blk.rdispls[ip],
                                         blk.recvcounts[ip],
                                         mpi_type,
                                         ip,
                                         0,
                                         this->pool_world,
                                         &blk.recv_requests[ip]),
                              "PW_Basis::gatherp_scatters");
        }
        for (int ip = 0; ip < poolnproc_gps; ++ip)
        {
            if (ip == poolrank_gps || blk.sendcounts[ip] == 0)
            {
                continue;
            }
            detail::check_mpi(MPI_Isend(sendbufs[buf_id] + blk.sdispls[ip],
                                         blk.sendcounts[ip],
                                         mpi_type,
                                         ip,
                                         0,
                                         this->pool_world,
                                         &blk.send_requests[ip]),
                              "PW_Basis::gatherp_scatters");
        }
#endif
        ModuleBase::timer::end(this->classname, "gatherp_overlap_comm");
    };

    auto finalize_block = [&](BlockState& blk, const int buf_id)
    {
        ModuleBase::timer::start(this->classname, "gatherp_overlap_comm");
#if MPI_VERSION >= 3
        detail::check_mpi(MPI_Wait(&blk.alltoall_request, MPI_STATUS_IGNORE),
                          "PW_Basis::gatherp_scatters");
#else
        detail::check_mpi(MPI_Waitall(poolnproc_gps, blk.recv_requests.data(), MPI_STATUSES_IGNORE),
                          "PW_Basis::gatherp_scatters");
        detail::check_mpi(MPI_Waitall(poolnproc_gps, blk.send_requests.data(), MPI_STATUSES_IGNORE),
                          "PW_Basis::gatherp_scatters");
#endif
        ModuleBase::timer::end(this->classname, "gatherp_overlap_comm");

        for (int ip = 0; ip < poolnproc_gps; ++ip)
        {
            const int nzip = numz_gps[ip];
            if (ip == poolrank_gps || nzip == 0 || blk.recvcounts[ip] == 0)
            {
                continue;
            }
            ModuleBase::timer::start(this->classname, "gatherp_unpack");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
            for (int is = 0; is < blk.local_bsticks; ++is)
            {
                std::complex<T>* outp = &out[(blk.is0 + is) * nz_gps + startz_gps[ip]];
                const std::complex<T>* inp = recvbufs[buf_id] + blk.rdispls[ip] + is * nzip;
                detail::copy_complex_buffer(inp, outp, nzip);
            }
            ModuleBase::timer::end(this->classname, "gatherp_unpack");
        }
    };

    int current = 0;
    prepare_block(blocks[current], current, 0);
    for (int iblock = 0; iblock < num_blocks; ++iblock)
    {
        const int next_is0 = (iblock + 1) * block_sticks;
        const int next = 1 - current;
        if (next_is0 < nst_max)
        {
            prepare_block(blocks[next], next, next_is0);
        }
        finalize_block(blocks[current], current);
        current = next;
    }
#endif
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
    if (this->poolnproc == 1) // In this case nrxx=fftnx*fftny*nz, nst = nstot
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
            int ixy = istot2ixy_[is];
            std::complex<T>* outp = &out[ixy * nz_];
            std::complex<T>* inp = &in[is * nz_];
            ModulePW::simd_copy_n(reinterpret_cast<T*>(outp),
                                  reinterpret_cast<const T*>(inp),
                                  2 * nz_);
        }
        ModuleBase::timer::end(this->classname, "gathers_copy_serial");
        return;
    }

#ifdef __MPI
    const int poolnproc_ = this->poolnproc;
    const int nst_ = this->nst;
    const int nstot = this->nstot;
    const int nz_ = this->nz;
    const int nplane = this->nplane;
    const int* numz_ = this->numz;
    const int* startg_ = this->startg;
    const int* startz_ = this->startz;
    const int* nst_per_ = this->nst_per;
    const int* startr_ = this->startr;
    const int* istot2ixy = this->istot2ixy;
    const int poolrank_ = this->poolrank;
    const MPI_Datatype mpi_type = detail::mpi_complex_dtype<T>();
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value,
                  "PW_Basis::gathers_scatterp only supports float or double");

    if (!detail::prefer_overlap_pipeline(this->npwtot, poolnproc_))
    {
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
                detail::copy_complex_buffer(inp, outp, nzip);
            }
        }
        MPI_Alltoallv(out, this->numg, startg_, mpi_type, in, this->numr, startr_, mpi_type, this->pool_world);
        const int nrxx_fallback = this->nrxx;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int i = 0; i < nrxx_fallback; ++i)
        {
            out[i] = std::complex<T>(0, 0);
        }
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int istot = 0; istot < nstot; ++istot)
        {
            const int ixy = istot2ixy[istot];
            std::complex<T>* outp = &out[ixy * nplane];
            const std::complex<T>* inp = &in[istot * nplane];
            detail::copy_complex_buffer(inp, outp, nplane);
        }
        return;
    }

    const int nrxx_gsp = this->nrxx;
    const int block_sticks = detail::overlap_block_sticks(nstot, std::max(nz_, nplane));
    int nst_max = 0;
    for (int ip = 0; ip < poolnproc_; ++ip)
    {
        nst_max = std::max(nst_max, nst_per_[ip]);
    }
    const int num_blocks = std::max(1, (nst_max + block_sticks - 1) / block_sticks);

    if (num_blocks == 1)
    {
        ModuleBase::timer::start(this->classname, "gathers_single_block_fallback");
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
                detail::copy_complex_buffer(inp, outp, nzip);
            }
        }
        detail::check_mpi(MPI_Alltoallv(out,
                                        this->numg,
                                        startg_,
                                        mpi_type,
                                        in,
                                        this->numr,
                                        startr_,
                                        mpi_type,
                                        this->pool_world),
                          "PW_Basis::gathers_scatterp");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int i = 0; i < nrxx_gsp; ++i)
        {
            out[i] = std::complex<T>(0, 0);
        }
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int istot = 0; istot < nstot; ++istot)
        {
            const int ixy = istot2ixy[istot];
            std::complex<T>* outp = &out[ixy * nplane];
            const std::complex<T>* inp = &in[istot * nplane];
            detail::copy_complex_buffer(inp, outp, nplane);
        }
        ModuleBase::timer::end(this->classname, "gathers_single_block_fallback");
        return;
    }

    const int send_count_ = block_sticks * nz_;
    const int recv_count_ = block_sticks * nplane * poolnproc_;
    const int buf_span_ = std::max(1, send_count_ + recv_count_);
    std::vector<std::complex<T>> commbuf(std::max(1, 2 * buf_span_));
    std::complex<T>* sendbufs[2] = {commbuf.data(), commbuf.data() + buf_span_};
    std::complex<T>* recvbufs[2] = {sendbufs[0] + send_count_, sendbufs[1] + send_count_};

    ModuleBase::timer::start(this->classname, "gathers_clear");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < nrxx_gsp; ++i)
    {
        out[i] = std::complex<T>(0, 0);
    }
    ModuleBase::timer::end(this->classname, "gathers_clear");

    struct BlockState
    {
        int is0 = 0;
        int local_bsticks = 0;
        std::vector<int> sendcounts;
        std::vector<int> recvcounts;
        std::vector<int> sdispls;
        std::vector<int> rdispls;
        std::vector<MPI_Request> recv_requests;
        std::vector<MPI_Request> send_requests;
        MPI_Request alltoall_request = MPI_REQUEST_NULL;
    };

    BlockState blocks[2];
    for (int ib = 0; ib < 2; ++ib)
    {
        blocks[ib].sendcounts.assign(poolnproc_, 0);
        blocks[ib].recvcounts.assign(poolnproc_, 0);
        blocks[ib].sdispls.assign(poolnproc_, 0);
        blocks[ib].rdispls.assign(poolnproc_, 0);
        blocks[ib].recv_requests.assign(poolnproc_, MPI_REQUEST_NULL);
        blocks[ib].send_requests.assign(poolnproc_, MPI_REQUEST_NULL);
    }

    auto prepare_block = [&](BlockState& blk, const int buf_id, const int is0)
    {
        blk.is0 = is0;
        blk.local_bsticks = std::max(0, std::min(block_sticks, nst_ - is0));
        std::fill(blk.sendcounts.begin(), blk.sendcounts.end(), 0);
        std::fill(blk.recvcounts.begin(), blk.recvcounts.end(), 0);
        std::fill(blk.sdispls.begin(), blk.sdispls.end(), 0);
        std::fill(blk.rdispls.begin(), blk.rdispls.end(), 0);
        std::fill(blk.recv_requests.begin(), blk.recv_requests.end(), MPI_REQUEST_NULL);
        std::fill(blk.send_requests.begin(), blk.send_requests.end(), MPI_REQUEST_NULL);
        blk.alltoall_request = MPI_REQUEST_NULL;

        int send_disp = 0;
        int recv_disp = 0;
        for (int ip = 0; ip < poolnproc_; ++ip)
        {
            blk.sdispls[ip] = send_disp;
            if (ip != poolrank_)
            {
                blk.sendcounts[ip] = blk.local_bsticks * numz_[ip];
                send_disp += blk.sendcounts[ip];
            }

            blk.rdispls[ip] = recv_disp;
            if (ip != poolrank_ && nplane > 0)
            {
                const int peer_bsticks = std::max(0, std::min(block_sticks, nst_per_[ip] - is0));
                blk.recvcounts[ip] = peer_bsticks * nplane;
                recv_disp += blk.recvcounts[ip];
            }
        }

        ModuleBase::timer::start(this->classname, "gathers_pack");
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1)
#endif
        for (int ip = 0; ip < poolnproc_; ++ip)
        {
            if (ip == poolrank_ || blk.sendcounts[ip] == 0)
            {
                continue;
            }
            const int nzip = numz_[ip];
            std::complex<T>* outp = sendbufs[buf_id] + blk.sdispls[ip];
            for (int is = 0; is < blk.local_bsticks; ++is)
            {
                const std::complex<T>* inp = &in[(is0 + is) * nz_ + startz_[ip]];
                ModulePW::simd_copy_n(reinterpret_cast<T*>(outp + is * nzip),
                                      reinterpret_cast<const T*>(inp),
                                      2 * nzip);
            }
        }
        ModuleBase::timer::end(this->classname, "gathers_pack");

        ModuleBase::timer::start(this->classname, "gathers_unpack");
        const int self_istot0 = startr_[poolrank_] / nplane;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int is = 0; is < blk.local_bsticks; ++is)
        {
            const int istot = self_istot0 + is0 + is;
            const int ixy = istot2ixy[istot];
            std::complex<T>* outp = &out[ixy * nplane];
            const std::complex<T>* inp = &in[(is0 + is) * nz_ + startz_[poolrank_]];
            detail::copy_complex_buffer(inp, outp, nplane);
        }
        ModuleBase::timer::end(this->classname, "gathers_unpack");

        ModuleBase::timer::start(this->classname, "gathers_overlap_comm");
#if MPI_VERSION >= 3
        detail::check_mpi(MPI_Ialltoallv(sendbufs[buf_id],
                                         blk.sendcounts.data(),
                                         blk.sdispls.data(),
                                         mpi_type,
                                         recvbufs[buf_id],
                                         blk.recvcounts.data(),
                                         blk.rdispls.data(),
                                         mpi_type,
                                         this->pool_world,
                                         &blk.alltoall_request),
                          "PW_Basis::gathers_scatterp");
#else
        for (int ip = 0; ip < poolnproc_; ++ip)
        {
            if (ip == poolrank_ || blk.recvcounts[ip] == 0)
            {
                continue;
            }
            detail::check_mpi(MPI_Irecv(recvbufs[buf_id] + blk.rdispls[ip],
                                         blk.recvcounts[ip],
                                         mpi_type,
                                         ip,
                                         0,
                                         this->pool_world,
                                         &blk.recv_requests[ip]),
                              "PW_Basis::gathers_scatterp");
        }
        for (int ip = 0; ip < poolnproc_; ++ip)
        {
            if (ip == poolrank_ || blk.sendcounts[ip] == 0)
            {
                continue;
            }
            detail::check_mpi(MPI_Isend(sendbufs[buf_id] + blk.sdispls[ip],
                                         blk.sendcounts[ip],
                                         mpi_type,
                                         ip,
                                         0,
                                         this->pool_world,
                                         &blk.send_requests[ip]),
                              "PW_Basis::gathers_scatterp");
        }
#endif
        ModuleBase::timer::end(this->classname, "gathers_overlap_comm");
    };

    auto finalize_block = [&](BlockState& blk, const int buf_id)
    {
        ModuleBase::timer::start(this->classname, "gathers_overlap_comm");
#if MPI_VERSION >= 3
        detail::check_mpi(MPI_Wait(&blk.alltoall_request, MPI_STATUS_IGNORE),
                          "PW_Basis::gathers_scatterp");
#else
        detail::check_mpi(MPI_Waitall(poolnproc_, blk.recv_requests.data(), MPI_STATUSES_IGNORE),
                          "PW_Basis::gathers_scatterp");
        detail::check_mpi(MPI_Waitall(poolnproc_, blk.send_requests.data(), MPI_STATUSES_IGNORE),
                          "PW_Basis::gathers_scatterp");
#endif
        ModuleBase::timer::end(this->classname, "gathers_overlap_comm");

        for (int ip = 0; ip < poolnproc_; ++ip)
        {
            const int peer_bsticks = (nplane > 0) ? (blk.recvcounts[ip] / nplane) : 0;
            if (ip == poolrank_ || peer_bsticks == 0)
            {
                continue;
            }
            const int istot0 = startr_[ip] / nplane + blk.is0;
            ModuleBase::timer::start(this->classname, "gathers_unpack");
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
            for (int is = 0; is < peer_bsticks; ++is)
            {
                const int ixy = istot2ixy[istot0 + is];
                std::complex<T>* outp = &out[ixy * nplane];
                const std::complex<T>* inp = recvbufs[buf_id] + blk.rdispls[ip] + is * nplane;
                detail::copy_complex_buffer(inp, outp, nplane);
            }
            ModuleBase::timer::end(this->classname, "gathers_unpack");
        }
    };

    int current = 0;
    prepare_block(blocks[current], current, 0);
    for (int iblock = 0; iblock < num_blocks; ++iblock)
    {
        const int next_is0 = (iblock + 1) * block_sticks;
        const int next = 1 - current;
        if (next_is0 < nst_max)
        {
            prepare_block(blocks[next], next, next_is0);
        }
        finalize_block(blocks[current], current);
        current = next;
    }
#endif
    return;
}

} // namespace ModulePW

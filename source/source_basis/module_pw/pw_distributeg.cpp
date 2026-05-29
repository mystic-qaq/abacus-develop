#include "pw_basis.h"
#include "source_base/tool_quit.h"
#include "source_base/global_function.h"
#include "source_base/timer.h"
#include <algorithm>
#include <limits>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif
namespace ModulePW
{
/**
 * @brief distribute plane waves to different cores
 * @param in: G, GT, GGT, fftnx, fftny, nz, poolnproc, poolrank, ggecut
 * @param out: ig2isz[ig], istot2ixy[is], is2fftixy[is], fftixy2ip[ixy], gg[ig], gcar[ig], gdirect[ig], nst, nstot
 */
void PW_Basis::distribute_g()
{
    ModuleBase::timer::start(this->classname, "distributeg");
    if(this->distribution_type == 1)
    {
        this->distribution_method1();
    }
    else if(this->distribution_type == 2)
    {
        this->distribution_method2();
    }
    else
    {
        ModuleBase::WARNING_QUIT("divide", "No such division type.");
    }
    const char* no_pw_message = "Current core has no plane waves! Please reduce the cores.";
    ModuleBase::CHECK_WARNING_QUIT((this->npw == 0), "pw_distributeg.cpp", no_pw_message);
    ModuleBase::timer::end(this->classname, "distributeg");
    return;
}

/**
 * @brief (1) We count the total number of planewaves (tot_npw) and sticks (this->nstot) here.
 *
 *  Meanwhile, we record the number of planewaves on (x, y) in st_length2D, and store the smallest z-coordinate of each stick in st_bottom2D,
 *  so that we can scan a much smaller area in step(2).
 *
 * @param in: fftnx, fftny, nz, ggecut, GGT
 * @param out: tot_npw, this->nstot, st_length2D, st_bottom2D, this->riy, this->liy
 */

void PW_Basis::count_pw_st(
        int* st_length2D, // the number of planewaves that belong to the stick located on (x, y).
        int* st_bottom2D, // the z-coordinate of the bottom of stick on (x, y).
        const IPWCriterion* criterion // injectable plane-wave cutoff predicate; nullptr uses energy cutoff.
)
{
    ModuleBase::GlobalFunc::ZEROS(st_length2D, this->fftnxy);
    std::fill(st_bottom2D, st_bottom2D + this->fftnxy, std::numeric_limits<int>::max());

    // determine the scaning area along x-direct, if gamma-only && xprime, only positive axis is used.
    int ix_end = int(this->nx / 2) + 1;
    int ix_start = -ix_end;
    // determine the scaning area along y-direct, if gamma-only && !xprime, only positive axis is used.
    int iy_end = int(this->ny / 2) + 1;
    int iy_start = -iy_end;

    int iz_end = int(this->nz / 2) + 1;
    int iz_start = -iz_end;

    if (this->full_pw)
    {
        ix_end = int(this->nx / 2);
        ix_start = ix_end - this->nx + 1;

        iy_end = int(this->ny / 2);
        iy_start = iy_end - this->ny + 1;

        iz_end = int(this->nz / 2);
        iz_start = iz_end - this->nz + 1;
    }

    if (this->gamma_only)
    {
        if(this->xprime)
        {
            ix_start = 0;
            ix_end = this->fftnx - 1;
        }
        else
        {
            iy_start = 0;
            iy_end = this->fftny - 1;
        }
    }

    this->liy = this->riy = 0;
    this->lix = this->rix = 0;
    this->npwtot = 0;
    this->nstot = 0;
    int npwtot_total = 0;
    int nstot_total = 0;
    int liy_local = 0;
    int riy_local = 0;
    int lix_local = 0;
    int rix_local = 0;

    const int fftny_ = this->fftny;
    const int nx_ = this->nx;
    const int ny_ = this->ny;
    const int iy_range = iy_end - iy_start + 1;

    const EnergyCutoffCriterion default_criterion(this->ggecut, this->GGT, this->full_pw);
    const IPWCriterion* criterion_ptr = (criterion == nullptr) ? &default_criterion : criterion;

    struct StickRecord
    {
        int order;
        int index;
        int length;
        int bottom;
    };

#ifdef _OPENMP
    const int max_threads = omp_get_max_threads();
#else
    const int max_threads = 1;
#endif
    std::vector<std::vector<StickRecord>> stick_records(max_threads);

    // OpenMP parallelization: the ix and iy loops are collapsed into a single
    // iteration space and distributed among threads. Each thread accumulates
    // its findings into a private StickRecord buffer selected by thread id. The
    // buffers are merged and replayed in the original serial scan order after
    // the parallel loop to keep st_length2D and st_bottom2D deterministic.
    // Scalar counters and x/y bounds use OpenMP reductions; f, length and
    // bottom are loop-private, while criterion_ptr only performs const reads.
#ifdef _OPENMP
#pragma omp parallel for collapse(2) reduction(+ : npwtot_total, nstot_total) reduction(min : riy_local, rix_local) reduction(max : liy_local, lix_local)
#endif
    for (int ix = ix_start; ix <= ix_end; ++ix)
    {
        for (int iy = iy_start; iy <= iy_end; ++iy)
        {
            ModuleBase::Vector3<double> f;
#ifdef _OPENMP
            const int tid = omp_get_thread_num();
#else
            const int tid = 0;
#endif
            auto& records = stick_records[tid];

            // we shift all sticks to the first quadrant in x-y plane here.
            // (ix, iy, iz) is the direct coordinates of planewaves.
            // x and y is the coordinates of shifted sticks in x-y plane.
            // for example, if fftny = fftnx = 10, we will shift the stick on (-1, 2) to (9, 2),
            // so that its index in st_length and st_bottom is 9 * 10 + 2 = 92.
            int x = ix;
            int y = iy;
            if (x < 0)
            {
                x += nx_;
            }
            if (y < 0)
            {
                y += ny_;
            }
            int index = x * fftny_ + y;

            int length = 0; // number of planewave on stick (x, y).
            int bottom = std::numeric_limits<int>::max();
            for (int iz = iz_start; iz <= iz_end; ++iz)
            {
                f.x = ix;
                f.y = iy;
                f.z = iz;
                if (criterion_ptr->is_in_sphere(f))
                {
                    if (length == 0)
                    {
                        bottom = iz; // length == 0 means this point is the bottom of stick (x, y).
                    }
                    ++npwtot_total;
                    ++length;
                    if(iy < riy_local)
                    {
                        riy_local = iy;
                    }
                    if(iy > liy_local)
                    {
                        liy_local = iy;
                    }
                    if(ix < rix_local)
                    {
                        rix_local = ix;
                    }
                    if(ix > lix_local)
                    {
                        lix_local = ix;
                    }
                }
            }
            if (length > 0)
            {
                records.push_back({(ix - ix_start) * iy_range + (iy - iy_start), index, length, bottom});
                ++nstot_total;
            }
        }
    }

    std::vector<StickRecord> ordered_records;
    this->npwtot = npwtot_total;
    this->nstot = nstot_total;
    for (int tid = 0; tid < max_threads; ++tid)
    {
        ordered_records.insert(ordered_records.end(), stick_records[tid].begin(), stick_records[tid].end());
    }
    std::sort(ordered_records.begin(), ordered_records.end(), [](const StickRecord& a, const StickRecord& b) {
        return a.order < b.order;
    });
    for (const StickRecord& stick : ordered_records)
    {
        st_length2D[stick.index] = stick.length;
        st_bottom2D[stick.index] = stick.bottom;
    }

    for (int ixy = 0; ixy < this->fftnxy; ++ixy)
    {
        if (st_length2D[ixy] == 0)
        {
            st_bottom2D[ixy] = 0;
        }
    }
    this->riy = riy_local;
    this->liy = liy_local;
    this->rix = rix_local;
    this->lix = lix_local;
    riy += this->ny;
    rix += this->nx;
    return;
}

/**
 * @brief (5) Construct ig2isz, and is2fftixy.
 *
 *  is2fftixy contains the x-coordinate and y-coordinate of sticks on current core.
 *  ig2isz contains the z-coordinate of planewaves on current core.
 *  We will scan all the sticks and find the planewaves on them, then store the information into ig2isz and is2fftixy.
 *
 * @param in: this->nstot, st_bottom2D, st_length2D
 * @param out: ig2isz, is2fftixy
 */

void PW_Basis::get_ig2isz_is2fftixy(
    int* st_bottom2D,     // minimum z of stick, stored in 1d array with this->nstot elements.
    int* st_length2D     // the stick on (x, y) consists of st_length[x*fftny+y] planewaves.
)
{
    if (this->npw == 0)
    {
        delete[] this->ig2isz; this->ig2isz = nullptr; // map ig to the z coordinate of this planewave.
        delete[] this->is2fftixy; this->is2fftixy = nullptr; // map is (index of sticks) to ixy (iy + ix * fftny).
        this->invalidate_cache();
#if defined(__CUDA) || defined(__ROCM)
        if (this->device == "gpu") {
            delmem_int_op()(this->d_is2fftixy);
            d_is2fftixy = nullptr;
        }
#endif
        return;
    }

    delete[] this->ig2isz; this->ig2isz = new int[this->npw]; // map ig to the z coordinate of this planewave.
    ModuleBase::GlobalFunc::ZEROS(this->ig2isz, this->npw);
    delete[] this->is2fftixy; this->is2fftixy = new int[this->nst]; // map is (index of sticks) to ixy (iy + ix * fftny).
    for (int is = 0; is < this->nst; ++is)
    {
        this->is2fftixy[is] = -1;
    }

    int st_move = 0; // this is the st_move^th stick on current core.
    int pw_filled = 0; // how many current core's planewaves have been found.
    for (int ixy = 0; ixy < this->fftnxy; ++ixy)
    {
        if (this->fftixy2ip[ixy] == this->poolrank)
        {
            int zstart = st_bottom2D[ixy];
            for (int iz = zstart; iz < zstart + st_length2D[ixy]; ++iz)
            {
                int z = iz;
                if (z < 0)
                {
                    z += this->nz;
                }
                this->ig2isz[pw_filled] = st_move * this->nz + z;
                pw_filled++;
            }
            this->is2fftixy[st_move] = ixy;
            st_move++;
            if (xprime && ixy / fftny == 0)
            {
                ng_xeq0 = pw_filled;
            }
        }
        if (st_move == this->nst && pw_filled == this->npw)
        {
            break;
        }
    }
    std::vector<int> ig2ixyz(this->npw);
    for (int igl = 0; igl < this->npw; ++igl)
    {
        int isz = this->ig2isz[igl];
        int iz = isz % this->nz;
        int is = isz / this->nz;
        int ixy = this->is2fftixy[is];
        int iy = ixy % this->ny;
        int ix = ixy / this->ny;
        ig2ixyz[igl] = iz + iy * nz + ix * ny * nz;
    }
#if defined(__CUDA) || defined(__ROCM)
    if (this->device == "gpu") {
        resmem_int_op()(d_is2fftixy, this->nst);
        syncmem_int_h2d_op()(this->d_is2fftixy, this->is2fftixy, this->nst);
        resmem_int_op()(ig2ixyz_gpu,this->npw);
        syncmem_int_h2d_op()(ig2ixyz_gpu, ig2ixyz.data(), this->npw);
    }
#endif
    this->invalidate_cache();
    return;
}
} // namespace ModulePW
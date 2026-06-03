#ifndef THREAD_AFFINITY_H
#define THREAD_AFFINITY_H
/**
 * @file thread_affinity.h
 * @brief Lightweight CPU-affinity helpers for OpenMP parallel regions.
 *
 * Pinning OpenMP threads to consecutive physical cores improves cache
 * locality for the pack/unpack loops in pw_gatherscatter.h because the
 * working sets are per-stick / per-plane and re-accessed within the same
 * NUMA domain.
 *
 * Usage:
 *   // At the top of a parallel region or before an omp parallel for:
 *   ModuleThread::pin_thread_to_core(omp_get_thread_num());
 *
 * The implementation is deliberately minimal — it does NOT depend on
 * libnuma or hwloc.  It uses sched_setaffinity (Linux) and assumes a
 * flat contiguous core numbering.  This is correct for the vast majority
 * of HPC cluster nodes.
 */

#if defined(__linux__) && defined(_OPENMP)
#include <sched.h>
#include <omp.h>
#include <unistd.h>

namespace ModuleThread
{

/**
 * @brief Pin the calling thread to the given logical core id.
 *
 * @param core_id  Zero-based logical core index.  Typically
 *                 omp_get_thread_num() so that threads 0..N-1
 *                 map to cores 0..N-1.
 * @return 0 on success, -1 on failure (errno is set by sched_setaffinity).
 */
inline int pin_thread_to_core(int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

/**
 * @brief Pin all OpenMP threads to consecutive physical cores.
 *
 * Call this once before a performance-critical parallel region.
 * It is idempotent within a single parallel region (each thread
 * sets its own affinity).
 */
inline void pin_all_omp_threads()
{
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        pin_thread_to_core(tid);
    }
}

} // namespace ModuleThread

#else // non-Linux or no OpenMP — provide no-op stubs

namespace ModuleThread
{
inline int  pin_thread_to_core(int)              { return 0; }
inline void pin_all_omp_threads()                {}
} // namespace ModuleThread

#endif // __linux__ && _OPENMP
#endif // THREAD_AFFINITY_H

#include "../pw_basis.h"
#include "pw_test.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <vector>

namespace
{
class PWCountPwStTestBasis : public ModulePW::PW_Basis
{
  public:
    using ModulePW::PW_Basis::count_pw_st;
};

class BoxCriterion : public ModulePW::IPWCriterion
{
  public:
    bool is_in_sphere(const ModuleBase::Vector3<double>& g) const override
    {
        return g.x >= -1.0 && g.x <= 1.0 && g.y >= -1.0 && g.y <= 1.0 && g.z >= -1.0 && g.z <= 1.0;
    }
};

void setup_basis(PWCountPwStTestBasis& pw, const int nx, const int ny, const int nz, const double ggecut)
{
    pw.nx = nx;
    pw.ny = ny;
    pw.nz = nz;
    pw.fftnx = nx;
    pw.fftny = ny;
    pw.fftnz = nz;
    pw.fftnxy = nx * ny;
    pw.gamma_only = false;
    pw.full_pw = false;
    pw.xprime = true;
    pw.ggecut = ggecut;
    pw.GGT = ModuleBase::Matrix3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
}

void run_count(PWCountPwStTestBasis& pw,
               std::vector<int>& st_length,
               std::vector<int>& st_bottom,
               const ModulePW::IPWCriterion* criterion = nullptr)
{
    st_length.assign(pw.fftnxy, -1);
    st_bottom.assign(pw.fftnxy, -1);
    pw.count_pw_st(st_length.data(), st_bottom.data(), criterion);
}
} // namespace

TEST_F(PWTEST, count_pw_st_energy_cutoff_small_grid)
{
    PWCountPwStTestBasis pw;
    setup_basis(pw, 5, 5, 5, 1.0);

    std::vector<int> st_length;
    std::vector<int> st_bottom;
    run_count(pw, st_length, st_bottom);

    EXPECT_EQ(pw.npwtot, 7);
    EXPECT_EQ(pw.nstot, 5);
    EXPECT_EQ(st_length[0 * pw.fftny + 0], 3);
    EXPECT_EQ(st_bottom[0 * pw.fftny + 0], -1);
    EXPECT_EQ(st_length[1 * pw.fftny + 0], 1);
    EXPECT_EQ(st_bottom[1 * pw.fftny + 0], 0);
    EXPECT_EQ(st_length[4 * pw.fftny + 0], 1);
    EXPECT_EQ(st_bottom[4 * pw.fftny + 0], 0);
    EXPECT_EQ(st_length[2 * pw.fftny + 2], 0);
    EXPECT_EQ(st_bottom[2 * pw.fftny + 2], 0);
}

TEST_F(PWTEST, count_pw_st_injected_criterion)
{
    PWCountPwStTestBasis pw;
    setup_basis(pw, 5, 5, 5, 0.0);
    const BoxCriterion criterion;

    std::vector<int> st_length;
    std::vector<int> st_bottom;
    run_count(pw, st_length, st_bottom, &criterion);

    EXPECT_EQ(pw.npwtot, 27);
    EXPECT_EQ(pw.nstot, 9);
    EXPECT_EQ(st_length[0 * pw.fftny + 0], 3);
    EXPECT_EQ(st_bottom[0 * pw.fftny + 0], -1);
    EXPECT_EQ(st_length[1 * pw.fftny + 1], 3);
    EXPECT_EQ(st_bottom[1 * pw.fftny + 1], -1);
    EXPECT_EQ(st_length[4 * pw.fftny + 4], 3);
    EXPECT_EQ(st_bottom[4 * pw.fftny + 4], -1);
    EXPECT_EQ(st_length[2 * pw.fftny + 2], 0);
    EXPECT_EQ(st_bottom[2 * pw.fftny + 2], 0);
}

TEST_F(PWTEST, count_pw_st_parallel_matches_serial)
{
    PWCountPwStTestBasis pw;
    setup_basis(pw, 7, 6, 5, 4.0);

    std::vector<int> serial_length;
    std::vector<int> serial_bottom;
    std::vector<int> parallel_length;
    std::vector<int> parallel_bottom;

#ifdef _OPENMP
    const int old_threads = omp_get_max_threads();
    omp_set_num_threads(1);
#endif
    run_count(pw, serial_length, serial_bottom);
    const int serial_npwtot = pw.npwtot;
    const int serial_nstot = pw.nstot;

#ifdef _OPENMP
    omp_set_num_threads(std::min(4, std::max(1, old_threads)));
#endif
    run_count(pw, parallel_length, parallel_bottom);

    EXPECT_EQ(pw.npwtot, serial_npwtot);
    EXPECT_EQ(pw.nstot, serial_nstot);
    EXPECT_EQ(parallel_length, serial_length);
    EXPECT_EQ(parallel_bottom, serial_bottom);

#ifdef _OPENMP
    omp_set_num_threads(old_threads);
#endif
}

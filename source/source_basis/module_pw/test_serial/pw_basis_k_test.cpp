#include "gtest/gtest.h"
#include "source_base/global_function.h"
#include "source_base/constants.h"
#include "source_base/matrix3.h"
#include "source_base/timer.h"
#include <chrono>
#include <cstdlib>
#include <map>
#include <sstream>
#include <vector>

/************************************************
 *  serial unit test of functions in pw_basis.cpp
 ***********************************************/

/**
 * - Tested Functions:
 *   - Constructor
 *     - PW_Basis_K() and ~PW_Basis_K()
 *   - Initgrids1
 *     - initgrids() from gridecut, derived from class PW_Basis
 *   - Initgrids2
 *     - initgrids() from nx,ny,nz, derived from class PW_Basis
 *   - Initparameters
 *     - initparameters(), including k coordinates
 *   - SetupTransform
 *     - setuptransform(): for fft transform
 *   - CollectLocalPW
 *     - collect_local_pw: get gk2, gcar for local npw plane waves
 */

#define protected public
#define private public
#include "../pw_basis_k.h"
#include "../pw_basis.h"
#include "../pw_gatherscatter.h"
#undef private
#undef protected

#include <algorithm>
#include <cmath>
#include <complex>

class PWBasisKTEST: public testing::Test
{
public:
	std::string precision_double = "double";
	std::string precision_single = "single";
	std::string device_flag = "cpu";
	ModulePW::PW_Basis_K basis_k;
};

namespace
{
std::string gkey(const ModuleBase::Vector3<double>& g)
{
	std::ostringstream os;
	os << static_cast<int>(g.x) << "," << static_cast<int>(g.y) << "," << static_cast<int>(g.z);
	return os.str();
}

ModuleBase::Vector3<double> negate_g(const ModuleBase::Vector3<double>& g)
{
	return ModuleBase::Vector3<double>(-g.x, -g.y, -g.z);
}

void setup_gamma_compare_basis(ModulePW::PW_Basis_K& basis, const bool gamma_only)
{
	const double lat0 = 2.0;
	const ModuleBase::Matrix3 latvec(1.0, 0.0, 0.0,
	                                 0.0, 1.0, 0.0,
	                                 0.0, 0.0, 1.0);
	const int nx = 18;
	const int ny = 18;
	const int nz = 18;
	const double gk_ecut = 18.0;
	const int nks = 1;
	const ModuleBase::Vector3<double> kvec_d[1] = {{0.0, 0.0, 0.0}};
	const int distribution_type = 1;
	const bool xprime = true;

	basis.initgrids(lat0, latvec, nx, ny, nz);
	basis.initparameters(gamma_only, gk_ecut, nks, kvec_d, distribution_type, xprime);
	basis.setuptransform();
	basis.collect_local_pw();
}

std::map<std::string, int> make_g_to_index(const ModulePW::PW_Basis_K& basis)
{
	std::map<std::string, int> index;
	for (int ig = 0; ig < basis.npwk[0]; ++ig)
	{
		index[gkey(basis.getgdirect(0, ig))] = ig;
	}
	return index;
}
}

TEST_F(PWBasisKTEST,Constructor)
{
	ModulePW::PW_Basis_K basis_k2(device_flag, precision_double);
	EXPECT_EQ(basis_k2.classname,"PW_Basis_K");
	EXPECT_EQ(basis_k2.device,"cpu");
	EXPECT_EQ(basis_k2.fft_bundle.device,"cpu");
	EXPECT_EQ(basis_k2.precision,"double");
	EXPECT_EQ(basis_k2.fft_bundle.precision,"double");
	ModulePW::PW_Basis_K basis_k3(device_flag, precision_single);
	EXPECT_EQ(basis_k3.precision,"single");
	EXPECT_EQ(basis_k3.fft_bundle.precision,"single");
}

TEST_F(PWBasisKTEST,Initgrids1)
{
	double lat0 = 1.8897261254578281;
	ModuleBase::Matrix3 latvec(10.0,0.0,0.0,
				0.0,10.0,0.0,
				0.0,0.0,10.0);
	double gridecut=10.0;
	basis_k.initgrids(lat0,latvec,gridecut);
	EXPECT_DOUBLE_EQ(basis_k.lat0,lat0);
	EXPECT_DOUBLE_EQ(basis_k.tpiba,ModuleBase::TWO_PI/lat0);
	EXPECT_DOUBLE_EQ(basis_k.tpiba2,basis_k.tpiba*basis_k.tpiba);
	EXPECT_DOUBLE_EQ(basis_k.latvec.e11,latvec.e11);
	EXPECT_DOUBLE_EQ(basis_k.GT.e11,latvec.Inverse().e11);
	EXPECT_DOUBLE_EQ(basis_k.G.e11,basis_k.GT.Transpose().e11);
	EXPECT_DOUBLE_EQ(basis_k.GGT.e11,(basis_k.G*basis_k.GT).e11);
	EXPECT_DOUBLE_EQ(basis_k.gridecut_lat,gridecut/basis_k.tpiba2);
	EXPECT_NEAR(basis_k.gridecut_lat,0.904561,1e-4);
	EXPECT_EQ(basis_k.nx,20);
	EXPECT_EQ(basis_k.ny,20);
	EXPECT_EQ(basis_k.nz,20);
	EXPECT_TRUE(basis_k.nx%2==0 || basis_k.nx%3==0 || basis_k.nx%5==0);
	EXPECT_TRUE(basis_k.ny%2==0 || basis_k.ny%3==0 || basis_k.ny%5==0);
	EXPECT_TRUE(basis_k.nz%2==0 || basis_k.nz%3==0 || basis_k.nz%5==0);
}

TEST_F(PWBasisKTEST,Initgrids2)
{
	double lat0 = 1.8897261254578281;
	ModuleBase::Matrix3 latvec(10.0,0.0,0.0,
				0.0,10.0,0.0,
				0.0,0.0,10.0);
	int nx_in = 20;
	int ny_in = 20;
	int nz_in = 20;
	basis_k.initgrids(lat0,latvec,nx_in,ny_in,nz_in);
	EXPECT_DOUBLE_EQ(basis_k.lat0,lat0);
	EXPECT_DOUBLE_EQ(basis_k.tpiba,ModuleBase::TWO_PI/lat0);
	EXPECT_DOUBLE_EQ(basis_k.tpiba2,basis_k.tpiba*basis_k.tpiba);
	EXPECT_DOUBLE_EQ(basis_k.latvec.e11,latvec.e11);
	EXPECT_DOUBLE_EQ(basis_k.GT.e11,latvec.Inverse().e11);
	EXPECT_DOUBLE_EQ(basis_k.G.e11,basis_k.GT.Transpose().e11);
	EXPECT_DOUBLE_EQ(basis_k.GGT.e11,(basis_k.G*basis_k.GT).e11);
	EXPECT_EQ(basis_k.nx,nx_in);
	EXPECT_EQ(basis_k.ny,ny_in);
	EXPECT_EQ(basis_k.nz,nz_in);
	EXPECT_NEAR(basis_k.gridecut_lat,0.999999,1e-4);
	EXPECT_NEAR(basis_k.gridecut_lat*basis_k.tpiba2,11.0551,1e-4);
}

TEST_F(PWBasisKTEST, Initparameters) 
{
	ModulePW::PW_Basis_K basis_k(device_flag, precision_single);
	double lat0 = 1.8897261254578281;
	ModuleBase::Matrix3 latvec(10.0,0.0,0.0,
				0.0,10.0,0.0,
				0.0,0.0,10.0);
	int nx_in = 20;
	int ny_in = 20;
	int nz_in = 20;
	basis_k.initgrids(lat0,latvec,nx_in,ny_in,nz_in);
	const bool gamma_only_in = true;
	const double gk_ecut_in = 2.0;
	const int nks_in = 3;
	const ModuleBase::Vector3<double> kvec_d_in[3] = { {0.0, 0.0, 0.0}, {0.1, 0.2, 0.3}, {0.4, 0.5, 0.6} };
	const int distribution_type_in = 1;
	const bool xprime_in = true;	
	basis_k.initparameters(gamma_only_in, gk_ecut_in, nks_in,kvec_d_in, distribution_type_in, xprime_in);	
	EXPECT_EQ(basis_k.nks, nks_in);	
	EXPECT_NE(basis_k.kvec_d, nullptr);
	for(int i=0; i<nks_in; i++) {
	    EXPECT_EQ(basis_k.kvec_d[i], kvec_d_in[i]);
	}	
	EXPECT_NE(basis_k.kvec_c, nullptr);
	for(int i=0; i<nks_in; i++) {
	    EXPECT_EQ(basis_k.kvec_c[i], kvec_d_in[i] * basis_k.G);
	}	
	EXPECT_GT(basis_k.gk_ecut, 0.0);
	EXPECT_GT(basis_k.ggecut, 0.0);
	EXPECT_LE(basis_k.ggecut, basis_k.gridecut_lat);	
	EXPECT_FALSE(basis_k.gamma_only);
	EXPECT_EQ(basis_k.xprime, xprime_in);	
	if(basis_k.gamma_only) {
	    EXPECT_EQ(basis_k.fftny, basis_k.ny);
	    EXPECT_EQ(basis_k.fftnx, int(basis_k.nx / 2) + 1);
	} else {
	    EXPECT_EQ(basis_k.fftny, basis_k.ny);
	    EXPECT_EQ(basis_k.fftnx, basis_k.nx);
	}	
	EXPECT_EQ(basis_k.fftnz, basis_k.nz);
	EXPECT_EQ(basis_k.fftnxy, basis_k.fftnx * basis_k.fftny);
	EXPECT_EQ(basis_k.fftnxyz, basis_k.fftnxy * basis_k.fftnz);	
	EXPECT_EQ(basis_k.distribution_type, distribution_type_in);
}

TEST_F(PWBasisKTEST, SetupTransform) 
{
	ModulePW::PW_Basis_K basis_k(device_flag, precision_double);
	double lat0 = 1.8897261254578281;
	ModuleBase::Matrix3 latvec(10.0,0.0,0.0,
				0.0,10.0,0.0,
				0.0,0.0,10.0);
	double gridecut=10.0;
	basis_k.initgrids(lat0,latvec,gridecut);
	const bool gamma_only_in = true;
	const double gk_ecut_in = 10.0;
	const int nks_in = 3;
	const ModuleBase::Vector3<double> kvec_d_in[3] = { {0.0, 0.0, 0.0}, {0.1, 0.2, 0.3}, {0.4, 0.5, 0.6} };
	const int distribution_type_in = 1;
	const bool xprime_in = true;	
	basis_k.initparameters(gamma_only_in, gk_ecut_in, nks_in,kvec_d_in, distribution_type_in, xprime_in);	
	EXPECT_NO_THROW(basis_k.setuptransform());
	EXPECT_EQ(basis_k.npw,3695);
}

TEST_F(PWBasisKTEST, CollectLocalPW) 
{
	ModulePW::PW_Basis_K basis_k(device_flag, precision_double);
	double lat0 = 1.8897261254578281;
	ModuleBase::Matrix3 latvec(10.0,0.0,0.0,
				0.0,10.0,0.0,
				0.0,0.0,10.0);
	double gridecut=10.0;
	basis_k.initgrids(lat0,latvec,gridecut);
	const bool gamma_only_in = true;
	const double gk_ecut_in = 11.0;
	const int nks_in = 3;
	const ModuleBase::Vector3<double> kvec_d_in[3] = { {0.0, 0.0, 0.0}, {0.1, 0.2, 0.3}, {0.4, 0.5, 0.6} };
	const int distribution_type_in = 1;
	const bool xprime_in = true;	
	basis_k.initparameters(gamma_only_in, gk_ecut_in, nks_in,kvec_d_in, distribution_type_in, xprime_in);	
	EXPECT_NO_THROW(basis_k.setuptransform());
	basis_k.reset_k_cache_stats();
	EXPECT_NO_THROW(basis_k.collect_local_pw());
	ASSERT_GT(basis_k.npwk[0], 0);
	auto* gk2_ptr = basis_k.get_gk2_data<double>();
	auto* gcar_ptr = basis_k.get_gcar_data<double>();
	const double gk2_sample = basis_k.getgk2(0,0);
	const auto stats_after_build = basis_k.get_k_cache_stats();
	EXPECT_EQ(stats_after_build.gcar_misses, 1);
	EXPECT_EQ(stats_after_build.gk2_misses, 1);
	EXPECT_NO_THROW(basis_k.collect_local_pw());
	EXPECT_EQ(basis_k.get_gk2_data<double>(), gk2_ptr);
	EXPECT_EQ(basis_k.get_gcar_data<double>(), gcar_ptr);
	EXPECT_DOUBLE_EQ(basis_k.getgk2(0,0), gk2_sample);
	EXPECT_NO_THROW(basis_k.collect_local_pw(1.0, 0.5, 0.2));
	EXPECT_EQ(basis_k.get_gcar_data<double>(), gcar_ptr);
	const auto stats_after_hits = basis_k.get_k_cache_stats();
	EXPECT_EQ(stats_after_hits.gcar_hits, 2);
	EXPECT_EQ(stats_after_hits.gcar_misses, 1);
	EXPECT_EQ(stats_after_hits.gk2_hits, 1);
	EXPECT_EQ(stats_after_hits.gk2_misses, 2);
	EXPECT_GT(stats_after_hits.cache_bytes, 0);
	basis_k.initparameters(gamma_only_in, gk_ecut_in, nks_in, kvec_d_in, distribution_type_in, xprime_in);
	EXPECT_EQ(basis_k.gcar, nullptr);
	EXPECT_EQ(basis_k.gk2, nullptr);
	EXPECT_EQ(basis_k.k_gcar_cache_storage, nullptr);
	EXPECT_EQ(basis_k.k_gk2_cache_storage, nullptr);
	EXPECT_EQ(basis_k.get_gcar_data<double>(), nullptr);
	EXPECT_EQ(basis_k.get_gk2_data<double>(), nullptr);
	EXPECT_EQ(basis_k.get_k_cache_stats().cache_bytes, 0);
	EXPECT_EQ(basis_k.npw,3695);
	EXPECT_EQ(basis_k.npwk_max,2721);
}

TEST_F(PWBasisKTEST, CollectLocalPWRecordsTimers)
{
	ModuleBase::timer::timer_pool.clear();
	ModulePW::PW_Basis_K basis_k(device_flag, precision_double);
	double lat0 = 1.8897261254578281;
	ModuleBase::Matrix3 latvec(10.0,0.0,0.0,
				0.0,10.0,0.0,
				0.0,0.0,10.0);
	double gridecut = 10.0;
	basis_k.initgrids(lat0, latvec, gridecut);
	const bool gamma_only_in = true;
	const double gk_ecut_in = 11.0;
	const int nks_in = 3;
	const ModuleBase::Vector3<double> kvec_d_in[3] = { {0.0, 0.0, 0.0}, {0.1, 0.2, 0.3}, {0.4, 0.5, 0.6} };
	const int distribution_type_in = 1;
	const bool xprime_in = true;
	basis_k.initparameters(gamma_only_in, gk_ecut_in, nks_in, kvec_d_in, distribution_type_in, xprime_in);
	basis_k.setuptransform();
	basis_k.collect_local_pw();
	const auto& timer_pool = ModuleBase::timer::timer_pool[basis_k.classname];
	EXPECT_TRUE(timer_pool.count("collect_local_pw"));
	EXPECT_GE(timer_pool.at("collect_local_pw").calls, 1u);
}

TEST_F(PWBasisKTEST, ComplexTransformRoundTrip)
{
	ModulePW::PW_Basis_K basis_k(device_flag, precision_double);
	double lat0 = 2.0;
	ModuleBase::Matrix3 latvec(1.0,0.0,1.0,
				0.0,2.0,0.0,
				0.0,0.0,2.0);
	double gridecut = 30.0;
	const bool gamma_only_in = false;
	const double gk_ecut_in = 20.0;
	const int nks_in = 1;
	const ModuleBase::Vector3<double> kvec_d_in[1] = { {0.0, 0.0, 0.0} };
	const int distribution_type_in = 2;
	const bool xprime_in = false;

	basis_k.initgrids(lat0, latvec, gridecut);
	basis_k.initparameters(gamma_only_in, gk_ecut_in, nks_in, kvec_d_in, distribution_type_in, xprime_in);
	ASSERT_NO_THROW(basis_k.setuptransform());
	ASSERT_NE(basis_k.npwk, nullptr);
	ASSERT_GT(basis_k.npwk[0], 0);

	// Use reciprocal-space input because arbitrary real-space data is projected
	// by the plane-wave cutoff and is not exactly recoverable.
	std::vector<std::complex<double>> recip_in(basis_k.npwk[0]);
	std::vector<std::complex<double>> real_space(basis_k.nrxx);
	std::vector<std::complex<double>> recip_out(basis_k.npwk[0]);
	for (int ig = 0; ig < basis_k.npwk[0]; ++ig)
	{
		const double real_part = (ig % 17 - 8) / 11.0;
		const double imag_part = (ig % 19 - 9) / 13.0;
		recip_in[ig] = std::complex<double>(real_part, imag_part);
	}

	basis_k.recip2real(recip_in.data(), real_space.data(), 0);
	basis_k.real2recip(real_space.data(), recip_out.data(), 0);

	for (int ig = 0; ig < basis_k.npwk[0]; ++ig)
	{
		EXPECT_NEAR(recip_in[ig].real(), recip_out[ig].real(), 1e-10);
		EXPECT_NEAR(recip_in[ig].imag(), recip_out[ig].imag(), 1e-10);
	}
}

TEST_F(PWBasisKTEST, GammaRealForwardMatchesFullComplex)
{
	ModulePW::PW_Basis_K full_basis(device_flag, precision_double);
	ModulePW::PW_Basis_K gamma_basis(device_flag, precision_double);
	setup_gamma_compare_basis(full_basis, false);
	setup_gamma_compare_basis(gamma_basis, true);

	ASSERT_FALSE(full_basis.gamma_only);
	ASSERT_TRUE(gamma_basis.gamma_only);
	ASSERT_EQ(full_basis.nrxx, gamma_basis.nrxx);
	ASSERT_GT(gamma_basis.npwk[0], 0);

	std::vector<double> real_in(full_basis.nrxx);
	std::vector<std::complex<double>> complex_real_in(full_basis.nrxx);
	for (int ir = 0; ir < full_basis.nrxx; ++ir)
	{
		real_in[ir] = std::sin(0.17 * ir) + 0.25 * std::cos(0.31 * ir);
		complex_real_in[ir] = {real_in[ir], 0.0};
	}

	std::vector<std::complex<double>> full_out(full_basis.npwk[0]);
	std::vector<std::complex<double>> gamma_out(gamma_basis.npwk[0]);
	full_basis.real2recip(complex_real_in.data(), full_out.data(), 0);
	gamma_basis.real2recip(real_in.data(), gamma_out.data(), 0);

	const auto full_index = make_g_to_index(full_basis);
	for (int ig = 0; ig < gamma_basis.npwk[0]; ++ig)
	{
		const auto found = full_index.find(gkey(gamma_basis.getgdirect(0, ig)));
		ASSERT_NE(found, full_index.end()) << gkey(gamma_basis.getgdirect(0, ig));
		EXPECT_NEAR(full_out[found->second].real(), gamma_out[ig].real(), 1e-10);
		EXPECT_NEAR(full_out[found->second].imag(), gamma_out[ig].imag(), 1e-10);
	}
}

TEST_F(PWBasisKTEST, GammaProjectedInverseMatchesFullComplex)
{
	ModulePW::PW_Basis_K full_basis(device_flag, precision_double);
	ModulePW::PW_Basis_K gamma_basis(device_flag, precision_double);
	setup_gamma_compare_basis(full_basis, false);
	setup_gamma_compare_basis(gamma_basis, true);

	std::vector<double> real_in(full_basis.nrxx);
	std::vector<std::complex<double>> complex_real_in(full_basis.nrxx);
	for (int ir = 0; ir < full_basis.nrxx; ++ir)
	{
		real_in[ir] = std::sin(0.17 * ir) + 0.25 * std::cos(0.31 * ir);
		complex_real_in[ir] = {real_in[ir], 0.0};
	}

	std::vector<std::complex<double>> full_in(full_basis.npwk[0]);
	std::vector<std::complex<double>> gamma_in(gamma_basis.npwk[0]);
	std::vector<std::complex<double>> full_real(full_basis.nrxx);
	std::vector<std::complex<double>> gamma_real(gamma_basis.nrxx);
	full_basis.real2recip(complex_real_in.data(), full_in.data(), 0);
	gamma_basis.real2recip(real_in.data(), gamma_in.data(), 0);
	full_basis.recip2real(full_in.data(), full_real.data(), 0);
	gamma_basis.recip2real(gamma_in.data(), gamma_real.data(), 0);

	double max_real_error = 0.0;
	double max_full_imag = 0.0;
	double max_gamma_imag = 0.0;
	for (int ir = 0; ir < full_basis.nrxx; ++ir)
	{
		max_real_error = std::max(max_real_error, std::abs(full_real[ir].real() - gamma_real[ir].real()));
		max_full_imag = std::max(max_full_imag, std::abs(full_real[ir].imag()));
		max_gamma_imag = std::max(max_gamma_imag, std::abs(gamma_real[ir].imag()));
	}
	EXPECT_LT(max_real_error, 1e-10);
	EXPECT_LT(max_full_imag, 1e-10);
	EXPECT_LT(max_gamma_imag, 1e-10);
}

TEST_F(PWBasisKTEST, GammaComplexRealSpaceInputIsNotFullComplexEquivalent)
{
	ModulePW::PW_Basis_K full_basis(device_flag, precision_double);
	ModulePW::PW_Basis_K gamma_basis(device_flag, precision_double);
	setup_gamma_compare_basis(full_basis, false);
	setup_gamma_compare_basis(gamma_basis, true);

	std::vector<std::complex<double>> complex_in(full_basis.nrxx);
	for (int ir = 0; ir < full_basis.nrxx; ++ir)
	{
		complex_in[ir] = {std::sin(0.11 * ir), std::cos(0.07 * ir)};
	}

	std::vector<std::complex<double>> full_out(full_basis.npwk[0]);
	std::vector<std::complex<double>> gamma_out(gamma_basis.npwk[0]);
	full_basis.real2recip(complex_in.data(), full_out.data(), 0);
	gamma_basis.real2recip(complex_in.data(), gamma_out.data(), 0);

	const auto full_index = make_g_to_index(full_basis);
	double max_error = 0.0;
	for (int ig = 0; ig < gamma_basis.npwk[0]; ++ig)
	{
		const auto found = full_index.find(gkey(gamma_basis.getgdirect(0, ig)));
		ASSERT_NE(found, full_index.end()) << gkey(gamma_basis.getgdirect(0, ig));
		max_error = std::max(max_error, std::abs(full_out[found->second] - gamma_out[ig]));
	}
	EXPECT_GT(max_error, 1e-4);
}

TEST_F(PWBasisKTEST, CopyComplexBufferTimerBenchmark)
{
	if (std::getenv("ABACUS_PW_SIMD_TIMER_TEST") == nullptr)
	{
		GTEST_SKIP() << "Set ABACUS_PW_SIMD_TIMER_TEST=1 to run the copy timer benchmark.";
	}

	const int count = 1 << 20;
	const int repeats = 64;
	std::vector<std::complex<double>> src(count);
	std::vector<std::complex<double>> copy_n_dst(count);
	std::vector<std::complex<double>> scalar_dst(count);

	for (int i = 0; i < count; ++i)
	{
		src[i] = std::complex<double>((i % 97) / 17.0, (i % 89) / 19.0);
	}

	volatile double checksum = 0.0;

	const auto copy_n_start = std::chrono::steady_clock::now();
	for (int repeat = 0; repeat < repeats; ++repeat)
	{
		ModulePW::detail::copy_complex_buffer(src.data(), copy_n_dst.data(), count);
		checksum += copy_n_dst[repeat].real();
	}
	const auto copy_n_end = std::chrono::steady_clock::now();

	const auto scalar_start = std::chrono::steady_clock::now();
	for (int repeat = 0; repeat < repeats; ++repeat)
	{
		for (int i = 0; i < count; ++i)
		{
			scalar_dst[i] = src[i];
		}
		checksum += scalar_dst[repeat].imag();
	}
	const auto scalar_end = std::chrono::steady_clock::now();

	const double copy_n_time = std::chrono::duration<double>(copy_n_end - copy_n_start).count();
	const double scalar_time = std::chrono::duration<double>(scalar_end - scalar_start).count();
	const double bytes_moved = static_cast<double>(count) * sizeof(std::complex<double>) * repeats;
	const double gib = bytes_moved / (1024.0 * 1024.0 * 1024.0);

	std::cout << "PW_SIMD_TEST copy_n_helper " << copy_n_time << " s, "
	          << gib / copy_n_time << " GiB/s\n";
	std::cout << "PW_SIMD_TEST scalar_loop " << scalar_time << " s, "
	          << gib / scalar_time << " GiB/s\n";
	std::cout << "PW_SIMD_TEST speedup copy_n/scalar " << scalar_time / copy_n_time
	          << ", checksum " << checksum << "\n";

	ASSERT_EQ(copy_n_dst, scalar_dst);
}

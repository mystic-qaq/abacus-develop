#include "../compact_gamma_data.h"
#include "../pw_basis.h"
#include "../pw_basis_k.h"

#include <chrono>
#include <complex>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
using ModulePW::CompactGammaData;
using Complex = std::complex<double>;

TEST(CompactGammaData, CompressDecompressWithExplicitMinusGMap)
{
    const int ng = 7;
    const int minus_g[ng] = {0, 2, 1, 4, 3, 6, 5};
    const std::vector<std::complex<double>> dense = {
        {10.0, 3.0},
        {1.0, 2.0},
        {1.0, -2.0},
        {-3.0, 4.0},
        {-3.0, -4.0},
        {5.0, -6.0},
        {5.0, 6.0},
    };

    CompactGammaData<double> compact(ng, minus_g);
    compact.compress_from(dense.data());

    std::vector<std::complex<double>> restored(ng);
    compact.decompress_to(restored.data());

    EXPECT_EQ(compact.compact_size(), 4);
    EXPECT_DOUBLE_EQ(compact.get(0).imag(), 0.0); // G=0 is self-conjugate and stored real.
    EXPECT_DOUBLE_EQ(compact.get(1).real(), dense[1].real());
    EXPECT_DOUBLE_EQ(compact.get(1).imag(), dense[1].imag());
    EXPECT_DOUBLE_EQ(compact.get(2).real(), dense[2].real());
    EXPECT_DOUBLE_EQ(compact.get(2).imag(), dense[2].imag());
    EXPECT_GT(compact.memory_saving_ratio(), 0.40);

    EXPECT_DOUBLE_EQ(restored[0].real(), dense[0].real());
    EXPECT_DOUBLE_EQ(restored[0].imag(), 0.0);
    for (int ig = 1; ig < ng; ++ig)
    {
        EXPECT_DOUBLE_EQ(restored[ig].real(), dense[ig].real());
        EXPECT_DOUBLE_EQ(restored[ig].imag(), dense[ig].imag());
    }
}

TEST(CompactGammaData, DefaultHalfByIndexFormatHandlesOddAndEvenSizes)
{
    for (const int ng : {0, 1, 2, 5, 8})
    {
        CompactGammaData<float> compact(ng);
        EXPECT_EQ(compact.logical_size(), ng);
        EXPECT_EQ(compact.compact_size(), ng == 0 ? 0 : 1 + ng / 2);
        EXPECT_LE(compact.compact_bytes(), compact.dense_bytes());
        for (int ig = 0; ig < ng; ++ig)
        {
            compact.set_representative(ig, std::complex<float>(ig + 1.0F, ig == 0 ? 2.0F : -1.0F));
            (void)compact.get(ig);
        }
    }
}

TEST(CompactGammaData, StaticPerformanceCountersAreAvailable)
{
    const int ng = 1025;
    std::vector<std::complex<double>> dense(ng);
    for (int ig = 0; ig < ng; ++ig)
    {
        dense[ig] = std::complex<double>(ig, -ig);
    }

    const auto t0 = std::chrono::high_resolution_clock::now();
    auto compact = ModulePW::compress_gamma_data(dense.data(), ng);
    std::vector<std::complex<double>> restored(ng);
    ModulePW::decompress_gamma_data(compact, restored.data());
    const auto t1 = std::chrono::high_resolution_clock::now();

    EXPECT_EQ(compact.compact_size(), 513);
    EXPECT_EQ(compact.dense_bytes(), static_cast<std::size_t>(ng) * sizeof(std::complex<double>));
    EXPECT_EQ(compact.compact_bytes(), static_cast<std::size_t>(513) * sizeof(std::complex<double>));
    EXPECT_GE(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count(), 0);
}

TEST(CompactGammaData, PWTransformCompactInterfacesAreAvailable)
{
    auto rho_to_real = static_cast<void (ModulePW::PW_Basis::*)(const CompactGammaData<double>&,
                                                                double*,
                                                                bool,
                                                                double) const>(&ModulePW::PW_Basis::recip2real_compact<double>);
    auto real_to_rho = static_cast<void (ModulePW::PW_Basis::*)(const double*,
                                                                CompactGammaData<double>&,
                                                                bool,
                                                                double) const>(&ModulePW::PW_Basis::real2recip_compact<double>);
    auto wfc_to_real = static_cast<void (ModulePW::PW_Basis_K::*)(const CompactGammaData<double>&,
                                                                  double*,
                                                                  int,
                                                                  bool,
                                                                  double) const>(&ModulePW::PW_Basis_K::recip2real_compact<double>);
    auto real_to_wfc = static_cast<void (ModulePW::PW_Basis_K::*)(const double*,
                                                                  CompactGammaData<double>&,
                                                                  int,
                                                                  bool,
                                                                  double) const>(&ModulePW::PW_Basis_K::real2recip_compact<double>);

    EXPECT_NE(rho_to_real, nullptr);
    EXPECT_NE(real_to_rho, nullptr);
    EXPECT_NE(wfc_to_real, nullptr);
    EXPECT_NE(real_to_wfc, nullptr);

    std::vector<Complex> dense = {{2.0, 1.0}, {3.0, 4.0}, {3.0, -4.0}};
    ModulePW::PW_Basis pw;
    auto compact = pw.compress_gamma_only_data(dense.data());
    EXPECT_EQ(compact.logical_size(), pw.npw);
}

TEST(CompactGammaData, PWBasisExplicitMinusGMapEnablesAutomaticGammaPath)
{
    ModulePW::PW_Basis pw;
    pw.gamma_only = true;
    pw.npw = 5;

    // 用 new[] 分配，匹配 PW_Basis 析构时的 delete[]
    pw.gdirect = new ModuleBase::Vector3<double>[5];
    pw.gdirect[0] = ModuleBase::Vector3<double>(0.0, 0.0, 0.0);
    pw.gdirect[1] = ModuleBase::Vector3<double>(1.0, 0.0, 0.0);
    pw.gdirect[2] = ModuleBase::Vector3<double>(-1.0, 0.0, 0.0);
    pw.gdirect[3] = ModuleBase::Vector3<double>(0.0, 2.0, 0.0);
    pw.gdirect[4] = ModuleBase::Vector3<double>(0.0, -2.0, 0.0);

    EXPECT_TRUE(pw.can_use_gamma_only_compact());
    const auto minus_g = pw.gamma_only_minus_g_map();
    ASSERT_EQ(minus_g.size(), 5);
    EXPECT_EQ(minus_g[0], 0);
    EXPECT_EQ(minus_g[1], 2);
    EXPECT_EQ(minus_g[2], 1);
    EXPECT_EQ(minus_g[3], 4);
    EXPECT_EQ(minus_g[4], 3);

    std::vector<Complex> dense = {{1.0, 8.0}, {2.0, 3.0}, {2.0, -3.0}, {4.0, -5.0}, {4.0, 5.0}};
    const auto compact = pw.compress_gamma_only_data(dense.data());
    std::vector<Complex> restored(pw.npw);
    pw.decompress_gamma_only_data(compact, restored.data());

    EXPECT_LT(compact.compact_bytes(), compact.dense_bytes());
    EXPECT_DOUBLE_EQ(restored[0].imag(), 0.0);
    EXPECT_EQ(restored[1], dense[1]);
    EXPECT_EQ(restored[2], dense[2]);
    EXPECT_EQ(restored[3], dense[3]);
    EXPECT_EQ(restored[4], dense[4]);
}

TEST(CompactGammaData, PWBasisAutomaticGammaPathFallsBackWithoutLocalMinusGPair)
{
    ModulePW::PW_Basis pw;
    pw.gamma_only = true;
    pw.npw = 2;

    pw.gdirect = new ModuleBase::Vector3<double>[2];
    pw.gdirect[0] = ModuleBase::Vector3<double>(0.0, 0.0, 0.0);
    pw.gdirect[1] = ModuleBase::Vector3<double>(1.0, 0.0, 0.0);

    EXPECT_FALSE(pw.can_use_gamma_only_compact());
    EXPECT_TRUE(pw.gamma_only_minus_g_map().empty());
}

TEST(CompactGammaData, ElecStatePWGammaOnlyChargePathUsesCompactHelperStatically)
{
    std::ifstream source("source/source_estate/elecstate_pw.cpp");
    ASSERT_TRUE(source.good());

    const std::string code((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    EXPECT_NE(code.find("can_compact_smooth_rhog = PARAM.inp.gamma_only"), std::string::npos);
    EXPECT_NE(code.find("PARAM.inp.device == \"cpu\""), std::string::npos);
    EXPECT_NE(code.find("rhopw_smooth->can_use_gamma_only_compact()"), std::string::npos);
    EXPECT_NE(code.find("rhopw_smooth->real2recip_compact"), std::string::npos);
    EXPECT_NE(code.find("charge->rhopw->recip2real_compact"), std::string::npos);
    EXPECT_NE(code.find("charge->rhopw->recip2real(this->rhog"), std::string::npos);
}

} // namespace
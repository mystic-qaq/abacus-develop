#include "read_input.h"
#include "read_input_tool.h"
#include "source_base/global_variable.h"
#include "source_base/tool_quit.h"
#include "source_io/module_parameter/parameter.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
namespace ModuleIO
{
namespace
{
bool is_pw_gamma_only_kpt(const std::string& kpoint_file)
{
    std::ifstream ifs(kpoint_file);
    if (!ifs.good())
    {
        return true;
    }

    std::string header;
    int nks = 0;
    std::string mode;
    if (!(ifs >> header >> nks >> mode))
    {
        return false;
    }
    std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

    if (nks == 0)
    {
        int nkx = 0;
        int nky = 0;
        int nkz = 0;
        double sx = 0.0;
        double sy = 0.0;
        double sz = 0.0;
        if (!(ifs >> nkx >> nky >> nkz >> sx >> sy >> sz))
        {
            return false;
        }
        return mode == "gamma" && nkx == 1 && nky == 1 && nkz == 1 && std::abs(sx) < 1e-12
               && std::abs(sy) < 1e-12 && std::abs(sz) < 1e-12;
    }

    for (int ik = 0; ik < nks; ++ik)
    {
        double kx = 0.0;
        double ky = 0.0;
        double kz = 0.0;
        double weight = 0.0;
        if (!(ifs >> kx >> ky >> kz >> weight))
        {
            return false;
        }
        if (std::abs(kx) > 1e-12 || std::abs(ky) > 1e-12 || std::abs(kz) > 1e-12)
        {
            return false;
        }
    }
    return true;
}
} // namespace

/// @note Here para.inp has been synchronized of all ranks.
///       All para.inp have the same value.
void ReadInput::set_globalv(const Input_para& inp, System_para& sys)
{
    /// caculate the gamma_only_pw and gamma_only_local
    if (inp.gamma_only)
    {
        if (inp.basis_type == "lcao")
        {
            sys.gamma_only_local = true;
        }
        else if (inp.basis_type == "pw")
        {
            sys.gamma_only_pw = true;
            if (!is_pw_gamma_only_kpt(inp.kpoint_file))
            {
                GlobalV::ofs_running << " WARNING : PW GammaOnly currently requires a single Gamma k-point; "
                                     << "fallback to full-complex plane-wave grids." << std::endl;
                sys.gamma_only_pw = false;
            }
        }
    }
    if (sys.gamma_only_local)
    {
        if (inp.esolver_type == "tddft")
        {
            GlobalV::ofs_running << " WARNING : gamma_only is not applicable for tddft" << std::endl;
            sys.gamma_only_local = false;
        }
    }
    if (sys.gamma_only_pw)
    {
        if (inp.esolver_type == "tddft")
        {
            GlobalV::ofs_running << " WARNING : gamma_only is not applicable for tddft" << std::endl;
            sys.gamma_only_pw = false;
        }
        else if (inp.ks_solver != "cg")
        {
            GlobalV::ofs_running
                << " WARNING : PW GammaOnly is currently validated only with ks_solver=cg; "
                << "fallback to full-complex plane-wave grids." << std::endl;
            sys.gamma_only_pw = false;
        }
    }
    /// set deepks_setorb
    if (inp.deepks_scf || inp.deepks_out_labels == 1)
    {
        sys.deepks_setorb = true;
    }
    /// set the noncolin and lspinorb from nspin
    switch (inp.nspin)
    {
    case 4:
        if (inp.noncolin)
        {
            sys.domag = true;
            sys.domag_z = false;
        }
        else
        {
            sys.domag = false;
            sys.domag_z = true;
        }
        sys.npol = 2;
        break;
    case 2:
    case 1:
        sys.domag = false;
        sys.domag_z = false;
        sys.npol = 1;
    default:
        break;
    }
    sys.nqx = static_cast<int>((sqrt(inp.ecutwfc) / sys.dq + 4.0) * inp.cell_factor);
    sys.nqxq = static_cast<int>((sqrt(inp.ecutrho) / sys.dq + 4.0) * inp.cell_factor);
    /// set ncx,ncy,ncz
    sys.ncx = inp.nx;
    sys.ncy = inp.ny;
    sys.ncz = inp.nz;
#ifdef __MPI
    Parallel_Common::bcast_bool(sys.double_grid);
#endif
    /// set ks_run
    if (inp.ks_solver != "bpcg" && inp.bndpar > 1)
    {
        sys.all_ks_run = false;
    }
    // set the has_double_data and has_float_data
#ifdef __ENABLE_FLOAT_FFTW
    bool float_cond = inp.cal_cond && inp.esolver_type == "sdft";
#else
    bool float_cond = false;
#endif
    sys.has_double_data = (inp.precision == "double") || (inp.precision == "mixing") || float_cond;
    sys.has_float_data = (inp.precision == "single") || (inp.precision == "mixing") || float_cond;
}

/// @note Here para.inp has not been synchronized of all ranks.
///       Only para.inp in rank 0 is right.
///       So we need to broadcast the results to all ranks.
void ReadInput::set_global_dir(const Input_para& inp, System_para& sys)
{
    /// caculate the global output directory
    const std::string prefix = "OUT.";
    sys.global_out_dir = prefix + inp.suffix + "/";
    sys.global_out_dir = to_dir(sys.global_out_dir);

    /// get the global output directory
    sys.global_stru_dir = sys.global_out_dir + "STRU/";
    sys.global_stru_dir = to_dir(sys.global_stru_dir);

    /// get the global output directory
    sys.global_matrix_dir = sys.global_out_dir + "matrix/";
    sys.global_matrix_dir = to_dir(sys.global_matrix_dir);

    /// get the global output directory
    sys.global_wfc_dir = sys.global_out_dir + "WFC/";
    sys.global_wfc_dir = to_dir(sys.global_wfc_dir);

    /// get the global ML KEDF descriptor directory
    sys.global_mlkedf_descriptor_dir = sys.global_out_dir + "MLKEDF_Descriptors/";
    sys.global_mlkedf_descriptor_dir = to_dir(sys.global_mlkedf_descriptor_dir);

    /// get the global directory for DeePKS labels during electronic steps
    sys.global_deepks_label_elec_dir = sys.global_out_dir + "DeePKS_Labels_Elec/";
    sys.global_deepks_label_elec_dir = to_dir(sys.global_deepks_label_elec_dir);

    /// get the global readin directory
    sys.global_readin_dir = inp.read_file_dir;
    sys.global_readin_dir = to_dir(sys.global_readin_dir);

    /// get the stru file for md restart case
    if (inp.calculation == "md" && inp.mdp.md_restart)
    {
        int istep = current_md_step(sys.global_readin_dir);

        if (inp.read_file_dir == to_dir("OUT." + inp.suffix))
        {
            sys.global_in_stru = sys.global_stru_dir + "STRU_MD_" + std::to_string(istep);
        }
        else
        {
            sys.global_in_stru = inp.read_file_dir + "STRU_MD_" + std::to_string(istep);
        }
    }
    else
    {
        sys.global_in_stru = inp.stru_file;
    }

    // set the global log file
    bool out_alllog = inp.out_alllog;
    // set the global calculation type
    std::string cal_type = inp.calculation;
#ifdef __MPI
    // because log_file is different for each rank, so we need to bcast the out_alllog
    Parallel_Common::bcast_bool(out_alllog);
    // In `ReadInput::read_parameters`, `bcastfunc(param)` is after `set_global_dir`,
    // so `cal_type` must be synchronized here manually
    Parallel_Common::bcast_string(cal_type);
#endif
    if (out_alllog)
    {
        PARAM.sys.log_file = "running_" + cal_type + "_" + std::to_string(PARAM.sys.myrank + 1) + ".log";
    }
    else
    {
        PARAM.sys.log_file = "running_" + cal_type + ".log";
    }
#ifdef __MPI
    Parallel_Common::bcast_string(sys.global_in_card);
    Parallel_Common::bcast_string(sys.global_out_dir);
    Parallel_Common::bcast_string(sys.global_readin_dir);
    Parallel_Common::bcast_string(sys.global_stru_dir);
    Parallel_Common::bcast_string(sys.global_matrix_dir);
    Parallel_Common::bcast_string(sys.global_wfc_dir);
    Parallel_Common::bcast_string(sys.global_in_stru);
#endif
}
} // namespace ModuleIO

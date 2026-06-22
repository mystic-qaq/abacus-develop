#include "gamma_compact.h"
#include "pw_basis.h"

#include <cassert>
#include <map>

namespace ModulePW
{

void GammaCompact::initialize(const PW_Basis* pw_basis)
{
    assert(pw_basis != nullptr);
    assert(pw_basis->gamma_only == true);
    assert(pw_basis->ig2isz != nullptr);
    assert(pw_basis->is2fftixy != nullptr);

    npw_compact_ = pw_basis->npw;
    const int nx = pw_basis->nx;
    const int ny = pw_basis->ny;
    const int nz = pw_basis->nz;
    const int fftny = pw_basis->fftny;
    const bool xprime = pw_basis->xprime;
    const int* ig2isz = pw_basis->ig2isz;
    const int* is2fftixy = pw_basis->is2fftixy;

    auto to_signed = [](int c, const int n) -> int
    {
        if (c >= n / 2 + 1)
        {
            c -= n;
        }
        return c;
    };
    auto neg_signed = [&to_signed](const int c, const int n) -> int
    {
        return to_signed(((-c % n) + n) % n, n);
    };

    // Build a map from signed (ix, iy, iz) to compact ig.
    // Key: ((ix + offset) | ((iy + offset) << 20) | ((iz + offset) << 40))
    const int64_t offset = static_cast<int64_t>(std::max({nx, ny, nz})) + 2;
    auto make_key = [offset](int ix, int iy, int iz) -> int64_t
    {
        return (static_cast<int64_t>(ix + offset))
               | (static_cast<int64_t>(iy + offset) << 20)
               | (static_cast<int64_t>(iz + offset) << 40);
    };

    // Lambda: G is self-conjugate iff G == -G modulo FFT grid
    // i.e., each coordinate c satisfies (-c mod N) == (c mod N)
    auto is_self_conjugate_g = [nx, ny, nz](int ix, int iy, int iz) -> bool
    {
        auto is_self = [](int c, int N) -> bool
        {
            int mod_c = ((c % N) + N) % N;
            int mod_neg_c = ((-c % N) + N) % N;
            return mod_c == mod_neg_c;
        };
        return is_self(ix, nx) && is_self(iy, ny) && is_self(iz, nz);
    };

    // Step 1: extract signed coords, build coord->ig map, detect self-conjugate
    self_conj_.resize(npw_compact_, false);
    std::vector<int> gx(npw_compact_), gy(npw_compact_), gz(npw_compact_);
    std::map<int64_t, int> coord_to_ig;

    for (int ig = 0; ig < npw_compact_; ++ig)
    {
        int isz = ig2isz[ig];
        int iz = isz % nz;
        int is = isz / nz;
        int ixy = is2fftixy[is];
        int ix = ixy / fftny;
        int iy = ixy % fftny;

        // Convert from first-quadrant shifted to signed coordinates
        if (ix >= nx / 2 + 1) { ix -= nx; }
        if (iy >= ny / 2 + 1) { iy -= ny; }
        if (iz >= nz / 2 + 1) { iz -= nz; }

        gx[ig] = ix;
        gy[ig] = iy;
        gz[ig] = iz;
        self_conj_[ig] = is_self_conjugate_g(ix, iy, iz);

        int64_t key = make_key(ix, iy, iz);
        coord_to_ig[key] = ig;
    }
    num_self_conj_ = 0;
    for (int ig = 0; ig < npw_compact_; ++ig)
    {
        if (self_conj_[ig]) { ++num_self_conj_; }
    }

    // Step 2: Build compact-to-full mapping, dynamically sizing npw_full.
    // For each canonical (compact) G:
    //   - self-conjugate: 1 slot  (no partner needed)
    //   - non-self-conjugate, partner NOT canonical: 2 slots (canonical + partner)
    //   - non-self-conjugate, partner IS canonical: 1 slot (partner gets own slot)
    c2f_.assign(npw_compact_, -1);

    int full_idx = 0;
    for (int ig = 0; ig < npw_compact_; ++ig)
    {
        // Assign canonical slot
        c2f_[ig] = full_idx++;

        if (!self_conj_[ig])
        {
            // Check if -G is also in the compact set.
        int neg_ix = neg_signed(gx[ig], nx);
        int neg_iy = neg_signed(gy[ig], ny);
        int neg_iz = neg_signed(gz[ig], nz);
        int64_t neg_key = make_key(neg_ix, neg_iy, neg_iz);
        auto it = coord_to_ig.find(neg_key);
        const bool neg_is_compact_global = xprime ? (neg_ix >= 0) : (neg_iy >= 0);

            if (it == coord_to_ig.end() && !neg_is_compact_global)
            {
                // -G is not canonical; reserve an extra slot for it.
                ++full_idx;
            }
            // else: -G is canonical → it already has (or will have) its own
            // canonical slot assigned in its own iteration.
        }
    }
    npw_full_ = full_idx;

    // Step 3: Build f2c_ and conj_of_ arrays.
    f2c_.assign(npw_full_, -1);
    conj_of_.assign(npw_full_, -1);
    conjugate_weight_.assign(npw_compact_, 1.0);
    compact_conj_.assign(npw_compact_, -1);

    for (int ig = 0; ig < npw_compact_; ++ig)
    {
        int canonical_full = c2f_[ig];
        f2c_[canonical_full] = ig;

        if (self_conj_[ig])
        {
            conj_of_[canonical_full] = -1;
            compact_conj_[ig] = ig;
        }
        else
        {
            int neg_ix = neg_signed(gx[ig], nx);
            int neg_iy = neg_signed(gy[ig], ny);
            int neg_iz = neg_signed(gz[ig], nz);
            int64_t neg_key = make_key(neg_ix, neg_iy, neg_iz);
            auto it = coord_to_ig.find(neg_key);
            const bool neg_is_compact_global = xprime ? (neg_ix >= 0) : (neg_iy >= 0);

            if (it != coord_to_ig.end())
            {
                // Both G and -G are canonical — they point to each other.
                int conj_compact_ig = it->second;
                int conj_full = c2f_[conj_compact_ig];
                conj_of_[canonical_full] = conj_full;
                conjugate_weight_[ig] = 1.0;
                compact_conj_[ig] = conj_compact_ig;
            }
            else if (neg_is_compact_global)
            {
                // The conjugate compact G exists globally but is owned by
                // another MPI rank. This local rank must not double-count it.
                conj_of_[canonical_full] = -1;
                conjugate_weight_[ig] = 1.0;
            }
            else
            {
                // -G is not canonical; its full slot was reserved immediately
                // after the canonical slot.
                int conj_full = canonical_full + 1;
                conj_of_[canonical_full] = conj_full;
                // f2c_[conj_full] stays -1 (non-canonical slot)
                conj_of_[conj_full] = canonical_full;
                conjugate_weight_[ig] = 2.0;
            }
        }
    }

    initialized_ = true;
}

} // namespace ModulePW

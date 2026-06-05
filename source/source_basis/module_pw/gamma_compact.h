#ifndef GAMMA_COMPACT_H
#define GAMMA_COMPACT_H

#include <complex>
#include <vector>
#include <cstring>

namespace ModulePW
{

class PW_Basis; // forward declaration

/**
 * @brief Lightweight half-spectrum helper for GammaOnly plane-wave storage.
 *
 * When gamma_only is active (all k-points are Gamma points), real-space
 * functions satisfy f(r) = f*(r), so their Fourier components obey
 *   F(-G) = conj(F(G))
 *
 * This class provides:
 *   - Self-conjugate detection (G == -G modulo FFT grid):
 *       g=0, Nyquist-plane components whose imaginary parts are identically zero.
 *   - Conjugate weight: 1.0 for self-conjugate, 2.0 for regular G.
 *     This replaces ad-hoc `fact = 2.0` scattered across production code.
 *   - expand_to_full / pack_from_full: convert between half-spectrum (compact)
 *     layout and full G-space layout (both +/-G explicitly stored).
 *     Useful for verification against full-complex results and for operations
 *     that genuinely need the full spectrum.
 *
 * The half-spectrum layout is exactly the existing `npw`-sized array produced
 * by PW_Basis with gamma_only=true (only the canonical half of the G-sphere,
 * where ix>=0 when xprime=true or iy>=0 when xprime=false).
 *
 * The full layout size satisfies:
 *   npw_full <= 2 * npw_compact - num_self_conjugate
 *
 * where equality holds when no two canonical G vectors are conjugates of
 * each other. When both G and -G happen to be canonical (e.g., ix=0 plane
 * in xprime=true mode), they share a single conjugate relation and
 * npw_full is correspondingly smaller.
 */
class GammaCompact
{
  public:
    GammaCompact() = default;
    ~GammaCompact() = default;

    /**
     * @brief Initialize from a PW_Basis that has gamma_only=true.
     * @param pw_basis  Pointer to an initialized PW_Basis (with gamma_only=true).
     *
     * Builds the half-spectrum metadata: which G vectors are self-conjugate,
     * their conjugate weights, and the expand/pack index mappings.
     * Must be called after distribute_g() has set up ig2isz and is2fftixy.
     */
    void initialize(const PW_Basis* pw_basis);

    /// Whether this GammaCompact has been initialized.
    bool is_initialized() const { return initialized_; }

    /// Number of plane waves in the (existing) half-spectrum layout.
    int npw_compact() const { return npw_compact_; }

    /// Number of plane waves in the expanded full-spectrum layout.
    int npw_full() const { return npw_full_; }

    /// Number of self-conjugate G vectors (g=0, Nyquist boundaries).
    int num_self_conjugate() const { return num_self_conj_; }

    /**
     * @brief Check if a compact-index G vector is self-conjugate.
     * @param ig  Compact index in [0, npw_compact).
     */
    bool is_self_conjugate(int ig) const
    {
        return ig >= 0 && ig < npw_compact_ && self_conj_[ig];
    }

    /**
     * @brief Get the conjugate weight for a compact-index G vector.
     * @param ig  Compact index in [0, npw_compact).
     * @return 1.0 for self-conjugate G, 2.0 for regular G.
     *
     * When summing a real-symmetric quantity over all G, each regular G pair
     * {G, -G} contributes twice, while self-conjugate G contributes once.
     * Multiply by this weight to account correctly.
     */
    double conjugate_weight(int ig) const
    {
        return self_conj_[ig] ? 1.0 : 2.0;
    }

    /**
     * @brief Expand half-spectrum data to full G-space.
     *
     * @tparam FPTYPE  float or double.
     * @param compact  [npw_compact] half-spectrum data (existing PW_Basis layout).
     * @param full     [npw_full] output: full G-space data with explicit +/-G pairs.
     *                 For non-self-conjugate G at compact index ic:
     *                   full[compact_to_full(ic)] = compact[ic]
     *                   full[conjugate_of(compact_to_full(ic))] = conj(compact[ic])
     *                 Self-conjugate G: stored once, imaginary part forced to zero.
     */
    template <typename FPTYPE>
    void expand_to_full(const std::complex<FPTYPE>* compact,
                        std::complex<FPTYPE>* full) const;

    /**
     * @brief Pack full G-space data to half-spectrum.
     *
     * @tparam FPTYPE  float or double.
     * @param full     [npw_full] full G-space data.
     * @param compact  [npw_compact] output: half-spectrum data.
     *
     * For each canonical G, its value is extracted from the full array.
     * For self-conjugate G, only the real part is kept (imaginary forced to 0).
     * This is the inverse of expand_to_full().
     */
    template <typename FPTYPE>
    void pack_from_full(const std::complex<FPTYPE>* full,
                        std::complex<FPTYPE>* compact) const;

    /// Map: compact index -> full index.
    int compact_to_full(int ic) const { return c2f_[ic]; }

    /// Map: full index -> compact index (-1 if not a canonical representative).
    int full_to_compact(int ig_full) const
    {
        return (ig_full >= 0 && ig_full < npw_full_) ? f2c_[ig_full] : -1;
    }

    /// Map: full index -> full index of its canonical partner (-1 if self-conjugate).
    int conjugate_of(int ig_full) const
    {
        return (ig_full >= 0 && ig_full < npw_full_) ? conj_of_[ig_full] : -1;
    }

  private:
    bool initialized_ = false;
    int npw_compact_ = 0;                     ///< Half-spectrum plane-wave count (= PW_Basis::npw)
    int npw_full_ = 0;                        ///< Full-spectrum plane-wave count
    int num_self_conj_ = 0;                   ///< Number of self-conjugate G vectors
    std::vector<bool> self_conj_;             ///< [npw_compact] per-compact-index self-conjugate flag
    std::vector<int> c2f_;                    ///< [npw_compact] compact -> full index map
    std::vector<int> f2c_;                    ///< [npw_full] full -> compact index map (-1 = conjugate partner)
    std::vector<int> conj_of_;                ///< [npw_full] full -> full index of canonical partner (-1 = self)
};

// ============================================================================
// Template method implementations (must be in header)
// ============================================================================

template <typename FPTYPE>
void GammaCompact::expand_to_full(const std::complex<FPTYPE>* compact,
                                  std::complex<FPTYPE>* full) const
{
    // Step 1: Fill canonical entries from compact data
    for (int ic = 0; ic < npw_compact_; ++ic)
    {
        int ig_full = c2f_[ic];
        full[ig_full] = static_cast<std::complex<FPTYPE>>(compact[ic]);
        if (self_conj_[ic])
        {
            // Self-conjugate: force imaginary part to zero (should already be
            // zero for physical data, but enforce for numerical stability)
            full[ig_full] = std::complex<FPTYPE>(full[ig_full].real(), 0.0);
        }
    }
    // Step 2: Fill non-canonical (conjugate-only) entries. These are full-index
    // slots where f2c_ == -1 and conj_of_ points to the canonical partner.
    for (int ig_full = 0; ig_full < npw_full_; ++ig_full)
    {
        if (f2c_[ig_full] == -1 && conj_of_[ig_full] >= 0)
        {
            // This is a conjugate-only slot: copy conj(canonical)
            full[ig_full] = std::conj(full[conj_of_[ig_full]]);
        }
    }
}

template <typename FPTYPE>
void GammaCompact::pack_from_full(const std::complex<FPTYPE>* full,
                                  std::complex<FPTYPE>* compact) const
{
    for (int ic = 0; ic < npw_compact_; ++ic)
    {
        int ig_full = c2f_[ic];
        compact[ic] = static_cast<std::complex<FPTYPE>>(full[ig_full]);
        if (self_conj_[ic])
        {
            // Self-conjugate: keep only real part
            compact[ic] = std::complex<FPTYPE>(compact[ic].real(), 0.0);
        }
    }
}

} // namespace ModulePW

#endif // GAMMA_COMPACT_H

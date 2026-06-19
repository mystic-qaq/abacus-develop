#ifndef MODULE_PW_COMPACT_GAMMA_DATA_H
#define MODULE_PW_COMPACT_GAMMA_DATA_H

#include <algorithm>
#include <complex>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ModulePW
{

/**
 * @brief Compact reciprocal-space storage for gamma-only data.
 *
 * For charge density, local potential and Gamma-point wave functions the
 * reciprocal coefficients obey F(-G) = conj(F(G)).  CompactGammaData stores
 * one representative of each conjugate pair and reconstructs the missing side
 * on demand.  The class is intentionally independent from FFT internals, so
 * existing PW_Basis/PW_Basis_K transform interfaces can keep accepting the
 * traditional dense reciprocal arrays while callers that keep data between
 * transforms can store it in this compact format.
 *
 * @tparam FPTYPE Floating-point type (e.g., double, float).
 *
 * @param[in] logical_size     Number of G-vectors in the dense reciprocal
 *                              array.  Determines the size of the logical
 *                              (full) view.
 * @param[in] minus_g_index    Optional array of length @p logical_size that
 *                              maps each G-index to its negative-G partner.
 *                              If nullptr or omitted, a deterministic
 *                              half-by-index map is used (suitable for unit
 *                              tests and symmetrically arranged data).
 * @param[in] dense            Pointer to a dense reciprocal-space array of
 *                              length @p logical_size.  Must not be nullptr
 *                              unless size is zero.
 * @param[out] dense           Destination buffer for decompression.  Must have
 *                              room for @p logical_size elements.
 *
 * Mapping convention:
 * - rep_index_[ig] is the compact representative index of logical G index ig.
 * - need_conj_[ig] tells whether logical value ig is conj(data[rep_index_[ig]]).
 * - self-conjugate coefficients, e.g. G=0, are represented by themselves and
 *   are forced to be real during compression.
 *
 * If no explicit minus-G map is supplied, a deterministic half-by-index map is
 * used.  This keeps the helper usable for static/unit tests and for callers
 * that already arrange conjugate pairs symmetrically.
 */
template <typename FPTYPE>
class CompactGammaData
{
  public:
    using value_type = std::complex<FPTYPE>;

    CompactGammaData() = default;

    explicit CompactGammaData(const int logical_size)
    {
        this->reset(logical_size);
    }

    CompactGammaData(const int logical_size, const int* minus_g_index)
    {
        this->reset(logical_size, minus_g_index);
    }

    void reset(const int logical_size)
    {
        if (logical_size < 0)
        {
            throw std::invalid_argument("CompactGammaData logical size must be non-negative.");
        }

        this->logical_size_ = logical_size;
        this->rep_index_.assign(logical_size, 0);
        this->need_conj_.assign(logical_size, false);
        this->self_conj_.assign(logical_size, false);

        const int compact_size = logical_size == 0 ? 0 : 1 + logical_size / 2;
        this->data_.assign(compact_size, value_type(0.0, 0.0));

        for (int ig = 0; ig < logical_size; ++ig)
        {
            if (ig == 0 || ig < compact_size)
            {
                this->rep_index_[ig] = ig;
                this->need_conj_[ig] = false;
            }
            else
            {
                const int mate = logical_size - ig;
                this->rep_index_[ig] = std::max(0, mate);
                this->need_conj_[ig] = true;
            }
        }

        if (logical_size > 0)
        {
            this->self_conj_[0] = true; // G = 0 boundary condition.
        }
    }

    void reset(const int logical_size, const int* minus_g_index)
    {
        if (logical_size < 0)
        {
            throw std::invalid_argument("CompactGammaData logical size must be non-negative.");
        }
        if (logical_size > 0 && minus_g_index == nullptr)
        {
            this->reset(logical_size);
            return;
        }

        this->logical_size_ = logical_size;
        this->rep_index_.assign(logical_size, -1);
        this->need_conj_.assign(logical_size, false);
        this->self_conj_.assign(logical_size, false);
        this->data_.clear();

        for (int ig = 0; ig < logical_size; ++ig)
        {
            if (this->rep_index_[ig] >= 0)
            {
                continue;
            }

            const int mate = minus_g_index[ig];
            if (mate < 0 || mate >= logical_size)
            {
                throw std::out_of_range("CompactGammaData minus-G map contains an invalid index.");
            }

            const int rep = static_cast<int>(this->data_.size());
            this->data_.push_back(value_type(0.0, 0.0));

            if (mate == ig)
            {
                this->rep_index_[ig] = rep;
                this->self_conj_[ig] = true;
                continue;
            }

            this->rep_index_[ig] = rep;
            this->need_conj_[ig] = false;
            this->rep_index_[mate] = rep;
            this->need_conj_[mate] = true;
        }
    }

    int logical_size() const { return this->logical_size_; }
    int compact_size() const { return static_cast<int>(this->data_.size()); }

    std::size_t dense_bytes() const
    {
        return static_cast<std::size_t>(this->logical_size_) * sizeof(value_type);
    }

    std::size_t compact_bytes() const
    {
        return this->data_.size() * sizeof(value_type);
    }

    double memory_saving_ratio() const
    {
        const std::size_t dense = this->dense_bytes();
        return dense == 0 ? 0.0 : 1.0 - static_cast<double>(this->compact_bytes()) / static_cast<double>(dense);
    }

    value_type* data() { return this->data_.data(); }
    const value_type* data() const { return this->data_.data(); }

    value_type get(const int ig) const
    {
        this->check_index(ig);
        const value_type value = this->data_[this->rep_index_[ig]];
        return this->need_conj_[ig] ? std::conj(value) : value;
    }

    void set_representative(const int ig, const value_type& value)
    {
        this->check_index(ig);
        this->data_[this->rep_index_[ig]] = this->self_conj_[ig] ? value_type(value.real(), 0.0) : value;
    }

    void compress_from(const value_type* dense)
    {
        if (this->logical_size_ > 0 && dense == nullptr)
        {
            throw std::invalid_argument("CompactGammaData cannot compress from a null dense pointer.");
        }
        for (int ig = 0; ig < this->logical_size_; ++ig)
        {
            if (!this->need_conj_[ig])
            {
                this->data_[this->rep_index_[ig]] = this->self_conj_[ig] ? value_type(dense[ig].real(), 0.0) : dense[ig];
            }
        }
    }

    void decompress_to(value_type* dense) const
    {
        if (this->logical_size_ > 0 && dense == nullptr)
        {
            throw std::invalid_argument("CompactGammaData cannot decompress to a null dense pointer.");
        }
        for (int ig = 0; ig < this->logical_size_; ++ig)
        {
            dense[ig] = this->get(ig);
        }
    }

  private:
    void check_index(const int ig) const
    {
        if (ig < 0 || ig >= this->logical_size_)
        {
            throw std::out_of_range("CompactGammaData logical index is out of range.");
        }
    }

    int logical_size_ = 0;
    std::vector<value_type> data_;
    std::vector<int> rep_index_;
    std::vector<bool> need_conj_;
    std::vector<bool> self_conj_;
};

template <typename FPTYPE>
CompactGammaData<FPTYPE> compress_gamma_data(const std::complex<FPTYPE>* dense,
                                             const int logical_size,
                                             const int* minus_g_index = nullptr)
{
    CompactGammaData<FPTYPE> compact(logical_size, minus_g_index);
    compact.compress_from(dense);
    return compact;
}

template <typename FPTYPE>
void decompress_gamma_data(const CompactGammaData<FPTYPE>& compact, std::complex<FPTYPE>* dense)
{
    compact.decompress_to(dense);
}

} // namespace ModulePW

#endif // MODULE_PW_COMPACT_GAMMA_DATA_H
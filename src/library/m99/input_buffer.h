#pragma once

#include <include/endian.h>
#include <cstdint>
#include <array>
#include <memory>
#include <vector>


namespace maniscalco
{

    using input_buffer_ptr = std::unique_ptr<class input_buffer>;


    class input_buffer
    {
    public:

        static std::size_t constexpr capacity = ((1 << 10) * 8);
        static std::size_t constexpr maxBit_ = ((capacity - sizeof(std::size_t)) << 3);

        input_buffer();

        ~input_buffer() = default;

        bool pop
        (
            std::size_t,
            std::size_t &
        );

        std::size_t pop_bit();

        std::uint8_t const * load
        (
            std::uint8_t const *,
            std::uint8_t const *
        );

        std::size_t get_underflow() const{return (popBit_ - maxBit_);}

    protected:

    private:

        std::size_t                         popBit_;

        std::array<std::uint8_t, capacity>  data_;
    };

} // namespace maniscalco


//======================================================================================================================
inline bool maniscalco::input_buffer::pop
(
    std::size_t codeLength,
    std::size_t & code
)
{
    code = *(std::size_t *)(data_.data() + (popBit_ >> 0x03));
    code = endian_swap<network_order_type, host_order_type>(code);
    code >>= ((sizeof(std::size_t) << 3) - codeLength - (popBit_ & 0x07));
    code &= ((1ull << codeLength) - 1);
    popBit_ += codeLength;
    return (popBit_ >= maxBit_);
}


//======================================================================================================================
inline std::size_t maniscalco::input_buffer::pop_bit
(
)
{
    std::size_t value = data_[popBit_ >> 0x03];
    value >>= (7 - (popBit_ & 0x07));
    ++popBit_;
    return (value & 0x01);
}


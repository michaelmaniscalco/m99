#pragma once

#include <include/endian.h>
#include <cstdint>
#include <array>
#include <memory>
#include <vector>


namespace maniscalco
{

    using output_buffer_ptr = std::unique_ptr<class output_buffer>;


    class output_buffer
    {
    public:

        static std::size_t constexpr capacity = ((1 << 10) * 8);
        static std::size_t constexpr maxBit_ = ((capacity - sizeof(std::size_t)) << 3);

        output_buffer();

        ~output_buffer() = default;

        std::size_t push
        (
            std::size_t,
            std::size_t
        );

        std::size_t get_size() const;

        void operator >>
        (
            std::vector<std::uint8_t> &
        ) const;

        std::size_t get_overflow() const{return (pushBit_ - maxBit_);}

    protected:

    private:

        std::size_t                         pushBit_;

        std::array<std::uint8_t, capacity>  data_;
    };

} // namespace maniscalco


//======================================================================================================================
inline std::size_t maniscalco::output_buffer::push
(
    std::size_t code,
    std::size_t codeLength
)
{
    code &= ((1ull << codeLength) - 1);
    code <<= ((sizeof(std::size_t) << 3) - codeLength - (pushBit_ & 0x07));
    code = endian_swap<host_order_type, network_order_type>(code);
    *(std::size_t *)(data_.data() + (pushBit_ >> 0x03)) |= code;
    return ((pushBit_ += codeLength) > maxBit_);
}


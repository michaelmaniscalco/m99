#include "./input_buffer.h"


//======================================================================================================================
maniscalco::input_buffer::input_buffer
(
):
    popBit_(0),
    data_()
{
}


//======================================================================================================================
std::uint8_t const * maniscalco::input_buffer::load
(
    std::uint8_t const * begin,
    std::uint8_t const * end
)
{
    std::size_t bytesAvailable = std::distance(begin, end);
    if (bytesAvailable > ((maxBit_ + 7) >> 3))
        bytesAvailable = ((maxBit_ + 7) >> 3);
    std::copy(begin, begin + bytesAvailable, data_.data());
    return (begin + bytesAvailable);
}


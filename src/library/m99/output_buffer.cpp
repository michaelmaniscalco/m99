#include "./output_buffer.h"


//======================================================================================================================
maniscalco::output_buffer::output_buffer
(
):
    pushBit_(0),
    data_()
{
}


//======================================================================================================================
std::size_t maniscalco::output_buffer::get_size
(
) const
{
    return (pushBit_ > maxBit_) ? maxBit_ : pushBit_;
}


//======================================================================================================================
void maniscalco::output_buffer::operator >>
(
    std::vector<std::uint8_t> & output
) const
{
    std::size_t bytesToWrite = ((get_size() + 7) >> 3);
    if ((output.size() + bytesToWrite) > output.capacity())
        output.reserve(output.size() + bytesToWrite);
    output.insert(output.end(), data_.begin(), data_.begin() + bytesToWrite);
}


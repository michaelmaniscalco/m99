#include "./output_stream.h"

//======================================================================================================================
maniscalco::output_stream::output_stream
(
):
    buffer_(new output_buffer), 
    buffers_()
{
    buffers_.reserve(16);
}


//======================================================================================================================
maniscalco::output_stream::output_stream
(
    output_stream && other
):
    buffer_(std::move(other.buffer_)),
    buffers_(std::move(other.buffers_))
{
}


//======================================================================================================================
auto maniscalco::output_stream::operator =
(
    output_stream && other
) -> output_stream &
{
    buffer_ = std::move(other.buffer_);
    for (auto & e : other.buffers_)
        buffers_.push_back(std::move(e));
    other.buffers_.clear();
    return *this;
}


//======================================================================================================================
std::size_t maniscalco::output_stream::get_size
(
) const
{
    return ((output_buffer::maxBit_ * buffers_.size()) + buffer_->get_size());
}


//======================================================================================================================
void maniscalco::output_stream::operator >>
(
    std::vector<std::uint8_t> & output
) const
{
    std::uint32_t size = ((get_size() + 7) >> 3);
    if (size)
    {
        if (size < (1 << 7))
        {
            output.push_back((std::uint8_t)size);
        }
        else
        {
            size |= 0x80000000;
            size = endian_swap<host_order_type, network_order_type>(size);
            for (auto i = 0; i < 4; ++i)
            {
                output.push_back(size & 0xff);
                size >>= 8;
            }
        }
        // write the actual data
        for (auto const & e : buffers_)
            *e >> output;
        if (buffer_)
            *buffer_ >> output;
    }
}


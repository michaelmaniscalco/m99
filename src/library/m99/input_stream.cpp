#include "./input_stream.h"
#include <algorithm>


//======================================================================================================================
maniscalco::input_stream::input_stream
(
):
    buffer_(new input_buffer), 
    buffers_()
{
    buffers_.reserve(16);
}


//======================================================================================================================
maniscalco::input_stream::input_stream
(
    input_stream && other
):
    buffer_(std::move(other.buffer_)),
    buffers_(std::move(other.buffers_))
{
}


//======================================================================================================================
auto maniscalco::input_stream::operator =
(
    input_stream && other
) -> input_stream &
{
    buffer_ = std::move(other.buffer_);
    for (auto & e : other.buffers_)
        buffers_.push_back(std::move(e));
    other.buffers_.clear();
    return *this;
}


//======================================================================================================================
std::uint8_t const * maniscalco::input_stream::load
(
    std::uint8_t const * begin,
    std::uint8_t const * end
)
{
    std::uint32_t size = 0;
    if (begin[0] & 0x80)
    {
        size = *(std::uint32_t const *)begin;
        size = endian_swap<network_order_type, host_order_type>(size);
        size &= ~0x80000000;
        begin += 4;
    }
    else
    {
        size = begin[0];
        ++begin;
    }
    end = (begin + size);
    auto current = begin;
    while (current != end) 
    {
        current = buffer_->load(current, end);
        buffers_.push_back(std::move(buffer_));
        buffer_.reset(new input_buffer);
    }
    buffers_.push_back(std::move(buffer_));
    std::reverse(buffers_.begin(), buffers_.end());
    buffer_ = std::move(buffers_.back());
    buffers_.pop_back();
    return current;
}


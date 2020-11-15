#pragma once

#include <library/io.h>

#include <vector>
#include <cstdint>


namespace maniscalco
{

    class m99_decode_stream
    {
    public:

        static auto constexpr stream_direction = io::stream_direction::forward;
        using stream_type = io::push_stream<stream_direction>;
        using packet_type = stream_type::packet_type;

        m99_decode_stream
        (
            buffer b,
            buffer::size_type size
        ):
            stream_
            (
                {.inputHandler_ = [this](){return std::move(packet_);}}
            ),
            packet_(std::move(b), 0, size * 8)
        {
        }

        auto pop
        (
            std::size_t codeLength
        )
        {
            return stream_.pop(codeLength);
        }

        auto pop_bit()
        {
            return stream_.pop_bit();
        }
        
    private:

        io::forward_pop_stream stream_;

        packet_type packet_;

    };
} // namespace maniscalco
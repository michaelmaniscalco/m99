#pragma once

#include "./input_buffer.h"
#include <memory>
#include <vector>
#include <cstdint>


namespace maniscalco
{

    class input_stream
    {
    public:

        input_stream();

        ~input_stream() = default;

        input_stream
        (
            input_stream &&
        );

        input_stream & operator =
        (
            input_stream &&
        );

        std::size_t pop
        (
            std::size_t
        );

        std::size_t pop_bit();

        std::uint8_t const * load
        (
            std::uint8_t const *,
            std::uint8_t const * 
        );

    protected:

    private:

        input_buffer_ptr                 buffer_;

        std::vector<input_buffer_ptr>    buffers_;
    };

} // namespace maniscalco


//======================================================================================================================
inline std::size_t maniscalco::input_stream::pop
(
    std::size_t codeLength
)
{
    std::size_t code = 0;
    if (buffer_->pop(codeLength, code))
    {
        codeLength = buffer_->get_underflow();
        if (!buffers_.empty())
        {
            buffer_ = std::move(buffers_.back());
            buffers_.pop_back();
        }
        if (codeLength)
        {
            std::size_t code2 = 0;
            buffer_->pop(codeLength, code2);
            code = ((code & ~((1ull << codeLength) - 1)) | code2);
        }
    }
    return code;
}


//======================================================================================================================
inline std::size_t maniscalco::input_stream::pop_bit
(
)
{
    return buffer_->pop_bit();
}


#pragma once

#include "./output_buffer.h"
#include <vector>
#include <cstdint>


namespace maniscalco
{


    class output_stream
    {
    public:

        output_stream();

        ~output_stream() = default;

        output_stream
        (
            output_stream &&
        );

        output_stream & operator =
        (
            output_stream &&
        );

        bool empty() const;

        void push
        (
            std::size_t,
            std::size_t
        );

        std::size_t get_size() const;

        void operator >>
        (
            std::vector<std::uint8_t> &
        ) const;

    protected:

    private:

        output_buffer_ptr                 buffer_;

        std::vector<output_buffer_ptr>    buffers_;
    };

} // namespace maniscalco


//======================================================================================================================
inline void maniscalco::output_stream::push
(
    std::size_t code,
    std::size_t codeLength
)
{
    if (buffer_->push(code, codeLength))
    {
        codeLength = buffer_->get_overflow();
        buffers_.push_back(std::move(buffer_));
        buffer_ = output_buffer_ptr(new output_buffer);
        buffer_->push(code, codeLength);
    }
}


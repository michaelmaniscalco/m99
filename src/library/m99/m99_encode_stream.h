#pragma once

#include <library/io.h>

#include <cstdint>
#include <queue>


namespace maniscalco
{

    class m99_encode_stream
    {
    public:

        static auto constexpr stream_direction = io::stream_direction::reverse;
        using stream_type = io::push_stream<stream_direction>;
        using packet_type = stream_type::packet_type;
        using element_type = packet_type;
        using container_type = std::deque<element_type>;
        using iterator = container_type::iterator;
        using const_iterator = container_type::const_iterator;
        using size_type = std::size_t;

        m99_encode_stream();

        template <typename ... T>
        auto push(T && ... args);

        size_type size() const;

        const_iterator begin() const;

        const_iterator end() const;

        void clear();

        void flush();

    //private:

        container_type packets_;
        
        stream_type stream_;

    }; // class m99_encode_stream


} // namespace maniscalco


//=============================================================================
template <typename ... T>
auto maniscalco::m99_encode_stream::push
(
    T && ... args
)
{
    return stream_.push(std::forward<T>(args) ...);
}

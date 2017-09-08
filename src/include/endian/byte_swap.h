#pragma once

#include <cstdint>
#include <byteswap.h>
#include <type_traits>


namespace maniscalco
{

    namespace impl
    {

        template <typename T, typename = void> class integral_byte_swap;

        template <typename T>
        class integral_byte_swap<T, typename std::enable_if<sizeof(T) == sizeof(int8_t)>::type>
        {
        public:
            inline T operator()(T value) const{return value;}
        };

        template <typename T>
        class integral_byte_swap<T, typename std::enable_if<sizeof(T) == sizeof(int16_t)>::type>
        {
        public:
            inline T operator()(T value) const{auto v = static_cast<uint16_t>(value);return static_cast<T>((v >> 8) | (v << 8));}
        };

        template <typename T>
        class integral_byte_swap<T, typename std::enable_if<sizeof(T) == sizeof(int32_t)>::type>
        {
        public:
            inline T operator()(T value) const{return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(value)));}
        };

        template <typename T>
        class integral_byte_swap<T, typename std::enable_if<sizeof(T) == sizeof(int64_t)>::type>
        {
        public:
            inline T operator()(T value) const{return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(value)));}
        };



        template <typename T, typename = void> class byte_swap_impl;


        // specialization for integral types
        template <typename T>
        class byte_swap_impl<T, typename std::enable_if<std::is_integral<T>::value>::type>
        {
        public:
            inline T operator()(T value) const{return integral_byte_swap<T>()(value);}
        };


        // specialization for enums
        template <typename T>
        class byte_swap_impl<T, typename std::enable_if<std::is_enum<T>::value>::type>
        {
        public:
            using underlying_type = typename std::underlying_type<T>::type;
            inline T operator()(T value) const{return static_cast<T>(integral_byte_swap<underlying_type>()(static_cast<underlying_type>(value)));}
        };

    } // namespace impl


    template <typename T>
    static inline T byte_swap
    (
        T
    );

} // namespace maniscalco


//==============================================================================
template <typename T>
static inline T maniscalco::byte_swap
(
    T value
)
{
    return impl::byte_swap_impl<T>()(value);
}


#pragma once

#include <type_traits>


namespace maniscalco
{

    template 
    <
        typename data_type
    >
    using enable_if_endian_underlying_type = 
    typename std::enable_if
    <
        true || //allow any type.  byte_swap_impl must be specialized for that type
        std::is_enum<data_type>::value || 
        std::is_pointer<data_type>::value || 
        std::is_integral<data_type>::value
    >::type;


    template 
    <
        typename, 
        typename, 
        typename = void
    > 
    class endian;

} // namespace maniscalco



#include "./endian_type.h"
#include "./endian_swap.h"


namespace maniscalco
{

    //==============================================================================
    template 
    <
        typename data_type, 
        typename endian_type
    >
    class endian
    <
        data_type, 
        endian_type, 
        enable_if_endian_underlying_type<data_type>
    >
    {
    public:

        using underlying_type = data_type;
        using type = endian_type;

        template <typename, typename, typename> 
        friend class endian;

        endian();

        endian
        (
            endian const &
        );

        endian
        (
            endian &&
        );

        endian
        (
            underlying_type
        );

        endian & operator =
        (
            endian const &
        );

        endian & operator =
        (
            endian &&
        );

        endian & operator =
        (
            underlying_type
        );

        operator underlying_type() const;

        underlying_type get() const;
        
    protected:

    private:

        underlying_type  value_;

    };

    template <typename data_type> using big_endian = endian<data_type, big_endian_type>;
    template <typename data_type> using little_endian = endian<data_type, little_endian_type>;
    template <typename data_type> using network_order = endian<data_type, network_order_type>;
    template <typename data_type> using host_order = endian<data_type, host_order_type>;

    // global operator overloads involving endian types
    template <typename input_type, typename data_type, typename endian_type> inline static bool operator < (input_type a, endian<data_type, endian_type> b){return (a < (data_type)b);}
    template <typename data_type, typename endian_type> inline static bool operator < (endian<data_type, endian_type> a, data_type b){return ((data_type)a < b);}
    template <typename data_type, typename endian_type> inline static bool operator <= (data_type a, endian<data_type, endian_type> b){return (a <= (data_type)b);}
    template <typename data_type, typename endian_type> inline static bool operator <= (endian<data_type, endian_type> a, data_type b){return ((data_type)a <= b);}
    template <typename data_type, typename endian_type> inline static bool operator == (data_type a, endian<data_type, endian_type> b){return (a == (data_type)b);}
    template <typename data_type, typename endian_type> inline static bool operator == (endian<data_type, endian_type> a, data_type b){return ((data_type)a == b);}
    template <typename data_type, typename endian_type> inline static bool operator >= (data_type a, endian<data_type, endian_type> b){return (a >= (data_type)b);}
    template <typename data_type, typename endian_type> inline static bool operator >= (endian<data_type, endian_type> a, data_type b){return ((data_type)a >= b);}
    template <typename data_type, typename endian_type> inline static bool operator > (data_type a, endian<data_type, endian_type> b){return (a > (data_type)b);}
    template <typename data_type, typename endian_type> inline static bool operator > (endian<data_type, endian_type> a, data_type b){return ((data_type)a > b);}
    template <typename data_type, typename endian_type> inline static bool operator != (data_type a, endian<data_type, endian_type> b){return (a != (data_type)b);}
    template <typename data_type, typename endian_type> inline static bool operator != (endian<data_type, endian_type> a, data_type b){return ((data_type)a != b);}

    // static make functions
    template <typename data_type, typename endian_type> big_endian<data_type> make_big_endian(endian<data_type, endian_type>);
    template <typename data_type> big_endian<data_type> make_big_endian(data_type);
    template <typename data_type, typename endian_type> little_endian<data_type> make_little_endian(endian<data_type, endian_type>);
    template <typename data_type> little_endian<data_type> make_little_endian(data_type);
    template <typename data_type, typename endian_type> host_order<data_type> make_host_order(endian<data_type, endian_type>);
    template <typename data_type> host_order<data_type> make_host_order(data_type);
    template <typename data_type, typename endian_type> network_order<data_type> make_network_order(endian<data_type, endian_type>);
    template <typename data_type> network_order<data_type> make_network_order(data_type);

}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::endian
(
):
    value_()
{
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::endian
(
    endian && input
):
    value_(input.value_)
{
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::endian
(
    endian const & input
):
    value_(input.value_)
{
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::endian
(
    data_type input
):
    value_(endian_swap<host_order_type, endian_type>(input))
{
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
auto maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::operator =
(
    endian const & input
) -> endian &
{
    value_ = input.value_;
    return *this;
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
auto maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::operator =
(
    endian && input
) -> endian &
{
    value_ = input.value_;
    return *this;
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
auto maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::operator =
(
    data_type input
) -> endian & 
{
    value_ = endian_swap<host_order_type, endian_type>(input);
    return *this;
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::operator underlying_type
(
) const
{
    return endian_swap<endian_type, host_order_type>(value_);
}


//==============================================================================
template
<
    typename data_type, 
    typename endian_type
>
auto maniscalco::endian<data_type, endian_type, maniscalco::enable_if_endian_underlying_type<data_type>>::get
(
) const -> underlying_type
{
    return endian_swap<endian_type, host_order_type>(value_);
}


//==============================================================================
template <typename T, typename E> 
auto maniscalco::make_big_endian
(
    maniscalco::endian<T, E> value
) -> big_endian<T> 
{
    return big_endian<T>((T)value);
}


//==============================================================================
template <typename T> 
auto maniscalco::make_big_endian
(
    T value
) -> big_endian<T> 
{
    return big_endian<T>((T)value);
}


//==============================================================================
template <typename T, typename E> 
auto maniscalco::make_little_endian
(
    endian<T, E> value
) -> little_endian<T> 
{
    return little_endian<T>((T)value);
}


//==============================================================================
template <typename T> 
auto maniscalco::make_little_endian
(
    T value
) -> little_endian<T> 
{
    return little_endian<T>((T)value);
}


//==============================================================================
template <typename T, typename E> 
auto maniscalco::make_host_order
(
    endian<T, E> value
) -> host_order<T> 
{
    return host_order<T>((T)value);
}


//==============================================================================
template <typename T> 
auto maniscalco::make_host_order
(
    T value
) -> host_order<T> 
{
    return host_order<T>((T)value);
}


//==============================================================================
template <typename T, typename E> 
auto maniscalco::make_network_order
(
    endian<T, E> value
) -> network_order<T> 
{
    return network_order<T>((T)value);
}


//==============================================================================
template <typename T> 
auto maniscalco::make_network_order
(
    T value
) -> network_order<T> 
{
    return network_order<T>((T)value);
}


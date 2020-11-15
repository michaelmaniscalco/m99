#include "./m99_encode_stream.h"


//=============================================================================
maniscalco::m99_encode_stream::m99_encode_stream
(
):
    stream_({
        .bufferOutputHandler_ = [this](packet_type packet)
        {
            packets_.emplace_front(std::move(packet));
        },
        .bufferAllocationHandler_ = [](){return maniscalco::buffer((1 << 10) * 16);}
    })
{
}


//=============================================================================
auto maniscalco::m99_encode_stream::begin
(
) const -> const_iterator
{
    return packets_.begin();
}


//=============================================================================
auto maniscalco::m99_encode_stream::end
(
) const -> const_iterator
{
    return packets_.end();
}


//=============================================================================
void maniscalco::m99_encode_stream::clear
(
)
{
    stream_.flush();
    packets_.clear();
}


//=============================================================================
void maniscalco::m99_encode_stream::flush
(
)   
{
    stream_.flush();
}


//=============================================================================
auto maniscalco::m99_encode_stream::size
(
) const -> size_type
{
    return stream_.size();
}
#pragma once

#include "./m99_decode_stream.h"

#include <cstdint>


namespace maniscalco
{

    template <typename T>
    void m99_decode
    (
        m99_decode_stream &,
        T *,
        T *
    );

} // namespace maniscalco


#pragma once

#include "./m99_encode_stream.h"

#include <cstdint>


namespace maniscalco
{

    template <typename T>
    void m99_encode
    (
        T const *,
        T const *,
        m99_encode_stream &
    );

} // namespace maniscalco


#pragma once

#include "./m99_encode_stream.h"

#include <cstdint>


namespace maniscalco
{

    void m99_encode
    (
        std::uint8_t const *,
        std::uint8_t const *,
        m99_encode_stream &
    );

} // namespace maniscalco


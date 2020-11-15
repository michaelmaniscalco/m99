#pragma once

#include "./m99_decode_stream.h"

#include <cstdint>


namespace maniscalco
{

    void m99_decode
    (
        m99_decode_stream &,
        std::uint8_t *,
        std::uint8_t *
    );

} // namespace maniscalco


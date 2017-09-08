#pragma once

#include <cstdint>
#include <vector>


namespace maniscalco
{

    std::vector<std::uint8_t> m99_encode
    (
        std::uint8_t const *,
        std::uint8_t const *
    );

    std::vector<std::uint8_t> m99_decode
    (
        std::uint8_t const *,
        std::uint8_t const *
    );

} // namespace maniscalco


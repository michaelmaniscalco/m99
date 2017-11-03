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

    void m99_decode
    (
        std::uint8_t const *,
        std::uint8_t const *,
        std::uint8_t *,
        std::uint8_t *,
        std::int32_t *
    );

} // namespace maniscalco


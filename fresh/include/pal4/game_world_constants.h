#pragma once

#include <cstdint>

namespace pal4 {

class GameWorldConstants final {
public:
    static constexpr std::uint32_t kFirstAngleBits = 0x3fc90fdbu;
    static constexpr std::uint32_t kSecondAngleBits = 0x40490fdbu;

    static void Initialize(std::uint32_t& angle_bits);
    static void InitializeValue(std::uint32_t& angle_bits);
};

}  // namespace pal4

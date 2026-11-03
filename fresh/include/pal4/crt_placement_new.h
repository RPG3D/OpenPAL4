#pragma once

#include <cstdint>

namespace pal4 {

class CrtPlacementNew final {
public:
    [[nodiscard]] static std::uint32_t At(
        std::uint32_t bytes, std::uint32_t destination) noexcept;
};

}  // namespace pal4

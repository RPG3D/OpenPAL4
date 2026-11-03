#pragma once

#include <cstdint>

namespace pal4 {

class RangeNoOpCallback final {
public:
    static void Invoke(std::uint32_t, std::uint32_t) noexcept {}
};

}  // namespace pal4

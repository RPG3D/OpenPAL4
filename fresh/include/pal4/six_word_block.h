#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct SixWordBlockState {
    std::array<std::uint32_t, 6> words{};
};
static_assert(sizeof(SixWordBlockState) == 24u);

class SixWordBlock final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        SixWordBlockState& state, std::uint32_t object_address);
};

}  // namespace pal4

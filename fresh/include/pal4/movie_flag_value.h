#pragma once
#include <cstdint>
#include <optional>

namespace pal4 {
inline std::optional<std::uint32_t> OpenMovieFlagValue(std::uint32_t flags,std::uint32_t bit) {
    if(bit>=32)return {};
    return flags|(std::uint32_t{1}<<bit);
}
} // namespace pal4

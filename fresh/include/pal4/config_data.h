#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ConfigDataState final {
    std::array<std::uint32_t, 15> words{};
};

static_assert(sizeof(ConfigDataState) == 60);

class ConfigData final {
public:
    // IDA 0x5fc7e0.
    [[nodiscard]] static ConfigDataState& Initialize(ConfigDataState& state);
};

}  // namespace pal4

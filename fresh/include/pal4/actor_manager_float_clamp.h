#pragma once

#include <cstdint>

namespace pal4 {

class ActorManagerFloatClamp final {
public:
    [[nodiscard]] static std::uint32_t Clamp(
        float& value, float lower, float upper);
};

}  // namespace pal4

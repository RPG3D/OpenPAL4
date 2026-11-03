#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

#include <cstdint>

namespace pal4 {

class BehaviorPathEndpointDistance final {
public:
    [[nodiscard]] static bool WithinRadius(
        const BehaviorPathEndpointState& state,
        std::uint32_t first_word, const BehaviorVector3& position,
        float radius);
};

}  // namespace pal4

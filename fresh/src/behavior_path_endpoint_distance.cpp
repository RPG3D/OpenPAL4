#include "pal4/behavior_path_endpoint_distance.h"

#include <bit>
#include <cmath>

namespace pal4 {

bool BehaviorPathEndpointDistance::WithinRadius(
    const BehaviorPathEndpointState& state, std::uint32_t first_word,
    const BehaviorVector3& position, float radius) {
    const double x = static_cast<double>(position[0]) -
                     std::bit_cast<float>(state.words[first_word]);
    const double y = static_cast<double>(position[1]) -
                     std::bit_cast<float>(state.words[first_word + 1u]);
    const double z = static_cast<double>(position[2]) -
                     std::bit_cast<float>(state.words[first_word + 2u]);
    return std::sqrt(x * x + y * y + z * z) < radius;
}

}  // namespace pal4

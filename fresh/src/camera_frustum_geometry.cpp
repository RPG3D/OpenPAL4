#include "pal4/camera_frustum_geometry.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

std::array<float, 3> ReadVector(const CameraDataState& state,
                                 std::size_t offset) {
    return {
        std::bit_cast<float>(state.At(offset)),
        std::bit_cast<float>(state.At(offset + 4u)),
        std::bit_cast<float>(state.At(offset + 8u))};
}

}  // namespace

std::uint32_t CameraFrustumGeometry::UpdateVertices(
    CameraDataState& state, float depth) {
    const auto target = ReadVector(state, 0x74u);
    const auto up = ReadVector(state, 0x80u);
    const auto right = ReadVector(state, 0x5cu);
    const auto at = ReadVector(state, 0x50u);
    const auto extent = std::bit_cast<float>(state.At(0x1bcu));

    for (std::size_t pair = 0; pair < 4u; ++pair) {
        const auto& basis = pair < 2u ? up : right;
        const bool positive = pair == 0u || pair == 3u;
        for (std::size_t axis = 0; axis < 3u; ++axis) {
            // IDA5EA423..5EA4CB under the game's PC24 control word:
            // round each multiply before adding/subtracting the target.
            const auto offset = extent * (positive ? basis[axis] : -basis[axis]);
            const auto front = target[axis] + offset;
            const auto projection = at[axis] * depth;
            const auto back = front - projection;
            state.At(0x1c0u + pair * 24u + axis * 4u) =
                std::bit_cast<std::uint32_t>(front);
            state.At(0x1c0u + pair * 24u + 12u + axis * 4u) =
                std::bit_cast<std::uint32_t>(back);
        }
    }
    return state.At(0x21cu);
}

}  // namespace pal4

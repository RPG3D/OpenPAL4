#include "pal4/camera_bounds.h"

#include <bit>

namespace pal4 {

std::uint32_t CameraBounds::Prepare(CameraDataState& state) {
    CameraBoundsVertices vertices{};
    for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex)
        for (std::size_t axis = 0; axis < 3u; ++axis)
            vertices[vertex][axis] = std::bit_cast<float>(
                state.At(0x1c0u + vertex * 12u + axis * 4u));

    CameraBoundsMatrix transform{};
    constexpr auto one = std::bit_cast<std::uint32_t>(1.0f);
    transform[0] = one;
    transform[5] = one;
    transform[10] = one;
    transform[3] = 0x20003u;
    return runtime_.CalculateBoundingBox(state, 0x230u, vertices,
                                         transform, state.At(0));
}

}  // namespace pal4

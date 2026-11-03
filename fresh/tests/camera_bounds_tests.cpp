#include "pal4/camera_bounds.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraBoundsRuntime {
public:
    unsigned calls = 0u;

    std::uint32_t CalculateBoundingBox(
        pal4::CameraDataState& state, std::uint32_t bounds_offset,
        const pal4::CameraBoundsVertices& vertices,
        const pal4::CameraBoundsMatrix& transform,
        std::uint32_t camera) override {
        ++calls;
        Check(bounds_offset == 0x230u && camera == 0x1000u &&
                  state.At(0x230u) == 0xdeadbeefu,
              "bounds destination and camera handle");
        for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex)
            for (std::size_t axis = 0; axis < 3u; ++axis)
                Check(vertices[vertex][axis] ==
                          static_cast<float>(vertex * 3u + axis),
                      "eight vertices preserve source order");
        for (std::size_t index = 0; index < transform.size(); ++index) {
            const auto expected = index == 3u ? 0x20003u :
                (index == 0u || index == 5u || index == 10u ?
                     std::bit_cast<std::uint32_t>(1.0f) : 0u);
            Check(transform[index] == expected,
                  "identity matrix values and flags");
        }
        return 0x88u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraBounds bounds(runtime);
    pal4::CameraDataState state;
    state.words.fill(0xdeadbeefu);
    state.At(0) = 0x1000u;
    for (std::size_t vertex = 0; vertex < 8u; ++vertex)
        for (std::size_t axis = 0; axis < 3u; ++axis)
            state.At(0x1c0u + vertex * 12u + axis * 4u) =
                std::bit_cast<std::uint32_t>(
                    static_cast<float>(vertex * 3u + axis));
    Check(bounds.Prepare(state) == 0x88u && runtime.calls == 1u &&
              state.At(0x1bcu) == 0xdeadbeefu &&
              state.At(0x230u) == 0xdeadbeefu,
          "bounds preparation forwards native result without rewriting state");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

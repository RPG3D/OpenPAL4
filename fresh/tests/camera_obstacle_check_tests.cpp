#include "pal4/camera_obstacle_check.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraObstacleRuntime {
public:
    bool intersects = false;
    unsigned calls = 0u;

    bool Intersects(pal4::CameraDataState& state,
                    std::uint32_t bounds_offset,
                    std::uint32_t point_offset,
                    std::uint32_t flags) override {
        ++calls;
        Check(bounds_offset == 0x230u && point_offset == 0x2acu &&
                  flags == 0u && state.At(0x230u) == 0x1234u,
              "obstacle intersection operands");
        return intersects;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraObstacleCheck obstacle(runtime);
    pal4::CameraDataState state;
    state.At(0x230u) = 0x1234u;
    state.At(0x2b8u) = std::bit_cast<std::uint32_t>(3.5f);
    float distance = 9.0f;
    Check(!obstacle.RetrieveDistance(state, distance) &&
              distance == 9.0f && runtime.calls == 1u,
          "miss preserves caller distance");
    runtime.intersects = true;
    Check(obstacle.RetrieveDistance(state, distance) &&
              distance == 3.5f && runtime.calls == 2u &&
              state.At(0x2b8u) == std::bit_cast<std::uint32_t>(3.5f),
          "hit forwards cached camera distance");
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

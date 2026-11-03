#include "pal4/effect_camera_constraints.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectCameraConstraintsRuntime {
    std::vector<std::string> events;
    pal4::CameraVector3 source_position{};
    pal4::CameraVector3 written_position{};
    std::uint32_t matrix_address = 0x8010u;

    void UpdateTransform(pal4::EffectCameraObjectState& state) override {
        Check(state.words[31u] == 0x8000u,
              "transform update receives camera object");
        events.push_back("update");
    }
    void Translate(std::uint32_t matrix, const pal4::CameraVector3& offset,
                   std::uint32_t mode) override {
        Check(matrix == matrix_address && mode == 2u,
              "translation uses frame matrix and native combine mode");
        events.push_back(offset[0u] < 0.0f ? "translate-negative" :
                         "translate-positive");
        Check(offset == (offset[0u] < 0.0f ?
                  pal4::CameraVector3{-1.0f, -2.0f, -3.0f} :
                  pal4::CameraVector3{1.0f, 2.0f, 3.0f}),
              "pivot is negated before rotation and restored afterward");
    }
    void Rotate(std::uint32_t matrix, const pal4::CameraVector3& axis,
                float degrees, std::uint32_t mode) override {
        Check(matrix == matrix_address && mode == 2u,
              "rotation uses frame matrix and native combine mode");
        if (axis == pal4::CameraVector3{1.0f, 0.0f, 0.0f}) {
            Check(degrees == 10.0f, "X axis rotation argument");
            events.push_back("rotate-x");
        } else if (axis == pal4::CameraVector3{0.0f, 1.0f, 0.0f}) {
            Check(degrees == 20.0f, "Y axis rotation argument");
            events.push_back("rotate-y");
        } else {
            Check(axis == pal4::CameraVector3{0.0f, 0.0f, 1.0f} &&
                      degrees == 30.0f, "Z axis rotation argument");
            events.push_back("rotate-z");
        }
    }
    pal4::CameraVector3 MatrixTranslation(std::uint32_t matrix) override {
        Check(matrix == matrix_address, "read frame matrix translation");
        events.push_back("matrix-position");
        return source_position;
    }
    std::uint32_t SetPosition(pal4::EffectCameraObjectState& state,
                             const pal4::CameraVector3& position) override {
        Check(state.words[31u] == 0x8000u,
              "position callback receives camera object");
        written_position = position;
        events.push_back("set-position");
        return 0x12345678u;
    }
};

}  // namespace

int main() {
    pal4::EffectCameraObjectState state{};
    state.words[31u] = 0x8000u;
    Runtime runtime;
    const pal4::CameraVector3 pivot{1.0f, 2.0f, 3.0f};
    const pal4::CameraVector3 rotation{10.0f, 20.0f, 30.0f};
    const pal4::CameraVector3 step{2.0f, 4.0f, 6.0f};
    Check(pal4::EffectCameraConstraints::Apply(
              state, pivot, rotation, step, runtime) == 0u &&
              runtime.events.empty(), "inactive camera does no matrix work");
    state.words[1u] = 1u;
    Check(pal4::EffectCameraConstraints::Apply(
              state, pivot, rotation, step, runtime) == 0u &&
              runtime.events.empty(), "unready camera does no matrix work");
    state.words[20u] = 1u;
    state.words[11u] = 1u;
    Check(pal4::EffectCameraConstraints::Apply(
              state, pivot, rotation, step, runtime) == 1u &&
              runtime.events.empty(),
          "inherit mode bypasses constrained camera update");
    state.words[11u] = 0u;
    runtime.source_position = {5.0f, -5.0f, 3.0f};
    Check(pal4::EffectCameraConstraints::Apply(
              state, pivot, rotation, step, runtime) == 0x12345678u &&
              runtime.events == std::vector<std::string>{
                  "update", "translate-negative", "rotate-x", "rotate-y",
                  "rotate-z", "translate-positive", "matrix-position",
                  "set-position"} &&
              runtime.written_position ==
                  pal4::CameraVector3{3.0f, -1.0f, 3.0f},
          "transform operations and per-axis movement match IDA order");
    runtime.events.clear();
    runtime.source_position = {2.0f, 1.0f, 10.0f};
    Check(pal4::EffectCameraConstraints::Apply(
              state, pivot, rotation, {3.0f, 3.0f, 3.0f}, runtime) ==
              0x12345678u &&
              runtime.written_position ==
                  pal4::CameraVector3{1.0f, 2.0f, 7.0f},
          "overshoot is clipped on each axis independently");
    runtime.source_position = pivot;
    Check(pal4::EffectCameraConstraints::Apply(
              state, pivot, rotation, {-1.0f, -1.0f, -1.0f}, runtime) ==
              0x12345678u && runtime.written_position == pivot,
          "equal position remains unchanged before step arithmetic");
}

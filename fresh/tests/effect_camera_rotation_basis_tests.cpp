#include "pal4/effect_camera_rotation_basis.h"

#include <array>
#include <bit>
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

struct Runtime final : pal4::IEffectCameraRotationBasisRuntime {
    std::vector<std::string> events;
    pal4::CameraVector3 expected_right{};
    pal4::CameraVector3 expected_up{};
    pal4::CameraVector3 expected_forward{};

    void Normalize(pal4::CameraVector3& vector) override {
        events.push_back("normalize");
        Check(vector == expected_forward || vector == expected_right ||
                  vector == expected_up,
              "normalization receives one of the native basis vectors");
    }
    void UpdateMatrix(pal4::CameraMatrixWords& matrix) override {
        events.push_back("matrix-update");
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            Check(std::bit_cast<float>(matrix[index]) ==
                      expected_right[index] &&
                      std::bit_cast<float>(matrix[4u + index]) ==
                          expected_up[index] &&
                      std::bit_cast<float>(matrix[8u + index]) ==
                          expected_forward[index] &&
                      std::bit_cast<float>(matrix[12u + index]) == 0.0f,
                  "right, up, forward and zero translation matrix rows");
        }
        Check((matrix[3u] & 0x20003u) == 0x20003u,
              "native matrix flags are set");
    }
    std::array<std::uint32_t, 3> CalculateAngles(
        const pal4::CameraMatrixWords& matrix) override {
        events.push_back("angles");
        Check(std::bit_cast<float>(matrix[8u]) == expected_forward[0u],
              "angle conversion receives updated matrix");
        return {0x11111111u, 0x22222222u, 0x33333333u};
    }
    std::uint8_t ApplyAngles(
        pal4::EffectCameraObjectState& state,
        const std::array<std::uint32_t, 3>& angles,
        std::uint32_t mode) override {
        events.push_back("apply");
        Check(state.words[1u] == 1u && state.words[20u] == 1u &&
                  mode == 0u &&
                  angles == std::array<std::uint32_t, 3>{
                      0x11111111u, 0x22222222u, 0x33333333u},
              "angles dispatch through ready camera facet with mode zero");
        return 0x7bu;
    }
};

}  // namespace

int main() {
    pal4::EffectCameraObjectState state{};
    Runtime runtime;
    const pal4::CameraVector3 direction{0.0f, 0.0f, 1.0f};
    Check(pal4::EffectCameraRotationBasis::SetFromDirection(
              state, direction, runtime) == 0u && runtime.events.empty(),
          "inactive camera bypasses normalization and callback");
    state.words[1u] = 1u;
    Check(pal4::EffectCameraRotationBasis::SetFromDirection(
              state, direction, runtime) == 0u && runtime.events.empty(),
          "unready camera bypasses normalization and callback");
    state.words[20u] = 1u;
    runtime.expected_forward = direction;
    runtime.expected_right = {1.0f, 0.0f, 0.0f};
    runtime.expected_up = {0.0f, 1.0f, 0.0f};
    Check(pal4::EffectCameraRotationBasis::SetFromDirection(
              state, direction, runtime) == 0x7bu &&
              runtime.events == std::vector<std::string>{
                  "normalize", "normalize", "normalize",
                  "matrix-update", "angles", "apply"},
          "orthogonal basis and callback follow native order");
    runtime.events.clear();
    runtime.expected_forward = {1.0f, 0.0f, 0.0f};
    runtime.expected_right = {0.0f, 0.0f, -1.0f};
    Check(pal4::EffectCameraRotationBasis::SetFromDirection(
              state, runtime.expected_forward, runtime) == 0x7bu,
          "cross product orientation follows native right-handed basis");

    const std::array<std::uint32_t, 3> values{
        0xabcdef01u, 0x12345602u, 0x876543a5u};
    Check(pal4::EffectCameraRotationBasis::SetProperties(
              state, values) == 0xa5u &&
              state.words[25u] == values[0u] &&
              state.words[26u] == values[1u] &&
              state.words[27u] == values[2u],
          "ready camera writes three raw properties and returns low byte");
    state.words[20u] = 0u;
    Check(pal4::EffectCameraRotationBasis::SetProperties(
              state, {0u, 0u, 0u}) == 0u &&
              state.words[25u] == values[0u],
          "unready camera leaves properties intact");
    state.words[1u] = 0u;
    state.words[20u] = 1u;
    Check(pal4::EffectCameraRotationBasis::SetProperties(
              state, {0u, 0u, 0u}) == 0u &&
              state.words[25u] == values[0u],
          "inactive camera leaves properties intact");
}

#include "pal4/camera_parameter_setters.h"

#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

float FloatAt(const pal4::CameraDataState& state, std::size_t offset) {
    return std::bit_cast<float>(state.At(offset));
}

class Runtime final : public pal4::ICameraParameterRuntime {
public:
    std::vector<std::uint32_t> modes;
    std::uint32_t minimum_calls = 0u;
    std::uint32_t maximum_calls = 0u;

    float MinimumDistance() override {
        ++minimum_calls;
        return 1.0f;
    }
    float MaximumDistance() override {
        ++maximum_calls;
        return 50.0f;
    }
    std::uint32_t UpdateMatrix(pal4::CameraDataState&,
                               std::uint32_t mode) override {
        modes.push_back(mode);
        return 0x7000u + mode;
    }
};

void PositionAndDistanceCases() {
    Runtime runtime;
    pal4::CameraParameterSetters setters(runtime);
    pal4::CameraDataState state;
    Check(!setters.SetEye(state, {1, 2, 3}, 2u).has_value() &&
              !setters.SetLookAt(state, {4, 5, 6}, 3u).has_value() &&
              !setters.SetTargetXyz(state, 7, 8, 9, 4u).has_value() &&
              !setters.SetDistance(state, 2.0f, 5u).has_value() &&
              runtime.modes.empty() && runtime.minimum_calls == 0u,
          "four camera-gated setters skip writes and callbacks when absent");

    state.At(0) = 0x1000u;
    Check(setters.SetEye(state, {1, 2, 3}, 2u) == 0x7002u &&
              FloatAt(state, 0x68) == 1.0f &&
              FloatAt(state, 0x6c) == 2.0f &&
              FloatAt(state, 0x70) == 3.0f &&
              runtime.modes == std::vector<std::uint32_t>{2u},
          "eye setter writes three raw XYZ slots then updates matrix");

    Check(setters.SetLookAt(state, {4, 5, 6}, 3u) == 0x7003u &&
              FloatAt(state, 0x74) == 4.0f &&
              FloatAt(state, 0x78) == 5.0f &&
              FloatAt(state, 0x7c) == 6.0f,
          "look-at setter copies target vector");
    Check(setters.SetTargetXyz(state, 7, 8, 9, 4u) == 0x7004u &&
              FloatAt(state, 0x74) == 7.0f &&
              FloatAt(state, 0x78) == 8.0f &&
              FloatAt(state, 0x7c) == 9.0f &&
              runtime.modes == std::vector<std::uint32_t>{2u, 3u, 4u},
          "scalar target setter writes the same three target offsets");

    Check(setters.SetDistance(state, 0.25f, 5u) == 0x7005u &&
              FloatAt(state, 0x4c) == 1.0f &&
              FloatAt(state, 0x48) == 1.0f &&
              runtime.minimum_calls == 1u && runtime.maximum_calls == 1u,
          "distance setter clamps below game minimum and mirrors current");
    Check(setters.SetDistance(state, 60.0f, 6u) == 0x7006u &&
              FloatAt(state, 0x4c) == 50.0f &&
              FloatAt(state, 0x48) == 50.0f,
          "distance setter clamps above game maximum");
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    Check(setters.SetDistance(state, nan, 7u) == 0x7007u &&
              std::isnan(FloatAt(state, 0x4c)) &&
              std::isnan(FloatAt(state, 0x48)),
          "unordered distance passes through x87 comparisons");
}

void AngleCases() {
    Runtime runtime;
    pal4::CameraParameterSetters setters(runtime);
    pal4::CameraDataState state;
    Check(setters.SetFirstAngle(state, -30.0f, 1u) == 0x7001u &&
              FloatAt(state, 0x3c) == 330.0f,
          "first angle adds 360 once when negative without camera gate");
    Check(setters.SetSecondAngle(state, 390.0f, 2u) == 0x7002u &&
              FloatAt(state, 0x40) == 30.0f,
          "second angle subtracts 360 once when above range");
    Check(setters.SetThirdAngle(state, 360.0f, 3u) == 0x7003u &&
              FloatAt(state, 0x44) == 0.0f,
          "third angle wraps exact 360 to zero");

    Check(setters.SetAllAngles(state, {-720.0f, 720.0f, 15.0f},
                               4u) == 0x7004u &&
              FloatAt(state, 0x3c) == -360.0f &&
              FloatAt(state, 0x40) == 360.0f &&
              FloatAt(state, 0x44) == 15.0f,
          "combined angle setter makes only one adjustment per axis");

    Check(setters.SetAnglesAndDistance(
              state, {-30.0f, 360.0f, 390.0f}, 0.25f, 5u) == 0x7005u &&
              FloatAt(state, 0x3c) == 330.0f &&
              FloatAt(state, 0x40) == 0.0f &&
              FloatAt(state, 0x44) == 30.0f &&
              FloatAt(state, 0x4c) == 1.0f &&
              FloatAt(state, 0x48) == 0.25f &&
              runtime.minimum_calls == 1u && runtime.maximum_calls == 1u,
          "combined setter clamps target distance but retains raw current");
    Check(setters.SetAnglesAndDistance(
              state, {0.0f, 0.0f, 0.0f}, 80.0f, 6u) == 0x7006u &&
              FloatAt(state, 0x4c) == 50.0f &&
              FloatAt(state, 0x48) == 80.0f &&
              runtime.modes ==
                  std::vector<std::uint32_t>{1u, 2u, 3u, 4u, 5u, 6u},
          "combined setter forwards matrix mode and preserves input distance");
}

void StateSlotCases() {
    Runtime runtime;
    pal4::CameraParameterSetters setters(runtime);
    pal4::CameraDataState state;
    state.words.fill(0xdeadbeefu);
    Check(setters.InitializeParameters(state, 11u, 22u, 33u) == 0u &&
              state.At(0x90u) == 0u && state.At(0x94u) == 11u &&
              state.At(0x98u) == 22u && state.At(0x9cu) == 33u &&
              state.At(0xa0u) == 0u && state.At(0xa4u) == 0xdeadbeefu &&
              runtime.modes.empty(),
          "camera parameter initializer writes only five observed slots");
    state.At(0x78u) = std::bit_cast<std::uint32_t>(6.5f);
    Check(setters.SetActiveState(state, 2u) == 2u &&
              state.At(0x8cu) == 2u &&
              state.At(0xa4u) == std::bit_cast<std::uint32_t>(6.5f) &&
              runtime.modes.empty(),
          "active state snapshots target Y and returns input value");
}

}  // namespace

int main() {
    try {
        PositionAndDistanceCases();
        AngleCases();
        StateSlotCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "pal4/camera_environment_probe.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void SetFloat(pal4::CameraDataState& state, std::size_t offset,
              float value) {
    state.At(offset) = std::bit_cast<std::uint32_t>(value);
}

float GetFloat(const pal4::CameraDataState& state, std::size_t offset) {
    return std::bit_cast<float>(state.At(offset));
}

void SetPoint(pal4::CameraDataState& state, std::size_t offset,
              const pal4::CameraPoint& point) {
    for (std::size_t axis = 0; axis < point.size(); ++axis)
        SetFloat(state, offset + axis * 4u, point[axis]);
}

class Runtime final : public pal4::ICameraEnvironmentRuntime {
public:
    std::uint32_t frame = 0x2000u;
    unsigned frame_calls = 0u;
    unsigned auxiliary_calls = 0u;
    unsigned collision_calls = 0u;
    unsigned magnitude_calls = 0u;
    std::array<std::optional<pal4::CameraCollision>, 4> collisions{};
    std::array<std::optional<pal4::CameraCollision>, 4> auxiliary{};

    std::uint32_t EnvironmentFrame(std::uint32_t camera) override {
        Check(camera == 0x1000u, "environment frame camera");
        ++frame_calls;
        return frame;
    }

    bool ProbeAuxiliary(std::uint32_t callback,
                        const pal4::CameraSegment& segment,
                        float& fraction,
                        pal4::CameraPoint& point) override {
        Check(callback == 0x1234u && auxiliary_calls < 4u &&
                  segment[0] == static_cast<float>(auxiliary_calls),
              "auxiliary callback and segment order");
        const auto index = auxiliary_calls++;
        if (!auxiliary[index]) return false;
        fraction = auxiliary[index]->fraction;
        point = auxiliary[index]->point;
        return true;
    }

    bool Intersect(std::uint32_t world,
                   const pal4::CameraSegment& segment,
                   pal4::CameraCollision& collision) override {
        Check(world == frame && collision_calls < 4u &&
                  segment[0] == static_cast<float>(collision_calls) &&
                  segment[3] == static_cast<float>(collision_calls + 10u),
              "collision frame and consecutive segment pairs");
        const auto index = collision_calls++;
        if (!collisions[index]) return false;
        collision = *collisions[index];
        return true;
    }

    float Magnitude(const pal4::CameraPoint& point) override {
        ++magnitude_calls;
        return std::sqrt(point[0] * point[0] + point[1] * point[1] +
                         point[2] * point[2]);
    }
};

pal4::CameraDataState MakeState() {
    pal4::CameraDataState state;
    state.words.fill(0xdeadbeefu);
    state.At(0) = 0x1000u;
    state.At(0x1acu) = 1u;
    state.At(0x2bcu) = 0u;
    SetPoint(state, 0x74u, {10.0f, 20.0f, 30.0f});
    SetFloat(state, 0x1b8u, 5.0f);
    for (std::size_t index = 0; index < 4u; ++index) {
        SetPoint(state, 0x1c0u + index * 24u,
                 {static_cast<float>(index), 1.0f, 2.0f});
        SetPoint(state, 0x1ccu + index * 24u,
                 {static_cast<float>(index + 10u), 3.0f, 4.0f});
    }
    return state;
}

void MissingEnvironment() {
    Runtime runtime;
    runtime.frame = 0u;
    pal4::CameraEnvironmentProbe probe(runtime);
    auto state = MakeState();
    float distance = 123.0f;
    pal4::CameraPoint direction{7.0f, 8.0f, 9.0f};
    Check(!probe.Update(state, &distance, &direction) &&
              runtime.frame_calls == 1u && runtime.collision_calls == 0u &&
              state.At(0x2d4u) == 0xdeadbeefu && distance == 123.0f &&
              direction == pal4::CameraPoint{7.0f, 8.0f, 9.0f},
          "missing environment exits before state and output writes");
}

void NoIntersection() {
    Runtime runtime;
    pal4::CameraEnvironmentProbe probe(runtime);
    auto state = MakeState();
    float distance = 123.0f;
    Check(!probe.Update(state, &distance, nullptr) &&
              runtime.frame_calls == 5u && runtime.collision_calls == 4u &&
              runtime.magnitude_calls == 0u &&
              state.At(0x2d4u) == 0u && state.At(0x2e8u) == 0xffffffffu &&
              GetFloat(state, 0x2d8u) == std::numeric_limits<float>::max() &&
              distance == 123.0f,
          "no intersection keeps outputs and records no selected segment");
    for (std::size_t index = 0; index < 4u; ++index)
        Check(state.At(0x220u + index * 4u) == 0xff0000ffu,
              "debug lines reset to default color");
}

void NearestIntersection() {
    Runtime runtime;
    runtime.collisions[1] = pal4::CameraCollision{
        0.4f, {10.0f, 20.0f, 28.0f}};
    runtime.collisions[2] = pal4::CameraCollision{
        0.2f, {10.0f, 20.0f, 27.0f}};
    runtime.collisions[3] = pal4::CameraCollision{
        0.7f, {10.0f, 20.0f, 24.0f}};
    pal4::CameraEnvironmentProbe probe(runtime);
    auto state = MakeState();
    float distance = 0.0f;
    pal4::CameraPoint relative{};
    Check(probe.Update(state, &distance, &relative) &&
              state.At(0x2d4u) == 1u && state.At(0x2e8u) == 2u &&
              GetFloat(state, 0x2acu) == 10.0f &&
              GetFloat(state, 0x2b0u) == 20.0f &&
              GetFloat(state, 0x2b4u) == 27.0f &&
              relative == pal4::CameraPoint{0.0f, 0.0f, 3.0f} &&
              GetFloat(state, 0x2e4u) == 3.0f &&
              distance == 5.0f && GetFloat(state, 0x2b8u) == 5.0f &&
              state.At(0x220u + 2u * 4u) == 0xff00ff00u &&
              runtime.magnitude_calls == 1u,
          "nearest collision supplies point, relative vector and clamped distance");
    Check(GetFloat(state, 0x2c4u) == 0.7f &&
              GetFloat(state, 0x2d0u) == 24.0f,
          "last collision scratch data is independent of selected point");
}

void AuxiliaryAndUnordered() {
    Runtime runtime;
    runtime.auxiliary[0] = pal4::CameraCollision{
        0.1f, {10.0f, 20.0f, 29.0f}};
    runtime.collisions[0] = pal4::CameraCollision{
        0.4f, {10.0f, 20.0f, 26.0f}};
    runtime.collisions[1] = pal4::CameraCollision{
        std::numeric_limits<float>::quiet_NaN(), {0.0f, 0.0f, 0.0f}};
    pal4::CameraEnvironmentProbe probe(runtime);
    auto state = MakeState();
    state.At(0x2bcu) = 0x1234u;
    state.At(0x1acu) = 0u;
    Check(probe.Update(state, nullptr, nullptr) &&
              runtime.auxiliary_calls == 4u &&
              state.At(0x2e8u) == 0u &&
              GetFloat(state, 0x2b4u) == 29.0f &&
              GetFloat(state, 0x2e4u) == 1.0f &&
              GetFloat(state, 0x2b8u) == 5.0f &&
              state.At(0x220u) == 0xdeadbeefu,
          "auxiliary candidate wins, unordered collision loses, debug mode stays off");
}

}  // namespace

int main() {
    try {
        MissingEnvironment();
        NoIntersection();
        NearestIntersection();
        AuxiliaryAndUnordered();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

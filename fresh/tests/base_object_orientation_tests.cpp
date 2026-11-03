#include "pal4/base_object_orientation.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void WriteFloat(pal4::BaseObjectState& state,
                std::uint32_t offset, float value) {
    state.Write(offset, std::bit_cast<std::uint32_t>(value));
}

struct Runtime final : pal4::IBaseObjectOrientationRuntime {
    std::vector<char> events;
    pal4::BaseObjectVector3 normalized{};
    pal4::BaseObjectVector3 pose{};
    pal4::BaseObjectVector3 next{};
    double cosine{};
    double angle{1.25};

    void NormalizeAndClamp(pal4::BaseObjectVector3& direction) override {
        Check(direction.x == 9.0f && direction.y == 8.0f &&
                  direction.z == 7.0f,
              "normalizer receives copied direction argument");
        events.push_back('N');
        direction = normalized;
    }
    double AngleFromCosine(double value) override {
        cosine = value;
        events.push_back('A');
        return angle;
    }
    void SetPose(pal4::BaseObjectState& state,
                 const pal4::BaseObjectVector3& value,
                 std::uint32_t mode) override {
        Check(mode == 0u && state.Read(0xa0u) ==
                                  std::bit_cast<std::uint32_t>(11.0f),
              "pose virtual call uses zero mode without overwriting state");
        pose = value;
        events.push_back('P');
    }
    void SetDirection(pal4::BaseObjectState&,
                      const pal4::BaseObjectVector3& value,
                      std::uint32_t mode) override {
        Check(mode == 0u, "direction virtual call uses zero mode");
        next = value;
        events.push_back('D');
    }
};

void CheckUpdate(float normalized_x, double expected_radians) {
    pal4::BaseObjectState state{};
    WriteFloat(state, 0xa0u, 11.0f);
    WriteFloat(state, 0xa4u, 22.0f);
    WriteFloat(state, 0xa8u, 33.0f);
    const pal4::BaseObjectVector3 source{9.0f, 8.0f, 7.0f};
    const pal4::BaseObjectVector3 output{4.0f, 5.0f, 6.0f};
    Runtime runtime;
    runtime.normalized = {normalized_x, 0.0f, 0.25f};

    pal4::BaseObjectOrientation::Update(state, source, 0x123u, 0x456u,
                                        output, runtime);
    Check(runtime.events.empty(),
          "missing owner clump returns before normalization and virtual calls");

    state.Write(0x14u, 0x1000u);
    pal4::BaseObjectOrientation::Update(state, source, 0x123u, 0x456u,
                                        output, runtime);
    const auto expected_degrees = static_cast<float>(
        static_cast<double>(57.295780181884766f) * expected_radians);
    Check(runtime.events == std::vector<char>{'N', 'A', 'P', 'D'} &&
              runtime.cosine == 0.25 &&
              runtime.pose.x == 11.0f &&
              runtime.pose.y == expected_degrees &&
              runtime.pose.z == 33.0f &&
              runtime.next.x == 4.0f && runtime.next.y == 5.0f &&
              runtime.next.z == 6.0f &&
              source.x == 9.0f && output.z == 6.0f,
          "orientation uses normalized Z, X sign and ordered virtual writes");
}

}  // namespace

int main() {
    CheckUpdate(1.0f, 1.25);
    CheckUpdate(0.0f, 1.25);
    CheckUpdate(-1.0f, 6.283185307179586 - 1.25);
}

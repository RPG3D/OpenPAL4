#include "pal4/object_path_movement.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float left, float right) {
    return std::fabs(left - right) < 0.0001f;
}

struct Runtime final : pal4::IObjectPathMovementRuntime {
    pal4::ObjectMovementState* state = nullptr;
    std::vector<std::string> calls;
    pal4::BehaviorVector3 positions[3]{};
    pal4::BehaviorVector3 point{10.0f, 0.0f, 0.0f};
    pal4::BehaviorVector3 last{20.0f, 0.0f, 0.0f};
    pal4::BehaviorVector3 written{};
    float distance = 2.0f;
    float length = 5.0f;
    float evaluated_distance = -1.0f;
    int position_calls = 0;
    int speed_calls = 0;

    pal4::BehaviorVector3 Position(
        pal4::ObjectMovementState& object) override {
        calls.emplace_back("position");
        Check(&object == state && position_calls < 3,
              "position query uses object and occurs three times");
        return positions[position_calls++];
    }
    float DistanceAlong(std::uint32_t path,
                        const pal4::BehaviorVector3& position) override {
        calls.emplace_back("distance");
        Check(path == 0x1234u && position == positions[0],
              "path distance receives first position");
        return distance;
    }
    float Speed(pal4::ObjectMovementState& object) override {
        calls.emplace_back("speed");
        Check(&object == state && speed_calls < 2,
              "speed is queried twice");
        return speed_calls++ == 0 ? 2.0f : 3.0f;
    }
    float PathLength(std::uint32_t path) override {
        calls.emplace_back("length");
        Check(path == 0x1234u, "path length address");
        return length;
    }
    pal4::BehaviorVector3 PointAt(std::uint32_t path,
                                  float position) override {
        calls.emplace_back("point");
        Check(path == 0x1234u, "interpolation path address");
        evaluated_distance = position;
        return point;
    }
    pal4::BehaviorVector3 LastPoint(std::uint32_t path) override {
        calls.emplace_back("last");
        Check(path == 0x1234u, "last point path address");
        return last;
    }
    void SetPosition(pal4::ObjectMovementState& object,
                     const pal4::BehaviorVector3& position) override {
        calls.emplace_back("set");
        Check(&object == state, "position setter object");
        written = position;
    }
    std::uint32_t NotifyMember(
        std::uint32_t member,
        const pal4::BehaviorVector3& path_point) override {
        calls.emplace_back("notify");
        Check(member == 0x5678u &&
                  path_point == written,
              "member receives actual next position, including off-path movement");
        Check((state->words[46] & 0xff00u) == 0xa500u,
              "active flag changes after notification");
        return 0xbeefu;
    }
};

void TestNoPath() {
    pal4::ObjectMovementState state;
    state.words[46] = 0xabcda55au;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ObjectPathMovement::Advance(
              state, 0x9999u, 0.5f, runtime) == 0u &&
              state.words[46] == 0xabcd005au && runtime.calls.empty(),
          "no path clears only the active byte and returns zero");
}

void TestInterpolatedPath() {
    pal4::ObjectMovementState state;
    state.words[42] = 0x5678u;
    state.words[46] = 0xabcda55au;
    state.words[47] = 0x1234u;
    Runtime runtime;
    runtime.state = &state;
    runtime.positions[0] = {1.0f, 0.0f, 0.0f};
    runtime.positions[1] = {4.0f, 0.0f, 0.0f};
    runtime.positions[2] = {7.0f, 0.0f, 0.0f};
    Check(pal4::ObjectPathMovement::Advance(
              state, 0x9999u, 0.5f, runtime) == 0xbeefu &&
              state.words[46] == 0xabcd015au,
          "active path returns member result and marks activity");
    Check(Near(runtime.evaluated_distance, 3.0f) &&
              Near(runtime.written[0], 8.5f) &&
              Near(runtime.written[1], 0.0f) &&
              Near(runtime.written[2], 0.0f),
          "path distance and movement use distinct speed queries");
    Check(runtime.calls ==
              std::vector<std::string>{"position", "distance", "speed",
                                       "length", "point", "position",
                                       "speed", "position", "set", "notify"},
          "interpolated call order");
}

void TestEndpointAndZeroDirection() {
    pal4::ObjectMovementState state;
    state.words[42] = 0x5678u;
    state.words[46] = 0xabcda55au;
    state.words[47] = 0x1234u;
    Runtime runtime;
    runtime.state = &state;
    runtime.length = 3.0f;
    runtime.positions[1] = runtime.last;
    runtime.positions[2] = {2.0f, 3.0f, 4.0f};
    Check(pal4::ObjectPathMovement::Advance(
              state, 0u, 0.5f, runtime) == 0xbeefu &&
              runtime.written == runtime.positions[2] &&
              runtime.calls[4] == "last",
          "equal path distance selects last point and preserves zero direction");
}

void TestUnorderedLength() {
    pal4::ObjectMovementState state;
    state.words[42] = 0x5678u;
    state.words[46] = 0xabcda55au;
    state.words[47] = 0x1234u;
    Runtime runtime;
    runtime.state = &state;
    runtime.length = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::ObjectPathMovement::Advance(
              state, 0u, 0.5f, runtime) == 0xbeefu &&
              runtime.calls[4] == "point",
          "unordered length follows the x87 parity branch to interpolation");
}

}  // namespace

int main() {
    TestNoPath();
    TestInterpolatedPath();
    TestEndpointAndZeroDirection();
    TestUnorderedLength();
    std::cout << "4 object path movement cases passed\n";
}

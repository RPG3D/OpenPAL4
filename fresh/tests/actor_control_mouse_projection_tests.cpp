#include "pal4/actor_control_mouse_projection.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kActor = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void CheckVector(const pal4::ActorControlVec3& vector,
                 float x, float y, float z, const char* message) {
    Check(vector.x == x && vector.y == y && vector.z == z, message);
}

struct Runtime final : pal4::IActorControlMouseProjectionRuntime {
    std::vector<std::string> calls;
    double distance = 2.0;
    pal4::ActorControlRay ray{{1.0f, 2.0f, 3.0f},
                              {4.0f, 5.0f, 6.0f}};

    pal4::ActorControlCursorPoint CursorPosition() override {
        calls.emplace_back("cursor");
        return {120, -25};
    }
    void ScreenToClient(pal4::ActorControlCursorPoint& point) override {
        calls.emplace_back("client");
        Check(point.x == 120 && point.y == -25,
              "client conversion receives screen cursor");
        point = {-37, 52};
    }
    std::uint32_t CameraSystem() override {
        calls.emplace_back("system");
        return 0x2000u;
    }
    std::uint32_t ActiveCameraInternalId(std::uint32_t system) override {
        calls.emplace_back("camera");
        Check(system == 0x2000u, "camera lookup receives system");
        return 0x3000u;
    }
    void CalculateNormalizedPixelRay(std::uint32_t camera, float x,
                                     float y,
                                     pal4::ActorControlRay& output) override {
        calls.emplace_back("ray");
        Check(camera == 0x3000u && x == -37.0f && y == 52.0f,
              "camera ray receives converted client pixel as floats");
        output = ray;
    }
    std::uint32_t ReadActorWord(std::uint32_t address) override {
        calls.emplace_back("actor");
        Check(address == kActor + 0xacu ||
                  address == kActor + 0xb0u ||
                  address == kActor + 0xb4u,
              "actor read uses position fields");
        const float value = address == kActor + 0xacu ? 10.0f :
                            address == kActor + 0xb0u ? 20.0f : 30.0f;
        return std::bit_cast<std::uint32_t>(value);
    }
    double CalculateIntersection(
        const pal4::ActorControlRay& input,
        const pal4::ActorControlVec3& normal,
        const pal4::ActorControlVec3& position) override {
        calls.emplace_back("intersection");
        CheckVector(input.origin, 1.0f, 2.0f, 3.0f,
                    "intersection receives ray origin");
        CheckVector(input.direction, 4.0f, 5.0f, 6.0f,
                    "intersection receives ray direction");
        CheckVector(normal, 0.0f, 1.0f, 0.0f,
                    "intersection uses horizontal plane normal");
        CheckVector(position, 10.0f, 20.0f, 30.0f,
                    "intersection uses actor world position");
        return distance;
    }
};

void TestProjectionAndCallOrder() {
    Runtime runtime;
    pal4::ActorControlVec3 output{-1.0f, -1.0f, -1.0f};
    Check(pal4::ActorControlMouseProjection::Project(
              kActor, output, runtime),
          "finite intersection succeeds");
    CheckVector(output, 9.0f, 12.0f, 15.0f,
                "ray origin plus scaled direction is returned");
    Check(runtime.calls == std::vector<std::string>{
              "cursor", "client", "system", "camera", "ray",
              "actor", "actor", "actor", "intersection"},
          "cursor, camera, actor and intersection calls keep IDA order");
}

void TestIntersectionBoundary() {
    for (const auto value : {
             static_cast<double>(std::numeric_limits<float>::max()),
             std::numeric_limits<double>::infinity(),
             std::numeric_limits<double>::quiet_NaN()}) {
        Runtime runtime;
        runtime.distance = value;
        pal4::ActorControlVec3 output{7.0f, 8.0f, 9.0f};
        Check(!pal4::ActorControlMouseProjection::Project(
                  kActor, output, runtime),
              "FLT_MAX, infinity and unordered intersections fail");
        CheckVector(output, 7.0f, 8.0f, 9.0f,
                    "failed intersection leaves output untouched");
    }
    Runtime runtime;
    runtime.distance = -2.0;
    pal4::ActorControlVec3 output{};
    Check(pal4::ActorControlMouseProjection::Project(
              kActor, output, runtime),
          "negative intersection is accepted by original comparison");
    CheckVector(output, -7.0f, -8.0f, -9.0f,
                "negative distance extrapolates the ray");
}

}  // namespace

int main() {
    TestProjectionAndCallOrder();
    TestIntersectionBoundary();
    pal4::ActorControlMouseProjectionRuntimeStub runtime;
    pal4::ActorControlVec3 output{1.0f, 2.0f, 3.0f};
    Check(!pal4::ActorControlMouseProjection::Project(
              kActor, output, runtime),
          "linkable stub reports no intersection");
    CheckVector(output, 1.0f, 2.0f, 3.0f,
                "linkable stub preserves the output");
}

#include "pal4/object_complex_collision.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

using pal4::BehaviorVector3;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Object {
    float radius = 0.0f;
    float speed = 0.0f;
    BehaviorVector3 position{};
    BehaviorVector3 offset{};
    BehaviorVector3 forward{};
    BehaviorVector3 direction{};
};

struct Runtime final : pal4::IObjectComplexCollisionRuntime {
    std::map<std::uint32_t, Object> objects;
    std::vector<std::uint32_t> proximity;
    std::vector<std::uint32_t> collisions;
    BehaviorVector3 selected_self{};
    BehaviorVector3 selected_other{};
    float sign = 0.0f;
    int speed_calls = 0;
    int offset_calls = 0;
    int direction_calls = 0;
    int global_calls = 0;

    float Radius(std::uint32_t object) override {
        return objects.at(object).radius;
    }
    float Speed(std::uint32_t object) override {
        ++speed_calls;
        return objects.at(object).speed;
    }
    BehaviorVector3 Position(std::uint32_t object) override {
        return objects.at(object).position;
    }
    BehaviorVector3 Offset(std::uint32_t object) override {
        ++offset_calls;
        return objects.at(object).offset;
    }
    BehaviorVector3 Forward(std::uint32_t object) override {
        return objects.at(object).forward;
    }
    BehaviorVector3 Direction(std::uint32_t object) override {
        ++direction_calls;
        return objects.at(object).direction;
    }
    BehaviorVector3 GlobalZeroVector() override {
        ++global_calls;
        return {};
    }
    void NotifyProximity(std::uint32_t object, std::uint32_t other,
                         std::uint32_t argument) override {
        Check(object == 1u && argument == 0u, "proximity callback arguments");
        proximity.push_back(other);
    }
    void NotifyCollision(std::uint32_t object, std::uint32_t other,
                         float value, const BehaviorVector3& projected_self,
                         const BehaviorVector3& projected_other) override {
        Check(object == 1u, "collision callback object");
        collisions.push_back(other);
        selected_self = projected_self;
        selected_other = projected_other;
        sign = value;
    }
};

BehaviorVector3 Cached(const pal4::ObjectMovementState& state,
                       std::size_t word) {
    return {std::bit_cast<float>(state.words[word]),
            std::bit_cast<float>(state.words[word + 1u]),
            std::bit_cast<float>(state.words[word + 2u])};
}

void TestEmptyAndSelfOnly() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    runtime.objects[1u].direction = {1.0f, 2.0f, 3.0f};
    BehaviorVector3 output{};
    Check(&pal4::ObjectComplexCollision::Process(
              state, 1u, output, 3.0f, {}, runtime) == &output &&
          output == BehaviorVector3{} && runtime.proximity.empty() &&
          runtime.collisions.empty() && runtime.direction_calls == 1,
          "empty collection returns direction times zero");
    const std::uint32_t ids[]{1u};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 3.0f, ids, runtime);
    Check(output == BehaviorVector3{} && runtime.offset_calls == 0,
          "self is skipped in both scans");
}

void TestProximityExit() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    runtime.objects[1u] = Object{1.0f, 0.0f, {}, {}, {0.0f, 0.0f, 1.0f}};
    runtime.objects[2u] = Object{1.0f, 0.0f, {1.0f, 0.0f, 0.0f}};
    BehaviorVector3 output{};
    const std::uint32_t ids[]{1u, 2u};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 3.0f, ids, runtime);
    Check(output == BehaviorVector3{-1.0f, 0.0f, 0.0f} &&
          runtime.proximity == std::vector<std::uint32_t>{2u} &&
          runtime.collisions.empty() && runtime.offset_calls == 0 &&
          runtime.direction_calls == 0,
          "first overlap returns projected separation");
}

void TestZeroProjectionFallsThrough() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    runtime.objects[1u] = Object{1.0f, 0.0f, {}, {}, {1.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 1.0f}};
    runtime.objects[2u] = Object{1.0f, 0.0f, {1.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f}};
    BehaviorVector3 output{};
    const std::uint32_t ids[]{2u};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, -1.0f, ids, runtime);
    Check(output == BehaviorVector3{} &&
          runtime.proximity == std::vector<std::uint32_t>{2u} &&
          runtime.offset_calls == 2 && runtime.collisions.empty(),
          "zero projection enters second scan");
}

void TestNearestCandidateAndAlignedSpeedGate() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    runtime.objects[1u] = Object{0.1f, 0.0f, {}, {}, {1.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 1.0f}};
    runtime.objects[2u] = Object{0.1f, 1.0f, {-2.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    runtime.objects[3u] = Object{0.1f, 1.0f, {-1.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    BehaviorVector3 output{};
    const std::uint32_t ids[]{2u, 3u};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 3.0f, ids, runtime);
    Check(runtime.collisions == std::vector<std::uint32_t>{3u} &&
          runtime.sign == -1.0f &&
          runtime.selected_self == BehaviorVector3{} &&
          runtime.selected_other == BehaviorVector3{} &&
          Cached(state, 18u) == BehaviorVector3{} &&
          Cached(state, 15u) == BehaviorVector3{} &&
          output == BehaviorVector3{0.0f, 0.0f, -1.0f},
          "nearest valid time replaces prior candidate");
    Check(runtime.direction_calls == 1,
          "aligned faster other keeps negative sign");
}

void TestOppositeAndTransverseSign() {
    pal4::ObjectMovementState state;
    Runtime opposite;
    opposite.objects[1u] = Object{0.75f, 0.0f, {1.0f, 0.0f, 0.0f},
                                  {}, {1.0f, 0.0f, 0.0f},
                                  {1.0f, 0.0f, 0.0f}};
    opposite.objects[2u] = Object{0.75f, 0.0f, {},
                                  {1.0f, 0.0f, 0.0f},
                                  {-1.0f, 0.0f, 0.0f}};
    BehaviorVector3 output{};
    const std::uint32_t ids[]{2u};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 2.0f, ids, opposite);
    Check(opposite.collisions == std::vector<std::uint32_t>{2u} &&
          opposite.sign == 1.0f &&
          output == BehaviorVector3{1.0f, 0.0f, 0.0f},
          "opposed headings use predicted other-to-self direction");

    Runtime transverse;
    transverse.objects[1u] = Object{0.2f, 0.0f, {}, {},
                                    {0.0f, 1.0f, 0.0f},
                                    {1.0f, 0.0f, 0.0f}};
    transverse.objects[2u] = Object{0.2f, 1.0f, {-1.0f, 0.0f, 0.0f},
                                    {1.0f, 0.0f, 0.0f},
                                    {1.0f, 0.0f, 0.0f}};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 2.0f, ids, transverse);
    Check(transverse.collisions == std::vector<std::uint32_t>{2u} &&
          transverse.sign == 1.0f &&
          output == BehaviorVector3{1.0f, 0.0f, 0.0f},
          "transverse headings use current other-to-self direction");
}

void TestZeroDistanceAndSignEquality() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    runtime.objects[1u] = Object{1.1f, 0.0f, {}, {},
                                 {1.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 1.0f}};
    runtime.objects[2u] = Object{0.0f, 0.0f, {2.0f, 0.0f, 0.0f},
                                 {}, {0.0f, 1.0f, 0.0f}};
    const std::uint32_t ids[]{2u};
    BehaviorVector3 output{};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 1.0f, ids, runtime);
    Check(runtime.collisions == std::vector<std::uint32_t>{2u} &&
          runtime.sign == 1.0f &&
          output == BehaviorVector3{0.0f, 0.0f, 1.0f},
          "zero separation time and zero sign dot select positive");

    pal4::ObjectComplexCollisionRuntimeStub stub;
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 1.0f, {}, stub);
    Check(output == BehaviorVector3{}, "linkable complex collision stub");
}

void TestStrictTimeAndDiameter() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    runtime.objects[1u] = Object{0.1f, 0.0f, {}, {},
                                 {1.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 1.0f}};
    runtime.objects[2u] = Object{0.1f, 1.0f, {-1.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f}};
    const std::uint32_t ids[]{2u};
    BehaviorVector3 output{};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 1.0f, ids, runtime);
    Check(runtime.collisions.empty() && runtime.speed_calls == 0 &&
          output == BehaviorVector3{},
          "time equal to maximum is rejected before projection");

    Runtime boundary;
    boundary.objects[1u] = Object{0.5f, 0.0f, {}, {},
                                  {1.0f, 0.0f, 0.0f}};
    boundary.objects[2u] = Object{0.0f, 0.0f, {-1.0f, 0.0f, 0.0f},
                                  {1.0f, 0.0f, 0.0f},
                                  {1.0f, 0.0f, 0.0f}};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 2.0f, ids, boundary);
    Check(boundary.collisions.empty() &&
          Cached(state, 15u) == BehaviorVector3{-1.0f, 0.0f, 0.0f} &&
          output == BehaviorVector3{},
          "diameter equality rejects after state cache writes");
}

void TestAlignedEqualSpeed() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    runtime.objects[1u] = Object{0.6f, 1.0f, {}, {},
                                 {1.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f}};
    runtime.objects[2u] = Object{0.1f, 1.0f, {-1.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f}};
    const std::uint32_t ids[]{2u};
    BehaviorVector3 output{};
    pal4::ObjectComplexCollision::Process(
        state, 1u, output, 2.0f, ids, runtime);
    Check(runtime.collisions == std::vector<std::uint32_t>{2u} &&
          runtime.sign == -1.0f && runtime.direction_calls == 2 &&
          output == BehaviorVector3{-1.0f, 0.0f, 0.0f},
          "aligned equal speed uses offset-direction sign branch");
}

void TestCapturedPredictionOrder() {
    // Pair314 frame2268 inputs, first projected-point fork at2269. IDA42BEAD/
    //42BEB4 and42BEE6/42BEED scale forward by speed, then by prediction time.
    const auto vector=[](std::array<std::uint32_t,3> bits){return std::bit_cast<BehaviorVector3>(bits);};
    const auto scalar=[](std::uint32_t bits){return std::bit_cast<float>(bits);};
    Runtime runtime;pal4::ObjectMovementState state{};BehaviorVector3 output{};
    runtime.objects[1u]=Object{scalar(1124425960u),scalar(1112014848u),
        vector({1167878315u,1152290793u,1152409860u}),{},
        vector({1049630310u,0u,3212158803u}),vector({3212158803u,0u,3197113958u})};
    runtime.objects[2u]=Object{scalar(1124425960u),scalar(1112014849u),
        vector({1168009786u,1152031524u,1150024047u}),{},
        vector({3198173632u,0u,1064510204u}),vector({1064510206u,2147483648u,1050689985u})};
    for(auto& [id,object]:runtime.objects)
        for(unsigned axis=0;axis<3u;++axis)object.offset[axis]=object.forward[axis]*object.speed;
    const std::uint32_t ids[]{2u};
    pal4::ObjectComplexCollision::Process(state,1u,output,3.f,ids,runtime);
    Check(std::bit_cast<std::array<std::uint32_t,3>>(Cached(state,15u))==
        std::array<std::uint32_t,3>{1167914531u,1152031524u,1151180193u},
        "captured other prediction retains original speed-then-time rounding");
    Check(std::bit_cast<std::array<std::uint32_t,3>>(Cached(state,18u))==
        std::array<std::uint32_t,3>{1167963959u,1152290793u,1151241746u},
        "captured self prediction matches original");
}

}  // namespace

int main() {
    TestEmptyAndSelfOnly();
    TestProximityExit();
    TestZeroProjectionFallsThrough();
    TestNearestCandidateAndAlignedSpeedGate();
    TestOppositeAndTransverseSign();
    TestZeroDistanceAndSignEquality();
    TestStrictTimeAndDiameter();
    TestAlignedEqualSpeed();
    TestCapturedPredictionOrder();
}

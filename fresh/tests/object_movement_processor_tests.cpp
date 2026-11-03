#include "pal4/object_movement_processor.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pal4::BehaviorVector3;
using pal4::ObjectCollisionCandidate;
using pal4::ObjectMovementState;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void CheckVector(const BehaviorVector3& actual, const BehaviorVector3& expected,
                 const char* message) {
    Check(actual == expected, message);
}

struct Runtime final : pal4::IObjectMovementRuntime {
    BehaviorVector3 position{1.0f, 2.0f, 3.0f};
    BehaviorVector3 reference{};
    BehaviorVector3 candidate_response{};
    BehaviorVector3 complex_response{};
    BehaviorVector3 drift_response{};
    BehaviorVector3 collision_response{};
    BehaviorVector3 collision_input{11.0f, 12.0f, 13.0f};
    BehaviorVector3 unclamped_input{21.0f, 22.0f, 23.0f};
    std::vector<std::int32_t> random_values{0, 0};
    std::vector<ObjectCollisionCandidate> records;
    std::vector<std::string> calls;
    std::size_t random_index = 0u;
    float candidate_range = 1.0f;
    float movement_range = 2.0f;
    std::uint32_t selected = 0u;
    float selected_range = 0.0f;
    float target_range = 0.0f;
    float first_angle = 0.0f;
    float second_angle = 0.0f;
    std::uint32_t collision_option = 0u;
    std::uint32_t collision_proxy = 0u;

    BehaviorVector3 Position(ObjectMovementState&) override {
        calls.emplace_back("position");
        return position;
    }
    std::int32_t Rand() override {
        calls.emplace_back("rand");
        Check(random_index < random_values.size(), "unexpected rand call");
        return random_values[random_index++];
    }
    float CandidateRange(ObjectMovementState&) override {
        calls.emplace_back("candidate_range");
        return candidate_range;
    }
    void CalculateCandidate(ObjectMovementState&, std::uint32_t object,
                            ObjectCollisionCandidate& candidate) override {
        calls.emplace_back("candidate");
        Check(object < records.size(), "candidate index");
        candidate = records[object];
    }
    void NotifyCandidate(ObjectMovementState&, std::uint32_t candidate_token,
                         float range) override {
        calls.emplace_back("notify_candidate");
        selected = candidate_token;
        selected_range = range;
    }
    BehaviorVector3 CandidateResponse(
        ObjectMovementState&, const ObjectCollisionCandidate&, float) override {
        calls.emplace_back("candidate_response");
        return candidate_response;
    }
    BehaviorVector3 ComparisonVector() override {
        calls.emplace_back("comparison");
        return reference;
    }
    float MovementRange(ObjectMovementState&) override {
        calls.emplace_back("movement_range");
        return movement_range;
    }
    void CopyGlobalCollection() override {
        calls.emplace_back("copy_collection");
    }
    void NotifyTarget(ObjectMovementState&, float range,
                      pal4::ObjectMovementTarget& target) override {
        calls.emplace_back("notify_target");
        target_range = range;
        target.collision_input = collision_input;
        target.unclamped_input = unclamped_input;
    }
    BehaviorVector3 ComplexCollision(
        ObjectMovementState&, const BehaviorVector3& input) override {
        calls.emplace_back("complex_collision");
        Check(input == collision_input, "complex collision input");
        return complex_response;
    }
    float RandomClamped(float value, float radius) override {
        calls.emplace_back("random_clamped");
        Check(radius == 6.0f, "delta times twelve");
        return value + 0.5f;
    }
    BehaviorVector3 DirectionDrift(ObjectMovementState&, float first,
                                   float second,
                                   const BehaviorVector3& input) override {
        calls.emplace_back("drift");
        Check(input == collision_input, "drift collision input");
        first_angle = first;
        second_angle = second;
        return drift_response;
    }
    std::uint32_t ResolveCollisionProxy(std::uint32_t object) override {
        calls.emplace_back("proxy");
        return object + 4u;
    }
    BehaviorVector3 ClampedCollision(ObjectMovementState&,
                                     const BehaviorVector3& input,
                                     std::uint32_t option,
                                     std::uint32_t proxy) override {
        calls.emplace_back("clamped");
        Check(input == collision_input, "clamped collision input");
        collision_option = option;
        collision_proxy = proxy;
        return collision_response;
    }
    BehaviorVector3 UnclampedCollision(ObjectMovementState&,
                                       const BehaviorVector3& input,
                                       std::uint32_t proxy) override {
        calls.emplace_back("unclamped");
        Check(input == unclamped_input, "unclamped collision input");
        collision_proxy = proxy;
        return collision_response;
    }
};

ObjectCollisionCandidate Candidate(bool hit, float time, std::uint32_t id) {
    ObjectCollisionCandidate result;
    result.words[0] = hit ? 1u : 0u;
    result.words[1] = std::bit_cast<std::uint32_t>(time);
    result.words[8] = id;
    return result;
}

void TestCandidateSelectionAndEarlyExit() {
    ObjectMovementState state;
    Runtime runtime;
    runtime.random_values = {32767};
    runtime.records = {Candidate(false, 0.1f, 1u),
                       Candidate(true, 0.5f, 2u),
                       Candidate(true, 0.5f, 3u),
                       Candidate(true, 0.8f, 4u)};
    runtime.candidate_response = {2.0f, 4.0f, 5.0f};
    BehaviorVector3 output{};
    const std::uint32_t ids[]{0u, 1u, 2u, 3u};
    auto& returned = pal4::ObjectMovementProcessor::Process(
        state, output, 0.5f, ids, runtime);
    Check(&returned == &output, "return output address");
    Check(runtime.selected == 2u && runtime.selected_range == 4.0f,
          "strict nearest hit and range times four");
    Check(runtime.random_index == 1u, "nonzero first move skips second gate");
    CheckVector(output, {3.0f, 0.0f, 8.0f}, "first move projects XZ");
    Check(runtime.calls.back() == "comparison", "first exit call order");
}

void TestCandidateThresholdAndMiss() {
    ObjectMovementState state;
    Runtime runtime;
    runtime.random_values = {32767, 0};
    runtime.records = {Candidate(true, 4.0f, 1u)};
    runtime.collision_response = {4.0f, 5.0f, 6.0f};
    BehaviorVector3 output{};
    const std::uint32_t ids[]{0u};
    pal4::ObjectMovementProcessor::Process(state, output, 0.5f, ids, runtime);
    Check(runtime.selected == 0u, "time equal to range has no response");
    Check(runtime.target_range == 12.0f, "movement range times six");
    Check(runtime.random_index == 2u, "second random gate reached");
    CheckVector(output, {3.0f, 0.0f, 6.0f}, "fallback response halved");
}

void TestComplexCollisionExit() {
    ObjectMovementState state;
    Runtime runtime;
    runtime.random_values = {0, 32767};
    runtime.complex_response = {0.5f, 1.0f, -0.5f};
    BehaviorVector3 output{};
    pal4::ObjectMovementProcessor::Process(state, output, 0.0f, {}, runtime);
    CheckVector(output, {6.0f, 0.0f, -2.0f}, "complex collision times ten");
    Check(runtime.calls.back() == "comparison", "second exit avoids proxy");
}

void TestDriftAndClampedCollision() {
    ObjectMovementState state;
    state.words[13] = std::bit_cast<std::uint32_t>(0.25f);
    state.words[14] = std::bit_cast<std::uint32_t>(-0.25f);
    state.words[47] = 0x7000u;
    state.words[48] = 0x123u;
    state.words[49] = 0x00000101u;
    Runtime runtime;
    runtime.drift_response = {2.0f, 3.0f, 4.0f};
    runtime.collision_response = {4.0f, 8.0f, 6.0f};
    BehaviorVector3 output{};
    pal4::ObjectMovementProcessor::Process(state, output, 0.5f, {}, runtime);
    Check(runtime.first_angle == 0.75f && runtime.second_angle == 0.25f,
          "random angles forwarded in order");
    Check(runtime.collision_option == 0x123u &&
          runtime.collision_proxy == 0x7004u, "clamped inputs");
    Check(std::bit_cast<float>(state.words[13]) == 0.75f &&
          std::bit_cast<float>(state.words[14]) == 0.25f,
          "orientation state writes");
    CheckVector(output, {5.0f, 0.0f, 10.0f}, "drift and half collision");
    Check(runtime.calls.back() == "clamped", "clamped route");
}

void TestUnclampedAndStub() {
    ObjectMovementState state;
    state.words[47] = 0x2000u;
    Runtime runtime;
    runtime.collision_response = {-2.0f, 0.0f, 4.0f};
    BehaviorVector3 output{};
    pal4::ObjectMovementProcessor::Process(state, output, 0.0f, {}, runtime);
    CheckVector(output, {0.0f, 0.0f, 5.0f}, "unclamped half response");
    Check(runtime.calls.back() == "unclamped" &&
          runtime.collision_proxy == 0x2004u, "unclamped route");

    pal4::ObjectMovementRuntimeStub stub;
    pal4::ObjectMovementProcessor::Process(state, output, 0.0f, {}, stub);
    CheckVector(output, {0.0f, 0.0f, 0.0f}, "linkable empty stub");
}

}  // namespace

int main() {
    TestCandidateSelectionAndEarlyExit();
    TestCandidateThresholdAndMiss();
    TestComplexCollisionExit();
    TestDriftAndClampedCollision();
    TestUnclampedAndStub();
}

#include "pal4/object_random_movement_collision.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectRandomMovementCollisionRuntime {
    pal4::BehaviorVector3 base{1.0f, 2.0f, 3.0f};
    pal4::BehaviorVector3 complex{};
    pal4::BehaviorVector3 scan_position{3.0f, 4.0f, 5.0f};
    pal4::BehaviorVector3 candidate_vector{};
    pal4::BehaviorVector3 positions[2]{};
    std::vector<std::int32_t> randoms{0, 0};
    std::vector<std::string> calls;
    std::vector<float> candidate_times;
    std::vector<bool> candidate_hits;
    std::vector<std::uint32_t> candidate_addresses;
    std::uint32_t notified_candidate = 0;
    int rand_count = 0;
    int speed_count = 0;
    int position_count = 0;

    pal4::BehaviorVector3 Position(
        pal4::ObjectRandomMovementState&) override {
        calls.emplace_back("position");
        Check(position_count < 2, "at most two spatial position reads");
        return positions[position_count++];
    }
    pal4::BehaviorVector3 RandomVector() override {
        calls.emplace_back("random_vector");
        return {};
    }
    pal4::BehaviorVector3 BaseVector(
        pal4::ObjectRandomMovementState&) override {
        calls.emplace_back("base");
        return base;
    }
    std::int32_t Rand() override {
        calls.emplace_back("rand");
        Check(rand_count < static_cast<int>(randoms.size()),
              "random source read count");
        return randoms[rand_count++];
    }
    float Speed(pal4::ObjectRandomMovementState&) override {
        calls.emplace_back("speed");
        ++speed_count;
        return 2.0f;
    }
    void CalculateCandidate(pal4::ObjectRandomMovementState&,
                            std::uint32_t address,
                            pal4::ObjectCollisionCandidate& record) override {
        calls.emplace_back("candidate");
        const auto index = candidate_addresses.size();
        Check(index < candidate_times.size(), "candidate count");
        candidate_addresses.push_back(address);
        record.words[0] = candidate_hits.empty() || candidate_hits[index]
            ? 1u : 0u;
        record.words[1] = std::bit_cast<std::uint32_t>(candidate_times[index]);
        record.words[8] = address;
    }
    void NotifyCandidate(pal4::ObjectRandomMovementState&,
                         std::uint32_t address, float range) override {
        calls.emplace_back("notify_candidate");
        notified_candidate = address;
        Check(range == 8.0f, "candidate callback receives four times speed");
    }
    pal4::BehaviorVector3 CollisionPosition(
        pal4::ObjectRandomMovementState& state) override {
        calls.emplace_back("collision_position");
        Check(notified_candidate != 0u,
              "collision position follows candidate notification");
        return {std::bit_cast<float>(state.words[10]),
                std::bit_cast<float>(state.words[11]),
                std::bit_cast<float>(state.words[12])};
    }
    pal4::BehaviorVector3 CandidateVector(std::uint32_t address) override {
        calls.emplace_back("candidate_vector");
        Check(address == notified_candidate + 8u,
              "candidate vector uses selected candidate token plus eight");
        return candidate_vector;
    }
    pal4::BehaviorVector3 CollisionAxis(
        pal4::ObjectRandomMovementState& state) override {
        calls.emplace_back("axis");
        return {std::bit_cast<float>(state.words[7]),
                std::bit_cast<float>(state.words[8]),
                std::bit_cast<float>(state.words[9])};
    }
    float CorrectionScale(pal4::ObjectRandomMovementState& state) override {
        calls.emplace_back("scale");
        return std::bit_cast<float>(state.words[26]);
    }
    pal4::BehaviorVector3 GlobalZeroVector() override {
        calls.emplace_back("zero");
        return {};
    }
    float FallbackRange(pal4::ObjectRandomMovementState&) override {
        calls.emplace_back("fallback_range");
        return 4.0f;
    }
    void CopyGlobalCollection() override {
        calls.emplace_back("copy_global");
    }
    pal4::BehaviorVector3 NotifyMemberScan(
        pal4::ObjectRandomMovementState&, std::uint32_t member,
        float range) override {
        calls.emplace_back("notify_member");
        Check(member == 0x6000u && range == 24.0f,
              "member scan receives address and six times fallback range");
        return scan_position;
    }
    pal4::BehaviorVector3 ComplexCollision(
        pal4::ObjectRandomMovementState&,
        const pal4::BehaviorVector3& position, float range) override {
        calls.emplace_back("complex");
        Check(position == scan_position && range == 3.0f,
              "complex collision receives member-scan position and range three");
        return complex;
    }
};

void TestEarliestCandidateAndDirectCorrection() {
    pal4::ObjectRandomMovementState state;
    state.words[42] = 0x6000u;
    state.words[7] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[8] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[9] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[26] = std::bit_cast<std::uint32_t>(4.0f);
    Runtime runtime;
    runtime.randoms = {20000};
    runtime.candidate_times = {3.0f, 1.0f, 2.0f};
    const std::uint32_t candidates[] = {0x1000u, 0x2000u, 0x3000u};
    pal4::BehaviorVector3 output{};
    Check(&pal4::ObjectRandomMovementCollision::Process(
              state, output, 0.25f, candidates, runtime) == &output &&
              output == pal4::BehaviorVector3{4.0f, 0.0f, 9.0f} &&
              runtime.notified_candidate == 0x2000u &&
              runtime.candidate_addresses ==
                  std::vector<std::uint32_t>{0x1000u, 0x2000u, 0x3000u},
          "earliest candidate correction adds to base and projects XZ");
    Check(runtime.calls ==
              std::vector<std::string>{"base", "rand", "speed",
                                       "candidate", "candidate", "candidate",
                                       "notify_candidate", "collision_position",
                                       "candidate_vector", "axis", "scale",
                                       "scale", "axis", "zero"},
          "nonzero correction skips fallback path");
}

void TestComplexFallback() {
    pal4::ObjectRandomMovementState state;
    state.words[42] = 0x6000u;
    Runtime runtime;
    runtime.randoms = {0, 20000};
    runtime.complex = {0.5f, 9.0f, -0.25f};
    pal4::BehaviorVector3 output{};
    pal4::ObjectRandomMovementCollision::Process(
        state, output, 0.25f, {}, runtime);
    Check(output == pal4::BehaviorVector3{6.0f, 0.0f, 0.5f} &&
              runtime.speed_count == 0 && runtime.position_count == 0,
          "second random gate scales complex collision by ten");
    Check(runtime.calls ==
              std::vector<std::string>{"base", "rand", "zero",
                                       "fallback_range", "copy_global",
                                       "notify_member", "rand", "complex"},
          "complex fallback call order");
}

void TestNoHitThenHitAndZeroCorrection() {
    pal4::ObjectRandomMovementState state;
    state.words[42] = 0x6000u;
    Runtime runtime;
    runtime.randoms = {20000, 20000};
    runtime.candidate_times = {7.0f, 1.0f};
    runtime.candidate_hits = {false, true};
    runtime.complex = {0.5f, 0.0f, 0.0f};
    const std::uint32_t candidates[] = {0x1000u, 0x2000u};
    pal4::BehaviorVector3 output{};
    pal4::ObjectRandomMovementCollision::Process(
        state, output, 0.25f, candidates, runtime);
    Check(runtime.notified_candidate == 0x2000u &&
              output == pal4::BehaviorVector3{6.0f, 0.0f, 3.0f} &&
              runtime.rand_count == 2 &&
              runtime.calls ==
                  std::vector<std::string>{"base", "rand", "speed",
                                           "candidate", "candidate",
                                           "notify_candidate", "collision_position",
                                           "candidate_vector", "axis", "scale",
                                           "scale", "axis", "zero", "fallback_range",
                                           "copy_global", "notify_member",
                                           "rand", "complex"},
          "hit after no-hit is selected and zero correction enters fallback");
}

void TestStrictCollisionRange() {
    pal4::ObjectRandomMovementState state;
    state.words[42] = 0x6000u;
    Runtime runtime;
    runtime.randoms = {20000, 20000};
    runtime.candidate_times = {8.0f};
    runtime.complex = {0.5f, 0.0f, 0.0f};
    const std::uint32_t candidates[] = {0x1000u};
    pal4::BehaviorVector3 output{};
    pal4::ObjectRandomMovementCollision::Process(
        state, output, 0.25f, candidates, runtime);
    Check(runtime.notified_candidate == 0u &&
              output == pal4::BehaviorVector3{6.0f, 0.0f, 3.0f} &&
              runtime.calls ==
                  std::vector<std::string>{"base", "rand", "speed",
                                           "candidate", "zero",
                                           "fallback_range", "copy_global",
                                           "notify_member", "rand", "complex"},
          "hit time equal to four times speed is rejected");
}

void TestHeadingFallback() {
    pal4::ObjectRandomMovementState state;
    state.words[42] = 0x6000u;
    state.words[47] = std::bit_cast<std::uint32_t>(5.0f);
    state.words[53] = std::bit_cast<std::uint32_t>(10.0f);
    Runtime runtime;
    runtime.positions[0] = {0.0f, 0.0f, 0.0f};
    runtime.positions[1] = {0.0f, 0.0f, 0.0f};
    pal4::BehaviorVector3 output{};
    pal4::ObjectRandomMovementCollision::Process(
        state, output, 0.25f, {}, runtime);
    Check(output == pal4::BehaviorVector3{3.0f, 0.0f, 3.0f} &&
              runtime.speed_count == 1 && runtime.position_count == 2 &&
              std::bit_cast<float>(state.words[50]) == 1.0f,
          "zero secondary correction uses reconstructed heading update");
    Check(runtime.calls ==
              std::vector<std::string>{"base", "rand", "zero",
                                       "fallback_range", "copy_global",
                                       "notify_member", "rand", "position",
                                       "position", "speed"},
          "heading fallback call order");
}

void TestEqualTimesKeepFirstCandidate() {
    pal4::ObjectRandomMovementState state;
    state.words[7] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[26] = std::bit_cast<std::uint32_t>(4.0f);
    Runtime runtime;
    runtime.randoms = {3277};
    runtime.candidate_times = {1.0f, 1.0f};
    runtime.candidate_vector = {80.0f, -30.0f, 40.0f};
    const std::uint32_t candidates[] = {0x1000u, 0x2000u};
    pal4::BehaviorVector3 output{};
    pal4::ObjectRandomMovementCollision::Process(
        state, output, 0.25f, candidates, runtime);
    Check(runtime.notified_candidate == 0x1000u &&
              std::fabs(output[0] - 4.0f) < 0.0001f &&
              output[1] == 0.0f &&
              std::fabs(output[2] + 0.2f) < 0.0001f,
          "equal collision times retain first candidate and projected difference affects correction");
}

void TestRandomGateThreshold() {
    pal4::ObjectRandomMovementState state;
    state.words[42] = 0x6000u;
    Runtime below;
    below.randoms = {3276, 0};
    pal4::BehaviorVector3 output{};
    pal4::ObjectRandomMovementCollision::Process(
        state, output, 0.25f, {}, below);
    Check(below.speed_count == 1 && below.rand_count == 2,
          "sample below threshold skips candidate scan");

    Runtime above;
    above.randoms = {3277, 0};
    pal4::ObjectRandomMovementCollision::Process(
        state, output, 0.25f, {}, above);
    Check(above.speed_count == 2 && above.rand_count == 2,
          "sample above threshold enters candidate scan and heading fallback");
}

}  // namespace

int main() {
    TestEarliestCandidateAndDirectCorrection();
    TestComplexFallback();
    TestNoHitThenHitAndZeroCorrection();
    TestStrictCollisionRange();
    TestHeadingFallback();
    TestEqualTimesKeepFirstCandidate();
    TestRandomGateThreshold();
    pal4::ObjectRandomMovementState state;
    pal4::BehaviorVector3 output{};
    pal4::ObjectRandomMovementCollisionRuntimeStub stub;
    Check(&pal4::ObjectRandomMovementCollision::Process(
              state, output, 0.0f, {}, stub) == &output,
          "default runtime links");
    pal4::ObjectCollisionCandidate candidate{};
    candidate.InitializeBody();
    stub.CalculateCandidate(state, 0x1234u, candidate);
    Check(candidate.words[8] == 0x1234u,
          "default runtime composes reconstructed collision-time calculator");
    std::cout << "7 random movement collision cases passed\n";
}

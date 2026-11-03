#include "pal4/object_collision_time.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectCollisionTimeRuntime {
    pal4::ObjectCollisionCandidate* record = nullptr;
    pal4::BehaviorVector3 vector{};
    float object_radius = 0.0f;
    float candidate_radius = 0.0f;
    std::vector<std::string> calls;

    pal4::BehaviorVector3 RelativeVector(
        pal4::ObjectMovementState&, std::uint32_t address) override {
        calls.emplace_back("vector");
        Check(address == 0x4008u && record->words[0] == 0u &&
              record->words[8] == 0x4000u,
              "candidate reset and token precede virtual vector");
        return vector;
    }
    float ObjectRadius(pal4::ObjectMovementState&) override {
        calls.emplace_back("object_radius");
        return object_radius;
    }
    float CandidateRadius(std::uint32_t address) override {
        calls.emplace_back("candidate_radius");
        Check(address == 0x4000u, "candidate radius address");
        return candidate_radius;
    }
};

void TestRecordHelpers() {
    pal4::ObjectCollisionCandidate record;
    record.words.fill(0xaabbccddu);
    Check(&record.InitializeBody() == &record &&
          record.words[0] == 0xaabbccddu &&
          record.words[1] == 0xaabbccddu &&
          record.words[8] == 0xaabbccddu,
          "initialize preserves outer three words");
    for (std::size_t word = 2u; word != 8u; ++word)
        Check(record.words[word] == 0u, "initialize clears six body words");

    pal4::ObjectCollisionCandidate source;
    for (std::uint32_t word = 0u; word != 9u; ++word)
        source.words[word] = word + 10u;
    Check(&record.CopyFrom(source) == &record && record.words == source.words,
          "copy all nine words and return destination");
    Check(&record.CopyFrom(record) == &record && record.words == source.words,
          "self copy remains stable");
}

void TestPositiveAndNegativeRoots() {
    pal4::ObjectMovementState state;
    pal4::ObjectCollisionCandidate record;
    record.words.fill(0x11223344u);
    Runtime runtime;
    runtime.record = &record;
    runtime.vector = {0.0f, 0.0f, 3.0f};
    runtime.object_radius = 1.0f;
    pal4::ObjectCollisionTime::Calculate(
        state, 0x4000u, record, runtime);
    Check(record.HasHit() && record.Time() == 2.0f &&
          record.words[8] == 0x4000u &&
          record.words[2] == 0x11223344u &&
          runtime.calls == std::vector<std::string>{
              "vector", "object_radius", "candidate_radius"},
          "choose smaller positive root and preserve body");

    runtime.vector = {0.0f, 0.0f, -3.0f};
    pal4::ObjectCollisionTime::Calculate(
        state, 0x4000u, record, runtime);
    Check(!record.HasHit() && record.words[1] ==
          std::bit_cast<std::uint32_t>(2.0f),
          "two negative roots leave old time untouched");
}

void TestNoRealRootAndSplitRadius() {
    pal4::ObjectMovementState state;
    pal4::ObjectCollisionCandidate record;
    record.words[1] = 0xdeadbeefu;
    Runtime runtime;
    runtime.record = &record;
    runtime.vector = {3.0f, 0.0f, 0.0f};
    runtime.object_radius = 1.0f;
    runtime.candidate_radius = 1.0f;
    pal4::ObjectCollisionTime::Calculate(
        state, 0x4000u, record, runtime);
    Check(!record.HasHit() && record.words[1] == 0xdeadbeefu,
          "negative discriminant leaves time unchanged");
    runtime.candidate_radius = 3.0f;
    pal4::ObjectCollisionTime::Calculate(
        state, 0x4000u, record, runtime);
    Check(record.HasHit() &&
          std::abs(record.Time() - std::sqrt(7.0f)) < 0.00001f,
          "radii add before squaring");
}

void TestZeroRootAndUnordered() {
    pal4::ObjectMovementState state;
    pal4::ObjectCollisionCandidate record;
    Runtime runtime;
    runtime.record = &record;
    runtime.vector = {0.0f, 0.0f, -1.0f};
    runtime.object_radius = 1.0f;
    pal4::ObjectCollisionTime::Calculate(
        state, 0x4000u, record, runtime);
    Check(record.HasHit() && record.Time() == -2.0f,
          "zero larger root stores negative smaller root");

    runtime.vector[0] = std::numeric_limits<float>::quiet_NaN();
    pal4::ObjectCollisionTime::Calculate(
        state, 0x4000u, record, runtime);
    Check(record.HasHit() && std::isnan(record.Time()),
          "unordered FPU comparisons keep hit path");

    pal4::ObjectCollisionTimeRuntimeStub stub;
    pal4::ObjectCollisionTime::Calculate(state, 0u, record, stub);
    Check(record.words[8] == 0u, "linkable time runtime stub");
}

}  // namespace

int main() {
    TestRecordHelpers();
    TestPositiveAndNegativeRoots();
    TestNoRealRootAndSplitRadius();
    TestZeroRootAndUnordered();
}

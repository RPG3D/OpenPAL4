#include "pal4/pal_movement_secondary_collision.h"

#include <array>
#include <cstdint>
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

struct Runtime final : pal4::IPalMovementSecondaryCollisionRuntime {
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> clump_lookups;
    std::vector<std::uint32_t> queries;
    std::vector<std::uint32_t> processed;
    bool allow_query = true;
    bool allow_collision = true;
    float height = 6.0f;
    double threshold = 20.0;
    std::vector<std::string> switches;

    std::uint32_t ReadElement(std::uint32_t address) override {
        reads.push_back(address);
        return (address - 0x1000u) / 4u + 1u;
    }
    std::uint32_t GetClump(std::uint32_t element) override {
        clump_lookups.push_back(element);
        return element + 0x100u;
    }
    bool QueryCollisionData(std::uint32_t clump) override {
        queries.push_back(clump);
        return allow_query && clump != 0x101u;
    }
    bool ProcessClumpCollision(
        const std::array<float, 6>& line, std::uint32_t clump,
        pal4::PalMovementCollisionCandidate& candidate) override {
        processed.push_back(clump);
        Check(line == std::array<float, 6>{1.0f, 23.0f, 3.0f,
                                            1.0f, 16.0f, 3.0f},
              "collision service receives the exact vertical line");
        candidate.field80 = clump + 0x1000u;
        candidate.field84 = clump + 0x2000u;
        candidate.height = height;
        if (clump == 0x102u) return false;
        if (clump == 0x104u) candidate.height = 3.0f;
        if (clump == 0x105u) candidate.height = 8.0f;
        return allow_collision;
    }
    double SwitchThreshold(std::uint32_t element) override {
        Check(element == 3u, "switch threshold belongs to selected element");
        switches.emplace_back("threshold");
        return threshold;
    }
    void Leave(std::uint32_t element) override {
        Check(element == 9u, "leave callback receives prior active element");
        switches.emplace_back("leave");
    }
    void Enter(std::uint32_t element) override {
        Check(element == 3u, "enter callback receives selected element");
        switches.emplace_back("enter");
    }
};

void TestEmptyAndDisabledQueries() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1000u;
    state.words[29] = 0x1000u;
    state.words[32] = 77u;
    state.words[33] = 88u;
    const std::array<float, 3> position{1.0f, 20.0f, 3.0f};
    std::array<float, 3> output{9.0f, 9.0f, 9.0f};
    Runtime runtime;
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 1u, 1u, position, 3.0f, 4.0f,
              &output, runtime) == 0u &&
              state.words[32] == 0xffffffffu &&
              state.words[33] == 88u && runtime.reads.empty() &&
              output == std::array<float, 3>{9.0f, 9.0f, 9.0f},
          "empty array resets field80 and leaves other outputs untouched");

    state.words[29] = 0x1004u;
    runtime.allow_query = false;
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 1u, 1u, position, 3.0f, 4.0f,
              &output, runtime) == 0u &&
              runtime.reads == std::vector<std::uint32_t>{0x1000u} &&
              runtime.clump_lookups ==
                  std::vector<std::uint32_t>{1u} &&
              runtime.queries ==
                  std::vector<std::uint32_t>{0x101u} &&
              runtime.processed.empty(),
          "world-query failure skips the second virtual call and collision");
}

void TestHighestHitAndOutputFlags() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1000u;
    state.words[29] = 0x1014u;
    state.words[33] = 0xdeadbeefu;
    const std::array<float, 3> position{1.0f, 20.0f, 3.0f};
    std::array<float, 3> output{};
    Runtime runtime;
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 1u, 1u, position, 3.0f, 4.0f,
              &output, runtime) == 1u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1000u, 0x1004u, 0x1008u, 0x100cu, 0x1010u} &&
              runtime.clump_lookups == std::vector<std::uint32_t>{
                  1u, 2u, 2u, 3u, 3u, 4u, 4u, 5u, 5u} &&
              runtime.processed == std::vector<std::uint32_t>{
                  0x102u, 0x103u, 0x104u, 0x105u} &&
              state.words[32] == 0x1105u &&
              state.words[33] == 0x2105u &&
              output == std::array<float, 3>{1.0f, 8.0f, 3.0f},
          "strictly highest hit replaces optional fields and output position");

    runtime.reads.clear();
    runtime.clump_lookups.clear();
    runtime.processed.clear();
    state.words[33] = 0x77777777u;
    state.words[29] = 0x100cu;
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 0u, 0u, position, 3.0f, 4.0f,
              nullptr, runtime) == 1u &&
              state.words[32] == 0xffffffffu &&
              state.words[33] == 0x77777777u,
          "accepted hit leaves disabled optional outputs unchanged");
}

void TestThresholdOrdering() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1008u;
    state.words[29] = 0x100cu;
    const std::array<float, 3> position{1.0f, 20.0f, 3.0f};
    Runtime runtime;
    runtime.height = -std::numeric_limits<float>::max();
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 1u, 1u, position, 3.0f, 4.0f,
              nullptr, runtime) == 0u &&
              state.words[32] == 0xffffffffu,
          "height equal to initial best is rejected");
    runtime.height = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 1u, 1u, position, 3.0f, 4.0f,
              nullptr, runtime) == 0u,
          "unordered height is rejected");
    runtime.allow_collision = false;
    runtime.height = 50.0f;
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 1u, 1u, position, 3.0f, 4.0f,
              nullptr, runtime) == 0u,
          "failed collision cannot update the best hit");
}

void TestLinkableStub() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1000u;
    state.words[29] = 0x1004u;
    pal4::PalMovementSecondaryCollisionRuntimeStub runtime;
    Check(pal4::PalMovementSecondaryChild::FindHighestCollision(
              state, 1u, 1u, {1.0f, 20.0f, 3.0f},
              3.0f, 4.0f, nullptr, runtime) == 0u,
          "external collision services have a linkable no-hit stub");
    Check(pal4::PalMovementSecondaryChild::HasCollisionWithinHeight(
              state, {1.0f, 20.0f, 3.0f}, 3.0f, 4.0f,
              1u, runtime) == 0u &&
              pal4::PalMovementSecondaryChild::UpdateActiveCollision(
                  state, {1.0f, 20.0f, 3.0f},
                  3.0f, 4.0f, runtime) == 0u,
          "other collision entries also compose with no-hit stub");
}

void TestHeightGate() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1008u;
    state.words[29] = 0x100cu;
    const std::array<float, 3> position{1.0f, 20.0f, 3.0f};
    Runtime runtime;
    runtime.height = 10.0f;
    Check(pal4::PalMovementSecondaryChild::HasCollisionWithinHeight(
              state, position, 3.0f, 4.0f, 1u, runtime) == 1u,
          "distance equal to ten preserves collision");
    runtime.height = 9.0f;
    Check(pal4::PalMovementSecondaryChild::HasCollisionWithinHeight(
              state, position, 3.0f, 4.0f, 1u, runtime) == 0u &&
              pal4::PalMovementSecondaryChild::HasCollisionWithinHeight(
                  state, position, 3.0f, 4.0f, 0u, runtime) == 1u,
          "enabled gate rejects distance above ten; disabled gate keeps hit");
    runtime.height = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::PalMovementSecondaryChild::HasCollisionWithinHeight(
              state, position, 3.0f, 4.0f, 1u, runtime) == 0u,
          "unordered candidate height cannot become best hit");
    state.words[29] = state.words[28];
    Check(pal4::PalMovementSecondaryChild::HasCollisionWithinHeight(
              state, position, 3.0f, 4.0f, 1u, runtime) == 0u,
          "empty collection returns zero with height check enabled");
}

void TestActiveSwitch() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1008u;
    state.words[29] = 0x100cu;
    state.words[31] = 9u;
    const std::array<float, 3> position{1.0f, 20.0f, 3.0f};
    Runtime runtime;
    runtime.height = 6.0f;
    runtime.threshold = 14.0;
    Check(pal4::PalMovementSecondaryChild::UpdateActiveCollision(
              state, position, 3.0f, 4.0f, runtime) == 1u &&
              runtime.switches ==
                  std::vector<std::string>{"threshold", "leave", "enter"} &&
              state.words[31] == 3u,
          "equal switch threshold leaves old before entering new element");
    runtime.switches.clear();
    Check(pal4::PalMovementSecondaryChild::UpdateActiveCollision(
              state, position, 3.0f, 4.0f, runtime) == 1u &&
              runtime.switches.empty(),
          "already-active best element skips threshold and callbacks");

    state.words[31] = 9u;
    runtime.threshold = 13.0;
    Check(pal4::PalMovementSecondaryChild::UpdateActiveCollision(
              state, position, 3.0f, 4.0f, runtime) == 0u &&
              state.words[31] == 9u &&
              runtime.switches == std::vector<std::string>{"threshold"},
          "threshold below distance rejects switch without callbacks");
    runtime.switches.clear();
    runtime.threshold = std::numeric_limits<double>::quiet_NaN();
    Check(pal4::PalMovementSecondaryChild::UpdateActiveCollision(
              state, position, 3.0f, 4.0f, runtime) == 0u &&
              runtime.switches == std::vector<std::string>{"threshold"},
          "unordered threshold also rejects switch");

    runtime.switches.clear();
    state.words[31] = 0u;
    runtime.threshold = 14.0;
    Check(pal4::PalMovementSecondaryChild::UpdateActiveCollision(
              state, position, 3.0f, 4.0f, runtime) == 1u &&
              runtime.switches ==
                  std::vector<std::string>{"threshold", "enter"},
          "switch from null active element only invokes enter");

    runtime.switches.clear();
    state.words[29] = state.words[28];
    Check(pal4::PalMovementSecondaryChild::UpdateActiveCollision(
              state, position, 3.0f, 4.0f, runtime) == 0u &&
              runtime.switches.empty() && state.words[31] == 3u,
          "no clumps leaves the current active element untouched");
}

}  // namespace

int main() {
    TestEmptyAndDisabledQueries();
    TestHighestHitAndOutputFlags();
    TestThresholdOrdering();
    TestHeightGate();
    TestActiveSwitch();
    TestLinkableStub();
}

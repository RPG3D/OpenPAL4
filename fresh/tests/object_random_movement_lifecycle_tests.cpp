#include "pal4/object_random_movement_lifecycle.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectRandomMovementRuntime {
    pal4::BehaviorVector3 random{3.0f, 99.0f, 4.0f};
    bool throw_on_random = false;
    std::vector<std::string> calls;
    std::uint32_t freed = 0;

    void ConstructBase(pal4::ObjectRandomMovementState& state) override {
        calls.emplace_back("construct");
        Check(state.words[0] == 0x1111u && state.words[55] == 0x5555u,
              "base constructor precedes derived field writes");
    }
    void CleanupBase(pal4::ObjectRandomMovementState& state) override {
        calls.emplace_back("cleanup");
        Check(state.words[0] == pal4::ObjectRandomMovementLifecycle::kVtable,
              "derived vtable is set before base cleanup");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    pal4::BehaviorVector3 RandomVector() override {
        calls.emplace_back("random");
        if (throw_on_random) throw std::runtime_error("random failed");
        return random;
    }
};

void TestConstructAndNormalize() {
    pal4::ObjectRandomMovementState state;
    state.words.fill(0x5555u);
    state.words[0] = 0x1111u;
    Runtime runtime;
    Check(&pal4::ObjectRandomMovementLifecycle::Construct(
              state, runtime) == &state &&
              runtime.calls == std::vector<std::string>{"construct", "random"},
          "constructor returns object after random source");
    Check(state.words[0] == pal4::ObjectRandomMovementLifecycle::kVtable &&
              state.words[47] == 0u &&
              state.words[48] == 0x40000000u && state.words[49] == 0u,
          "vtable and scalar fields follow observed initialization");
    Check(std::fabs(std::bit_cast<float>(state.words[50]) - 0.6f) < 0.0001f &&
              state.words[51] == 0u &&
              std::fabs(std::bit_cast<float>(state.words[52]) - 0.8f) < 0.0001f &&
              state.words[53] == 0u && state.words[54] == 0u &&
              state.words[55] == 0u && state.words[46] == 0x5555u,
          "only XZ random components are normalized into 224-byte state");
}

void TestZeroAndUnorderedDirection() {
    pal4::ObjectRandomMovementState state;
    state.words.fill(0x5555u);
    state.words[0] = 0x1111u;
    Runtime runtime;
    runtime.random = {-0.0f, 100.0f, 0.0f};
    pal4::ObjectRandomMovementLifecycle::Construct(state, runtime);
    Check(state.words[50] == std::bit_cast<std::uint32_t>(-0.0f) &&
              state.words[52] == 0u,
          "zero horizontal length copies signed zero directly");

    state.words[0] = 0x1111u;
    state.words[55] = 0x5555u;
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    runtime.random = {nan, 8.0f, 2.0f};
    pal4::ObjectRandomMovementLifecycle::Construct(state, runtime);
    Check(std::isnan(std::bit_cast<float>(state.words[50])) &&
              state.words[52] == std::bit_cast<std::uint32_t>(2.0f),
          "unordered horizontal length preserves the source components");
}

void TestConstructorFailureCleanup() {
    pal4::ObjectRandomMovementState state;
    state.words.fill(0x5555u);
    state.words[0] = 0x1111u;
    Runtime runtime;
    runtime.throw_on_random = true;
    bool thrown = false;
    try {
        pal4::ObjectRandomMovementLifecycle::Construct(state, runtime);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && runtime.calls ==
              std::vector<std::string>{"construct", "random", "cleanup"},
          "constructor exception runs base cleanup after random source failure");
}

void TestDeleteGate() {
    pal4::ObjectRandomMovementState state;
    state.words[0] = 0x1111u;
    state.words[55] = 0x5555u;
    Runtime runtime;
    Check(pal4::ObjectRandomMovementLifecycle::Delete(
              state, 0x7000u, 2u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::string>{"cleanup"},
          "delete without bit zero leaves allocation");
    runtime.calls.clear();
    Check(pal4::ObjectRandomMovementLifecycle::Delete(
              state, 0x7000u, 3u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::string>{"cleanup", "free"} &&
              runtime.freed == 0x7000u,
          "delete with bit zero frees after base cleanup");

    pal4::ObjectRandomMovementRuntimeStub stub;
    Check(pal4::ObjectRandomMovementLifecycle::Delete(
              state, 0x7000u, 0u, stub) == 0x7000u,
          "default runtime links");
}

}  // namespace

int main() {
    TestConstructAndNormalize();
    TestZeroAndUnorderedDirection();
    TestConstructorFailureCleanup();
    TestDeleteGate();
    std::cout << "4 random movement lifecycle cases passed\n";
}

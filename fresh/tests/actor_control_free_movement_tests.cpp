#include "pal4/actor_control_free_movement.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
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

void CheckVector(const pal4::ActorControlVec3& actual,
                 float x, float y, float z, const char* message) {
    Check(std::fabs(actual.x - x) < 0.00002f &&
              std::fabs(actual.y - y) < 0.00002f &&
              std::fabs(actual.z - z) < 0.00002f,
          message);
}

pal4::ActorControlVec3 Direction(
    const pal4::ActorControlInputComponentState& state) {
    return {std::bit_cast<float>(state.words[6]),
            std::bit_cast<float>(state.words[7]),
            std::bit_cast<float>(state.words[8])};
}

struct Runtime final : pal4::IActorControlFreeMovementRuntime {
    std::map<std::uint32_t, std::uint32_t> input;
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::uint32_t> reads;
    std::vector<std::string> calls;
    bool mouse_valid = false;
    pal4::ActorControlVec3 mouse_world{13.0f, 99.0f, 34.0f};
    float threshold = 4.0f;
    unsigned camera_lookups = 0u;
    unsigned normalize_calls = 0u;
    unsigned dispatch_calls = 0u;
    pal4::ActorControlVec3 before_normalize{};
    pal4::ActorControlVec3 dispatched{};

    Runtime() {
        memory[0x5004u] = 0x6000u;
        memory[0x7000u] = std::bit_cast<std::uint32_t>(1.0f);
        memory[0x7004u] = std::bit_cast<std::uint32_t>(2.0f);
        memory[0x7008u] = std::bit_cast<std::uint32_t>(3.0f);
        memory[0x7020u] = std::bit_cast<std::uint32_t>(4.0f);
        memory[0x7024u] = std::bit_cast<std::uint32_t>(5.0f);
        memory[0x7028u] = std::bit_cast<std::uint32_t>(6.0f);
        memory[kActor + 0xacu] = std::bit_cast<std::uint32_t>(10.0f);
        memory[kActor + 0xb0u] = std::bit_cast<std::uint32_t>(20.0f);
        memory[kActor + 0xb4u] = std::bit_cast<std::uint32_t>(30.0f);
    }
    std::uint32_t InputManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::uint32_t ReadInputWord(std::uint32_t manager,
                                std::uint32_t offset) override {
        Check(manager == 0x9000u, "input read uses fresh manager");
        reads.push_back(offset);
        return input[offset];
    }
    std::uint32_t CameraSystem() override {
        calls.emplace_back("camera_system");
        return 0x4000u;
    }
    std::uint32_t ActiveCameraObjectPointer(
        std::uint32_t system) override {
        Check(system == 0x4000u, "camera lookup uses system");
        ++camera_lookups;
        calls.emplace_back("camera_object");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(memory.contains(address), "expected camera or actor read");
        return memory[address];
    }
    std::uint32_t ObjectTransform(std::uint32_t object) override {
        Check(object == 0x6000u, "camera object supplies transform object");
        calls.emplace_back("transform");
        return 0x7000u;
    }
    bool MouseWorldCoordinates(
        pal4::ActorControlInputComponentState& state,
        std::uint32_t actor, pal4::ActorControlVec3& output) override {
        Check(actor == kActor && state.words[4] <= 1u,
              "mouse query receives actor and component");
        calls.emplace_back("mouse_world");
        output = mouse_world;
        return mouse_valid;
    }
    float ControllerThreshold() override {
        calls.emplace_back("threshold");
        return threshold;
    }
    double ApproximateMagnitude(
        const pal4::ActorControlVec3& vector) override {
        calls.emplace_back("magnitude");
        CheckVector(vector, 3.0f, 0.0f, 4.0f,
                    "mouse delta uses actor x/z and zero vertical offset");
        return std::sqrt(static_cast<double>(vector.x) * vector.x +
               static_cast<double>(vector.y) * vector.y +
               static_cast<double>(vector.z) * vector.z);
    }
    void NormalizeDirection(
        pal4::ActorControlInputComponentState& state) override {
        ++normalize_calls;
        calls.emplace_back("normalize");
        before_normalize = Direction(state);
        state.words[6] = std::bit_cast<std::uint32_t>(
            before_normalize.x + 0.5f);
    }
    void DispatchVectorMovement(
        pal4::ActorControlInputComponentState& state,
        std::uint32_t actor, float delta_seconds) override {
        Check(actor == kActor &&
                  std::bit_cast<std::uint32_t>(delta_seconds) ==
                      std::bit_cast<std::uint32_t>(0.25f),
              "vector dispatch receives actor and frame delta");
        ++dispatch_calls;
        calls.emplace_back("dispatch");
        dispatched = Direction(state);
        state.words[4] = 7u;
    }
    void Activate(std::uint32_t count_offset,
                  std::uint32_t flag_offset) {
        input[count_offset] = 1u;
        input[flag_offset] = 0x11u;
    }
};

pal4::ActorControlInputComponentState State() {
    pal4::ActorControlInputComponentState state;
    state.words.fill(0xaaaaaaaau);
    state.words[4] = 99u;
    state.words[6] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[7] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[8] = std::bit_cast<std::uint32_t>(4.0f);
    return state;
}

std::uint32_t Run(pal4::ActorControlInputComponentState& state,
                  Runtime& runtime) {
    return pal4::ActorControlInputComponent::UpdateFreeMovement(
        state, kActor, 0.25f, runtime);
}

void TestIdle() {
    auto state = State();
    Runtime runtime;
    Check(Run(state, runtime) == 0u && state.words[4] == 0u &&
              state.words[6] == 0u && state.words[7] == 0u &&
              state.words[8] == 0u && runtime.reads.size() == 9u &&
              runtime.camera_lookups == 0u &&
              runtime.normalize_calls == 0u &&
              runtime.dispatch_calls == 0u,
          "idle path clears status and vector after nine input checks");
}

void TestKeyboardDirections() {
    const struct {
        std::uint32_t count;
        std::uint32_t flags;
        float x;
        float z;
    } cases[] = {
        {0x1f44u, 0x1f38u, 4.0f, 6.0f},
        {0x1e34u, 0x1e28u, 4.0f, 6.0f},
        {0x1f54u, 0x1f48u, -4.0f, -6.0f},
        {0x1df4u, 0x1de8u, -4.0f, -6.0f},
        {0x1f24u, 0x1f18u, 3.0f, 7.0f},
        {0x1cd4u, 0x1cc8u, 3.0f, 7.0f},
        {0x1f34u, 0x1f28u, 1.0f, 1.0f},
        {0x1d04u, 0x1cf8u, 1.0f, 1.0f},
    };
    for (const auto& test : cases) {
        auto state = State();
        Runtime runtime;
        runtime.Activate(test.count, test.flags);
        Check(Run(state, runtime) == 7u &&
                  state.words[4] == 7u &&
                  runtime.camera_lookups == 1u &&
                  runtime.normalize_calls == 1u &&
                  runtime.dispatch_calls == 1u,
              "each key source activates one movement action");
        CheckVector(runtime.before_normalize,
                    test.x, 0.0f, test.z,
                    "direction matches camera row and prior state");
        CheckVector(runtime.dispatched,
                    test.x + 0.5f, 0.0f, test.z,
                    "dispatch sees normalized state mutation");
    }
    auto state = State();
    Runtime runtime;
    runtime.Activate(0x1f44u, 0x1f38u);
    runtime.Activate(0x1f54u, 0x1f48u);
    runtime.Activate(0x1f24u, 0x1f18u);
    runtime.Activate(0x1f34u, 0x1f28u);
    Check(Run(state, runtime) == 7u &&
              runtime.camera_lookups == 4u,
          "simultaneous directions acquire camera on every active branch");
    CheckVector(runtime.before_normalize, -4.0f, 0.0f, -6.0f,
                "backward overrides forward and side directions cancel");
}

void TestMouseThreshold() {
    for (const auto threshold : {
             4.0f, 5.0f, 6.0f,
             std::numeric_limits<float>::quiet_NaN()}) {
        auto state = State();
        Runtime runtime;
        runtime.Activate(0x21d4u, 0x21c8u);
        runtime.mouse_valid = true;
        runtime.threshold = threshold;
        const bool accepted = threshold < 5.0f;
        Check(Run(state, runtime) == (accepted ? 7u : 0u) &&
                  runtime.normalize_calls == (accepted ? 1u : 0u) &&
                  runtime.dispatch_calls == (accepted ? 1u : 0u),
              "mouse distance must strictly exceed threshold");
        if (accepted)
            CheckVector(runtime.before_normalize, 3.0f, 0.0f, 4.0f,
                        "accepted mouse target replaces direction");
        else
            Check(state.words[6] == 0u && state.words[7] == 0u &&
                      state.words[8] == 0u,
                  "rejected mouse target leaves no active direction");
    }
    auto state = State();
    Runtime runtime;
    runtime.Activate(0x1f44u, 0x1f38u);
    runtime.Activate(0x21d4u, 0x21c8u);
    runtime.mouse_valid = true;
    runtime.threshold = 5.0f;
    Check(Run(state, runtime) == 7u,
          "rejected mouse target preserves keyboard action");
    CheckVector(runtime.before_normalize, 4.0f, 0.0f, 6.0f,
                "rejected mouse does not replace keyboard direction");

    state = State();
    runtime = Runtime{};
    runtime.Activate(0x21d4u, 0x21c8u);
    Check(Run(state, runtime) == 0u &&
              runtime.calls.back() == "mouse_world" &&
              runtime.normalize_calls == 0u,
          "failed mouse-world lookup skips threshold and normalization");
}

void TestMovementInputStatus() {
    constexpr std::uint32_t counts[] = {
        0x1f44u, 0x1e34u, 0x1f54u, 0x1df4u, 0x21d4u};
    constexpr std::uint32_t flags[] = {
        0x1f38u, 0x1e28u, 0x1f48u, 0x1de8u, 0x21c8u};
    for (std::size_t i = 0; i < 5u; ++i) {
        auto state = State();
        state.words[9] = 0u;
        Runtime runtime;
        runtime.Activate(counts[i], flags[i]);
        Check(pal4::ActorControlInputComponent::HasMovementInput(
                  state, runtime),
              "each forward, backward or mouse source activates status");
        Check(runtime.reads.size() == i + 2u &&
                  runtime.reads.back() == flags[i],
              "active source stops later manager reads");
    }
    auto state = State();
    state.words[9] = 0u;
    Runtime runtime;
    runtime.input[0x1f44u] = 0xffffffffu;
    runtime.input[0x1f38u] = 0x1u;
    runtime.input[0x1e34u] = 1u;
    runtime.input[0x1e28u] = 0x10u;
    Check(!pal4::ActorControlInputComponent::HasMovementInput(
              state, runtime) && runtime.reads.size() == 6u,
          "signed-negative count and upper flag bits do not activate");
    state.words[9] = 0x80000000u;
    runtime.reads.clear();
    Check(pal4::ActorControlInputComponent::HasMovementInput(
              state, runtime) && runtime.reads.size() == 6u,
          "nonzero pending state activates only after five input checks");
}

}  // namespace

int main() {
    TestIdle();
    TestKeyboardDirections();
    TestMouseThreshold();
    TestMovementInputStatus();

    pal4::ActorControlInputComponentState state;
    state.words[6] = 1u;
    state.words[7] = 2u;
    state.words[8] = 3u;
    pal4::ActorControlFreeMovementRuntimeStub runtime;
    Check(pal4::ActorControlInputComponent::UpdateFreeMovement(
              state, kActor, 0.25f, runtime) == 0u &&
              state.words[6] == 0u && state.words[7] == 0u &&
              state.words[8] == 0u,
          "linkable runtime stub clears idle vector");
}

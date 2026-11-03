#include "pal4/behavior_path_endpoint_movement.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void StoreVector(pal4::BehaviorPathEndpointState& state,
                 std::uint32_t first_word,
                 const pal4::BehaviorVector3& value) {
    for (std::uint32_t i = 0u; i != 3u; ++i)
        state.words[first_word + i] = std::bit_cast<std::uint32_t>(value[i]);
}

struct MovementRuntime final : pal4::IBehaviorPathMovementRuntime {
    pal4::BehaviorPathEndpointState* state = nullptr;
    std::vector<std::string> calls;
    std::vector<pal4::BehaviorPositionResult> positions;
    std::size_t position_index = 0u;
    float radius = 1.0f;
    bool expect_turns = false;

    std::uint32_t ProcessCollision(
        pal4::BehaviorPathEndpointState& target,
        pal4::BehaviorContext3& context,
        std::uint32_t input, float delta) override {
        calls.emplace_back("collision");
        Check(&target == state && input == 0x77u && delta == 0.25f,
              "collision receives movement inputs");
        context = {1u, 2u, 3u};
        return 0x3f400000u;
    }
    void UpdateBase(pal4::BehaviorPathEndpointState& target,
                    std::uint32_t result, float delta) override {
        calls.emplace_back("base_update");
        Check(&target == state && result == 0x3f400000u &&
                  delta == 0.25f,
              "base update receives collision result and delta");
    }
    pal4::BehaviorPositionResult ReadCurrentPosition(
        pal4::BehaviorPathEndpointState& target,
        pal4::BehaviorContext3& context) override {
        calls.emplace_back("position");
        Check(&target == state &&
                  context == pal4::BehaviorContext3{1u, 2u, 3u} &&
                  position_index < positions.size(),
              "position virtual receives shared collision context");
        if (expect_turns && position_index == 1u)
            Check(target.words[48] == 1u,
                  "first endpoint changes direction before second read");
        if (expect_turns && position_index == 2u)
            Check(target.words[48] == 0xffffffffu,
                  "last endpoint overwrites first endpoint direction");
        const auto result = positions[position_index++];
        if (position_index == positions.size()) target.words[42] = 0x9004u;
        return result;
    }
    float ReadPathRadius(std::uint32_t path_address) override {
        calls.emplace_back("radius");
        Check(path_address == 0x4000u,
              "radius is read from current path at plus 0x10");
        return radius;
    }
    std::uint32_t Read32(std::uint32_t address) override {
        calls.emplace_back("target_vtable");
        Check(address == 0x9000u,
              "target vtable captured before final position call");
        return 0xa000u;
    }
    std::uint32_t NotifyTargetSlot04(
        std::uint32_t captured_vtable, std::uint32_t current_target,
        std::uint32_t position_address) override {
        calls.emplace_back("notify");
        Check(captured_vtable == 0xa000u &&
                  current_target == 0x9004u &&
                  position_address == positions.back().address,
              "notification uses captured vtable and current target pointer");
        return 0xbeefu;
    }
};

struct CallbackRuntime final : pal4::IBehaviorPathCallbackRuntime {
    pal4::BehaviorPathEndpointState* state = nullptr;
    std::vector<std::string> calls;
    std::uint32_t binding_result = 0x9010u;
    bool throw_on_bind = false;

    std::uint32_t InvokeCallback(
        std::uint32_t callback, pal4::BehaviorPathEndpointState& target,
        std::uint32_t argument) override {
        calls.emplace_back("callback");
        Check(callback == 0x6000u && &target == state &&
                  argument == 0x88u,
              "raw callback receives helper and argument");
        return 0x1234u;
    }
    std::uint32_t DeleteObject(
        std::uint32_t object, std::uint32_t flags) override {
        calls.emplace_back("delete");
        Check(object == 0x7000u && flags == 1u,
              "non-null object calls virtual slot +0x98 with flag one");
        return 0x4321u;
    }
    void ReleaseAssociation(std::uint32_t object,
                             std::uint32_t flags) override {
        calls.emplace_back("release");
        Check(object == 0x9000u && flags == 1u &&
                  state->words[42] == 0x9000u,
              "old association is released before replacement");
    }
    std::uint32_t BindAssociation(
        std::uint32_t binder,
        pal4::BehaviorPathEndpointState& target) override {
        calls.emplace_back("bind");
        if (throw_on_bind) throw std::runtime_error("injected binder allocation failure");
        Check(binder == 0x8000u && &target == state,
              "binder virtual slot zero receives helper this pointer");
        return binding_result;
    }
};

void TestAbsentPath() {
    pal4::BehaviorPathEndpointState state;
    MovementRuntime runtime;
    runtime.state = &state;
    state.words[46] = 0xaabbccddu;
    state.words[48] = 0x123u;
    Check(pal4::BehaviorPathEndpointMovement::Update(
              state, 0x77u, 0.25f, runtime) == 0u &&
              state.words[46] == 0xaabb00ddu &&
              state.words[48] == 0x123u && runtime.calls.empty(),
          "missing path clears only byte +0xb9 and skips all services");
}

void TestEndpointTurns() {
    pal4::BehaviorPathEndpointState state;
    MovementRuntime runtime;
    runtime.state = &state;
    runtime.expect_turns = true;
    state.words[47] = 0x4000u;
    state.words[42] = 0x9000u;
    state.words[49] = 0xaaaa0100u;
    state.words[46] = 0xaabbccddu;
    state.words[48] = 0x123u;
    StoreVector(state, 50u, {0.0f, 0.0f, 0.0f});
    StoreVector(state, 53u, {10.0f, 0.0f, 0.0f});
    runtime.positions = {
        {0x6000u, {0.5f, 0.0f, 0.0f}},
        {0x6004u, {9.5f, 0.0f, 0.0f}},
        {0x6008u, {7.0f, 0.0f, 0.0f}}};
    Check(pal4::BehaviorPathEndpointMovement::Update(
              state, 0x77u, 0.25f, runtime) == 0xbeefu &&
              runtime.position_index == 3u &&
              state.words[48] == 0xffffffffu &&
              state.words[46] == 0xaabb01ddu &&
              runtime.calls == std::vector<std::string>{
                  "collision", "base_update", "position", "radius",
                  "position", "radius", "target_vtable", "position",
                  "notify"},
          "both endpoint hits flip direction in order and notify target");
}

void TestInactiveAndStrictRadius() {
    pal4::BehaviorPathEndpointState state;
    MovementRuntime runtime;
    runtime.state = &state;
    state.words[47] = 0x4000u;
    state.words[42] = 0x9000u;
    state.words[49] = 0u;
    state.words[48] = 0x123u;
    runtime.positions = {{0x6008u, {0.0f, 0.0f, 0.0f}}};
    Check(pal4::BehaviorPathEndpointMovement::Update(
              state, 0x77u, 0.25f, runtime) == 0xbeefu &&
              state.words[48] == 0x123u &&
              runtime.calls == std::vector<std::string>{
                  "collision", "base_update", "target_vtable",
                  "position", "notify"},
          "clear endpoint flag skips both distance checks");

    runtime.calls.clear();
    runtime.position_index = 0u;
    state.words[42] = 0x9000u;
    state.words[49] = 0x100u;
    StoreVector(state, 50u, {0.0f, 0.0f, 0.0f});
    StoreVector(state, 53u, {10.0f, 0.0f, 0.0f});
    runtime.positions = {
        {0x6000u, {1.0f, 0.0f, 0.0f}},
        {0x6004u, {9.0f, 0.0f, 0.0f}},
        {0x6008u, {7.0f, 0.0f, 0.0f}}};
    Check(pal4::BehaviorPathEndpointMovement::Update(
              state, 0x77u, 0.25f, runtime) == 0xbeefu &&
              state.words[48] == 0x123u,
          "distance equal to radius does not flip direction");
    pal4::BehaviorPathMovementRuntimeStub stub;
    state.words[47] = 0x4000u;
    Check(pal4::BehaviorPathEndpointMovement::Update(
              state, 0u, 0.0f, stub) == 0u,
          "movement update links without collision or GridManager");
}

void TestCallbacks() {
    pal4::BehaviorPathEndpointState state;
    CallbackRuntime runtime;
    runtime.state = &state;
    Check(pal4::BehaviorPathEndpointCallbacks::Call(
              state, 0x88u, runtime) == 0u && runtime.calls.empty(),
          "null callback returns zero");
    state.words[44] = 0x6000u;
    Check(pal4::BehaviorPathEndpointCallbacks::Call(
              state, 0x88u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{"callback"},
          "non-null callback forwards raw return");
    runtime.calls.clear();
    Check(pal4::BehaviorPathEndpointCallbacks::DeleteObject(
              0u, 0xdeadbeefu, runtime) == 0xdeadbeefu &&
              runtime.calls.empty(),
          "null object preserves incoming EAX value");
    Check(pal4::BehaviorPathEndpointCallbacks::DeleteObject(
              0x7000u, 0u, runtime) == 0x4321u &&
              runtime.calls == std::vector<std::string>{"delete"},
          "non-null object delegates deletion with flag one");
    runtime.calls.clear();
    state.words[42] = 0x9000u;
    Check(pal4::BehaviorPathEndpointCallbacks::SetAssociation(
              state, 0x8000u, runtime) == 0x9010u &&
              state.words[42] == 0x9010u &&
              runtime.calls == std::vector<std::string>{
                  "release", "bind"},
          "association releases old target before binding new target");
    runtime.calls.clear();
    state.words[42] = 0u;
    runtime.binding_result = 0u;
    Check(pal4::BehaviorPathEndpointCallbacks::SetAssociation(
              state, 0x8000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"bind"},
          "null old target skips release but still invokes binder");
    runtime.calls.clear();
    state.words[42] = 0x9000u;
    runtime.throw_on_bind = true;
    bool caught = false;
    try { (void)pal4::BehaviorPathEndpointCallbacks::SetAssociation(state, 0x8000u, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && state.words[42] == 0u &&
              runtime.calls == std::vector<std::string>{"release", "bind"},
          "throwing replacement does not retain a freed association for helper teardown");
    pal4::BehaviorPathCallbackRuntimeStub stub;
    Check(pal4::BehaviorPathEndpointCallbacks::Call(
              state, 0u, stub) == 0u,
          "callback methods link without original virtual services");
}

}  // namespace

int main() {
    TestAbsentPath();
    TestEndpointTurns();
    TestInactiveAndStrictRadius();
    TestCallbacks();
}

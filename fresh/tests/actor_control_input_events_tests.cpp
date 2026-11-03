#include "pal4/actor_control_input_events.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kActor = 0x123400u;
constexpr float kDelta = 0.25f;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IActorControlInputEventRuntime {
    std::map<std::uint32_t, std::uint32_t> input;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> commands;
    std::vector<std::string> calls;
    pal4::ActorControlScreenPoint cursor{5, 5};
    pal4::ActorControlScreenRect rect{0, 0, 10, 10};
    std::int32_t mouse_value = 0;
    std::uint32_t manager_calls = 0;

    std::uint32_t InputManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t ReadInputWord(std::uint32_t manager,
                                std::uint32_t offset) override {
        Check(manager == 0x5000u, "read uses freshly obtained manager");
        calls.emplace_back("read");
        reads.push_back(offset);
        return input[offset];
    }
    pal4::ActorControlScreenPoint CursorPoint() override {
        calls.emplace_back("cursor");
        return cursor;
    }
    pal4::ActorControlScreenRect WindowRect() override {
        calls.emplace_back("rect");
        return rect;
    }
    void GetInputStateValues(std::uint32_t manager,
                             std::uint32_t& input_bits,
                             std::uint32_t& scratch_actor) override {
        Check(manager == 0x5000u &&
                  input_bits == std::bit_cast<std::uint32_t>(kDelta) &&
                  scratch_actor == kActor,
              "mouse state receives original local argument words");
        calls.emplace_back("state_values");
        input_bits = std::bit_cast<std::uint32_t>(mouse_value);
        scratch_actor = 0xbeefu;
    }
    void DispatchMovement(pal4::ActorControlInputComponentState& state,
                          std::uint32_t actor, std::uint32_t command,
                          float delta_seconds) override {
        Check(actor == kActor &&
                  std::bit_cast<std::uint32_t>(delta_seconds) ==
                      std::bit_cast<std::uint32_t>(kDelta),
              "movement receives captured actor and delta");
        calls.emplace_back("movement");
        commands.push_back(command);
        state.words[4] = 42u;
    }
    void TurnFromMouse(pal4::ActorControlInputComponentState& state,
                       std::uint32_t actor,
                       std::int32_t input_value) override {
        Check(actor == kActor && input_value == mouse_value,
              "mouse turn receives captured actor and signed input");
        calls.emplace_back("mouse_turn");
        state.words[4] = 42u;
    }
    void Activate(std::uint32_t count_offset,
                  std::uint32_t flag_offset) {
        input[count_offset] = 1u;
        input[flag_offset] = 0x11u;
    }
};

pal4::ActorControlInputComponentState State(
    std::uint32_t pending = 0u) {
    pal4::ActorControlInputComponentState state;
    state.words.fill(0xaaaaaaaau);
    state.words[4] = 99u;
    state.words[9] = pending;
    return state;
}

std::uint32_t Run(pal4::ActorControlInputComponentState& state,
                  Runtime& runtime) {
    return pal4::ActorControlInputComponent::HandleInputEvents(
        state, kActor, kDelta, runtime);
}

bool ReadOffset(const Runtime& runtime, std::uint32_t offset) {
    for (const auto address : runtime.reads)
        if (address == offset) return true;
    return false;
}

void TestIdleAndPending() {
    auto state = State();
    Runtime runtime;
    Check(Run(state, runtime) == 0u && state.words[4] == 0u &&
              state.words[9] == 0u && runtime.commands.empty() &&
              runtime.manager_calls == 10u,
          "idle path clears status and checks all ten input pairs");
    Check(!ReadOffset(runtime, 0x1f38u) &&
              !ReadOffset(runtime, 0x21c8u),
          "nonpositive counts skip corresponding flag reads");

    state = State(1u);
    runtime = Runtime{};
    Check(Run(state, runtime) == 1u &&
              runtime.commands == std::vector<std::uint32_t>{1u} &&
              state.words[4] == 1u && state.words[9] == 1u &&
              runtime.calls.front() == "movement",
          "pending flag dispatches first without clearing itself");
}

void TestForwardAndBackward() {
    for (const auto count : {0x1f44u, 0x1e34u}) {
        auto state = State(1u);
        Runtime runtime;
        runtime.Activate(count, count - 0xcu);
        Check(Run(state, runtime) == 1u &&
                  runtime.commands ==
                      std::vector<std::uint32_t>{1u, 1u} &&
                  state.words[9] == 0u,
              "forward keyboard repeats pending move then clears flag");
        Check(!ReadOffset(runtime, 0x21d4u) &&
                  !ReadOffset(runtime, 0x1f54u),
              "forward keyboard short-circuits mouse and backward checks");
    }
    for (const auto count : {0x1f54u, 0x1df4u}) {
        auto state = State(1u);
        Runtime runtime;
        runtime.Activate(count, count - 0xcu);
        Check(Run(state, runtime) == 1u &&
                  runtime.commands ==
                      std::vector<std::uint32_t>{1u, 2u} &&
                  state.words[9] == 0u,
              "backward keyboard follows pending forward and clears flag");
    }
    auto state = State();
    Runtime runtime;
    runtime.input[0x1f44u] = 0xffffffffu;
    runtime.input[0x1f38u] = 1u;
    runtime.input[0x1e34u] = 1u;
    runtime.input[0x1e28u] = 0x10u;
    Check(Run(state, runtime) == 0u && runtime.commands.empty() &&
              !ReadOffset(runtime, 0x1f38u),
          "negative count and zero low nibble do not activate input");
}

void TestMouseForwardBoundary() {
    for (const auto point : {
             pal4::ActorControlScreenPoint{5, 5},
             pal4::ActorControlScreenPoint{0, 5}}) {
        auto state = State();
        Runtime runtime;
        runtime.Activate(0x21d4u, 0x21c8u);
        runtime.Activate(0x1f54u, 0x1f48u);
        runtime.cursor = point;
        Check(Run(state, runtime) == (point.x == 5 ? 1u : 0u) &&
                  runtime.commands ==
                      (point.x == 5 ? std::vector<std::uint32_t>{1u}
                                    : std::vector<std::uint32_t>{}) &&
                  !ReadOffset(runtime, 0x1f54u),
              "active forward mouse skips backward even at window edge");
        Check(runtime.calls.end() !=
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "cursor") &&
                  runtime.calls.end() !=
                      std::find(runtime.calls.begin(), runtime.calls.end(),
                                "rect"),
              "forward mouse obtains cursor and window rectangle");
    }
}

void TestTurnsAndMouseInput() {
    for (const auto count : {0x1f24u, 0x1cd4u}) {
        auto state = State();
        Runtime runtime;
        runtime.Activate(count, count - 0xcu);
        runtime.Activate(0x1f34u, 0x1f28u);
        Check(Run(state, runtime) == 1u &&
                  runtime.commands == std::vector<std::uint32_t>{3u} &&
                  !ReadOffset(runtime, 0x1f34u),
              "left turn wins over right turn and exits early");
    }
    for (const auto count : {0x1f34u, 0x1d04u}) {
        auto state = State();
        Runtime runtime;
        runtime.Activate(count, count - 0xcu);
        Check(Run(state, runtime) == 1u &&
                  runtime.commands == std::vector<std::uint32_t>{4u} &&
                  !ReadOffset(runtime, 0x21e4u),
              "right turn exits before mouse turn");
    }
    auto state = State();
    Runtime runtime;
    runtime.Activate(0x21e4u, 0x21d8u);
    runtime.mouse_value = -2;
    Check(Run(state, runtime) == 1u && state.words[4] == 1u &&
              runtime.commands.empty() &&
              runtime.manager_calls == 11u &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "state_values" &&
              runtime.calls.back() == "mouse_turn",
          "mouse turn reads updated signed input from a new manager");
    state = State();
    runtime = Runtime{};
    runtime.Activate(0x21e4u, 0x21d8u);
    runtime.cursor = {10, 5};
    Check(Run(state, runtime) == 0u &&
              runtime.commands.empty() &&
              runtime.manager_calls == 10u,
          "mouse turn rejects a cursor on the right edge");
}

}  // namespace

int main() {
    TestIdleAndPending();
    TestForwardAndBackward();
    TestMouseForwardBoundary();
    TestTurnsAndMouseInput();

    pal4::ActorControlInputComponentState state;
    state.words[9] = 1u;
    pal4::ActorControlInputEventRuntimeStub stub;
    Check(pal4::ActorControlInputComponent::HandleInputEvents(
              state, kActor, kDelta, stub) == 1u,
          "linkable stub composes pending movement with camera gate");
}

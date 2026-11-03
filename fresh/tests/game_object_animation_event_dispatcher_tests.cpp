#include "pal4/game_object_animation_event_dispatcher.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

struct Event final {
    float trigger = 0.0f;
    std::uint32_t fired = 0;
};

class Runtime final : public pal4::IGameObjectAnimationEventDispatchRuntime {
public:
    std::vector<Event> events;
    std::vector<std::string> calls;
    std::uint32_t match_listener = 0;

    Event& At(std::uint32_t address) {
        Check(address >= 0x2000u &&
                  (address - 0x2000u) % 0x14cu == 0 &&
                  (address - 0x2000u) / 0x14cu < events.size(),
              "valid 332-byte event address");
        return events[(address - 0x2000u) / 0x14cu];
    }
    std::uint32_t EventBegin(std::uint32_t record) override {
        Check(record == 0x1000u, "reads event begin from current record");
        calls.emplace_back("begin");
        return 0x2000u;
    }
    std::uint32_t EventEnd(std::uint32_t record) override {
        Check(record == 0x1000u, "reads event end from current record");
        calls.emplace_back("end");
        return 0x2000u +
               static_cast<std::uint32_t>(events.size()) * 0x14cu;
    }
    float TriggerTime(std::uint32_t event) override {
        calls.emplace_back("trigger");
        return At(event).trigger;
    }
    std::uint32_t FiredFlag(std::uint32_t event) override {
        calls.emplace_back("fired");
        return At(event).fired;
    }
    void SetFiredFlag(std::uint32_t event, std::uint32_t value) override {
        Check(value == 1, "sets fired flag to one");
        calls.emplace_back("mark");
        At(event).fired = value;
    }
    std::uint32_t ListenerAt(std::uint32_t slot) override {
        Check(slot == 0x3000u || slot == 0x3004u,
              "listener vector uses four-byte pointers");
        calls.emplace_back("listener:" + std::to_string(slot));
        return slot == 0x3000u ? 0xaau : 0xbbu;
    }
    bool MatchesMessage(std::uint32_t listener,
                        std::uint32_t message_name) override {
        Check(message_name >= 0x2080u &&
                  (message_name - 0x2080u) % 0x14cu == 0,
              "message name starts at event +128");
        calls.emplace_back("match:" + std::to_string(listener));
        return listener == match_listener;
    }
    void Dispatch(std::uint32_t listener,
                  pal4::GameObjectEmbeddedAnimationState& state,
                  std::uint32_t event) override {
        Check(event >= 0x2000u &&
                  (listener == 0xccu || state.words[10] == listener),
              "matched selection precedes dispatch");
        calls.emplace_back("dispatch:" + std::to_string(listener));
    }
};

pal4::GameObjectEmbeddedAnimationState ReadyState() {
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[1] = 0x55u;
    state.words[3] = 0x1000u;
    state.words[7] = 0x3000u;
    state.words[8] = 0x3008u;
    state.words[11] = 0xccu;
    return state;
}

void GatesAndTiming() {
    Runtime runtime;
    auto state = ReadyState();
    state.words[1] = 0;
    Check(!pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 1.0f, runtime) && runtime.calls.empty(),
          "missing clump returns zero before event access");
    state.words[1] = 0x55u;
    state.words[3] = 0;
    Check(!pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 1.0f, runtime) && runtime.calls.empty(),
          "missing current record returns zero");
    state.words[3] = 0x1000u;
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 1.0f, runtime) &&
              runtime.calls == std::vector<std::string>{"begin", "end"},
          "empty range returns one");

    runtime.events = {{2.0f, 0}};
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 1.0f, runtime) && runtime.events[0].fired == 0 &&
              runtime.calls == std::vector<std::string>{
                  "begin", "end", "trigger", "end"},
          "event before trigger remains unmarked");
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, std::numeric_limits<float>::quiet_NaN(), runtime) &&
              runtime.events[0].fired == 0,
          "unordered time does not trigger event");
    runtime.events[0].fired = 9;
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 2.0f, runtime) && runtime.events[0].fired == 9 &&
              runtime.calls == std::vector<std::string>{
                  "begin", "end", "trigger", "fired", "end"},
          "already fired event is not dispatched again");
}

void MatchAndFallback() {
    Runtime runtime;
    runtime.events = {{1.0f, 0}, {2.0f, 0}};
    runtime.match_listener = 0xbbu;
    auto state = ReadyState();
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 1.0f, runtime) &&
              runtime.events[0].fired == 1 &&
              runtime.events[1].fired == 0 &&
              state.words[10] == 0xbbu &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "dispatch:187") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "dispatch:204") == runtime.calls.end(),
          "second matching listener is selected without fallback");
    runtime.match_listener = 0;
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 2.0f, runtime) &&
              runtime.events[1].fired == 1 &&
              state.words[10] == 0xbbu &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "dispatch:204") != runtime.calls.end(),
          "unmatched second event reaches fallback and preserves selection");
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 3.0f, runtime) &&
              std::find_if(runtime.calls.begin(), runtime.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("dispatch:");
                           }) == runtime.calls.end(),
          "fired events dispatch only once");

    Runtime no_fallback;
    no_fallback.events = {{0.0f, 0}};
    state = ReadyState();
    state.words[7] = state.words[8];
    state.words[11] = 0;
    Check(pal4::GameObjectAnimationEventDispatcher::UpdateTime(
              state, 0.0f, no_fallback) &&
              no_fallback.events[0].fired == 1 && state.words[10] == 0,
          "empty listener vector with no fallback still marks event");
}

}  // namespace

int main() {
    GatesAndTiming();
    MatchAndFallback();
    std::cout << "game_object_animation_event_dispatcher_tests passed\n";
}

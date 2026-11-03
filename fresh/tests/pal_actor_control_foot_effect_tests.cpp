#include "pal4/pal_actor_control_foot_effect.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlFootEffectRuntime {
    std::vector<std::string> events;
    bool throw_on_update = false;

    std::uint32_t EffectTransform(std::uint32_t linked) override {
        Check(linked == 0x900u, "linked object address");
        events.emplace_back("transform");
        return 0x1000u;
    }
    std::uint32_t CreateEffect(std::string_view name,
                               std::uint32_t transform,
                               std::uint32_t first,
                               std::uint32_t second) override {
        Check(transform == 0x1030u && first == 1u && second == 0u,
              "effect creation arguments");
        events.emplace_back("create " + std::string(name));
        return 0x400u;
    }
    void UpdateEffect(std::uint32_t effect,
                      std::uint32_t actor_position,
                      std::uint8_t option) override {
        Check(effect == 0x400u && actor_position == 0x1a0u &&
                  option == 0u, "effect update arguments");
        events.emplace_back("update");
        if (throw_on_update) throw std::runtime_error("update failed");
    }
    void ReleaseEffect(std::uint32_t effect) override {
        Check(effect == 0x400u, "effect release argument");
        events.emplace_back("release");
    }
};

pal4::PalActorControlState ReadyState() {
    pal4::PalActorControlState state{};
    state.words[0] = 1u;
    state.words[1] = 0x100u;
    state.words[223] = 0x900u;
    state.words[33] = 1u;
    state.words[28] = 1u;
    return state;
}

void TestGates() {
    for (const auto index : {0u, 1u, 223u, 33u, 28u}) {
        auto state = ReadyState();
        Runtime runtime;
        state.words[index] = 0u;
        pal4::PalActorControlFootEffect::Spawn(state, 1u, runtime);
        Check(runtime.events.empty(), "missing prerequisite skips effect");
    }
    auto state = ReadyState();
    state.words[183] = 1u;
    Runtime runtime;
    pal4::PalActorControlFootEffect::Spawn(state, 1u, runtime);
    Check(runtime.events.empty(), "active controller skips foot effect");
    state.words[183] = 0u;
    pal4::PalActorControlFootEffect::Spawn(state, 0u, runtime);
    Check(runtime.events == std::vector<std::string>{"transform"},
          "unknown mode stops after transform lookup");
}

void TestModesAndCleanup() {
    for (const auto [mode, name] : {
             std::pair{1u, "foot_2"}, std::pair{2u, "foot_1"},
             std::pair{3u, "foot_0"}}) {
        auto state = ReadyState();
        Runtime runtime;
        pal4::PalActorControlFootEffect::Spawn(state, mode, runtime);
        Check(runtime.events == std::vector<std::string>{
                  "transform", "create " + std::string(name),
                  "update", "release"},
              "foot effect name and lifecycle order");
    }
    auto state = ReadyState();
    Runtime runtime;
    runtime.throw_on_update = true;
    try {
        pal4::PalActorControlFootEffect::Spawn(state, 3u, runtime);
        Check(false, "update exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>{
                  "transform", "create foot_0", "update", "release"},
              "effect handle released during exception unwind");
    }
}

void TestFloorCodeClassification() {
    const auto state = ReadyState();
    for (const auto [path, name] : {
             std::pair{"a/WF011.bin", "foot_2"},
             std::pair{"WF013", "foot_2"},
             std::pair{"WF015", "foot_2"},
             std::pair{"WF017", "foot_2"},
             std::pair{"WF023", "foot_2"},
             std::pair{"WF041", "foot_1"},
             std::pair{"WF037", "foot_1"},
             std::pair{"WF021", "foot_0"},
             std::pair{"WF039", "foot_0"}}) {
        auto current = state;
        Runtime runtime;
        pal4::PalActorControlFloorEffectClassifier::Classify(
            path, current, runtime);
        Check(runtime.events.size() == 4u &&
                  runtime.events[1] == "create " + std::string(name),
              "floor code maps to the observed foot effect");
    }
    {
        auto current = state;
        Runtime runtime;
        pal4::PalActorControlFloorEffectClassifier::Classify(
            "WF039/WF041/WF011", current, runtime);
        Check(runtime.events[1] == "create foot_2",
              "earlier group wins even when its code appears last");
    }
    for (const auto path : {"WF099", "wf011"}) {
        auto current = state;
        Runtime runtime;
        pal4::PalActorControlFloorEffectClassifier::Classify(
            path, current, runtime);
        Check(runtime.events.empty(),
              "unmatched or differently cased code does not spawn");
    }
    {
        auto current = state;
        Runtime runtime;
        constexpr char path[] = {'x', '\0', 'W', 'F', '0', '1', '1'};
        pal4::PalActorControlFloorEffectClassifier::Classify(
            std::string_view(path, sizeof(path)), current, runtime);
        Check(runtime.events.empty(),
              "CRT substring search stops at the first NUL byte");
    }
}

}  // namespace

int main() {
    TestGates();
    TestModesAndCleanup();
    TestFloorCodeClassification();
}

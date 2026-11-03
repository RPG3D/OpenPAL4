#include "pal4/actor_manager_combat_event.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t facet = actor + 716u;

struct Runtime final : pal4::IActorManagerTurnStateRuntime,
                       pal4::IActorAnimationControlRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint32_t state_result = 4u;
    bool state3 = true;
    float converted_input = 0.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        trace.push_back("write:" + std::to_string(address));
    }
    void SetStateZero(std::uint32_t target) override {
        Check(target == actor, "state-zero actor");
        trace.push_back("state-zero");
    }
    void CountScriptProperties(std::uint32_t script,
                               std::uint32_t target) override {
        Check(script == 0xb000u && target == actor,
              "script property source and actor");
        trace.push_back("count-properties");
    }
    void InvokeActorSlot260(std::uint32_t target,
                            std::uint32_t selector,
                            std::uint32_t argument) override {
        Check(target == actor && selector == 6u && argument == actor,
              "actor slot +260 arguments");
        trace.push_back("slot260");
    }
    void ProcessDynamicList(std::uint32_t target,
                            std::uint32_t mode) override {
        Check(target == actor && mode == 1u,
              "dynamic list mode");
        trace.push_back("dynamic-list");
    }
    bool ObjectCheckState3(std::uint32_t target) override {
        Check(target == actor, "state-three actor");
        trace.push_back("state3");
        return state3;
    }
    void HandleEvent14(std::uint32_t target,
                       std::uint32_t selector,
                       std::uint32_t flag) override {
        Check(target == actor && selector == 3u && flag == 1u,
              "death handler selector and flag");
        trace.push_back("event14");
    }
    std::uint32_t ScriptCombatDead(std::uint32_t target) override {
        Check(target == actor, "combat-dead actor");
        trace.push_back("combat-dead");
        return 0x8000u;
    }
    void RegisterActorEvent(std::uint32_t event) override {
        Check(event == 0x8000u, "death event registration");
        trace.push_back("register-event");
    }
    std::uint32_t InvokeActorSlot152(std::uint32_t target) override {
        Check(target == actor, "actor state slot +152");
        trace.push_back("slot152");
        return state_result;
    }
    std::uint32_t CombatActorDataPtr() override {
        trace.push_back("combat-data");
        return 0xa000u;
    }
    std::uint32_t InvokeChildSlot(std::uint32_t child,
                                  std::uint32_t slot,
                                  std::uint32_t argument) override {
        Check(child == 0x5000u && (slot == 52u || slot == 56u),
              "child pointer and virtual slot");
        trace.push_back("child:" + std::to_string(slot) + ":" +
                        std::to_string(argument));
        return slot + argument;
    }
    std::int64_t FloatToInteger(float value) override {
        converted_input = value;
        return value < 100.0f ? 13 : 255;
    }
    void InvokeActorSlot24(std::uint32_t target,
                           std::uint32_t color) override {
        Check((target == 0x5000u || target == 0x5000u + 1088u) &&
              (color == 0x0dffffffu || color == 0xffffffffu),
              "child alpha color and actor slot +24");
        trace.push_back("alpha-slot");
    }
    std::uint32_t ProcessClumpAtomics(std::uint32_t clump,
                                      std::uint32_t callback) override {
        Check(callback == 0xa000u, "combat data atomic callback");
        trace.push_back("atomics:" + std::to_string(clump));
        return clump + 7u;
    }
    void RenderCamera(std::uint32_t, std::uint32_t) override {}
    void CleanupCamera(std::uint32_t) override {}
};

void HandleDeathAndAlpha() {
    Runtime runtime;
    runtime.words[facet + 116u] = 7u;
    runtime.words[facet - 4u] = 1u;
    runtime.words[facet - 8u] = 1u;
    runtime.words[facet + 84u] = 0xb000u;
    runtime.words[facet + 92u] = 0x5000u;
    runtime.words[0x5000u + 20u] = 0x6000u;
    runtime.words[0x5000u + 1108u] = 0x7000u;
    Check(pal4::ActorManagerCombatEvent::Handle(facet, runtime,
                                                runtime) == 0x7007u &&
          runtime.words[facet + 116u] == 8u &&
          runtime.words[facet - 4u] == 0u &&
          runtime.words[facet - 8u] == 0u &&
          std::abs(runtime.converted_input - 12.75f) < 0.01f &&
          runtime.trace == std::vector<std::string>{
              "write:4928", "write:4808", "write:4804",
              "state-zero", "count-properties", "slot260",
              "dynamic-list", "state3", "event14", "combat-dead",
              "register-event", "slot152", "combat-data", "alpha-slot",
              "atomics:24576", "alpha-slot", "atomics:28672"},
          "death event updates state before applying 0.05 child alpha");
    runtime.state_result = 3u;
    runtime.state3 = false;
    runtime.trace.clear();
    Check(pal4::ActorManagerCombatEvent::Handle(facet, runtime,
                                                runtime) == 0x7007u &&
          runtime.converted_input == 255.0f &&
          runtime.trace[8] == "slot152" &&
          runtime.trace[9] == "combat-data",
          "other state restores full alpha and skips death events");
    runtime.words[facet + 92u] = 0u;
    runtime.trace.clear();
    Check(pal4::ActorManagerCombatEvent::Handle(facet, runtime,
                                                runtime) == 3u &&
          runtime.trace.back() == "slot152",
          "missing child returns state result without alpha processing");
}

void ChildVirtualWrappers() {
    Runtime runtime;
    Check(pal4::ActorManagerCombatEvent::ChildSlot56(
              actor, 9u, runtime) == 0u &&
          pal4::ActorManagerCombatEvent::ChildSlot52(
              actor, 9u, runtime) == 0u && runtime.trace.empty(),
          "missing child normalizes undefined native return to zero");
    runtime.words[actor + 808u] = 0x5000u;
    Check(pal4::ActorManagerCombatEvent::ChildSlot56(
              actor, 9u, runtime) == 65u &&
          pal4::ActorManagerCombatEvent::ChildSlot52(
              actor, 7u, runtime) == 59u &&
          runtime.trace == std::vector<std::string>{
              "child:56:9", "child:52:7"},
          "child wrappers dispatch exact virtual slots and argument");
}

}  // namespace

int main() {
    try {
        HandleDeathAndAlpha();
        ChildVirtualWrappers();
        std::cout << "actor_manager_combat_event_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

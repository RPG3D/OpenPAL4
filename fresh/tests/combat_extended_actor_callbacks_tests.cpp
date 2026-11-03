#include "pal4/combat_extended_actor_callbacks.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatExtendedActorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> actor_states;
    std::size_t state_index = 0u;
    bool ready = false;
    bool inactive = false;

    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == 0x1000u, "facet event address");
        calls.emplace_back("facet");
        return 0x3000u;
    }
    bool ProcessReadyCandidate(std::uint32_t actor) override {
        Check(actor == 0x3000u, "ready candidate actor");
        calls.emplace_back("ready");
        return ready;
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0x4000u;
    }
    bool IsCombatSystemInactive(std::uint32_t system) override {
        Check(system == 0x4000u, "combat system inactivity source");
        calls.emplace_back("inactive");
        return inactive;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) +
                           ":" + std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t ActorState(std::uint32_t actor) override {
        Check(actor == 0x3000u, "actor state source");
        calls.emplace_back("state");
        Check(state_index < actor_states.size(), "actor state read count");
        return actor_states[state_index++];
    }
    void ProcessActor(std::uint32_t actor,
                      std::uint32_t mode) override {
        Check(actor == 0x3000u && mode == 1u,
              "actor process arguments");
        calls.emplace_back("process");
    }
    void FinalizeActor(std::uint32_t actor) override {
        Check(actor == 0x3000u, "actor finalization target");
        calls.emplace_back("finalize");
    }
    std::uint32_t ResetActionComponent(
        std::uint32_t component) override {
        Check(component == 0x3000u + 716u,
              "actor action component address");
        calls.emplace_back("reset");
        return 0x88u;
    }
    std::uint32_t ActorId(std::uint32_t actor) override {
        Check(actor == 0x3000u, "actor id source");
        calls.emplace_back("id");
        return 0x42u;
    }
    std::uint32_t ActorSecondaryId(
        std::uint32_t actor) override {
        Check(actor == 0x3000u, "actor secondary id source");
        calls.emplace_back("secondary");
        return 0x33u;
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x5000u;
    }
    std::uint32_t ProcessActorEvent(
        std::uint32_t router, std::uint32_t descriptor,
        std::uint32_t event, std::uint32_t mode) override {
        Check(router == 0x5000u && descriptor == 0x8b62d4u &&
                  event == 0x1000u && mode == 1u,
              "restart UI event arguments");
        calls.emplace_back("actor-event");
        return 0x77u;
    }
    std::uint32_t ProcessValueEvent(
        std::uint32_t router, std::uint32_t descriptor,
        std::uint32_t value, std::uint32_t mode) override {
        Check(router == 0x5000u && descriptor == 0x8b8b8cu &&
                  value == 0x22u && mode == 1u,
              "combat-over event arguments");
        calls.emplace_back("value-event");
        return 0x99u;
    }
};

void ActorProcessBranches() {
    Runtime runtime;
    Check(pal4::CombatExtendedActorCallbacks::ProcessActorEvent(
              0x2000u, 0x1000u, runtime) == 0x88u &&
              runtime.calls == std::vector<std::string>{
                  "facet", "ready", "facet", "reset"},
          "unready actor only resets action component");

    runtime.calls.clear();
    runtime.ready = true;
    runtime.inactive = true;
    Check(pal4::CombatExtendedActorCallbacks::ProcessActorEvent(
              0x2000u, 0x1000u, runtime) == 0x88u &&
              runtime.words[0x2000u + 64u] == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "facet", "ready", "facet", "write:8256:12288",
                  "system", "inactive", "facet", "finalize",
                  "facet", "reset"},
          "inactive combat records selected actor then finalizes and resets");

    runtime.calls.clear();
    runtime.inactive = false;
    runtime.actor_states = {5u};
    Check(pal4::CombatExtendedActorCallbacks::ProcessActorEvent(
              0x2000u, 0x1000u, runtime) == 0x88u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "state") == 1 &&
              runtime.calls.back() == "reset",
          "actor state five skips second state lookup and dispatch");

    runtime.calls.clear();
    runtime.state_index = 0u;
    runtime.actor_states = {4u, 6u};
    Check(pal4::CombatExtendedActorCallbacks::ProcessActorEvent(
              0x2000u, 0x1000u, runtime) == 0x88u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "state") == 2 &&
              runtime.calls.back() == "reset",
          "actor state six skips restart dispatch");

    runtime.calls.clear();
    runtime.state_index = 0u;
    runtime.actor_states = {4u, 4u};
    Check(pal4::CombatExtendedActorCallbacks::ProcessActorEvent(
              0x2000u, 0x1000u, runtime) == 0x77u &&
              runtime.words[0x4000u + 508u] == 4u &&
              runtime.words[0x2000u + 68u] == 0x3000u &&
              runtime.bytes[0x2000u + 72u] == 1u &&
              runtime.calls[runtime.calls.size() - 2] == "actor-event" &&
              runtime.calls.back() == "byte:8264",
          "active ready actor restarts UI and sets owner latch last");
}

void CombatEventBranches() {
    Runtime runtime;
    runtime.words[0x4000u + 432u] = 0xffffffffu;
    Check(pal4::CombatExtendedActorCallbacks::HandleActorCombatEvent(
              0x1000u, runtime) == 0x88u &&
              runtime.calls == std::vector<std::string>{
                  "system", "read:16816", "facet", "ready",
                  "facet", "reset"},
          "signed negative combat counter uses fallback without actor match");

    runtime.calls.clear();
    runtime.ready = true;
    runtime.words[0x4000u + 432u] = 0u;
    Check(pal4::CombatExtendedActorCallbacks::HandleActorCombatEvent(
              0x1000u, runtime) == 0x88u &&
              runtime.calls == std::vector<std::string>{
                  "system", "read:16816", "facet", "ready",
                  "facet", "finalize", "facet", "reset"},
          "ready fallback finalizes actor before action component reset");

    runtime.calls.clear();
    runtime.words[0x4000u + 432u] = 0x33u;
    runtime.words[0x3000u + 804u] = 2u;
    runtime.words[0x4000u + 436u] = 0x42u;
    runtime.words[0x4000u + 448u] = 0x22u;
    Check(pal4::CombatExtendedActorCallbacks::HandleActorCombatEvent(
              0x1000u, runtime) == 0x99u &&
              runtime.calls == std::vector<std::string>{
                  "system", "read:16816", "facet", "read:13092",
                  "system", "id", "read:16820", "system",
                  "secondary", "read:16816", "system",
                  "read:16832", "router", "value-event"},
          "matching type, primary and secondary ids dispatch combat-over value");

    runtime.calls.clear();
    runtime.words[0x4000u + 436u] = 0x41u;
    Check(pal4::CombatExtendedActorCallbacks::HandleActorCombatEvent(
              0x1000u, runtime) == 0x88u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "secondary") == runtime.calls.end() &&
              runtime.calls.back() == "reset",
          "primary id mismatch short-circuits secondary comparison");
}

}  // namespace

int main() {
    try {
        ActorProcessBranches();
        CombatEventBranches();
        std::cout << "combat extended actor callbacks tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

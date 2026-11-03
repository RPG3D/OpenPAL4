#include "pal4/actor_manager_state_events.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;

struct Runtime final : pal4::IActorManagerStateEventRuntime {
    std::vector<std::string> trace;
    std::vector<std::uint32_t> payload;
    std::uint32_t dispatch_result = 0x1234u;

    void TouchSkillSystem(std::uint32_t subject,
                          std::uint32_t state) override {
        Check(subject == actor && state <= 7u, "skill call state");
        trace.push_back("skill:" + std::to_string(state));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == actor + 828u, "state field offset");
        trace.push_back("state:" + std::to_string(value));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == actor + 800u, "property field offset");
        trace.emplace_back("read-property");
        return 0x7000u;
    }
    std::uint32_t Dispatch(std::string_view event,
                           std::span<const std::uint32_t> arguments,
                           std::uint32_t mode) override {
        Check(mode == 1u, "script dispatch mode");
        payload.assign(arguments.begin(), arguments.end());
        trace.emplace_back(event);
        return dispatch_result;
    }
    std::uint32_t InvokeProperty(std::uint32_t property,
                                 std::uint32_t slot,
                                 std::uint32_t subject) override {
        Check(property == 0x7000u && subject == actor,
              "property receiver and actor");
        trace.push_back("property:" + std::to_string(slot));
        return slot + 100u;
    }
    void ResetObjectProperties(std::uint32_t subject) override {
        Check(subject == actor, "script reset receiver");
        trace.emplace_back("reset-properties");
    }
    void InvokeActorSlot(std::uint32_t subject,
                         std::uint32_t slot) override {
        Check(subject == actor && (slot == 584u || slot == 276u),
              "actor virtual receiver and slot");
        trace.push_back("actor:" + std::to_string(slot));
    }
};

void StateTransitions() {
    Runtime runtime;
    Check(pal4::ActorManagerStateEvents::SetState(
              actor, 0u, false, runtime) == 0u &&
          runtime.trace == std::vector<std::string>{
              "skill:0", "state:0"},
          "silent state zero touches skill system then writes state");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::SetState(
              actor, 7u, true, runtime) == 0x34u &&
          runtime.trace == std::vector<std::string>{
              "skill:7", "state:7", "ae_state_changed"} &&
          runtime.payload == std::vector<std::uint32_t>{actor + 716u},
          "notified state returns dispatch low byte");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::SetState(
              actor, 8u, true, runtime) == 0x34u &&
          runtime.trace == std::vector<std::string>{
              "state:8", "ae_state_changed"},
          "out-of-range state skips skill path but still writes and notifies");
}

void FleeAndDefend() {
    Runtime runtime;
    Check(pal4::ActorManagerStateEvents::BeginFlee(actor, runtime) ==
              0x1234u &&
          runtime.trace == std::vector<std::string>{
              "skill:5", "state:5", "ae_state_changed", "ae_fleeing"},
          "begin flee state and event order");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::Fled(actor, runtime) == 180u &&
          runtime.trace == std::vector<std::string>{
              "skill:6", "state:6", "ae_state_changed", "ae_fleed",
              "read-property", "property:80"},
          "fled returns property slot eighty result");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::FleeFailed(actor, runtime) ==
              0x1234u &&
          runtime.trace == std::vector<std::string>{
              "skill:2", "state:2", "ae_state_changed", "ae_flee_fail"},
          "failed flee restores action state");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::Defend(actor, runtime) == 184u &&
          runtime.trace == std::vector<std::string>{
              "ae_defende", "read-property", "property:84"},
          "defend does not change state");
}

void DeathAndRevival() {
    Runtime runtime;
    Check(pal4::ActorManagerStateEvents::BecomeDead(
              actor, 0x9000u, runtime) == 188u &&
          runtime.trace == std::vector<std::string>{
              "reset-properties", "actor:584", "actor:276", "skill:4",
              "state:4", "ae_state_changed", "ae_become_dead",
              "read-property", "property:88"} &&
          runtime.payload == std::vector<std::uint32_t>{
              actor + 716u, 0x9000u},
          "death reset, state and cause payload ordering");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::BecomeAlive(actor, runtime) ==
              192u &&
          runtime.trace == std::vector<std::string>{
              "skill:1", "state:1", "ae_state_changed",
              "ae_become_alive", "read-property", "property:92"},
          "revival state and property virtual order");
}

void AttackRelations() {
    Runtime runtime;
    Check(pal4::ActorManagerStateEvents::ProtectAction(
              actor, 0x2000u, 0x3000u, runtime) == 0x1234u &&
          runtime.trace == std::vector<std::string>{"ae_protect_action"} &&
          runtime.payload == std::vector<std::uint32_t>{
              0x2000u + 716u, 0x3000u, actor},
          "protect event anchors payload to second actor");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::ProtectAction(
              actor, 0u, 0u, runtime) == 0x1234u &&
          runtime.payload == std::vector<std::uint32_t>{0u, 0u, actor},
          "protect event permits zero arguments");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::JointAttack(
              actor, 0x2000u, runtime) == 0x1234u &&
          runtime.trace == std::vector<std::string>{"ae_joint_attack"} &&
          runtime.payload == std::vector<std::uint32_t>{
              actor + 716u, 0x2000u},
          "joint attack payload");
    runtime.trace.clear();
    Check(pal4::ActorManagerStateEvents::ReactAttack(
              actor, 0u, runtime) == 0x1234u &&
          runtime.trace == std::vector<std::string>{"ae_react_attack"} &&
          runtime.payload == std::vector<std::uint32_t>{
              actor + 716u, 0u},
          "react attack zero argument still dispatches");
}

}  // namespace

int main() {
    try {
        StateTransitions();
        FleeAndDefend();
        DeathAndRevival();
        AttackRelations();
        std::cout << "actor_manager_state_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

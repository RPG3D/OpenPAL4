#include "pal4/actor_manager_facet_action_events.h"

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
constexpr std::uint32_t pending = facet + 448u;
constexpr std::uint32_t combat_threshold = facet + 116u;

struct Runtime final : pal4::IActorManagerPlayerFacetActionEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint8_t check_result = 0u;
    std::int32_t actor_state = 0;
    std::uint32_t pending_after_handle = 0u;
    std::uint32_t dispatch_result = 0x1234u;
    bool combat_inactive = false;
    std::uint32_t slot532_result = 0x12a5u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == pending || address == combat_threshold,
              "facet field read offset");
        trace.emplace_back(address == pending
                               ? "read-pending" : "read-threshold");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == pending && value == 0u,
              "pending field cleared at exact offset");
        words[address] = value;
        trace.emplace_back("clear-pending");
    }
    void HandleStateEvent(std::uint32_t address) override {
        Check(address == facet, "state handler receives facet");
        words[pending] = pending_after_handle;
        trace.emplace_back("handle-state");
    }
    std::uint8_t CheckState3(std::uint32_t address) override {
        Check(address == actor, "state check receives actor");
        trace.emplace_back("check-state");
        return check_result;
    }
    void ProcessActorEvents(std::uint32_t address) override {
        Check(address == actor, "event processing receives actor");
        trace.emplace_back("process-actor");
    }
    bool CombatSystemInactive() override {
        trace.emplace_back("combat-inactive");
        return combat_inactive;
    }
    std::uint32_t ActorSlot532(const std::uint32_t address) override {
        Check(address == actor, "slot 532 receives actor");
        trace.emplace_back("slot-532");
        return slot532_result;
    }
    std::int32_t ActorStateSlot20(std::uint32_t address) override {
        Check(address == actor, "state virtual receives actor");
        trace.emplace_back("slot-20");
        return actor_state;
    }
    void SetProperty2(std::uint32_t address,
                      std::int32_t value) override {
        Check(address == facet && value == 42,
              "property setter receives original facet and value");
        trace.emplace_back("set-property");
    }
    std::uint32_t Dispatch(std::string_view event,
                           std::uint32_t address,
                           std::uint32_t mode) override {
        Check(address == actor, "event payload actor");
        trace.emplace_back("dispatch:" + std::string(event) + ":" +
                           std::to_string(mode));
        return dispatch_result;
    }
};

void CanTakeAction() {
    Runtime ready;
    ready.check_result = 0x7fu;
    ready.pending_after_handle = 1u;
    Check(pal4::ActorManagerFacetActionEvents::CanTakeAction(
              facet, ready) == 0x7fu && ready.words.at(pending) == 1u &&
              ready.trace == std::vector<std::string>{
                  "clear-pending", "handle-state", "check-state"},
          "ready state returns check byte without processing pending events");

    Runtime no_pending;
    no_pending.dispatch_result = 0x12a5u;
    Check(pal4::ActorManagerFacetActionEvents::CanTakeAction(
              facet, no_pending) == 0xa5u &&
              no_pending.trace == std::vector<std::string>{
                  "clear-pending", "handle-state", "check-state",
                  "read-pending", "dispatch:me_can_take_action:1"},
          "unready state dispatches even when no pending event exists");

    Runtime with_pending;
    with_pending.pending_after_handle = 9u;
    Check(pal4::ActorManagerFacetActionEvents::CanTakeAction(
              facet, with_pending) == 0x34u &&
              with_pending.words.at(pending) == 0u &&
              with_pending.trace == std::vector<std::string>{
                  "clear-pending", "handle-state", "check-state",
                  "read-pending", "process-actor", "clear-pending",
                  "dispatch:me_can_take_action:1"},
          "pending actor events are processed and cleared before dispatch");
}

void MovedAtActionSequence() {
    for (const auto state : {-3, -1, 6, 7}) {
        Runtime runtime;
        runtime.actor_state = state;
        Check(pal4::ActorManagerFacetActionEvents::MovedAtActionSequence(
                  facet, 42, runtime) ==
                  static_cast<std::uint32_t>(state) &&
                  runtime.trace == std::vector<std::string>{"slot-20"},
              "out-of-range signed state returns without dispatch");
    }
    for (const auto state : {0, 3, 5}) {
        Runtime runtime;
        runtime.actor_state = state;
        Check(pal4::ActorManagerFacetActionEvents::MovedAtActionSequence(
                  facet, 42, runtime) == 0x1234u &&
                  runtime.trace == std::vector<std::string>{
                      "slot-20", "set-property",
                      "dispatch:me_moved_at_action_sequence:3"},
              "state 0..5 sets the property before event dispatch");
    }
}

void PlayerCanTakeAction() {
    Runtime inactive;
    inactive.words[combat_threshold] = 50u;
    inactive.combat_inactive = true;
    Check(pal4::ActorManagerPlayerFacetActionEvents::CanTakeAction(
              facet, inactive) == 0xa5u &&
              inactive.trace == std::vector<std::string>{
                  "read-threshold", "combat-inactive", "slot-532"},
          "inactive combat at threshold delegates to actor slot 532");

    Runtime active;
    active.words[combat_threshold] = 50u;
    active.dispatch_result = 0x12b6u;
    Check(pal4::ActorManagerPlayerFacetActionEvents::CanTakeAction(
              facet, active) == 0xb6u &&
              active.trace == std::vector<std::string>{
                  "read-threshold", "combat-inactive", "handle-state",
                  "check-state", "dispatch:re_can_take_action:1"},
          "active combat dispatches player readiness event");

    Runtime below;
    below.words[combat_threshold] = 49u;
    below.check_result = 7u;
    Check(pal4::ActorManagerPlayerFacetActionEvents::CanTakeAction(
              facet, below) == 7u &&
              below.trace == std::vector<std::string>{
                  "read-threshold", "handle-state", "check-state"},
          "below threshold skips combat query and returns ready state");
}

void PlayerMovedAtActionSequence() {
    for (const auto state : {-1, 3, 6}) {
        Runtime runtime;
        runtime.actor_state = state;
        Check(pal4::ActorManagerPlayerFacetActionEvents::MovedAtActionSequence(
                  facet, 42, runtime) ==
                  static_cast<std::uint32_t>(state) &&
                  runtime.trace == std::vector<std::string>{"slot-20"},
              "player action state outside 0..2 returns unchanged");
    }
    for (const auto state : {0, 1, 2}) {
        Runtime runtime;
        runtime.actor_state = state;
        Check(pal4::ActorManagerPlayerFacetActionEvents::MovedAtActionSequence(
                  facet, 42, runtime) == 0x1234u &&
                  runtime.trace == std::vector<std::string>{
                      "slot-20", "set-property",
                      "dispatch:re_moved_at_action_sequence:3"},
              "player action state 0..2 sets property before dispatch");
    }
}

}  // namespace

int main() {
    try {
        CanTakeAction();
        MovedAtActionSequence();
        PlayerCanTakeAction();
        PlayerMovedAtActionSequence();
        std::cout << "actor_manager_facet_action_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

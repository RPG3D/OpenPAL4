#include "pal4/combat_game_event_callbacks.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kEvent = 0x2000u;
constexpr std::uint32_t kActor = 0x3000u;
constexpr std::uint32_t kTarget = 0x4000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatGameEventCallbackRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> updates;
    std::uint32_t actor_calls = 0u;

    Runtime() {
        words[kEvent + 4u] = kTarget;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == kEvent, "actor lookup receives the event");
        ++actor_calls;
        return kActor;
    }
    std::uint32_t UpdateStat(std::uint32_t object,
                             std::uint32_t stat,
                             std::uint32_t increment) override {
        Check(object == kObject, "stats update targets event handler");
        updates.emplace_back(stat, increment);
        return 0x9000u + stat;
    }
};

using Handler = std::uint32_t (*)(
    std::uint32_t, std::uint32_t,
    pal4::ICombatGameEventCallbackRuntime&);

void TargetActionCallbacks() {
    constexpr std::array<std::pair<Handler, std::uint32_t>, 5> cases{{
        {&pal4::CombatGameEventCallbacks::OnEvent21, 21u},
        {&pal4::CombatGameEventCallbacks::OnEvent7, 7u},
        {&pal4::CombatGameEventCallbacks::OnEvent6, 6u},
        {&pal4::CombatGameEventCallbacks::OnEvent8, 8u},
        {&pal4::CombatGameEventCallbacks::OnGuard, 2u},
    }};
    for (const auto& [handler, stat] : cases) {
        Runtime runtime;
        runtime.words[kActor + 804u] = 1u;
        runtime.words[kTarget + 804u] = 1u;
        runtime.words[kTarget + 1192u] = 2u;
        const auto slot = kObject + 0x248u + 8u;
        runtime.words[slot] = 9u;
        Check(handler(kObject, kEvent, runtime) == slot &&
                  runtime.words[slot] == 10u &&
                  runtime.updates ==
                      std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                          {stat, 1u}} &&
                  runtime.actor_calls == 1u,
              "player action updates the specified stat and target slot");

        runtime.words[kActor + 804u] = 2u;
        runtime.updates.clear();
        Check(handler(kObject, kEvent, runtime) == slot &&
                  runtime.words[slot] == 11u &&
                  runtime.updates.empty(),
              "target slot can increment without player stat update");

        runtime.words[kTarget + 804u] = 2u;
        Check(handler(kObject, kEvent, runtime) == kTarget &&
                  runtime.words[slot] == 11u,
              "nonplayer target returns the original target pointer");
    }
}

void FleeBranches() {
    Runtime runtime;
    runtime.words[kActor + 804u] = 2u;
    runtime.words[kTarget + 804u] = 1u;
    runtime.words[kTarget + 1192u] = 3u;
    const auto slot = kObject + 0x228u + 12u;
    runtime.words[slot] = 6u;
    Check(pal4::CombatGameEventCallbacks::OnFlee(
              kObject, kEvent, runtime) == 3u &&
              runtime.words[slot] == 7u &&
              runtime.updates.back() == std::pair{18u, 1u},
          "monster flee records stat 18 and a bounded target count");

    runtime.words[kActor + 804u] = 1u;
    runtime.words[kTarget + 1192u] = 4u;
    Check(pal4::CombatGameEventCallbacks::OnFlee(
              kObject, kEvent, runtime) == 4u &&
              runtime.words[slot] == 7u &&
              runtime.updates.back() == std::pair{11u, 1u},
          "other flee records stat 11 and skips out-of-range count");

    runtime.words[kTarget + 804u] = 2u;
    Check(pal4::CombatGameEventCallbacks::OnFlee(
              kObject, kEvent, runtime) == kTarget &&
              runtime.updates.size() == 3u,
          "nonplayer flee target returns its pointer after stat update");
}

void DeathAndActorEvents() {
    Runtime death;
    death.words[kTarget + 804u] = 1u;
    Check(pal4::CombatGameEventCallbacks::OnDeath(
              kObject, kEvent, death) == 0x9004u &&
              death.updates.back() == std::pair{4u, 1u} &&
              death.actor_calls == 0u,
          "death updates stat 4 without facet lookup");
    death.words[kTarget + 804u] = 2u;
    Check(pal4::CombatGameEventCallbacks::OnDeath(
              kObject, kEvent, death) == kEvent &&
              death.updates.size() == 1u,
          "nonplayer death returns event pointer");

    Runtime dodge;
    dodge.words[kTarget + 804u] = 1u;
    Check(pal4::CombatGameEventCallbacks::OnDodge(
              kObject, kEvent, dodge) == 0x9003u &&
              dodge.updates.back() == std::pair{3u, 1u},
          "player dodge records stat 3");
    dodge.words[kTarget + 804u] = 2u;
    Check(pal4::CombatGameEventCallbacks::OnDodge(
              kObject, kEvent, dodge) == kEvent &&
              dodge.updates.size() == 1u,
          "nonplayer dodge returns event pointer");

    constexpr std::array<std::pair<Handler, std::uint32_t>, 6> cases{{
        {&pal4::CombatGameEventCallbacks::OnRoundStart, 1u},
        {&pal4::CombatGameEventCallbacks::OnBattleStart, 0u},
        {&pal4::CombatGameEventCallbacks::OnSpecial, 10u},
        {&pal4::CombatGameEventCallbacks::OnEscape, 17u},
        {&pal4::CombatGameEventCallbacks::OnAttack, 5u},
        {&pal4::CombatGameEventCallbacks::OnItemUse, 9u},
    }};
    for (const auto& [handler, stat] : cases) {
        Runtime runtime;
        const auto matching_state = stat == 17u ? 2u : 1u;
        runtime.words[kActor + 804u] = matching_state;
        Check(handler(kObject, kEvent, runtime) == 0x9000u + stat &&
                  runtime.updates.back() == std::pair{stat, 1u} &&
                  runtime.actor_calls == 1u,
              "actor event updates its stat for the matching state");
        runtime.words[kActor + 804u] = matching_state == 1u ? 2u : 1u;
        Check(handler(kObject, kEvent, runtime) == kActor &&
                  runtime.updates.size() == 1u,
              "nonmatching actor state returns actor pointer");
    }
}

void PlayerDeathCounter() {
    Runtime runtime;
    runtime.words[kActor + 1192u] = 2u;
    const auto slot = kObject + 0x238u + 8u;
    runtime.words[slot] = 4u;
    Check(pal4::CombatGameEventCallbacks::OnPlayerDeath(
              kObject, kEvent, runtime) == 2u &&
              runtime.words[slot] == 5u &&
              runtime.updates.empty() && runtime.actor_calls == 1u,
          "player death increments indexed counter");
    runtime.words[kActor + 1192u] = 4u;
    Check(pal4::CombatGameEventCallbacks::OnPlayerDeath(
              kObject, kEvent, runtime) == 4u &&
              runtime.words[slot] == 5u,
          "out-of-range player death index is returned without write");
}

}  // namespace

int main() {
    try {
        TargetActionCallbacks();
        FleeBranches();
        DeathAndActorEvents();
        PlayerDeathCounter();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

#include "pal4/combat_status_tasks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;

struct Call {
    std::uint32_t actor;
    std::uint32_t slot;
    std::uint32_t argument;
};

struct Runtime final : pal4::ICombatStatusTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> order;
    std::vector<Call> calls;
    std::uint32_t state_one_actor = 0;
    bool replace_actor = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        order.emplace_back("read_word");
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == kTask + 64u, "flee flag offset");
        order.emplace_back("read_byte");
        return bytes[address];
    }
    void CallActor(const std::uint32_t actor,
                   const std::uint32_t slot,
                   const std::uint32_t argument) override {
        calls.push_back({actor, slot, argument});
        order.emplace_back("call");
        if (replace_actor) words[kTask + 60u] = 0x5001u;
    }
    void SetStateOne(const std::uint32_t actor) override {
        state_one_actor = actor;
        order.emplace_back("state_one");
    }
};

void DefendAndFlee() {
    Runtime runtime;
    Check(pal4::CombatStatusTasks::Defend(kTask, runtime) == 1u &&
          runtime.calls.empty() && runtime.state_one_actor == 0u,
          "defend without actor still returns handled");
    runtime.words[kTask + 60u] = 0x5000u;
    runtime.replace_actor = true;
    runtime.order.clear();
    Check(pal4::CombatStatusTasks::Defend(kTask, runtime) == 1u &&
          runtime.calls.back().actor == 0x5000u &&
          runtime.calls.back().slot == 540u &&
          runtime.state_one_actor == 0x5001u &&
          runtime.order == std::vector<std::string>{"read_word", "call",
              "read_word", "state_one"},
          "defend invokes slot 540 then reloads actor for state one");
    runtime.replace_actor = false;
    runtime.words[kTask + 60u] = 0u;
    runtime.calls.clear();
    Check(pal4::CombatStatusTasks::Flee(kTask, runtime) == 0u &&
          runtime.calls.empty(), "flee without actor returns zero");
    runtime.words[kTask + 60u] = 0x5000u;
    Check(pal4::CombatStatusTasks::Flee(kTask, runtime) == 1u &&
          runtime.calls.back().slot == 528u,
          "flee invokes actor slot 528");
}

void FleeTargetAndDead() {
    Runtime runtime;
    Check(pal4::CombatStatusTasks::SetFleeTarget(kTask, runtime) == 0u &&
          runtime.calls.empty(), "flee target requires actor");
    runtime.words[kTask + 60u] = 0x5000u;
    Check(pal4::CombatStatusTasks::SetFleeTarget(kTask, runtime) == 1u &&
          runtime.calls.back().slot == 536u,
          "zero flee flag selects actor slot 536");
    runtime.bytes[kTask + 64u] = 1u;
    Check(pal4::CombatStatusTasks::SetFleeTarget(kTask, runtime) == 1u &&
          runtime.calls.back().slot == 532u,
          "nonzero flee flag selects actor slot 532");

    runtime.words[kTask + 60u] = 0u;
    const auto previous = runtime.calls.size();
    Check(pal4::CombatStatusTasks::Dead(kTask, runtime) == 0u &&
          runtime.calls.size() == previous,
          "dead task requires actor");
    runtime.words[kTask + 60u] = 0x5000u;
    Check(pal4::CombatStatusTasks::Dead(kTask, runtime) == 0u &&
          runtime.calls.size() == previous,
          "dead task requires target");
    runtime.words[kTask + 64u] = 0x6000u;
    Check(pal4::CombatStatusTasks::Dead(kTask, runtime) == 1u &&
          runtime.calls.back().actor == 0x5000u &&
          runtime.calls.back().slot == 544u &&
          runtime.calls.back().argument == 0x6000u,
          "dead task invokes actor slot 544 with target");
}

void FendOff() {
    Runtime runtime;
    Check(pal4::CombatStatusTasks::FendOff(kTask, runtime) == 0u &&
          runtime.calls.empty(), "fend-off requires actor");
    runtime.words[kTask + 60u] = 0x5000u;
    Check(pal4::CombatStatusTasks::FendOff(kTask, runtime) == 0u &&
          runtime.calls.empty(), "fend-off requires target");
    runtime.words[kTask + 64u] = 0x6000u;
    Check(pal4::CombatStatusTasks::FendOff(kTask, runtime) == 1u &&
          runtime.calls.back().actor == 0x5000u &&
          runtime.calls.back().slot == 508u &&
          runtime.calls.back().argument == 0x6000u,
          "fend-off invokes actor slot 508 with target");
}

}  // namespace

int main() {
    try {
        DefendAndFlee();
        FleeTargetAndDead();
        FendOff();
        std::cout << "combat_status_tasks_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

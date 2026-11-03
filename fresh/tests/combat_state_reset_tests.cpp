#include "pal4/combat_state_reset.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatStateResetRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> actors;
    std::unordered_map<std::uint32_t, std::uint32_t> properties;
    std::size_t position = 0u;
    std::uint8_t active = 0u;
    std::uint32_t iterator = 0x6000u;
    std::uint32_t destroy_result = 0x7au;
    std::uint32_t script_result = 0x5bu;
    bool fail_property = false;

    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0x2000u;
    }
    std::uint8_t IsCombatStateActive(std::uint32_t system) override {
        Check(system == 0x2000u, "combat system identity");
        calls.emplace_back("active");
        return active;
    }
    std::uint32_t SkillSystemInstance(
        std::uint32_t mode, const char* source,
        const char* action) override {
        Check(mode == 1u &&
                  std::string_view(source) ==
                      "cbStageDirectCenter::onQuickEnd" &&
                  std::string_view(action) == "QuickEndCombat!!",
              "skill trace arguments");
        calls.emplace_back("skill");
        return 0x2500u;
    }
    void NullSub91(std::uint32_t skill) override {
        Check(skill == 0x2500u, "nullsub skill argument");
        calls.emplace_back("nullsub91");
    }
    std::uint32_t ActorManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    void ResetTaskFlags(std::uint32_t queue) override {
        Check(queue == 0x3000u + 96u, "task flag queue offset");
        calls.emplace_back("reset-flags");
    }
    void CleanupTaskQueue(std::uint32_t queue) override {
        Check(queue == 0x3000u + 96u, "task cleanup queue offset");
        calls.emplace_back("cleanup-tasks");
    }
    std::uint32_t FirstMonsterIterator(
        std::uint32_t manager) override {
        Check(manager == 0x3000u, "monster iterator manager");
        calls.emplace_back("iterator");
        position = 0u;
        return iterator;
    }
    bool IteratorAtEnd(std::uint32_t value) override {
        Check(value == iterator, "iterator end identity");
        calls.emplace_back("end");
        return position >= actors.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t value) override {
        Check(value == iterator && position < actors.size(),
              "iterator current identity");
        calls.emplace_back("current");
        return actors[position];
    }
    void IteratorAdvance(std::uint32_t value) override {
        Check(value == iterator, "iterator advance identity");
        calls.emplace_back("advance");
        ++position;
    }
    std::uint32_t DestroyIterator(std::uint32_t value) override {
        Check(value == iterator, "iterator destroy identity");
        calls.emplace_back("destroy");
        return destroy_result;
    }
    std::uint32_t ActorProperty(std::uint32_t actor,
                                std::uint32_t index) override {
        Check(index == 7u, "actor property index");
        calls.emplace_back("property:" + std::to_string(actor));
        if (fail_property) throw std::runtime_error("property failed");
        return properties[actor];
    }
    void ApplyPropertyDelta(std::uint32_t actor,
                            std::uint32_t index,
                            std::uint32_t delta_bits) override {
        Check(index == 7u && delta_bits == 0u - properties[actor],
              "negated property bits");
        calls.emplace_back("delta:" + std::to_string(actor));
    }
    void ResetActor(std::uint32_t actor,
                    std::uint32_t argument) override {
        Check(actor == argument, "actor self argument");
        calls.emplace_back("reset-actor:" + std::to_string(actor));
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x7000u;
    }
    std::uint32_t ProcessScriptEvent(std::uint32_t router,
                                     std::uint32_t descriptor,
                                     std::uint32_t payload,
                                     std::uint32_t mode) override {
        Check(router == 0x7000u && descriptor == 0x8b8b8cu &&
                  payload == 1u && mode == 1u,
              "combat-over event arguments");
        calls.emplace_back("dispatch");
        return script_result;
    }
};

void ResetBranches() {
    Runtime runtime;
    Check(pal4::CombatStateReset::Reset(0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"system", "active"},
          "inactive combat skips all reset work");

    runtime.calls.clear();
    runtime.active = 1u;
    Check(pal4::CombatStateReset::Reset(0x1000u, runtime) == 0x7au &&
              runtime.calls == std::vector<std::string>{
                  "system", "active", "skill", "nullsub91", "manager",
                  "reset-flags", "manager", "cleanup-tasks", "manager",
                  "iterator", "end", "router", "dispatch", "destroy"},
          "empty monster range still dispatches then returns destructor byte");

    runtime.calls.clear();
    runtime.actors = {0x4000u, 0x5000u};
    runtime.properties[0x4000u] = 5u;
    runtime.properties[0x5000u] = 0xffffffffu;
    Check(pal4::CombatStateReset::Reset(0x1000u, runtime) == 0x7au &&
              runtime.calls == std::vector<std::string>{
                  "system", "active", "skill", "nullsub91", "manager",
                  "reset-flags", "manager", "cleanup-tasks", "manager",
                  "iterator", "end", "current", "property:16384",
                  "delta:16384", "reset-actor:16384", "advance", "end",
                  "current", "property:20480", "delta:20480",
                  "reset-actor:20480", "advance", "end", "router",
                  "dispatch", "destroy"},
          "each monster property is negated and actor reset before dispatch");

    runtime.calls.clear();
    runtime.fail_property = true;
    bool failed = false;
    try {
        (void)pal4::CombatStateReset::Reset(0x1000u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "dispatch") == runtime.calls.end(),
          "iterator is released when monster processing fails");
}

void EmptyStub() {
    pal4::CombatStateReset::NullSub3(0x1234u);
}

}  // namespace

int main() {
    try {
        ResetBranches();
        EmptyStub();
        std::cout << "combat state reset tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

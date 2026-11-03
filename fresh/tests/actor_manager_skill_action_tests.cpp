#include "pal4/actor_manager_skill_action.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t target = 0x2000u;
constexpr std::uint32_t skill = 0x3000u;

struct Runtime final : pal4::IActorManagerSkillActionRuntime {
    std::vector<std::string> trace;
    std::uint32_t found_skill = skill;
    std::uint32_t state = 2u;
    std::uint32_t chance = 0u;
    std::uint32_t complex_result = 0x102u;
    bool random = false;
    bool extended = false;

    std::uint32_t FindSkill(std::uint32_t id) override {
        trace.push_back("find:" + std::to_string(id));
        return found_skill;
    }
    bool CheckRandom(std::uint32_t subject,
                     std::uint32_t index) override {
        Check(subject == actor && index <= 4u,
              "special random actor and index");
        trace.push_back("random:" + std::to_string(index));
        return random;
    }
    std::uint32_t CurrentState(std::uint32_t subject) override {
        Check(subject == actor, "state receiver");
        trace.emplace_back("state");
        return state;
    }
    void InvokeSpecial(std::uint32_t subject,
                       std::uint32_t other,
                       std::uint32_t record,
                       std::uint32_t index) override {
        Check(subject == actor && other == target &&
              record == skill && index <= 4u,
              "special action slot +324 arguments");
        trace.emplace_back("special");
    }
    pal4::ActorSkillActionSnapshot CaptureState(
        std::uint32_t object) override {
        Check(object == actor || object == target,
              "snapshot source");
        trace.push_back(object == target ? "capture-target" :
                                         "capture-actor");
        pal4::ActorSkillActionSnapshot snapshot;
        snapshot.conditions[74] = object == actor ? chance : 99u;
        snapshot.dynamic_values = {1u, 2u};
        snapshot.dynamic_pairs = {3u};
        return snapshot;
    }
    void ProcessSkillChain(
        std::uint32_t record, pal4::ActorSkillActionSnapshot& target_state,
        pal4::ActorSkillActionSnapshot& actor_state) override {
        Check(record == skill && target_state.FollowupChance() == 99u &&
              actor_state.FollowupChance() == chance &&
              target_state.dynamic_values.size() == 2u &&
              actor_state.dynamic_pairs.size() == 1u,
              "skill chain receives both independent copies");
        trace.emplace_back("chain");
    }
    bool CheckRandomExtended(std::uint32_t subject,
                             std::uint32_t threshold) override {
        Check(subject == actor && threshold == chance,
              "extended random threshold from actor snapshot");
        trace.emplace_back("extended");
        return extended;
    }
    void InvokeFollowup(std::uint32_t subject,
                        std::uint32_t other,
                        std::uint32_t record,
                        std::uint32_t threshold) override {
        Check(subject == actor && other == target &&
              record == skill && threshold == chance,
              "followup slot +328 arguments");
        trace.emplace_back("followup");
    }
    std::uint32_t ProcessComplexList(std::uint32_t subject,
                                     std::uint32_t record,
                                     std::uint32_t other) override {
        Check(subject == actor && record == skill && other == target,
              "complex-list arguments");
        trace.emplace_back("complex");
        return complex_result;
    }
};

void MissingAndSpecialSkills() {
    Runtime runtime;
    runtime.found_skill = 0u;
    Check(pal4::ActorManagerSkillAction::Process(
              actor, 17006u, target, runtime) == 0u &&
          runtime.trace == std::vector<std::string>{"find:17006"},
          "missing skill exits before random gate");
    runtime = {};
    runtime.random = true;
    Check(pal4::ActorManagerSkillAction::Process(
              actor, 17010u, target, runtime) == 0u &&
          runtime.trace == std::vector<std::string>{
              "find:17010", "random:4", "special"},
          "random special path skips state and snapshots");
    runtime = {};
    runtime.state = 4u;
    Check(pal4::ActorManagerSkillAction::Process(
              actor, 17006u, target, runtime) == 0u &&
          runtime.trace == std::vector<std::string>{
              "find:17006", "random:0", "state", "special"},
          "state four also invokes special path");
}

void NormalSkillPaths() {
    Runtime runtime;
    Check(pal4::ActorManagerSkillAction::Process(
              actor, 17005u, target, runtime) == 2u &&
          runtime.trace == std::vector<std::string>{
              "find:17005", "capture-target", "capture-actor",
              "chain", "complex"},
          "ordinary skill skips special gate and zero chance skips roll");
    runtime = {};
    runtime.chance = 27u;
    runtime.extended = true;
    Check(pal4::ActorManagerSkillAction::Process(
              actor, 17011u, target, runtime) == 0u &&
          runtime.trace == std::vector<std::string>{
              "find:17011", "capture-target", "capture-actor",
              "chain", "extended", "followup"},
          "extended random hit invokes followup without complex list");
    runtime = {};
    runtime.chance = 27u;
    Check(pal4::ActorManagerSkillAction::Process(
              actor, 17008u, target, runtime) == 2u &&
          runtime.trace == std::vector<std::string>{
              "find:17008", "random:2", "state", "capture-target",
              "capture-actor", "chain", "extended", "complex"},
          "special-range miss falls through to normal skill path");
}

}  // namespace

int main() {
    try {
        MissingAndSpecialSkills();
        NormalSkillPaths();
        std::cout << "actor_manager_skill_action_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

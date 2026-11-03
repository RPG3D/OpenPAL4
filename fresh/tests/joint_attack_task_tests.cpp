#include "pal4/joint_attack_task.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kAttacker = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kIterator = 0x7000u;

struct Runtime final : pal4::IJointAttackRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 60u, kAttacker}, {kTask + 64u, kTarget},
        {kAttacker + 804u, 1u}, {kTarget + 804u, 2u}};
    std::map<std::uint32_t, std::uint32_t> modes;
    std::map<std::uint32_t, bool> valid_states;
    std::map<std::uint32_t, float> properties;
    std::vector<std::uint32_t> members;
    std::vector<std::uint32_t> random_values{500u, 1u};
    std::vector<std::string> calls;
    std::size_t cursor = 0;
    std::size_t random_cursor = 0;
    bool target_state_three = false;
    bool throw_on_event = false;
    std::int32_t target_value = 10;
    float chance = 0.5f;
    float property_threshold = 0.2f;
    std::uint32_t releases = 0;
    std::uint32_t responder = 0;
    std::uint32_t events = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("word");
        return words[address];
    }
    bool StateThree(const std::uint32_t actor) override {
        Check(actor == kTarget, "target state-three argument");
        calls.emplace_back("state3");
        return target_state_three;
    }
    std::int32_t Value7(const std::uint32_t actor) override {
        Check(actor == kTarget, "target value-seven argument");
        calls.emplace_back("value7");
        return target_value;
    }
    float GameDBFloat(const std::uint32_t index) override {
        calls.emplace_back("database");
        Check(index == 15u || index == 16u, "joint attack database index");
        return index == 15u ? chance : property_threshold;
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        Check(random_cursor < random_values.size(), "random call count");
        return random_values[random_cursor++];
    }
    std::uint32_t CreateIterator(const std::uint32_t actor) override {
        Check(actor == kAttacker, "joint candidate collection source");
        calls.emplace_back("create_iterator");
        cursor = 0;
        return kIterator;
    }
    bool IteratorAtEnd(const std::uint32_t iterator) override {
        Check(iterator == kIterator, "iterator end handle");
        calls.emplace_back("end");
        return cursor >= members.size();
    }
    std::uint32_t IteratorCurrent(const std::uint32_t iterator) override {
        Check(iterator == kIterator, "iterator current handle");
        calls.emplace_back("current");
        return members[cursor];
    }
    void IteratorNext(const std::uint32_t iterator) override {
        Check(iterator == kIterator, "iterator next handle");
        calls.emplace_back("next");
        ++cursor;
    }
    void ReleaseIterator(const std::uint32_t iterator) override {
        Check(iterator == kIterator, "iterator release handle");
        calls.emplace_back("release");
        ++releases;
    }
    bool NotStateThree(const std::uint32_t actor) override {
        calls.emplace_back("not_state3");
        return valid_states.count(actor) == 0u || valid_states[actor];
    }
    std::uint32_t ActorMode(const std::uint32_t actor) override {
        calls.emplace_back("mode");
        return modes[actor];
    }
    float ActorProperty(const std::uint32_t address) override {
        calls.emplace_back("property");
        return properties[address];
    }
    void EmitRespond(const std::string_view event_name,
                     const std::uint32_t chosen,
                     const std::uint32_t target,
                     const std::uint32_t mode) override {
        Check(event_name == "joint_attack_respond" && target == kTarget &&
              mode == 1u, "joint response event arguments");
        if (throw_on_event) throw std::runtime_error("event failed");
        responder = chosen;
        ++events;
        calls.emplace_back("respond");
    }
};

void AttackableModes() {
    Runtime runtime;
    for (const auto mode : {0u, 1u, 2u, 3u, 4u}) {
        runtime.modes[0x8000u] = mode;
        Check(pal4::JointAttackTask::IsAttackable(0x8000u, runtime) ==
                  (mode == 0u || mode == 2u || mode == 3u),
              "attackable modes are zero, two and three");
    }
    runtime.valid_states[0x8000u] = false;
    runtime.calls.clear();
    Check(!pal4::JointAttackTask::IsAttackable(0x8000u, runtime) &&
          runtime.calls == std::vector<std::string>{"not_state3"},
          "state-three actor short circuits mode lookup");
}

void EarlyGates() {
    Runtime state;
    state.target_state_three = true;
    Check(pal4::JointAttackTask::Process(kTask, state) == 1u &&
          state.calls == std::vector<std::string>{"word", "state3"},
          "process wrapper returns one despite state-three target");
    Runtime value;
    value.target_value = 0;
    pal4::JointAttackTask::ProcessInternal(kTask, value);
    Check(value.calls == std::vector<std::string>{"word", "state3", "value7"},
          "nonpositive target value exits before faction lookup");
    Runtime same_faction;
    same_faction.words[kTarget + 804u] = 1u;
    pal4::JointAttackTask::ProcessInternal(kTask, same_faction);
    Check(same_faction.random_cursor == 0u && same_faction.releases == 0u,
          "same faction exits before chance roll");
    Runtime wrong_faction;
    wrong_faction.words[kAttacker + 804u] = 3u;
    pal4::JointAttackTask::ProcessInternal(kTask, wrong_faction);
    Check(wrong_faction.random_cursor == 0u,
          "attacker faction must be one");
    Runtime chance;
    chance.random_values[0] = 501u;
    pal4::JointAttackTask::ProcessInternal(kTask, chance);
    Check(chance.random_cursor == 1u && chance.releases == 0u,
          "chance above threshold skips candidate iterator");
}

void CandidateSelection() {
    Runtime runtime;
    runtime.members = {kAttacker, 0x8000u, 0x8100u, 0x8200u, 0x8300u};
    runtime.modes[0x8000u] = 0u;
    runtime.modes[0x8100u] = 1u;
    runtime.modes[0x8200u] = 2u;
    runtime.modes[0x8300u] = 3u;
    runtime.properties[0x8000u + 716u] = 20.0f;
    runtime.properties[0x8200u + 716u] = 16.0f;
    runtime.properties[0x8300u + 716u] = 10.0f;
    Check(pal4::JointAttackTask::Process(kTask, runtime) == 1u &&
          runtime.responder == 0x8200u && runtime.events == 1u &&
          runtime.random_cursor == 2u && runtime.releases == 1u &&
          runtime.calls.back() == "release",
          "chance boundary selects second eligible candidate and cleans up");

    Runtime none;
    none.members = {0x8000u};
    none.modes[0x8000u] = 4u;
    pal4::JointAttackTask::ProcessInternal(kTask, none);
    Check(none.events == 0u && none.random_cursor == 1u &&
          none.releases == 1u,
          "empty eligible collection does not draw a second random value");

    runtime.throw_on_event = true;
    runtime.random_cursor = 0;
    try {
        pal4::JointAttackTask::ProcessInternal(kTask, runtime);
        throw std::runtime_error("missing event exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "event failed" &&
              runtime.releases == 2u,
              "iterator releases on event failure");
    }
}

void RecordedProgressBoundary() {
    // Live301, original CombatFormulaConfig keys16/17 (indices15/16).
    // MT draw207 passes probability .25. Different accumulated frame inputs
    //leave Dream on opposite sides of48; do not compensate by skipping RNG.
    for(const auto progress:{47.984108f,48.0f,49.392605f}) {
        Runtime runtime;
        runtime.chance=.25f;runtime.property_threshold=.75f;
        runtime.random_values={556794152u,2439982424u};
        runtime.members={kAttacker,0x8000u};
        runtime.properties[0x8000u+716u]=progress;
        pal4::JointAttackTask::ProcessInternal(kTask,runtime);
        const bool eligible=progress>48.f;
        Check(runtime.random_cursor==(eligible?2u:1u) && runtime.events==(eligible?1u:0u) &&
              runtime.releases==1u,"recorded joint boundary preserves strict threshold and conditional RNG draw");
    }
}

}  // namespace

int main() {
    try {
        AttackableModes();
        EarlyGates();
        CandidateSelection();
        RecordedProgressBoundary();
        std::cout << "joint_attack_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "pal4/ai_action_task.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kRequested = 0x6000u;
constexpr std::uint32_t kTarget = 0x7000u;
constexpr std::uint32_t kRecord = 0x8000u;
constexpr std::uint32_t kSkill = 0x9000u;

struct Runtime final : pal4::IAiSkillActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 4u, kActor}, {kRecord, kSkill},
        {kRecord + 4u, 11u}, {kRecord + 8u, 12u},
        {kSkill + 8u, 13u}};
    std::uint32_t selected = kTarget;
    pal4::AiPipelineKind expected_kind =
        pal4::AiPipelineKind::Stunt;
    std::uint32_t expected_skill = kRecord;
    std::uint32_t expected_state = 11u;
    std::uint32_t expected_mode = 2u;
    std::string_view expected_source = "selectStuntUsedPipeline";
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) == 1u,
              "action reads only IDA-backed fields");
        calls.emplace_back(address == kTask + 4u ? "actor" : "field");
        return words.at(address);
    }
    std::uint32_t TargetCategory(
        std::uint32_t skill,
        pal4::AiPipelineKind kind) override {
        Check(skill == expected_skill && kind == expected_kind,
              "category resolves from selected skill");
        calls.emplace_back("category");
        return 0xa000u;
    }
    std::uint32_t SelectTarget(
        std::uint32_t task, std::uint32_t category,
        std::uint32_t state, std::uint32_t requested,
        std::uint32_t mode) override {
        Check(task == kTask && category == 0xa000u &&
                  state == expected_state &&
                  requested == kRequested &&
                  mode == expected_mode,
              "selector forwards category, skill state and mode");
        calls.emplace_back("select");
        return selected;
    }
    std::string_view TargetName(std::uint32_t target) override {
        Check(target == kTarget, "name uses selected target");
        calls.emplace_back("name");
        return "target";
    }
    void LogAction(std::uint32_t task,
                   std::string_view source,
                   std::string_view message,
                   std::string_view value) override {
        Check(task == kTask && source == expected_source &&
                  message == "selectTarget:%s" &&
                  value == "target",
              "action uses original log strings");
        calls.emplace_back("log");
    }
    std::uint32_t CreateAction(
        pal4::AiPipelineKind kind, std::uint32_t skill,
        std::uint32_t actor, std::uint32_t target) override {
        Check(kind == expected_kind && skill == expected_skill &&
                  actor == kActor && target == kTarget,
              "combat factory receives skill, actor, target");
        calls.emplace_back("combat");
        return 0xb000u;
    }
};

using Selector = std::uint32_t (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    pal4::IAiSkillActionRuntime&);

void AllKinds() {
    constexpr std::array<Selector, 3> selectors{{
        &pal4::AiSkillAction::SelectStunt,
        &pal4::AiSkillAction::SelectMagic,
        &pal4::AiSkillAction::SelectProperty,
    }};
    constexpr std::array<std::string_view, 3> sources{{
        "selectStuntUsedPipeline", "selectMagicUsedPipeline",
        "selectPropertyUsedPipeline"}};
    for (std::uint32_t index = 0u; index < selectors.size(); ++index) {
        Runtime runtime;
        runtime.expected_kind =
            static_cast<pal4::AiPipelineKind>(index);
        runtime.expected_skill = index == 2u ? kSkill : kRecord;
        runtime.expected_state = 11u + index;
        runtime.expected_mode = 2u + index;
        runtime.expected_source = sources[index];
        Check(selectors[index](kTask, kRecord, kRequested,
                               runtime) == 0xb000u &&
              runtime.calls == (index == 2u
                  ? std::vector<std::string>{
                      "field", "field", "category", "select",
                      "name", "log", "actor", "combat"}
                  : std::vector<std::string>{
                      "field", "category", "select",
                      "name", "log", "actor", "combat"}),
              "stunt, magic and property wrappers preserve offsets and order");
    }
}

void NoTarget() {
    Runtime runtime;
    runtime.selected = 0u;
    Check(pal4::AiSkillAction::SelectStunt(
              kTask, kRecord, kRequested, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "field", "category", "select"},
          "null target skips logging, actor read and combat action");
}

}  // namespace

int main() {
    try {
        AllKinds();
        NoTarget();
        std::cout << "ai_skill_action_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

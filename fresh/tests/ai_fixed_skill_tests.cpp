#include "pal4/ai_action_task.h"

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

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kStunt = 0x7000u;
constexpr std::uint32_t kMagic = 0x8000u;

struct Runtime final : pal4::IAiFixedSkillRuntime {
    bool has_stunt = true;
    bool has_magic = true;
    bool can_stunt = true;
    bool can_magic = true;
    std::uint32_t selected = 0x9000u;
    float id_value = 17.25f;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "actor stored at task +4");
        calls.emplace_back("actor");
        return kActor;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == kTarget + 16u, "skill ID at target +16");
        calls.emplace_back("read-id");
        return id_value;
    }
    std::int32_t ConvertSkillId(float value) override {
        Check(value == id_value, "CRT conversion receives original float");
        calls.emplace_back("convert");
        return 17;
    }
    bool HasSkill(pal4::AiPipelineKind kind,
                  std::int32_t id) override {
        Check(id == 17, "lookup uses converted integer ID");
        const bool stunt = kind == pal4::AiPipelineKind::Stunt;
        Check(stunt || kind == pal4::AiPipelineKind::Magic,
              "fixed skill only probes stunt or magic");
        calls.emplace_back(stunt ? "has-stunt" : "has-magic");
        return stunt ? has_stunt : has_magic;
    }
    bool ActorCapability(std::uint32_t actor,
                         std::uint32_t code) override {
        Check(actor == kActor && (code == 2u || code == 3u),
              "actor skill capability code");
        calls.emplace_back(code == 2u ? "can-stunt" : "can-magic");
        return code == 2u ? can_stunt : can_magic;
    }
    std::uint32_t FindSkill(pal4::AiPipelineKind kind,
                            std::int32_t id) override {
        Check(id == 17, "find uses converted ID");
        const bool stunt = kind == pal4::AiPipelineKind::Stunt;
        calls.emplace_back(stunt ? "find-stunt" : "find-magic");
        return stunt ? kStunt : kMagic;
    }
    std::string_view SkillName(pal4::AiPipelineKind kind,
                               std::uint32_t skill) override {
        Check(skill == (kind == pal4::AiPipelineKind::Stunt
                            ? kStunt : kMagic),
              "name uses resolved skill");
        calls.emplace_back("name");
        return kind == pal4::AiPipelineKind::Stunt ? "stunt" : "magic";
    }
    void LogAction(std::uint32_t task, std::string_view source,
                   std::string_view message,
                   std::string_view value) override {
        Check(task == kTask && source == "cbActorAi::doSummonAction",
              "fixed skill source literal");
        if (value.empty()) {
            Check(message ==
                      "can't find the appropriate stunt/magic/property, doNormalAttackAction",
                  "fallback log literal");
            calls.emplace_back("log-fallback");
        } else {
            Check((message == "selectStuntPipeline, stunt:%s" &&
                   value == "stunt") ||
                      (message == "selectMagicPipeline, magic:%s" &&
                       value == "magic"),
                  "skill selection log literal");
            calls.emplace_back("log-skill");
        }
    }
    std::uint32_t SelectSkill(pal4::AiPipelineKind kind,
                              std::uint32_t task,
                              std::uint32_t skill,
                              std::uint32_t target) override {
        Check(task == kTask && target == kTarget &&
                  skill == (kind == pal4::AiPipelineKind::Stunt
                                ? kStunt : kMagic),
              "selector receives chosen skill and target");
        calls.emplace_back("select");
        return selected;
    }
    std::uint32_t FallbackNormal(std::uint32_t task,
                                 std::uint32_t target) override {
        Check(task == kTask && target == kTarget,
              "fallback preserves input target");
        calls.emplace_back("fallback");
        return 0xa000u;
    }
};

void Selection() {
    Runtime runtime;
    Check(pal4::AiFixedSkillAction::Select(
              kTask, kTarget, runtime) == 0x9000u &&
          runtime.calls == std::vector<std::string>{
              "read-id", "convert", "has-stunt", "actor",
              "can-stunt", "find-stunt", "name", "log-skill",
              "select"},
          "existing stunt wins without probing magic");

    runtime = Runtime{};
    runtime.has_stunt = false;
    Check(pal4::AiFixedSkillAction::Select(
              kTask, kTarget, runtime) == 0x9000u &&
          runtime.calls == std::vector<std::string>{
              "read-id", "convert", "has-stunt", "has-magic",
              "actor", "can-magic", "find-magic", "name",
              "log-skill", "select"},
          "magic is considered only when stunt ID is absent");
}

void Fallbacks() {
    Runtime runtime;
    runtime.has_stunt = false;
    runtime.has_magic = false;
    Check(pal4::AiFixedSkillAction::Select(
              kTask, kTarget, runtime) == 0xa000u &&
          runtime.calls == std::vector<std::string>{
              "read-id", "convert", "has-stunt", "has-magic",
              "log-fallback", "fallback"},
          "unknown ID falls back before actor capability query");

    runtime = Runtime{};
    runtime.can_stunt = false;
    Check(pal4::AiFixedSkillAction::Select(
              kTask, kTarget, runtime) == 0xa000u &&
          runtime.calls == std::vector<std::string>{
              "read-id", "convert", "has-stunt", "actor",
              "can-stunt", "log-fallback", "fallback"},
          "disabled stunt does not try magic with same ID");

    runtime = Runtime{};
    runtime.has_stunt = false;
    runtime.can_magic = false;
    Check(pal4::AiFixedSkillAction::Select(
              kTask, kTarget, runtime) == 0xa000u &&
          runtime.calls.back() == "fallback" &&
          runtime.calls.size() == 8u,
          "disabled magic falls back");

    runtime = Runtime{};
    runtime.selected = 0u;
    Check(pal4::AiFixedSkillAction::Select(
              kTask, kTarget, runtime) == 0xa000u &&
          runtime.calls[runtime.calls.size() - 2u] ==
              "log-fallback" &&
          runtime.calls.back() == "fallback",
          "failed candidate falls back after skill log");
}

}  // namespace

int main() {
    try {
        Selection();
        Fallbacks();
        std::cout << "ai_fixed_skill_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

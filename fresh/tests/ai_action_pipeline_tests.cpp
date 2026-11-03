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
constexpr std::uint32_t kTarget = 0x6000u;

struct Log {
    std::string source;
    std::string message;
    std::string value;
};

struct Runtime final : pal4::IAiActionPipelineRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 4u, kActor}};
    std::array<bool, 3> capabilities{true, true, true};
    std::array<std::uint32_t, 3> pipelines{0x7000u, 0x7100u,
                                           0x7200u};
    std::array<std::uint32_t, 3> candidates{0x8000u, 0x8100u,
                                            0x8200u};
    std::vector<std::uint32_t> checked_capabilities;
    std::vector<std::uint32_t> lookup_slots;
    std::vector<std::uint32_t> lookup_modes;
    std::vector<pal4::AiPipelineKind> selected_kinds;
    std::vector<std::uint32_t> released;
    std::vector<Log> logs;
    bool first_property = true;
    std::uint32_t random = 5u;
    int fallback_calls = 0;
    bool throw_on_final_log = false;
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "pipeline reads actor at task +4");
        return words[address];
    }
    bool ActorCapability(std::uint32_t actor,
                         std::uint32_t code) override {
        Check(actor == kActor && code >= 2u && code <= 4u,
              "capability probes actor modes 2 through 4");
        checked_capabilities.push_back(code);
        return capabilities[code - 2u];
    }
    std::uint32_t LookupPipeline(std::uint32_t task,
                                 std::uint32_t slot,
                                 std::uint32_t mode) override {
        Check(task == kTask, "pipeline lookup receives AI task");
        lookup_slots.push_back(slot);
        lookup_modes.push_back(mode);
        return pipelines[slot == 44u ? 0u : slot == 40u ? 1u : 2u];
    }
    bool HasFirstProperty(std::uint32_t pipeline) override {
        Check(pipeline == 0x7200u,
              "property logging probes first collection entry");
        return first_property;
    }
    std::string_view PipelineName(
        std::uint32_t pipeline,
        pal4::AiPipelineKind kind) override {
        Check(pipeline != 0u, "name query uses selected pipeline");
        if (kind == pal4::AiPipelineKind::Stunt) return "stunt";
        if (kind == pal4::AiPipelineKind::Magic) return "magic";
        return "property";
    }
    void LogPipeline(std::uint32_t task,
                     std::string_view source,
                     std::string_view message,
                     std::string_view value) override {
        Check(task == kTask, "pipeline log receives AI task");
        logs.push_back({std::string(source), std::string(message),
                        std::string(value)});
        if (throw_on_final_log && message == "fint pipeline:%s")
            throw std::runtime_error("final log failure");
    }
    std::uint32_t SelectCandidate(
        std::uint32_t task, pal4::AiPipelineKind kind,
        std::uint32_t pipeline,
        std::uint32_t selected_target) override {
        Check(task == kTask && selected_target == kTarget &&
              pipeline != 0u,
              "candidate selection receives task, pipeline and target");
        selected_kinds.push_back(kind);
        return candidates[static_cast<std::size_t>(kind)];
    }
    std::uint32_t RandomNumber() override { return random; }
    void ReleaseCandidate(std::uint32_t candidate) override {
        released.push_back(candidate);
    }
    std::string_view CandidateName(std::uint32_t candidate) override {
        Check(candidate != 0u, "final label uses retained candidate");
        return candidate == 0x8200u ? "selected property" : "selected";
    }
    std::uint32_t FallbackNormal(
        std::uint32_t task,
        std::uint32_t selected_target) override {
        Check(task == kTask && selected_target == kTarget,
              "fallback receives task and selected target");
        ++fallback_calls;
        return 0x9000u;
    }
};

void SelectionModesAndOwnership() {
    using Selector = std::uint32_t (*)(
        std::uint32_t, std::uint32_t,
        pal4::IAiActionPipelineRuntime&);
    constexpr std::array<Selector, 3> selectors{{
        &pal4::AiActionPipeline::SelectCure,
        &pal4::AiActionPipeline::SelectMagicAttack,
        &pal4::AiActionPipeline::SelectOther,
    }};
    constexpr std::array<std::uint32_t, 3> modes{2u, 0u, 1u};
    constexpr std::array<std::string_view, 3> sources{
        "cbActorAi::doCureAction", "cbActorAi::doMagicAttackAction",
        "cbActorAi::doOtherAction"};
    for (std::size_t index = 0u; index < selectors.size(); ++index) {
        Runtime runtime;
        Check(selectors[index](kTask, kTarget, runtime) == 0x8200u &&
              runtime.checked_capabilities ==
                  std::vector<std::uint32_t>{2u, 3u, 4u} &&
              runtime.lookup_slots ==
                  std::vector<std::uint32_t>{44u, 40u, 48u} &&
              runtime.lookup_modes ==
                  std::vector<std::uint32_t>{modes[index], modes[index],
                                              modes[index]} &&
              runtime.selected_kinds ==
                  std::vector<pal4::AiPipelineKind>{
                      pal4::AiPipelineKind::Stunt,
                      pal4::AiPipelineKind::Magic,
                      pal4::AiPipelineKind::Property} &&
              runtime.released ==
                  std::vector<std::uint32_t>{0x8000u, 0x8100u} &&
              runtime.fallback_calls == 0 &&
              runtime.logs.size() == 4u &&
              runtime.logs[0].source == sources[index] &&
              runtime.logs[0].message ==
                  "selectStuntPipeline, stunt:%s" &&
              runtime.logs[1].message ==
                  "selectMagicPipeline, magic:%s" &&
              runtime.logs[2].message ==
                  "selectPropertyPipeline, Property:%s" &&
              runtime.logs[3].message == "fint pipeline:%s" &&
              runtime.logs[3].value == "selected property",
              "three action modes pick random candidate and delete others");
    }
}

void GatesAndFallback() {
    Runtime runtime;
    runtime.capabilities = {false, true, false};
    runtime.candidates[1] = 0x8100u;
    Check(pal4::AiActionPipeline::SelectCure(
              kTask, kTarget, runtime) == 0x8100u &&
          runtime.lookup_slots == std::vector<std::uint32_t>{40u} &&
          runtime.released.empty() && runtime.logs.size() == 2u,
          "disabled capabilities skip pipeline lookup");
    runtime = Runtime{};
    runtime.pipelines[0] = 0u;
    runtime.first_property = false;
    runtime.candidates[1] = 0u;
    Check(pal4::AiActionPipeline::SelectOther(
              kTask, kTarget, runtime) == 0x8200u &&
          runtime.logs.size() == 2u &&
          runtime.logs[0].message ==
              "selectMagicPipeline, magic:%s" &&
          runtime.logs[1].message == "fint pipeline:%s" &&
          runtime.selected_kinds.size() == 2u,
          "null stunt pipeline and empty property list suppress their labels");
    runtime = Runtime{};
    runtime.candidates = {0u, 0u, 0u};
    Check(pal4::AiActionPipeline::SelectMagicAttack(
              kTask, kTarget, runtime) == 0x9000u &&
          runtime.fallback_calls == 1 &&
          runtime.released.empty() &&
          runtime.logs.back().message ==
              "can't find the appropriate stunt/magic/property, doNormalAttackAction",
          "no candidate falls back to normal attack");
    runtime = Runtime{};
    runtime.throw_on_final_log = true;
    bool threw = false;
    try {
        (void)pal4::AiActionPipeline::SelectCure(
            kTask, kTarget, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.released ==
              std::vector<std::uint32_t>{0x8000u, 0x8100u,
                                          0x8200u},
          "exception after candidate selection releases retained object");
}

void SummonAndReviveModes() {
    using Selector = std::uint32_t (*)(
        std::uint32_t, std::uint32_t,
        pal4::IAiActionPipelineRuntime&);
    constexpr std::array<Selector, 2> selectors{{
        &pal4::AiActionPipeline::SelectSummon,
        &pal4::AiActionPipeline::SelectReviveMate,
    }};
    for (std::uint32_t mode = 0u; mode < selectors.size(); ++mode) {
        Runtime runtime;
        runtime.random = 3u;
        Check(selectors[mode](kTask, kTarget, runtime) == 0x8100u &&
              runtime.checked_capabilities ==
                  std::vector<std::uint32_t>{2u, 3u} &&
              runtime.lookup_slots ==
                  std::vector<std::uint32_t>{44u, 40u} &&
              runtime.lookup_modes ==
                  std::vector<std::uint32_t>{mode + 4u, mode + 4u} &&
              runtime.released ==
                  std::vector<std::uint32_t>{0x8000u} &&
              runtime.logs.size() == 3u &&
              runtime.logs[0].source ==
                  "cbActorAi::doSummonAction" &&
              runtime.logs.back().message == "fint pipeline:%s",
              "summon and revive scan only stunt and magic with source literal");
        runtime = Runtime{};
        runtime.capabilities = {false, false, true};
        Check(selectors[mode](kTask, kTarget, runtime) == 0x9000u &&
              runtime.lookup_slots.empty() &&
              runtime.fallback_calls == 1 &&
              runtime.logs.size() == 1u,
              "summon and revive fall back without qualifying candidates");
    }
}

}  // namespace

int main() {
    try {
        SelectionModesAndOwnership();
        GatesAndFallback();
        SummonAndReviveModes();
        std::cout << "ai_action_pipeline_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
